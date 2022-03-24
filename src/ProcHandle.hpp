/**
 * @file ProcHandle.hpp
 * @brief VEO process handle
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
 * ProcHandle class definition.
 * 
 * Copyright (c) 2018-2021 NEC Corporation
 * Copyright (c) 2020-2021 Erich Focht
 */
#ifndef _VEO_PROC_HANDLE_HPP_
#define _VEO_PROC_HANDLE_HPP_
#include <unordered_map>
#include <utility>
#include <memory>
#include <mutex>
#include <iostream>

#include <urpc.h>
#include <ve_offload.h>
#include "CallArgs.hpp"
#include "Context.hpp"
#include "VEOException.hpp"

namespace std {
    template <>
    class hash<std::pair<uint64_t, std::string>> {
    public:
        size_t operator()(const std::pair<uint64_t, std::string>& x) const{
            return hash<uint64_t>()(x.first) ^ hash<std::string>()(x.second);
        }
    };
}

namespace veo {

  int _getProcIdentifier(ProcHandle *);
  int _getProcIdentifierNolock(ProcHandle *);

/**
 * @brief VEO process handle
 */
class ProcHandle {
private:
  std::unordered_map<std::pair<uint64_t, std::string>, uint64_t> sym_name;
  std::mutex sym_mtx;
  std::mutex lib_mtx;
  std::mutex main_mutex;		//!< acquire when opening a new context
  std::mutex ctx_mutex;                 //!< acquire when referencing opened contexts
  urpc_peer_t *up;			//!< ve-urpc peer pointer
  uint64_t ve_sp;       		//!< stack pointer on VE side
  Context *mctx;			//!< context also used for sync proc ops
  std::vector<std::unique_ptr<Context>> ctx;	//!< vector of opened contexts
  int ve_number;			//!< store the VE number
  std::unordered_map<const char *, uint64_t> ve2velibh; //!< library handle for VE2VE communication

public:
  ProcHandle(int, char *);
  ~ProcHandle() { this->exitProc(); }

  static ProcHandle *getProcHandle(int);
  uint64_t loadLibrary(const char *);
  int unloadLibrary(const uint64_t libhdl);
  uint64_t getSym(const uint64_t, const char *);

  uint64_t allocBuff(const size_t);
  void freeBuff(const uint64_t);

  int readMem(void *, uint64_t, size_t);
  int writeMem(uint64_t, const void *, size_t);
  int exitProc(void);
  int getProcIdentifier(void);
  int numProcs(void);

  int numContexts(void);
  Context *getContext(int);

  int callSync(uint64_t, CallArgs &, uint64_t *);

  Context *mainContext() { return this->mctx; };
  Context *openContext(size_t stack_sz = VEO_DEFAULT_STACKSIZE);
  void delContext(Context *);
  void delContextNolock(Context *);

  veo_proc_handle *toCHandle() {
    return reinterpret_cast<veo_proc_handle *>(this);
  }

  int veNumber() { return this->ve_number; }
  void accessRegister();
  uint64_t loadVE2VELibrary(const char *);
  uint64_t getVEDMASyms(uint64_t, const char*);
  uint64_t getLibh_from_hashmap(const char *);
  uint64_t getSym_from_hashmap(uint64_t , const char *);
  void setLibh_to_hashmap(const char *libname, uint64_t libhandle) {
    lib_mtx.lock();
    this->ve2velibh[libname] = libhandle;
    lib_mtx.unlock();
 };
  pid_t getPid(void) { return up->child_pid; };
  void *veMemcpy(void *dst, const void *src, size_t size);
};
} // namespace veo
#endif
