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
        //
        // set VE_OMP_NUM_THREADS to 4 such that we can accomodate 2 contexts
        //
        setenv("VE_OMP_NUM_THREADS", "4", 1);
	struct veo_proc_handle *proc = veo_proc_create(-1);
	if (proc == NULL) {
		printf("veo_proc_create() failed!\n");
		exit(1);
	}
	uint64_t handle = veo_load_library(proc, "./libvetestomp.so");
	printf("handle = %p\n", (void *)handle);
	uint64_t sym = veo_get_sym(proc, handle, "omp_loop");
	printf("symbol address = %p\n", (void *)sym);
	
	struct veo_thr_ctxt *ctx1 = veo_context_open(proc);

	struct veo_args *arg = veo_args_alloc();
	veo_args_set_i64(arg, 0, 4);
	
	uint64_t req = veo_call_async(ctx1, sym, arg);
	long retval;
	ret = veo_call_wait_result(ctx1, req, &retval),
	printf("ctx1 ret %d nthread %d\n", ret, retval);
	if (ret != 0 || retval < 2) {
		printf("Test failed\n");
		exit(1);
	}

	struct veo_thr_ctxt *ctx2 = veo_context_open(proc);
        req = veo_call_async(ctx2, sym, arg);
	ret = veo_call_wait_result(ctx2, req, &retval),
	printf("ctx2 ret %d nthread %d\n", ret, retval);
	if (ret != 0 || retval < 2) {
		printf("Test failed\n");
		exit(1);
	}

        req = veo_call_async(ctx1, sym, arg);
	ret = veo_call_wait_result(ctx1, req, &retval),
	printf("2nd call ctx1 ret %d nthread %d\n", ret, retval);
	if (ret != 0 || retval < 2) {
		printf("Test failed\n");
		exit(1);
	}
        
	veo_args_free(arg);

	veo_context_close(ctx2);
	veo_context_close(ctx1);
	return 0;
}

