# destination path for install
DEST ?= ../install

# install prefix (when building an RPM)
PREF ?=

# build path
BUILD ?= ../build

include ../make_aveo.inc

LIBVEIO = libveio.so
LIBVEOHMEM = libveohmem.so
ifeq ($(VE_ARCH),ve3)
	VEORUN_BIN ?= $(abspath $(DEST)/libexec/aveorun_ve3)
else
	VEORUN_BIN ?= $(abspath $(DEST)/libexec/aveorun_ve1)
endif

VEORUN_BIN_VE1 = $(abspath $(DEST)/libexec/aveorun_ve1)
VEORUN_BIN_VE3 = $(abspath $(DEST)/libexec/aveorun_ve3)
VERSION = $(shell cat ../VERSION)

DEFINES_VE1 := -DVEORUN_BIN_VE1=\"$(VEORUN_BIN_VE1)\"
DEFINES_VE3 := -DVEORUN_BIN_VE3=\"$(VEORUN_BIN_VE3)\"
DEFINES_LIBS := -DLIBVEIO=\"$(LIBVEIO)\" -DLIBVEOHMEM=\"$(LIBVEOHMEM)\"
GPPFLAGS := $(GPPFLAGS) $(DEFINES_VE1) $(DEFINES_VE3) $(DEFINES_LIBS)

VHLIB_OBJS := $(addprefix $(BVH)/,\
 ProcHandle.o Context.o AsyncTransfer.o Command.o CallArgs.o veo_api.o \
 veo_urpc.o veo_urpc_vh.o log.o veo_hmem.o veo_veshm.o veo_vhveshm.o \
 veo_vedma.o veo_get_arch_info.o)

AVEORUN_OBJS := $(addprefix $(BVE)/,veo_urpc.o veo_urpc_ve.o log.o veo_hmem.o)
HMEM_OBJS := $(addprefix $(BVE)/,veo_hmem.o)
VELIB_OBJS := $(AVEORUN_OBJS) $(HMEM_OBJS) 

LIBS = $(addprefix $(BLIB)/,libveo.so libveo.a libveo.so.1 libveo.so.1.0.0)
ARCS = $(addprefix $(BLIB)/,libveo.a)
VELIBS = $(addprefix $(BVELIB)/,libaveoVE.so.1.0.0 libaveoVE.so libaveoVE.so.1 \
	libveohmem.so.1.0.0 libveohmem.so libveohmem.so.1)
VEARCS = $(addprefix $(BVELIB)/,libaveoVE.a)

PROGS = $(addprefix $(BLIBEX)/,aveorun_$(VE_ARCH) aveorun-ftrace_$(VE_ARCH) relink_aveorun_$(VE_ARCH) mk_veorun_static_$(VE_ARCH))

SCRIPTS = $(addprefix $(BB)/,relink_aveorun mk_veorun_static)

PYSCRIPTS = $(addprefix $(BLIBEX)/,gen_veorun_static_symtable)

INCLUDES := $(addprefix $(BINC)/,ve_offload.h veo_time.h veo_hmem.h \
	veo_veshm.h veo_vhveshm.h veo_vedma.h veo_veshm_defs.h)
VEINCLUDES := $(addprefix $(BVEINC)/,veo_hmem.h veo_ve.h)

ALL: all-ve all-vh

all-ve: $(VEINCLUDES) $(VELIBS) $(VEARCS) $(PROGS)
all-vh: $(INCLUDES) $(LIBS) $(ARCS) $(SCRIPTS) $(PYSCRIPTS)

.PRECIOUS: $(DEST)/ $(DEST)%/

%/:
	mkdir -p $@

install: install-ve install-vh

