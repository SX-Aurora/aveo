#include <stdio.h>
#include <stdint.h>
#include <sys/syscall.h>
#include <pthread.h>

int64_t buffer = 0xdeadbeefdeadbeef;

int gettid(void)
{
  return syscall(SYS_gettid);
}

void *ve_child(void *arg)
{
  printf ("[ve_child]VE:PID = %d, tid = %d\n", getpid(), gettid());
  return 0;
}

uint64_t hello(int i)
{
  pthread_t th;
  void *retval;
  pthread_create (&th, NULL, ve_child, NULL);
  pthread_join (th, &retval);

  if( i != 42 ) {
    printf("VE:an argment is incorrect.\n");
    return 0;
  }
  printf("Hello, %d\n", i);
  printf("[hello]VE:PID:%d TID:%d\n", getpid(), gettid());
  fflush(stdout);
  return i + 1;
}

uint64_t print_buffer()
{
  printf("0x%016lx\n", buffer);
  fflush(stdout);
  return 1;
}
