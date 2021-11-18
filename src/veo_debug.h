#ifndef VEO_DEBUG_INCLUDE
#define VEO_DEBUG_INCLUDE

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
 * Debug print macros.
 * 
 * Copyright (c) 2020 Erich Focht
 */
//#define DEBUGMEM
//#define SYNCDMA

#ifndef __ve__
#include <unistd.h>
#include <sys/syscall.h>
#ifndef __cplusplus
long syscall(long n, ...);
#endif
static inline int _gettid(void)
{
  return syscall(SYS_gettid);
}
#endif

extern int veo_debug;

#ifdef __ve__
#define veo_dprintf(fmt, ...) do {                                      \
    if (veo_debug == 1) {                                               \
      fprintf(stdout, "[VE] " fmt, ## __VA_ARGS__);  \
      fflush(stdout);                                                   \
    }                                                                   \
  } while(0)
#else
#define veo_dprintf(fmt, ...) do {                                      \
    if (veo_debug == 1) {                                               \
      fprintf(stdout, "[VH] [TID %d]" fmt, _gettid(), ## __VA_ARGS__);    \
      fflush(stdout);                                                   \
    }                                                                   \
  } while(0)
#endif

#ifdef __ve__
#define veo_eprintf(fmt, ...) do {                                      \
    fprintf(stdout, "[VE] ERROR: " fmt, ## __VA_ARGS__); \
    fflush(stdout);                                                     \
  } while(0)
#else
#define veo_eprintf(fmt, ...) do {                                      \
    fprintf(stdout, "[VH] [TID %d] ERROR: " fmt, _gettid(), ## __VA_ARGS__); \
    fflush(stdout);                                                     \
  } while(0)
#endif

#endif /* VEO_DEBUG_INCLUDE */

