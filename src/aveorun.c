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
#include <errno.h>

#define __USE_GNU
#include <dlfcn.h>

#include <urpc.h>
#include <urpc_debug.h>
#include <urpc_time.h>
#include <veo_urpc.h>

urpc_peer_t *main_up;

static struct {
  int signum;
  sig_t handler;
  int sigaction;
} signal_tbl[] = {
  {SIGABRT, SIG_DFL, 0},
  {SIGFPE,  SIG_DFL, 0},
  {SIGILL,  SIG_DFL, 0},
  {SIGSEGV, SIG_DFL, 0},
  {SIGBUS,  SIG_DFL, 0},
  {0,       SIG_DFL, 0}  // termination
};

static void sigactionHandler(int signum, siginfo_t *sig_info, void *ucontext) {
  int idx;
  union {
    sig_t handler;
    void (*sigaction)(int, siginfo_t *, void *);
  } handle;
  int sigaction=0;

  VEO_ERROR("Interrupt signal %d received", signum);

  // try to print info about stack trace
  handle.handler = SIG_DFL;
  for (idx=0 ; signal_tbl[idx].signum != 0 ; idx++) {
    if (signum == signal_tbl[idx].signum) {
      if(signal_tbl[idx].handler != SIG_DFL) {
        handle.handler = signal_tbl[idx].handler;
        sigaction = signal_tbl[idx].sigaction;
      }
      break;
    }
  }
  if (handle.handler != SIG_DFL) {
    if (sigaction) {
      (*(handle.sigaction))(signum, sig_info, ucontext);
    } else {
      (*(handle.handler))(signum);
    }
  } else {
    // processes as before
    Dl_info di;
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
  int idx;
  struct sigaction act;

  for (idx=0 ; signal_tbl[idx].signum != 0 ; idx++) {
    err = sigaction(signal_tbl[idx].signum, NULL, &act);
    if (err != 0) {
      perror("sigaction");
      return 1;
    }
    if (act.sa_handler == SIG_DFL || act.sa_handler == SIG_IGN || act.sa_handler == SIG_ERR) {
      err = sigemptyset(&(act.sa_mask));
      if (err != 0) {
        perror("sigemptyset");
        return 1;
      }
      err = sigaddset(&(act.sa_mask), signal_tbl[idx].signum);
      if (err != 0) {
        perror("sigaddset");
        return 1;
      }
      act.sa_flags = SA_SIGINFO | SA_RESTART;
      act.sa_restorer = NULL;
    } else {
      // act.sa_handler and act.sa_sigaction is defined by union.
      // Old handler is saved using act.sa_handler to make code easy.
      signal_tbl[idx].handler = act.sa_handler;
    }
    act.sa_sigaction = &sigactionHandler;
    if (act.sa_flags & SA_SIGINFO) {
      signal_tbl[idx].sigaction = 1;
    }
    err = sigaction(signal_tbl[idx].signum, &act, NULL);
    if (err != 0) {
      perror("sigaction");
      return 1;
    }
  }
  dlopen("libnc++.so.2", RTLD_NOW | RTLD_GLOBAL);
  dlopen("libc++.so.1-2", RTLD_NOW | RTLD_GLOBAL);

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

  return 0;
}
