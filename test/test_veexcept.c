#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>

#include <ve_offload.h>
#include <urpc_time.h>

int main(int argc, char *argv[])
{
  int err = 0, rc = 0;

  struct veo_proc_handle *proc;
  uint64_t libh, sym;

  proc = veo_proc_create(-1);
  printf("proc = %p\n", (void *)proc);
  if (proc == NULL) {
    rc = -1;
    goto done;
  }

  libh = veo_load_library(proc, "./libveexcept.so");
  printf("libh = %p\n", (void *)libh);
  if (libh == 0) {
    rc = -1;
    goto done;
  }
  sym = veo_get_sym(proc, libh, "hello");
  printf("'hello' sym = %p\n", (void *)sym);
  if (libh == 0) {
    rc = -1;
    goto done;
  }

  struct veo_args *argp = veo_args_alloc();
  uint64_t result = 0;
  veo_args_clear(argp);
  veo_args_set_i32(argp, 0, 42);

  rc = veo_call_sync(proc, sym, argp, &result);
  printf("call 'hello' on proc returned %ld, rc=%d\n", result, rc);
  veo_args_free(argp);

  printf("sleeping 20s\n");
  sleep(20);

 done:
  if (proc != NULL) {
    err = veo_proc_destroy(proc);
    printf("veo_proc_destroy(proc) returned %d\n", err);
  }
  return rc;
}

