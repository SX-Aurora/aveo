#ifndef VEO_URPC_INCLUDE
#define VEO_URPC_INCLUDE

#include <urpc_common.h>

// reply timeout in us
#define REPLY_TIMEOUT 20000000

// multipart SEND/RECVFRAG transfer size
#define PART_SENDFRAG (6*1024*1024)

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
	URPC_CMD_NONE_         =  0,
	URPC_CMD_PING          =  1,
	URPC_CMD_EXIT          =  2,
	URPC_CMD_ACK           =  3, // ACK is a result with no (void) content
	URPC_CMD_RESULT        =  4, // result (int64_t) without stack frame
	URPC_CMD_RES_STK       =  5, // result with stack frame
        URPC_CMD_EXCEPTION     =  6, // notify about exception
	URPC_CMD_LOADLIB       =  7, // load .so
	URPC_CMD_UNLOADLIB     =  8, // unload .so
	URPC_CMD_GETSYM        =  9, // find symbol in .so
	URPC_CMD_ALLOC         = 10, // allocate buffer on VE
	URPC_CMD_FREE          = 11, // free buffer on VE
	URPC_CMD_SENDBUFF      = 12, 
	URPC_CMD_RECVBUFF      = 13,
	URPC_CMD_CALL          = 14, // simple call with no stack transfer
	URPC_CMD_CALL_STKIN    = 15, // call with stack "IN" only
	URPC_CMD_CALL_STKOUT   = 16, // call with stack "OUT" only
	URPC_CMD_CALL_STKINOUT = 17, // call with stack IN and OUT
	URPC_CMD_SLEEPING      = 18, // notify peer that we're going to sleep
	URPC_CMD_NEWPEER       = 19  // create new remote peer (AKA context) inside same proc
};


#endif /* VEO_URPC_INCLUDE */
