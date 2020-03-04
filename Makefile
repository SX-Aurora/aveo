CWD = $(shell pwd)
DEST ?= $(CWD)/install
BUILD ?= $(CWD)/build

ifeq ($(URPC_INST_DIR),)
PREPARE := ve-urpc
URPC_INST_DIR := $(DEST)
endif

include make_aveo.inc

MAKEVARS = DEST=$(DEST) BUILD=$(BUILD) URPC_INST_DIR=$(URPC_INST_DIR)

ALL: $(PREPARE) aveo test

aveo:
	make -C src $(MAKEVARS)

test:
	make -C test $(MAKEVARS)


install:
	make -C src install $(MAKEVARS)
	make -C test install $(MAKEVARS)

# -- rules for prerequisites

$(CWD)/prereqs/ve-urpc/.git:
	git clone https://github.com/SX-Aurora/ve-urpc.git prereqs/ve-urpc

ve-urpc: $(CWD)/prereqs/ $(CWD)/prereqs/ve-urpc/.git
	make -C prereqs/ve-urpc BUILD=$(BUILD) DEST=$(URPC_INST_DIR)
	make -C prereqs/ve-urpc install BUILD=$(BUILD) DEST=$(URPC_INST_DIR)

# --------------------------

clean:
	make -C src clean $(MAKEVARS)
	make -C test clean $(MAKEVARS)

realclean: clean
	make -C prereqs/ve-urpc clean BUILD=$(BUILD) DEST=$(URPC_INST_DIR)
	rm -rf $(BUILD) prereqs

.PHONY: aveo test install clean aveo ve-urpc
