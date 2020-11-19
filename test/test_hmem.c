#include <stdio.h>
#include <ve_offload.h>
#include <stdlib.h>

int
main()
{
	struct veo_proc_handle *proc = veo_proc_create(-1);
	printf("proc = %p\n", proc);
	struct veo_thr_ctxt    *ctx  = veo_context_open( proc );
	struct veo_args        *argp = veo_args_alloc();
	uint64_t handle = veo_load_library( proc, "./libvehello.so" );
	void *vebuf;
	int nelems = 1000;
	int ret = veo_alloc_hmem( proc, &vebuf, sizeof(int) * nelems );
	if (ret != 0) {
		fprintf(stderr, "veo_alloc_mem failed: %d", ret);
		exit(1);
	}

	ret = veo_args_set_hmem( argp, 0, vebuf );
	if (ret != 0) {
		fprintf(stderr, "veo_args_set_hmem failed: %d", ret);
		exit(1);
	}
	veo_args_set_i32( argp, 1, 1 );
	veo_args_set_i32( argp, 2, nelems );
	uint64_t id     = veo_call_async_by_name( ctx, handle, "init", argp );
	uint64_t rc;
	if (veo_call_wait_result( ctx, id, &rc ) != 0)
		exit(1);

	void *q = vebuf;
	if (veo_is_ve_addr(vebuf)) {
		q = malloc( sizeof(int)*nelems );
		// transfer data VE to VH
		if (veo_hmemcpy(q, vebuf, sizeof(int)*nelems)) {
			fprintf(stderr, "veo_hmemcpy failed: %d", ret);
			exit(1);
		}
	}
	// cast
	int *a = (int *)q;
	// check the transfered data.
	for (int i = 0; i < nelems; i++) {
		if (a[i] != 2) {
			printf("veo_hmemcpy() failed a[%d] = %d\n", i, a[i]);
			exit(1);
		}
	}

	// restore 
	for (int i = 0; i < nelems; i++)
		a[i] = 1;
	q = (void *)a;

	// transfer data VH to VE
	if (veo_is_ve_addr(vebuf)) {
		if (veo_hmemcpy(vebuf, q, sizeof(int)*nelems)) {
			fprintf(stderr, "veo_hmemcpy failed: %d", ret);
			exit(1);
		}
	}
	veo_args_clear( argp );

	ret = veo_args_set_hmem(argp, 0, vebuf );
	if (ret != 0) {
		fprintf(stderr, "veo_args_set_hmem failed: %d", ret);
		exit(1);
	}
	veo_args_set_i32( argp, 1, 1 );
	veo_args_set_i32( argp, 2, nelems );

	// check transferd data in check()
	id = veo_call_async_by_name( ctx, handle, "check", argp );
	if (veo_call_wait_result( ctx, id, &rc ) != 0)
		exit(1);

	printf( "rc:%lx (%s)\n", rc, rc ? "fail" : "success" );
	if (rc)
		exit(1);
	if (veo_free_hmem( vebuf ) != 0)
		exit(1);
	veo_context_close( ctx );
	veo_proc_destroy( proc );

	return 0;
}
