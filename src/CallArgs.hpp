/**
 * @file CallArgs.hpp
 * @brief VEO function call arguments
 *
 * @internal
 * @author VEO
 */
#ifndef _VEO_CALL_ARGS_HPP_
#define _VEO_CALL_ARGS_HPP_
#include <functional>
#include <memory>
#include <string>
#include <vector>
#include <type_traits>
#include <initializer_list>
#include "ve_offload.h"
#include "VEOException.hpp"

namespace veo {
constexpr int NUM_ARGS_ON_REGISTER = 8;
constexpr int PARAM_AREA_OFFSET = 176;
namespace internal {
/**
 * Base abstract class of an argument
 */
struct ArgBase {
  virtual ~ArgBase() = default;
  virtual int64_t getRegVal(uint64_t, int, size_t &) const = 0;
  virtual size_t sizeOnStack() const = 0;
  virtual void setStackImage(uint64_t, std::string &, int, bool &, bool &) = 0;
  virtual void copyoutFromStackImage(uint64_t, const char *) = 0;
};
} // namespace internal

class CallArgs {
  std::vector<std::unique_ptr<internal::ArgBase> > arguments;
  template<typename T> void push_(T val);
  template<typename T> void set_(int argnum, T val);

  std::string getStackImage(uint64_t);

public:
  uint64_t stack_top;
  size_t stack_size;
  std::unique_ptr<char[]> stack_buf;

  bool copied_in;// necessary to copy stack image to VE
  bool copied_out;// necessary to copy stack image out from VE

  CallArgs(): arguments(0) {}
  CallArgs(std::initializer_list<int64_t> args) {
    for (auto a: args)
      this->push_(a);
  }
  ~CallArgs() = default;
  CallArgs(const CallArgs &) = delete;

  /**
   * @brief clear all aruguments
   */
  void clear() {
    this->arguments.clear();
  }

  /**
   * @brief set an argument for VEO function
   * @param argnum argument number
   * @param val argument value
   * TODO: support more types
   */
  template <typename T> void set(int argnum, T val) {
    // TODO: trace
    this->set_(argnum, val);
  }

  void setOnStack(enum veo_args_intent inout, int argnum,
                  char *buff, size_t len);

  /**
   * @brief number of arguments for VEO function
   */
  int numArgs() const {
    return this->arguments.size();
  }

  std::vector<uint64_t> getRegVal(uint64_t) const;

  void setup(uint64_t);
  void copyin(std::function<int(uint64_t, const void *, size_t)>);
  void copyout();

  veo_args *toCHandle() {
    return reinterpret_cast<veo_args *>(this);
  }
};
}
#endif
