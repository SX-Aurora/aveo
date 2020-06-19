#include <stdio.h>
#include <stdlib.h>
#include <ve_offload.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/syscall.h>

long syscall(long n, ...);

int gettid(void)
{
  return syscall(SYS_gettid);
}

void *child(void *arg)
{
  printf("PID = %d, tid = %d\n", getpid(), gettid());
  struct veo_proc_handle *proc = veo_proc_create(-1);
  if (proc == NULL)
  {
    perror ("veo_proc_create");
    exit(1);
  }
  uint64_t handle = veo_load_library(proc, "./libvehello.so");
  if (handle == 0)
  {
    printf("veo_load_library() failed!\n");
    exit(1);
  }
  printf ("handle = %p\n", (void *) handle);
  uint64_t sym = veo_get_sym(proc, handle, "hello");
  if (sym == (uint64_t) 0)
  {
    printf("veo_get_sym() failed!\n");
    exit(1);
  }
  printf("symbol address = %p\n", (void *) sym);
 
  struct veo_thr_ctxt *ctx = veo_context_open(proc);
  if (ctx == NULL)
  {
    printf("veo_context_open() failed!\n");
    exit(1);
  }
  printf("VEO context = %p\n", ctx);
  struct veo_args *argp = veo_args_alloc();
  if(argp == NULL){
    printf("veo_args_alloc() failed!\n");
    exit(1);
  }
  if (veo_args_set_i64(argp, 0, 42) != 0) {
    printf("veo_args_set_i64() failed!\n");
    exit(1);
  }
  uint64_t id = veo_call_async(ctx, sym, argp);
  if (id == VEO_REQUEST_ID_INVALID) {
    printf("veo_call_async() failed!\n");
    exit(1);
  }
  printf("VEO request ID = 0x%lx\n", id);
  uint64_t retval;
  uint64_t ret;
  ret = veo_call_wait_result(ctx, id, &retval);
  if (ret != VEO_COMMAND_OK) {
    printf("veo_call_wait_result() failed!\n");
    exit(1);
  }
  if (retval != 43) {
    printf("veo_call_wait_result() failed!\n");
    exit(1);
  }
  printf("0x%lx: %d, %lu\n", id, ret, retval);
  veo_args_free(argp);
  int err = 0;
  err = veo_context_close(ctx);
  if (err != 0) {
    printf("veo_context_close() failed!\n");
    exit(1);
  }
  err = veo_proc_destroy(proc);
  if (err != 0) {
    printf("veo_proc_destroy() failed!\n");
    exit(1);
  }
}

int 
main()
{
  printf("main PID = %d, tid = %d\n", getpid (), gettid ());
  pthread_t th;
  struct veo_proc_handle *proc;
  pthread_create(&th, NULL, child, NULL);
  pthread_join(th, (void **)&proc);
  printf("end\n");
  return 0;
}
