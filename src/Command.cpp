/**
 * @file Command.cpp
 * @brief implementation of communication between main and pseudo thread
 */
#include "Command.hpp"

namespace veo {
typedef std::unique_ptr<Command> CmdPtr;

/**
 * @brief push a command to queue
 * @param cmd a pointer to a command to be pushed (sent).
 */
void BlockingQueue::push(CmdPtr cmd) {
  std::lock_guard<std::mutex> lock(this->mtx);
  this->queue.push_back(std::move(cmd));
  this->cond.notify_all();
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

/**
 * @brief push a command to request queue
 * @param req a pointer to a command to be pushed (sent)
 * @return zero upon pushing a request; one upon not pushing a request
 */
int CommQueue::pushRequest(std::unique_ptr<Command> req)
{
  if( this->request.getStatus() == VEO_QUEUE_READY ){
    this->request.push(std::move(req));
    return 0;
  }
  return 1;
}

void CommQueue::pushRequestFront(std::unique_ptr<Command> req)
{
  this->request.push_front(std::move(req));
}

std::unique_ptr<Command> CommQueue::popRequest()
{
  return this->request.pop();
}

std::unique_ptr<Command> CommQueue::tryPopRequest()
{
  return this->request.popNoWait();
}

void CommQueue::pushInFlight(std::unique_ptr<Command> req)
{
  this->inflight.push(std::move(req));
}

std::unique_ptr<Command> CommQueue::popInFlight()
{
  return this->inflight.popNoWait();
}

void CommQueue::pushCompletion(std::unique_ptr<Command> req)
{
  this->completion.insert(std::move(req));
}

std::unique_ptr<Command> CommQueue::peekCompletion(uint64_t msgid)
{
  return this->completion.tryFind(msgid);
}

//std::unique_ptr<Command> CommQueue::waitCompletion(uint64_t msgid)
//{
//  return this->completion.wait(msgid);
//}

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
} // namespace veo
