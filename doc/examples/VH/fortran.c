#include <ve_offload.h>
#include <stdio.h>
int main()
{
  /* Load "vefortran" on VE node 0 */
  struct veo_proc_handle *proc = veo_proc_create_static(0, "./vefortran");
  uint64_t handle = 0UL;/* find a function in the executable */
  struct veo_thr_ctxt *ctx = veo_context_open(proc);
  struct veo_args *argp = veo_args_alloc();
  long x = 42;
  long y;
  veo_args_set_stack(argp, VEO_INTENT_IN, 0, (char *)&x, sizeof(x));
  veo_args_set_stack(argp, VEO_INTENT_OUT, 1, (char *)&y, sizeof(y));
  uint64_t id = veo_call_async_by_name(ctx, handle, "sub1_", argp);
  uint64_t retval;
  veo_call_wait_result(ctx, id, &retval);
  printf("SUB1 outputs %lu\n", y);
  veo_args_clear(argp);
  veo_args_set_i64(argp, 0, 1);
  veo_args_set_i64(argp, 1, 2);
  id = veo_call_async_by_name(ctx, handle, "func1_", argp);
  veo_call_wait_result(ctx, id, &retval);
  printf("FUNC1 return %lu\n", retval);
  veo_args_free(argp);
  veo_context_close(ctx);
  veo_proc_destroy(proc);
  return 0;
}
