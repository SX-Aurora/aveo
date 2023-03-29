/**
 * @file Context.hpp
 * @brief VEO thread context
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
 * Context class definition.
 * 
 * Copyright (c) 2018-2021 NEC Corporation
 * Copyright (c) 2020-2021 Erich Focht
 */
#ifndef _VEO_THREAD_CONTEXT_HPP_
#define _VEO_THREAD_CONTEXT_HPP_

#include "Command.hpp"
#include "CommandImpl.hpp"
#include <mutex>
#include <unordered_set>
#include <utility>
#include <tuple>
#include <functional>

#include <pthread.h>
#include <semaphore.h>

#include "log.h"
#include <urpc.h>
#include "veo_urpc.h"
#include "veo_urpc_vh.hpp"

#include <ve_offload.h>

namespace veo {

class ProcHandle;
class CallArgs;
class ThreadContextAttr;

/**
 * @brief VEO thread context
 */
class Context {
  friend class ProcHandle;// ProcHandle controls the main thread directly.
private:
  CommQueue comq;
  veo_context_state state;
  bool is_main;
  uint64_t seq_no;
  uint64_t ve_sp;
  int core;			//!< VE core to which the handler is pinned
  urpc_peer_t *up;		//!< ve-urpc peer pointer, each ctx has one
  std::unordered_set<uint64_t> rem_reqid; // TODO: move this away from here!
  std::mutex req_mtx;   //!< protects rem_reqid
  std::recursive_mutex submit_mtx;//!< for synchronous calls prohibit submission of new reqs
  std::recursive_mutex prog_mtx;
  void progress();
  int _progress_nolock(bool);
  pthread_t progress_thread;
  uint64_t count;
  /**
   * @brief Issue a new request ID
   * @return a request ID, 64 bit integer, to identify a command
   */
  uint64_t issueRequestID() {
    uint64_t ret = VEO_REQUEST_ID_INVALID;
    while (ret == VEO_REQUEST_ID_INVALID) {
      ret = __atomic_fetch_add(&this->seq_no, 1, __ATOMIC_SEQ_CST);
    }
    std::lock_guard<std::mutex> lock(this->req_mtx);
    rem_reqid.insert(ret);
    return ret;
  }
  /**
   * @brief check if context is alive
   *
   * @return true or false
   *
   * Check if context state is exiting or receiver end has unexpected state.
   */
  bool is_alive() {
    urpc_comm_t *uc = &this->up->send;
    if (this->state == VEO_STATE_EXIT) {
      VEO_TRACE("context %p state is VEO_STATE_EXIT", this);
      return false;
    } else if (urpc_get_receiver_flags(uc) & URPC_FLAG_EXITED) {
      VEO_TRACE("context %p urpc recv flag is EXITING", this);
      return false;
    }
    // TODO: add sleeping / wakeup some day
    return true;
  }

  uint64_t simpleCallAsync(uint64_t, std::vector<uint64_t>, uint64_t, size_t, bool, bool, void *, void *, std::function<void(void*)>);
  uint64_t doCallAsync(uint64_t, CallArgs &);

  // handlers for commands
  int _readMem(void *, uint64_t, size_t);
  int _writeMem(uint64_t, const void *, size_t);
  uint64_t _callOpenContext(ProcHandle *, uint64_t, CallArgs &);
  void _delFromProc();
  int _peekResult(uint64_t reqid, uint64_t *retp);

public:
  Context(ProcHandle *, urpc_peer_t *up, bool is_main);
  Context(ProcHandle *);
  ~Context() {}
  Context(const Context &) = delete;//non-copyable
  void getStackPointer(uint64_t *sp);
  veo_context_state getState() { return this->state; }
  void reqBlockBegin() { submit_mtx.lock(); }
  void reqBlockEnd() { submit_mtx.unlock(); }
  int callSync(uint64_t addr, CallArgs &arg, uint64_t *result);
  uint64_t callAsync(uint64_t, CallArgs &);
  uint64_t callAsyncByName(uint64_t, const char *, CallArgs &);
  uint64_t callVHAsync(uint64_t (*)(void *), void *);
  int callWaitResult(uint64_t, uint64_t *);
  int callPeekResult(uint64_t, uint64_t *);
  void synchronize();

  bool progressInit();
  void progressTerminate();
  void progressExec();
  bool waitProgress();

  uint64_t sendBuffAsync(uint64_t dst, void *src, size_t size, uint64_t prev);
  uint64_t recvBuffAsync(void *dst, uint64_t src, size_t size, uint64_t prev);

