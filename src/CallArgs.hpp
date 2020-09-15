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
  virtual int64_t getRegVal() const = 0;
  virtual size_t sizeOnStack() const = 0;
  virtual void setStackImage(uint64_t, std::string &, int, bool &, bool &) = 0;
  virtual std::function<void(void *)> copyoutFromStackImage(uint64_t) = 0;
};
} // namespace internal

#if 0
/**
 * @brief Arguments structure used in submitting a command
 */
struct SendArgs {
  std::vector<uint64_t> regs;
  uint64_t stack_top;
  size_t stack_size;
  bool copyin;
  bool copyout;
  std::unique_ptr<char[]> stack_buf;

  SendArgs(uint64_t stack_top_, size_t stack_size_, bool copyin_,
           bool copyout_)
    : stack_top(stack_top_), stack_size(stack_size_), copyin(copyin_),
      copyout(copyout_) {}
};
#endif

/**
 * @brief Class used for constructing the arguments of a call
 */
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
   * @brief clear all arguments
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

  std::vector<uint64_t> getRegVal();

  void setup(uint64_t);

  /*
  SendArgs *send_args() {
    SendArgs sa(this->stack_top, this->stack_size,
                this->copied_in, this->copied_out);
    sa.regs = this->getRegVal();
    sa.stack_buf = std::move(this->stack_buf);
    return &sa;
  }
  */
  /**
   * @brief create lambda functions for copying stack passed data out
   */
  std::function<void(void *)> copyout();

  veo_args *toCHandle() {
    return reinterpret_cast<veo_args *>(this);
  }
};

}
#endif
