#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <dlfcn.h>
#include <unistd.h>
#include <sys/syscall.h>

#include "veo_urpc.h"
#include "log.h"

__thread int __veo_finish;
const char *AVEO_VERSION = AVEO_VERSION_STRING;	// the version comes as a #define

typedef struct {const char *n; void *v;} static_sym_t;
/* without generated static symtable, this weak version is used. */
__attribute__((weak)) static_sym_t _veo_static_symtable[] = {
  {.n = 0, .v = 0},
};
long syscall(long n, ...);

static int gettid(void)
{
  return syscall(SYS_gettid);
}


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
  VEO_DEBUG("up=%p has received the EXIT command", (void *)up);
  urpc_generic_send(up, URPC_CMD_ACK, (char *)"");
#ifdef __ve__
  ve_urpc_fini(up);
#else
  vh_urpc_peer_destroy(up);
#endif
  if (getpid() != gettid())
    pthread_exit(0);
  else
    exit(0);
  return 0;
}

static int loadlib_handler(urpc_peer_t *up, urpc_mb_t *m, int64_t req,
                           void *payload, size_t plen)
{
  size_t psz = 0;
  char *libname = NULL;

  urpc_unpack_payload(payload, plen, (char *)"P", &libname, &psz);

  uint64_t handle = (uint64_t)dlopen(libname, RTLD_NOW);
  VEO_DEBUG("libname=%s handle=%p", libname, handle);
  if (!handle) {
    char *e = dlerror();
    VEO_ERROR("dlerror: %s", e);
  }

  int64_t new_req = urpc_generic_send(up, URPC_CMD_RESULT, (char *)"L", handle);
  CHECK_REQ(new_req, req);
  return 0;
}

static int unloadlib_handler(urpc_peer_t *up, urpc_mb_t *m, int64_t req,
                             void *payload, size_t plen)
{
  size_t psz = 0;
  uint64_t libhndl = 0;

  urpc_unpack_payload(payload, plen, (char *)"L", &libhndl);

  int rc = dlclose((void *)libhndl);

  int64_t new_req = urpc_generic_send(up, URPC_CMD_RESULT, (char *)"L", (int64_t)rc);
  CHECK_REQ(new_req, req);
  return 0;
}

static int getsym_handler(urpc_peer_t *up, urpc_mb_t *m, int64_t req,
                          void *payload, size_t plen)
{
  uint64_t libhndl = 0;
  char *sym = NULL;
  size_t psz = 0;
  uint64_t symaddr = 0;

  urpc_unpack_payload(payload, plen, (char *)"LP", &libhndl, &sym, &psz);
  if (libhndl) {
    symaddr = (uint64_t)dlsym((void *)libhndl, sym);
    if (!symaddr)
      VEO_ERROR("dlerror: %s", dlerror());
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
  CHECK_REQ(new_req, req);
  return 0;
}

static int alloc_handler(urpc_peer_t *up, urpc_mb_t *m, int64_t req,
                         void *payload, size_t plen)
{
  size_t psz = 0;
  size_t allocsz = 0;

  urpc_unpack_payload(payload, plen, (char *)"L", &allocsz);

  void *addr = malloc(allocsz);
  VEO_DEBUG("addr=%p size=%lu", addr, allocsz);

  int64_t new_req = urpc_generic_send(up, URPC_CMD_RESULT, (char *)"L", (uint64_t)addr);
  CHECK_REQ(new_req, req);
  return 0;
}

static int free_handler(urpc_peer_t *up, urpc_mb_t *m, int64_t req,
                        void *payload, size_t plen)
{
  size_t psz = 0;
  uint64_t addr = 0;

  urpc_unpack_payload(payload, plen, (char *)"L", &addr);

  free((void *)addr);
  VEO_DEBUG("addr=%p", (void *)addr);

  int64_t new_req = urpc_generic_send(up, URPC_CMD_ACK, (char *)"");
  CHECK_REQ(new_req, req);
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
  uint64_t dst = 0;
  void *buff = NULL;
  size_t buffsz = 0;

  urpc_unpack_payload(payload, plen, (char *)"LP", &dst, &buff, &buffsz);
  VEO_DEBUG("dst=%p size=%lu", (void *)dst, buffsz);

  memcpy((void *)dst, buff, buffsz);

  int64_t new_req = urpc_generic_send(up, URPC_CMD_ACK, (char *)"");
  CHECK_REQ(new_req, req);
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
  uint64_t src, dst = 0;
  size_t size = 0;

  urpc_unpack_payload(payload, plen, (char *)"LLL", &src, &dst, &size);
  VEO_DEBUG("src=%p dst=%p size=%lu", (void *)src, (void *)dst, size);

  int64_t new_req = urpc_generic_send(up, URPC_CMD_SENDBUFF, (char *)"LP",
                                      dst, (void *)src, size);
  CHECK_REQ(new_req, req);
  return 0;
}

void veo_urpc_register_handlers(urpc_peer_t *up)
{
  int err;

  if ((err = urpc_register_handler(up, URPC_CMD_PING, &ping_handler)) < 0)
    VEO_ERROR("register_handler failed for cmd %d\n", 1);
  if ((err = urpc_register_handler(up, URPC_CMD_EXIT, &exit_handler)) < 0)
    VEO_ERROR("register_handler failed for cmd %d\n", 1);
  if ((err = urpc_register_handler(up, URPC_CMD_LOADLIB, &loadlib_handler)) < 0)
    VEO_ERROR("register_handler failed for cmd %d\n", 1);
  if ((err = urpc_register_handler(up, URPC_CMD_UNLOADLIB, &unloadlib_handler)) < 0)
    VEO_ERROR("register_handler failed for cmd %d\n", 1);
  if ((err = urpc_register_handler(up, URPC_CMD_GETSYM, &getsym_handler)) < 0)
    VEO_ERROR("register_handler failed for cmd %d\n", 1);
  if ((err = urpc_register_handler(up, URPC_CMD_ALLOC, &alloc_handler)) < 0)
    VEO_ERROR("register_handler failed for cmd %d\n", 1);
  if ((err = urpc_register_handler(up, URPC_CMD_FREE, &free_handler)) < 0)
    VEO_ERROR("register_handler failed for cmd %d\n", 1);
  if ((err = urpc_register_handler(up, URPC_CMD_RECVBUFF, &recvbuff_handler)) < 0)
    VEO_ERROR("register_handler failed for cmd %d\n", 1);
  if ((err = urpc_register_handler(up, URPC_CMD_SENDBUFF, &sendbuff_handler)) < 0)
    VEO_ERROR("register_handler failed for cmd %d\n", 1);
}

__attribute__((constructor(10001)))
static void _veo_urpc_init_register(void)
{
  VEO_TRACE("registering common URPC handlers");
  urpc_set_handler_init_hook(&veo_urpc_register_handlers);
}
