# AVEO

This is a re-implementation of the VEO API that uses a completely
different communication method between vector host (VH) and vector
engine (VE) than the original
[veoffload](https://github.com/veos-sxarr-NEC/veoffload) project. Instead of the system call semantics for controlling VE processes AVEO uses a little RPC layer built on top of User DMA communication mechanisms: [ve-urpc](https://github.com/SX-Aurora/ve-urpc).

This project aims at tackling and solving some of the issues of the original VEO:
* Call latency: AVEO's asynchronous call latency is down from ~100us to the range of 5.5-6us.
* Memory transfer bandwidth is much increased and benefits for small transfers also of the latency improvement.
* The VH and VE processes are now decoupled, VEOS-wise the VE side is a separate process. This solves following long-standing issues:
  * Debugging of VE and VH side is now possible at the same time. Just attach a gdb to each side, set breakpoints and have fun.
  * In AVEO using multiple VEs from one process is no problem. Thus creating multiple procs is possible.
  * Performance analysis of the VE side with **ftrace** is working and straight forward.
  * There are no additional worker threads competing for resources on the VE, therefore OpenMP scheduling on VE is simple and less conflict-prone.

There is a drawback for the reduced latency: it is obtained by polling
in tight loops, therefore the VE core running the `aveorun` tool will
be continuously spinning. Also the VH cores actively waiting for the
request to finish (by `veo_call_wait_result()` will be spinning and
using power.

**NOTE:** This code is being actively developed, so is rough around the edges. Take it as a working beta. Please report issues in github!


## Installation

Make sure you can connect to github.com. Then

```
git clone https://github.com/SX-Aurora/aveo.git

cd aveo

make

make install
```

The installation goes into the `install/` subdirectory. If you decide
for another installation path, specify it with the `DEST`
variable. This needs to be done for the **make** and **make install
step!:
```
make DEST=<install_path>

make install DEST=<install_path>
```
or just
```
make install DEST=<install_path>
```



## Testing

A set of tests and examples are in the `test/` subdirectory. By **make
install** their binaries get installed to `<install_path>/tests/`.


## Usage

Usage is similar to the original VEO. The relevant include file is
```
#include <ve_offload.h>
```
therefore point your compiler to the place where it is located by adding the option
```
-I<install_path>/include
```

Instead of linking with `-lveo` you now need to link your program with
```
-L<install_path>/lib -laveoVH
```

The VE helper tool is renamed from `veorun` to `aveorun` but pointing
to it when it lives in a non-standard location still works by setting
the same environment variable:
```
export VEORUN_BIN=<path_to_aveorun>
```



## API extensions

AVEO aims at implementing fully the [VEO
API](https://veos-sxarr-nec.github.io/veoffload/index.html). Use that
as a reference.

But AVEO also brings the opportunity to extend the API and test new
things. The following new offloading functionality is only available
in AVEO:


### Specify the core of a proc

The VE core on which a proc shall run can be specified by setting the environment variable
```
export VE_CORE_NUMBER=<core>
```
The proc will run the `aveorun` helper on that core.


### Find contexts of a proc

A *ProcHandle* instance can have multiple contexts. You should not use
more contexts than cores on a VE.

The number of contexts opened on a *proc* can be determined by the function
```
int veo_num_contexts(struct veo_proc_handle *);
```

Pointers to context structures can be retrieved by calling:
```
struct veo_thr_ctxt *veo_get_context(struct veo_proc_handle *proc, int idx);
```
with `idx` taking values between 0 and the result of
`veo_num_contexts()`.  The context with `idx=0` is the main context of
the *ProcHandle*. It will not be destroyed when closed, instead it is
destroyed when the *proc* is killed by `veo_proc_destroy()`, or when
the program ends.


### Context synchronization

The call
```
void veo_context_sync(veo_thr_ctxt *ctx);
```
will return when all requests in the passed context queue have been
processed. Note that their results still need to be picked up by
either `veo_call_peek_result()` or `veo_call_wait_result()`!


### Synchronous kernel call

A synchronous kernel call is a shortcut of an async call + wait. In
AVEO it is implemented differently from an async call and is being
executed on a *proc* instead of a context. You can use this without
actually having opened any context.
```
int veo_call_sync(struct veo_proc_handle *h, uint64_t addr,
                  struct veo_args *ca, uint64_t *result);
```


