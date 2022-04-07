#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include "veo_veshm.h"
#include "VEOException.hpp"
#include "veo_api.hpp"
#include "veo_veos_defs.h"
#include "veo_sysve.h"
#include "veo_hmem_macros.h"

#define VESHM_MAX_ARGS  5

using veo::api::callSyscallWrapper;
using veo::api::ProcHandleFromC;
using veo::api::ContextFromC;
using veo::api::CallArgsFromC;
using veo::api::ThreadContextAttrFromC;
using veo::VEOException;

/**
 * \defgroup veoveshmapi VEO VESHM API (low-level)
 *
 * VESHM API functions for VEO.
 * This API is a low-level API and is intended to be called by upper layer software.
 * This API is not intended to be called by a user program.
 * To use VEO VESHM API functions, include "ve_offload.h" header and "veo_veshm.h" header.
 */

//@{

/**
 * @brief Register a VESHM area
 *
 * @param [in] vemva	Virtual address of VESHM area (HMEM addr)
 * @param [in] size	Size in byte
 * @param [in] syncnum	Pair number of PCISYAR/PCISYMR
 * -    Physical register number: 0-3 (Supported 0 only)
 * @param [in] mode_flag Mode flag which is ORed value of the following macros
 * -    VE_REGISTER_PCI:	Set up a memory as VESHM and 
 * 				register the memory with PCIATB.
 * 				The values of vemva and size need to
 * 				be aligned on the PCIATB page size (the
 * 				PCIATB page size of almost all of the
 * 				models of VEs is 64 MB).
 *				A caller process specifies this flag
 *				to allow VE processes on remote VEs
 *				and the local VE to access the specified
 *				memory.
 * -    VE_REGISTER_NONE: 	Set up a memory as VESHM and 
 * 				NOT register the memory with PCIATB.
 * 				The values of vemva and size need to
 * 				be aligned on the page size of VE
 * 				memory(default:64MB).
 *				A caller process specifies this flag
 *				to allow VE processes on the local VE
 *				to access the specified memory.
 * -    VE_PCISYNC: 		Enable synchronization.
 *                              VE_REGISTER_PCI must be specified.
 * -    VE_SHM_RO: 		Set "Read Only" permission.
 *
 * @retval 0 On success
 * @retval -1 On failure (set errno)
 * - EINVAL	Invalid value. (E.g., it is negative value, or too big)
 * - EINVAL	Invalid value. (Different paze size from PCIATB)
 * - ENOMEM	Creating a VESHM failed.
 * - EACCESS	Permission denied.
 * - ECANCELED	Operation canceled.
 * - ENOTSUP	Library load or symbol retrieval failed.
 * - EBUSY	Starting VESHM creation on VE or wating for result of it failed.
 */
int veo_shared_mem_open(void *vemva, size_t size, int syncnum, long long mode_flag)
{
	// get veo_proc_handle
	veo_proc_handle *h = veo_get_proc_handle_from_hmem(vemva);
	if (h == NULL) {
                errno = EINVAL;
		return -1;
        }

	// set arguments of ve_shared_mem_open
	veo_args *ca = veo_args_alloc();
	veo_args_set_u64(ca, 0, (uint64_t)SYS_sysve);
	veo_args_set_u64(ca, 1, (uint64_t)VE_SYSVE_VESHM_CTL);
	veo_args_set_u64(ca, 2, (uint64_t)VESHM_OPEN);
	veo_args_set_hmem(ca, 3, veo_get_hmem_addr(vemva));
        veo_args_set_u64(ca, 4, size);
        veo_args_set_i64(ca, 5, syncnum);
        veo_args_set_i64(ca, 6, mode_flag);

	uint64_t result = callSyscallWrapper(h, ca);
	return result;
}

/**
 * @brief Unregister a VESHM area
 *
 * @param [in] vemva Virtual address of VESHM area (HMEM addr)
 * @param [in] size Size in byte
 * @param [in] syncnum Pair number of PCISYAR/PCISYMR
 * -    Physical register number: 0-3 (Supported 0 only)
 * @param [in] mode_flag Mode flag which is the same value as the
 *                       argument of veo_shared_mem_open().
 *
 * @retval 0 On success
 * @retval -1 On failure (set errno)
 * - EINVAL	Invalid value.
 * - ECANCELED	Operation canceled.
 * - ENOTSUP	Library load or symbol retrieval failed.
 * - EBUSY	Starting VESHM close on VE or wating for result of it failed.
 */
int  veo_shared_mem_close(void *vemva, size_t size, int syncnum, long long mode_flag)
{
	// ve_shared_mem_close(vemva, size, syncnum, mode_flag);
	veo_proc_handle *h = veo_get_proc_handle_from_hmem(vemva);
        if (h == NULL) {
                errno = EINVAL;
                return -1;
        }

	veo_args *ca = veo_args_alloc();
	veo_args_set_u64(ca, 0, (uint64_t)SYS_sysve);
	veo_args_set_u64(ca, 1, (uint64_t)VE_SYSVE_VESHM_CTL);
	veo_args_set_u64(ca, 2, (uint64_t)VESHM_CLOSE);
	veo_args_set_hmem(ca, 3, veo_get_hmem_addr(vemva));
        veo_args_set_u64(ca, 4, size);
        veo_args_set_i64(ca, 5, syncnum);
        veo_args_set_i64(ca, 6, mode_flag);

	uint64_t result = callSyscallWrapper(h, ca);
	return result;
}

