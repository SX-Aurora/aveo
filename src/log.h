#ifndef _VEO_LOG_H_
#define _VEO_LOG_H_
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
 * Debug printout macros.
 * 
 * Copyright (c) 2018-2021 NEC Corporation
 * Copyright (c) 2020-2021 Erich Focht
 */
#include <stdarg.h>
#include "veo_debug.h"

#define VEO_ERROR(fmt, ...) veo_eprintf("%s() " fmt "\n", __FUNCTION__, ## __VA_ARGS__)
#define VEO_DEBUG(fmt, ...) veo_dprintf("[DEBUG] %s() " fmt "\n", __FUNCTION__, ## __VA_ARGS__)
#define VEO_TRACE(fmt, ...) veo_dprintf("[TRACE] %s() " fmt "\n", __FUNCTION__, ## __VA_ARGS__)

#define VEO_ASSERT(_cond) do { \
  if (!(_cond)) { \
    fprintf(stderr, "%s %d: Assertion failure: %s\n", \
              __FILE__, __LINE__, #_cond); \
    VEO_ERROR("%s %d: Assertion failure: %s", \
              __FILE__, __LINE__, #_cond); \
    abort(); \
  } \
} while (0)

// check req IDs. Result expected with exactly same req ID.
#define CHECK_REQ(nreq, req) do { \
  if (nreq != req) { \
    VEO_ERROR("send result req ID mismatch: %ld instead of %ld", nreq, req); \
    return -1; \
  } \
} while (0)

#endif // VEO_LOG_H
