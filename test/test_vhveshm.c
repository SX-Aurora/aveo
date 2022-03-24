/*
-std=c11 -pthread -O2 -g -I/opt/nec/ve/veos/include -I`pwd`/src \
 *  -L/opt/nec/ve/veos/lib64 -Wl,-rpath=/opt/nec/ve/veos/lib64 -o `pwd`/build/bin/test_vhveshm \
 *   *  `pwd`/test/test_vhveshm.c -lveo
 *    */

#define _GNU_SOURCE
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/stat.h>

#include "ve_offload.h"
#include "veo_vhveshm.h"

#define PG_SIZE (64*1024*1024)

int main(int argc, char *argv[])
{
        int key = 0x19761215;
        size_t size = PG_SIZE;

        struct veo_proc_handle *proc1;
        proc1 = veo_proc_create(-1);
        printf("proc1 = %p ", (void *)proc1);
        if (proc1 == NULL)
                return -1;

        int shmid = shmget(key, size, IPC_CREAT | IPC_EXCL | SHM_HUGETLB | 0600);
        if (shmid < 0) {
                perror("shmget");
                return 1;
        }
        printf("shmid = %d\n", shmid);
        void *p = shmat(shmid, NULL, 0);
        if (p == (void*)-1) {
                perror("shmat");
                return 1;
        }
        printf("p = %lx\n", p);
        *(int*)p = 1215;
        fprintf(stderr, "Created shm with key=%x and wrote initial value=%d\n",
            key, *(int*)p);
        shmdt(p);

        shmid = veo_shmget(proc1, key, size, SHM_HUGETLB);
        if (shmid == -1) {
                printf("veo_shmget returns -1\n");
                return -1;
        }
        printf("shmid = %d\n", shmid);
        void *vehva_vh;
        void *p_veo = veo_shmat(proc1, shmid, NULL, 0, &vehva_vh);
        printf("veo_shmat returns %lx\n", (int *)p_veo);
        if (p_veo == (void*)-1) {
                perror("vh_shmat");
                return 1;
        }
        fprintf(stderr, "vehva_vh=%p\n", vehva_vh);

        int ret = veo_shmdt(proc1, p_veo);
        if (ret == -1) {
                printf("veo_shmdt returns -1\n");
                return -1;
        }

        if (shmctl(shmid, IPC_RMID, NULL) == -1) {
                perror("shmctl");
                return -1;
        }

        veo_proc_destroy(proc1);
        return 0;
}
