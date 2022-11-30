# Restrictions

VEO requires 32 Huge Pages per VE context. Make sure the system is configured with huge pages by checking `/proc/meminfo`.

VEO does not support quadruple precision real number and variable length character strings as arguments of Fortran subroutines and functions.

VEO does not support quadruple precision real number and variable length character strings as return values of Fortran functions.

VEO does not support accelerated I/O. If you set the environment variable VE_ACC_IO=1 and use VEO, accelerated I/O remains unavailable.

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

The transfer speed of veo_write_mem() or veo_async_write_mem() become slow depending on the memory location(NUMA node) of the write destination. The data transfer speed may become stably high by running the program via numactl. Please note that the optimal command option will change depending on the operating conditions of the machine and software. Execute the following command and try to validate if the transfer speed become stably high.
-# `numactl --localalloc <filename>`
-# `numactl --cpunodebind=<NUMA node> --localalloc <filename>`

The default values of tuning parameters have been changed from v2.7.5 to improve the performance of asynchronous data transfers. If you find the decrease of the performance of data transfers, please set both environment variable VEO_SENDCUT and VEO_RECVCUT to 524288, so that the behavior will be similar to the behavior of the previous version.
~~~
$ export VEO_SENDCUT=524288
$ export VEO_RECVCUT=524288
~~~

If you use FTRACE to get performance information, please do not unload shared library built with -ftrace.

VEO overwrites the environment variable VE_FTRACE_OUT_NAME when creating a VE process.
