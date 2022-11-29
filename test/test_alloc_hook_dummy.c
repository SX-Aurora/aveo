#include <stdio.h>
#include <ve_offload.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdbool.h>

struct veo_proc_handle *proc;
struct veo_thr_ctxt    *ctx;
uint64_t vebuf = 0;
uint64_t arg_vebuf;
size_t set_size;
bool alloc_flag = false;
bool free_flag = false;

void necmpi_hmem_notify_alloc_dummy(void *addr, size_t size)
{
	printf("VH veo_alloc_mem hook: proc=%p vebuf=%p addr=%p size=%ld\n", proc, vebuf, addr, size);

        // check arguments
        if ( !veo_is_ve_addr(addr) ) {
                printf("addr is not HMEM\n");
                exit(1);
        }
        if ( set_size != size ) {
                printf("The hook argument(size) is unexpected, should be: %p\n", set_size);
                exit(1);
        }
        arg_vebuf = (uint64_t)addr;
        alloc_flag = true;
}

void necmpi_hmem_notify_free_dummy(uint64_t addr)
{
	printf("VH veo_free_mem hook: proc=%p vebuf=%p addr=%p\n", proc, vebuf, addr);

        // check arguments
        if ( !veo_is_ve_addr((void *)addr) ) {
                printf("addr is not HMEM\n");
                exit(1);
        }
        free_flag = true;
}

int
main(void)
{
	proc = veo_proc_create(-1);
	printf("proc = %p\n", proc);
	ctx  = veo_context_open(proc);
	struct veo_args        *argp = veo_args_alloc();
	uint64_t handle = veo_load_library(proc, "./libvealloc.so");
	int nelems = 1000;

	// register hook
	void *dummy = 0;

	veo_register_hmem_hook_functions(&necmpi_hmem_notify_alloc_dummy, &necmpi_hmem_notify_free_dummy);

	set_size = sizeof(int) * nelems;
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

	// unregister hooks
	veo_unregister_hmem_hook_functions();
	void (*test_alloc_hook)(void *, ...) = veo_get_hook((void *)&veo_alloc_mem);
	if (test_alloc_hook != NULL) {
		printf("The hook value is unexpected, should be: NULL\n");
	}
	void (*test_free_hook)(void *, ...) = veo_get_hook((void *)&veo_free_mem);
	if (test_free_hook != NULL) {
		printf("The hook value is unexpected, should be: NULL\n");
	}
	void (*test_alloc_hook_async)(void *, ...) = veo_get_hook((void *)&veo_alloc_mem_async);
	if (test_alloc_hook_async != NULL) {
		printf("The hook value is unexpected, should be: NULL\n");
	}
	void (*test_free_hook_async)(void *, ...) = veo_get_hook((void *)&veo_free_mem_async);
	if (test_free_hook_async != NULL) {
		printf("The hook value is unexpected, should be: NULL\n");
	}
	
	veo_context_close( ctx );
	veo_proc_destroy( proc );

	return 0;
}
