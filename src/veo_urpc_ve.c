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
 * VE specific handlers for ve-urpc.
 * 
 * Copyright (c) 2020 Erich Focht
 */
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <dlfcn.h>
#include <pthread.h>

#include "veo_urpc.h"

extern __thread int __veo_finish;
pthread_t __handler_loop_pthreads[MAX_VE_CORES];
int __num_ve_peers = 0;

//
// VE side main handler loop
//
void *ve_handler_loop(void *arg)
{
  ve_handler_loop_arg_t *a = (ve_handler_loop_arg_t *)arg;

  urpc_peer_t *up = a->up;
  VEO_DEBUG("pid=%d core=%d", getpid(), a->core);
  int rc = ve_urpc_init_dma(up, a->core);

  urpc_set_receiver_flags(&up->recv, 1);

  __veo_finish = 0;
  while (!__veo_finish) {
    // carefull with number of progress calls
    // number * max_send_buff_size should not be larger than what we have
    // as send buffer memory
    ve_urpc_recv_progress(up, 3);
  }
  ve_urpc_fini(up);
  return NULL;
}

//
// Handlers
//

static int newpeer_handler(urpc_peer_t *up, urpc_mb_t *m, int64_t req,
                           void *payload, size_t plen)
{
  int segid, core;
  uint64_t stack_sz;
  pthread_attr_t _a;
  urpc_peer_t *new_up;

  urpc_unpack_payload(payload, plen, (char *)"IIL", &segid, &core, &stack_sz);
  new_up = ve_urpc_init(segid);

  ve_handler_loop_arg_t arg = { .up = new_up, .core = core };

  ve_urpc_unpin();

  VEO_DEBUG("Received stacksize of VEO context thread : 0x%lx", stack_sz);
  pthread_attr_init(&_a);
  pthread_attr_setstacksize(&_a, stack_sz);

  // start new pthread
  int rc = pthread_create(&__handler_loop_pthreads[__num_ve_peers], &_a, ve_handler_loop, (void *)&arg);
  if (rc) {
    int new_req = urpc_generic_send(up, URPC_CMD_RESULT, (char *)"L", (int64_t)rc);
    VEO_ERROR("pthread_create failed with rc=%d", rc);
    return -1;
  }

  pthread_attr_destroy(&_a);

  __num_ve_peers++;

  ve_urpc_init_dma(up, up->core);
  
  int new_req = urpc_generic_send(up, URPC_CMD_RESULT, (char *)"L", (int64_t)rc);
  CHECK_REQ(new_req, req);
  return rc;
}