install-ve: all-ve
	mkdir -p $(PREF)$(VEDEST)/lib/ $(PREF)$(dir $(VEORUN_BIN))/ $(PREF)$(DEST)/libexec/ $(PREF)$(VEDEST_COM)/bin/ $(PREF)$(VEDEST)/include
	/usr/bin/install -t $(PREF)$(VEDEST)/lib $(VELIBS)
	( cd $(PREF)$(VEDEST)/lib; ln -sf libaveoVE.so.1.0.0 libaveoVE.so; ln -sf libaveoVE.so.1.0.0 libaveoVE.so.1; \
	ln -sf libveohmem.so.1.0.0 libveohmem.so;ln -sf libveohmem.so.1.0.0 libveohmem.so.1;)
	/usr/bin/install -t $(PREF)$(VEDEST)/lib -m 0644 $(VEARCS)
	/usr/bin/cp -P $(BLIBEX)/* $(PREF)$(dir $(VEORUN_BIN))
	ln -sf $(DEST)/libexec/mk_veorun_static_$(VE_ARCH) $(PREF)$(VEDEST_COM)/bin/mk_veorun_static_$(VE_ARCH)
	ln -sf $(DEST)/libexec/relink_aveorun_$(VE_ARCH) $(PREF)$(VEDEST_COM)/bin/relink_aveorun_$(VE_ARCH)
	/usr/bin/install -t $(PREF)$(VEDEST)/include -m 0644 $(VEINCLUDES)

install-vh: all-vh
	mkdir -p $(PREF)$(DEST)/lib64/ $(PREF)$(DEST)/include/ $(PREF)$(VEDEST_COM)/bin $(PREF)$(DEST)/libexec/
	/usr/bin/install -t $(PREF)$(DEST)/lib64 $(LIBS)
	( cd $(PREF)$(DEST)/lib64; ln -sf libveo.so.1.0.0 libveo.so; ln -sf libveo.so.1.0.0 libveo.so.1)
	/usr/bin/install -t $(PREF)$(DEST)/lib64 -m 0644 $(ARCS)
	/usr/bin/install -t $(PREF)$(DEST)/include -m 0644 $(INCLUDES)
	/usr/bin/install -t $(PREF)$(VEDEST_COM)/bin -m 0755 $(SCRIPTS)
	ln -sf $(VEDEST_COM)/bin/relink_aveorun $(PREF)$(VEDEST_COM)/bin/mk_veorun_static
	/usr/bin/install $(BLIBEX)/gen_veorun_static_symtable $(PREF)$(dir $(VEORUN_BIN)) -m 0755

%/ProcHandle.o: ProcHandle.cpp ProcHandle.hpp VEOException.hpp veo_urpc.h CallArgs.hpp log.h
%/Context.o: Context.cpp Context.hpp VEOException.hpp veo_urpc.h CallArgs.hpp \
                   CommandImpl.hpp log.h
%/AsyncTransfer.o: AsyncTransfer.cpp Context.hpp VEOException.hpp CommandImpl.hpp log.h
%/CallArgs.o: CallArgs.cpp CallArgs.hpp VEOException.hpp ve_offload.h
%/veo_urpc.o: veo_urpc.c veo_urpc.h
%/veo_api.o: veo_api.cpp ProcHandle.hpp CallArgs.hpp VEOException.hpp log.h
%/veo_get_arch_info.o: veo_get_arch_info.cpp veo_get_arch_info.h
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

$(BB)/relink_aveorun: ../scripts/relink_aveorun
	mkdir -p $(BB)
	cp -P ../scripts/relink_aveorun $(BB)/
	chmod 755 $@


$(BB)/mk_veorun_static: $(BB)/relink_aveorun
	ln -sf $< $@

$(BLIBEX)/aveorun_$(VE_ARCH): $(BVE)/aveorun.o $(BVELIB)/libaveoVE.a $(BVELIB)/liburpcVE_omp.a | $$(@D)/
	$(NFORT) $(NFORTFLAGS) -v -cxxlib -fopenmp -o $@ $^ \
		$(NLDFLAGS) $(BVELIB)/libaveoVE.a $(BVELIB)/liburpcVE_omp.a \
		-lveio -pthread

$(BLIBEX)/aveorun-ftrace_$(VE_ARCH): $(BVE)/aveorun-ftrace.o $(BVELIB)/libaveoVE.a $(BVELIB)/liburpcVE_omp.a | $$(@D)/
	$(NFORT) $(NFORTFLAGS) -ftrace -cxxlib -fopenmp -o $@ $^ \
		$(NLDFLAGS) $(BVELIB)/libaveoVE.a $(BVELIB)/liburpcVE_omp.a \
		-lveio -pthread

$(BLIBEX)/relink_aveorun_$(VE_ARCH): ../scripts/relink_aveorun.in
	sed -e "s,@libexecdir@,$(DEST)/libexec,g" -e "s,@velibdir@,$(VEDEST)/lib,g" \
	-e "s,@ve_arch@,$(VE_ARCH),g" -e "s,@ncc_path@,$(VEBIN),g" -e "s,@dyn_linker@,$(SCRIPT_DYN_LINKER),g" \
	-e "s,@opt_arch@,$(OPT_ARCH),g" -e "s,@opt_abi@,$(OPT_ABI),g" \
		< $< > $@
	chmod 755 $@

$(BLIBEX)/mk_veorun_static_$(VE_ARCH): $(BLIBEX)/relink_aveorun_$(VE_ARCH)
	ln -sf ./relink_aveorun_$(VE_ARCH) $@

$(BLIBEX)/gen_veorun_static_symtable: ../scripts/gen_veorun_static_symtable
	mkdir -p $(BLIBEX)
	cp -P ../scripts/gen_veorun_static_symtable $(BLIBEX)/
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
		$(VELIBS) $(VEARCS) $(PROGS) $(INCLUDES) $(VEINCLUDES) $(SCRIPTS) $(PYSCRIPTS) \
		$(BVE)/aveorun.o $(BVE)/aveorun-ftrace.o 
