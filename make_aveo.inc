NCPP = /opt/nec/ve/bin/nc++
NCC = /opt/nec/ve/bin/ncc
NFORT = /opt/nec/ve/bin/nfort
NAR = /opt/nec/ve/bin/nar
GCC = gcc
GPP = g++
AR = ar

DEBUG = -g
OPT = -O0

ifneq ($(MAKECMDGOALS),clean)
ifndef URPC_INST_DIR
$(error "URPC_INST_DIR is undefined! Please point it to the place where veo-urpc is installed.")
endif
endif

INCL = -I$(BUILD)/include -I$(DEST)/include -I. -I$(URPC_INST_DIR)/include
GPPFLAGS = -std=gnu++11 -pthread $(OPT) $(DEBUG) $(INCL)
GCCFLAGS = -std=c11 -pthread  $(OPT) $(DEBUG) $(INCL)
NCPPFLAGS = $(FTRACE) -std=gnu++11 -pthread  $(OPT) $(DEBUG) $(INCL) -finline -finline-functions
NCCFLAGS = $(FTRACE) -pthread  $(OPT) $(DEBUG) $(INCL) -finline -finline-functions
NFORTFLAGS = $(FTRACE) -pthread  $(OPT) $(DEBUG) $(INCL)
LDFLAGS = \
  -Wl,-rpath,$(abspath $(DEST)/lib64) -L$(DEST)/lib64 \
  -Wl,-rpath,$(abspath $(BLIB)) -L$(BLIB) \
  -Wl,-rpath,$(abspath $(BVELIB)) -L$(BVELIB) \
  -Wl,-rpath,$(abspath $(URPC_INST_DIR)/lib64) -L$(URPC_INST_DIR)/lib64 
NLDFLAGS = -Wl,-rpath,/opt/nec/ve/lib64 -L/opt/nec/ve/lib64 $(LDFLAGS)


# build directory substructure
BVE     = $(BUILD)/ve
BVE_OMP = $(BUILD)/ve_omp
BVH     = $(BUILD)/vh
BB      = $(BUILD)/bin
BLIB    = $(BUILD)/lib64
BVELIB	= $(BUILD)/lib
BLIBEX  = $(BUILD)/libexec
BINC    = $(BUILD)/include

# rule for creating directories
%/:
	mkdir -p $@
