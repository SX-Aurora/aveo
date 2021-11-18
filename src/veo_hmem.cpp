/**
 * @file veo_hmem_macros.h
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

/**
 * @brief check the address
 *
 * This function determines if the address is allocated by
 * veo_alloc_hmem().
 *
 * @param addr a pointer to virtual address
 * @return one when addr was allocated by veo_alloc_hmem();
 *         zero when addr was not allocated by veo_alloc_hmem().
 */
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include "veo_hmem.h"
#include "veo_hmem_macros.h"

int veo_is_ve_addr(const void *addr)
{
  return IS_VE(addr) ? 1 : 0;
}

/**
 * @brief get a virtual address of VE or VH memory without 
 * identifier allocated by veo_alloc_hmem().
 * @param addr a pointer to virtual address of VE or VH memory
 * @return a virtual address without identifier
 */
void* veo_get_hmem_addr(void *addr)
{
  if (veo_is_ve_addr(addr))
    return (void *)(~VEIDENT_MBITS & (uint64_t)addr);
  else
    return addr;
}
//@}
