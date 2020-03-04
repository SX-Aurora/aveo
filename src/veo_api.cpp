#include "ve_offload.h"

#include <cstdio>
#include <cstdlib>
#include <vector>
#include <string>
#include <sstream>
#include <stdexcept>
#include <cstring>
#include <cstddef>

#include "ProcHandle.hpp"
#include "CallArgs.hpp"
#include "VEOException.hpp"
#include "log.hpp"

namespace veo {
namespace api {
ProcHandle *ProcHandleFromC(veo_proc_handle *h)
{
  return reinterpret_cast<ProcHandle *>(h);
}
ThreadContext *ThreadContextFromC(veo_thr_ctxt *c)
{
  return reinterpret_cast<ThreadContext *>(c);
}
CallArgs *CallArgsFromC(veo_args *a)
{
  return reinterpret_cast<CallArgs *>(a);
}

template <typename T> int veo_args_set_(veo_args *ca, int argnum, T val)
{
  try {
    CallArgsFromC(ca)->set(argnum, val);
    return 0;
  } catch (VEOException &e) {
    VEO_ERROR(nullptr, "failed to set the argument #%d <- %ld: %s",
              argnum, val, e.what());
    return -1;
  }
}
} // namespace veo::api
} // namespace veo

using veo::api::ProcHandleFromC;
using veo::api::ThreadContextFromC;
using veo::api::CallArgsFromC;
using veo::api::veo_args_set_;
using veo::VEOException;

// implementation of VEO API functions
/**
 * \defgroup veoapi VEO API
 *
 * VE Offloading API functions.
 * To use VEO API functions, include "ve_offload.h" header.
 */
//@{
/**
 * @brief return the API version of the VE Offload implementation
 *
 * @retval integer value with API version
 */
int veo_api_version()
{
  return VEO_API_VERSION;
}

/**
 * @brief create a VE process with non-default veorun binary
 *
 * A VE process is created on the VE node specified by venode. If
 * venode is -1, a VE process is created on the VE node specified by
 * environment variable VE_NODE_NUMBER. If venode is -1 and
 * environment variable VE_NODE_NUMBER is not set, a VE process is
 * created on the VE node #0
 *
 * A user executes the program which invokes this function through the
 * job scheduler, the value specified by venode will be treated as a
 * logical VE node number. It will be translated into physical VE node
 * number assigned by the job scheduler. If venode is -1, the first VE
 * node of the VE nodes assigned by the job scheduler is used.
 *
 * @param venode VE node number
 * @param veobin VE alternative veorun binary path
 * @return pointer to VEO process handle upon success
 * @retval NULL VE process creation failed.
 */
veo_proc_handle *veo_proc_create_static(int venode, char *veobin)
{
  if (venode < -1) {
    VEO_ERROR(nullptr, "venode(%d) is an invalid value.", venode);
    return NULL;
  }
  try {
    const char *venodelist = getenv("_VENODELIST");
    if (venodelist != nullptr) {
      // _VENODELIST is set
      std::string str = std::string(venodelist);
      std::vector<std::string> v;
      std::stringstream ss(str);
      std::string buffer;
      char sep = ' ';
      while(std::getline(ss, buffer, sep)) {
        v.push_back(buffer);
      }
      if (venode == -1) {
        const char *venodenum = getenv("VE_NODE_NUMBER");
        if (venodenum != nullptr) {
          // If VE_NODE_NUMBER is set, check is's value is in _VENODELIST
          venode = stoi(std::string(venodenum));
          int found = 0;
          for (unsigned int i; i < v.size(); i++) {
            if (stoi(v[i]) == venode) {
              found = 1;
              break;
            }
          }
          if (found == 0) {
            VEO_ERROR(nullptr, "VE node #%d is not assigned by the scheduler",
                      venode);
            return NULL;
          }
        } else {
          // If VE_NODE_NUMBER is not set, use the first value in _VENODELIST
          if (v.size() > 0) 
            venode = stoi(v[0]);
          else { 
            VEO_ERROR(nullptr, "_VENODELIST is empty.", NULL);
            return NULL;
          }
        }
      } else {
	// translate venode using _VENODELIST
        if ((int)v.size() <= venode) {
          VEO_ERROR(nullptr, "venode = %d exceeds the size of _VENDOELIST %d", venode, v.size());
          return NULL;
        }
        venode = stoi(v[venode]);
      }
    } else {
      // _VENODELIST is not set
      if (venode == -1) {
        const char *venodenum = getenv("VE_NODE_NUMBER");
        if (venodenum != nullptr) {
          venode = stoi(std::string(venodenum));
        } else {
          venode = 0;
        }
      }
    }

    auto rv = new veo::ProcHandle(venode, veobin);
    return rv->toCHandle();
    
  } catch (std::invalid_argument &e) {
    VEO_ERROR(nullptr, "failed to create proc: %s", e.what());
    return 0;
  } catch (std::out_of_range &e) {
    VEO_ERROR(nullptr, "failed to create proc: %s", e.what());
    return 0;
  }
}
 
