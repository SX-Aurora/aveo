/**
 * @file veo_hmem.cpp
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
 * VEO heterogeneous memory implementation.
 * 
 * Copyright (c) 2018-2021 NEC Corporation
 *
 *
 * \defgroup veohmemapi VEO HMEM API
 *
 * VE heterogeneous memory API functions.
 * Include "ve_offload.h" header from VH program. Specify -lveo when you link VH program.
 * Include "veo_hmem.h" header from VH program. Specify -lveohmem when you link VE program.
 */
//@{

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include "veo_vedma.h"
#include "veo_hmem.h"
#include "veo_hmem_macros.h"

/**
 * @brief check the address
 *
 * This function determines if the address is allocated by
 * veo_alloc_hmem().
 *
 * @param [in] addr a pointer to virtual address
 * @retval 1 addr was allocated by veo_alloc_hmem().
 * @retval 0 addr was not allocated by veo_alloc_hmem().
 */
int veo_is_ve_addr(const void *addr)
{
  return IS_VE(addr) ? 1 : 0;
}

/**
 * @brief get a virtual address of VE or VH memory without 
 * identifier allocated by veo_alloc_hmem().
 * @param [in] addr a pointer to virtual address of VE or VH memory
 * @return a virtual address without identifier
 */
void* veo_get_hmem_addr(void *addr)
{
  if (veo_is_ve_addr(addr))
    return (void *)(~VEIDENT_MBITS & (uint64_t)addr);
  else
    return addr;
}

/**
 * @brief Get the number of identifiable VE processes
 *
 * @note This API is a low-level API and is intended to be called
 *       by upper layer software. This API is not intended to be 
 *       called by a user program.
 *
 * @return the number of identifiable VE processes
 */
int veo_get_max_proc_identifier(void)
{
  return VEO_MAX_HMEM_PROCS;
}

/**
 * @brief Get the number of VE process identifier from HMEM addr
 *
 * @note This API is a low-level API and is intended to be called
 *       by upper layer software. This API is not intended to be 
 *       called by a user program.
 *
 * @param [in] addr a pointer to VEMVA address with the identifier (HMEM addr)
 * @return a VE process identifer
 */
int veo_get_proc_identifier_from_hmem(const void *addr)
{
  if(IS_VE(addr) == (uint64_t)0) {
    return -1;
  } else {
    return GET_PROC_IDENT(addr);
  }
}
//@}

signed long syscall_wrapper(int number, signed long arg0, signed long arg1,
	signed long arg2, signed long arg3, signed long arg4, signed long arg5)
{
	signed long ret;
	ret = syscall(number, arg0, arg1, arg2, arg3, arg4, arg5);
	if (ret == -1)
		ret = -errno;
	return ret;
}
