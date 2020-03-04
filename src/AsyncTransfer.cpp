/**
 * @file AsyncTransfer.cpp
 * @brief implementation of asynchronous memory transfer
 */
#include "ProcHandle.hpp"
#include "ThreadContext.hpp"
#include "CommandImpl.hpp"
#include "log.hpp"
#include "veo_urpc.hpp"
#include <sys/types.h>
#include <unistd.h>

namespace veo {
/**
 * @brief Asynchronous SENDBUFF call
 *
 * @param dst remote destination target address for buffer
 * @param src local src address for buffer data
 * @param size size of transfer
 * @return request ID
 */
uint64_t ThreadContext::sendbuffAsync(uint64_t dst, void *src, size_t size)
{
  VEO_TRACE(this, "sendbuffAsync enter...\n");
  if (this->state == VEO_STATE_EXIT)
    return VEO_REQUEST_ID_INVALID;

  auto id = this->issueRequestID();
  //
  // submit function, called when cmd is issued to URPC
  //
  auto f = [this, src, dst, size, id] (Command *cmd)
           {
             VEO_TRACE(this, "[request #%d] start...", id);
             VEO_TRACE(this, "sendbuff src=%p dst=%p, size=%lu", src, (void *)dst, size);
             int req = urpc_generic_send(this->up, URPC_CMD_SENDBUFF, (char *)"LP",
                                         dst, src, size);
             VEO_TRACE(this, "[request #%d] VE-URPC req ID = %ld", id, req);
             if (req >= 0) {
               cmd->setURPCReq(req, VEO_COMMAND_UNFINISHED);
             } else {
               // TODO: anything more meaningful into result?
               cmd->setResult(0, VEO_COMMAND_ERROR);
               VEO_TRACE(this, "[request #%d] error return...", id);
               return -EAGAIN;
             }
             VEO_TRACE(this, "[request #%d] end...", id);
             return 0;
           };

  //
  // result function, called when response has arrived from URPC
  //
  auto u = [this, id] (Command *cmd, urpc_mb_t *m, void *payload, size_t plen)
           {
             VEO_TRACE(this, "[request #%d] reply ACK received (cmd=%d)...", id, m->c.cmd);
             cmd->setResult(0, VEO_COMMAND_OK);
             VEO_TRACE(this, "[request #%d] result end...", id);
             return 0;
           };

  std::unique_ptr<Command> cmd(new internal::CommandImpl(id, f, u));
  if(this->comq.pushRequest(std::move(cmd)))
    return VEO_REQUEST_ID_INVALID;
  this->progress(3);
  VEO_TRACE(this, "sendbuffAsync leave...\n");
  return id;
}

/**
 * @brief Asynchronous RECVBUFF call
 *
 * @param dst local destination address for buffer data
 * @param src remote source address for buffer
 * @param size size of transfer
 * @return request ID
 */
uint64_t ThreadContext::recvbuffAsync(void *dst, uint64_t src, size_t size)
{
  VEO_TRACE(this, "recvbuffAsync enter...\n");
  if (this->state == VEO_STATE_EXIT)
    return VEO_REQUEST_ID_INVALID;

  auto id = this->issueRequestID();
  //
  // submit function, called when cmd is issued to URPC
  //
  auto f = [this, src, dst, size, id] (Command *cmd)
           {
             VEO_TRACE(this, "[request #%d] start...", id);
             dprintf("recvbuff src=%p dst=%p, size=%lu\n", (void *)src, dst, size);
             int req = urpc_generic_send(this->up, URPC_CMD_RECVBUFF, (char *)"LLL",
                                         src, (uint64_t)dst, size);
             VEO_TRACE(this, "[request #%d] VE-URPC req ID = %ld", id, req);
             if (req >= 0) {
               cmd->setURPCReq(req, VEO_COMMAND_UNFINISHED);
             } else {
               // TODO: anything more meaningful into result?
               cmd->setResult(0, VEO_COMMAND_ERROR);
               return -EAGAIN;
             }
             VEO_TRACE(this, "[request #%d] end...", id);
             return 0;
           };

  //
  // result function, called when response has arrived from URPC
  //
  auto u = [this, src, dst, size, id] (Command *cmd, urpc_mb_t *m, void *payload, size_t plen)
           {
             dprintf("[request #%d] reply sendbuff received (cmd=%d)...\n", id, m->c.cmd);
             uint64_t sent_dst;
             void *buff;
             size_t buffsz;

             urpc_unpack_payload(payload, plen, (char *)"LP", &sent_dst, &buff, &buffsz);
             dprintf("received sendbuff dst=%p size=%lu\n", (void *)sent_dst, buffsz);
             if ((uint64_t)dst != sent_dst) {
               eprintf("recvbuffAsync mismatch: dst=%lx sent_dst=%lx\n", (uint64_t)dst, sent_dst);
               printf("debug with : gdb -p %d\n", getpid());
               sleep(60);
               return -1;
             }
             if (size != buffsz) {
               eprintf("recvbuffAsync mismatch: size=%lu sent_size=%lu\n", size, buffsz);
               return -1;
             }
             memcpy((void *)dst, buff, buffsz);

             cmd->setResult(0, VEO_COMMAND_OK);
             VEO_TRACE(this, "[request #%d] result end...", id);
             return 0;
           };

  std::unique_ptr<Command> cmd(new internal::CommandImpl(id, f, u));
  if(this->comq.pushRequest(std::move(cmd)))
    return VEO_REQUEST_ID_INVALID;
  this->progress(3);
  VEO_TRACE(this, "recvbuffAsync leave...\n");
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
uint64_t ThreadContext::asyncReadMem(void *dst, uint64_t src, size_t size)
{
  VEO_TRACE(this, "asyncReadMem enter...\n");
  if( this->state == VEO_STATE_EXIT )
    return VEO_REQUEST_ID_INVALID;

  const char* env_p = std::getenv("VEO_RECVFRAG");
  size_t maxfrag = PART_SENDFRAG;

  if (env_p)
    maxfrag = atoi(env_p);
  if (size < PART_SENDFRAG * 4)
    if (size > 120 * 1024)
      maxfrag = ALIGN8B(size / 2);
    else if (size > 240 * 1024)
      maxfrag = ALIGN8B(size / 3);
    else if (size > 512 * 1024)
      maxfrag = ALIGN8B(size / 4);

  auto id = this->issueRequestID();
  auto f = [this, maxfrag, dst, src, size, id] (Command *cmd)
           {
             VEO_TRACE(this, "[request #%d] start...", id);
             dprintf("asyncReadMem [request #%d] start...\n", id);
             dprintf("asyncReadMem src=%p dst=%p size=%lu\n", src, (void *)dst, size);
             size_t psz;
             size_t rsize = size;
             char *s = (char *)src;
             char *d = (char *)dst;
             std::vector<uint64_t> reqs;

             while (rsize > 0) {
               psz = rsize <= maxfrag ? rsize : maxfrag;
               auto req = this->recvbuffAsync((void *)d, (uint64_t)s, psz);
               if (req == VEO_REQUEST_ID_INVALID) {
                 eprintf("recvbuffAsync failed! Aborting.");
                 return -1;
               }
               reqs.push_back(req);
               rsize -= psz;
               s += psz;
               d += psz;
             }
             int rv = 0;
             uint64_t dummy;
             // now collect "results"
             for (auto r=reqs.begin(); r!=reqs.end(); ++r) {
               rv += this->callWaitResult(*r, &dummy);
             }
             cmd->setResult(rv, rv == 0 ? VEO_COMMAND_OK : VEO_COMMAND_ERROR);
             VEO_TRACE(this, "[request #%d] end...", id);
             return rv;
           };
  std::unique_ptr<Command> req(new internal::CommandImpl(id, f));
  {
    std::lock_guard<std::mutex> lock(this->submit_mtx);
    if(this->comq.pushRequest(std::move(req)))
      return VEO_REQUEST_ID_INVALID;
  }
  this->progress(3);
  VEO_TRACE(this, "asyncReadMem leave...\n");
  return id;
}

/**
 * @brief asynchronously write data to VE memory
 *
 * @param dst VEMVA destination address
 * @param src VH buffer source address
 * @param size size to transfer in byte
 * @return request ID
 */
uint64_t ThreadContext::asyncWriteMem(uint64_t dst, const void *src,
                                      size_t size)
{
  VEO_TRACE(this, "asyncWriteMem enter...\n");
  if( this->state == VEO_STATE_EXIT )
    return VEO_REQUEST_ID_INVALID;

  const char* env_p = std::getenv("VEO_SENDFRAG");
  size_t maxfrag = PART_SENDFRAG;

  if (env_p)
    maxfrag = atoi(env_p);
  if (size < PART_SENDFRAG * 4)
    if (size > 120 * 1024)
      maxfrag = ALIGN8B(size / 2);
    else if (size > 240 * 1024)
      maxfrag = ALIGN8B(size / 3);
    else if (size > 512 * 1024)
      maxfrag = ALIGN8B(size / 4);

  auto id = this->issueRequestID();
  auto f = [this, maxfrag, dst, src, size, id] (Command *cmd)
           {
             VEO_TRACE(this, "[request #%d] start...", id);
             dprintf("asyncWriteMem [request #%d] start...\n", id);
             dprintf("asyncWriteMem src=%p dst=%p size=%lu\n", src, (void *)dst, size);
             size_t psz;
             size_t rsize = size;
             char *s = (char *)src;
             uint64_t d = dst;
             std::vector<uint64_t> reqs;

             while (rsize > 0) {
               psz = rsize <= maxfrag ? rsize : maxfrag;
               auto req = this->sendbuffAsync(d, s, psz);
               if (req == VEO_REQUEST_ID_INVALID) {
                 eprintf("sendbuffAsync failed! Aborting.");
                 return -1;
               }
               reqs.push_back(req);
               rsize -= psz;
               s += psz;
               d += psz;
             }
             int rv = 0;
             uint64_t dummy;
             // now collect "results"
             for (auto r=reqs.begin(); r!=reqs.end(); ++r) {
               rv += this->callWaitResult(*r, &dummy);
             }
             cmd->setResult(rv, rv == 0 ? VEO_COMMAND_OK : VEO_COMMAND_ERROR);
             return rv;
           };
  std::unique_ptr<Command> req(new internal::CommandImpl(id, f));
  dprintf("cmd is VH? %d", req.get()->isVH());
  {
    std::lock_guard<std::mutex> lock(this->submit_mtx);
    if(this->comq.pushRequest(std::move(req)))
      return VEO_REQUEST_ID_INVALID;
  }
  this->progress(3);
  VEO_TRACE(this, "asyncWriteMem leave...\n");
  return id;
}
} // namespace veo
