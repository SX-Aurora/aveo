# Restriction

VEO requires 32 Huge Pages per VE context.

VEO does not support quadruple precision real number and variable length character strings as arguments of Fortran subroutines and functions.

VEO does not support quadruple precision real number and variable length character strings as return values of Fortran functions.

When veo_proc_create() is invoked, multiple threads for a OpenMP program are created on VE side in the default context. If you do not use OpenMP, set the environment variable VE_OMP_NUM_THREADS=1.

If using more VE contexts inside one proc, restrict the contexts to use only one OpenMP thread. Multiple contexts with multiple OpenMP threads do not work.

Synchronous APIs wait the completion of previous requests submitted by asynchronous APIs.
Synchronous APIs are below:
 - veo_alloc_mem()
 - veo_call_sync()
 - veo_context_open()
 - veo_context_open_with_attr()
 - veo_free_mem()
 - veo_get_sym()
 - veo_load_library()
 - veo_proc_destroy()
 - veo_read_mem()
 - veo_unload_library()
 - veo_write_mem()

The size of arguments passed to functions is limited to 63MB, since the size of the initial stack is 64MB. Allocate and use memory buffers on heap when you have huge argument arrays to pass.
