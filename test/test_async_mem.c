#include <stdio.h>
#include <stdlib.h>
#include <ve_offload.h>
int
main()
{
  uint64_t retval;
  int ret;

  struct veo_proc_handle *proc = veo_proc_create(0);
  if (proc == NULL) {
    perror("veo_proc_create");
    exit(1);
  }
  uint64_t handle = veo_load_library(proc, "./libveasyncmem.so");
  printf("handle = %p\n", (void *)handle);

  struct veo_thr_ctxt *ctx = veo_context_open(proc);
  printf("VEO context = %p\n", ctx);
  struct veo_args *argp = veo_args_alloc();

  uint64_t buffer = 0;
  uint64_t bufptr = veo_get_sym(proc, handle, "buffer");
  uint64_t sym = veo_get_sym(proc, handle, "print_buffer");

  uint64_t req;
  req = veo_async_read_mem(ctx, &buffer, bufptr, sizeof(buffer));
  printf("veo_async_read_mem() returned %ld\n", req);
  veo_call_wait_result(ctx, req, &retval);

  printf("%016lx\n", buffer);

  uint64_t id = veo_call_async(ctx, sym, argp);
  printf("veo_call_async() returned %ld\n", id);
  ret = veo_call_wait_result(ctx, id, &retval);
  printf("0x%lx: %d, %lu\n", id, ret, retval);

  buffer = 0xc0ffee;
  req = veo_async_write_mem(ctx, bufptr, &buffer, sizeof(buffer));
  printf("veo_async_write_mem() returned %ld\n", req);

  id = veo_call_async(ctx, sym, argp);
  printf("veo_call_async() returned %ld\n", id);
  ret = veo_call_wait_result(ctx, id, &retval);
  printf("0x%lx: %d, %lu\n", id, ret, retval);
  ret = veo_call_wait_result(ctx, req, &retval);
  printf("0x%lx: %d, %lu\n", req, ret, retval);

  int close_status = veo_context_close(ctx);
  printf("close status = %d\n", close_status);
  return 0;
}
