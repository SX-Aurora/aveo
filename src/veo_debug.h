#ifndef VEO_DEBUG_INCLUDE
#define VEO_DEBUG_INCLUDE

//#define DEBUGMEM
//#define SYNCDMA

extern int veo_debug;

#ifdef __ve__
#define veo_dprintf(fmt, ...) do {                \
    if (veo_debug == 1) { \
        fprintf(stdout, "[VE] " fmt, ## __VA_ARGS__);    \
        fflush(stdout); \
    }                              \
  } while(0)
#else
#define veo_dprintf(fmt, ...) do {                                       \
    if (veo_debug == 1) { \
        fprintf(stdout, "[VH] " fmt, ## __VA_ARGS__);         \
        fflush(stdout);                             \
    } \
  } while(0)
#endif

#ifdef __ve__
#define veo_eprintf(fmt, ...) do {               \
    fprintf(stdout, "[VE] ERROR: " fmt, ## __VA_ARGS__);        \
    fflush(stdout);                             \
  } while(0)
#else
#define veo_eprintf(fmt, ...) do {                                        \
    fprintf(stdout, "[VH] ERROR: " fmt, ## __VA_ARGS__);              \
    fflush(stdout);                             \
  } while(0)
#endif

#endif /* VEO_DEBUG_INCLUDE */

