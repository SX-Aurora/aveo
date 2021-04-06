#include <ve_offload.h>
int main()
{
  struct veo_proc_handle *proc = veo_proc_create(0);
  uint64_t handle = veo_load_library(proc, "./libvehello.so");
  struct veo_thr_ctxt *ctx = veo_context_open(proc);

  struct veo_args *argp = veo_args_alloc();
  uint64_t id = veo_call_async_by_name(ctx, handle, "hello", argp);
  uint64_t retval;
  veo_call_wait_result(ctx, id, &retval);
  veo_args_free(argp);
  veo_context_close(ctx);
  veo_proc_destroy(proc);
  return 0;
}
