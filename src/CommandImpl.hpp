#ifndef _COMMAND_IMPL_HPP_
#define _COMMAND_IMPL_HPP_
/**
 * @file CommandImpl.hpp
 * @brief VEO command implementation
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
 * VEO command implementation.
 * 
 * Copyright (c) 2018-2021 NEC Corporation
 * Copyright (c) 2020-2021 Erich Focht
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
#endif
