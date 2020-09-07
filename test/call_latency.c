#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>

#include <ve_offload.h>
#include "veo_time.h"

int main(int argc, char *argv[])
{
	int err = 0;
        int nloop = 10000;

        struct veo_proc_handle *proc;
        struct veo_thr_ctxt *ctx;

	if (argc == 1) {
		printf("Usage:\n\t%s <nloop>\n", argv[0]);
		exit(1);
	}
	nloop = atoi(argv[1]);

        proc = veo_proc_create(-1);
        printf("proc = %p\n", (void *)proc);
        if (proc == NULL)
		return -1;
        uint64_t libh = veo_load_library(proc, "./libvehello.so");
        if (libh == 0)
		return -1;
        uint64_t sym = veo_get_sym(proc, libh, "empty");
        if (sym == 0)
		return -1;
        ctx = veo_context_open(proc);
        
        struct veo_args *argp = veo_args_alloc();
        uint64_t result = 0;

        veo_args_clear(argp);
        long ts, te;
        uint64_t reqs[nloop], res[nloop];

        //------- warm up ------
        err = 0;
        reqs[0] = veo_call_async(ctx, sym, argp);
        err += veo_call_wait_result(ctx, reqs[0], &res[0]);

	//----------------------
	printf("Test 1: submit %d calls, then wait for %d results\n", nloop, nloop);
        ts = get_time_us();
        err = 0;
        for (int i=0; i<nloop; i++) {
          reqs[i] = veo_call_async(ctx, sym, argp);
        }
        for (int i=0; i<nloop; i++) {
          err += veo_call_wait_result(ctx, reqs[i], &res[i]);
        }
        
        te = get_time_us();
        printf("%d async calls + waits took %fs, %fus/call\n\n",
               nloop, (double)(te-ts)/1.e6, (double)(te-ts)/nloop);
	if (err != 0)
		printf("cummulated err=%d !? Something's wrong.\n", err);

	//----------------------
	printf("Test 2: submit one call and wait for its result, %d times\n", nloop);
        ts = get_time_us();
        err = 0;
        for (int i=0; i<nloop; i++) {
          reqs[i] = veo_call_async(ctx, sym, argp);
          err += veo_call_wait_result(ctx, reqs[i], &res[i]);
        }
        te = get_time_us();
        printf("%d x (1 async call + 1 wait) took %fs, %fus/call\n\n",
               nloop, (double)(te-ts)/1.e6, (double)(te-ts)/nloop);
	if (err != 0)
		printf("cummulated err=%d !? Something's wrong.\n", err);

	//----------------------
	printf("Test 3: submit %d calls and wait only for last result\n", nloop);
        ts = get_time_us();
        err = 0;
        for (int i=0; i<nloop; i++) {
          reqs[i] = veo_call_async(ctx, sym, argp);
        }
	err += veo_call_wait_result(ctx, reqs[nloop - 1], &res[nloop - 1]);
        te = get_time_us();
        printf("%d async calls + 1 wait took %fs, %fus/call\n\n",
               nloop, (double)(te-ts)/1.e6, (double)(te-ts)/nloop);
	if (err != 0)
		printf("cummulated err=%d !? Something's wrong.\n", err);

	//----------------------
	printf("Test 4: submit %d synchronous calls\n", nloop);
        ts = get_time_us();
        err = 0;
        for (int i=0; i<nloop; i++) {
		err += veo_call_sync(proc, sym, argp, &res[i]);
        }
        te = get_time_us();
        printf("%d sync calls took %fs, %fus/call\n\n",
               nloop, (double)(te-ts)/1.e6, (double)(te-ts)/nloop);
	if (err != 0)
		printf("cummulated err=%d !? Something's wrong.\n", err);

        err = veo_context_close(ctx);
        err = veo_proc_destroy(proc);
        return 0;
}
