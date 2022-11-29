#include <stdio.h>
#include <ve_offload.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdbool.h>

struct veo_proc_handle *proc;
struct veo_thr_ctxt    *ctx;
uint64_t arg_vebuf;
uint64_t vebuf = 0;
size_t set_size;
bool alloc_flag = false;
bool free_flag = false;

void necmpi_hmem_notify_alloc_dummy(void *addr, size_t size)
{
        printf("VH veo_alloc_async_mem hook: proc=%p vebuf=%p addr=%p size=%ld\n", proc, vebuf, addr, size);
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
        printf("VH veo_free_mem_async hook: proc=%p vebuf=%p addr=%p\n", proc, vebuf, addr);
	// check arguments
        if ( !veo_is_ve_addr((void *)addr) ) {
                printf("addr is not HMEM\n");
                exit(1);
        }
	uint64_t addr_without_hmem = (uint64_t)veo_get_hmem_addr((void *)addr);
        if ( addr_without_hmem != vebuf ) {
                printf("The free hook argument(addr) is unexpected, should be: %p\n", addr);
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

	void (*test_hook)(void *, ...) = veo_get_hook((void *)&veo_alloc_mem_async);
	printf("Async alloc hook function pointer retrieved: %p\n", test_hook);
	if (test_hook == NULL) {
		printf("The hook value is unexpected\n");
		exit(1);
	}

	set_size = sizeof(int) * nelems;
        uint64_t id = veo_alloc_mem_async(ctx, sizeof(int) * nelems);
        if (id == VEO_REQUEST_ID_INVALID) {
                fprintf(stderr, "veo_alloc_mem_async failed");
                exit(1);
        }

        if (veo_call_wait_result(ctx, id, &vebuf) != 0)
                exit(1);

        printf("veo_alloc_mem_async returned addr=%p\n", vebuf);

        // check arguments
	// veo_alloc_async_mem() returns VEMVA, but hook function retrieves HMEM.
	uint64_t arg_vebuf_without_hmem = (uint64_t)veo_get_hmem_addr((void *)arg_vebuf);
        if ( arg_vebuf_without_hmem != vebuf ) {
                printf("The alloc hook argument(addr) is unexpected, should be: %p\n", arg_vebuf);
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
