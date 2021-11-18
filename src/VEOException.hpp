#ifndef _VEO_EXCEPTION_HPP_
#define _VEO_EXCEPTION_HPP_
/**
 * @file VEOException.hpp
 * @brief Exception
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
 * VEO specific exception class.
 * 
 * Copyright (c) 2018-2021 NEC Corporation
 */
#include <stdexcept>
#include <cerrno>

namespace veo {
/**
 * @brief exception class used in VEO
 */
class VEOException: public std::runtime_error {
  int errno_;//!< saved errno
public:
  VEOException(const char *msg, int e): runtime_error(msg), errno_(e) {}
  explicit VEOException(const char *msg): runtime_error(msg), errno_(errno) {}
  /**
   * @brief get the saved errno value
   */
  int err() { return this->errno_; }
};
} // namespace veo
#endif
