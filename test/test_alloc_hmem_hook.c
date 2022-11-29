#include <stdio.h>
#include <ve_offload.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdbool.h>

struct veo_proc_handle *set_proc;
struct veo_thr_ctxt    *ctx;
void *vebuf;
uint64_t arg_vebuf;
size_t set_size;
bool alloc_flag = false;
bool free_flag = false;

void alloc_hook(void *dummy, ...) //struct veo_proc_handle *proc, uint64_t *addr, size_t size)
{
        va_list args;
        va_start(args, dummy);
        struct veo_proc_handle *proc = (struct veo_proc_handle *)va_arg(args, void *);
        uint64_t *addr = va_arg(args, uint64_t *); // is HMEM
        size_t size = va_arg(args, size_t);
        va_end(args);

        // check arguments
        if( set_proc != proc ) {
                printf("The alloc_hook argument(proc) is unexpected, should be: %p\n", set_proc);
                exit(1);
        }
        if ( veo_proc_identifier(proc) != 0 ) {
                printf("The alloc_hook argument(proc) is unexpected, should be: %p\n", set_proc);
                exit(1);
        }
        if ( !veo_is_ve_addr(addr) ) {
                printf("addr is HMEM\n");
                exit(1);
        }
        if ( set_size != size ) {
                printf("The alloc_hook argument(size) is unexpected, should be: %p\n", set_size);
                exit(1);
        }
        arg_vebuf = (uint64_t)addr;
        alloc_flag = true;
}

void free_hook(void *dummy, ...) //struct veo_proc_handle *proc, uint64_t addr
{
        va_list args;
        va_start(args, dummy);
        struct veo_proc_handle *proc = (struct veo_proc_handle *)va_arg(args, void *);
        uint64_t addr = va_arg(args, uint64_t); // is HMEM
        va_end(args);

        printf("VH veo_free_hmem hook: proc=%p addr=%p\n");

        // check arguments
        if( set_proc != proc ) {
                printf("The free_hook argument(proc) is unexpected, should be: %p\n", set_proc);
                exit(1);
        }
        if ( veo_proc_identifier(proc) != 0 ) {
                printf("The free_hook argument(proc) is unexpected, should be: %p\n", set_proc);
                exit(1);
        }
        if ( (uint64_t)vebuf != addr ) {
                printf("The free_hook argument(addr) is unexpected, should be: %p\n", vebuf);
                exit(1);
        }
        if ( !veo_is_ve_addr((void *)addr) ) {
                printf("addr is not HMEM\n");
                exit(1);
        }
        free_flag = true;
}

int
main(void)
{
	struct veo_proc_handle *proc = veo_proc_create(-1);
	printf("proc = %p\n", proc);
	set_proc = proc;
	ctx  = veo_context_open(proc);
	struct veo_args        *argp = veo_args_alloc();
	uint64_t handle = veo_load_library(proc, "./libvealloc.so");
	int nelems = 1000;

	// register hook
	void *dummy = 0;

	veo_register_hook((void *)&veo_alloc_hmem, &alloc_hook, (void *)&dummy);
	veo_register_hook((void *)&veo_free_hmem, &free_hook, (void *)&dummy);

	set_size = sizeof(int) * nelems;
	int ret = veo_alloc_hmem(proc, &vebuf, sizeof(int) * nelems);
	if (ret != 0) {
		fprintf(stderr, "veo_alloc_mem failed: %d", ret);
		exit(1);
	}

	// check arguments
	if ( (uint64_t)vebuf != arg_vebuf ) {
                printf("The alloc hook argument(addr) is unexpected, should be: %p\n", arg_vebuf);
                exit(1);
        }

	ret = veo_args_set_hmem(argp, 0, vebuf);
	if (ret != 0) {
		fprintf(stderr, "veo_args_set_hmem failed: %d", ret);
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

	ret = veo_args_set_hmem(argp, 0, vebuf);
	if (ret != 0) {
		fprintf(stderr, "veo_args_set_hmem failed: %d", ret);
		exit(1);
	}
	veo_args_set_i32(argp, 1, nelems);
	id = veo_call_async_by_name(ctx, handle, "check", argp);
	if (veo_call_wait_result(ctx, id, &rc) != 0)
		exit(1);

	printf("rc:%lx (%s)\n", rc, rc ? "fail" : "success");
	if (rc)
		exit(1);


	if (veo_free_hmem(vebuf) != 0) {
		fprintf(stderr, "The return value of veo_free_mem() is not expected\n", rc);
		exit(1);
	}

	veo_context_close( ctx );
	veo_proc_destroy( proc );

        if (alloc_flag && free_flag)
                return 0;
        return 1;
}
