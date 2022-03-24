#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include "veo_veos_defs.h"
#include "veo_sysve.h"
#include "veo_vhveshm.h"
#include "veo_vhshm_defs.h"
#include "veo_api.hpp"
#include "VEOException.hpp"

/**
 * \defgroup veovhveshmapi VEO VH-VE SHM API (low-level)
 *
 * VH-VE SHM API functions for VEO.
 * This API is a low-level API and is intended to be called by upper layer software.
 * This API is not intended to be called by a user program.
 * To use VEO VH-VE SHM API functions, include "ve_offload.h" header and "veo_vhveshm.h" header.
 */
//@{

using veo::api::callSyscallWrapper;
using veo::api::ProcHandleFromC;
using veo::api::ContextFromC;
using veo::api::CallArgsFromC;
using veo::api::ThreadContextAttrFromC;
using veo::VEOException;

/**
 * @brief This function gets the identifier of system V shared memory on VH.
 *
 * @note Argument is similar to shmget(2). Different points are shown below.
 * @note If a specified size is smaller than actual shared memory
 *       size, actual shared memory size is used.
 * @note Invoking this function is not required if the program knows
 *       the shared memory identifier.
 *
 * @param [in] h VEO process handle
 * @param [in] key	A key value associated with System V shared memory
 *            segment on VH. Don't specify IPC_PRIVATE.
 * @param [in] size	Size of System V shared memory segment on VH
 * @param [in] shmflag	SHM_HUGETLB must be specified. Don't specify
 *            SHM_NORESERVE IPC_EXCL and IPC_CREAT.
 *
 * @return A valid shared memory identifier on success, -1 and set errno on failure.
 * - EINVAL	SHM_HUGETLB is not specified as 3rd argument.
 * 		SHM_NORESERVE or IPC_EXCL or IPC_CREAT are specified as 3rd
 * 		argument.
 * - EINVAL	IPC_PRIVATE is specified as 1st argument.
 * - EINVAL	A segment with given key existed, but size is greater than
 * 		the size of the segment.
 * - EACCES	The user does not have permission to access the shared
 * 		memory segment, and does not have the CAP_IPC_OWNER capability.
 * - ENOENT	No segment exists for the given key.
 * - ENOTSUP	Library load or symbol retrieval failed.
 * - EBUSY	Starting VH-VE SHM creation on VE or result wait failed.
 */
int veo_shmget(veo_proc_handle *h, key_t key, size_t size, int shmflag) {
	veo_args *ca = veo_args_alloc();
        veo_args_set_u64(ca, 0, (uint64_t)SYS_sysve);
        veo_args_set_u64(ca, 1, (uint64_t)VE_SYSVE_VHSHM_CTL);
        veo_args_set_u64(ca, 2, (uint64_t)VHSHM_GET);
        veo_args_set_u64(ca, 3, (uint64_t)key);
        veo_args_set_u64(ca, 4, size);
        veo_args_set_i64(ca, 5, shmflag);

	uint64_t result = callSyscallWrapper(h, ca);
        return (int)result;
}

/**
 * @brief This function attaches system V shared memory on VH and register it with DMAATB.
 *
 * @note Argument is similar to shmat(2). Different points are shown below.
 * @note On Linux, it is possible to attach a shared memory segment even if it is already marked to be deleted. vh_shmat() follows it.
 *
 * @param [in] h VEO process handle
 * @param [in] shmid	System V shared memory segment identifier.
 * @param [in] shmaddr	This argument must be NULL.
 * @param [in] shmflag	SHM_RDONLY can be specified. Don't specify SHM_EXEC, SHM_REMAP and SHM_RND.
 * @param [out] vehva	An address of pointer to store VEHVA.
 *
 * @return A valid shared memory identifier on success, (void *)-1 and set errno on failure.
 * - EINVAL	shmaddr is not NULL.
 * - EINVAL	SHM_EXEC, SHM_REMAP or SHM_RND are specified.
 * - EINVAL	Invalid shmid value.
 * - EFAULT	vehva is invalid or the specified segment is not huge page.
 * - ENOMEM	Can't allocate DMAATB more. No enough memory at VH side.
 * - ECANCELED	Failed to update resource information (VEOS internal error).
 * - EACCES	The calling process does not have the required permissions
 * 		for the requested attach type, and does not have the
 * 		CAP_IPC_OWNER capability.
 * - ENOTSUP	VEOS does not connect to IVED.
 * - ENOTSUP	Library load or symbol retrieval failed.
 * - EIDRM	shmid points to a removed identifier.
 * - EBUSY	Starting VH-VE SHM attachment on VE or result wait failed.
 */
void *veo_shmat(veo_proc_handle *h, int shmid, const void *shmaddr, int shmflag, void **vehva) {
	if (vehva == NULL) {
		VEO_ERROR("NULL was specified for vehva.");
		errno = EFAULT;
		return (void *)-1;
	}
	veo_args *ca = veo_args_alloc();
        veo_args_set_u64(ca, 0, (uint64_t)SYS_sysve);
        veo_args_set_u64(ca, 1, (uint64_t)VE_SYSVE_VHSHM_CTL);
        veo_args_set_u64(ca, 2, (uint64_t)VHSHM_AT);
        veo_args_set_i64(ca, 3, shmid);
        veo_args_set_hmem(ca, 4, (void *)shmaddr);
        veo_args_set_i64(ca, 5, shmflag);
	veo_args_set_stack(ca, VEO_INTENT_OUT, 6, (char *)vehva, sizeof(void *));

	void *result = (void *)callSyscallWrapper(h, ca);
        return result;
}

/**
 * @brief This function detaches system V shared memory on VH and releases DMAATB entry.
 *
 * @param [in] h VEO process handle
 * @param [in] shmaddr	An address returned by veo_shmat(), which is attached to System V shared memory on VH
 *
 * @return 0 on success, -1 and set errno on failure.
 * - ECANCELED	Failed to update resource information (VEOS internal error).
 * - EINVAL	shmaddr is invalid. There is no shared memory segment
 * 		attached at shmaddr, or shmaddr is not aligned on a page
 * 		boundary.
 * - ENOTSUP	Library load or symbol retrieval failed.
 * - EBUSY	Starting VH-VE SHM detachment on VE or result wait failed.
 */
int veo_shmdt(veo_proc_handle *h, const void *shmaddr) {
	veo_args *ca = veo_args_alloc();
        veo_args_set_u64(ca, 0, (uint64_t)SYS_sysve);
        veo_args_set_u64(ca, 1, (uint64_t)VE_SYSVE_VHSHM_CTL);
        veo_args_set_u64(ca, 2, (uint64_t)VHSHM_DT);
        veo_args_set_hmem(ca, 3, (void *)shmaddr);

	uint64_t result = callSyscallWrapper(h, ca);
        return (int)result;
}
//@}
