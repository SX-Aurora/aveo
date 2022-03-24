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
  std::mutex mtx;
  std::condition_variable cond;
  std::deque<std::unique_ptr<Command> > queue;
  std::unique_ptr<Command> tryFindNoLock(uint64_t);
  std::atomic<QueueStatus> queue_state;

public:
  BlockingQueue() : queue_state(VEO_QUEUE_READY) {}
  void push(std::unique_ptr<Command>);
  void push_front(std::unique_ptr<Command>);
  std::unique_ptr<Command> pop();
  std::unique_ptr<Command> tryFind(uint64_t);
  std::unique_ptr<Command> wait(uint64_t);
  std::unique_ptr<Command> popNoWait();
  void setStatus(QueueStatus s) { this->queue_state.store(s); }
  QueueStatus getStatus() { return this->queue_state.load(); }
  bool empty() { return this->queue.empty(); }
};

/**
 * @brief blocking map used in CommQueue
 */
class BlockingMap {
private:
  std::mutex mtx;
  std::condition_variable cond; // Note: not needed any more, since we busy wait, maybe later
  std::unordered_map<uint64_t, std::unique_ptr<Command>> map;

public:
  BlockingMap() {}
  void insert(std::unique_ptr<Command> cmd) {
    std::lock_guard<std::mutex> lock(this->mtx);
    auto id = cmd->getID();
    this->map.insert(std::make_pair(id, std::move(cmd)));
  };
  void insert(uint64_t id, std::unique_ptr<Command> cmd) {
    std::lock_guard<std::mutex> lock(this->mtx);
    this->map.insert(std::make_pair(id, std::move(cmd)));
  };
  void insert(int64_t id, std::unique_ptr<Command> cmd) {
    std::lock_guard<std::mutex> lock(this->mtx);
    this->map.insert(std::make_pair((uint64_t)id, std::move(cmd)));
  };
  std::unique_ptr<Command> tryFind(uint64_t id) {
    std::lock_guard<std::mutex> lock(this->mtx);
    auto got = this->map.find(id);
    if (got == this->map.end())
      return nullptr;
    auto rv = std::move(got->second);
    this->map.erase(id);
    return rv;
  };
  std::unique_ptr<Command> tryFind(int64_t id) {
    return this->tryFind((uint64_t)id);
  };
  std::unique_ptr<Command> popNoWait() {
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
  
  bool empty() { return this->map.empty(); }
};

/**
 * @brief high level request handling queue
 */
class CommQueue {
private:
  BlockingQueue request;/*! request queue: for async calls */
  BlockingMap inflight;/*! reqs that have been submitted to URPC */
  BlockingMap completion;/*! completion map: finished reqs picked up from URPC */

public:
  CommQueue() {};

  int pushRequest(std::unique_ptr<Command>);
  void pushRequestFront(std::unique_ptr<Command>);
  std::unique_ptr<Command> popRequest();
  std::unique_ptr<Command> tryPopRequest();
  bool emptyRequest() { return this->request.empty(); }
  void pushInFlight(std::unique_ptr<Command>);
  bool emptyInFlight() { return this->inflight.empty(); }
  std::unique_ptr<Command> popInFlight(int64_t);
  void pushCompletion(std::unique_ptr<Command>);
  //std::unique_ptr<Command> waitCompletion(uint64_t msgid);
  std::unique_ptr<Command> peekCompletion(uint64_t msgid);
  void cancelAll();
  void setRequestStatus(QueueStatus s){ this->request.setStatus(s); }
};
} // namespace veo
#endif
