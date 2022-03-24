/**
 * @file veo_vedma.h
 */
#ifndef _VEO_VEDMA_H_
#define _VEO_VEDMA_H_

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

typedef struct ve_dma_handle {
        int     status;
        int     index;
} ve_dma_handle_t;

int veo_dma_post(struct veo_proc_handle *, uint64_t, uint64_t, int, ve_dma_handle_t *);
int veo_dma_poll(struct veo_proc_handle *, ve_dma_handle_t *);
int veo_dma_post_wait(struct veo_proc_handle *, uint64_t, uint64_t, int);
uint64_t veo_register_mem_to_dmaatb(void *, size_t);
int veo_unregister_mem_from_dmaatb(struct veo_proc_handle *, uint64_t);
#ifdef __cplusplus
} // extern "C"
#endif
#endif
