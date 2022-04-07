#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include "veo_vedma.h"
#include "veo_veshm.h"
#include "veo_api.hpp"
#include "VEOException.hpp"

using veo::api::ProcHandleFromC;
using veo::api::ContextFromC;
using veo::api::CallArgsFromC;
using veo::api::ThreadContextAttrFromC;
using veo::VEOException;

/**
 * \defgroup veovedmaapi VEO VE DMA API (low-level)
 *
 * VE DMA API functions for VEO.
 * This API is a low-level API and is intended to be called by upper layer software.
 * This API is not intended to be called by a user program.
 * To use VEO VE DMA API functions, include "ve_offload.h" header and "veo_vedma.h" header.
 */



//@{

/**
 * @brief This function issues asynchronous DMA
 *
 * @note This function writes the DMA transfer request to the DMA descriptor table.
 *
 * @param [in] phandle VEO process handle
 * @param [in] dst 4 byte aligned VE host virtual address of destination
 * @param [in] src 4 byte aligned VE host virtual address of source
 * @param [in] size Transfer size which is a multiple of 4 and less than 128MB
 * @param [out] dhandle Handle used to inquire DMA completion
 *
 * @retval 0 On success
 * @retval -EAGAIN The DMA using the DMA descriptor to be used next is not yet completed 
 * @n
 *      Need to call vedma_post() again.
 * @retval -ENOTSUP	Library load or symbol retrieval failed.
 * @retval -EBUSY	DMA start on VE or result wait failed.
 * @retval -EFAULT	dhandle is invalid.
 */
int veo_dma_post(veo_proc_handle *phandle, uint64_t dst, uint64_t src, int size, ve_dma_handle_t *dhandle) {
	if (dhandle == NULL) {
                VEO_ERROR("NULL was specified for VE DMA handle.");
		return -EFAULT;
	}
	veo_args *ca = veo_args_alloc();
	veo_args_set_u64(ca, 0, dst);
	veo_args_set_u64(ca, 1, src);
	veo_args_set_i64(ca, 2, size);
	veo_args_set_stack(ca, VEO_INTENT_OUT, 3, (char *)dhandle, sizeof(ve_dma_handle_t));

	uint64_t result = 0;
	uint64_t libh = ProcHandleFromC(phandle)->loadVE2VELibrary(LIBVEIO);
	if (libh == 0UL)
                return -ENOTSUP;
	uint64_t sym_addr = ProcHandleFromC(phandle)->getVEDMASyms(libh, "ve_dma_post");
	if (sym_addr == 0UL)
                return -ENOTSUP;
	int ret = ProcHandleFromC(phandle)->callSync(sym_addr, *CallArgsFromC(ca), &result);
	if (ret != VEO_COMMAND_OK) {
		return -EBUSY;
        }

	return (int)result;
}

/**
 * @brief This function inquiries the completion of asynchronous DMA
 *
 * @note This function inquiries the completion of DMA transfer request issued by vedma_post().
 *
 * @param [in] phandle VEO process handle
 * @param [in] dhandle DMA transfer request issued by vedma_post()
 *
 * @retval 0 DMA completed normally
 * @retval 1-65535 DMA failed @n
 *      Exception value of DMA descriptor is returned. @n
 *      DMA failed. The value is bitwise ORed combination of the values described below.
 * -    0x8000: Memory protection exception
 * -    0x4000: Missing page exception
 * -    0x2000: Missing space exception
 * -    0x1000: Memory access exception
 * -    0x0800: I/O access exception
 * @retval -EAGAIN	DMA has not completed yet @n
 * 			Need to call vedma_poll() again
 * @retval -ENOTSUP	Library load or symbol retrieval failed.
 * @retval -EBUSY	DMA start on VE or result wait failed.
 * @retval -EFAULT	dhandle is invalid.
 */
int veo_dma_poll(veo_proc_handle *phandle, ve_dma_handle_t *dhandle) {
        if (dhandle == NULL) {
                VEO_ERROR("NULL was specified for VE DMA handle.");
                return -EFAULT;
        }
	veo_args *ca = veo_args_alloc();
	veo_args_set_stack(ca, VEO_INTENT_IN, 0, (char *)dhandle, sizeof(ve_dma_handle_t));

	uint64_t result = 0;
	uint64_t libh = ProcHandleFromC(phandle)->loadVE2VELibrary(LIBVEIO);
	if (libh == 0UL)
                return -ENOTSUP;
	uint64_t sym_addr = ProcHandleFromC(phandle)->getVEDMASyms(libh, "ve_dma_poll");
	if (sym_addr == 0UL)
                return -ENOTSUP;

	int ret = ProcHandleFromC(phandle)->callSync(sym_addr, *CallArgsFromC(ca), &result);
	if (ret != VEO_COMMAND_OK)
                return -EBUSY;

        if (result & 0x8000) {
                VEO_ERROR("memory protection exception\n");
        } else if (result & 0x4000) {
                VEO_ERROR("missing page exception\n");
        } else if (result & 0x2000) {
                VEO_ERROR("missing space exception\n");
        } else if (result & 0x1000) {
                VEO_ERROR("memory access exception\n");
        } else if (result & 0x0800) {
                VEO_ERROR("I/O access exception\n");
        }
	return (int)result;
}