/**
 * @brief Attach a VESHM area
 *
 * This function basically does the same as ve_shared_mem_attach(). 
 * The difference point is that if VE_REGISTER_VEMVA is specified for 
 * the mode flag, HMEM address will be returned as attached address.
 *
 * @param [in] h VEO process handle
 * @param [in] pid Pid of an owner process that is gotten by veo_get_pid_from_hmem()
 * @param [in] veshm_vemva Virtual address of VESHM area (HMEM addr)
 * @param [in] size Size in byte
 * @param [in] syncnum Pair number of PCISYAR/PCISYMR
 * -    Physical register number: 0-3 (Supported 0 only)
 * @param [in] mode_flag Mode flag which is ORed value of one of the
 *             following macros and the same value as the argument of
 *             veo_shared_mem_open()
 * -	VE_REGISTER_VEHVA:	VESHM on a remote VE will be attached to 
 *  				VEHVA (using DMAATB).
 *				A caller process specifies this flag to
 *				access the memory registered by a VE process
 *				running on a remote VE.
 *				The caller process can transfer data using
 *				veo_dma_post() or veo_dma_post_wait() with
 *				returned address.
 * -	VE_REGISTER_VEMVA:	VESHM on the local VE will be attached to 
 *				VEMVA (using ATB).
 *				A caller process specifies this flag to
 *				access the memory registered by a VE process
 *				running on the local VE.
 *				The caller process can transfer data using
 *				veo_hmemcpy() with returned address.
 * -	VE_MEM_LOCAL:		An own memory will be attached to VEHVA
 *                              (using DMAATB).
 * @note A VESHM area is recognized by a combination of (vemva, size,
 *       syncnum, mode_flag) which are arguments of this function.
 *
 * @retval Attached-address On success
 * @retval 0xffffffffffffffff On failure (set errno)
 * - EINVAL	Invalid value.
 * - EFAULT	Bad address.
 * - ESRCH	No such process.
 * - ENOENT	No such memory.
 * - ENOMEM	Cannot attach VESHM.
 * - EACCESS	Cannot attach VESHM.
 * - EACCESS	Permission denied.
 * - ECANCELED	Operation canceled.
 * - EAGAIN	Owner process is swapped-out.
 * - ENOTSUP	Library load or symbol retrieval failed.
 * - EBUSY	Starting VESHM attachment on VE or wating for result of it failed.
 */
void *veo_shared_mem_attach(veo_proc_handle *h, pid_t pid, void *veshm_vemva, 
			size_t size, int syncnum, long long mode_flag)
{
	uint64_t arg[VESHM_MAX_ARGS];
	arg[0] = (uint64_t)pid;
	arg[1] = (uint64_t)veo_get_hmem_addr(veshm_vemva);
	arg[2] = (uint64_t)size;
	arg[3] = (uint64_t)syncnum;
	arg[4] = (uint64_t)mode_flag;

	veo_args *ca = veo_args_alloc();
	veo_args_set_u64(ca, 0, (uint64_t)SYS_sysve);
	veo_args_set_u64(ca, 1, (uint64_t)VE_SYSVE_VESHM_CTL);
	veo_args_set_u64(ca, 2, (uint64_t)VESHM_ATTACH);
	veo_args_set_stack(ca, VEO_INTENT_IN, 3, (char *)arg, sizeof(uint64_t)*VESHM_MAX_ARGS);

	void *result = (void *)callSyscallWrapper(h, ca);
	if (mode_flag & VE_REGISTER_VEMVA) {
 		uint64_t ident = ProcHandleFromC(h)->getProcIdentifier();
		result = (void *)SET_VE_FLAG(result);
		result = (void *)SET_PROC_IDENT(result, ident);
	}
	return result;
}

/**
 * @brief Detach a VESHM area
 *
 * This function basically does the same as ve_shared_mem_detach().
 * The difference point is that if VE_REGISTER_VEMVA is specified for
 * the mode flag, HMEM address is must specified as attached address.
 *
 * @param [in] h 		VEO process handle
 * @param [in] veshm_addr	Virtual address of an attached VESHM area 
 * 				(HMEM addr in case of VE_REGISTERM_VEMVA)
 * @param [in] mode_flag Mode flag which is one of the following macros
 * -	VE_REGISTER_VEHVA:	VESHM on a remote VE will be attached to 
 *  				VEHVA (using DMAATB).
 * -	VE_REGISTER_VEMVA:	VESHM on the local VE will be attached to 
 *  				VEMVA (using ATB).
 * -	VE_MEM_LOCAL:		An own memory will be attached to VEHVA
 *                              (using DMAATB).
 *
 * @retval 0 On success
 * @retval -1 On failure (set errno)
 * - EINVAL	Invalid argument.
 * - ECANCELED	No such memory.
 * - ENOTSUP	Library load or symbol retrieval failed.
 * - EBUSY	Starting VESHM detachment on VE or wating for result of it failed.
 */
int veo_shared_mem_detach(veo_proc_handle *h, void *veshm_addr, long long mode_flag)
{
	veo_args *ca = veo_args_alloc();
	veo_args_set_u64(ca, 0, (uint64_t)SYS_sysve);
	veo_args_set_u64(ca, 1, (uint64_t)VE_SYSVE_VESHM_CTL);
	veo_args_set_u64(ca, 2, (uint64_t)VESHM_DETACH);
	veo_args_set_hmem(ca, 3, veo_get_hmem_addr(veshm_addr));
        veo_args_set_i64(ca, 4, mode_flag);

	uint64_t result = callSyscallWrapper(h, ca);
	return result;
}

//@}
