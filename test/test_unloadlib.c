#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <assert.h>
#include <ve_offload.h>

int main(int argc, char *argv[])
{
  int err = 0;

  struct veo_proc_handle *proc;
  struct veo_thr_ctxt *ctx;

  proc = veo_proc_create(-1);
  assert(proc != NULL);
  uint64_t libh = veo_load_library(proc, "./libvehello.so");
  assert(libh != 0);
  uint64_t sym = veo_get_sym(proc, libh, "print_buffer");
  assert(sym > 0);

  err = veo_unload_library(proc, libh);
  assert(err == 0);
  libh = veo_load_library(proc, "./libvehello2.so");
  assert(libh != 0);
  
  uint64_t sym2 = veo_get_sym(proc, libh, "print_buffer");
  assert(sym2 > 0);
  printf("the addresses of the same-name functions from different "
         "libraries: %lx, %lx\n", sym, sym2);
  assert(sym != sym2);

  assert(veo_proc_destroy(proc) == 0);
  return 0;
}
