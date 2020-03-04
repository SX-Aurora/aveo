//
// gcc -std=gnu99 -o test_stackargs test_stackargs.c -I/opt/nec/ve/veos/include -pthread -L/opt/nec/ve/veos/lib64 -Wl,-rpath=/opt/nec/ve/veos/lib64 -lveo
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ve_offload.h>

int main()
{
  int ret;
  long retval;

  struct veo_proc_handle *proc = veo_proc_create(-1);
  if (proc == NULL) {
    printf("veo_proc_create() failed!\n");
    exit(1);
  }
  uint64_t handle = veo_load_library(proc, "./libvestackargs.so");
  printf("handle = %p\n", (void *)handle);

  struct veo_args *arg = veo_args_alloc();
  arg = veo_args_alloc();
  veo_args_set_double(arg, 0, 1.0);
  veo_args_set_double(arg, 1, 2.0);
  veo_args_set_double(arg, 2, 3.0);
  veo_args_set_double(arg, 3, 4.0);
  veo_args_set_double(arg, 4, 5.0);
  veo_args_set_double(arg, 5, 6.0);
  veo_args_set_double(arg, 6, 7.0);
  veo_args_set_double(arg, 7, 8.0);
  veo_args_set_double(arg, 8, 9.0);
  veo_args_set_double(arg, 9, 10.0);
  uint64_t sym_test_many_args = veo_get_sym(proc, handle,
                                            "test_many_args");
  printf("symbol address (test_many_args) = %p\n",
         (void *)sym_test_many_args);
  ret = veo_call_sync(proc, sym_test_many_args, arg, &retval);

  veo_args_clear(arg);
  uint64_t sym = veo_get_sym(proc, handle, "ftest");
  printf("symbol address (ftest) = %p\n", (void *)sym);
  
  double ad = -1.876;
  char *at = "hello stack!\0";
  int ai = 19181716;
	
  veo_args_set_stack(arg, VEO_INTENT_IN, 0, (char *)&ad, sizeof(ad));
  veo_args_set_stack(arg, VEO_INTENT_IN, 1, at, strlen(at) + 1);
  veo_args_set_stack(arg, VEO_INTENT_IN, 2, (char *)&ai, sizeof(ai));
	
  ret = veo_call_sync(proc, sym, arg, &retval);
  
  veo_args_free(arg);

  printf("-----\n");
  sleep(2);

  arg = veo_args_alloc();
  veo_args_set_i32(arg, 0, -2);
  veo_args_set_u32(arg, 1, 0xa0a0a0a0);
  veo_args_set_float(arg, 2, 1.0f);
  uint64_t sym_32 = veo_get_sym(proc, handle, "test_32");

  printf("symbol address (test_32) = %p\n", sym_32);
  ret = veo_call_sync(proc, sym_32, arg, &retval);
  veo_args_free(arg);
  
  printf("-----\n");
  sleep(2);

  uint64_t sym_many_io = veo_get_sym(proc, handle, "test_many_inout");
  printf("symbol address (test_many_inout) = %p\n", (void *)sym_many_io);
  arg = veo_args_alloc();
  char in0[] = "Hello, world.";
  veo_args_set_stack(arg, VEO_INTENT_IN, 0, in0, sizeof(in0));
  int inout1 = 42;
  veo_args_set_stack(arg, VEO_INTENT_INOUT, 1, (char *)&inout1, sizeof(inout1));
  printf("VH: inout1 = %d\n", inout1);
  float out2;
  veo_args_set_stack(arg, VEO_INTENT_OUT, 2, (char *)&out2, sizeof(out2));
  veo_args_set_double(arg, 3, 1.0);
  veo_args_set_double(arg, 4, 2.0);
  veo_args_set_double(arg, 5, 3.0);
  veo_args_set_double(arg, 6, 4.0);
  veo_args_set_double(arg, 7, 5.0);
  char out8[10];
  veo_args_set_stack(arg, VEO_INTENT_OUT, 8, out8, sizeof(out8));
  veo_args_set_u32(arg, 9, sizeof(out8));
  ret = veo_call_sync(proc, sym_many_io, arg, &retval);
  veo_args_free(arg);
  printf("VH: inout1 = %d\n", inout1);
  union {
    float f;
    uint64_t x;
  } u;
  u.f = out2;
  printf("VH: out2 = %f (%#08x)\n", (double)out2, u.x);
  printf("VH: out8 = %s\n", out8);

  printf("-----\n");
  sleep(2);

  arg = veo_args_alloc();
  veo_args_set_i16(arg, 0, -2);
  veo_args_set_u16(arg, 1, 0xa0a0a0a0);
  uint64_t sym_16 = veo_get_sym(proc, handle, "test_16");
  printf("symbol address (test_16) = %p\n", sym_16);
  ret = veo_call_sync(proc, sym_16, arg, &retval);
  veo_args_free(arg);
  
  printf("-----\n");
  sleep(2);

  arg = veo_args_alloc();
  veo_args_set_i8(arg, 0, -2);
  veo_args_set_u8(arg, 1, 0xa0a0a0a0);
  uint64_t sym_8 = veo_get_sym(proc, handle, "test_8");
  printf("symbol address (test_8) = %p\n", sym_8);
  ret = veo_call_sync(proc, sym_8, arg, &retval);
  veo_args_free(arg);

  veo_proc_destroy(proc);

  return 0;
}

