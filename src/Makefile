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
 veo_urpc.o veo_urpc_vh.o log.o)

VELIB_OBJS := $(addprefix $(BVE)/,veo_urpc.o veo_urpc_ve.o log.o)

LIBS = $(addprefix $(BLIB)/,libveo.so libveo.a libveo.so.1 libveo.so.1.0.0)
VELIBS = $(addprefix $(BVELIB)/,libaveoVE.so.1.0.0 libaveoVE.so libaveoVE.so.1 libaveoVE.a)
PROGS = $(addprefix $(BLIBEX)/,aveorun aveorun-ftrace relink_aveorun gen_veorun_static_symtable)
INCLUDES := $(addprefix $(BINC)/,ve_offload.h veo_time.h)


ALL: $(LIBS) $(VELIBS) $(INCLUDES) $(PROGS)

.PRECIOUS: $(DEST)/ $(DEST)%/

%/:
	mkdir -p $@

install: ALL
	mkdir -p $(PREF)$(DEST)/lib64/ $(PREF)$(DEST)/lib/ $(PREF)$(DEST)/libexec/ $(PREF)$(DEST)/include/ $(PREF)$(dir $(VEORUN_BIN))/
	/usr/bin/install -t $(PREF)$(DEST)/lib64 $(LIBS)
	if [ -L $(PREF)$(DEST)/lib64/libveo.so ]; then \
		rm -f $(PREF)$(DEST)/lib64/libveo.so; \
	fi
	( cd $(PREF)$(DEST)/lib64; ln -sf libveo.so.1.0.0 libveo.so; ln -sf libveo.so.1.0.0 libveo.so.1)
	/usr/bin/install -t $(PREF)$(DEST)/lib $(VELIBS)
	if [ -L $(PREF)$(DEST)/lib/libaveoVE.so ]; then \
		rm -f $(PREF)$(DEST)/lib/libaveoVE.so; \
	fi
	( cd $(PREF)$(DEST)/lib; ln -sf libaveoVE.so.1.0.0 libaveoVE.so; ln -sf libaveoVE.so.1.0.0 libaveoVE.so.1)
	/usr/bin/install -t $(PREF)$(DEST)/include $(INCLUDES)
	/usr/bin/install $(BLIBEX)/* $(PREF)$(dir $(VEORUN_BIN))
	/usr/bin/install ../scripts/gen_veorun_static_symtable $(PREF)$(DEST)/libexec
	if [ -z $(PREF) ]; then \
		sed -e "s,@libexecdir@,$(DEST)/libexec,g" -e "s,@velibdir@,$(DEST)/lib,g" \
		< ../scripts/relink_aveorun.in > $(PREF)$(DEST)/libexec/mk_veorun_static; \
	else \
		sed -e "s,@libexecdir@,$(DEST)/libexec,g" -e "s,@velibdir@,/opt/nec/ve/lib,g" \
		< ../scripts/relink_aveorun.in > $(PREF)$(DEST)/libexec/mk_veorun_static; \
	fi
	chmod 755 $(PREF)$(DEST)/libexec/mk_veorun_static

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

$(BVELIB)/libaveoVE.so.1.0.0: $(VELIB_OBJS) | $$(@D)/
	$(NCPP) -Wl,-zdefs $(NCPPFLAGS) -fopenmp -fpic -shared \
	-Wl,-soname=libaveoVE.so.1 -o $@ $^ $(BVELIB)/liburpcVE_omp.a \
	-L/opt/nec/ve/lib64 -lveio -lveftrace -ldl
#	$(NLDFLAGS) -lveio -lveftrace -ldl

$(BVELIB)/libaveoVE.so.1: $(BVELIB)/libaveoVE.so.1.0.0
	ln -sf $< $@

$(BVELIB)/libaveoVE.so: $(BVELIB)/libaveoVE.so.1
	ln -sf $< $@

$(BLIB)/libveo.so.1.0.0: $(VHLIB_OBJS) | $$(@D)/
	$(GPP) -Wl,-zdefs $(GPPFLAGS) -fpic -shared -Wl,-soname=libveo.so.1 -o $@ $^ \
		$(BLIB)/liburpcVH.a -Wl,-rpath,/opt/nec/ve/veos/lib64 \
		-Wl,--version-script=libaveoVH.map -ldl
#		$(BLIB)/liburpcVH.a $(LDFLAGS) -Wl,--version-script=libaveoVH.map -ldl

$(BLIB)/libveo.so.1: $(BLIB)/libveo.so.1.0.0
	ln -sf $< $@

$(BLIB)/libveo.so: $(BLIB)/libveo.so.1.0.0
	ln -sf $< $@

$(BVELIB)/libaveoVE.a: $(VELIB_OBJS) $(BVE)/aveorun.o | $$(@D)/
	$(NAR) rv $@ $^

$(BLIB)/libveo.a: $(VHLIB_OBJS) | $$(@D)/
	$(AR) rv $@ $^

$(BLIBEX)/aveorun: $(BVE)/aveorun.o | $$(@D)/
	if [ -z $(PREF) ]; then \
		$(NFORT) $(NFORTFLAGS) -v -cxxlib -fopenmp -o $@ $^ -L. \
			-L$(BLIB) -L$(BVELIB) -Wl,-rpath,/opt/nec/ve/lib \
			$(NLDFLAGS) $(BVELIB)/libaveoVE.a $(BVELIB)/liburpcVE_omp.a \
			-lveio -lpthread;\
	else \
		$(NFORT) $(NFORTFLAGS) -v -cxxlib -fopenmp -o $@ $^ -L. \
			-L$(BLIB) -L$(BVELIB) -Wl,-rpath,/opt/nec/ve/lib \
			$(BVELIB)/libaveoVE.a $(BVELIB)/liburpcVE_omp.a \
			-lveio -lpthread;\
	fi

$(BLIBEX)/aveorun-ftrace: $(BVE)/aveorun-ftrace.o | $$(@D)/
	$(NFORT) $(NFORTFLAGS) -ftrace -cxxlib -fopenmp -o $@ $^ $(NLDFLAGS) -L. \
		-L$(BLIB) $(BVELIB)/libaveoVE.a $(BVELIB)/liburpcVE_omp.a -lveio -lpthread

$(BLIBEX)/relink_aveorun: ../scripts/relink_aveorun.in
	sed -e "s,@libexecdir@,$(DEST)/libexec,g" -e "s,@velibdir@,$(BUILD)/lib,g" \
		< $< > $@
	chmod 755 $@

$(BLIBEX)/gen_veorun_static_symtable: ../scripts/gen_veorun_static_symtable
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
	rm -f $(VHLIB_OBJS) $(VELIB_OBJS) $(VELIB_OBJS_OMP) $(LIBS) $(VELIBS) $(PROGS)
