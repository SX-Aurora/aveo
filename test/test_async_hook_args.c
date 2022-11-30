#include <stdio.h>
#include <ve_offload.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdbool.h>

struct veo_thr_ctxt    *set_ctx;
uint64_t vebuf;
uint64_t arg_vebuf;
size_t set_size;
bool alloc_flag = false;
bool free_flag = false;

void alloc_hook(void *dummy, ...) //struct veo_thr_ctxt *ctx, uint64_t addr, size_t size)
{
	va_list args;
	va_start(args, dummy);
	struct veo_thr_ctxt *ctx = (struct veo_thr_ctxt *)va_arg(args, void *);
	uint64_t addr = va_arg(args, uint64_t); // is not HMEM
	size_t size = va_arg(args, size_t);
	va_end(args);

	printf("VH veo_alloc_mem_async hook: ctx=%p addr=%p size=%lx\n", ctx, addr, size);

	// check arguments
	if( set_ctx != ctx ) {
		printf("The hook argument(ctx) is unexpected, should be: %p\n", set_ctx);
		exit(1);
	}
	if ( veo_get_context_state(ctx) != VEO_STATE_RUNNING ) {
		printf("The hook argument(ctx) is unexpected, should be: %p\n", set_ctx);
		exit(1);
	}
	if ( veo_is_ve_addr((void *)addr) ) {
                printf("addr is HMEM\n");
                exit(1);
        }
	if ( set_size != size ) {
		printf("The hook argument(size) is unexpected, should be: %p\n", set_size);
		exit(1);
	}
	arg_vebuf = addr;
	alloc_flag = true;
}

void free_hook(void *dummy, ...) //struct veo_thr_ctxt *ctx, uint64_t addr)
{
	va_list args;
	va_start(args, dummy);
	struct veo_thr_ctxt *ctx = (struct veo_thr_ctxt *)va_arg(args, void *);
	uint64_t addr = va_arg(args, uint64_t);
	va_end(args);

	printf("VH veo_free_mem_async hook: ctx=%p addr=%p\n");
	if( set_ctx != ctx ) {
		printf("The hook argument is unexpected, should be: %p\n", set_ctx);
		exit(1);
	}
	if ( veo_get_context_state(ctx) != VEO_STATE_RUNNING ) {
		printf("The hook argument(ctx) is unexpected, should be: %p\n", set_ctx);
		exit(1);
	}
	if ( vebuf != addr ) {
		printf("The hook argument is unexpected, should be: %p\n", vebuf);
		exit(1);
	}
        if ( veo_is_ve_addr((void *)addr) ) {
                printf("addr is HMEM\n");
                exit(1);
        }
	free_flag = true;
}

int
main()
{
	struct veo_proc_handle *proc = veo_proc_create(-1);
	printf("proc = %p\n", proc);
	struct veo_thr_ctxt *ctx = veo_context_open(proc);
	printf("ctx = %p\n", ctx);
	set_ctx = ctx;
	struct veo_args        *argp = veo_args_alloc();
	uint64_t handle = veo_load_library(proc, "./libvealloc.so");
	int nelems = 1000;

	// register hooks
	void *dummy = 0;
	veo_register_hook((void *)&veo_alloc_mem_async, &alloc_hook, (void *)&dummy);
	veo_register_hook((void *)&veo_free_mem_async, &free_hook, (void *)&dummy);

	void (*test_hook)(void *, ...) = veo_get_hook((void *)&veo_alloc_mem_async);
	printf("Async alloc hook function pointer retrieved: %p\n", test_hook);
	if (test_hook != &alloc_hook) {
		printf("The hook value is unexpected, should be: %p\n", &alloc_hook);
		exit(1);
	}
	set_size = sizeof(int) * nelems;
	uint64_t id = veo_alloc_mem_async(ctx, sizeof(int) * nelems);
	if (id == VEO_REQUEST_ID_INVALID) {
		fprintf(stderr, "veo_alloc_mem_async failed");
		exit(1);
	}
	printf("req issued\n");
	if (veo_call_wait_result(ctx, id, &vebuf) != 0)
		exit(1);
	printf("veo_alloc_mem_async returned addr=%p\n", vebuf);

	// check arguments
	if (vebuf != arg_vebuf) {
		printf("The alloc_hook argument(addr) is unexpected, should be: %p\n", vebuf);
                exit(1);
        }

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

	if (alloc_flag && free_flag)
		return 0;
	return 1;
}
