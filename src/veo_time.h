#ifndef VEO_TIME_INCLUDE
#define VEO_TIME_INCLUDE

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
