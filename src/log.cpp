/**
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
 * Logging.
 * 
 * Copyright (c) 2020 NEC Corporation
 */
#include <iostream>
#include "log.h"

int veo_debug;

__attribute__((constructor(10000)))
static void veo_log_init(void)
{
	const char* env_p = std::getenv("VEO_LOG_DEBUG");
	if (env_p) {
		VEO_DEBUG("Enable Debug log");
		veo_debug = 1;
	}
}