/**
 * @brief create a VE process
 *
 * A VE process is created on the VE node specified by venode. If
 * venode is -1, a VE process is created on the VE node specified by
 * environment variable VE_NODE_NUMBER. If venode is -1 and
 * environment variable VE_NODE_NUMBER is not set, a VE process is
 * created on the VE node #0
 *
 * A user executes the program which invokes this function through the
 * job scheduler, the value specified by venode will be treated as a
 * logical VE node number. It will be translated into physical VE node
 * number assigned by the job scheduler. If venode is -1, the first VE
 * node of the VE nodes assigned by the job scheduler is used.
 *
 * @param venode VE node number
 * @return pointer to VEO process handle upon success
 * @retval NULL VE process creation failed.
 */
veo_proc_handle *veo_proc_create(int venode)
{
  char *veobin = getenv("VEORUN_BIN");
  if (veobin != nullptr)
    return veo_proc_create_static(venode, veobin);
  else
    return veo_proc_create_static(venode, (char *)VEORUN_BIN);
}

/**
 * @brief destroy a VE process
 * @param proc pointer to VEO process handle
 * @retval 0 VEO process handle is successfully destroyed.
 * @retval -1 VEO process handle destruction failed.
 */
int veo_proc_destroy(veo_proc_handle *proc)
{
  return ProcHandleFromC(proc)->exitProc();
}

/**
 * @brief load a VE library
 * @param proc VEO process handle
 * @param libname a library file name to load
 * @return a handle for the library
 * @retval 0 library loading request failed.
 */
uint64_t veo_load_library(veo_proc_handle *proc, const char *libname)
{
  try {
    return ProcHandleFromC(proc)->loadLibrary(libname);
  } catch (VEOException &e) {
    VEO_ERROR(nullptr, "failed to load library: %s", e.what());
    errno = e.err();
    return 0;
  }
}

/**
 * @brief find a symbol in VE program
 * @param proc VEO process handle
 * @param libhdl a library handle
 * @param symname symbol name to find
 * @return VEMVA of the symbol upon success.
 * @retval 0 failed to find symbol.
 */
uint64_t veo_get_sym(veo_proc_handle *proc, uint64_t libhdl,
                     const char *symname)
{
  try {
    return ProcHandleFromC(proc)->getSym(libhdl, symname);
  } catch (VEOException &e) {
    VEO_ERROR(nullptr, "failed to get symbol: %s", e.what());
    errno = e.err();
    return 0;
  }
}

/**
 * @brief Allocate a VE memory buffer
 *
 * @param h VEO process handle
 * @param addr [out] VEMVA address
 * @param size [in] size in bytes
 * @retval 0 memory allocation succeeded.
 * @retval -1 memory allocation failed.
 * @retval -2 internal error.
 */
int veo_alloc_mem(veo_proc_handle *h, uint64_t *addr, const size_t size)
{
  try {
    *addr = ProcHandleFromC(h)->allocBuff(size);
    if (*addr == 0UL)
      return -1;
  } catch (VEOException &e) {
    return -2;
  }
  return 0;
}

/**
 * @brief Free a VE memory buffer
 *
 * @param h VEO process handle
 * @param addr [in] VEMVA address
 * @retval 0 memory is successfully freed.
 * @retval -1 internal error.
 */
int veo_free_mem(veo_proc_handle *h, uint64_t addr)
{
  try {
    ProcHandleFromC(h)->freeBuff(addr);
  } catch (VEOException &e) {
    return -1;
  }
  return 0;
}

/**
 * @brief Read VE memory
 *
 * @param h VEO process handle
 * @param dst destination VHVA
 * @param src source VEMVA
 * @param size size in byte
 * @return zero upon success; negative upon failure.
 */
int veo_read_mem(veo_proc_handle *h, void *dst, uint64_t src, size_t size)
{
  try {
    return ProcHandleFromC(h)->readMem(dst, src, size);
  } catch (VEOException &e) {
    return -1;
  }
}

/**
 * @brief Write VE memory
 *
 * @param h VEO process handle
 * @param dst destination VEMVA
 * @param src source VHVA
 * @param size size in byte
 * @return zero upon success; negative upon failure.
 */
