#ifndef _VEO_HMEM_H_
#define _VEO_HMEM_H_
/**
 * @file veo_hmem.h
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
 * VEO heterogeneous memory related prototypes.
 * 
 * Copyright (c) 2020-2021 NEC Corporation
 */

#ifdef __cplusplus
extern "C" {
#endif
int veo_is_ve_addr(const void *);
void *veo_get_hmem_addr(void *);
int veo_get_max_proc_identifier(void);
int veo_get_proc_identifier_from_hmem(const void *);
signed long syscall_wrapper(int, signed long, signed long, signed long,
                signed long, signed long, signed long);
#ifdef __cplusplus
} // extern "C"
#endif
#endif
