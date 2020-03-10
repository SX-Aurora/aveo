#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <assert.h>
#include <ve_offload.h>
#include "veo_time.h"

int nloop = 10000;

int main(int argc, char *argv[])
{
	int err = 0;
        struct veo_proc_handle *proc;

	if (argc > 1) {
		nloop = atoi(argv[1]);
	}

  proc = veo_proc_create(-1);
  //printf("proc = %p\n", (void *)proc);
  if (proc == NULL)
		return -1;
  uint64_t libh = veo_load_library(proc, "./libvearith_ftrace.so");
  //printf("libh = %p\n", (void *)libh);
  if (libh == 0)
		return -1;

  struct veo_thr_ctxt *ctx = veo_context_open(proc);
        
  struct veo_args *argp = veo_args_alloc();
  veo_args_set_i32(argp, 0, 1000000);
  uint64_t result = 0;

  uint64_t req = veo_call_async_by_name(ctx, libh, "alloc_init", argp);
  err = veo_call_wait_result(ctx, req, &result);
  assert(err == 0);
  
  veo_args_clear(argp);
  veo_args_set_i32(argp, 0, 1000000);
  req = veo_call_async_by_name(ctx, libh, "sum", argp);
  err = veo_call_wait_result(ctx, req, &result);
  assert(err == 0);
  
  veo_args_clear(argp);
  veo_args_set_i32(argp, 0, 1000000);
  
  req = veo_call_async_by_name(ctx, libh, "mult_add", argp);
  err = veo_call_wait_result(ctx, req, &result);
  assert(err == 0);

  err = veo_context_close(ctx);
  assert(err == 0);
  err = veo_proc_destroy(proc);
  assert(err == 0);
  return 0;
}
