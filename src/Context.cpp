/**
 * @file Context.cpp
 * @brief implementation of Context
 */
#include <set>

#include <pthread.h>
#include <cerrno>
#include <semaphore.h>
#include <signal.h>
#include <unistd.h>
#include <wait.h>

#include "veo_urpc.h"
#include "CallArgs.hpp"
#include "Context.hpp"
#include "ProcHandle.hpp"
#include "CommandImpl.hpp"
#include "VEOException.hpp"
#include "log.h"

#include "veo_urpc_vh.hpp"

namespace veo {

  Context::Context(ProcHandle *p, urpc_peer_t *up, bool is_main):
  proc(p), up(up), state(VEO_STATE_UNKNOWN), is_main(is_main), seq_no(0) {}

/**
 * @brief close this context
 *
 * @return zero upon success; negative upon failure.
 *
 * Close this VEO thread context; terminate the pseudo thread.
 * @return 0 if all went well
 */
int Context::close()
{
  VEO_TRACE("ctx=%p", this);
  if ( this->state == VEO_STATE_EXIT )
    return 0;
  this->state = VEO_STATE_EXIT;
  uint64_t req = urpc_generic_send(this->up, URPC_CMD_EXIT, (char *)"");
  auto rc = wait_req_ack(this->up, req);
  if (rc < 0) {
    VEO_ERROR("child sent no ACK to EXIT. Killing it.");
    vh_urpc_child_destroy(this->up);
  }
  rc = vh_urpc_peer_destroy(this->up);
  return rc;
}

/**
 * @brief worker function for progress
 *
 *
 */
void Context::_progress_nolock(int ops)
{
  urpc_comm_t *uc = &this->up->recv;
  transfer_queue_t *tq = uc->tq;
  urpc_mb_t m;
  void *payload = nullptr;
  size_t plen;
  int recvd, sent;

  do {
    --ops;
    //
    // try to receive a command reply
    //
    recvd = sent = 0;
    int64_t req = urpc_get_cmd(tq, &m);
    if (req >= 0) {
      ++recvd;
      auto cmd = std::move(this->comq.popInFlight());
      if (cmd == nullptr) {
        // ooops!
        throw VEOException("URPC req without corresponding cmd!?", req);
      }
      set_recv_payload(uc, &m, &payload, &plen);
      //
      // call command "result function"
      //
      auto rv = (*cmd)(&m, payload, plen);
      urpc_slot_done(tq, REQ2SLOT(req), &m);
      this->comq.pushCompletion(std::move(cmd));
      if (rv < 0) {
        this->state = VEO_STATE_EXIT;
        this->comq.cancelAll();
        VEO_ERROR("Internal error on executing a command(%d)", rv);
        return;
      }
    }
    //
    // try to submit a new command
    //
    if (urpc_next_send_slot(this->up) < 0) {
      continue;
    }
    auto cmd = std::move(this->comq.tryPopRequest());
    if (cmd) {
      if (cmd->isVH()) {
        if (this->comq.emptyInFlight()) {
          //
          // call command "submit function"
          //
          auto rv = (*cmd)();
          this->comq.pushCompletion(std::move(cmd));
          ++sent;
        }
      } else {
        //
        // call command "submit function"
        //
        auto rv = (*cmd)();
        if (rv == 0) {
          ++sent;
          this->comq.pushInFlight(std::move(cmd));
        }
      }
    }
  } while(recvd + sent > 0); // || ops != 0);
}

/**
 * @brief Progress function for asynchronous calls
 *
 * @param ops number of operations. zero: as many as possible
 *
 * Check if any URPC request has finished.
 * If yes, pop a cmd from inflight queue, receive its result.
 * Push a new command, if any.
 * Repeat.
 */
void Context::progress(int ops)
{
  std::lock_guard<std::recursive_mutex> lock(this->prog_mtx);
  _progress_nolock(ops);
}

/**
 * @brief Synchronize this context.
 *
 * Block other threads from submitting requests to this context,
 * call progress() until request queue and inflight queues are empty.
 */
void Context::synchronize()
{
  std::lock_guard<std::mutex> lock(this->submit_mtx);
  this->_synchronize_nolock();
}
  
/**
 * @brief The actual synchronize work function
 *
 * This function should only be called with the main_mutex locked!
 */
void Context::_synchronize_nolock()
{
  while(!(this->comq.emptyRequest() && this->comq.emptyInFlight())) {
    this->progress(0);
  }
}

/**
 * @brief start a function on the VE, wait for result and return it.
 *
 * @param addr VEMVA of function called
 * @param args arguments of the function
 * @param result pointer to result
 * @return 0 if all went well.
 */
int Context::callSync(uint64_t addr, CallArgs &arg, uint64_t *result)
{
  urpc_mb_t m;
  void *payload;
  size_t plen;

  std::lock_guard<std::mutex> lock(this->submit_mtx);
  this->_synchronize_nolock();
  VEO_TRACE("VE function %#lx", addr);

  int64_t req = send_call_nolock(this->up, this->ve_sp, addr, arg);

  // TODO: make sync call timeout configurable
  if (!urpc_recv_req_timeout(this->up, &m, req, (long)(15*REPLY_TIMEOUT), &payload, &plen)) {
    // timeout! complain.
    VEO_ERROR("callSync timeout waiting for RESULT req=%ld", req);
    return -1;
  }
  int rc = unpack_call_result(&m, &arg, payload, plen, result);
  urpc_slot_done(this->up->recv.tq, REQ2SLOT(req), &m);
  return rc;
}

/**
 * @brief call a VE function asynchronously
 *
 * @param addr VEMVA of VE function to call
 * @param args arguments of the function
 * @return request ID
 */
uint64_t Context::callAsync(uint64_t addr, CallArgs &args)
{
  VEO_TRACE("VE function %lx", addr);
  if ( addr == 0 || this->state == VEO_STATE_EXIT)
    return VEO_REQUEST_ID_INVALID;
  
  auto id = this->issueRequestID();
  //
  // submit function, called when cmd is issued to URPC
  //
  auto f = [&args, this, addr, id] (Command *cmd)
           {
             VEO_TRACE("[request #%d] start...", id);
             int req = send_call_nolock(this->up, this->ve_sp, addr, args);
             VEO_TRACE("[request #%d] VE-URPC req ID = %ld", id, req);
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
  auto u = [&args, this, id] (Command *cmd, urpc_mb_t *m, void *payload, size_t plen)
           {
             VEO_TRACE("[request #%d] reply sendbuff received (cmd=%d)...", id, m->c.cmd);
             uint64_t result;
             int rv = unpack_call_result(m, &args, payload, plen, &result);
             VEO_TRACE("[request #%d] unpacked", id);
             if (rv < 0) {
               cmd->setResult(result, VEO_COMMAND_EXCEPTION);
               return rv;
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
  this->progress(3);
  return id;
}

/**
 * @brief call a VE function specified by symbol name asynchronously
 *
 * @param libhdl handle of library
 * @param symname a symbol name to find
 * @param args arguments of the function
 * @return request ID
 */
uint64_t Context::callAsyncByName(uint64_t libhdl, const char *symname, CallArgs &args)
{
  uint64_t addr = this->proc->getSym(libhdl, symname);
  return this->callAsync(addr, args);
}

/**
 * @brief call a VH function asynchronously
 *
 * @param func address of VH function to call
 * @param arg pointer to opaque arguments structure for the function
 * @return request ID
 */
uint64_t Context::callVHAsync(uint64_t (*func)(void *), void *arg)
{
  VEO_TRACE("VH function %lx", func);
  if ( func == nullptr || this->state == VEO_STATE_EXIT)
    return VEO_REQUEST_ID_INVALID;

  auto id = this->issueRequestID();
  auto f = [this, func, arg, id] (Command *cmd)
           {
             VEO_TRACE("[request #%lu] start...", id);
             auto rv = (*func)(arg);
             VEO_TRACE("[request #%lu] executed. (return %ld)", id, rv);
             cmd->setResult(rv, VEO_COMMAND_OK);
             VEO_TRACE("[request #%lu] done", id);
             return 0;
           };
  std::unique_ptr<Command> req(new internal::CommandImpl(id, f));
  {
    std::lock_guard<std::mutex> lock(this->submit_mtx);
    this->comq.pushRequest(std::move(req));
  }
  this->progress(3);
  return id;
}

/**
 * @brief check if the result of a request (command) is available
 *
 * @param reqid request ID to wait
 * @param retp pointer to buffer to store the return value.
 * @retval VEO_COMMAND_OK the execution of the function succeeded.
 * @retval VEO_COMMAND_EXCEPTION exception occured on the execution.
 * @retval VEO_COMMAND_ERROR error occured on handling the command.
 * @retval VEO_COMMAND_UNFINISHED the command is not finished.
 */
int Context::callPeekResult(uint64_t reqid, uint64_t *retp)
{
  VEO_TRACE("req %lu", reqid);
  this->progress(3);
  std::lock_guard<std::mutex> lock(this->req_mtx);
  auto itr = rem_reqid.find(reqid);
  if( itr == rem_reqid.end() ) {
    return VEO_COMMAND_ERROR;
  }
  auto c = this->comq.peekCompletion(reqid);
  if (c != nullptr) {
    if (!rem_reqid.erase(reqid))
      return VEO_COMMAND_ERROR;
    *retp = c->getRetval();
    return c->getStatus();
  }
  return VEO_COMMAND_UNFINISHED;
}

/**
 * @brief wait for the result of request (command)
 *
 * @param reqid request ID to wait
 * @param retp pointer to buffer to store the return value.
 * @retval VEO_COMMAND_OK the execution of the function succeeded.
 * @retval VEO_COMMAND_EXCEPTION exception occured on the execution.
 * @retval VEO_COMMAND_ERROR error occured on handling the command.
 * @retval VEO_COMMAND_UNFINISHED the command is not finished.
 */
int Context::callWaitResult(uint64_t reqid, uint64_t *retp)
{
  VEO_TRACE("req %lu", reqid);
#if 1
  //
  // polling here because we need to call the progress function!
  //
  int rv;
  do {
    rv = this->callPeekResult(reqid, retp);
  } while (rv == VEO_COMMAND_UNFINISHED);
  return rv;
#else
  req_mtx.lock();
  auto itr = rem_reqid.find(reqid);
  if( itr == rem_reqid.end() ) {
    req_mtx.unlock();
    return VEO_COMMAND_ERROR;
  }
  if (!rem_reqid.erase(reqid)) {
    req_mtx.unlock();
    return VEO_COMMAND_ERROR;
  }
  req_mtx.unlock();
  auto c = this->comq.waitCompletion(reqid);
  *retp = c->getRetval();
  return c->getStatus();
#endif
}

} // namespace veo
