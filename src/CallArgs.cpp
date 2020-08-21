/**
 * @file CallArgs.cpp
 * @brief implementation of arguments for VEO function
 */

#include "CallArgs.hpp"
#include <cstdio>
#include <cstring>
#include <numeric>
#include <string>
#include "log.h"

namespace veo{
namespace internal {
/* *******************
 * Stack image
 *                    (<-%fp) 176 +8 * n + data_size
 * -------------------
 * Parameter  data on
 *   area      stack
 *            (arg#n)  (176 + 8 * n)(%sp)
 *            (arg#8)  (176 + 8 * 8)(%sp)
 *             ...
 *            (arg#0)   176(%sp)
 * - - - - - - - - - -
 *   RSA
 *
 *  return addr
 *    fp               <-%sp
 * -------------------
 * ******************* */

template<typename T> void set_value(std::string &str, size_t offset, T value)
{
  // assume little endian
  static_assert(std::is_fundamental<T>::value, "T must be fundamental");
  union {
    T value_;
    char image_[sizeof(T)];
  } data;
  data.value_ = value;
  // extend if necessary
  if (str.size() < offset) {
    str.resize(offset);
  }
  // put
  str.replace(offset, sizeof(T), data.image_, sizeof(T));
  // pad for 8 byte-aligned
  if (sizeof(T) % 8 > 0) {
    auto padsize = 8 - (sizeof(T) % 8);
    str.replace(offset + sizeof(T), padsize, padsize, '\0');
  }
}

template<typename T> class ArgType: public ArgBase {
  static_assert(std::is_fundamental<T>::value, "not fundamental type.");
  static_assert(std::is_integral<T>::value, "integer types are supported");
  T value_;
public:
  explicit ArgType(const T arg): value_(arg) {}

  /**
   * @brief a value on register
   * @param sp stack pointer
   * @param n_args the number of arguments
   * @param used_size the size of stack used
   * @return a value to be set to on a register
   */
  int64_t getRegVal(uint64_t sp, int n_args, size_t &used_size) const {
    return static_cast<int64_t>(this->value_);
  }

  void setStackImage(uint64_t sp, std::string &stack, int n,
                     bool &in, bool &out) {
    VEO_TRACE("%s(%#lx, _, %d)", __func__, sp, n);
    out = false;
    in = false;
    if (n < NUM_ARGS_ON_REGISTER)
      return;// do nothing
    in = true;
    static_assert(std::is_fundamental<T>::value && sizeof(T) <= 8,
      "template parameter T must be fundamental");
    auto pos = PARAM_AREA_OFFSET + n * 8;
    set_value(stack, pos, this->value_);
  }

  size_t sizeOnStack() const { return 0;}
  void copyoutFromStackImage(uint64_t sp, const char *img){}// nothing
};

template<> class ArgType<double>: public ArgBase {
  union u {
    double d_;
    int64_t i64_;
    u(double d): d_(d) {}
  } u_;
public:
  explicit ArgType(const double d): u_(d) {}
  int64_t getRegVal(uint64_t sp, int n_args, size_t &used_size) const {
    return this->u_.i64_;
  }
  void setStackImage(uint64_t sp, std::string &stack, int n,
                     bool &in, bool &out) {
    VEO_TRACE("%s(%#lx, _, %d)", __func__, sp, n);
    out = false;
    in = false;
    if (n < NUM_ARGS_ON_REGISTER)
      return;// do nothing
    in = true;
    auto pos = PARAM_AREA_OFFSET + n * 8;
    set_value(stack, pos, this->u_.i64_);
  }
  size_t sizeOnStack() const { return 0;}
  void copyoutFromStackImage(uint64_t sp, const char *img){}// nothing
};

template<> class ArgType<float>: public ArgBase {
  union u {
    float f_[2];
    int64_t i64_;
    u(float f): f_{0.0, f} {}
  } u_;
public:
  explicit ArgType(const float f): u_(f) {}
  int64_t getRegVal(uint64_t sp, int n_args, size_t &used_size) const {
    return this->u_.i64_;
  }
  void setStackImage(uint64_t sp, std::string &stack, int n,
                     bool &in, bool &out) {
    VEO_TRACE("%s(%#lx, _, %d)", __func__, sp, n);
    out = false;
    in = false;
    if (n < NUM_ARGS_ON_REGISTER)
      return;// do nothing
    in = true;
    auto pos = PARAM_AREA_OFFSET + n * 8;
    set_value(stack, pos, this->u_.i64_);
  }

  size_t sizeOnStack() const { return 0;}
  void copyoutFromStackImage(uint64_t sp, const char *img){}// nothing
};

class ArgOnStack: public ArgBase {
  char *buff_;
  size_t len_;
  bool in_;// copy in from VH memory to VE stack
  bool out_;// copy out to VH memory from VE stack
  uint64_t vemva_;
public:
  ArgOnStack(char *buff, size_t len, bool in, bool out): buff_(buff),
    len_(len), in_(in), out_(out) {}

