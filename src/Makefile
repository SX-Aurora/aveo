# destination path for install
DEST ?= ../install

# install prefix (when building an RPM)
PREF ?=

# build path
BUILD ?= ../build

include ../make_aveo.inc

LIBVEIO = /opt/nec/ve/lib/libveio.so
LIBVEOHMEM = $(abspath $(VEDEST)/lib/libveohmem.so)
VEORUN_BIN ?= $(abspath $(DEST)/libexec/aveorun)
VERSION = $(shell cat ../VERSION)

DEFINES := -DVEORUN_BIN=\"$(VEORUN_BIN)\" -DLIBVEIO=\"$(LIBVEIO)\" -DLIBVEOHMEM=\"$(LIBVEOHMEM)\"
GPPFLAGS := $(GPPFLAGS) $(DEFINES)

VHLIB_OBJS := $(addprefix $(BVH)/,\
 ProcHandle.o Context.o AsyncTransfer.o Command.o CallArgs.o veo_api.o \
 veo_urpc.o veo_urpc_vh.o log.o veo_hmem.o veo_veshm.o veo_vhveshm.o \
 veo_vedma.o)

AVEORUN_OBJS := $(addprefix $(BVE)/,veo_urpc.o veo_urpc_ve.o log.o veo_hmem.o)
HMEM_OBJS := $(addprefix $(BVE)/,veo_hmem.o)
VELIB_OBJS := $(AVEORUN_OBJS) $(HMEM_OBJS) 

LIBS = $(addprefix $(BLIB)/,libveo.so libveo.a libveo.so.1 libveo.so.1.0.0)
ARCS = $(addprefix $(BLIB)/,libveo.a)
VELIBS = $(addprefix $(BVELIB)/,libaveoVE.so.1.0.0 libaveoVE.so libaveoVE.so.1 \
	libveohmem.so.1.0.0 libveohmem.so libveohmem.so.1)
VEARCS = $(addprefix $(BVELIB)/,libaveoVE.a)
PROGS = $(addprefix $(BLIBEX)/,aveorun aveorun-ftrace relink_aveorun mk_veorun_static gen_veorun_static_symtable)
INCLUDES := $(addprefix $(BINC)/,ve_offload.h veo_time.h veo_hmem.h \
	veo_veshm.h veo_vhveshm.h veo_vedma.h veo_veshm_defs.h)
VEINCLUDES := $(addprefix $(BVEINC)/,veo_hmem.h)

ALL: all-ve all-vh

all-ve: $(VEINCLUDES) $(VELIBS) $(VEARCS) $(PROGS)
all-vh: $(INCLUDES) $(LIBS) $(ARCS)

.PRECIOUS: $(DEST)/ $(DEST)%/

%/:
	mkdir -p $@

install: install-ve install-vh

