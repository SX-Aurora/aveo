/**
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *
 *
 * VE-side main program running the ve-urpc handler loop.
 * 
 * Copyright (c) 2020 Erich Focht
 */
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
  unsigned long frame = (unsigned long)__builtin_frame_address(0);
  if (frame) {
    __builtin_traceback((unsigned long *)frame);
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
  }
  urpc_generic_send(main_up, URPC_CMD_EXCEPTION, (char *)"L", (int64_t)signum);
  // mark this side as "in exception". "this side" is the sender side
  urpc_set_receiver_flags(&main_up->recv, urpc_get_receiver_flags(&main_up->recv) | URPC_FLAG_EXCEPTION);
  ve_urpc_fini(main_up);
  exit(signum);  
}


int main()
{
  int err = 0;
  int core = -1;
  long ts = get_time_us();

  signal(SIGABRT, signalHandler);
  signal(SIGFPE, signalHandler);
  signal(SIGILL, signalHandler);
  signal(SIGSEGV, signalHandler);
  signal(SIGBUS, signalHandler);

  main_up = ve_urpc_init(0);
  if (main_up == NULL)
    return 1;
  
  char *e;
  e = getenv("URPC_VE_CORE");
  if (e)
    core = atoi(e);

  ve_handler_loop_arg_t arg = { .up = main_up,
                                .core = core };

  ve_handler_loop((void *)&arg);

  for (int i = 0; i < __num_ve_peers; i++) {
    void *ret;
    pthread_join(__handler_loop_pthreads[i], &ret);
  }
  return 0;
}
