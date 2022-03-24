#define _GNU_SOURCE

#include <stdio.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <sys/types.h>
//#include <bits/shm.h>
#include "ve_offload.h"
//#include "veo_veshm.h"
#include "veo_vhveshm.h"
#include "veo_vedma.h"

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
/**
 * cd aveo
 * make install
 * cd test
 * gcc -std=c11 -pthread -O2 -g -I/opt/nec/ve/veos/include -I`pwd`/../src -L/opt/nec/ve/veos/lib64 -Wl,-rpath=/opt/nec/ve/veos/lib64 -o `pwd`/mkshm_dma `pwd`/mkshm_dma.c -lveo
 * ./mkshm_dma
 * gcc -o rdshm rdshm.c
 * ./rdshm
 * ipcrm -M 0x19761215
 */
int main(int argc, char* argv[])
{
	int key = 0x19761215;
	size_t size = 64 * 1024 * 1024;
	size_t align = 64 * 1024 * 1024;

	// make shm area
	int shmid = shmget(key, size, IPC_CREAT | IPC_EXCL | SHM_HUGETLB | 0600);
	if (shmid < 0) {
 		perror("shmget");
		return 1;
	}
	void* p = shmat(shmid, NULL, 0);
	if (p == (void*)-1) {
		perror("shmat");
		return 1;
	}
	// write initial value
	*(int*)p = 1215;
	fprintf(stderr, "Created shm with key=%x and wrote initial value=%d\n",
            	key, *(int*)p);
	shmdt(p);

        struct veo_proc_handle *proc1;
        proc1 = veo_proc_create(-1);
        //printf("proc1 = %p \n", (void *)proc1);
        if (proc1 == NULL)
                return -1;

	int vh_shmid = veo_shmget(proc1, key, size, SHM_HUGETLB);
        if (shmid == -1) {
                perror("veo_shmget");
                return -1;
        }
	fprintf(stderr, "vh_shmid=%d\n", vh_shmid);
        void *vehva_vh;
        //printf("vehva_vh = %lx\n", vehva_vh);
        void *p_veo = veo_shmat(proc1, shmid, NULL, 0, &vehva_vh);
        printf("vehva_vh = %lx, veo_shmat returns %lx\n", vehva_vh, p_veo);
        if (p_veo == (void*)-1) {
                perror("veo_shmat");
                return -1;
        }

	void *vemva3;
	int ret = veo_posix_memalign_hmem(proc1, &vemva3, size, size);
        printf("vemva = %lx\n", vemva3);

	uint64_t vehva_ve = veo_register_mem_to_dmaatb(vemva3, size);
	printf("vehva_ve = %lx\n", vehva_ve);
	if (vehva_ve == (uint64_t)-1) {
		perror("veo_register_mem_to_dmaatb");
		return -1;
	}

	size_t transfer_size = 64 * 1024 * 1024;
	// read (Sys V -> VE)
	ret = veo_dma_post_wait(proc1, (uint64_t)veo_get_hmem_addr((void *)vehva_ve), (uint64_t)vehva_vh, transfer_size);
	fprintf(stderr, "ve_dma_post_wait(read): ret=%d\n", ret);

	// check read data
	uint64_t handle = veo_load_library(proc1, "../build/bin/libvehello.so");
	uint64_t print_sym = veo_get_sym(proc1, handle, "print_ui");
        struct veo_args *argp1 = veo_args_alloc();
	uint64_t result;
        veo_args_set_hmem(argp1, 0, veo_get_hmem_addr(vemva3));
	// expected print 1215
	veo_call_sync(proc1, print_sym, argp1, &result);

	uint64_t buffer = 0UL;
	// read vemva3.
	// The value pointed to by the address vemva3 was incremented with the print_ui function.
	ret = veo_read_mem(proc1, &buffer, (uint64_t)veo_get_hmem_addr(vemva3), sizeof(uint64_t));
	// expected print 1216.
	printf("buffer = %u, ret = %d\n", buffer, ret);
	
	// write (VE -> Sys V)
	buffer = 0UL;
	ret = veo_dma_post_wait(proc1, (uint64_t)vehva_vh, (uint64_t)veo_get_hmem_addr((void *)vehva_ve), transfer_size);
	fprintf(stderr, "ve_dma_post_wait(write): ret=%d\n", ret);

	ret = veo_shmdt(proc1, p_veo);
	fprintf(stderr, "vh_shmdt: ret=%d\n", ret);
	ret = veo_unregister_mem_from_dmaatb(vehva_ve);
	fprintf(stderr, "ve_unregister_mem_from_dmaatb: ret=%d\n", ret);
    return 0;
}
