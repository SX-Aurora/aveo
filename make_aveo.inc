NCPP = /opt/nec/ve/bin/nc++
NCC = /opt/nec/ve/bin/ncc
NFORT = /opt/nec/ve/bin/nfort
NAR = /opt/nec/ve/bin/nar
GCC = gcc
GPP = g++
AR = ar

DEBUG = -g
OPT = -O2

ifneq ($(MAKECMDGOALS),clean)
ifndef URPC_INST_DIR
$(error "URPC_INST_DIR is undefined! Please point it to the place where veo-urpc is installed.")
endif
endif

VEDEST?=$(DEST)

INCL = -I$(BUILD)/include -I$(DEST)/include -I. -I$(URPC_INST_DIR)/include
NINCL = -I$(BUILD)/include -I$(VEDEST)/include -I. -I$(URPC_INST_DIR)/include
ifndef NOCPP17
GPPFLAGS = -std=c++17 -pthread $(OPT) $(DEBUG) $(INCL)
else
GPPFLAGS = -std=gnu++11 -pthread $(OPT) $(DEBUG) $(INCL) -DNOCPP17
endif
GCCFLAGS = -std=c11 -pthread  $(OPT) $(DEBUG) $(INCL)
NCPPFLAGS = $(FTRACE) -std=gnu++11 -pthread  $(OPT) $(DEBUG) $(NINCL) -finline -finline-functions
NCCFLAGS = $(FTRACE) -pthread  $(OPT) $(DEBUG) $(NINCL) -finline -finline-functions
NFORTFLAGS = $(FTRACE) -pthread  $(OPT) $(DEBUG) $(NINCL)
LDFLAGS = \
  -Wl,-rpath,$(DEST)/lib64 -L$(BLIB) -L$(PERF)$(DEST)/lib64 $(DEBUG) \
  -Wl,-rpath,$(URPC_INST_DIR)/lib64 -L$(URPC_INST_DIR)/lib64
NLDFLAGS = -Wl,-rpath,$(VEDEST)/lib -L$(BVELIB) -L$(PREF)$(VEDEST)/lib -Wl,--build-id=sha1 $(DEBUG)

# build directory substructure
BVE     = $(BUILD)/ve
BVE_OMP = $(BUILD)/ve_omp
BVH     = $(BUILD)/vh
BB      = $(BUILD)/bin
BLIB    = $(BUILD)/lib64
BVELIB	= $(BUILD)/lib
BLIBEX  = $(BUILD)/libexec
BINC    = $(BUILD)/include
BVEINC    = $(BUILD)/include-ve

# rule for creating directories
%/:
	mkdir -p $@