int veo_write_mem(veo_proc_handle *h, uint64_t dst, const void *src,
                  size_t size)
{
  try {
    return ProcHandleFromC(h)->writeMem(dst, src, size);
  } catch (VEOException &e) {
    return -1;
  }
}

/**
 * @brief Return number of open contexts in a proc
 *
 * @param h VEO process handle
 * @return number of open contexts.
 */
int veo_num_contexts(veo_proc_handle *h)
{
  return ProcHandleFromC(h)->numContexts();
}

veo_thr_ctxt *veo_get_context(veo_proc_handle *proc, int idx)
{
  try {
    veo_thr_ctxt *ctx = ProcHandleFromC(proc)->getContext(idx)->toCHandle();
    auto rv = reinterpret_cast<intptr_t>(ctx);
    if ( rv < 0 ) {
      errno = -rv;
      return NULL;
    }
    return ctx;
  } catch (VEOException &e) {
    VEO_ERROR(nullptr, "failed to retrieve context %d: %s", idx, e.what());
    errno = e.err();
    return NULL;
  }
}

/**
 * @brief open a VEO context
 *
 * Create a new VEO context, a pseudo thread and VE thread for the context.
 *
 * @param proc VEO process handle
 * @return a pointer to VEO thread context upon success.
 * @retval NULL failed to create a VEO context.
 */
veo_thr_ctxt *veo_context_open(veo_proc_handle *proc)
{
  try {
    veo_thr_ctxt *ctx = ProcHandleFromC(proc)->openContext()->toCHandle();
    auto rv = reinterpret_cast<intptr_t>(ctx);
    if ( rv < 0 ) {
      errno = -rv;
      return NULL;
    }
    return ctx;
  } catch (VEOException &e) {
    VEO_ERROR(nullptr, "failed to open context: %s", e.what());
    errno = e.err();
    return NULL;
  }
}

/**
 * @brief close a VEO context
 *
 * @param ctx a VEO context to close
 * @retval 0 VEO context is successfully closed.
 * @retval non-zero failed to close VEO context.
 */
int veo_context_close(veo_thr_ctxt *ctx)
{
  auto c = ThreadContextFromC(ctx);
  if (c->isMain()) {
    return 0;
  }
  veo::ProcHandle *p = c->proc;
  p->delContext(c);
  return 0;
}

/**
 * @brief synchronize a VEO context
 *
 * While the submission lock is held, wait until all
 * commands have been processed in this context.
 *
 * @param ctx the VEO context to synchronize
 */
void veo_context_sync(veo_thr_ctxt *ctx)
{
  auto c = ThreadContextFromC(ctx);
  c->synchronize();
}

/**
 * @brief get VEO context state
 *
 * @return the state of the VEO context state.
 * @retval VEO_STATE_RUNNING VEO context is running.
 * @retvav VEO_STATE_EXIT VEO context  exited.
 */
int veo_get_context_state(veo_thr_ctxt *ctx)
{
  return ThreadContextFromC(ctx)->getState();
}

/**
 * @brief allocate VEO arguments object (veo_args)
 *
 * @return pointer to veo_args
 * @retval NULL the allocation of veo_args failed.
 */
veo_args *veo_args_alloc(void)
{
  try {
    auto rv = new veo::CallArgs();
    return rv->toCHandle();
  } catch (VEOException &e) {
    errno = e.err();
    return NULL;
  }
}

/**
 * @brief set a 64-bit integer argument
 *
 * @param ca veo_args
 * @param argnum the argnum-th argument
 * @param val value to be set
 * @return zero upon success; negative upon failure.
 */
int veo_args_set_i64(veo_args *ca, int argnum, int64_t val)
{
  return veo_args_set_(ca, argnum, val);
}

/**
 * @brief set a 64-bit uunsigned integer argument
 *
 * @param ca veo_args
 * @param argnum the argnum-th argument
 * @param val value to be set
 * @return zero upon success; negative upon failure.
 */
int veo_args_set_u64(veo_args *ca, int argnum, uint64_t val)
{
  return veo_args_set_(ca, argnum, val);
}

/**
 * @brief set a 32-bit integer argument
 *
 * @param ca veo_args
 * @param argnum the argnum-th argument
 * @param val value to be set
 * @return zero upon success; negative upon failure.
 */
int veo_args_set_i32(veo_args *ca, int argnum, int32_t val)
{
  return veo_args_set_(ca, argnum, val);
}

/**
 * @brief set a 32-bit unsigned integer argument
 *
 * @param ca veo_args
 * @param argnum the argnum-th argument
 * @param val value to be set
 * @return zero upon success; negative upon failure.
 */
