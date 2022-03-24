/**
 * @file veo_veshm.h
 */
#ifndef _VEO_VESHM_H_
#define _VEO_VESHM_H_

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <signal.h>
#include "ve_offload.h"
#include "veo_veshm_defs.h"

#ifdef __cplusplus
extern "C" {
#endif
int veo_shared_mem_open(void *, size_t, int, long long);
int veo_shared_mem_close(void *, size_t, int, long long);
void *veo_shared_mem_attach(struct veo_proc_handle *, pid_t, void *, size_t , int, long long);
int veo_shared_mem_detach(struct veo_proc_handle *, void *, long long);
#ifdef __cplusplus
} // extern "C"
#endif
#endif
