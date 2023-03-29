/**
 * @file Command.hpp
 * @brief classes for communication between main and pseudo thread
 *
 * @internal
 * @author VEO
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
 * Command queues class definitions.
 * 
 * Copyright (c) 2018-2021 NEC Corporation
 * Copyright (c) 2020-2021 Erich Focht
 */
#ifndef _VEO_COMMAND_HPP_
#define _VEO_COMMAND_HPP_
#include <queue>
#include <memory>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <unordered_map>
#include <urpc.h>
#include "ve_offload.h"

namespace veo {

/**
 * @brief base class of command handled by pseudo thread
 *
 * a command is to be implemented as a function object inheriting Command
 * (see CommandImpl in Context.cpp).
 */
class Command {
private:
  uint64_t msgid;/*! message ID */
  uint64_t retval;/*! returned value from the function on VE */
  int64_t urpc_req;/*! URPC request ID, assigned once in-flight */
  int status;
  bool nowait=false;

public:
  explicit Command(uint64_t id): msgid(id) {}
  Command() = delete;
  Command(const Command &) = delete;
  virtual ~Command() {}
  virtual int operator()() = 0;
  virtual int operator()(urpc_mb_t *m, void *payload, size_t plen) = 0;
  void setURPCReq(int64_t req, int s) { this->urpc_req = req; this->status = s; }
  int64_t getURPCReq() { return this->urpc_req; }
  void setResult(uint64_t r, int s) { this->retval = r; this->status = s; }
  uint64_t getID() { return this->msgid; }
  int getStatus() { return this->status; }
  uint64_t getRetval() { return this->retval; }
  bool getNowaitFlag() { return this->nowait; }
  void setNowaitFlag(bool flg) { this->nowait = flg; }
  virtual bool isVH() = 0;
};

typedef std::unique_ptr<Command> CmdPtr;
}

//namespace std {
//    template <>
//    class hash<std::pair<uint64_t&, std::unique_ptr<veo::Command>&> {
//    public:
//      size_t operator()(const std::pair<uint64_t&, std::unique_ptr<veo::Command>&>& x) const{
//            return hash<uint64_t>()(x.first);
//        }
//    };
//}

namespace veo {
class Context;

typedef enum veo_command_state CommandStatus;
typedef enum veo_queue_state QueueStatus;

/**
 * @brief blocking queue used in CommQueue
 */
class BlockingQueue {
private:
  std::deque<CmdPtr > queue;
  CmdPtr tryFindNoLock(uint64_t);

public:
  BlockingQueue() {}
  void push(CmdPtr);
  void push_front(CmdPtr);
  CmdPtr pop();
  CmdPtr tryFind(uint64_t);
  CmdPtr wait(uint64_t);
  CmdPtr popNoWait();
  void waitIfEmpty();
  bool empty() {
    return this->queue.empty();
  };
  int size() {
    return queue.size();
  };
};

/**
 * @brief blocking map used in CommQueue
 */
class BlockingMap {
private:
  std::mutex mtx;
  std::condition_variable cond;
  std::unordered_map<uint64_t, CmdPtr> map;

public:
  BlockingMap() {}
  void insert(CmdPtr cmd) {
    std::lock_guard<std::mutex> lock(this->mtx);
    auto id = cmd->getID();
    this->map.insert(std::make_pair(id, std::move(cmd)));
    this->cond.notify_all();
  };
  void insert(uint64_t id, CmdPtr cmd) {
    std::lock_guard<std::mutex> lock(this->mtx);
    this->map.insert(std::make_pair(id, std::move(cmd)));
    this->cond.notify_all();
  };
  void insert(int64_t id, CmdPtr cmd) {
    std::lock_guard<std::mutex> lock(this->mtx);
    this->map.insert(std::make_pair((uint64_t)id, std::move(cmd)));
    this->cond.notify_all();
  };
  CmdPtr tryFind(uint64_t id) {
    std::lock_guard<std::mutex> lock(this->mtx);
    return this->tryFindNoWait(id);
  };
  CmdPtr tryFindNoWait(uint64_t id) {
    auto got = this->map.find(id);
    if (got == this->map.end())
      return nullptr;
    auto rv = std::move(got->second);
    this->map.erase(id);
    return rv;
  };
  CmdPtr tryFind(int64_t id) {
    return this->tryFind((uint64_t)id);
  };
  CmdPtr wait(uint64_t id) {
    for (;;) {
      std::unique_lock<std::mutex> lock(this->mtx);
      auto rv = this->tryFindNoWait(id);
      if (rv != nullptr)
        return rv;
      this->cond.wait(lock);
    }
  };
  CmdPtr popNoWait() {
    std::lock_guard<std::mutex> lock(this->mtx);
    if (!this->map.empty()) {
      auto it = map.begin();
      auto id = it->first;
      auto rv = std::move(it->second);
      map.erase(id);
      return rv;
    }
    return nullptr;
  };
  
  bool empty() {
    std::lock_guard<std::mutex> lock(this->mtx);
    return this->map.empty();
  };
  int size() {
    std::lock_guard<std::mutex> lock(this->mtx);
    return map.size();
  };
};

/**
 * @brief high level request handling queue
 */
class CommQueue {
private:
  BlockingQueue request;/*! request queue: for async calls */
  BlockingMap inflight;/*! reqs that have been submitted to URPC */
  BlockingMap completion;/*! completion map: finished reqs picked up from URPC */
  bool terminateFlag = false;/*!A flag to terminate a thread waiting on
			       req_fli_cond */
  std::condition_variable req_fli_cond;/*! wait for request and inflight */
  std::mutex req_fli_mtx;/*! protect request, inflight, terminate, req_fli_cond  */

public:
  CommQueue() {};

  int pushRequest(CmdPtr);
  void pushRequestFront(CmdPtr);
  CmdPtr tryPopRequest();
  bool waitRequest();
  bool emptyRequest() {
    std::lock_guard<std::mutex> lock(this->req_fli_mtx);
    return this->request.empty();
  }
  void pushInFlight(CmdPtr);
  bool emptyInFlight() {
    std::lock_guard<std::mutex> lock(this->req_fli_mtx);
    return this->inflight.empty();
  }
  bool isActive() {
    std::unique_lock<std::mutex> lock(this->req_fli_mtx);
    return !this->inflight.empty() || !this->request.empty();
  }
  CmdPtr popInFlight(int64_t);
  void pushCompletion(CmdPtr);
  CmdPtr waitCompletion(uint64_t msgid);
  CmdPtr peekCompletion(uint64_t msgid);
  void cancelAll();
  void notifyAll();
  void notifyAllForce();
  void terminate();
};
} // namespace veo
#endif
