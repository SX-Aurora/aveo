#ifndef _VEO_LOG_HPP_
#define _VEO_LOG_HPP_
#include <log4c.h>
#include <stdarg.h>
#include "urpc_debug.h"
namespace veo {
class ThreadContext;

enum VEOLogLevel{
  VEO_LOG_ERROR = LOG4C_PRIORITY_ERROR,
  VEO_LOG_DEBUG = LOG4C_PRIORITY_DEBUG,
  VEO_LOG_TRACE = LOG4C_PRIORITY_TRACE,
};

void veo__vlog(const ThreadContext *, const log4c_location_info_t *,
               int, const char *, va_list);
void veo__log(const ThreadContext *, const log4c_location_info_t *,
              int, const char *, ...);
} // namespace veo
#if 0
#define VEO_LOG(ctx, prio, fmt, ...) do { \
  const log4c_location_info_t location__ = \
    LOG4C_LOCATION_INFO_INITIALIZER(NULL); \
  veo__log(ctx, &location__, prio, fmt, ## __VA_ARGS__); \
} while (0)
#else
#define VEO_LOG(ctx, prio, fmt, ...) do { \
  const log4c_location_info_t location__ = \
    LOG4C_LOCATION_INFO_INITIALIZER(NULL); \
  printf("[VH] " fmt, ## __VA_ARGS__); \
} while (0)

#endif

#if 0
#define VEO_ERROR(ctx, fmt, ...) do {                 \
  VEO_LOG(ctx, veo::VEO_LOG_ERROR, fmt, ## __VA_ARGS__); \
  printf("VEO_ERROR " fmt, __VA_ARGS__);              \
  } while(0)
#define VEO_DEBUG(ctx, fmt, ...) VEO_LOG(ctx, veo::VEO_LOG_DEBUG, "[DEBUG]" fmt "\n", ## __VA_ARGS__)
#define VEO_TRACE(ctx, fmt, ...) VEO_LOG(ctx, veo::VEO_LOG_TRACE, "[TRACE]" fmt "\n", ## __VA_ARGS__)
#endif

#define VEO_ERROR(ctx, fmt, ...) eprintf("VEO_ERROR " fmt "\n", ## __VA_ARGS__)
#define VEO_DEBUG(ctx, fmt, ...) dprintf("[VH DEBUG] " fmt "\n", ## __VA_ARGS__)
#define VEO_TRACE(ctx, fmt, ...) dprintf("[VH TRACE] " fmt "\n", ## __VA_ARGS__)

#define VEO_ASSERT(_cond) do { \
  if (!(_cond)) { \
    fprintf(stderr, "%s %d: Assertion failure: %s\n", \
              __FILE__, __LINE__, #_cond); \
    VEO_ERROR(nullptr, "%s %d: Assertion failure: %s", \
              __FILE__, __LINE__, #_cond); \
    abort(); \
  } \
} while (0)

#endif
