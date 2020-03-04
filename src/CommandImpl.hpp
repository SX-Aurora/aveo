/**
 * @file CommandImpl.hpp
 * @brief VEO command implementation
 */
#include <functional>
#include "Command.hpp"

namespace veo {
namespace internal {
/**
 * @brief a command handled by pseudo thread
 */
class CommandImpl: public Command {
public:
  using proctype = std::function<int(Command *)>;
  using unpacktype = std::function<int(Command *, urpc_mb_t *, void *, size_t)>;
private:
  bool is_vh;
  proctype handler;
  unpacktype unpack;
public:
  CommandImpl(uint64_t id, proctype h):
    handler(h), unpack(nullptr), is_vh(true), Command(id) {}
  CommandImpl(uint64_t id, proctype h, unpacktype u):
    handler(h), unpack(u), is_vh(false), Command(id) {}
  int operator()() {
    return this->handler(this);
  }
  int operator()(urpc_mb_t *m, void *payload, size_t plen) {
    return this->unpack(this, m, payload, plen);
  }
  bool isVH() { return this->is_vh; }
  CommandImpl() = delete;
  CommandImpl(const CommandImpl &) = delete;
};

} // namespace internal
} // namespace veo
