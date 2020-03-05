#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <dlfcn.h>

#include <CallArgs.hpp>
#include "veo_urpc.hpp"

#ifdef __cplusplus
extern "C" {
#endif

__thread int __veo_finish;
const char *VERSION = AVEO_VERSION_STRING;	// the version comes as a #define

typedef struct {char *n; void *v;} static_sym_t;
/* in dummy.c or the self-compiled generated static symtable */
extern static_sym_t *_veo_static_symtable;
  
using namespace veo;

  //
  // Handlers
  //
  
static int ping_handler(urpc_peer_t *up, urpc_mb_t *m, int64_t req,
                        void *payload, size_t plen)
{
  urpc_generic_send(up, URPC_CMD_ACK, (char *)"");
  return 0;
}

static int exit_handler(urpc_peer_t *up, urpc_mb_t *m, int64_t req,
                        void *payload, size_t plen)
{
  __veo_finish = 1;
  dprintf("up=%p has received the EXIT command\n", (void *)up);
  urpc_generic_send(up, URPC_CMD_ACK, (char *)"");
#ifdef __ve__
  ve_urpc_fini(up);
#else
  vh_urpc_peer_destroy(up);
#endif
  pthread_exit(0);
  return 0;
}

static int loadlib_handler(urpc_peer_t *up, urpc_mb_t *m, int64_t req,
                           void *payload, size_t plen)
{
  size_t psz;
  char *libname;

  urpc_unpack_payload(payload, plen, (char *)"P", &libname, &psz);

  uint64_t handle = (uint64_t)dlopen(libname, RTLD_NOW);
  dprintf("loadlib_handler libname=%s handle=%p\n", libname, handle);
  if (!handle) {
    char *e = dlerror();
    eprintf("loadlib_handler dlerror: %s\n", e);
  }

  int64_t new_req = urpc_generic_send(up, URPC_CMD_RESULT, (char *)"L", handle);
  // check req IDs. Result expected with exactly same req ID.
  if (new_req != req) {
    eprintf("loadlib_handler: send result req ID mismatch: %ld instead of %ld\n",
            new_req, req);
    return -1;
  }
  return 0;
}

static int unloadlib_handler(urpc_peer_t *up, urpc_mb_t *m, int64_t req,
                             void *payload, size_t plen)
{
  size_t psz;
  uint64_t libhndl;

  urpc_unpack_payload(payload, plen, (char *)"L", &libhndl);

  int rc = dlclose((void *)libhndl);

  int64_t new_req = urpc_generic_send(up, URPC_CMD_RESULT, (char *)"L", (int64_t)rc);
  if (new_req != req || new_req < 0) {
    eprintf("unloadlib_handler: send result failed\n");
    return -1;
  }
  return 0;
}

static int getsym_handler(urpc_peer_t *up, urpc_mb_t *m, int64_t req,
                          void *payload, size_t plen)
{
  uint64_t libhndl;
  char *sym;
  size_t psz;
  uint64_t symaddr = 0;

  urpc_unpack_payload(payload, plen, (char *)"LP", &libhndl, &sym, &psz);
  if (libhndl) {
    symaddr = (uint64_t)dlsym((void *)libhndl, sym);
    if (!symaddr)
      eprintf("getsym_handler dlerror: %s\n", dlerror());
  }

  if (_veo_static_symtable) {
    static_sym_t *t = _veo_static_symtable;
    while (t->n != NULL) {
      if (strcmp(t->n, sym) == 0) {
        symaddr = (uint64_t)t->v;
        break;
      }
      t++;
    }
  }
	
  int64_t new_req = urpc_generic_send(up, URPC_CMD_RESULT, (char *)"L", symaddr);
  // check req IDs. Result expected with exactly same req ID.
  if (new_req != req) {
    printf("getsym_handler: send result req ID mismatch: %ld instead of %ld\n",
           new_req, req);
    return -1;
  }
  return 0;
}

static int alloc_handler(urpc_peer_t *up, urpc_mb_t *m, int64_t req,
                         void *payload, size_t plen)
{
  size_t psz;
  size_t allocsz;
    
  urpc_unpack_payload(payload, plen, (char *)"L", &allocsz);
    
  void *addr = malloc(allocsz);
  dprintf("alloc_handler addr=%p size=%lu\n", addr, allocsz);

  int64_t new_req = urpc_generic_send(up, URPC_CMD_RESULT, (char *)"L", (uint64_t)addr);
  // check req IDs. Result expected with exactly same req ID.
  if (new_req != req) {
    eprintf("alloc_handler: send result req ID mismatch: %ld instead of %ld\n",
            new_req, req);
    return -1;
  }
  return 0;
}

static int free_handler(urpc_peer_t *up, urpc_mb_t *m, int64_t req,
                        void *payload, size_t plen)
{
  size_t psz;
  uint64_t addr;

  urpc_unpack_payload(payload, plen, (char *)"L", &addr);
  
  free((void *)addr);
  dprintf("free_handler addr=%p\n", (void *)addr);

  int64_t new_req = urpc_generic_send(up, URPC_CMD_ACK, (char *)"");
  // check req IDs. Result expected with exactly same req ID.
  if (new_req != req) {
    eprintf("free_handler: send result req ID mismatch: %ld instead of %ld\n",
            new_req, req);
    return -1;
  }
  return 0;
}

/**
 * @brief Handles a SENDBUFF command
 *
 * Receives the buffer carried by the command and copies it to
 * the destination address.
 */
static int sendbuff_handler(urpc_peer_t *up, urpc_mb_t *m, int64_t req,
                            void *payload, size_t plen)
{
  uint64_t dst;
  void *buff;
  size_t buffsz;

  urpc_unpack_payload(payload, plen, (char *)"LP", &dst, &buff, &buffsz);
  dprintf("sendbuff_handler dst=%p size=%lu\n", (void *)dst, buffsz);

  memcpy((void *)dst, buff, buffsz);

  int64_t new_req = urpc_generic_send(up, URPC_CMD_ACK, (char *)"");
      
  // check req IDs. Result expected with exactly same req ID.
  if (new_req != req) {
    eprintf("sendbuff_handler: send result req ID mismatch:"
            " %ld instead of %ld\n", new_req, req);
    return -1;
  }
  return 0;
}

/**
 * @brief Handles a RECVBUFF command
 *
 * Sends the buffer requested by the command in a SENDBUFF reply.
 * The requested buffer must fit into one URPC transfer!
 */
static int recvbuff_handler(urpc_peer_t *up, urpc_mb_t *m, int64_t req,
                            void *payload, size_t plen)
{
  uint64_t src, dst;
  size_t size;

  urpc_unpack_payload(payload, plen, (char *)"LLL", &src, &dst, &size);
  dprintf("recvbuff_handler src=%p dst=%p size=%lu\n", (void *)src, (void *)dst, size);

  int64_t new_req = urpc_generic_send(up, URPC_CMD_SENDBUFF, (char *)"LP",
                                      dst, (void *)src, size);
  if (new_req != req) {
    eprintf("recvbuff_handler: send result req ID mismatch:"
            " %ld instead of %ld\n", new_req, req);
    return -1;
  }
  return 0;
}

void veo_urpc_register_handlers(urpc_peer_t *up)
{
  int err;

  if ((err = urpc_register_handler(up, URPC_CMD_PING, &ping_handler)) < 0)
    eprintf("register_handler failed for cmd %d\n", 1);
  if ((err = urpc_register_handler(up, URPC_CMD_EXIT, &exit_handler)) < 0)
    eprintf("register_handler failed for cmd %d\n", 1);
  if ((err = urpc_register_handler(up, URPC_CMD_LOADLIB, &loadlib_handler)) < 0)
    eprintf("register_handler failed for cmd %d\n", 1);
  if ((err = urpc_register_handler(up, URPC_CMD_UNLOADLIB, &unloadlib_handler)) < 0)
    eprintf("register_handler failed for cmd %d\n", 1);
  if ((err = urpc_register_handler(up, URPC_CMD_GETSYM, &getsym_handler)) < 0)
    eprintf("register_handler failed for cmd %d\n", 1);
  if ((err = urpc_register_handler(up, URPC_CMD_ALLOC, &alloc_handler)) < 0)
    eprintf("register_handler failed for cmd %d\n", 1);
  if ((err = urpc_register_handler(up, URPC_CMD_FREE, &free_handler)) < 0)
    eprintf("register_handler failed for cmd %d\n", 1);
  if ((err = urpc_register_handler(up, URPC_CMD_RECVBUFF, &recvbuff_handler)) < 0)
    eprintf("register_handler failed for cmd %d\n", 1);
  if ((err = urpc_register_handler(up, URPC_CMD_SENDBUFF, &sendbuff_handler)) < 0)
    eprintf("register_handler failed for cmd %d\n", 1);
}

__attribute__((constructor))
static void _veo_urpc_init_register(void)
{
  dprintf("registering common URPC handlers\n");
  urpc_set_handler_init_hook(&veo_urpc_register_handlers);
}
  
#ifdef __cplusplus
} //extern "C"
#endif
