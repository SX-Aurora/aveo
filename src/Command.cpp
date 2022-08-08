/**
 * @file Command.cpp
 * @brief implementation of communication between main and pseudo thread
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
 * Command queues methods implementation.
 * 
 * Copyright (c) 2018-2021 NEC Corporation
 * Copyright (c) 2020-2021 Erich Focht
 */
#include "Command.hpp"

namespace veo {

/**
 * @brief push a command to queue
 * @param cmd a pointer to a command to be pushed (sent).
 */
void BlockingQueue::push(CmdPtr cmd) {
  std::lock_guard<std::mutex> lock(this->mtx);
  this->queue.push_back(std::move(cmd));
}

/**
 * @brief push a command back to queue after it was popped
 * @param cmd a pointer to a command to be pushed (sent).
 *
 * Needed when submit to URPC doesn't succeed.
 */
void BlockingQueue::push_front(CmdPtr cmd) {
  std::lock_guard<std::mutex> lock(this->mtx);
  this->queue.push_front(std::move(cmd));
  this->cond.notify_all();
}

/**
 * @brief pop a command from queue
 * @return a pointer to a command to be poped (received).
 *
 * This function gets the first command in the queue.
 * If the queue is empty, this function blocks until a command is pushed.
 */
CmdPtr BlockingQueue::pop() {
  for (;;) {
    std::unique_lock<std::mutex> lock(this->mtx);
    if (!this->queue.empty()) {
      auto rv = std::move(this->queue.front());
      this->queue.pop_front();
      return rv;
    }
    this->cond.wait(lock);
  }
}

/**
 * @brief try to find a command with the specified message ID
 * @param msgid a message ID to find
 * @return a pointer to a command with the specified message ID;
 *         nullptr if a command with the message ID is not found in the queue.
 *
 * This function is expected to be called from a thread holding lock.
 */
CmdPtr BlockingQueue::tryFindNoLock(uint64_t msgid) {
  for (auto &&it = this->queue.begin(); it != this->queue.end(); ++it) {
    if ((*it)->getID() == msgid) {
      auto rv = std::move(*it);
      this->queue.erase(it);
      return rv;
    }
  }
  return nullptr;
}

CmdPtr BlockingQueue::tryFind(uint64_t msgid) {
  std::lock_guard<std::mutex> lock(this->mtx);
  return this->tryFindNoLock(msgid);
}

CmdPtr BlockingQueue::wait(uint64_t msgid) {
  for (;;) {
    std::unique_lock<std::mutex> lock(this->mtx);
    auto rv = this->tryFindNoLock(msgid);
    if (rv != nullptr)
      return rv;
    this->cond.wait(lock);
  }
}

CmdPtr BlockingQueue::popNoWait() {
    std::unique_lock<std::mutex> lock(this->mtx);
    if (!this->queue.empty()) {
      auto command = std::move(this->queue.front());
      this->queue.pop_front();

      return command;
    }
    return nullptr;
}

void BlockingQueue::waitIfEmpty() {
  std::unique_lock<std::mutex> lock(this->mtx);
  if (this->queue.empty()) {
    this->cond.wait(lock);
  }
  return;
}

/**
 * @brief push a command to request queue
 * @param req a pointer to a command to be pushed (sent)
 * @return zero upon pushing a request; one upon not pushing a request
 */
int CommQueue::pushRequest(CmdPtr req)
{
  if( this->request.getStatus() == VEO_QUEUE_READY ){
    this->request.push(std::move(req));
    return 0;
  }
  return 1;
}

void CommQueue::pushRequestFront(CmdPtr req)
{
  this->request.push_front(std::move(req));
}

CmdPtr CommQueue::popRequest()
{
  return this->request.pop();
}

CmdPtr CommQueue::tryPopRequest()
{
  return this->request.popNoWait();
}

void CommQueue::waitRequest()
{
  this->request.waitIfEmpty();
}

void CommQueue::pushInFlight(CmdPtr cmd)
{
  auto req = cmd.get()->getURPCReq();
  this->inflight.insert(req, std::move(cmd));
}

CmdPtr CommQueue::popInFlight(int64_t id)
{
  return this->inflight.tryFind(id);
}

void CommQueue::pushCompletion(CmdPtr req)
{
  this->completion.insert(std::move(req));
}

CmdPtr CommQueue::peekCompletion(uint64_t msgid)
{
  return this->completion.tryFind(msgid);
}

CmdPtr CommQueue::waitCompletion(uint64_t msgid)
{
  return this->completion.wait(msgid);
}

void CommQueue::cancelAll()
{
  for(;;) {
    auto command = this->request.popNoWait();
    if ( command == nullptr )
      break;
    command->setResult(0, VEO_COMMAND_ERROR);
    this->completion.insert(std::move(command));
  }
  for(;;) {
    auto command = this->inflight.popNoWait();
    if ( command == nullptr )
      break;
    command->setResult(0, VEO_COMMAND_ERROR);
    this->completion.insert(std::move(command));
  }
}

int CommQueue::getRequestSize()
{
  return this->request.size();
}

int CommQueue::getInFlightSize()
{
  return this->inflight.size();
}

} // namespace veo
