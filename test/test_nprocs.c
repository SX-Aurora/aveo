#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>

#include <ve_offload.h>
#include <urpc_time.h>

int main(int argc, char *argv[])
{
  int err = 0, rc = 0;
  const int nprocs = 4;

  struct veo_proc_handle *proc[nprocs];
  uint64_t libh[nprocs], sym[nprocs];

  for (int i = 0; i < nprocs; i++) {
    proc[i] = veo_proc_create(i % 2);
    printf("proc%d = %p\n", i, (void *)proc[i]);
    if (proc[i] == NULL) {
      rc = -1;
      goto done;
    }

    libh[i] = veo_load_library(proc[i], "./libvehello.so");
    printf("libh%d = %p\n", i, (void *)libh[i]);
    if (libh[i] == 0) {
      rc = -1;
      goto done;
    }
    sym[i] = veo_get_sym(proc[i], libh[i], "hello");
    printf("'hello' sym%d = %p\n", i, (void *)sym[i]);
    printf("libh%d = %p\n", i, (void *)libh[i]);
    if (libh[i] == 0) {
      rc = -1;
      goto done;
    }
  }

  struct veo_args *argp = veo_args_alloc();
  for (int i = 0; i < nprocs; i++) {
    uint64_t result = 0;
    veo_args_clear(argp);
    veo_args_set_i32(argp, 0, 42);

    int rc = veo_call_sync(proc[i], sym[i], argp, &result);
    printf("call 'hello' on proc%d returned %ld, rc=%d\n", i, result, rc);
  }
  veo_args_free(argp);

  printf("sleeping 30s\n");
  sleep(30);

 done:
  for (int i = 0; i < nprocs; i++) {
    if (proc[i] != NULL) {
      err = veo_proc_destroy(proc[i]);
      printf("veo_proc_destroy(proc%d) returned %d\n", i, err);
    }
  }
  return rc;
}

