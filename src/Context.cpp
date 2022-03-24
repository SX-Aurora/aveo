/**
 * @file Context.cpp
 * @brief implementation of Context
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
 * Context methods implementation.
 * 
 * Copyright (c) 2018-2021 NEC Corporation
 * Copyright (c) 2020-2021 Erich Focht
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
  if (!this->is_alive())
    return 0;
  this->state = VEO_STATE_EXIT;
  int64_t req = urpc_generic_send(this->up, URPC_CMD_EXIT, (char *)"");
  if (req < 0) {
    VEO_ERROR("failed to send cmd %d", URPC_CMD_EXIT);
    vh_urpc_peer_destroy(this->up);
    return -1;
  }
  auto rc = wait_req_ack(this->up, req);
  if (rc < 0) {
    VEO_ERROR("child sent no ACK to EXIT. Killing it.");
    vh_urpc_child_destroy(this->up);
  }
  if (this->up->child_pid > 0) {
    int status;
    VEO_TRACE("entering waitpid(%d)", this->up->child_pid);
    waitpid(this->up->child_pid, &status, 0);
    VEO_DEBUG("waitpid(%d) returned status=%d", this->up->child_pid, status);
    this->up->child_pid = -1;
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

  //VEO_TRACE("start");
  do {
    //
    // try to receive command replies
    //
    recvd = sent = 0;
    int64_t req = urpc_get_cmd(tq, &m);
    if (req >= 0) {
      ++recvd;
      auto cmd = std::move(this->comq.popInFlight(req));
      if (cmd == nullptr) {
        // ooops!
        VEO_ERROR("Reply urpcreq %ld -> No inflight cmd found!", req);
        VEO_ERROR("urpc cmd = %d", m.c.cmd);
        throw VEOException("URPC req without corresponding cmd!?", req);
      }
      set_recv_payload(uc, &m, &payload, &plen);
      //
      // call command "result function"
      //
      auto rv = (*cmd)(&m, payload, plen);
      urpc_slot_done(tq, REQ2SLOT(req), &m);

      // If cmd is URPC_CMD_ACCS_PCIRCVSYC,
      // it is flagged not to save the result to the completion queue.
      if (!cmd->getNowaitFlag())
        this->comq.pushCompletion(std::move(cmd));
      if (rv < 0) {
        this->state = VEO_STATE_EXIT;
        this->comq.cancelAll();
        VEO_ERROR("Internal error on executing a command(%d)", rv);
        return;
      }
      // continue receiving replies
      // We need this until we can manage buffer memory pressure
      continue;
    }
    //
    // try to submit a new command
    //
    if (urpc_next_send_slot(this->up) < 0)
      continue;
    auto cmd = std::move(this->comq.tryPopRequest());
    if (cmd) {
      if (cmd->isVH()) {
        if (this->comq.emptyInFlight()) {
          //
          // call command "submit function"
          //
          //VEO_TRACE("executing VH command id = %lu", cmd->getID());
          auto rv = (*cmd)();
          this->comq.pushCompletion(std::move(cmd));
          ++sent;
        } else {
          //VEO_TRACE("delaying VH cmd submit because VE cmds in flight");
          this->comq.pushRequestFront(std::move(cmd));
        }
      } else {
        //
        // call command "submit function"
        //
        auto rv = (*cmd)();
        if (rv == 0) {
          ++sent;
          this->comq.pushInFlight(std::move(cmd));
        } else {
          cmd->setResult(rv, VEO_COMMAND_ERROR);
          this->comq.pushCompletion(std::move(cmd));
          VEO_ERROR("submit function failed(%d)", rv);
        }
      }
    }
  } while((recvd + sent > 0) && (recvd + sent < ops));
  //VEO_TRACE("end");
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
  //VEO_TRACE("start");
  //std::lock_guard<std::recursive_mutex> lock(this->prog_mtx);
  if (this->prog_mtx.try_lock()) {
    _progress_nolock(ops);
    this->prog_mtx.unlock();
  }
  //VEO_TRACE("end");
}

/**
 * @brief Synchronize this context.
 *
 * Block other threads from submitting requests to this context,
 * call progress() until request queue and inflight queues are empty.
 */
void Context::synchronize()
{
  //VEO_TRACE("start");
  std::lock_guard<std::mutex> lock(this->submit_mtx);
  this->_synchronize_nolock();
  //VEO_TRACE("end");
}
  
/**
 * @brief The actual synchronize work function
 *
 * This function should only be called with the main_mutex locked!
 */
void Context::_synchronize_nolock()
{
  //VEO_TRACE("start");
  this->prog_mtx.lock();
  while(!(this->comq.emptyRequest() && this->comq.emptyInFlight())) {
    this->prog_mtx.unlock();
    this->progress(10000000);
    this->prog_mtx.lock();
  }
  this->prog_mtx.unlock();
  //VEO_TRACE("end");
}

/**
 * @brief Retrieve the stack pointer of a context thread on VE
 *
 * @param sp pointer to variable which receives the stack pointer
 * @throws VEOException when failing
 */
