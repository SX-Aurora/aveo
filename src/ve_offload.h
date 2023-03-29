/* Copyright (C) 2017-2022 by NEC Corporation
 * Copyright (C) 2020-2022 Erich Focht  
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */
/**
 * @file ve_offload.h
 */
#ifndef _VE_OFFLOAD_H_
#define _VE_OFFLOAD_H_

#define VEO_API_VERSION 15
#define VEO_SYMNAME_LEN_MAX (255)
#define VEO_LOG_CATEGORY "veos.veo.veo"
#define VEO_MAX_NUM_ARGS (256)

#define VEO_REQUEST_ID_INVALID (~0UL)

#define CMD_VEGDB "/opt/nec/ve/bin/ve-gdb"
#define CMD_XTERM "/usr/bin/xterm"

#include <stdint.h>
#include <stddef.h>
#include <unistd.h>
#include <sys/types.h>
#include "veo_hmem.h"

#ifdef __cplusplus
extern "C" {
#endif
enum veo_context_state {
  VEO_STATE_UNKNOWN = 0,
  VEO_STATE_RUNNING,
  VEO_STATE_SYSCALL,	// not possible in AVEO
  VEO_STATE_BLOCKED,	// not possible in AVEO
  VEO_STATE_EXIT,
};

enum veo_command_state {
  VEO_COMMAND_OK = 0,
  VEO_COMMAND_EXCEPTION,
  VEO_COMMAND_ERROR,
  VEO_COMMAND_UNFINISHED,
};

enum veo_queue_state {
  VEO_QUEUE_READY = 0,
  VEO_QUEUE_CLOSED,
};

enum veo_args_intent {
  VEO_INTENT_IN = 0,
  VEO_INTENT_INOUT,
  VEO_INTENT_OUT,
};

struct veo_args;
struct veo_proc_handle;
struct veo_thr_ctxt;
struct veo_thr_ctxt_attr;

void veo_register_hook(void* func, void (*hook)(void*, ...), void* payload);
void *veo_get_hook(void* func);
void veo_unregister_hook(void* func);

struct veo_proc_handle *veo_proc_create(int);
struct veo_proc_handle *veo_proc_create_static(int, char *);
int veo_proc_destroy(struct veo_proc_handle *);
int veo_proc_identifier(struct veo_proc_handle *);
void *veo_set_proc_identifier(void *addr, int proc_ident);
uint64_t veo_load_library(struct veo_proc_handle *, const char *);
int veo_unload_library(struct veo_proc_handle *, const uint64_t);
uint64_t veo_get_sym(struct veo_proc_handle *, uint64_t, const char *);
int veo_alloc_mem(struct veo_proc_handle *, uint64_t *, const size_t);
int veo_free_mem(struct veo_proc_handle *, uint64_t);
int veo_read_mem(struct veo_proc_handle *, void *, uint64_t, size_t);
struct veo_proc_handle *veo_get_proc_handle_from_hmem(const void *);
pid_t veo_get_pid_from_hmem(const void *);
int veo_write_mem(struct veo_proc_handle *, uint64_t, const void *, size_t);
int veo_num_contexts(struct veo_proc_handle *);
struct veo_thr_ctxt *veo_get_context(struct veo_proc_handle *, int);

struct veo_thr_ctxt *veo_context_open(struct veo_proc_handle *);
int veo_context_close(struct veo_thr_ctxt *);
int veo_get_context_state(struct veo_thr_ctxt *);
void veo_context_sync(struct veo_thr_ctxt *);

struct veo_args *veo_args_alloc(void);
int veo_args_set_i64(struct veo_args *, int, int64_t);
int veo_args_set_u64(struct veo_args *, int, uint64_t);
int veo_args_set_i32(struct veo_args *, int, int32_t);
int veo_args_set_u32(struct veo_args *, int, uint32_t);
int veo_args_set_i16(struct veo_args *, int, int16_t);
int veo_args_set_u16(struct veo_args *, int, uint16_t);
int veo_args_set_i8(struct veo_args *, int, int8_t);
int veo_args_set_u8(struct veo_args *, int, uint8_t);
int veo_args_set_double(struct veo_args *, int, double);
int veo_args_set_float(struct veo_args *, int, float);
int veo_args_set_stack(struct veo_args *, enum veo_args_intent,
                       int, char *, size_t);
void veo_args_clear(struct veo_args *);
void veo_args_free(struct veo_args *);

int veo_call_sync(struct veo_proc_handle *h, uint64_t addr,
                  struct veo_args *ca, uint64_t *result);

uint64_t veo_call_async(struct veo_thr_ctxt *, uint64_t, struct veo_args *);
uint64_t veo_call_async_by_name(struct veo_thr_ctxt *, uint64_t, const char *,
                                struct veo_args *);
uint64_t veo_call_async_vh(struct veo_thr_ctxt *, uint64_t (*)(void *), void *);

int veo_call_peek_result(struct veo_thr_ctxt *, uint64_t, uint64_t *);
int veo_call_wait_result(struct veo_thr_ctxt *, uint64_t, uint64_t *);

uint64_t veo_alloc_mem_async(struct veo_thr_ctxt *ctx, const size_t size);
uint64_t veo_free_mem_async(struct veo_thr_ctxt *ctx, uint64_t addr);
uint64_t veo_async_read_mem(struct veo_thr_ctxt *, void *, uint64_t, size_t);
uint64_t veo_async_write_mem(struct veo_thr_ctxt *, uint64_t, const void *,
                             size_t);
void veo_req_block_begin(struct veo_thr_ctxt *ctx);
void veo_req_block_end(struct veo_thr_ctxt *ctx);

struct veo_thr_ctxt *veo_context_open_with_attr(
			struct veo_proc_handle *, struct veo_thr_ctxt_attr *);
struct veo_thr_ctxt_attr *veo_alloc_thr_ctxt_attr(void);
int veo_free_thr_ctxt_attr(struct veo_thr_ctxt_attr *);
int veo_set_thr_ctxt_stacksize(struct veo_thr_ctxt_attr *, size_t);
int veo_get_thr_ctxt_stacksize(struct veo_thr_ctxt_attr *, size_t *);

const char *veo_version_string(void);
int veo_api_version(void);

int veo_alloc_hmem(struct veo_proc_handle *, void **, const size_t);
int veo_free_hmem(void *);
int veo_hmemcpy(void *, const void *, size_t);
int veo_args_set_hmem(struct veo_args *, int, void *);

int veo_get_venum_from_hmem(const void *);
int veo_get_ve_arch(int ve_node_numember);

void veo_register_hmem_hook_functions(void (*)(void *, size_t), void (*)(uint64_t));
void veo_unregister_hmem_hook_functions();
#ifdef __cplusplus
} // extern "C"
#endif
#endif
