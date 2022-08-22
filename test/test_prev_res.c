#include <stdio.h>
#include <ve_offload.h>
#include <stdlib.h>

int
main()
{
	struct veo_proc_handle *proc = veo_proc_create(-1);
	struct veo_thr_ctxt    *ctx  = veo_context_open(proc);
	struct veo_args        *argp = veo_args_alloc();
	uint64_t handle = veo_load_library(proc, "./libvealloc.so");
	uint64_t vebuf;
	int nelems = 1000;

	int ret = veo_alloc_mem(proc, &vebuf, sizeof(int) * nelems);
	if (ret != 0) {
		fprintf(stderr, "veo_alloc_mem failed: %d", ret);
		exit(1);
	}
        printf("veo_alloc_mem() has allocated a buffer at %p\n", vebuf);

        // since we're using veo_call_async_by_name() the first call to it hides
        // an implicit veo_get_sym() call! So we need veo_prev_req_result offs=1.
        veo_args_set_i32(argp, 0, 1);
	uint64_t id = veo_call_async_by_name(ctx, handle, "prevres", argp);
	uint64_t rc;
	if (veo_call_wait_result(ctx, id, &rc) != 0)
		exit(1);
	if (rc != vebuf) {
		fprintf(stderr, "veo_prev_req_result() returned the wrong value!\n"
                        "%p instead of %p\n", rc, vebuf);
		exit(1);
	}
	veo_context_close( ctx );
	veo_proc_destroy( proc );

	return 0;
}
