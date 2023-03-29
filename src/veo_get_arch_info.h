/**
 * Copyright (C) 2021 NEC Corporation
 * This file is part of the AVEO.
 *
 * The AVEO is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation; either version 2.1
 * of the License, or (at your option) any later version.
 *
 * The AVEO is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with the AVEO; if not, see
 * <http://www.gnu.org/licenses/>.
 */

/**
 * @file veo_get_arch_info.h 
 */
#ifndef _GET_ARCH_INFO_H_
#define _GET_ARCH_INFO_H_

#define CLASS_VE	"/sys/class/ve"	/* Part #1 of ve_arch_class */
#define ARCH_FILE	"ve_arch_class"	/* Part #2 of ve_arch_class */
#define ARCH_FILE_BSIZE	16		/* buffer of contents in ve_arch_class */
#define ARCH_PATH_BSIZE	64		/* buffer of "/sys/class/ve/ve#/ve_arch_class" */

extern int veo_arch_number_sysfs(int);

#endif
