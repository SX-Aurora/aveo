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
  int64_t send_call_nolock(urpc_peer_t *up, uint64_t ve_sp, uint64_t addr, CallArgs &args)
  {
    int64_t req;

    void *stack_buf = (void *)args.stack_buf.get();
    auto regs = args.getRegVal(ve_sp);
    size_t regs_sz = regs.size() * sizeof(uint64_t);

    if (!(args.copied_in || args.copied_out)) {
      // no stack transfer
      // transfered data: addr, regs array
    
      req = urpc_generic_send(up, URPC_CMD_CALL, (char *)"LP",
                              addr, (void *)regs.data(), regs_sz);

    } else if (args.copied_in && !args.copied_out) {
      // stack copied IN only
      // transfered data: addr, regs array, stack_top, stack_pointer, stack_image

      size_t size = args.stack_size;
      if (size > MAX_ARGS_STACK_SIZE - regs_sz)
        size = MAX_ARGS_STACK_SIZE - regs_sz;
      req = urpc_generic_send(up, URPC_CMD_CALL_STKIN, (char *)"LPLLP",
                              addr, (void *)regs.data(), regs_sz,
                              args.stack_top, ve_sp,
                              stack_buf, size);

      VEO_DEBUG("stack IN, nregs=%d stack_top=%p sp=%p stack_size=%d", regs_sz/8,
                (void *)args.stack_top, (void *)ve_sp, args.stack_size);
    } else if (args.copied_in && args.copied_out) {
      // stack transfered into VE and back,
      // transfered data: addr, regs array, stack_top, stack_pointer, stack_image

      size_t size = args.stack_size;
      if (size > MAX_ARGS_STACK_SIZE - regs_sz)
        size = MAX_ARGS_STACK_SIZE - regs_sz;
      req = urpc_generic_send(up, URPC_CMD_CALL_STKINOUT, (char *)"LPLLP",
                              addr, (void *)regs.data(), regs_sz,
                              args.stack_top, ve_sp,
                              stack_buf, size);
      VEO_DEBUG("stack INOUT, nregs=%d stack_top=%p sp=%p stack_size=%d",
                regs_sz/8, (void *)args.stack_top, (void *)ve_sp, args.stack_size);
    } else if (!args.copied_in && args.copied_out) {
      // stack transfered only back, from VE to VH
      // transfered data: addr, regs array, stack_top, stack_pointer, stack_image

      size_t size = args.stack_size;
      if (size > MAX_ARGS_STACK_SIZE - regs_sz)
        size = MAX_ARGS_STACK_SIZE - regs_sz;
      req = urpc_generic_send(up, URPC_CMD_CALL_STKOUT, (char *)"LPLLQ",
                              addr, (void *)regs.data(), regs_sz,
                              args.stack_top, ve_sp,
                              stack_buf, size);
      VEO_DEBUG("stack OUT, nregs=%d stack_top=%p sp=%p stack_size=%d",
                regs_sz/8, (void *)args.stack_top, (void *)ve_sp, args.stack_size);
    }
    return req;
  }

  
  int unpack_call_result(urpc_mb_t *m, CallArgs *args, void *payload, size_t plen, uint64_t *result)
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
      rc = urpc_unpack_payload(payload, plen, (char *)"LP", (int64_t *)result,
                               &stack_buf, &args->stack_size);
      if (rc == 0) {
        memcpy(args->stack_buf.get(), stack_buf, args->stack_size);
        args->copyout();
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
