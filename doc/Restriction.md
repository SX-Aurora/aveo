# Restriction

VEO requires 32 Huge Pages per a context.

VEO does not support a quadruple precision real number and a variable length character string as an argument of Fortran subroutines and functions.

VEO does not support a quadruple precision real number and a variable length character string as a return value of Fortran functions.

When veo_proc_create() is invoked, multiple threads for a OpenMP program are created on VE side by default. If you do not use OpenMP, set environment variable OMP_NUM_THREADS=1.

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
