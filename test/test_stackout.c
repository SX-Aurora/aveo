#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ve_offload.h>

int main()
{
  int ret;
  long retval;
  void *ptr = 0x00000000;
  struct veo_args *arg;

  struct veo_proc_handle *proc = veo_proc_create(-1);
  if (proc == NULL) {
    printf("veo_proc_create() failed!\n");
    exit(1);
  }
  uint64_t handle = veo_load_library(proc, "./libvestackout.so");
  printf("handle = %p\n", (void *)handle);
  uint64_t sym = veo_get_sym(proc, handle, "test_out");

#if 0
  // if this block is commented in, then the test succeeds
  // This is a reproducer for issue #6
  arg = veo_args_alloc();
  veo_args_set_i32(arg, 0, 0);
  veo_args_set_stack(arg, VEO_INTENT_INOUT, 1, &ptr, sizeof(ptr));
  ret = veo_call_sync(proc, sym, arg, &retval);
  veo_args_free(arg);
  printf("out = %p\n", ptr);
  assert(ptr == (void *)0x12345678);
#endif

  arg = veo_args_alloc();
  ptr = (void *)0x00000001;
  veo_args_set_i32(arg, 0, 1);
  veo_args_set_stack(arg, VEO_INTENT_OUT, 1, (char *)&ptr, sizeof(ptr));
  ret = veo_call_sync(proc, sym, arg, &retval);
  veo_args_free(arg);
  printf("out = %p\n", ptr);
  assert(ptr == (void *)0x12345679);

  veo_proc_destroy(proc);
  return 0;
}

