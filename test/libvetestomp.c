// /opt/nec/ve/bin/ncc -fopenmp -fpic -shared -o libvetestomp.so libvetestomp.c
//

#include <sched.h>
#include <stdio.h>
#include <omp.h>
#include <unistd.h>
#include <sys/syscall.h>

long syscall(long n, ...);

int gettid(void)
{
  return syscall(SYS_gettid);
}

int flag = 0;
//extern void __vthr$_init(void);

int omp_loop(void)
{
  int tid, i, nthreads = 0;
  //if (flag == 1) {
  //  __vthr$_init();
  //}
#pragma omp parallel private(tid, i)
  {
    tid = gettid();
    i = omp_get_thread_num();
    if (i == 0)
      nthreads = omp_get_num_threads();
    printf("omp thread %d has tid=%d on core=%d\n", i, tid, sched_getcpu());
  }
  flag = 1;
  return nthreads;
}
