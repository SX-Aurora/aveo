DEST ?= ../install
BUILD ?= ../build

include ../make_aveo.inc

NCCFLAGS := $(NCCFLAGS) -I../src
GCCFLAGS := $(GCCFLAGS) -I../src

TESTS = $(addprefix $(BB)/,test_callsync test_callasync test_stackargs test_nprocs \
 test_veexcept bandwidth latency call_latency test_getsym test_child1 test_child2 \
 test_async_mem test_thread_main_call_race test_2ctx_callasync test_omp \
 libvehello.so libvehello2.so libvestackargs.so libveexcept.so libveasyncmem.so \
 libvetestomp.so )


ALL: $(TESTS)


# install

install: $(TESTS) | $(DEST)/tests/
	cp -p $(TESTS) $(DEST)/tests


.PRECIOUS: $(BUILD)/ $(BUILD)%/ $(DEST)/ $(DEST)%/

%/:
	mkdir -p $@

.SECONDEXPANSION:

$(BB)/%.so: %.c  | $$(@D)/
	$(NCC) $(NCCFLAGS) -shared -fpic $(LDFLAGS) -o $@ $< -lveftrace

$(BB)/%: %.c  | $$(@D)/
	$(GCC) $(GCCFLAGS) -fpic $(LDFLAGS) -o $@ $< -L$(BLIB) -laveoVH -lurpcVH


clean:
	rm -f $(TESTS)
