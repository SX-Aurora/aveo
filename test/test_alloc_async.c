#include <stdio.h>
#include <ve_offload.h>
#include <stdlib.h>
#include <stdarg.h>

int
main()
{
	struct veo_proc_handle *proc = veo_proc_create(-1);
	printf("proc = %p\n", proc);
	struct veo_thr_ctxt    *ctx  = veo_context_open(proc);
	struct veo_args        *argp = veo_args_alloc();
	uint64_t handle = veo_load_library(proc, "./libvealloc.so");
	uint64_t vebuf;
	int nelems = 1000;

	// register hook
	void *dummy = 0;
	
	uint64_t id = veo_alloc_mem_async(ctx, sizeof(int) * nelems);
	if (id == VEO_REQUEST_ID_INVALID) {
		fprintf(stderr, "veo_alloc_mem_async failed");
		exit(1);
	}

	if (veo_call_wait_result(ctx, id, &vebuf) != 0)
		exit(1);

	printf("veo_alloc_mem_async returned addr=%p\n", vebuf);

	int ret = veo_args_set_u64(argp, 0, vebuf);
	if (ret != 0) {
		fprintf(stderr, "veo_args_set_u64 failed: %d", ret);
		exit(1);
	}
	veo_args_set_i32( argp, 1, nelems );
	id = veo_call_async_by_name(ctx, handle, "init", argp);
	uint64_t rc;
	if (veo_call_wait_result(ctx, id, &rc) != 0)
		exit(1);
	if (rc == 0) {
		fprintf(stderr, "The return value of init() is not expected : %d\n", rc);
		exit(1);
	}

	veo_args_clear( argp );

	ret = veo_args_set_u64(argp, 0, vebuf);
	if (ret != 0) {
		fprintf(stderr, "veo_args_set_u64 failed: %d", ret);
		exit(1);
	}
	veo_args_set_i32(argp, 1, nelems);
	id = veo_call_async_by_name(ctx, handle, "check", argp);
	if (veo_call_wait_result(ctx, id, &rc) != 0)
		exit(1);

	printf("rc:%lx (%s)\n", rc, rc ? "fail" : "success");
	if (rc)
		exit(1);

	id = veo_free_mem_async(ctx, vebuf);
	if (id == VEO_REQUEST_ID_INVALID) {
		fprintf(stderr, "veo_free_mem_async failed");
		exit(1);
	}
	if (veo_call_wait_result(ctx, id, &rc) != 0)
		exit(1);

	veo_context_close( ctx );
	veo_proc_destroy( proc );

	return 0;
}