int veo_args_set_u32(veo_args *ca, int argnum, uint32_t val)
{
  return veo_args_set_(ca, argnum, val);
}

/**
 * @brief set a 16-bit integer argument
 *
 * @param ca veo_args
 * @param argnum the argnum-th argument
 * @param val value to be set
 * @return zero upon success; negative upon failure.
 */
int veo_args_set_i16(veo_args *ca, int argnum, int16_t val)
{
  return veo_args_set_(ca, argnum, val);
}

/**
 * @brief set a 16-bit unsigned integer argument
 *
 * @param ca veo_args
 * @param argnum the argnum-th argument
 * @param val value to be set
 * @return zero upon success; negative upon failure.
 */
int veo_args_set_u16(veo_args *ca, int argnum, uint16_t val)
{
  return veo_args_set_(ca, argnum, val);
}

/**
 * @brief set a 8-bit integer argument
 *
 * @param ca veo_args
 * @param argnum the argnum-th argument
 * @param val value to be set
 * @return zero upon success; negative upon failure.
 */
int veo_args_set_i8(veo_args *ca, int argnum, int8_t val)
{
  return veo_args_set_(ca, argnum, val);
}

/**
 * @brief set a 8-bit unsigned integer argument
 *
 * @param ca veo_args
 * @param argnum the argnum-th argument
 * @param val value to be set
 * @return zero upon success; negative upon failure.
 */
int veo_args_set_u8(veo_args *ca, int argnum, uint8_t val)
{
  return veo_args_set_(ca, argnum, val);
}

/**
 * @brief set a double precision floating point number argument
 *
 * @param ca veo_args
 * @param argnum the argnum-th argument
 * @param val value to be set
 * @return zero upon success; negative upon failure.
 */
int veo_args_set_double(veo_args *ca, int argnum, double val)
{
  return veo_args_set_(ca, argnum, val);
}

/**
 * @brief set a single precision floating point number argument
 *
 * @param ca veo_args
 * @param argnum the argnum-th argument
 * @param val value to be set
 * @return zero upon success; negative upon failure.
 */
int veo_args_set_float(veo_args *ca, int argnum, float val)
{
  return veo_args_set_(ca, argnum, val);
}

/**
 * @brief set VEO function calling argument pointing to buffer on stack
 *
 * @param ca pointer to veo_args object
 * @param inout intent of argument, currently only VEO_INTENT_IN is supported
 * @param argnum argument number that is being set
 * @param buff char pointer to buffer that will be copied to the VE stack
 * @param len length of buffer that is copied to the VE stack
 * @retval  0 argumen is successfully set.
 * @retval -1 an error occurred.
 *
 * The buffer is copied to the stack and will look to the VE callee like a
 * local variable of the caller function. It is currently erased right after
 * the call returns, thus only intent IN args passing is supported. Use this
 * to pass structures to the VE "kernel" function. The size of arguments 
 * passed on the stack is limited to 63MB, since the size of the initial 
 * stack is 64MB. Try staying well below this value, allocate and use memory
 * buffers on heap when you have huge argument arrays to pass.
 */
int veo_args_set_stack(veo_args *ca, enum veo_args_intent inout,
                       int argnum, char *buff, size_t len)
{
  try {
    CallArgsFromC(ca)->setOnStack(inout, argnum, buff, len);
    return 0;
  } catch (VEOException &e) {
    VEO_ERROR(nullptr, "failed set_on_stack CallArgs(%d): %s",
              argnum, e.what());
    return -1;
  }
}

/**
 * @brief clear arguments set in VEO arguments object
 *
 * @param ca veo_args object
 */
void veo_args_clear(veo_args *ca)
{
  CallArgsFromC(ca)->clear();
}

/**
 * @brief free VEO arguments object
 *
 * @param ca veo_args object
 */
void veo_args_free(veo_args *ca)
{
  delete CallArgsFromC(ca);
}

/**
 * @brief Call VE function synchronously on a proc
 *
 * @param h VEO process handle
 * @param addr VEMVA address of VE function
 * @param ca call args
 * @param result pointer to result variable
 * @return zero upon success; negative upon failure.
 */
int veo_call_sync(veo_proc_handle *h, uint64_t addr, veo_args *ca,
                  uint64_t *result)
{
  try {
    return ProcHandleFromC(h)->callSync(addr, *CallArgsFromC(ca), result);
  } catch (VEOException &e) {
    return -1;
  }
}

