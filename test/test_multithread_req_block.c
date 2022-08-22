#include <stdio.h>
#include <stdlib.h>
#include <ve_offload.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/syscall.h>
 
long syscall(long n, ...);
 
int rounds = 100;
uint64_t return_arg_sym;
uint64_t prevres_quiet_sym;
struct veo_thr_ctxt *ctx;

int gettid(void)
{
  return syscall(SYS_gettid);
}
 
void *child(void *proc)
{
  int err;
  int trounds = rounds;
  int tid = gettid();
  printf("test PID = %d, tid = %d\n", getpid(), tid);
  srand((unsigned int)tid);

  struct veo_args *argp = veo_args_alloc();
  
  while (trounds > 0) {
    uint64_t res1, res2;
    res1 = res2 = 0;

    uint64_t r = (uint64_t)rand();
    veo_args_clear(argp);
    veo_args_set_u64(argp, 0, r);

    veo_req_block_begin(ctx);
    uint64_t req1 = veo_call_async(ctx, return_arg_sym, argp);

    veo_args_clear(argp);
    veo_args_set_i32(argp, 0, 0);
    uint64_t req2 = veo_call_async(ctx, prevres_quiet_sym, argp);
    veo_req_block_end(ctx);
    
    err = veo_call_wait_result(ctx, req1, &res1);
    err = veo_call_wait_result(ctx, req2, &res2);

    if (r != res1 || r != res2) {
      printf("tid=%d r=%lx res1=%lx res2=%lx\n", tid, r, res1, res2);
    }
    --trounds;
  }
  veo_args_free(argp);
  printf("TID %ld done\n", tid);
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

  uint64_t libh = veo_load_library(proc, "./libvealloc.so");
  if (libh == 0)
    return -1;
  return_arg_sym = veo_get_sym(proc, libh, "return_arg");
  prevres_quiet_sym = veo_get_sym(proc, libh, "prevres_quiet");
  printf("'return_arg' sym = %p\n", (void *)return_arg_sym);
  printf("'prevres_quiet' sym = %p\n", (void *)prevres_quiet_sym);
  if (prevres_quiet_sym == 0 || return_arg_sym == 0)
    return -1;

  ctx = veo_context_open(proc);
  printf("ctx = %p\n", (void *)ctx);

  //child(proc);

  pthread_create(&th1, NULL, child, proc);
  pthread_create(&th2, NULL, child, proc);
  pthread_create(&th3, NULL, child, proc);
  pthread_create(&th4, NULL, child, proc);
  pthread_join(th1, NULL);//(void **)&proc);
  pthread_join(th2, NULL);//(void **)&proc);
  pthread_join(th3, NULL);//(void **)&proc);
  pthread_join(th4, NULL);//(void **)&proc);
  printf("end\n");
  //veo_proc_destroy(proc);
 
  return 0;
}
