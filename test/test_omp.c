//
// gcc -std=gnu99 -o test_omp test_omp.c -I/opt/nec/ve/veos/include -pthread -L/opt/nec/ve/veos/lib64 -Wl,-rpath=/opt/nec/ve/veos/lib64 -lveo
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ve_offload.h>

int main()
{
	int ret;
	struct veo_proc_handle *proc = veo_proc_create(0);
	if (proc == NULL) {
		printf("veo_proc_create() failed!\n");
		exit(1);
	}
	uint64_t handle = veo_load_library(proc, "./libvetestomp.so");
	printf("handle = %p\n", (void *)handle);
	uint64_t sym = veo_get_sym(proc, handle, "omp_loop");
	printf("symbol address = %p\n", (void *)sym);
	
	struct veo_thr_ctxt *ctx = veo_context_open(proc);
	printf("VEO context = %p\n", ctx);

	struct veo_args *arg = veo_args_alloc();
	veo_args_set_i64(arg, 0, 4);
	
	uint64_t req = veo_call_async(ctx, sym, arg);
	long retval;
	ret = veo_call_wait_result(ctx, req, &retval),

	veo_args_free(arg);

	veo_context_close(ctx);
	//veo_proc_destroy(proc);
	return 0;
}