void Context::getStackPointer(uint64_t *sp)
{
  urpc_mb_t m;
  void *payload;
  size_t plen;
  auto req = urpc_generic_send(this->up, URPC_CMD_CALL,
                               (char *)"LP", 0, nullptr, 0);
  if (req < 0) {
    VEO_ERROR("Sending ve-urpc request failed\n");
    throw VEOException("Failed to get the VE SP.");
  }
  if (!urpc_recv_req_timeout(this->up, &m, req,
                             (long)(15*REPLY_TIMEOUT), &payload, &plen)) {
    // timeout! complain.
    VEO_ERROR("timeout waiting for RESULT req=%ld", req);
    throw VEOException("Failed to get the VE SP.");
  }
  int rc = unpack_call_result(&m, nullptr, payload, plen, sp, nullptr);
  urpc_slot_done(this->up->recv.tq, REQ2SLOT(req), &m);
}


/**
 * @brief start a function on the VE, wait for result and return it.
 *
 * @param addr VEMVA of function called
 * @param args arguments of the function
 * @param result pointer to result
 * @return 0 if all went well.
 */
int Context::callSync(uint64_t addr, CallArgs &args, uint64_t *result)
{
  if (!this->is_alive())
    return -1;

  args.setup(this->ve_sp - RESERVED_STACK_SIZE);
  auto req = this->doCallAsync(addr, args);
  if (req == VEO_REQUEST_ID_INVALID) {
    VEO_ERROR("failed! Aborting.");
    return -1;
  }
  auto rv = this->callWaitResult(req, result);
  if (rv != VEO_COMMAND_OK) {
    VEO_TRACE("done. rv=%d", rv);
    rv = -1;
  }
  return rv;
}

/**
 * @brief call a VE function asynchronously
 *
 * @param addr VEMVA of VE function to call
 * @param args arguments of the function
 * @return request ID
 * @note the size of args need to be less than or equal to MAX_ARGS_STACK_SIZE
 * @note The caller must invoke args.setup()
 */
