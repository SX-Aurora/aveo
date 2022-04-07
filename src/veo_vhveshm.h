/**
 * @file veo_vhveshm.h
 */
#ifndef _VEO_VHVESHM_H_
#define _VEO_VHVESHM_H_

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <signal.h>
#include "ve_offload.h"

#ifdef __cplusplus
extern "C" {
#endif
int veo_shmget(struct veo_proc_handle *, key_t, size_t, int);
void *veo_shmat(struct veo_proc_handle *, int, const void *, int, void **);
int veo_shmdt(struct veo_proc_handle *, const void *);
#ifdef __cplusplus
} // extern "C"
#endif
#endif