  /**
   * @brief a value on register
   * @param sp stack pointer
   * @param n_args the number of arguments
   * @param used_size the size of stack used
   * @return a value to be set to on a register
   */
  int64_t getRegVal(uint64_t sp, int n_args, size_t &used_size) const {
    return this->vemva_;
  }

  void setStackImage(uint64_t sp, std::string &stack, int n,
                     bool &in, bool &out) {
    VEO_TRACE("%s(%#lx, _, %d)", __func__, sp, n);
    auto oldlen = stack.size();
    this->vemva_ = sp + oldlen;
    in = this->in_;
    out = this->out_;
    if (this->in_) {
      stack.append(this->buff_, this->len_);
    } else {
      stack.append(this->len_, '\0');
    }
    // padding for 8 byte-aligned
    if (this->len_ % 8 > 0) {
      auto n_pad = 8 - (this->len_ % 8);
      stack.append(n_pad, '\0');
    }
    // point the image
    if (n >= NUM_ARGS_ON_REGISTER) {
      set_value(stack, PARAM_AREA_OFFSET + 8 * n, this->vemva_);
    }
  }

  size_t sizeOnStack() const {
    size_t rv = this->len_;
    if (this->len_ % 8 > 0) {
      rv += 8 - (this->len_ % 8);
    }
    return rv;
  }
  void copyoutFromStackImage(uint64_t sp, const char *img) {
    VEO_TRACE("%s(%#012lx, ...)", __func__, sp);
    if (!this->out_)
      return;
    VEO_DEBUG("copy out to VH: offset %#lx -> %p, size = %d",
      this->vemva_ - sp, this->buff_, this->len_);
    std::memcpy(this->buff_, img + (this->vemva_ - sp), this->len_);
  }
};
} // namespace internal

/**
 * @brief push, add at the last, an argument
 * @param val argument value
 */
template <typename T> void CallArgs::push_(T val) {
  this->arguments.push_back(std::unique_ptr<internal::ArgBase>(new internal::ArgType<T>(val)));
}

/**
 * @brief a template function for the set() member function
 * @param argnum argument number
 * @param val argument value
 */
template <typename T> void CallArgs::set_(int argnum, T val) {
  if (this->arguments.size() < argnum + 1) {
    //extend
    this->arguments.resize(argnum + 1);
  }
  this->arguments[argnum] = std::unique_ptr<internal::ArgBase>(new internal::ArgType<T>(val));
}

// force instantiation
template void CallArgs::push_<int64_t>(int64_t);
template void CallArgs::set_<int64_t>(int, int64_t);
template void CallArgs::set_<uint64_t>(int, uint64_t);
template void CallArgs::set_<int32_t>(int, int32_t);
template void CallArgs::set_<uint32_t>(int, uint32_t);
template void CallArgs::set_<int16_t>(int, int16_t);
template void CallArgs::set_<uint16_t>(int, uint16_t);
template void CallArgs::set_<int8_t>(int, int8_t);
template void CallArgs::set_<uint8_t>(int, uint8_t);
template void CallArgs::set_<double>(int, double);
template void CallArgs::set_<float>(int, float);

/**
 * @brief set an argument on stack
 * @param inout direction of data transfer
 * @param argnum argument number
 * @param buff pointer to memory buffer on VH
 * @param len length of memory buffer on VH
 */
void CallArgs::setOnStack(enum veo_args_intent inout, int argnum,
                               char *buff, size_t len) {
  bool copiedin = (inout == VEO_INTENT_IN || inout == VEO_INTENT_INOUT);
  bool copiedout = (inout == VEO_INTENT_OUT || inout == VEO_INTENT_INOUT);
  if (this->arguments.size() < argnum + 1) {
    //extend
    this->arguments.resize(argnum + 1);
  }
  this->arguments[argnum] = std::unique_ptr<internal::ArgBase>(new internal::ArgOnStack(buff, len, copiedin, copiedout));
}

/**
 * @brief get a value on register
 * @param sp stack pointer
 * @return registar arguments
 */
std::vector<uint64_t> CallArgs::getRegVal(uint64_t sp) const {
  size_t stack_consumed = 0;
  std::vector<uint64_t> rv;
  int count = 0;
  for (auto &arg: this->arguments) {
    rv.push_back(arg->getRegVal(sp, this->numArgs(), stack_consumed));
    if (++count >= NUM_ARGS_ON_REGISTER)
      break;
  }
  return rv;
}

/**
 * @brief get the stack image
 * @param[in,out] sp reference to stack pointer
 * @return stack image
 */
std::string CallArgs::getStackImage(uint64_t sp) {
  VEO_TRACE("getStackImage(%#lx)", sp);
  // allocate stack
  size_t stack_size = PARAM_AREA_OFFSET + 8 * this->numArgs()
    + std::accumulate(this->arguments.begin(), this->arguments.end(), 0,
                      [](size_t s, decltype(this->arguments)::reference arg) {
                        return s + arg->sizeOnStack();
                      });
  // 16 byte align
  stack_size = ((stack_size + 15) / 16) * 16;
  VEO_TRACE("stack size = %lu", stack_size);
  this->stack_size = stack_size;
  this->stack_top = sp - stack_size;
  std::string stack(PARAM_AREA_OFFSET + 8 * this->numArgs(), '\0');

  int n = 0;
  this->copied_in = false;
  this->copied_out = false;
  for (const auto &arg: this->arguments) {
    bool i, o;
    arg->setStackImage(this->stack_top, stack, n++, i, o);
    this->copied_in = this->copied_in || i;
    this->copied_out = this->copied_out || o;
  }
  int grow = ((stack.size() + 15) / 16) * 16 - stack.size();
  if (grow)
    stack.append(grow, '\0');
  return stack;
}

void CallArgs::setup(uint64_t sp)
{
  VEO_TRACE("setup CallArgs (sp = %#lx)...", sp);
  auto img = this->getStackImage(sp);
  VEO_ASSERT(this->stack_size == img.size());
  auto buf = new char[this->stack_size];
  memcpy(buf, img.c_str(), this->stack_size);
  this->stack_buf.reset(buf);
}

void CallArgs::copyin(std::function<int(uint64_t, const void *, size_t)> xfer)
{
  if (this->copied_in) {
    VEO_TRACE("transfer stack image (VH %p -> VE %#lx, %d bytes)",
              this->stack_buf.get(), this->stack_top, this->stack_size);
    xfer(this->stack_top, this->stack_buf.get(), this->stack_size);
  } else {
    VEO_TRACE("the current stack (%#lx) is not copied in.",
              this->stack_top);
  }
}

void CallArgs::copyout()
{
  if (this->copied_out) {
    VEO_TRACE("transfer stack image (VE %#lx -> VH %p, %d bytes)",
              this->stack_top, this->stack_buf.get(), this->stack_size);
    // transfer done by veo_urpc
    //xfer(this->stack_buf.get(), this->stack_top, this->stack_size);
    for (auto &arg: this->arguments) {
      arg->copyoutFromStackImage(this->stack_top, this->stack_buf.get());
    }
  } else {
    VEO_TRACE("the current stack (%#lx) is not copied out.",
              this->stack_top);
  }
}
} // namespace veo
