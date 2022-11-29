CWD = $(shell pwd)
DEST ?= $(CWD)/../install
BUILD ?= $(CWD)/../build

ifeq ($(URPC_INST_DIR),)
PREPARE := ve-urpc
PREPAREINSTALL := ve-urpc-install
URPC_INST_DIR := $(DEST)
endif

include ../make_aveo.inc

NCCFLAGS := $(NCCFLAGS) -I../src
GCCFLAGS := $(GCCFLAGS) -I../src
GPPFLAGS := $(GPPFLAGS) -I../src

TESTS = $(addprefix $(BB)/,test_callsync test_callasync test_stackargs \
 test_nprocs test_veexcept bandwidth latency call_latency test_getsym\
 test_child1 test_child2 test_async_mem test_thread_main_call_race \
 test_2ctx_callasync test_omp test_omp_static test_arith_ftrace \
 test_stackout test_unloadlib test_noproc test_memtransfers test_multithread_alloc_write_read_free \
 test_veexcept_async bandwidth_stackargs bandwidth_async test_omp_2ctx \
 test_hmem test_alloc_hook test_alloc_async_hook test_prev_res test_multithread_req_block \
 test_alloc_hook_dummy test_alloc_async_hook_dummy test_alloc_hmem_hook_dummy \
 test_alloc_async test_alloc_hmem_hook test_async_hook_args test_hook_args)

VELIBS = $(addprefix $(BB)/,libvehello.so libvehello2.so \
 libvestackargs.so libveexcept.so libveasyncmem.so libvetestomp.so \
 libvestackout.so libvearith_ftrace.so libvehmem.so libvealloc.so)

STATICS = $(addprefix $(BB)/, veorun_testomp)

SCRIPTS = $(addprefix $(BB)/,scan_bandwidth.sh scan_bandwidth_async.sh \
 scan_call_latency.sh run_tests.sh scan_bandwidth_stackargs.sh)


ALL: $(TESTS) $(VELIBS) $(STATICS) $(SCRIPTS)

.PHONY: install tests clean

#
# install
#
install: $(TESTS) $(VELIBS) $(STATICS) | $(PREF)$(DEST)/tests/
	/usr/bin/install $(TESTS) $(VELIBS) $(STATICS) $(PREF)$(DEST)/tests
	/usr/bin/install $(SCRIPTS) $(PREF)$(DEST)/tests

#
# run the tests from the build directory
#
test: ALL
	cd $(BB); \
	env VEORUN_BIN=$(BLIBEX)/aveorun \
	LD_LIBRARY_PATH=$(BLIB):$(LD_LIBRARY_PATH) \
	./run_tests.sh
#

.PRECIOUS: $(BUILD)/ $(BUILD)%/ $(DEST)/ $(DEST)%/

%/:
	mkdir -p $@

.SECONDEXPANSION:

$(BB)/%: %.c  | $$(@D)/
	$(GCC) $(GCCFLAGS) -fpic $(LDFLAGS) -o $@ $< -lveo
$(BB)/%: %.cpp  | $$(@D)/
	$(GPP) $(GPPFLAGS) -fpic $(LDFLAGS) -o $@ $< -lveo

$(BB)/%.so: $(BVE)/%.o  | $$(@D)/
	$(NCC) $(NCCFLAGS) -shared -fpic $(NLDFLAGS) -o $@ $< -lveftrace -lveohmem -laveoVE

$(BB)/%_ftrace.so: $(BVE)/%_ftrace.o  | $$(@D)/
	$(NCC) $(NCCFLAGS) -ftrace -shared -fpic $(NLDFLAGS) -o $@ $< -lveftrace

$(BB)/veorun_%: $(BVE)/libve%.o | $$(@D)/
	if [ -x $(BLIBEX)/relink_aveorun ]; then \
		env VELIBEXECDIR=$(BLIBEX) \
		    VELIBDIR=$(BVELIB) \
		    $(BLIBEX)/relink_aveorun $@ -fopenmp $< ; \
	else \
		$(DEST)/libexec/mk_veorun_static $@ -fopenmp $< ; \
	fi


$(BVE)/%.o: %.c | $$(@D)/
	$(NCC) $(NCCFLAGS) -fopenmp -fpic -o $@ -c $<

$(BVE)/%_ftrace.o: %_ftrace.c | $$(@D)/
	$(NCC) $(NCCFLAGS) -ftrace -fopenmp -fpic -o $@ -c $<

$(BB)/%.sh: %.sh | $$(@D)/
	cp -p $< $@

clean:
	rm -f $(TESTS) $(TESTS) $(VELIBS) $(STATICS) $(SCRIPTS)
