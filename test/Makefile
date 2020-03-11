DEST ?= ../install
BUILD ?= ../build

include ../make_aveo.inc

NCCFLAGS := $(NCCFLAGS) -I../src
GCCFLAGS := $(GCCFLAGS) -I../src

TESTS = $(addprefix $(BB)/,test_callsync test_callasync test_stackargs \
 test_nprocs test_veexcept bandwidth latency call_latency test_getsym \
 test_child1 test_child2 test_async_mem test_thread_main_call_race \
 test_2ctx_callasync test_omp test_omp_static test_arith_ftrace \
 test_unloadlib )

VELIBS = $(addprefix $(BB)/,libvehello.so libvehello2.so \
 libvestackargs.so libveexcept.so libveasyncmem.so libvetestomp.so \
 libvearith_ftrace.so)

STATICS = $(addprefix $(BB)/, veorun_testomp)

SCRIPTS = $(addprefix $(BB)/,scan_bandwidth.sh scan_call_latency.sh \
 run_tests.sh)

ALL: $(TESTS) $(VELIBS) $(STATICS) $(SCRIPTS)

.PHONY: install tests clean

#
# install
#
install: $(TESTS) $(VELIBS) $(STATICS) | $(DEST)/tests/
	/usr/bin/install $(TESTS) $(VELIBS) $(STATICS) $(DEST)/tests
	/usr/bin/install $(SCRIPTS) $(DEST)/tests

#
# run the tests from the build directory
#
test: ALL
	cd $(BB); ./run_tests.sh
#

.PRECIOUS: $(BUILD)/ $(BUILD)%/ $(DEST)/ $(DEST)%/

%/:
	mkdir -p $@

.SECONDEXPANSION:

$(BB)/%: %.c  | $$(@D)/
	$(GCC) $(GCCFLAGS) -fpic $(LDFLAGS) -o $@ $< -L$(BLIB) -laveoVH

$(BB)/%.so: $(BVE)/%.o  | $$(@D)/
	$(NCC) $(NCCFLAGS) -shared -fpic $(LDFLAGS) -o $@ $< -lveftrace

$(BB)/%_ftrace.so: $(BVE)/%_ftrace.o  | $$(@D)/
	$(NCC) $(NCCFLAGS) -ftrace -shared -fpic $(LDFLAGS) -o $@ $< -lveftrace

$(BB)/veorun_%: $(BVE)/libve%.o | $$(@D)/
	env RELINKVEO_GEN=../scripts/gen_aveorun_static_symtable \
	    RELINKVEO_LIB=$(BLIB)/libaveoVE.a \
	    $(BB)/relink_aveorun $@ -fopenmp $<

$(BVE)/%.o: %.c | $$(@D)/
	$(NCC) $(NCCFLAGS) -fopenmp -fpic -o $@ -c $<

$(BVE)/%_ftrace.o: %_ftrace.c | $$(@D)/
	$(NCC) $(NCCFLAGS) -ftrace -fopenmp -fpic -o $@ -c $<

$(BB)/%.sh: %.sh | $$(@D)/
	cp -p $< $@

clean:
	rm -f $(TESTS)
