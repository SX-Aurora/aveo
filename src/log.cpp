#include <mutex>
#include <string>
#include <cstdio>

#include "ve_offload.h"
#include "log.hpp"
#include "ThreadContext.hpp"

extern "C" {
log4c_category_t *cat_pseudo_core;
}

namespace veo {
namespace log {
std::mutex log_mtx_;
log4c_category_t *log_category_;
}

void veo__vlog(const ThreadContext *ctx, const log4c_location_info_t *loc,
               int prio, const char *fmt, va_list list)
{
  //TODO: switch to disable logging for performance
  // ctx can be nullptr.
  std::lock_guard<std::mutex> lock(log::log_mtx_);
  std::string newfmt;
  if (ctx != nullptr) {
    char header[32];
    snprintf(header, sizeof(header), "[context %p] ", ctx);
    newfmt = header;
    newfmt += fmt;
    fmt = newfmt.c_str();
  }
  __log4c_category_vlog(log::log_category_, loc, prio, fmt, list);
}
void veo__log(const ThreadContext *ctx, const log4c_location_info_t *loc,
              int prio, const char *fmt, ...)
{
  va_list ap;
  va_start(ap, fmt);
  veo__vlog(ctx, loc, prio, fmt, ap);
  va_end(ap);
}
}// namespace veo

__attribute__((constructor))
static void veo_log_init(void)
{
  log4c_init();
  veo::log::log_category_ = log4c_category_get(VEO_LOG_CATEGORY);
  cat_pseudo_core = log4c_category_get("veos.veo.pseudo");
}
