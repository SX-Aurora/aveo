#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>

#include <ve_offload.h>
#include "urpc_time.h"

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
        uint64_t sym = veo_get_sym(proc, libh, "hello");
        printf("'hello' sym = %p\n", (void *)sym);
        if (sym == 0)
		return -1;

        struct veo_thr_ctxt *ctx = veo_context_open(proc);
        printf("ctx = %p\n", (void *)ctx);
        
        struct veo_args *argp = veo_args_alloc();
        veo_args_set_i32(argp, 0, 42);
        uint64_t result = 0;

        uint64_t req = veo_call_async(ctx, sym, argp);
        printf("call_async returned %lu\n", req);

        err = veo_call_wait_result(ctx, req, &result);
        printf("wait_result returned %d result=%lu\n", err, result);

        sym = veo_get_sym(proc, libh, "empty");
        printf("'empty' sym = %p\n", (void *)sym);

        veo_args_clear(argp);
        long ts, te;
        uint64_t reqs[nloop], res[nloop];

        ts = get_time_us();
        for (int i=0; i<nloop; i++) {
          reqs[i] = veo_call_async(ctx, sym, argp);
          //printf("submitted req %d\n", i);
        }
        err = 0;
        uint64_t sum = 0;
        for (int i=0; i<nloop; i++) {
          err += veo_call_wait_result(ctx, reqs[i], &res[i]);
          sum += res[i];
          //printf("received result req %d\n", i);
        }
        
        te = get_time_us();
        printf("%d async calls took %fs, %f us/call\n",
               nloop, (double)(te-ts)/1.e6, (double)(te-ts)/nloop);
        printf("cumulated err=%d\n", err);
        printf("sum=%lu, expected=%ld\n", sum, nloop*(nloop+1)/2);

        err = veo_context_close(ctx);
        printf("context_close returned %d\n", err);
        
        err = veo_proc_destroy(proc);
        printf("veo_proc_destroy() returned %d\n", err);

        return 0;
}

