# Restriction

VEO does not support to execute VEO program without setting the number of HugePages more than 32.
VEO does not support to use quadruple precision real number a variable length character string as a return value and an argument of Fortran subroutines and functions,

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

Asynchronous APIs are below:
 - veo_get_context()
 - veo_num_contexts()
