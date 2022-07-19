#include <stdio.h>
#include <ve_offload.h>
#include <stdlib.h>
#include <stdarg.h>

void alloc_hook(void *dummy, ...) //struct veo_proc_handle *proc, uint64_t addr, size_t size)
{
	va_list args;
	va_start(args, dummy);
	struct veo_proc_handle *proc = (struct veo_proc_handle *)va_arg(args, void *);
	uint64_t addr = va_arg(args, uint64_t);
	size_t size = va_arg(args, size_t);
	va_end(args);
	
	printf("VH veo_alloc_mem hook: proc=%p addr=%p size=%ld\n");
}

void free_hook(void *dummy, ...) //struct veo_proc_handle *proc, uint64_t addr)
{
	va_list args;
	va_start(args, dummy);
	struct veo_proc_handle *proc = (struct veo_proc_handle *)va_arg(args, void *);
	uint64_t addr = va_arg(args, uint64_t);
	va_end(args);

	printf("VH veo_free_mem hook: proc=%p addr=%p\n");
}

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
	veo_register_hook((void *)&veo_alloc_mem, &alloc_hook, (void *)&dummy);
	veo_register_hook((void *)&veo_free_mem, &free_hook, (void *)&dummy);
	
	int ret = veo_alloc_mem(proc, &vebuf, sizeof(int) * nelems);
	if (ret != 0) {
		fprintf(stderr, "veo_alloc_mem failed: %d", ret);
		exit(1);
	}

	ret = veo_args_set_u64(argp, 0, vebuf);
	if (ret != 0) {
		fprintf(stderr, "veo_args_set_u64 failed: %d", ret);
		exit(1);
	}
	veo_args_set_i32( argp, 1, nelems );
	uint64_t id = veo_call_async_by_name(ctx, handle, "init", argp);
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
	if (veo_free_mem(proc, vebuf) != 0) {
		fprintf(stderr, "The return value of veo_free_mem() is not expected\n", rc);
		exit(1);
	}

	veo_context_close( ctx );
	veo_proc_destroy( proc );

	return 0;
}
