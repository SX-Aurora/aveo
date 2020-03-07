#ifndef VEO_DEBUG_INCLUDE
#define VEO_DEBUG_INCLUDE

//#define DEBUG 1
//#define DEBUGMEM
//#define SYNCDMA

#ifdef DEBUG

#ifdef __ve__
#define dprintf(fmt, ...) do {                \
    fprintf(stdout, "[VE] " fmt, ## __VA_ARGS__);    \
    fflush(stdout);                              \
  } while(0)
#else
#define dprintf(fmt, ...) do {                                       \
    fprintf(stdout, "[VH] " fmt, ## __VA_ARGS__);         \
    fflush(stdout);                             \
  } while(0)
#endif

#else

#define dprintf(args...)

#endif

#ifdef __ve__
#define eprintf(fmt, ...) do {               \
    fprintf(stdout, "[VE] ERROR: " fmt, ## __VA_ARGS__);        \
    fflush(stdout);                             \
  } while(0)
#else
#define eprintf(fmt, ...) do {                                        \
    fprintf(stdout, "[VH] ERROR: " fmt, ## __VA_ARGS__);              \
    fflush(stdout);                             \
  } while(0)
#endif

#endif /* VEO_DEBUG_INCLUDE */