uint64_t Context::simpleCallAsync(uint64_t addr, std::vector<uint64_t> regs, uint64_t stack_top, size_t stack_size,
                                  bool copyin, bool copyout, void *stack, void *stack_, std::function<void(void*)> copyout_func)
{
  VEO_TRACE("VE function %lx", addr);
  if ( addr == 0 || !this->is_alive())
    return VEO_REQUEST_ID_INVALID;
  
  auto id = this->issueRequestID();

  //
  // submit function, called when cmd is issued to URPC
  //
  auto f = [regs, stack_top, stack_size, copyin, copyout,
            stack, this, addr, id] (Command *cmd)
           {
             VEO_TRACE("[request #%d] start...", id);
             int64_t req = send_call_nolock(this->up, this->ve_sp, addr, regs,
                                            stack_top, stack_size,
                                            copyin, copyout, stack);
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
  auto u = [copyout_func, this, id, stack, stack_] (Command *cmd, urpc_mb_t *m, void *payload, size_t plen)
           {
             VEO_TRACE("[request #%d] reply sendbuff received (cmd=%d)...", id, m->c.cmd);
             uint64_t result;
             int rv = unpack_call_result(m, copyout_func, payload, plen, &result, stack_);
             VEO_TRACE("[request #%d] unpacked", id);
             if (!stack_)
               delete[] (char *)stack;
             if (rv < 0) {
               cmd->setResult(result, VEO_COMMAND_EXCEPTION);
               this->state = VEO_STATE_EXIT;
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
 * @brief call a VE function asynchronously
 *
 * @param addr VEMVA of VE function to call
 * @param args arguments of the function
 * @return request ID
 * @note The caller must invoke args.setup()
 */
uint64_t Context::doCallAsync(uint64_t addr, CallArgs &args)
{
  // alive check was done before
  if (addr == 0)
    return VEO_REQUEST_ID_INVALID;

  if ((this->ve_sp & 0xFFFFFFFFFC000000) !=
       (args.stack_top & 0xFFFFFFFFFC000000)) {
    VEO_ERROR("Stack crossed page boundary.");
    return VEO_REQUEST_ID_INVALID;
  }

  auto reg_args_num = args.numArgs();
  if (reg_args_num > NUM_ARGS_ON_REGISTER)
    reg_args_num = NUM_ARGS_ON_REGISTER;
  size_t reg_args_sz = reg_args_num * sizeof(uint64_t);

  // Copy CallArgs member variables
  auto regs = args.getRegVal();
  auto stack_top = args.stack_top;
  auto stack_size = args.stack_size;
  bool copyin = args.copied_in;
  bool copyout = args.copied_out;
  char *stack = args.stack_buf;
  args.stack_buf = nullptr;
  auto copyout_func = args.copyout();

  if (!(copyin || copyout) ||
      (args.stack_size <= MAX_ARGS_STACK_SIZE - reg_args_sz)) {
    VEO_TRACE("callAsync simple case");
    VEO_TRACE("VE function %lx", addr);
    // alive check and addr check was done before.
    // stack is freed by the result function of simpleCallAsync().
    return this->simpleCallAsync(addr, regs, stack_top, stack_size,
                                 copyin, copyout, stack, nullptr, copyout_func);
  }

  //VEO_TRACE("callAsync large arguments");
  auto id = this->issueRequestID();
  // stack is freed by this lambda function
  auto f = [this, addr, reg_args_sz, regs, stack_top, stack_size,
            copyin, copyout, stack, copyout_func] (Command *cmd)
           {
             uint64_t extra_stk = (uint64_t)stack_top
               + MAX_ARGS_STACK_SIZE - reg_args_sz;
             void *extra_buf = (char *)stack
               + MAX_ARGS_STACK_SIZE - reg_args_sz;
             uint64_t extra_size = (uint64_t)stack_size
               - MAX_ARGS_STACK_SIZE + reg_args_sz;
             int rv;
             if (copyin) {
               rv = this->writeMem(extra_stk, extra_buf, extra_size);
               if (rv != 0) {
                 VEO_ERROR("Writing memory failed! Aborting.");
                 delete[] stack;
                 return -1;
               }
             }
             auto req = this->simpleCallAsync(addr, regs, stack_top, stack_size,
                                              copyin, copyout, stack, stack, nullptr);
             if (req == VEO_REQUEST_ID_INVALID) {
               VEO_ERROR("failed! Aborting.");
               delete[] stack;
               return -1;
             }
             int status = 0;
             uint64_t result = 0;
             status = this->callWaitResult(req, &result);

             if (status != 0) {
               delete[] stack;
               return status;
             }

             if (copyout) {
               rv = this->readMem(extra_buf, extra_stk, extra_size);
               if (rv != 0) {
                 VEO_ERROR("Reading memory failed! Aborting.");
                 delete[] stack;
                 return -1;
               }
               copyout_func(stack);
             }
             cmd->setResult(result, status);
             delete[] stack;

             return status;
           };
  std::unique_ptr<Command> req(new internal::CommandImpl(id, f));
  {
    std::lock_guard<std::mutex> lock(this->submit_mtx);
    if(this->comq.pushRequest(std::move(req)))
      return VEO_REQUEST_ID_INVALID;
  }
  this->progress(3);
  //VEO_TRACE("callAsync leave...\n");
  
  return id;
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
  VEO_TRACE("callAsync");
  if ( addr == 0 || !this->is_alive())
    return VEO_REQUEST_ID_INVALID;

  args.setup(this->ve_sp - RESERVED_STACK_SIZE);

  return doCallAsync(addr, args);
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
  if ( func == nullptr ||  !this->is_alive())
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
  this->progress(2);
  return id;
}

int Context::_peekResult(uint64_t reqid, uint64_t *retp)
{
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
  this->progress(3);
  return this->_peekResult(reqid, retp);
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

/**
 * @brief constructor
 */

ThreadContextAttr::ThreadContextAttr()
{
  this->stacksize = VEO_DEFAULT_STACKSIZE;
}

void ThreadContextAttr::setStacksize(size_t stack_sz)
{
  if (stack_sz < VEO_STACK_MIN) {
    VEO_ERROR("stack size of VEO context must be more equal to 0x%lx", VEO_STACK_MIN);
   throw VEOException("invalid stack size of VEO context", EINVAL);
  }
 this->stacksize = stack_sz;
}

/**
 * @brief read data from VE memory
 * @param[out] dst buffer to store the data
 * @param src VEMVA to read
 * @param size size to transfer in byte
 * @return zero upon success; negative upon failure
 */
int Context::readMem(void *dst, uint64_t src, size_t size)
{
  VEO_TRACE("(%p, %lx, %ld)", dst, src, size);
  auto req = this->asyncReadMem(dst, src, size);
  if (req == VEO_REQUEST_ID_INVALID) {
    VEO_ERROR("failed! Aborting.");
    return -1;
  }
  uint64_t dummy;
  auto rv = this->callWaitResult(req, &dummy);
  if (rv != VEO_COMMAND_OK) {
    VEO_TRACE("done, rv=%d", rv);
    rv = -1;
  }
  return rv;
}

/**
 * @brief write data to VE memory
 * @param dst VEMVA to write the data
 * @param src buffer holding data to write
 * @param size size to transfer in byte
 * @return zero upon success; negative upon failure
 */
int Context::writeMem(uint64_t dst, const void *src, size_t size)
{
  VEO_TRACE("(%p, %lx, %ld)", dst, src, size);
  auto req = this->asyncWriteMem(dst, src, size);
  if (req == VEO_REQUEST_ID_INVALID) {
    VEO_ERROR("failed! Aborting.");
    return -1;
  }
  uint64_t dummy;
  auto rv = this->callWaitResult(req, &dummy);
  if (rv != VEO_COMMAND_OK) {
    VEO_TRACE("done. rv=%d", rv);
    rv = -1;
  }
  return rv;
}

} // namespace veo