/**
 * @brief This function issues synchronous DMA
 *
 * @note This function writes the DMA transfer request to the DMA descriptor table, and waits for finish.
 *
 * @param [in] phandle VEO process handle
 * @param [in] dst 4 byte aligned VE host virtual address of destination
 * @param [in] src 4 byte aligned VE host virtual address of source
 * @param [in] size Transfer size which is a multiple of 4 and less than 128MB
 *
 * @retval 0 On success
 * @retval 1-65535 DMA failed @n
 *      Exception value of DMA descriptor is returned. @n
 *      DMA failed. The value is bitwise ORed combination of the values described below. @n
 * -    0x8000: Memory protection exception
 * -    0x4000: Missing page exception
 * -    0x2000: Missing space exception
 * -    0x1000: Memory access exception
 * -    0x0800: I/O access exception
 * @retval -ENOTSUP	Library load or symbol retrieval failed.
 * @retval -EBUSY	DMA start on VE or result wait failed.
 */
int veo_dma_post_wait(veo_proc_handle *phandle, uint64_t dst, uint64_t src, int size) {
	veo_args *ca = veo_args_alloc();
	veo_args_set_u64(ca, 0, dst);
	veo_args_set_u64(ca, 1, src);
	veo_args_set_i64(ca, 2, size);

	uint64_t result = 0;
	uint64_t libh = ProcHandleFromC(phandle)->loadVE2VELibrary(LIBVEIO);
	if (libh == 0UL)
		return -ENOTSUP;
	uint64_t sym_addr = ProcHandleFromC(phandle)->getVEDMASyms(libh, "ve_dma_post_wait");
	if (sym_addr == 0UL)
		return -ENOTSUP;
	int ret = ProcHandleFromC(phandle)->callSync(sym_addr, *CallArgsFromC(ca), &result);
	if (ret != VEO_COMMAND_OK)
		return -EBUSY;

        if (result & 0x8000) {
                VEO_ERROR("memory protection exception\n");
        } else if (result & 0x4000) {
                VEO_ERROR("missing page exception\n");
        } else if (result & 0x2000) {
                VEO_ERROR("missing space exception\n");
        } else if (result & 0x1000) {
                VEO_ERROR("memory access exception\n");
        } else if (result & 0x0800) {
                VEO_ERROR("I/O access exception\n");
        }
	return (int)result;
}

/**
 * @brief This function registers VE local memory to DMAATB
 *
 * @note The default value of VE page size is 64MB. @n
 *      In this case, values of vemva and size need to be aligned with 64MB boundary. @n
 *      If VE page size is 2MB, values of vemva and size need to be aligned with 2MB boundary.
 *
 * @param [in] vemva     An address of memory to be registered @n
 *      The address needs to be aligned with 64MB boundary.
 * @param [in] size      Size of memory aligned aligned with 64MB boundary
 *
 * @retval vehva VE host virtual address on success
 * @retval  0xffffffffffffffff On failure (set errno)
 * - EINVAL	Invalid argument.
 * - EFAULT	Bad address.
 * - ESRCH	No such process.
 * - ENOENT	No such memory.
 * - EACCES	Permission denied.
 * - ECANCEL	Operation canceled.
 * - ENOTSUP    Library load or symbol retrieval failed.
 * - EBUSY      Starting memory registration to DMAATB or waiting for result of it failed.
 */
uint64_t veo_register_mem_to_dmaatb(void *vemva, size_t size) {
	veo_proc_handle *h = veo_get_proc_handle_from_hmem(vemva);
	pid_t pid = veo_get_pid_from_hmem(vemva);
        int syncnum = 0;
        int mode_flag = VE_MEM_LOCAL;
	
	return (uint64_t)veo_shared_mem_attach(h, pid, vemva, size, syncnum, mode_flag);
}

/**
 * @brief This function unregisters VE local memory from DMAATB
 *
 * @param [in] h VEO process handle
 * @param [in] vehva VE host virtual address of memory to be unregistered
 *
 * @retval 0 On success
 * @retval -1 On failure (set errno)
 * - EINVAL	Invalid argument.
 * - ECANCELED	No such memory.
 * - ENOTSUP    Library load or symbol retrieval failed.
 * - EBUSY      Starting memory unregistration from DMAATB or waiting for result of it failed.
 */
int veo_unregister_mem_from_dmaatb(veo_proc_handle *h, uint64_t vehva) {
        int mode_flag = VE_MEM_LOCAL;
	return veo_shared_mem_detach(h, (void *)vehva, mode_flag);
}
//@}
