DEST ?= ../install
BUILD ?= ../build

include ../make_aveo.inc

VEORUN_BIN ?= $(abspath $(DEST)/libexec/aveorun)
VERSION = $(shell cat ../VERSION)

DEFINES := -DVEORUN_BIN=\"$(VEORUN_BIN)\"
GPPFLAGS := $(GPPFLAGS) $(DEFINES)

VHLIB_OBJS := $(addprefix $(BVH)/,\
 ProcHandle.o ThreadContext.o AsyncTransfer.o Command.o CallArgs.o veo_api.o \
 veo_urpc.o veo_urpc_vh.o dummy.o)

VELIB_OBJS := $(addprefix $(BVE)/,veo_urpc.o veo_urpc_ve.o dummy.o)

LIBS = $(addprefix $(BLIB)/,libaveoVE.so libaveoVH.so libaveoVE.a libaveoVH.a)
PROGS = $(BB)/aveorun $(BB)/aveorun-ftrace $(BB)/relink_aveorun
INCLUDES := ve_offload.h


ALL: $(LIBS) $(PROGS)

.PRECIOUS: $(DEST)/ $(DEST)%/

%/:
	mkdir -p $@


install: ALL | $(DEST)/lib/ $(DEST)/libexec/ $(DEST)/include/ $(dir $(VEORUN_BIN))/
	/usr/bin/install -t $(DEST)/lib $(LIBS)
	/usr/bin/install -t $(DEST)/include $(INCLUDES)
	/usr/bin/install $(BB)/aveorun $(VEORUN_BIN)
	/usr/bin/install $(BB)/aveorun-ftrace $(VEORUN_BIN)-ftrace
	/usr/bin/install ../scripts/gen_aveorun_static_symtable $(DEST)/libexec
	/usr/bin/install $(BB)/relink_aveorun $(DEST)/libexec
	if [ -L $(DEST)/libexec/relink_aveorun ]; then rm -f $(DEST)/libexec/relink_aveorun; fi
	ln -s $(DEST)/libexec/relink_aveorun $(DEST)/libexec/mk_veorun_static


%/ProcHandle.o: ProcHandle.cpp ProcHandle.hpp VEOException.hpp veo_urpc.hpp CallArgs.hpp log.hpp
%/ThreadContext.o: ThreadContext.cpp ThreadContext.hpp VEOException.hpp veo_urpc.hpp CallArgs.hpp \
                   CommandImpl.hpp log.hpp
%/AsyncTransfer.o: AsyncTransfer.cpp ThreadContext.hpp VEOException.hpp CommandImpl.hpp log.hpp
%/CallArgs.o: CallArgs.cpp CallArgs.hpp VEOException.hpp ve_offload.h
%/veo_urpc.o: veo_urpc.c veo_urpc.hpp
%/veo_api.o: veo_api.cpp ProcHandle.hpp CallArgs.hpp VEOException.hpp log.hpp
%/log.o: log.cpp log.hpp
%/aveorun.o: aveorun.c veo_urpc.hpp
%/aveorun-ftrace.o: aveorun.c veo_urpc.hpp


.SECONDEXPANSION:

$(BLIB)/libaveoVE.so: $(VELIB_OBJS) | $$(@D)/
	$(NCPP) -Wl,-zdefs $(NCPPFLAGS) -fpic -shared -o $@ $^ \
		$(NLDFLAGS) -lurpcVE_omp -lveio -lveftrace -ldl

$(BLIB)/libaveoVH.so: $(VHLIB_OBJS) | $$(@D)/
	$(GPP) -Wl,-zdefs $(GPPFLAGS) -fpic -shared -o $@ $^ \
		$(LDFLAGS) -lurpcVH -ldl

$(BLIB)/libaveoVE.a: $(VELIB_OBJS) $(BVE)/aveorun.o | $$(@D)/
	$(NAR) rv $@ $^

$(BLIB)/libaveoVH.a: $(VHLIB_OBJS) | $$(@D)/
	$(AR) rv $@ $^


$(BB)/aveorun: $(BVE)/aveorun.o | $$(@D)/
	$(NFORT) $(NFORTFLAGS) -v -cxxlib -fopenmp -o $@ $^ $(NLDFLAGS) -L. \
		-L$(BLIB) -laveoVE -lurpcVE_omp -lveio -lpthread

$(BB)/aveorun-ftrace: $(BVE)/aveorun-ftrace.o | $$(@D)/
	$(NFORT) $(NFORTFLAGS) -ftrace -cxxlib -fopenmp -o $@ $^ $(NLDFLAGS) -L. \
		-L$(BLIB) -laveoVE -lurpcVE_omp -lveio -lpthread

$(BB)/relink_aveorun: ../scripts/relink_aveorun.in
	sed -e "s,@libexecdir@,$(DEST)/libexec,g" -e "s,@libdir@,$(DEST)/lib,g" \
		< $< > $@
	chmod 755 $@


$(BVE)/veo_urpc.o: veo_urpc.cpp
	$(NCC) $(NCCFLAGS) -fpic -DAVEO_VERSION_STRING=\"$(VERSION)\" -o $@ -c $<

$(BVH)/veo_urpc.o: veo_urpc.cpp
	$(GPP) $(GPPFLAGS) -fpic -DAVEO_VERSION_STRING=\"$(VERSION)\" -o $@ -c $<

$(BVH)/%.o: %.c | $$(@D)/
	$(GCC) $(GCCFLAGS) -fpic -o $@ -c $<

$(BVE)/%.o: %.c | $$(@D)/
	$(NCC) $(NCCFLAGS) -fpic -o $@ -c $<

$(BVH)/%.o: %.cpp | $$(@D)/
	$(GPP) $(GPPFLAGS) -fpic -o $@ -c $<

$(BVE)/%.o: %.cpp | $$(@D)/
	$(NCPP) $(NCPPFLAGS) -fpic -o $@ -c $<

$(BVE)/aveorun.o: aveorun.cpp | $$(@D)/
	$(NCC) $(NCCFLAGS) -fpic -o $@ -c $<


$(BVE)/aveorun-ftrace.o: aveorun.cpp | $$(@D)/
	$(NCC) $(NCCFLAGS) -ftrace -fpic -o $@ -c $<

clean:
	rm -f $(VHLIB_OBJS) $(VELIB_OBJS) $(VELIB_OBJS_OMP) $(LIBS) $(PROGS)
