#ifndef _VEO_API_HPP_
#define _VEO_API_HPP_

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

namespace veo {
namespace api {
inline ProcHandle *ProcHandleFromC(veo_proc_handle *h)
{
  return reinterpret_cast<ProcHandle *>(h);
}
inline Context *ContextFromC(veo_thr_ctxt *c)
{
  return reinterpret_cast<Context *>(c);
}
inline CallArgs *CallArgsFromC(veo_args *a)
{
  return reinterpret_cast<CallArgs *>(a);
}
inline ThreadContextAttr *ThreadContextAttrFromC(veo_thr_ctxt_attr *ta)
{
  return reinterpret_cast<ThreadContextAttr *>(ta);
}

template <typename T> int veo_args_set_(veo_args *ca, int argnum, T val)
{
  try {
    CallArgsFromC(ca)->set(argnum, val);
    return 0;
  } catch (VEOException &e) {
    VEO_ERROR("failed to set the argument #%d <- %ld: %s",
              argnum, val, e.what());
    return -1;
  }
}

inline uint64_t callSyscallWrapper(veo_proc_handle *h, veo_args *ca)
{
        uint64_t result = (uint64_t)-1;
        // get library handle and symbol address
        uint64_t libh = ProcHandleFromC(h)->loadVE2VELibrary(LIBVEOHMEM);
        if (libh == 0UL) {
                errno = ENOTSUP;
                return -1;
        }
        uint64_t sym_addr = ProcHandleFromC(h)->getSym(libh, "syscall_wrapper");
        if (sym_addr == 0UL) {
                errno = ENOTSUP;
                return -1;
        }

        int ret = ProcHandleFromC(h)->callSync(sym_addr, *CallArgsFromC(ca), &result);
        if (ret != VEO_COMMAND_OK) {
                errno = EBUSY;
                return -1;
        }
        if (-4096 < (int)result && (int)result < -1) {
                VEO_ERROR("syscall returns errno(%d)", errno);
                errno = std::abs((int)result);
                return (uint64_t)-1;
        }
        return result;
}
} // namespace veo::api
} // namespace veo

#endif
