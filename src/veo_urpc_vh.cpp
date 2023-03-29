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
 * VH side handlers and helpers for ve-urpc commands.
 * 
 * Copyright (c) 2020 Erich Focht
 */
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <dlfcn.h>

#include <CallArgs.hpp>
#include "veo_urpc.h"
#include "veo_urpc_vh.hpp"

namespace veo {
  
  //
  // Commands
  //

  /**
   * @brief Send kernel call through URPC
   *
   * @param up URPC peer
   * @param ve_sp the VE stack pointer inside the call handler
   * @param addr VEMVA of function called
   * @param args arguments of the function
   * @return request ID if successful, -1 if failed.
   */
  int64_t send_call_nolock(urpc_peer_t *up, uint64_t ve_sp, uint64_t addr,
                           std::vector<uint64_t> const &regs,
                           uint64_t stack_top, size_t stack_size,
                           bool copyin, bool copyout, void *stack_buf)
  {
    int64_t req;

    void *regsp = (void *)regs.data();
    size_t regsz = regs.size() * sizeof(uint64_t);
    size_t size = stack_size;
    int64_t max_args_size = urpc_max_send_cmd_size(up)
                             - SEND_CALL_CMD_SIZE_WITHOUT_DATA;
    if (size > max_args_size - regsz)
      size = max_args_size - regsz;

    if (!(copyin || copyout)) {
      // no stack transfer
      // transfered data: addr, regs array
    
      req = urpc_generic_send(up, URPC_CMD_CALL, (char *)"LP",
                              addr, regsp, regsz);

    } else if (copyin && !copyout) {
      // stack copied IN only
      // transfered data: addr, regs array, stack_top, stack_pointer, stack_image

      req = urpc_generic_send(up, URPC_CMD_CALL_STKIN, (char *)"LPLLP",
                              addr, regsp, regsz, stack_top, ve_sp,
                              stack_buf, size);

      VEO_DEBUG("stack IN, nregs=%d stack_top=%p sp=%p stack_size=%d", regsz/8,
                (void *)stack_top, (void *)ve_sp, stack_size);
    } else if (copyin && copyout) {
      // stack transfered into VE and back,
      // transfered data: addr, regs array, stack_top, stack_pointer, stack_image

      req = urpc_generic_send(up, URPC_CMD_CALL_STKINOUT, (char *)"LPLLP",
                              addr, regsp, regsz, stack_top, ve_sp,
                              stack_buf, size);
      VEO_DEBUG("stack INOUT, nregs=%d stack_top=%p sp=%p stack_size=%d",
                regsz/8, (void *)stack_top, (void *)ve_sp, stack_size);
    } else if (!copyin && copyout) {
      // stack transfered only back, from VE to VH
      // transfered data: addr, regs array, stack_top, stack_pointer, stack_image

      req = urpc_generic_send(up, URPC_CMD_CALL_STKOUT, (char *)"LPLLQ",
                              addr, regsp, regsz, stack_top, ve_sp,
                              stack_buf, size);
      VEO_DEBUG("stack OUT, nregs=%d stack_top=%p sp=%p stack_size=%d",
                regsz/8, (void *)stack_top, (void *)ve_sp, stack_size);
    }
    return req;
  }

  
  int unpack_call_result(urpc_mb_t *m, std::function<void(void *)> copyout,
                         void *payload, size_t plen, uint64_t *result, void *stack)
  {
    int rc = -1;

    if (m->c.cmd == URPC_CMD_RESULT) {
      if (plen) {
        rc = urpc_unpack_payload(payload, plen, (char *)"L", (int64_t *)result);
      } else {
        VEO_ERROR("message had no payload!?");
      }
    } else if (m->c.cmd == URPC_CMD_RES_STK) {
      void *stack_buf = nullptr;
      size_t stack_size;
      rc = urpc_unpack_payload(payload, plen, (char *)"LP", (int64_t *)result,
                               &stack_buf, &stack_size);
      if (rc == 0) {
        if (stack != nullptr) {
          memcpy(stack, stack_buf, stack_size);
          return rc;
        }
        copyout(stack_buf);
      }
    } else if (m->c.cmd == URPC_CMD_EXCEPTION) {
      uint64_t exc = 0;
      char *msg = nullptr;
      size_t msglen = 0;
      rc = urpc_unpack_payload(payload, plen, (char *)"LP", &exc, (void *)&msg, &msglen);
      VEO_ERROR("VE exception %d\n%s", exc, msg);
      *result = exc;
      rc = -4;
    } else {
      VEO_ERROR("expected RESULT or RES_STK, got cmd=%d", m->c.cmd);
      rc = -3;
    }
  return rc;
  }

  int wait_req_result(urpc_peer_t *up, int64_t req, int64_t *result)
  {
    // wait for result
    urpc_mb_t m;
    void *payload;
    size_t plen;
    if (!urpc_recv_req_timeout(up, &m, req, REPLY_TIMEOUT, &payload, &plen)) {
      // timeout! complain.
      VEO_ERROR("timeout waiting for RESULT req=%ld", req);
      return -1;
    }
    if (m.c.cmd == URPC_CMD_RESULT) {
      if (plen) {
        urpc_unpack_payload(payload, plen, (char *)"L", result);
        urpc_slot_done(up->recv.tq, REQ2SLOT(req), &m);
      } else {
        VEO_DEBUG("result message for req=%ld had no payload!?", req);
      }
    } else {
      VEO_ERROR("unexpected RESULT message type: %d", m.c.cmd);
      return -1;
    }
    return 0;
  }

  int wait_req_ack(urpc_peer_t *up, int64_t req)
  {
    // wait for result
    urpc_mb_t m;
    void *payload;
    size_t plen;
    if (!urpc_recv_req_timeout(up, &m, req, REPLY_TIMEOUT, &payload, &plen)) {
      // timeout! complain.
      VEO_ERROR("timeout waiting for ACK req=%ld", req);
      return -1;
    }
    if (m.c.cmd != URPC_CMD_ACK) {
      VEO_ERROR("unexpected ACK message type: %d", m.c.cmd);
      return -2;
    }
    urpc_slot_done(up->recv.tq, REQ2SLOT(req), &m);
    return 0;
  }

} // namespace veo

#ifdef __cplusplus
extern "C" {
#endif

using namespace veo;

//
// Handlers
//
  

void veo_urpc_register_vh_handlers(urpc_peer_t *up)
{
  int err;

  //if ((err = urpc_register_handler(up, URPC_CMD_PING, &ping_handler)) < 0)
  //  VEO_ERROR("register_handler failed for cmd %d\n", 1);
}

__attribute__((constructor(10001)))
static void _veo_urpc_init_register(void)
{
  VEO_TRACE("registering VH URPC handlers");
  urpc_set_handler_init_hook(&veo_urpc_register_vh_handlers);
}

#ifdef __cplusplus
} //extern "C"
#endif
