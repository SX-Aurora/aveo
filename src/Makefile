# destination path for install
DEST ?= ../install

# install prefix (when building an RPM)
PREF ?=

# build path
BUILD ?= ../build

include ../make_aveo.inc

VEORUN_BIN ?= $(abspath $(DEST)/libexec/aveorun)
VERSION = $(shell cat ../VERSION)

DEFINES := -DVEORUN_BIN=\"$(VEORUN_BIN)\"
GPPFLAGS := $(GPPFLAGS) $(DEFINES)

VHLIB_OBJS := $(addprefix $(BVH)/,\
 ProcHandle.o Context.o AsyncTransfer.o Command.o CallArgs.o veo_api.o \
 veo_urpc.o veo_urpc_vh.o)

VELIB_OBJS := $(addprefix $(BVE)/,veo_urpc.o veo_urpc_ve.o)

LIBS = $(addprefix $(BLIB)/,libaveoVE.so libaveoVH.so libaveoVE.a libaveoVH.a)
PROGS = $(addprefix $(BLIBEX)/,aveorun aveorun-ftrace relink_aveorun gen_aveorun_static_symtable)
INCLUDES := $(addprefix $(BINC)/,ve_offload.h)


ALL: $(LIBS) $(INCLUDES) $(PROGS)

.PRECIOUS: $(DEST)/ $(DEST)%/

%/:
	mkdir -p $@

install: ALL
	mkdir -p $(PREF)$(DEST)/lib/ $(PREF)$(DEST)/libexec/ $(PREF)$(DEST)/include/ $(PREF)$(dir $(VEORUN_BIN))/
	/usr/bin/install -t $(PREF)$(DEST)/lib $(LIBS)
	if [ -L $(PREF)$(DEST)/lib/libveo.so ]; then \
		rm -f $(PREF)$(DEST)/lib/libveo.so; \
	fi
	( cd $(PREF)$(DEST)/lib; ln -s libaveoVH.so libveo.so; )
	/usr/bin/install -t $(PREF)$(DEST)/include $(INCLUDES)
	/usr/bin/install $(BLIBEX)/* $(PREF)$(dir $(VEORUN_BIN))
	/usr/bin/install ../scripts/gen_aveorun_static_symtable $(PREF)$(DEST)/libexec
	sed -e "s,@libexecdir@,$(DEST)/libexec,g" -e "s,@libdir@,$(DEST)/lib,g" \
		< ../scripts/relink_aveorun.in > $(PREF)$(DEST)/libexec/relink_aveorun
	chmod 755 $(PREF)$(DEST)/libexec/relink_aveorun
	if [ -L $(PREF)$(DEST)/libexec/mk_veorun_static ]; then \
		rm -f $(PREF)$(DEST)/libexec/mk_veorun_static; \
	fi
	( cd $(PREF)$(DEST)/libexec; ln -s relink_aveorun mk_veorun_static; )


%/ProcHandle.o: ProcHandle.cpp ProcHandle.hpp VEOException.hpp veo_urpc.h CallArgs.hpp log.h
%/Context.o: Context.cpp Context.hpp VEOException.hpp veo_urpc.h CallArgs.hpp \
                   CommandImpl.hpp log.h
%/AsyncTransfer.o: AsyncTransfer.cpp Context.hpp VEOException.hpp CommandImpl.hpp log.h
%/CallArgs.o: CallArgs.cpp CallArgs.hpp VEOException.hpp ve_offload.h
%/veo_urpc.o: veo_urpc.c veo_urpc.h
%/veo_api.o: veo_api.cpp ProcHandle.hpp CallArgs.hpp VEOException.hpp log.h
%/log.o: log.cpp log.h
%/aveorun.o: aveorun.c veo_urpc.h
%/aveorun-ftrace.o: aveorun.c veo_urpc.h


.SECONDEXPANSION:


$(BINC)/%.h: %.h | $$(@D)/
	/usr/bin/install -t $(BINC) $<

$(BLIB)/libaveoVE.so: $(VELIB_OBJS) | $$(@D)/
	$(NCPP) -Wl,-zdefs $(NCPPFLAGS) -fopenmp -fpic -shared -o $@ $^ \
		$(BLIB)/liburpcVE_omp.a $(NLDFLAGS) -lveio -lveftrace -ldl

$(BLIB)/libaveoVH.so: $(VHLIB_OBJS) | $$(@D)/
	$(GPP) -Wl,-zdefs $(GPPFLAGS) -fpic -shared -o $@ $^ \
		$(BLIB)/liburpcVH.a $(LDFLAGS) -Wl,--version-script=libaveoVH.map -ldl

$(BLIB)/libaveoVE.a: $(VELIB_OBJS) $(BVE)/aveorun.o | $$(@D)/
	$(NAR) rv $@ $^

$(BLIB)/libaveoVH.a: $(VHLIB_OBJS) | $$(@D)/
	$(AR) rv $@ $^


$(BLIBEX)/aveorun: $(BVE)/aveorun.o | $$(@D)/
	$(NFORT) $(NFORTFLAGS) -v -cxxlib -fopenmp -o $@ $^ $(NLDFLAGS) -L. \
		-L$(BLIB) -laveoVE -lveio -lpthread

$(BLIBEX)/aveorun-ftrace: $(BVE)/aveorun-ftrace.o | $$(@D)/
	$(NFORT) $(NFORTFLAGS) -ftrace -cxxlib -fopenmp -o $@ $^ $(NLDFLAGS) -L. \
		-L$(BLIB) -laveoVE -lveio -lpthread

$(BLIBEX)/relink_aveorun: ../scripts/relink_aveorun.in
	sed -e "s,@libexecdir@,$(DEST)/libexec,g" -e "s,@libdir@,$(BUILD)/lib,g" \
		< $< > $@
	chmod 755 $@

$(BLIBEX)/gen_aveorun_static_symtable: ../scripts/gen_aveorun_static_symtable
	cp -p $< $@
	chmod 755 $@

$(BVE)/veo_urpc.o: veo_urpc.c
	$(NCC) $(NCCFLAGS) -fpic -DAVEO_VERSION_STRING=\"$(VERSION)\" -o $@ -c $<

$(BVH)/veo_urpc.o: veo_urpc.c
	$(GCC) $(GCCFLAGS) -fpic -DAVEO_VERSION_STRING=\"$(VERSION)\" -o $@ -c $<

$(BVH)/%.o: %.c | $$(@D)/
	$(GCC) $(GCCFLAGS) -fpic -o $@ -c $<

$(BVE)/%.o: %.c | $$(@D)/
	$(NCC) $(NCCFLAGS) -fpic -o $@ -c $<

$(BVH)/%.o: %.cpp | $$(@D)/
	$(GPP) $(GPPFLAGS) -fpic -o $@ -c $<

$(BVE)/%.o: %.cpp | $$(@D)/
	$(NCPP) $(NCPPFLAGS) -fpic -o $@ -c $<

$(BVE)/aveorun.o: aveorun.c | $$(@D)/
	$(NCC) $(NCCFLAGS) -fpic -o $@ -c $<


$(BVE)/aveorun-ftrace.o: aveorun.c | $$(@D)/
	$(NCC) $(NCCFLAGS) -ftrace -fpic -o $@ -c $<

clean:
	rm -f $(VHLIB_OBJS) $(VELIB_OBJS) $(VELIB_OBJS_OMP) $(LIBS) $(PROGS)
