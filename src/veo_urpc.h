#ifndef VEO_URPC_INCLUDE
#define VEO_URPC_INCLUDE

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
 * Common VEO specific ve-urpc commands and constants.
 * 
 * Copyright (c) 2020 Erich Focht
 */
#include <urpc.h>
#include <urpc_debug.h>
#include "log.h"

// reply timeout in us
#define REPLY_TIMEOUT 20000000

// multipart SEND/RECVFRAG transfer size
#define PART_SENDFRAG (4*1024*1024)

// The maximum size of arguments we can send using URPC_CMD_CALL_STKxx.
// 40 is the size to send message with "LPLLP".
//#define MAX_ARGS_STACK_SIZE (DATA_BUFF_END - 40)

// The size of a reserved stack area to avoid overwrite of arguments
// We chose a big value as possible as we can, because the code to
// print debug messages use stack.
#define RESERVED_STACK_SIZE 512*1024

// maximum number of arguments transfered in registers on VE
#define MAX_ARGS_IN_REGS 8

#ifdef __cplusplus
extern "C" {
#endif

extern __thread int __veo_finish;

void veo_urpc_register_handlers(urpc_peer_t *up);

#ifdef __ve__

extern pthread_t __handler_loop_pthreads[MAX_VE_CORES];
extern int __num_ve_peers;
  
typedef struct {
  urpc_peer_t *up;
  int core;
} ve_handler_loop_arg_t;

void veo_urpc_register_ve_handlers(urpc_peer_t *up);
void *ve_handler_loop(void *arg);

#else

void veo_urpc_register_vh_handlers(urpc_peer_t *up);

#endif
#ifdef __cplusplus
}
#endif

//
// URPC commands
//
enum veo_urpc_cmd
  {
   URPC_CMD_NONE_         =  0, // No command, marks that slot is done
   URPC_CMD_PING          =  1, // Alive check, replies with ACK
   URPC_CMD_EXIT          =  2, // Peer is signalled to exit
   URPC_CMD_ACK           =  3, // ACK is a result with no (void) content
   URPC_CMD_RESULT        =  4, // result (int64_t) without stack frame
   URPC_CMD_RES_STK       =  5, // result with stack frame
   URPC_CMD_EXCEPTION     =  6, // notify about exception
   URPC_CMD_LOADLIB       =  7, // load .so
   URPC_CMD_UNLOADLIB     =  8, // unload .so
   URPC_CMD_GETSYM        =  9, // find symbol in .so
   URPC_CMD_ALLOC         = 10, // allocate buffer on VE
   URPC_CMD_FREE          = 11, // free buffer on VE
   URPC_CMD_SENDBUFF      = 12, // send a buffer (as payload), args contain address and len
   URPC_CMD_RECVBUFF      = 13, // receive a buffer (comes as payload), args contain dst addr
   URPC_CMD_CALL          = 14, // simple call with no stack transfer
   URPC_CMD_CALL_STKIN    = 15, // call with stack "IN" only
   URPC_CMD_CALL_STKOUT   = 16, // call with stack "OUT" only
   URPC_CMD_CALL_STKINOUT = 17, // call with stack IN and OUT
   URPC_CMD_SLEEPING      = 18, // notify peer that we're going to sleep
   URPC_CMD_NEWPEER       = 19, // create new remote peer (AKA context) inside same proc
   URPC_CMD_ACS_PCIRCVSYC = 20, // access PCIRCVSYC register
   URPC_CMD_MEMCPY        = 21  // copy memory
  };


#endif /* VEO_URPC_INCLUDE */
