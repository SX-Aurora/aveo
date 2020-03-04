/**
 * @file VEOException.hpp
 * @brief Exception
 */
#ifndef _VEO_EXCEPTION_HPP_
#define _VEO_EXCEPTION_HPP_
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