/**
 * @brief request a VE thread to call a function
 *
 * @param ctx VEO context to execute the function on VE.
 * @param addr VEMVA of the function to call
 * @param args arguments to be passed to the function
 * @return request ID
 * @retval VEO_REQUEST_ID_INVALID request failed.
 */
uint64_t veo_call_async(veo_thr_ctxt *ctx, uint64_t addr, veo_args *args)
{
  try {
    return ThreadContextFromC(ctx)->callAsync(addr, *CallArgsFromC(args));
  } catch (VEOException &e) {
    return VEO_REQUEST_ID_INVALID;
  }
}

/**
 * @brief pick up a resutl from VE function if it has finished
 *
 * @param ctx VEO context
 * @param reqid request ID
 * @param retp pointer to buffer to store the return value from the function.
 * @retval VEO_COMMAND_OK function is successfully returned.
 * @retval VEO_COMMAND_EXCEPTION an exception occurred on function.
 * @retval VEO_COMMAND_ERROR an error occurred on function.
 * @retval VEO_COMMAND_UNFINISHED function is not finished.
 * @retval -1 internal error.
 */
int veo_call_peek_result(veo_thr_ctxt *ctx, uint64_t reqid, uint64_t *retp)
{
  try {
    return ThreadContextFromC(ctx)->callPeekResult(reqid, retp);
  } catch (VEOException &e) {
    return -1;
  }
}

/**
 * @brief pick up a resutl from VE function

 * @param ctx VEO context
 * @param reqid request ID
 * @param retp pointer to buffer to store the return value from the function.
 * @retval VEO_COMMAND_OK function is successfully returned.
 * @retval VEO_COMMAND_EXCEPTION an exception occurred on execution.
 * @retval VEO_COMMAND_ERROR an error occurred on execution.
 * @retval VEO_COMMAND_UNFINISHED function is not finished.
 * @retval -1 internal error.
 */
int veo_call_wait_result(veo_thr_ctxt *ctx, uint64_t reqid, uint64_t *retp)
{
  try {
    return ThreadContextFromC(ctx)->callWaitResult(reqid, retp);
  } catch (VEOException &e) {
    return -1;
  }
}

/**
 * @brief Asynchronously read VE memory
 *
 * @param ctx VEO context
 * @param dst destination VHVA
 * @param src source VEMVA
 * @param size size in byte
 * @return request ID
 * @retval VEO_REQUEST_ID_INVALID request failed.
 */
uint64_t veo_async_read_mem(veo_thr_ctxt *ctx, void *dst, uint64_t src,
                            size_t size)
{
  try {
    return ThreadContextFromC(ctx)->asyncReadMem(dst, src, size);
  } catch (VEOException &e) {
    return VEO_REQUEST_ID_INVALID;
  }
}

/**
 * @brief Asynchronously write VE memory
 *
 * @param ctx VEO context
 * @param dst destination VEMVA
 * @param src source VHVA
 * @param size size in byte
 * @return request ID
 * @retval VEO_REQUEST_ID_INVALID request failed.
 */
uint64_t veo_async_write_mem(veo_thr_ctxt *ctx, uint64_t dst, const void *src,
                             size_t size)
{
  try {
    return ThreadContextFromC(ctx)->asyncWriteMem(dst, src, size);
  } catch (VEOException &e) {
    return VEO_REQUEST_ID_INVALID;
  }
}

/**
 * @brief request a VE thread to call a function
 *
 * @param ctx VEO context to execute the function on VE.
 * @param libhdl a library handle
 * @param symname symbol name to find
 * @param args arguments to be passed to the function
 * @return request ID
 * @retval VEO_REQUEST_ID_INVALID request failed.
 */
uint64_t veo_call_async_by_name(veo_thr_ctxt *ctx, uint64_t libhdl,
                        const char *symname, veo_args *args)
{
  try {
    return ThreadContextFromC(ctx)->callAsyncByName(libhdl, symname, *CallArgsFromC(args));
  } catch (VEOException &e) {
    return VEO_REQUEST_ID_INVALID;
  }
}

/**
 * @brief call a VH function asynchronously
 *
 * @param ctx VEO context in which to execute the function.
 * @param func address of VH function to call
 * @param arg pointer to arguments structure for the function
 * @return request ID
 * @retval VEO_REQUEST_ID_INVALID if request failed.
 */
uint64_t veo_call_async_vh(veo_thr_ctxt *ctx, uint64_t (*func)(void *), void *arg)
{
  try {
    return ThreadContextFromC(ctx)->callVHAsync(func, arg);
  } catch (VEOException &e) {
    return VEO_REQUEST_ID_INVALID;
  }
}

//@}
