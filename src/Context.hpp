/**
 * @file Context.hpp
 * @brief VEO thread context
 */
#ifndef _VEO_THREAD_CONTEXT_HPP_
#define _VEO_THREAD_CONTEXT_HPP_

#include "Command.hpp"
#include <mutex>
#include <unordered_set>
#include <pthread.h>
#include <semaphore.h>

#include "log.h"
#include <urpc.h>
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
  std::mutex submit_mtx;//!< for synchronous calls prohibit submission of new reqs
  std::recursive_mutex prog_mtx;	// ensure that progress is not called concurrently

  void _progress_nolock(int ops);
  void progress(int ops);
  void _synchronize_nolock();
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

  uint64_t simpleCallAsync(uint64_t, CallArgs &);
  uint64_t doCallAsync(uint64_t, CallArgs &);

  // handlers for commands
  int _readMem(void *, uint64_t, size_t);
  int _writeMem(uint64_t, const void *, size_t);
  uint64_t _callOpenContext(ProcHandle *, uint64_t, CallArgs &);
  void _delFromProc();

public:
  Context(ProcHandle *, urpc_peer_t *up, bool is_main);
  Context(ProcHandle *);
  ~Context() {}
  Context(const Context &) = delete;//non-copyable
  veo_context_state getState() { return this->state; }
  int callSync(uint64_t addr, CallArgs &arg, uint64_t *result);
  uint64_t callAsync(uint64_t, CallArgs &);
  uint64_t callAsyncByName(uint64_t, const char *, CallArgs &);
  uint64_t callVHAsync(uint64_t (*)(void *), void *);
  int callWaitResult(uint64_t, uint64_t *);
  int callPeekResult(uint64_t, uint64_t *);
  void synchronize();

  uint64_t sendbuffAsync(uint64_t dst, void *src, size_t size);
  uint64_t recvbuffAsync(void *dst, uint64_t src, size_t size);
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