install-ve: all-ve
	mkdir -p $(PREF)$(VEDEST)/lib/ $(PREF)$(dir $(VEORUN_BIN))/ $(PREF)$(DEST)/libexec/ $(PREF)$(VEDEST)/bin/ $(PREF)$(VEDEST)/include
	/usr/bin/install -t $(PREF)$(VEDEST)/lib $(VELIBS)
	( cd $(PREF)$(VEDEST)/lib; ln -sf libaveoVE.so.1.0.0 libaveoVE.so; ln -sf libaveoVE.so.1.0.0 libaveoVE.so.1; \
	ln -sf libveohmem.so.1.0.0 libveohmem.so;ln -sf libveohmem.so.1.0.0 libveohmem.so.1;)
	/usr/bin/install -t $(PREF)$(VEDEST)/lib -m 0644 $(VEARCS)
	/usr/bin/cp -P $(BLIBEX)/* $(PREF)$(dir $(VEORUN_BIN))
	ln -sf $(DEST)/libexec/mk_veorun_static $(PREF)$(VEDEST)/bin/mk_veorun_static
	ln -sf $(DEST)/libexec/relink_aveorun $(PREF)$(VEDEST)/bin/relink_aveorun
	/usr/bin/install -t $(PREF)$(VEDEST)/include -m 0644 $(VEINCLUDES)

install-vh: all-vh
	mkdir -p $(PREF)$(DEST)/lib64/ $(PREF)$(DEST)/include/
	/usr/bin/install -t $(PREF)$(DEST)/lib64 $(LIBS)
	( cd $(PREF)$(DEST)/lib64; ln -sf libveo.so.1.0.0 libveo.so; ln -sf libveo.so.1.0.0 libveo.so.1)
	/usr/bin/install -t $(PREF)$(DEST)/lib64 -m 0644 $(ARCS)
	/usr/bin/install -t $(PREF)$(DEST)/include -m 0644 $(INCLUDES)

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
%/veo_hmem.o: veo_hmem.cpp veo_hmem.h veo_hmem_macros.h
%/veo_veshm.o: veo_veshm.cpp veo_veshm.h veo_veshm_defs.h veo_sysve.h veo_veos_defs.h veo_api.hpp
%/veo_vhveshm.o: veo_vhveshm.cpp veo_vhveshm.h veo_veshm_defs.h veo_sysve.h veo_veos_defs.h veo_vhshm_defs.h veo_api.hpp
%/veo_vedma.o: veo_vedma.cpp veo_vedma.h veo_veshm_defs.h veo_sysve.h veo_veos_defs.h veo_api.hpp

.SECONDEXPANSION:


$(BINC)/%.h: %.h | $$(@D)/
	/usr/bin/install -t $(BINC) $<

$(BVEINC)/%.h: %.h | $$(@D)/
	/usr/bin/install -t $(BVEINC) $<

$(BVELIB)/libaveoVE.so.1.0.0: $(AVEORUN_OBJS) | $$(@D)/
	$(NCPP) -Wl,-zdefs $(NCPPFLAGS) -fopenmp -fpic -shared \
	-Wl,-soname=libaveoVE.so.1 -o $@ $^ $(BVELIB)/liburpcVE_omp.a \
	$(NLDFLAGS) -lveio -lveftrace -ldl

$(BVELIB)/libaveoVE.so.1: $(BVELIB)/libaveoVE.so.1.0.0
	ln -sf $< $@

$(BVELIB)/libaveoVE.so: $(BVELIB)/libaveoVE.so.1
	ln -sf $< $@

$(BVELIB)/libveohmem.so.1.0.0: $(HMEM_OBJS) | $$(@D)/
	$(NCPP) -Wl,-zdefs $(NCPPFLAGS) -fpic -shared \
	-Wl,-soname=libveohmem.so.1 -o $@ $^ \
	$(NLDFLAGS) -Wl,-z,max-page-size=0x200000

$(BVELIB)/libveohmem.so.1: $(BVELIB)/libveohmem.so.1.0.0
	ln -sf $< $@

$(BVELIB)/libveohmem.so: $(BVELIB)/libveohmem.so.1
	ln -sf $< $@

$(BLIB)/libveo.so.1.0.0: $(VHLIB_OBJS) | $$(@D)/
	$(GPP) -Wl,-zdefs $(GPPFLAGS) -fpic -shared -Wl,-soname=libveo.so.1 -o $@ $^ \
		$(BLIB)/liburpcVH.a $(LDFLAGS) -Wl,--version-script=libaveoVH.map -ldl

$(BLIB)/libveo.so.1: $(BLIB)/libveo.so.1.0.0
	ln -sf $< $@

$(BLIB)/libveo.so: $(BLIB)/libveo.so.1.0.0
	ln -sf $< $@

$(BVELIB)/libaveoVE.a: $(AVEORUN_OBJS) $(BVE)/aveorun.o | $$(@D)/
	$(NAR) rv $@ $^

$(BLIB)/libveo.a: $(VHLIB_OBJS) | $$(@D)/
	$(AR) rv $@ $^

$(BLIBEX)/aveorun: $(BVE)/aveorun.o $(BVELIB)/libaveoVE.a $(BVELIB)/liburpcVE_omp.a | $$(@D)/
	$(NFORT) $(NFORTFLAGS) -v -cxxlib -fopenmp -o $@ $^ \
		$(NLDFLAGS) $(BVELIB)/libaveoVE.a $(BVELIB)/liburpcVE_omp.a \
		-lveio -pthread

$(BLIBEX)/aveorun-ftrace: $(BVE)/aveorun-ftrace.o $(BVELIB)/libaveoVE.a $(BVELIB)/liburpcVE_omp.a | $$(@D)/
	$(NFORT) $(NFORTFLAGS) -ftrace -cxxlib -fopenmp -o $@ $^ \
		$(NLDFLAGS) $(BVELIB)/libaveoVE.a $(BVELIB)/liburpcVE_omp.a \
		-lveio -pthread

$(BLIBEX)/relink_aveorun: ../scripts/relink_aveorun.in
	sed -e "s,@libexecdir@,$(DEST)/libexec,g" -e "s,@velibdir@,$(VEDEST)/lib,g" \
		< $< > $@
	chmod 755 $@

$(BLIBEX)/mk_veorun_static: $(BLIBEX)/relink_aveorun
	ln -sf ./relink_aveorun $@

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
	rm -f $(VHLIB_OBJS) $(VELIB_OBJS) $(VELIB_OBJS_OMP) $(LIBS) $(LIBS) \
		$(VELIBS) $(VEARCS) $(PROGS) $(INCLUDES) $(VEINCLUDES) \
		$(BVE)/aveorun.o $(BVE)/aveorun-ftrace.o 
