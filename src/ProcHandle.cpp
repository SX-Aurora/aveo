/**
 * @file ProcHandle.cpp
 * @brief implementation of ProcHandle
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
 * ProcHandle methods implementation.
 * 
 * Copyright (c) 2018-2021 NEC Corporation
 * Copyright (c) 2020-2021 Erich Focht
 */
#include <algorithm>
#include "ProcHandle.hpp"
#include "Context.hpp"
#include "VEOException.hpp"
#include "CallArgs.hpp"
#include "log.h"

#include "veo_urpc.h"
#include "veo_urpc_vh.hpp"
#include "veo_time.h"

#include <string.h>
#include <unistd.h>
#include <sys/auxv.h>
#include <sys/ipc.h>
#include <sys/mman.h>
#include <sys/resource.h>
#include <sys/shm.h>
#include <sys/types.h>
#include <wait.h>

namespace veo {

// remember active procs for the destructor
//   This needs to be a pointer with memory allocated later, otherwise
//   py-veo (cython?) erases the content and we have leftover VE processes
std::vector<ProcHandle *> *__procs;
std::mutex __procs_mtx;

/**
 * @brief Get VE process identifier for a proc
 * @return VE process identifier upon success; negative upon failure.
 */
int _getProcIdentifier(ProcHandle *proc)
{
  std::lock_guard<std::mutex> lock(veo::__procs_mtx);
  //std::lock_guard<std::mutex> lock2(this->mctx->submit_mtx);

  // Get element number of this pointer included in veo::__procs.
  std::vector<ProcHandle *>::iterator itr;
  itr = std::find(veo::__procs->begin(), veo::__procs->end(), proc);
  if (itr == veo::__procs->end()) {
    VEO_ERROR("Search failed.");
    return -1;
  }
  const int idx = std::distance(veo::__procs->begin(), itr);
  VEO_TRACE("proc %p identifier = %d", (void *)proc, idx);
  return idx;
}
  
/**
 * @brief Get VE process identifier for a proc with nolock
 * @return VE process identifier upon success; negative upon failure.
 */
int _getProcIdentifierNolock(ProcHandle *proc)
{
  // Get element number of this pointer included in veo::__procs.
  std::vector<ProcHandle *>::iterator itr;
  itr = std::find(veo::__procs->begin(), veo::__procs->end(), proc);
  if (itr == veo::__procs->end()) {
    VEO_ERROR("Search failed.");
    return -1;
  }
  const int idx = std::distance(veo::__procs->begin(), itr);
  VEO_TRACE("proc %p identifier = %d", (void *)proc, idx);
  return idx;
}
  
/**
 * @brief constructor
 *
 * @param venode VE node ID for running child peer
 * @param binname VE executable
 */
ProcHandle::ProcHandle(int venode, char *binname) : ve_number(-1), proc_survival(false)
{
  // create vh side peer
  this->up = vh_urpc_peer_create();
  if (this->up == nullptr) {
    throw VEOException("ProcHandle: failed to create VH side urpc peer.");
  }

  int vecore = -1;
  const char *e = getenv("VE_CORE_NUMBER");
  if (e != nullptr) {
    vecore = stoi(std::string(e));
  }

  // create VE process connected to this peer
  auto rv = vh_urpc_child_create(this->up, binname, venode, vecore);
  if (rv != 0) {
    vh_urpc_peer_destroy(this->up);
    throw VEOException("ProcHandle: failed to create VE process.");
    VEO_ERROR("failed to create VE process, binname=%s venode=%d core=%d",
              binname, venode, vecore);
  }
  if (urpc_wait_peer_attach(this->up) != 0) {
    vh_urpc_peer_destroy(this->up);
    throw VEOException("ProcHandle: VE process does not become ready.");
  }
  this->proc_survival = true;

  this->mctx = new Context(this, this->up, true);

  this->mctx->getStackPointer(&this->ve_sp);
  VEO_DEBUG("proc stack pointer: %p", (void *)this->ve_sp);

  // Progress thread creates and starts.
  if (this->mctx->progressInit() != true) {
    throw VEOException("ProcHandle: failed to initialize progress thread.");
  }

  this->mctx->state = VEO_STATE_RUNNING;
  this->mctx->ve_sp = this->ve_sp;
  // first ctx gets core 0 (could be changed later)
  this->mctx->core = vecore;
  this->ve_number = venode;

  std::lock_guard<std::mutex> lock(veo::__procs_mtx);
  if (veo::__procs == nullptr) {
    veo::__procs = new std::vector<ProcHandle *>();
    veo::__procs->push_back(this);
    VEO_DEBUG("Create new std::vector<ProcHandle *>");
    return;
  }

  // Reuse veo::__procs to identify VE processes.
  std::vector<ProcHandle *>::iterator itr;
  itr = std::find(veo::__procs->begin(), veo::__procs->end(), nullptr);
  if (itr == veo::__procs->end()) {
    veo::__procs->push_back(this);
    VEO_DEBUG("Added new ProcHandle pointer to veo::__prcos");
    return;
  }
  // Find the nullptr element and calculate the element number.
  const int idx = std::distance(veo::__procs->begin(), itr);
  VEO_DEBUG("Element number of the destroyed proc = %d", idx);
  veo::__procs->at(idx) = this;
  VEO_DEBUG("Added veo::__procs->at(%d) = %p", idx, veo::__procs->at(idx));
}

/**
 * @brief Get VE process identifier for this proc
 * @return VE process identifier upon success; negative upon failure.
 */
int ProcHandle::getProcIdentifier()
{
  return veo::_getProcIdentifier(this);
}

/**
 * @brief Get number of VE processes
 * @return the number of VE processes
 */
int ProcHandle::numProcs()
{
  std::lock_guard<std::mutex> lock(veo::__procs_mtx);
  return veo::__procs->size();
}

/**
 * @brief Get process handle
 * @return a pointer to ProcHandle upon success; nullptr upon failure.
 */
ProcHandle *ProcHandle::getProcHandle(int proc_ident)
{
  std::lock_guard<std::mutex> lock(veo::__procs_mtx);
  return veo::__procs->at(proc_ident);
}

/**
 * @brief Exit a proc
 *
 * This function first closes all but the main context, then closes
 * the main context of a proc handle. Finally it removes the proc
 * pointer from the vector keeping track of procs for the final cleanup.
 */
int ProcHandle::exitProc()
{
  int rc;
  std::lock_guard<std::mutex> procslock(__procs_mtx);
  std::lock_guard<std::mutex> ctxlock(ctx_mutex);
  VEO_TRACE("proc %p", (void *)this);
  //
  // delete all open contexts except the main one
  //
  for (auto c = this->ctx.begin(); c != this->ctx.end();) {
    auto ctx = (*c).get();
    if (ctx != this->mctx)
      this->delContextNolock(ctx);
    else
      c++;
  }
  // delete the main context
  rc = this->mctx->close();
  if (rc) {
    VEO_ERROR("main context close failed (rc=%d)\n"
              "Please check for leftover VE procs.", rc);
  }
  this->ve_number = -1;
  for (auto p = veo::__procs->begin(); p != veo::__procs->end(); p++) {
    if (*p == this) {
      VEO_DEBUG("erasing proc %lx", this);
      //veo::__procs->erase(p);
      // replace this obj pointer to nullptr
      veo::__procs->at(_getProcIdentifierNolock(this)) = nullptr;
      break;
    }
  }
  return 0;
}


/**
 * @brief Load a VE library in VE process space
 *
 * @param libname a library name
 * @return handle of the library loaded upon success; zero upon failure.
 */
uint64_t ProcHandle::loadLibrary(const char *libname)
{
  VEO_TRACE("libname %s", libname);
  size_t len = strlen(libname);
  if (len > VEO_SYMNAME_LEN_MAX) {
    throw VEOException("Library name too long", ENAMETOOLONG);
  }

  auto req = this->mctx->genericAsyncReq(URPC_CMD_LOADLIB, (char *)"P",
                                         libname, (size_t)strlen(libname) + 1);
  uint64_t handle = 0;
  auto rv = this->mctx->callWaitResult(req, &handle);
  if (rv != VEO_COMMAND_OK) {
    VEO_ERROR("rv=%d", rv);
    handle = 0;
  }

  VEO_TRACE("handle = %#lx", handle);
  return handle;
}

/**
 * @brief Unload a VE library from VE process space
 *
 * @param handle the library handle
 * @return zero upon success, nonzero if operation failed.
 */
int ProcHandle::unloadLibrary(const uint64_t handle)
{
  VEO_TRACE("lib handle %lx", handle);

  // remove all symbol names belonging to this library
  sym_mtx.lock();
  std::vector<decltype(sym_name)::key_type> vec;
  for (auto&& i : sym_name)
    if (i.first.first == handle)
        vec.emplace_back(i.first);
  for (auto&& key : vec)
    sym_name.erase(key);
  sym_mtx.unlock();

  auto req = this->mctx->genericAsyncReq(URPC_CMD_UNLOADLIB, (char *)"L",
                                         handle);
  int64_t result = -1;
  auto rv = this->mctx->callWaitResult(req, (uint64_t *)&result);
  if (rv != VEO_COMMAND_OK) {
    VEO_ERROR("rv=%d", rv);
  }

  VEO_TRACE("result = %ld", result);
  return (int)result;
}

/**
 * @brief Find a symbol in VE program
 *
 * @param libhdl handle of library
 * @param symname a symbol name to find
 * @return VEMVA of the symbol upon success; zero upon failure.
 */
uint64_t ProcHandle::getSym(const uint64_t libhdl, const char *symname)
{
  uint64_t symaddr = 0;
  size_t len = strlen(symname);
  if (len > VEO_SYMNAME_LEN_MAX) {
    throw VEOException("Too long name", ENAMETOOLONG);
  }
  sym_mtx.lock();
  auto sym_pair = std::make_pair(libhdl, symname);
  auto itr = sym_name.find(sym_pair);
  if( itr != sym_name.end() ) {
    sym_mtx.unlock();
    VEO_TRACE("symbol name = %s, addr = %#lx", symname, itr->second);
    return itr->second;
  }
  sym_mtx.unlock();

  auto req = this->mctx->genericAsyncReq(URPC_CMD_GETSYM, (char *)"LP",
                                         libhdl, symname, (size_t)strlen(symname) + 1);
  auto rv = this->mctx->callWaitResult(req, &symaddr);
  if (rv != VEO_COMMAND_OK) {
    VEO_ERROR("rv=%d", rv);
    symaddr = 0;
  }

  VEO_TRACE("symbol name = %s, addr = %#lx", symname, symaddr);
  if (symaddr == 0) {
    return 0;
  }
  sym_mtx.lock();
  sym_name[sym_pair] = symaddr;
  sym_mtx.unlock();
  return symaddr;
}

/**
 * @brief Allocate a buffer on VE
 *
 * @param size of buffer
 * @return VEMVA of the buffer upon success; zero upon failure.
 */
uint64_t ProcHandle::allocBuff(const size_t size)
{
  VEO_TRACE("start");
  auto req = this->mctx->genericAsyncReq(URPC_CMD_ALLOC, (char *)"L", size);
  uint64_t addr = 0;
  auto rv = this->mctx->callWaitResult(req, &addr);
  if (rv != VEO_COMMAND_OK) {
    VEO_ERROR("rv=%d", rv);
    addr = 0;
  }
  VEO_TRACE("returned addr 0x%lx", addr);
  return addr;
}

/**
 * @brief Free a buffer on VE
 *
 * @param buff VEMVA of the buffer
 * @return nothing
 */
void ProcHandle::freeBuff(const uint64_t buff)
{
  VEO_TRACE("start");
  auto req = this->mctx->genericAsyncReq(URPC_CMD_FREE, (char *)"L", buff);
  uint64_t dummy;
  auto rv = this->mctx->callWaitResult(req, &dummy);
  if (rv != VEO_COMMAND_OK) {
    VEO_ERROR("rv=%d", rv);
  }
  VEO_TRACE("end");
}

/**
 * @brief read data from VE memory
 * @param[out] dst buffer to store the data
 * @param src VEMVA to read
 * @param size size to transfer in byte
 * @return zero upon success; negative upon failure
 */
int ProcHandle::readMem(void *dst, uint64_t src, size_t size)
{
  VEO_TRACE("(%p, %lx, %ld)", dst, src, size);
  auto req = this->mctx->asyncReadMem(dst, src, size);
  if (req == VEO_REQUEST_ID_INVALID) {
    VEO_ERROR("failed! Aborting.");
    return -1;
  }
  VEO_TRACE("[request #%lu]", req);
  uint64_t dummy;
  auto rv = this->mctx->callWaitResult(req, &dummy);
  if (rv != VEO_COMMAND_OK) {
    VEO_TRACE("done, rv=%d", rv);
    rv = -1;
  }
  return rv;
}

/**
 * @brief write data to VE memory
 * @param dst VEMVA to write the data
 * @param src buffer holding data to write
 * @param size size to transfer in byte
 * @return zero upon success; negative upon failure
 */
int ProcHandle::writeMem(uint64_t dst, const void *src, size_t size)
{
  VEO_TRACE("(%p, %lx, %ld)", dst, src, size);
  auto req = this->mctx->asyncWriteMem(dst, src, size);
  if (req == VEO_REQUEST_ID_INVALID) {
    VEO_ERROR("failed! Aborting.");
    return -1;
  }
  VEO_TRACE("[request #%lu]", req);
  uint64_t dummy;
  auto rv = this->mctx->callWaitResult(req, &dummy);
  if (rv != VEO_COMMAND_OK) {
    VEO_TRACE("done. rv=%d", rv);
    rv = -1;
  }
  return rv;
}

/**
 * @brief start a function on the VE, wait for result and return it.
 *
 * @param addr VEMVA of function called
 * @param args arguments of the function
 * @param result pointer to result
 * @return 0 if all went well.
 */
int ProcHandle::callSync(uint64_t addr, CallArgs &args, uint64_t *result)
{
  return this->mctx->callSync(addr, args, result);
}

/**
 * @brief Return number of contexts in the ctx vector
 *
 * @return size of ctx vector
 */
int ProcHandle::numContexts()
{
  std::lock_guard<std::mutex> lock(ctx_mutex);
  return this->ctx.size();
}

/**
 * @brief Return pointer to a context in the ctx vector
 *
 * @param idx index / position inside the ctx vector
 *
 * @return pointer to ctx, raises exception if out of bounds.
 */
Context *ProcHandle::getContext(int idx)
{
  std::lock_guard<std::mutex> lock(ctx_mutex);
  return this->ctx.at(idx).get();
}

/**
 * @brief Remove context from the ctx vector
 *
 * The context is kept in a smart pointer, therefore it will be deleted implicitly.
 * 
 */
void ProcHandle::delContext(Context *ctx)
{
  std::lock_guard<std::mutex> lock(ctx_mutex);
  for (auto it = this->ctx.begin(); it != this->ctx.end(); it++) {
    if ((*it).get() == ctx) {
      (*it).get()->close();
      this->ctx.erase(it);
      break;
    }
  }
}

/**
 * @brief Remove context from the ctx vector with nolock
 *
 * The context is kept in a smart pointer, therefore it will be deleted implicitly.
 * 
 */
void ProcHandle::delContextNolock(Context *ctx)
{
  for (auto it = this->ctx.begin(); it != this->ctx.end(); it++) {
    if ((*it).get() == ctx) {
      (*it).get()->close();
      this->ctx.erase(it);
      break;
    }
  }
}

/**
 * @brief open a new context (VE thread)
 *
 * @return a new thread context created
 *
 * The first context returned is the this->mctx!
 */
Context *ProcHandle::openContext(size_t stack_sz)
{
  std::lock_guard<std::mutex> ctxlock(ctx_mutex);
  std::lock_guard<std::recursive_mutex> lock(this->mctx->submit_mtx);
  this->mctx->synchronize();

  if (this->ctx.empty()) {
    this->ctx.push_back(std::unique_ptr<Context>(this->mctx));
    return this->mctx;
  }
  // compute core
  int core = -1;
  int ve_omp_threads = -1;
  const char *e = getenv("VE_OMP_NUM_THREADS");
  if (e != nullptr) {
    ve_omp_threads = stoi(std::string(e));
  }
  if (ve_omp_threads != 1) {
    VEO_ERROR("Env variable VE_OMP_NUM_THREADS must be set to 1 when using multiple contexts!");
    throw VEOException("ProcHandle: VE_OMP_NUM_THREADS not set to 1 with multiple contexts.");
  }
  int prev_core = this->ctx.back()->core;
  if (prev_core >= 0) {
    core = prev_core + ve_omp_threads;
  }
  VEO_DEBUG("core = %d, prev_core = %d, ve_omp_threads = %d", core, prev_core, ve_omp_threads);

  // create vh side peer
  auto new_up = vh_urpc_peer_create();
  if (new_up == nullptr) {
    throw VEOException("ProcHandle: failed to create new VH side urpc peer.");
  }

  // start another thread inside peer proc that attaches to the new up
  auto req = urpc_generic_send(this->up, URPC_CMD_NEWPEER, (char *)"IIL",
                               new_up->shm_segid, core, stack_sz);
  if (req < 0) {
    VEO_ERROR("failed to send cmd %d", URPC_CMD_NEWPEER);
    vh_urpc_peer_destroy(new_up);
    return nullptr;
  }
  if (urpc_wait_peer_attach(new_up) != 0) {
    vh_urpc_peer_destroy(new_up);
    throw VEOException("ProcHandle: timeout while waiting for VE.");
  }
  int64_t rc;
  wait_req_result(this->up, req, &rc);
  if (rc) {
    vh_urpc_peer_destroy(new_up);
    return nullptr;
  }

  // wait for peer receiver to set flag to 1
  while(! (urpc_get_receiver_flags(&new_up->send) & 0x1) ) {
    busy_sleep_us(1000);
  }

  auto new_ctx = new Context(this, new_up, false);

  new_ctx->getStackPointer(&new_ctx->ve_sp);
  VEO_DEBUG("proc stack pointer: %p", (void *)new_ctx->ve_sp);

  // Progress thread creates and starts.
  if (new_ctx->progressInit() != true) {
    throw VEOException("ProcHandle: failed to initialize progress thread.");
  }
  new_ctx->core = core;
  this->ctx.push_back(std::unique_ptr<Context>(new_ctx));

  new_ctx->state = VEO_STATE_RUNNING;
  return new_ctx;
}

void ProcHandle::accessPciRecvSyncRegister()
{
  std::lock_guard<std::mutex> ctxlock(ctx_mutex);
  for (auto c = this->ctx.begin(); c != this->ctx.end(); c++) {
    auto ctx = (*c).get();
    auto req = ctx->genericAsyncReq(URPC_CMD_ACS_PCIRCVSYC, (char *)"");
  }
}

uint64_t ProcHandle::loadVE2VELibrary(const char *libname)
{
  uint64_t handle = 0UL;
  handle = getLibh_from_hashmap(libname);
  if (handle != 0UL)
    return handle;

  handle = loadLibrary(libname);
  this->setLibh_to_hashmap(libname, handle);

  return handle;
}

uint64_t ProcHandle::getVEDMASyms(uint64_t libh, const char *sym)
{
  uint64_t symaddr = getSym_from_hashmap(libh, sym);
  if (symaddr != 0UL) {
    VEO_DEBUG("symaddr has already been acquired.");
    return symaddr;
  }

  std::vector<std::string> vedmasym =
        {"ve_dma_post", "ve_dma_poll", "ve_dma_post_wait"};
  uint64_t target_symaddr = 0UL;
  for(std::string symname: vedmasym) {
    symaddr = getSym(libh, symname.c_str());
    if (strcmp(symname.c_str(), sym) == 0)
      target_symaddr = symaddr;
    if (symaddr == 0UL) {
      VEO_ERROR("getSym returns 0UL");
      return symaddr;
    }
  }
  return target_symaddr;
}

uint64_t ProcHandle::getLibh_from_hashmap(const char *libname)
{
  // check if it is loaded
  lib_mtx.lock();
  auto itr = ve2velibh.find(libname);
  lib_mtx.unlock();
  if (itr == ve2velibh.end())
    return 0UL;
  return itr->second;
}

uint64_t ProcHandle::getSym_from_hashmap(uint64_t libh, const char *sym)
{
  auto sym_pair = std::make_pair(libh, sym);
  sym_mtx.lock();
  auto itr = sym_name.find(sym_pair);
  sym_mtx.unlock();
  if (itr != sym_name.end())
    return itr->second;

  return 0UL;
}

void* ProcHandle::veMemcpy(void *dst, const void *src, size_t size)
{
  auto req = this->mctx->genericAsyncReq(URPC_CMD_MEMCPY, (char *)"LLL",
                        (uint64_t *)dst, (uint64_t *)src, size);

  uint64_t result = 0;
  auto rv = this->mctx->callWaitResult(req, &result);
  if (rv != VEO_COMMAND_OK) {
    VEO_ERROR("rv=%d", rv);
    return (void *)-1;
  }
  return 0;
}
} // namespace veo

//
// Destructor for procs, needed in case people forget to veo_proc_destroy().
// If not done, a VE process runnning aveorun stays in the system.
//
__attribute__((destructor))
static void _cleanup_procs(void)
{
  if (!veo::__procs)
    return;
  for (auto it = veo::__procs->begin(); it != veo::__procs->end();) {
    // we don't increment the iterator because erase() is actually
    // doing the remove.
    if (*it) {
      (*it)->exitProc();
      veo::__procs->erase(it);
    } else {
      VEO_DEBUG("proc destructor: proc pointer is NULL!");
      it++;
    }
  }
  delete veo::__procs;
}