  uint64_t asyncReadMem(void *dst, uint64_t src , size_t size);
  uint64_t asyncWriteMem(uint64_t dst, const void *src, size_t size);
  int readMem(void *dst, uint64_t src , size_t size);
  int writeMem(uint64_t dst, const void *src, size_t size);

  veo_thr_ctxt *toCHandle() {
    return reinterpret_cast<veo_thr_ctxt *>(this);
  }
  bool isMain() { return this->is_main;}
  int64_t _closeCommandHandler(uint64_t id);
  int close();

  /**
   * @brief Generic Async Request
   *
   * @param urpc_cmd VE-URPC command
   * @param fmt format string for urpc_generic_send
   * @param args... arguments for urpc_generic_send
   * @return request ID
   */
#ifndef NOCPP17
  template <typename ... Args>
  uint64_t genericAsyncReq(int urpc_cmd, char *fmt, Args&&... args)
#else
  uint64_t genericAsyncReq(int urpc_cmd, char *fmt, ...)
#endif
  {
    //VEO_TRACE("start");
    if (!this->is_alive())
      return VEO_REQUEST_ID_INVALID;

    auto id = this->issueRequestID();
#ifdef NOCPP17
    // We pass 3 arguments to urpc_generic_send().
    // If the caller of this function specify less than 3 arguments,
    // we will pass garbage data. But, it will not break something.
    va_list ap;
    va_start(ap, fmt);
    uint64_t arg1 = va_arg(ap, uint64_t);
    uint64_t arg2 = va_arg(ap, uint64_t);
    uint64_t arg3 = va_arg(ap, uint64_t);
    va_end(ap);
#endif
    //
    // submit function, called when cmd is issued to URPC
    //
#ifndef NOCPP17
    auto f = [this, id, urpc_cmd, fmt,
              args = std::make_tuple(this->up, urpc_cmd, fmt,
                                     std::forward<Args>(args)...)](Command *cmd)
             {
               int req = std::apply(urpc_generic_send, args);
#else
    auto f = [this, id, urpc_cmd, fmt, arg1, arg2, arg3](Command *cmd)
             {
               int req = urpc_generic_send(this->up, urpc_cmd, fmt,
					   arg1, arg2, arg3);
#endif
               VEO_TRACE("[request #%d] urpcreq = %ld", id, req);
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
    auto u = [this, id] (Command *cmd, urpc_mb_t *m, void *payload, size_t plen)
             {
               if (m->c.cmd == URPC_CMD_ACK)
                 cmd->setResult(0, VEO_COMMAND_OK);
               else if (m->c.cmd != URPC_CMD_RES_STK) {
                 uint64_t result;
                 int rv = unpack_call_result(m, nullptr, payload, plen, &result, nullptr);
                 if (rv < 0) {
                   cmd->setResult(result, VEO_COMMAND_EXCEPTION);
                   this->state = VEO_STATE_EXIT;
                   return rv;
                 }
                 cmd->setResult(result, VEO_COMMAND_OK);
               }
               VEO_TRACE("[request #%d] result end...", id);
               return 0;
             };

    CmdPtr cmd(new internal::CommandImpl(id, f, u));
    {
      // Flagged to not store cmd results in completion queue.
      if (urpc_cmd == URPC_CMD_ACS_PCIRCVSYC) {
        cmd->setNowaitFlag(true);
        std::lock_guard<std::mutex> reqlock(this->req_mtx);
        auto ret = this->rem_reqid.erase(id);
      }
      std::lock_guard<std::recursive_mutex> lock(this->submit_mtx);
      if(this->comq.pushRequest(std::move(cmd)))
        return VEO_REQUEST_ID_INVALID;
      VEO_TRACE("submitted [request #%lu]", id);
    }
    this->progress();
    this->comq.notifyAll();
    return id;
  }

  ProcHandle *proc;
};

/* 128MB, default stacksize of VE thread */
#define VEO_DEFAULT_STACKSIZE 0x8000000
/* PTHREAD_STACK_MIN in VE-glibc */
#define VEO_STACK_MIN (4 * 1024 * 1024)

class ThreadContextAttr {
private:
  size_t stacksize;

public:
  ThreadContextAttr();
  ~ThreadContextAttr() {};

  void setStacksize(size_t);
  size_t getStacksize() { return this->stacksize;}

  veo_thr_ctxt_attr *toCHandle() {
    return reinterpret_cast<veo_thr_ctxt_attr *>(this);
  }
};

} // namespace veo
#endif
