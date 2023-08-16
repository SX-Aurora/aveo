/**
 * @file AsyncTransfer.cpp
 * @brief implementation of asynchronous memory transfer
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *
 *
 * VEO async memory transfers implementation.
 * 
 * Copyright (c) 2018-2021 NEC Corporation
 * Copyright (c) 2020-2021 Erich Focht
 */

#include "ProcHandle.hpp"
#include "Context.hpp"
#include "CommandImpl.hpp"
#include "log.h"
#include "veo_urpc.h"
#include <sys/types.h>
#include <unistd.h>

namespace veo {
/**
 * @brief Asynchronous SENDBUFF call
 *
 * @param dst remote destination target address for buffer
 * @param src local src address for buffer data
 * @param size size of transfer
 * @param prev previous request in chain, VEO_REQUEST_ID_INVALID
 *             if there is no previous req this one depends on
 * @return request ID
 */
uint64_t
Context::sendBuffAsync(uint64_t dst, void *src, size_t size, uint64_t prev)
{
  VEO_TRACE("enter...");
  if (!this->is_alive())
    return VEO_REQUEST_ID_INVALID;

  auto id = this->issueRequestID();
  //
  // submit function, called when cmd is issued to URPC
  //
  auto f = [this, src, dst, size, id] (Command *cmd)
           {
             VEO_TRACE("sendbuffAsync [request #%d] start...", id);
             //VEO_DEBUG("sendbuff src=%p dst=%p, size=%lu", src, (void *)dst, size);
             int req = urpc_generic_send(this->up, URPC_CMD_SENDBUFF, (char *)"LP",
                                         dst, src, size);
             if (req >= 0) {
               cmd->setURPCReq(req, VEO_COMMAND_UNFINISHED);
             } else if (req == -EAGAIN) {
               VEO_TRACE("[request #%d] error return...", id);
               return -EAGAIN;
             } else {
               // TODO: anything more meaningful into result?
               cmd->setResult(0, VEO_COMMAND_ERROR);
               VEO_TRACE("[request #%d] error return...", id);
               return -1;
             }
             return 0;
           };

  //
  // result function, called when response has arrived from URPC
  //
  auto u = [this, id, prev] (Command *cmd, urpc_mb_t *m, void *payload, size_t plen)
           {
             //VEO_TRACE("[request #%d] reply ACK (cmd=%d)...", id, m->c.cmd);
             uint64_t result = 0;
	     int status = VEO_COMMAND_OK;
             if (prev != VEO_REQUEST_ID_INVALID) {
               auto rc = this->_peekResult(prev, &result);
               if (rc != VEO_COMMAND_OK) {
                 VEO_ERROR("request #%ld in chain has unexpected status %d",
                           prev, rc);
                 // TODO: handle this
		 status = rc;
               }
             }
             if (m->c.cmd == URPC_CMD_EXCEPTION) {
               cmd->setResult(-URPC_CMD_SENDBUFF, VEO_COMMAND_EXCEPTION);
               return (int)-URPC_CMD_SENDBUFF;
             }
             cmd->setResult(result, status);
             return 0;
           };

  CmdPtr cmd(new internal::CommandImpl(id, f, u));
  {
    std::lock_guard<std::recursive_mutex> lock(this->submit_mtx);
    if(this->comq.pushRequest(std::move(cmd)))
      return VEO_REQUEST_ID_INVALID;
  }
  return id;
}

/**
 * @brief Asynchronous RECVBUFF call
 *
 * @param dst local destination address for buffer data
 * @param src remote source address for buffer
 * @param size size of transfer
 * @param prev previous request in chain, VEO_REQUEST_ID_INVALID
 *             if there is no previous req this one depends on
 * @return request ID
 */
uint64_t
Context::recvBuffAsync(void *dst, uint64_t src, size_t size, uint64_t prev)
{
  VEO_TRACE("recvbuffAsync enter...");
  if (!this->is_alive())
    return VEO_REQUEST_ID_INVALID;

  auto id = this->issueRequestID();
  //
  // submit function, called when cmd is issued to URPC
  //
  auto f = [this, src, dst, size, id] (Command *cmd)
           {
             VEO_TRACE("recvbuffAsync [request #%d] start...", id);
             //VEO_DEBUG("recvbuff src=%p dst=%p, size=%lu", (void *)src, dst, size);
             int req = urpc_generic_send(this->up, URPC_CMD_RECVBUFF, (char *)"LLL",
                                         src, (uint64_t)dst, size);
             if (req >= 0) {
               cmd->setURPCReq(req, VEO_COMMAND_UNFINISHED);
             } else if (req == -EAGAIN) {
               VEO_TRACE("[request #%d] error return...", id);
               return -EAGAIN;
             } else {
               // TODO: anything more meaningful into result?
               cmd->setResult(0, VEO_COMMAND_ERROR);
	       VEO_TRACE("[request #%d] error return...", id);
	       return -1;
	     }
             return 0;
           };

  //
  // result function, called when response has arrived from URPC
  //
  auto u = [this, src, dst, size, id, prev] (Command *cmd, urpc_mb_t *m,
                                             void *payload, size_t plen)
           {
             //VEO_DEBUG("[request #%d] reply received (cmd=%d)...", id, m->c.cmd);
             uint64_t sent_dst;
             void *buff;
             size_t buffsz;

             urpc_unpack_payload(payload, plen, (char *)"LP", &sent_dst, &buff, &buffsz);
             if ((uint64_t)dst != sent_dst) {
               VEO_ERROR("mismatch: dst=%lx sent_dst=%lx", (uint64_t)dst, sent_dst);
               printf("debug with : gdb -p %d\n", getpid());
               sleep(60);
               cmd->setResult(-URPC_CMD_RECVBUFF, VEO_COMMAND_EXCEPTION);
               return -1;
             }
             if (size != buffsz) {
               VEO_ERROR("mismatch: size=%lu sent_size=%lu", size, buffsz);
               cmd->setResult(-URPC_CMD_RECVBUFF, VEO_COMMAND_EXCEPTION);
               return -1;
             }
             memcpy((void *)dst, buff, buffsz);
             uint64_t result = 0;
             int status = VEO_COMMAND_OK;
             if (prev != VEO_REQUEST_ID_INVALID) {
               auto rc = this->_peekResult(prev, &result);
               if (rc != VEO_COMMAND_OK) {
                 VEO_ERROR("request #%ld in chain has unexpected status %d",
                           prev, rc);
                 // TODO: handle this
                 status = rc;
               }
             }
             cmd->setResult(result, status);
             return 0;
           };

  CmdPtr cmd(new internal::CommandImpl(id, f, u));
  {
    std::lock_guard<std::recursive_mutex> lock(this->submit_mtx);
    if(this->comq.pushRequest(std::move(cmd)))
      return VEO_REQUEST_ID_INVALID;
  }
  return id;
}

/**
 * @brief asynchronously read data from VE memory
 *
 * @param[out] dst buffer to store data
 * @param src VEMVA to read
 * @param size size to transfer in byte
 * @param sub This function is called as a sub-part of a request
 * @return request ID
 */
uint64_t Context::asyncReadMem(void *dst, uint64_t src, size_t size)
{
  VEO_TRACE("asyncReadMem enter...");
  if(!this->is_alive())
    return VEO_REQUEST_ID_INVALID;

  if (size == 0) {
    auto id = this->issueRequestID();
    auto f = [id] (Command *cmd)
             {
               VEO_TRACE("[request #%d] start...", id);
               cmd->setResult(0, VEO_COMMAND_OK);
               return 0;
             };
    CmdPtr req(new internal::CommandImpl(id, f));
    {
      std::lock_guard<std::recursive_mutex> lock(this->submit_mtx);
      if(this->comq.pushRequest(std::move(req)))
        return VEO_REQUEST_ID_INVALID;
    }
    this->comq.notifyAll();
    VEO_TRACE("asyncWriteMem leave...\n");
    return id;
  }

  const char* env_p = std::getenv("VEO_RECVFRAG");
  const char* cut_p = std::getenv("VEO_RECVCUT");
  size_t maxfrag = PART_SENDFRAG;
  size_t cutsz = 2 * 1024 * 1024;

  if (urpc_max_send_cmd_size(this->up) < 2 * maxfrag)
    maxfrag = 1 * 1024 * 1024;

  if (env_p)
    maxfrag = atoi(env_p);
  if (cut_p)
    cutsz = atoi(cut_p);
  if (size < 5 * maxfrag)
    if (size > cutsz)
      maxfrag = ALIGN8B(size / 5);
    else if (size > cutsz / 2)
      maxfrag = ALIGN8B(size / 4);
    else if (size > cutsz / 3)
      maxfrag = ALIGN8B(size / 2);
    else
      maxfrag = ALIGN8B(size);

  size_t psz;
  size_t rsize = size;
  char *s = (char *)src;
  char *d = (char *)dst;
  uint64_t prev = VEO_REQUEST_ID_INVALID;
  bool flg = false;
  while (rsize > 0) {
    psz = rsize <= maxfrag ? rsize : maxfrag;
    auto req = recvBuffAsync((void *)d, (uint64_t)s, psz, prev);
    if (req == VEO_REQUEST_ID_INVALID) {
      VEO_ERROR("req chain submission failed! Aborting.");
      // TODO: abort chain? How?
      return req;
    }
    prev = req;
    rsize -= psz;
    s += psz;
    d += psz;
    if (flg == false) {
      this->progress();
      flg = true;
    }
  }
  this->comq.notifyAll();
  return prev;
}

/**
 * @brief asynchronously write data to VE memory
 *
 * @param dst VEMVA destination address
 * @param src VH buffer source address
 * @param size size to transfer in byte
 * @param sub This function is called as a sub-part of a request
 * @return request ID
 */
uint64_t Context::asyncWriteMem(uint64_t dst, const void *src,
				size_t size)
{
  VEO_TRACE("src=%p dst=%p size=%lu", src, (void *)dst, size);
  if(!this->is_alive())
    return VEO_REQUEST_ID_INVALID;

  if (size == 0) {
    auto id = this->issueRequestID();
    auto f = [id] (Command *cmd)
             {
               VEO_TRACE("[request #%d] start...", id);
               cmd->setResult(0, VEO_COMMAND_OK);
               return 0;
             };
    CmdPtr req(new internal::CommandImpl(id, f));
    {
      std::lock_guard<std::recursive_mutex> lock(this->submit_mtx);
      if(this->comq.pushRequest(std::move(req)))
        return VEO_REQUEST_ID_INVALID;
    }
    this->comq.notifyAll();
    VEO_TRACE("asyncWriteMem leave...\n");
    return id;
  }

  const char* env_p = std::getenv("VEO_SENDFRAG");
  const char* cut_p = std::getenv("VEO_SENDCUT");
  size_t maxfrag = PART_SENDFRAG;
  size_t cutsz = 2 * 1024 * 1024;

  if (urpc_max_send_cmd_size(this->up) < 2 * maxfrag)
    maxfrag = 1 * 1024 * 1024;

  bool flg = false;
  if (env_p)
    maxfrag = atoi(env_p);
  if (cut_p)
    cutsz = atoi(cut_p);
  if (size < 5 * maxfrag)
    if (size > cutsz)
      maxfrag = ALIGN8B(size / 4);
    else if (size > cutsz / 2)
      maxfrag = ALIGN8B(size / 3);
    else if (size > cutsz / 3)
      maxfrag = ALIGN8B(size / 2);
    else
      maxfrag = ALIGN8B(size);

  size_t psz;
  size_t rsize = size;
  char *s = (char *)src;
  uint64_t d = dst;
  uint64_t prev = VEO_REQUEST_ID_INVALID;

  while (rsize > 0) {
    psz = rsize <= maxfrag ? rsize : maxfrag;
    auto req = this->sendBuffAsync(d, s, psz, prev);
    if (req == VEO_REQUEST_ID_INVALID) {
      VEO_ERROR("req chain submission failed! Aborting.");
      // TODO: abort chain? How?
      return req;
    }
    prev = req;
    rsize -= psz;
    s += psz;
    d += psz;
    if (flg == false) {
      this->progress();
      flg = true;
    }
  }
  this->comq.notifyAll();
  return prev;
}
} // namespace veo
