#include <stdio.h>
#include <stdlib.h>
#include <ve_offload.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/syscall.h>
 
#define SIZE (10*1024*1024)
 
long syscall(long n, ...);
 
int rounds = 10;

int gettid(void)
{
  return syscall(SYS_gettid);
}
 
void *child(void *proc)
{
  int trounds = rounds;
  printf("test PID = %d, tid = %d\n", getpid(), gettid());
  uint64_t bufptr;
  uint64_t *buffer = (uint64_t *)malloc(SIZE * sizeof(uint64_t));
  for(int i = 0;i < SIZE;i++)
    buffer[i] = i;
 
  while (trounds-- > 0) {
    int ret = veo_alloc_mem(proc, &bufptr, SIZE * sizeof(uint64_t));
    if (ret != 0) {
      printf("veo_alloc_mem() failed!\n");
      exit(1);
    }
    ret = veo_write_mem(proc, bufptr, buffer, SIZE * sizeof(uint64_t));
    if(ret != 0) {
      printf("veo_write_mem() failed!\n");
      exit(1);
    }
    ret = veo_read_mem(proc, buffer, bufptr, SIZE * sizeof(uint64_t));
    if(ret != 0) {
      printf("veo_read_mem() failed!\n");
      exit(1);
    }
    int free_mem = veo_free_mem(proc, bufptr);
    if ( free_mem != 0) {
      printf("veo_free_mem() failed!\n");
      exit(1);
    }
  }
  printf("TID %ld done\n", gettid());
  free(buffer);
}
 
 
int
main(int argc, char *argv[])
{
  if (argc > 1) {
    rounds = atoi(argv[1]);
  }
  printf("running for %d rounds\n", rounds);
  printf("main PID = %d, tid = %d\n", getpid (), gettid ());
  pthread_t th1, th2, th3, th4;
  struct veo_proc_handle *proc = veo_proc_create(-1);
  pthread_create(&th1, NULL, child, proc);
  pthread_create(&th2, NULL, child, proc);
  pthread_create(&th3, NULL, child, proc);
  pthread_create(&th4, NULL, child, proc);
  pthread_join(th1, NULL);//(void **)&proc);
  pthread_join(th2, NULL);//(void **)&proc);
  pthread_join(th3, NULL);//(void **)&proc);
  pthread_join(th4, NULL);//(void **)&proc);
  printf("end\n");
  veo_proc_destroy(proc);
 
  return 0;
}
