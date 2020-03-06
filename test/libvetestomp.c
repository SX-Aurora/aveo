// /opt/nec/ve/bin/ncc -fopenmp -fpic -shared -o libvetestomp.so libvetestomp.c
//

#include <stdio.h>
#include <omp.h>
#include <unistd.h>
#include <sys/syscall.h>

long syscall(long n, ...);

int gettid(void)
{
  return syscall(SYS_gettid);
}

void omp_loop(void)
{
  int tid, i;
#pragma omp parallel private(tid)
  {
    tid = gettid();
    i = omp_get_thread_num();
    printf("omp thread %d has tid=%d\n", i, tid);
  }
}
