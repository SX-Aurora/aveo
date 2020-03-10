#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <string.h>
#include <signal.h>

#define __USE_GNU
#include <dlfcn.h>

#include <urpc.h>
#include <urpc_debug.h>
#include <urpc_time.h>
#include <veo_urpc.h>

urpc_peer_t *main_up;

void signalHandler( int signum ) {
  Dl_info di;
  
  //VEO_ERROR("Interrupt signal %s received", strsignal(signum));
  VEO_ERROR("Interrupt signal %d received", signum);

  // try to print info about stack trace
  __builtin_traceback((unsigned long *)__builtin_frame_address(0));
  void *f = __builtin_return_address(0);
  if (f) {
    if (dladdr(f, &di)) {
      printf("%p -> %s\n", f, di.dli_sname);
    } else {
      printf("%p\n", f);
    }
    void *f = __builtin_return_address(1);
    if (f) {
      if (dladdr(f, &di)) {
        printf("%p -> %s\n", f, di.dli_sname);
      } else {
        printf("%p\n", f);
      }
      void *f = __builtin_return_address(2);
      if (f) {
        if (dladdr(f, &di)) {
          printf("%p -> %s\n", f, di.dli_sname);
        } else {
          printf("%p\n", f);
        }
      }
    }
  }
  urpc_generic_send(main_up, URPC_CMD_EXCEPTION, (char *)"L", (int64_t)signum);
  ve_urpc_fini(main_up);
  exit(signum);  
}


int main()
{
  int err, core = 0;
  long ts = get_time_us();

  signal(SIGABRT, signalHandler);
  signal(SIGFPE, signalHandler);
  signal(SIGILL, signalHandler);
  signal(SIGSEGV, signalHandler);

  main_up = ve_urpc_init(0);
  
  char *e;
  e = getenv("URPC_VE_CORE");
  if (e)
    core = atoi(e);

  ve_handler_loop_arg_t arg = { .up = main_up,
                                .core = core };

  ve_handler_loop((void *)&arg);

  if (main_up == NULL)
    return -1;

  for (int i = 0; i < __num_ve_peers; i++) {
    void *ret;
    pthread_join(__handler_loop_pthreads[i], &ret);
  }
  return 0;
}
