/**
 * @file AsyncTransfer.cpp
 * @brief implementation of asynchronous memory transfer
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
             } else {
               // TODO: anything more meaningful into result?
               cmd->setResult(0, VEO_COMMAND_ERROR);
               VEO_TRACE("[request #%d] error return...", id);
               return -EAGAIN;
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
             if (prev != VEO_REQUEST_ID_INVALID) {
               auto rc = this->_peekResult(prev, &result);
               if (rc != VEO_COMMAND_OK) {
                 VEO_ERROR("request #%ld in chain has unexpected status %d",
                           prev, rc);
                 // TODO: handle this
               }
             }
             if (m->c.cmd == URPC_CMD_EXCEPTION) {
               cmd->setResult(-URPC_CMD_SENDBUFF, VEO_COMMAND_EXCEPTION);
               return (int)-URPC_CMD_SENDBUFF;
             }
             cmd->setResult(result, VEO_COMMAND_OK);
             return 0;
           };

  std::unique_ptr<Command> cmd(new internal::CommandImpl(id, f, u));
  {
    std::lock_guard<std::mutex> lock(this->submit_mtx);
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
             } else {
               // TODO: anything more meaningful into result?
               cmd->setResult(0, VEO_COMMAND_ERROR);
               return -EAGAIN;
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
               return -1;
             }
             if (size != buffsz) {
               VEO_ERROR("mismatch: size=%lu sent_size=%lu", size, buffsz);
               return -1;
             }
             memcpy((void *)dst, buff, buffsz);
             uint64_t result = 0;
             if (prev != VEO_REQUEST_ID_INVALID) {
               auto rc = this->_peekResult(prev, &result);
               if (rc != VEO_COMMAND_OK) {
                 VEO_ERROR("request #%ld in chain has unexpected status %d",
                           prev, rc);
                 // TODO: handle this
               }
             }
             cmd->setResult(result, VEO_COMMAND_OK);
             return 0;
           };

  std::unique_ptr<Command> cmd(new internal::CommandImpl(id, f, u));
  {
    std::lock_guard<std::mutex> lock(this->submit_mtx);
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
    std::unique_ptr<Command> req(new internal::CommandImpl(id, f));
    {
      std::lock_guard<std::mutex> lock(this->submit_mtx);
      if(this->comq.pushRequest(std::move(req)))
        return VEO_REQUEST_ID_INVALID;
    }
    this->progress(2);
    VEO_TRACE("asyncWriteMem leave...\n");
    return id;
  }

  const char* env_p = std::getenv("VEO_RECVFRAG");
  const char* cut_p = std::getenv("VEO_RECVCUT");
  size_t maxfrag = PART_SENDFRAG;
  size_t cutsz = 2 * 1024 * 1024;

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
    this->progress(2);
  }
  return prev;
}

/**
 * @brief asynchronously write data to VE memory
 *
 * @param dst VEMVA destination address
 * @param src VH buffer source address
 * @param size size to transfer in byte
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
    std::unique_ptr<Command> req(new internal::CommandImpl(id, f));
    {
      std::lock_guard<std::mutex> lock(this->submit_mtx);
      if(this->comq.pushRequest(std::move(req)))
        return VEO_REQUEST_ID_INVALID;
    }
    this->progress(2);
    VEO_TRACE("asyncWriteMem leave...\n");
    return id;
  }

  const char* env_p = std::getenv("VEO_SENDFRAG");
  const char* cut_p = std::getenv("VEO_SENDCUT");
  size_t maxfrag = PART_SENDFRAG;
  size_t cutsz = 2 * 1024 * 1024;

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
    this->progress(2);
  }
  return prev;
}
} // namespace veo
