DEST ?= ../install
BUILD ?= ../build

include ../make_aveo.inc

VEORUN_BIN ?= $(abspath $(DEST)/libexec/aveorun)

DEFINES := -DVEORUN_BIN=\"$(VEORUN_BIN)\"
GPPFLAGS := $(GPPFLAGS) $(DEFINES)

VHLIB_OBJS := $(addprefix $(BVH)/,\
 ProcHandle.o ThreadContext.o AsyncTransfer.o Command.o CallArgs.o veo_api.o \
 veo_urpc.o veo_urpc_vh.o log.o)

VELIB_OBJS := $(addprefix $(BVE)/,veo_urpc.o veo_urpc_ve.o)

LIBS = $(addprefix $(BLIB)/,libaveoVE.so libaveoVH.so libaveoVE.a libaveoVH.a)
INCLUDES := ve_offload.h


ALL: $(LIBS) $(BB)/aveorun

.PRECIOUS: $(DEST)/ $(DEST)%/

%/:
	mkdir -p $@


install: $(LIBS) | $(DEST)/lib/ $(DEST)/include/ $(dir $(VEORUN_BIN))/
	cp -p $(LIBS) $(DEST)/lib
	cp -p $(INCLUDES) $(DEST)/include
	cp -p $(BB)/aveorun $(VEORUN_BIN)


%/ProcHandle.o: ProcHandle.cpp ProcHandle.hpp VEOException.hpp veo_urpc.hpp CallArgs.hpp log.hpp
%/ThreadContext.o: ThreadContext.cpp ThreadContext.hpp VEOException.hpp veo_urpc.hpp CallArgs.hpp \
                   CommandImpl.hpp log.hpp
%/AsyncTransfer.o: AsyncTransfer.cpp ThreadContext.hpp VEOException.hpp CommandImpl.hpp log.hpp
%/CallArgs.o: CallArgs.cpp CallArgs.hpp VEOException.hpp ve_offload.h
%/veo_urpc.o: veo_urpc.c veo_urpc.hpp
%/veo_api.o: veo_api.cpp ProcHandle.hpp CallArgs.hpp VEOException.hpp log.hpp
%/log.o: log.cpp log.hpp
%/aveorun.o: aveorun.c veo_urpc.h


.SECONDEXPANSION:

$(BLIB)/libaveoVE.so: $(VELIB_OBJS) | $$(@D)/
	$(NCPP) -Wl,-zdefs $(NCPPFLAGS) -fpic -shared -o $@ $^ \
		$(NLDFLAGS) -lurpcVE_omp -lveio -lveftrace -ldl

$(BLIB)/libaveoVH.so: $(VHLIB_OBJS) | $$(@D)/
	$(GPP) -Wl,-zdefs $(GPPFLAGS) -fpic -shared -o $@ $^ \
		$(LDFLAGS) -lurpcVH -llog4c -ldl

$(BLIB)/libaveoVE.a: $(VELIB_OBJS) | $$(@D)/
	$(NAR) rv $@ $^

$(BLIB)/libaveoVH.a: $(VHLIB_OBJS) | $$(@D)/
	$(AR) rv $@ $^


$(BB)/aveorun: $(BVE)/aveorun.o | $$(@D)/
	$(NFORT) $(NFORTFLAGS) -cxxlib -fopenmp -o $@ $^ $(NLDFLAGS) -L. -laveoVE -lurpcVE_omp -lveio -lpthread
#	$(NCPP) $(NCPPFLAGS) -traceback -o $@ $^ $(LDFLAGS) -L. -laveoVE -L$(BLIB) -lurpcVE -lveio -lpthread -ldl


$(BVE)/veo_urpc.o: veo_urpc.cpp
	$(NCC) $(NCCFLAGS) -fpic -o $@ -c $<

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
#	$(NCC) $(NCPPFLAGS) -traceback -fpic -o $@ -c $<

clean:
	rm -f $(VHLIB_OBJS) $(VELIB_OBJS) $(VELIB_OBJS_OMP) $(LIBS) $(BB)/aveorun
