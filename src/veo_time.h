#ifndef VEO_TIME_INCLUDE
#define VEO_TIME_INCLUDE
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
 * Timing and delay functions.
 * 
 * Copyright (c) 2020 Erich Focht
 */

#include <stdint.h>
#include <sys/time.h>

#ifdef __ve__

static inline long getusrcc()
{
	asm("smir %s0, %usrcc");
}

static inline long get_time_us(void)
{
	/* This implies that the VE runs with 1400MHz,
	   the exact frequency doesn't actually matter for this purpose,
	   but should eventually be fixed.
	*/
	return getusrcc() / 1400;
}

#else /* VH, i.e. x86_64 */

static inline long get_time_us(void)
{
	struct timeval t;

	gettimeofday(&t, NULL);
	return (long)(t.tv_sec * 1000000 + t.tv_usec);
}

#endif

static inline long timediff_us(long ts)
{
	return get_time_us() - ts;
}

static inline void busy_sleep_us(long us)
{
	long ts = get_time_us();
	do {} while (timediff_us(ts) < us);
}

#endif /* VEO_TIME_INCLUDE */
