#ifndef _VEO_LOG_H_
#define _VEO_LOG_H_
#include <stdarg.h>
#include "veo_debug.h"

#define VEO_ERROR(fmt, ...) eprintf("%s() " fmt "\n", __FUNCTION__, ## __VA_ARGS__)
#define VEO_DEBUG(fmt, ...) dprintf("[DEBUG] %s() " fmt "\n", __FUNCTION__, ## __VA_ARGS__)
#define VEO_TRACE(fmt, ...) dprintf("[TRACE] %s() " fmt "\n", __FUNCTION__, ## __VA_ARGS__)

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
