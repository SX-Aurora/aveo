#include <iostream>
#include "log.h"

int veo_debug;

__attribute__((constructor))
static void veo_log_init(void)
{
	const char* env_p = std::getenv("VEO_LOG_DEBUG");
	if (env_p) {
		VEO_DEBUG("Enable Debug log");
		veo_debug = 1;
	}
}
