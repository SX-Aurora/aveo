#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>

#include "ve_offload.h"
#include "veo_time.h"

int main(int argc, char *argv[])
{
	int err = 0;

        struct veo_proc_handle *proc;

        proc = veo_proc_create(-1);
        printf("proc = %p\n", (void *)proc);
        if (proc == NULL)
		return -1;
	//sleep(40);
        uint64_t libh = veo_load_library(proc, "./libvehello.so");
        printf("libh = %p\n", (void *)libh);
        if (libh == 0)
		return -1;

        uint64_t sym = veo_get_sym(proc, libh, "hello");
        printf("'hello' sym = %p\n", (void *)sym);
        if (sym == 0)
		return -1;

        struct veo_args *argp = veo_args_alloc();

        veo_args_set_i32(argp, 0, 42);

        uint64_t result = 0;
        int rc = veo_call_sync(proc, sym, argp, &result);
        printf("call 'hello' returned %ld, rc=%d\n", result, rc);

        sym = veo_get_sym(proc, libh, "print_buffer");
        printf("'print_buffer' sym = %p\n", (void *)sym);

        veo_args_clear(argp);

        rc = veo_call_sync(proc, sym, argp, &result);
        printf("call 'print_buffer' returned %ld, rc=%d\n", result, rc);

        sym = veo_get_sym(proc, libh, "empty");
        printf("'empty' sym = %p\n", (void *)sym);

        veo_args_clear(argp);

        long ts, te;
        int nloop = 100000;
        ts = get_time_us();
        for (int i=0; i<nloop; i++) {
          rc = veo_call_sync(proc, sym, argp, &result);
        }
        te = get_time_us();
        printf("%d sync calls took %fs, %f us/call\n",
               nloop, (double)(te-ts)/1.e6, (double)(te-ts)/nloop);

        veo_args_free(argp);

        err = veo_proc_destroy(proc);
        printf("veo_proc_destroy() returned %d\n", err);

        return 0;
}