static int call_handler(urpc_peer_t *up, urpc_mb_t *m, int64_t req,
                        void *payload, size_t plen)
{
  void *stack = NULL;
  size_t stack_size = 0;
  uint64_t stack_top, recv_sp = 0, curr_sp;
  uint64_t addr = 0;
  uint64_t *regs, result;
  size_t nregs;
  int cmd = m->c.cmd;

  asm volatile ("or %0, 0, %sp": "=r"(curr_sp));

  VEO_DEBUG("cmd=%d", m->c.cmd);
  
  if (cmd == URPC_CMD_CALL) {
    urpc_unpack_payload(payload, plen, (char *)"LP", &addr, (void **)&regs, &nregs);
    nregs = nregs / 8;
    VEO_DEBUG("CALL nregs=%d", nregs);

    //
    // if addr == 0: send current stack pointer
    //
    if (addr == 0) {
      dprintf("call_handler sending stack pointer value sp=%lx\n", curr_sp);
      int64_t new_req = urpc_generic_send(up, URPC_CMD_RESULT, (char *)"L", curr_sp);
      CHECK_REQ(new_req, req);
      return 0;
    }
    VEO_DEBUG("no stack, nregs=%d", nregs);

  } else if (cmd == URPC_CMD_CALL_STKIN ||
             cmd == URPC_CMD_CALL_STKINOUT) {

    urpc_unpack_payload(payload, plen, (char *)"LPLLP",
                        &addr, (void **)&regs, &nregs,
                        &stack_top, &recv_sp,
                        &stack, &stack_size);
    nregs = nregs / 8;
    VEO_DEBUG("stack IN or INOUT, nregs=%d, stack_top=%p",
              nregs, (void *)stack_top);
  } else if (cmd == URPC_CMD_CALL_STKOUT) {
    urpc_unpack_payload(payload, plen, (char *)"LPLLQ",
                        &addr, (void **)&regs, &nregs,
                        &stack_top, &recv_sp,
                        &stack, &stack_size);
    nregs = nregs / 8;
    VEO_DEBUG("stack OUT, nregs=%d, stack_top=%p, stack_size=%lu",
              nregs, (void *)stack_top, stack_size);
  }
  //
  // check if sent stack pointer is the same as the current one
  //
  asm volatile ("or %0, 0, %sp": "=r"(curr_sp));

  if (recv_sp && recv_sp != curr_sp) {
    VEO_ERROR("stack pointer mismatch! curr=%p expected=%p\n", curr_sp, recv_sp);
    return -1;
  }
  //
  // prepare "fake" stack for current function, which enables us to pass
  // parameters and variables that look like local variables on this
  // function's stack.
  //
  if (cmd == URPC_CMD_CALL_STKIN ||
      cmd == URPC_CMD_CALL_STKINOUT) {
    memcpy((void *)stack_top, stack, stack_size);
  }
  //
  // set up registers and call the function
  //
  uint64_t r[MAX_ARGS_IN_REGS];
  for (int i = 0; i < (nregs > MAX_ARGS_IN_REGS ? MAX_ARGS_IN_REGS : nregs); i++)
    r[i] = regs[i];
  if (cmd == URPC_CMD_CALL) {
    if (nregs == 0) {
      asm volatile(
                   "or %s12, 0, %1\n\t"          /* target function address */
                   "bsic %lr, (,%s12)\n\t"       /* call */
                   "or %0, 0, %s0"
                   :"=r"(result):"r"(addr), "r"(r));
    } else {
      asm volatile("ld %s0, 0x0(,%2)\n\t"
                   "ld %s1, 0x8(,%2)\n\t"
                   "ld %s2, 0x10(,%2)\n\t"
                   "ld %s3, 0x18(,%2)\n\t"
                   "ld %s4, 0x20(,%2)\n\t"
                   "ld %s5, 0x28(,%2)\n\t"
                   "ld %s6, 0x30(,%2)\n\t"
                   "ld %s7, 0x38(,%2)\n\t"
                   "or %s12, 0, %1\n\t"          /* target function address */
                   "bsic %lr, (,%s12)\n\t"       /* call */
                   "or %0, 0, %s0"
                   :"=r"(result):"r"(addr), "r"(r)
                   :"s0", "s1", "s2", "s3", "s4", "s5", "s6", "s7");
    }
  } else {
    asm volatile("ld %s0, 0x0(,%2)\n\t"
                 "ld %s1, 0x8(,%2)\n\t"
                 "ld %s2, 0x10(,%2)\n\t"
                 "ld %s3, 0x18(,%2)\n\t"
                 "ld %s4, 0x20(,%2)\n\t"
                 "ld %s5, 0x28(,%2)\n\t"
                 "ld %s6, 0x30(,%2)\n\t"
                 "ld %s7, 0x38(,%2)\n\t"
                 "or %s12, 0, %1\n\t"          /* target function address */
                 "st %fp, 0x0(,%sp)\n\t"       /* save original fp */
                 "st %lr, 0x8(,%sp)\n\t"       /* fake prologue */
                 "st %got, 0x18(,%sp)\n\t"
                 "st %plt, 0x20(,%sp)\n\t"
                 "or %fp, 0, %sp\n\t"          /* switch fp to new frame */
                 "or %sp, 0, %3\n\t"           /* switch sp to new frame */
                 "bsic %lr, (,%s12)\n\t"       /* call */
                 "or %sp, 0, %fp\n\t"          /* restore original sp */
                 "ld %fp, 0x0(,%sp)\n\t"       /* restore original fp */
                 "or %0, 0, %s0"
                 :"=r"(result):"r"(addr), "r"(r), "r"(stack_top)
                 :"s0", "s1", "s2", "s3", "s4", "s5", "s6", "s7");
  }

  // "cmd" seems to be overwritten when we call the function.
  // We set "cmd", again
  cmd = m->c.cmd;

  if (cmd == URPC_CMD_CALL_STKOUT ||
      cmd == URPC_CMD_CALL_STKINOUT) {
    // copying back from stack must happen in the same function,
    // otherwise we might overwrite the stack! So we simply use a loop.
    #pragma _NEC ivdep
    for (int i = 0; i < stack_size / 8; i++) {
      ((uint64_t *)stack)[i] = ((uint64_t *)stack_top)[i];
    }
    VEO_DEBUG("sending RES_STK");
    int64_t new_req = urpc_generic_send(up, URPC_CMD_RES_STK, (char *)"LP",
                                        result, stack, stack_size);
    CHECK_REQ(new_req, req);
  } else {
    VEO_DEBUG("sending RESULT");
    int64_t new_req = urpc_generic_send(up, URPC_CMD_RESULT, (char *)"L", result);
    CHECK_REQ(new_req, req);
  }
  return 0;
}

static int access_pcircvsyc_register_handler(urpc_peer_t *up, urpc_mb_t *m, int64_t req,
                        void *payload, size_t plen)
{
  // use LHM. addr is 0x1400
  uint64_t addr = 0x1400;
  uint64_t regval;
  asm volatile("lhm.l %0,0(%1)":"=r"(regval):"r"(addr));
  VEO_DEBUG("access PCIRCVSYC register");
  urpc_generic_send(up, URPC_CMD_ACK, (char *)"");
  return 0;
}

void veo_urpc_register_ve_handlers(urpc_peer_t *up)
{
  int err;

  if ((err = urpc_register_handler(up, URPC_CMD_NEWPEER, &newpeer_handler)) < 0)
    VEO_ERROR("failed cmd %d", 1);
  if ((err = urpc_register_handler(up, URPC_CMD_CALL, &call_handler)) < 0)
    VEO_ERROR("failed cmd %d", 1);
  if ((err = urpc_register_handler(up, URPC_CMD_CALL_STKIN, &call_handler)) < 0)
    VEO_ERROR("failed cmd %d", 1);
  if ((err = urpc_register_handler(up, URPC_CMD_CALL_STKOUT, &call_handler)) < 0)
    VEO_ERROR("failed cmd %d", 1);
  if ((err = urpc_register_handler(up, URPC_CMD_CALL_STKINOUT, &call_handler)) < 0)
    VEO_ERROR("failed cmd %d", 1);
  if ((err = urpc_register_handler(up, URPC_CMD_ACS_PCIRCVSYC, &access_pcircvsyc_register_handler)) < 0)
    VEO_ERROR("failed cmd %d", 1);
}

__attribute__((constructor(10001)))
static void _veo_urpc_init_register(void)
{
  VEO_TRACE("registering VE URPC handlers");
  urpc_set_handler_init_hook(&veo_urpc_register_ve_handlers);
}
