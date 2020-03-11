#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>

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
  printf("proc = %p\n", (void *)proc);
  if (proc == NULL)
		return -1;
  uint64_t libh = veo_load_library(proc, "./libvehello.so");
  printf("libh = %p\n", (void *)libh);
  if (libh == 0)
		return -1;
  struct veo_thr_ctxt *ctx = veo_context_open(proc);
  printf("ctx = %p\n", (void *)ctx);

  struct veo_thr_ctxt *ctx2 = veo_context_open(proc);
  printf("ctx2 = %p\n", (void *)ctx2);

  struct veo_args *argp = veo_args_alloc();
  veo_args_clear(argp);
  long ts, te;
  int64_t res[nloop], res2[nloop];
  uint64_t reqs[nloop], reqs2[nloop];

  ts = get_time_us();
  for (int i=0; i<nloop; i++) {
    reqs[i] = veo_call_async_by_name(ctx, libh, "empty", argp);
  }
  for (int i=0; i<nloop; i++) {
    reqs2[i] = veo_call_async_by_name(ctx2, libh, "empty2", argp);
  }
        err = 0;
        int64_t sum = 0;
        
        for (int i=0; i<nloop; i++) {
          err += veo_call_wait_result(ctx, reqs[i], &res[i]);
          sum += res[i];
        }
        for (int i=0; i<nloop; i++) {
          err += veo_call_wait_result(ctx2, reqs2[i], &res2[i]);
          sum += res2[i];
        }
        
        te = get_time_us();
        printf("%d async calls took %fs, %f us/call\n",
               2*nloop, (double)(te-ts)/1.e6, (double)(te-ts)/nloop/2);
        printf("cumulated err=%d\n", err);
        printf("sum=%lu, expected=%ld\n", sum, nloop*(nloop+1));

        printf("veo_num_contexts() = %d\n", veo_num_contexts(proc));
        printf("context1 veo_get_context(0) = %p\n", veo_get_context(proc, 0));
        printf("context2 veo_get_context(1) = %p\n", veo_get_context(proc, 1));
        err = veo_context_close(ctx2);
        printf("context_close(ctx2) returned %d\n", err);
        printf("veo_num_contexts() = %d\n", veo_num_contexts(proc));
        err = veo_context_close(ctx);
        printf("context_close(ctx) returned %d\n", err);
        err = veo_proc_destroy(proc);
        printf("veo_proc_destroy() returned %d\n", err);

        return 0;
}

