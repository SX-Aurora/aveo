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
#include "veo_hmem.h"
#include "veo_hmem_macros.h"

namespace veo {
namespace api {
ProcHandle *ProcHandleFromC(veo_proc_handle *h)
{
  return reinterpret_cast<ProcHandle *>(h);
}
Context *ContextFromC(veo_thr_ctxt *c)
{
  return reinterpret_cast<Context *>(c);
}
CallArgs *CallArgsFromC(veo_args *a)
{
  return reinterpret_cast<CallArgs *>(a);
}
ThreadContextAttr *ThreadContextAttrFromC(veo_thr_ctxt_attr *ta)
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
} // namespace veo::api
} // namespace veo

extern "C" { extern const char *AVEO_VERSION; }

using veo::api::ProcHandleFromC;
using veo::api::ContextFromC;
using veo::api::CallArgsFromC;
using veo::api::ThreadContextAttrFromC;
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
 * @brief VEO version
 *
 * @return pointer to VEO version string
 */
const char *veo_version_string()
{
  return AVEO_VERSION;
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
 * VE supports normal mode and NUMA mode (Partitioning mode).
 * Under NUMA mode, if a user specifies the same VE node to this 
 * function and creates multiple VE processes, the processes will be 
 * created alternately on NUMA nodes.
 *
 * A user can control VE program execution using environment variables 
 * such as VE_LD_LIBRARY_PATH ,VE_OMP_NUM_THREADS and VE_NUMA_OPT.
 * These environmet variables are also available when a user calls 
 * this function.
 *
 * @param venode VE node number
 * @param tmp_veobin VE alternative veorun binary path
 * @return pointer to VEO process handle upon success
 * @retval NULL VE process creation failed.
 */
veo_proc_handle *veo_proc_create_static(int venode, char *tmp_veobin)
{
  if (venode < -1) {
    VEO_ERROR("venode(%d) is an invalid value.", venode);
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
          for (unsigned int i=0; i < v.size(); i++) {
            if (stoi(v[i]) == venode) {
              found = 1;
              break;
            }
          }
          if (found == 0) {
            VEO_ERROR("VE node #%d is not assigned by the scheduler",
                      venode);
            return NULL;
          }
        } else {
          // If VE_NODE_NUMBER is not set, use the first value in _VENODELIST
          if (v.size() > 0) 
            venode = stoi(v[0]);
          else { 
            VEO_ERROR("_VENODELIST is empty.", NULL);
            return NULL;
          }
        }
      } else {
	// translate venode using _VENODELIST
        if ((int)v.size() <= venode) {
          VEO_ERROR("venode = %d exceeds the size of _VENDOELIST %d", venode, v.size());
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

    // 4 is the total number of spaces and the \0
    size_t veobin_size = strlen(tmp_veobin) + strlen(CMD_VEGDB)
                         + strlen(CMD_XTERM) + strlen("-e") + 4;
    std::unique_ptr<char[]> veobin(new char[veobin_size]());
    // veobin = <path to aveorun>
    // if VEO_DEBUG is invalid value or not set, VEO program runs in the default.
    snprintf(veobin.get(), veobin_size, "%s", tmp_veobin);
    const char *veo_debug_mode = getenv("VEO_DEBUG");
    if (veo_debug_mode != nullptr) {
      VEO_DEBUG("VEO_DEBUG=%s", veo_debug_mode);
      if (strcmp(veo_debug_mode, "console") == 0) {
        // veobin = <path to VE gdb> <path to aveorun>
        snprintf(veobin.get(), veobin_size, "%s %s", CMD_VEGDB, tmp_veobin);
      } else if (strcmp(veo_debug_mode, "xterm") == 0) {
        // veobin = <path to xterm> -e <path to VE gdb> <path to aveorun>
        snprintf(veobin.get(), veobin_size, "%s -e %s %s", CMD_XTERM, CMD_VEGDB, tmp_veobin);
      } else {
        VEO_ERROR("VEO_DEBUG(%s) is invalid.", veo_debug_mode);
      }
    }
    VEO_DEBUG("veobin = %s", veobin.get());
    auto rv = new veo::ProcHandle(venode, veobin.get());
    return rv->toCHandle();

  } catch (std::invalid_argument &e) {
    VEO_ERROR("failed to create proc: %s", e.what());
    return 0;
  } catch (std::out_of_range &e) {
    VEO_ERROR("failed to create proc: %s", e.what());
    return 0;
  } catch (VEOException &e) {
    VEO_ERROR("failed to create proc: %s", e.what());
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
 * VE supports normal mode and NUMA mode (Partitioning mode).
 * Under NUMA mode, if a user specifies the same VE node to this 
 * function and creates multiple VE processes, the processes will be 
 * created alternately on NUMA nodes.
 *
 * A user can control VE program execution using environment variables 
 * such as VE_LD_LIBRARY_PATH ,VE_OMP_NUM_THREADS and VE_NUMA_OPT.
 * These environmet variables are also available when a user calls 
 * this function.
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
 * @brief find a veo_proc_handle's identifier
 * @param proc pointer to VEO process handle
 * @retval >= 0 VEO process identifier, i.e. index in the proc list
 * @retval -1 VEO process not found in list
 */
int veo_proc_identifier(veo_proc_handle *proc)
{
  return veo::_getProcIdentifier(ProcHandleFromC(proc));
}

/**
 * @brief set a veo_proc_handle's identifier to VEMVA
 * @param addr [in] VEMVA address
 * @param proc_ident [in] process identifier
 * @retval HMEM addr upon success; 0 upon failure.
 */
void *veo_set_proc_identifier(void *addr, int proc_ident)
{
    if (proc_ident < 0 || proc_ident >= VEO_MAX_HMEM_PROCS) {
      VEO_ERROR("proc_ident is invalid(%d)", proc_ident);
      return (void *)0;
    }
    void *hmem = (void *)SET_VE_FLAG(addr);
    hmem = (void *)SET_PROC_IDENT(hmem, proc_ident);
    return hmem;
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
    VEO_ERROR("failed to load library: %s", e.what());
    errno = e.err();
    return 0;
  }
}

/**
 * @brief unload a VE library
 * @param proc VEO process handle
 * @param libhandle a library handle
 * @retval 0 if request succeeded
 */
int veo_unload_library(veo_proc_handle *proc, const uint64_t libhandle)
{
  try {
    return ProcHandleFromC(proc)->unloadLibrary(libhandle);
  } catch (VEOException &e) {
    VEO_ERROR("failed to unload library: %s", e.what());
    errno = e.err();
    return -1;
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
    VEO_ERROR("failed to get symbol: %s", e.what());
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
 * @brief Allocate a VE memory buffer or a VH memory buffer which
 * users can use them as heterogeneous memory.
 *
 * This function allocates a VE memory buffer if h is a valid
 * handle. This function allocates a VH memory buffer if h is NULL.
 * This function returns the address with the process identifier.
 * Since the number of processes that can be identified by the process 
 * identifier is 16, this function will fail if the user attempts to 
 * identify more than 16 processes. The memory allocated using this 
 * function must be transferred to the VE side with veo_args_set_hmem().
 * and freed with veo_free_hmem(). The data transfer of this memory
 * between VH and VE must be done by veo_hmemcpy(). veo_hmemcpy()
 * allows user to transfer data from VH to VE, from VE to VH, or from 
 * VH to VH. The transfer direction is determined from the identifier 
 * of the virtual address of the argument passed to veo_hmemcpy(). 
 * Users can check with veo_is_ve_addr() that the target address is 
 * flagged with the VE process identifier or not.
 * Users need to remove the process identifier using
 * veo_get_hmem_addr() before using the allocated memory.
 *
 * @param h VEO process handle
 * @param addr [out] a pointer to VEMVA address with the identifier
 * @param size [in] size in bytes
 * @retval 0 memory allocation succeeded.
 * @retval -1 memory allocation or proc identifier acquisition failed.
 * @retval -2 internal error.
 */
int veo_alloc_hmem(veo_proc_handle *h, void **addr, const size_t size)
{
  uint64_t veaddr = 0UL;
  int ret = -1;
  try {
    if (h == nullptr) {
      *addr = malloc(size);
      if (*addr == nullptr)
        return -1;
      return 0;
    }
    int nprocs = ProcHandleFromC(h)->numProcs();
    VEO_DEBUG("nprocs = %d", nprocs);
    if (nprocs > VEO_MAX_HMEM_PROCS) {
      VEO_ERROR("VEO procs exceeded VEO_MAX_HMEM_PROCS.");
      return -1;
    }
    ret = veo_alloc_mem(h, &veaddr, size);
    if (ret != 0)
      return ret;
    //std::lock_guard<std::mutex> lock(ProcHandle::__procs_mtx);
    int proc_ident = ProcHandleFromC(h)->getProcIdentifier();
    if (proc_ident < 0)
      return proc_ident;
    *addr = (void *)SET_VE_FLAG(veaddr);
    *addr = (void *)SET_PROC_IDENT(*addr, proc_ident);
  } catch (VEOException &e) {
    VEO_ERROR("failed to allocate memory : %s", e.what());
    return -1;
  }
  return ret;
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
 * @brief Free a VE memory buffer
 *
 * This function free the memory allocated by veo_alloc_hmem().
 *
 * @param addr [in] a pointer to VEMVA address
 * @retval 0 memory is successfully freed.
 * @retval -1 internal error.
 */
int veo_free_hmem(void *addr)
{
  int ret = -1;
  try {
    //std::lock_guard<std::mutex> lock(ProcHandle::__procs_mtx);
    if (!veo_is_ve_addr(addr)) {
      free(addr);
      return 0;
    }
    int proc_ident = GET_PROC_IDENT(addr);
    veo::ProcHandle *p = veo::ProcHandle::getProcHandle(proc_ident);
    veo_proc_handle *h = p->toCHandle();
    ret = veo_free_mem(h, VIRT_ADDR_VE(addr));
  } catch (VEOException &e) {
    VEO_ERROR("failed to free memory : %s", e.what());
    return -1;
  }
  return ret;
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
 * @brief Copy VE/VH memory
 *
 * This function copies memory. When only the destination is the VE
 * memory allocated by veo_alloc_hmem(), the data is transferred 
 * from VH to VE. When only the source is the VE memory allocated by
 * veo_alloc_hmem(), the data is transferred from VE to VH. When 
 * both the source and the destination are VH memory, they are 
 * copied on VH. When both the source and the destination are VE 
 * memory allocated by veo_alloc_hmem(), veo_hmemcpy() returns a 
 * failure.
 *
 * @param dst a pointer to destination
 * @param src a pointer to source
 * @param size size in byte
 * @return zero upon success; negative upon failure.
 */
int veo_hmemcpy(void *dst, const void *src, size_t size)
{
  try
  {
    //std::lock_guard<std::mutex> lock(ProcHandle::__procs_mtx);
    veo_proc_handle *h;
    veo::ProcHandle *p;
    int proc_ident = -1;
    if (IS_VE(dst) && !IS_VE(src)) {
      proc_ident = GET_PROC_IDENT(dst);
      p = veo::ProcHandle::getProcHandle(proc_ident);
      h = p->toCHandle();
      return veo_write_mem(h, VIRT_ADDR_VE(dst), src, size);
      //return veo_write_mem(h, (uint64_t)dst, src, size);
    } else if (!IS_VE(dst) && IS_VE(src)) {
      proc_ident = GET_PROC_IDENT(src);
      p = veo::ProcHandle::getProcHandle(proc_ident);
      h = p->toCHandle();
      //return veo_read_mem(h, dst, (uint64_t)src, size);
      return veo_read_mem(h, dst, VIRT_ADDR_VE(src), size);
    } else if (!IS_VE(dst) && !IS_VE(src)) {
      memcpy(dst, src, size);
      return 0;
    }
    return -1;
  } catch (std::out_of_range &e) {
    VEO_ERROR("failed to get process handler: %s", e.what());
    return -1;
  } catch (VEOException &e) 
  {
    return -1;
  }
}

/**
 * @brief Return number of open contexts in a proc
 *
 * @param h VEO process handle
 * @return number of open contexts: negative upon failure.
 */
int veo_num_contexts(veo_proc_handle *h)
{
  if (h == nullptr) {
    errno = EINVAL;
    return -1;
  }
  return ProcHandleFromC(h)->numContexts();
}

/**
 * @brief Return a pointer of VEO thread context in a proc
 *
 * The argument idx takes an integer value zero or more and less 
 * than the result of veo_num_contexts(). The context with idx=0 
 * is the main context of the ProcHandle. It will not be destroyed 
 * when closed, instead it is destroyed when the proc is killed by 
 * veo_proc_destroy(), or when the program ends. veo_get_context() 
 * returns NULL if a user specifies idx with a value which is the 
 * result of veo_num_contexts() or more.
 * 
 * @param proc VEO process handle
 * @param idx a index which takes an integer value zero or more and less than the result of veo_num_contexts()
 * @return a pointer to VEO thread context upon success.
 */
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
    VEO_ERROR("failed to retrieve context %d: %s", idx, e.what());
    errno = e.err();
    return NULL;
  } catch (std::out_of_range &e) {
    VEO_ERROR("failed to retrieve context %d: %s", idx, e.what());
    return NULL;
  } 
}

/**
 * @brief open a VEO context
 *
 * Create a new VEO context, a pseudo thread and VE thread for the context.
 * All attributes which veo_context_open_with_attr() can specify have default value.
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
    VEO_ERROR("failed to open context: %s", e.what());
    errno = e.err();
    return NULL;
  }
}

/**
 * @brief open a VEO context with attributes.
 *
 * Create a new VEO context, a pseudo thread and VE thread for the context.
 *
 * @param proc VEO process handle
 * @param tca veo_thr_ctxt_attr object
 * @return a pointer to VEO thread context upon success.
 * @retval NULL failed to create a VEO context.
 */
veo_thr_ctxt *veo_context_open_with_attr(
				veo_proc_handle *proc, veo_thr_ctxt_attr *tca)
{
  if ((tca == nullptr) || (proc == nullptr)) {
    errno = EINVAL;
    return NULL;
  }
  auto attr = ThreadContextAttrFromC(tca);
  try {
    size_t stack_sz = attr->getStacksize();
    veo_thr_ctxt *ctx = ProcHandleFromC(proc)->openContext(stack_sz)->toCHandle();
    auto rv = reinterpret_cast<intptr_t>(ctx);
    if ( rv < 0 ) {
      errno = -rv;
      return NULL;
    }
    return ctx;
  } catch (VEOException &e) {
    VEO_ERROR("failed to open context: %s", e.what());
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
  auto c = ContextFromC(ctx);
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
  auto c = ContextFromC(ctx);
  c->synchronize();
}

/**
 * @brief get VEO context state
 *
 * @param ctx VEO context
 * @return the state of the VEO context state.
 * @retval VEO_STATE_RUNNING VEO context is running.
 * @retval VEO_STATE_EXIT VEO context  exited.
 */
int veo_get_context_state(veo_thr_ctxt *ctx)
{
  return ContextFromC(ctx)->getState();
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
 * @param inout intent of argument. VEO_INTENT_IN, VEO_INTENT_OUT, VEO_INTENT_INOUT are supported
 * @param argnum argument number that is being set
 * @param buff char pointer to buffer that will be copied to the VE stack
 * @param len length of buffer that is copied to the VE stack
 * @retval  0 argumen is successfully set.
 * @retval -1 an error occurred.
 *
 * The buffer is copied to the stack and will look to the VE callee
 * like a local variable of the caller function. Use this to pass
 * structures to the VE "kernel" function. The size of arguments
 * passed on the stack is limited to 63MB, since the size of the
 * initial stack is 64MB. Allocate and use memory buffers on heap when
 * you have huge argument arrays to pass.
 */
int veo_args_set_stack(veo_args *ca, enum veo_args_intent inout,
                       int argnum, char *buff, size_t len)
{
  try {
    CallArgsFromC(ca)->setOnStack(inout, argnum, buff, len);
    return 0;
  } catch (VEOException &e) {
    VEO_ERROR("failed set_on_stack CallArgs(%d): %s",
              argnum, e.what());
    return -1;
  }
}

 /**
 * @brief set a heteroginious memory argument
 *
 * @param ca veo_args
 * @param argnum the argnum-th argument
 * @param val a pointer to value to be set
 * @return zero upon success; negative upon failure.
 */
int veo_args_set_hmem(struct veo_args *ca, int argnum, void *val)
{
  return veo_args_set_u64(ca, argnum, (uint64_t)val);
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
    return ContextFromC(ctx)->callAsync(addr, *CallArgsFromC(args));
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
    return ContextFromC(ctx)->callPeekResult(reqid, retp);
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
    return ContextFromC(ctx)->callWaitResult(reqid, retp);
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
    return ContextFromC(ctx)->asyncReadMem(dst, src, size);
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
    return ContextFromC(ctx)->asyncWriteMem(dst, src, size);
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
    return ContextFromC(ctx)->callAsyncByName(libhdl, symname, *CallArgsFromC(args));
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
    return ContextFromC(ctx)->callVHAsync(func, arg);
  } catch (VEOException &e) {
    return VEO_REQUEST_ID_INVALID;
  }
}

/**
 * @brief allocate and initialize VEO thread context attributes object
 *        (veo_thr_ctxt_attr).
 *
 * @return pointer to veo_thr_ctxt_attr
 * @retval NULL the allocation of veo_thr_ctxt_attr failed.
 */
veo_thr_ctxt_attr *veo_alloc_thr_ctxt_attr(void)
{
  try {
    auto rv = new veo::ThreadContextAttr();
    return rv->toCHandle();
  } catch (VEOException &e) {
    errno = e.err();
    return NULL;
  }
}

/**
 * @brief free VEO thread context attributes object.
 *        freeing a VEO thread context attributes object has no effect on VEO
 *        thread contexts that were created using that object.
 *
 * @param tca veo_thr_ctxt_attr object
 * @retval 0 VEO thread context attributes are successfully freed.
 * @retval -1 VEO thread context attributes de-allocation failed.
 */
int veo_free_thr_ctxt_attr(veo_thr_ctxt_attr *tca)
{
  if (tca == nullptr) {
    errno = EINVAL;
    return -1;
  }
  delete ThreadContextAttrFromC(tca);
  return 0;
}

/**
 * @brief set stack size of VE thread which executes a VE function.
 *
 * @param tca veo_thr_ctxt_attr object
 * @param stack_sz stack size of VE thread
 *
 * @return 0 upon success; -1 upon failure.
 */
int veo_set_thr_ctxt_stacksize(veo_thr_ctxt_attr *tca, size_t stack_sz)
{
  if (tca == nullptr) {
    errno = EINVAL;
    return -1;
  }
  try {
    ThreadContextAttrFromC(tca)->setStacksize(stack_sz);
  } catch (VEOException &e) {
    VEO_ERROR("failed veo_set_thr_ctxt_stacksize (%p)", tca);
    errno = e.err();
    return -1;
  }
  return 0;
}

/**
 * @brief get stack size of VE thread which executes a VE function.
 *
 * @param tca veo_thr_ctxt_attr object
 * @param stack_sz pointer to store stack size of VE thread.
 *
 * @return 0 upon success; -1 upon failure.
 */
int veo_get_thr_ctxt_stacksize(veo_thr_ctxt_attr *tca, size_t *stack_sz)
{
  if (tca == nullptr || stack_sz == nullptr) {
    errno = EINVAL;
    return -1;
  }
  *stack_sz = ThreadContextAttrFromC(tca)->getStacksize();
  return 0;
}

//@}
