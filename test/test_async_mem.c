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
  //printf("handle = %p\n", (void *)handle);

  struct veo_thr_ctxt *ctx = veo_context_open(proc);
  //printf("VEO context = %p\n", ctx);
  struct veo_args *argp = veo_args_alloc();

  uint64_t buffer = 0;
  uint64_t bufptr = veo_get_sym(proc, handle, "buffer");
  uint64_t sym = veo_get_sym(proc, handle, "print_buffer");

  uint64_t req;
  req = veo_async_read_mem(ctx, &buffer, bufptr, sizeof(buffer));
  //printf("veo_async_read_mem() returned %ld\n", req);
  veo_call_wait_result(ctx, req, &retval);

  printf("%016lx\n", buffer);
  if (buffer != 0xdeadbeefdeadbeef) {
    printf("Unexpected buffer content!\n");
    return 1;
  }

  uint64_t id = veo_call_async(ctx, sym, argp);
  //printf("veo_call_async() returned %ld\n", id);
  ret = veo_call_wait_result(ctx, id, &retval);
  //printf("0x%lx: %d, %lu\n", id, ret, retval);
  if (ret) {
    printf("Failed to execute VE kernel function. rc=%d\n", ret);
    return 2;
  }

  printf("Writing from/to unaligned address...\n");
  buffer = 0x0403020100c0ffee;
  req = veo_async_write_mem(ctx, bufptr + 1, (char *)(&buffer) + 1, 5);
  ret = veo_call_wait_result(ctx, req, &retval);

  req = veo_async_read_mem(ctx, &buffer, bufptr, sizeof(buffer));
  veo_call_wait_result(ctx, req, &retval);

  printf("unaligned write/read of 5 bytes: %016lx\n", buffer);
  if (buffer != 0xdead020100c0ffef) {
    printf("Unexpected buffer content!\n");
    return 3;
  }

  int close_status = veo_context_close(ctx);
  //printf("close status = %d\n", close_status);
  return 0;
}
