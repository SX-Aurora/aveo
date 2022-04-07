/**
 * gcc -std=c11 -pthread -O2 -g -I`pwd`/build/include -I`pwd`/src -I/opt/nec/ve/include -L`pwd`/build/lib64 -Wl,-rpath=`pwd`/build/lib64 -o `pwd`/build/bin/test_veshm `pwd`/test/test_veshm.c -lveo
 *
 */

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>

#include "ve_offload.h"
#include "veo_veshm.h"

//#include "veo_hmem_macros.h"

#define PG_SIZE (64*1024*1024)

int main(int argc, char *argv[])
{
	int err = 0;
	char buffer1[14];
	size_t size = PG_SIZE;
	void *vebuf;
	pid_t pid;
	void *vebuf2;
	char hello[] = "HELLOVHWORLD!";
	uint64_t ret_cpy;

        struct veo_proc_handle *proc1;
	struct veo_proc_handle *proc2;

	// Create 2 VE processes
        proc1 = veo_proc_create(-1);
        printf("proc1 = %p ", (void *)proc1);
        if (proc1 == NULL)
		return -1;
        proc2 = veo_proc_create(-1);
        printf("proc2 = %p are created.\n", (void *)proc2);
        if (proc2 == NULL)
		return -1;

/*
	// Allocate proc1 memory to open shared memory
	int ret = veo_alloc_hmem(proc1, &vebuf, size);
	printf("Allocate proc1 memory to open shared memory. ret = %d\n", ret);
	// Data alignment
	int ident = veo_get_proc_identifier_from_hmem(vebuf);
	printf("ident = %d\n", ident);
	printf("vebuf = %lx\n", vebuf);
	void *vemva0 = veo_get_hmem_addr(vebuf);
	printf("vemva0 = %lx\n", vemva0);
	void *vemva1 = (void *) ((uint64_t)vemva0 + (size-1) & ~(size-1));
	printf("vemva1 = %lx\n", vemva1);
	void *vemva2 = (void *)SET_VE_FLAG((uint64_t)vemva1);
	printf("vemva2 = %lx\n", vemva2);
	void *vemva3 = (void *)SET_PROC_IDENT((uint64_t)vemva2, (uint64_t)ident);
	printf("vemva3 = %lx\n", vemva3);
*/
	void *vemva3;
        int ret = veo_posix_memalign_hmem(proc1, &vemva3, size, size);
        printf("vemva = %lx\n", vemva3);
	// Open shared memory
	ret = veo_shared_mem_open(vemva3, size, 0, VE_REGISTER_NONE);
	printf("open ret = %d\n", ret);

	// Attach
	pid = veo_get_pid_from_hmem(vemva3);
	void *attach_addr = veo_shared_mem_attach(proc2, pid, vemva3, size, 0, VE_REGISTER_NONE|VE_REGISTER_VEMVA);
	printf("attach_addr = %lx\n", (uint64_t)attach_addr);

	// Copy data from VH to VE(proc2)
	uint64_t handle2 = veo_load_library(proc2, "./libvehello.so");
	printf("VH: copy data that is string HELLOVHWORLD! from VH to VE(proc2)\n");
	veo_alloc_hmem(proc2, &vebuf2, 14);
	void *vebuf2_sub = veo_get_hmem_addr(vebuf2);
	veo_write_mem(proc2, (uint64_t)vebuf2_sub, hello, sizeof(hello));

	// Copy data from VE(proc2) to shared memory
	ret = 0;
	ret = veo_hmemcpy(attach_addr, vebuf2, sizeof(hello));
	printf("VH: call memcpy on VE side (offloading to proc2).\n" \
		"copy data from VE proc2 memory to shared memory\n");

	// Check transfered data
	uint64_t handle1 = veo_load_library(proc1, "./libvehello.so");
	uint64_t print_sym = veo_get_sym(proc1, handle1, "print_mem");
	struct veo_args *argp1 = veo_args_alloc();
	veo_args_set_u64(argp1, 0, (uint64_t)vemva3);
	printf("VH: execute print data(vemva1 %lx) function on VE proc1\n", vemva3);
	veo_call_sync(proc1, print_sym, argp1, &ret_cpy);

	// Detach
	ret = 0;
	printf("detach vemva1(%lx)\n", attach_addr);
	//ret = veo_shared_mem_detach(proc2, vemva1, VE_REGISTER_NONE|VE_REGISTER_VEMVA);
	ret = veo_shared_mem_detach(proc2, attach_addr, VE_REGISTER_VEMVA);
	printf("detach ret = %d\n", ret);

	// Close
	ret = veo_shared_mem_close(vemva3, size, 0, VE_REGISTER_NONE);
	printf("close ret = %d\n", ret);

        err = veo_proc_destroy(proc1);
        err = veo_proc_destroy(proc2);
        return 0;
}

