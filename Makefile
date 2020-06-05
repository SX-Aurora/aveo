CWD = $(shell pwd)
DEST ?= $(CWD)/install
BUILD ?= $(CWD)/build

ifeq ($(URPC_INST_DIR),)
PREPARE := ve-urpc
PREPAREINSTALL := ve-urpc-install
URPC_INST_DIR := $(DEST)
endif

PACKAGE := aveo
VERSION := $(shell cat VERSION)
TARBALL := $(PACKAGE)-$(VERSION).tar.gz
RELEASE := 1
include make_aveo.inc

MAKEVARS = DEST=$(DEST) BUILD=$(BUILD) URPC_INST_DIR=$(URPC_INST_DIR) PREF=$(PREF)

ALL: $(PREPARE) aveo tests doc

aveo:
	make -C src $(MAKEVARS)

tests:
	make -C test $(MAKEVARS)

test:
	make -C test test $(MAKEVARS)

doc:
	make -C doc 

install: ALL $(PREPAREINSTALL)
	make -C prereqs/ve-urpc install BUILD=$(BUILD) DEST=$(URPC_INST_DIR) PREF=$(PREF)
	make -C src install $(MAKEVARS)
	make -C test install $(MAKEVARS)

# -- rules for RPM build

aveo.spec: aveo.spec.in
	sed -e "s,@PACKAGE@,$(PACKAGE)," -e "s,@VERSION@,$(VERSION)," \
		-e "s,@PREFIX@,/opt/nec/ve/veos/," \
		-e "s,@SUBPREFIX@,/opt/nec/ve/," \
		-e "s,@RELEASE@,$(RELEASE)," < $< > $@
#		-e "s,@PREFIX@,/usr/local/ve/$(PACKAGE)-$(VERSION)," < $< > $@"

$(TARBALL): aveo.spec $(CWD)/prereqs/ve-urpc/.git
	if [ -d $(PACKAGE)-$(VERSION) ]; then rm -rf $(PACKAGE)-$(VERSION); fi
	mkdir -p $(PACKAGE)-$(VERSION)
	cp -p aveo.spec $(PACKAGE)-$(VERSION)
	cp -rl Makefile make_aveo.inc prereqs README.md scripts src doc test VERSION COPYING $(PACKAGE)-$(VERSION)
	tar czvf $(TARBALL) $(PACKAGE)-$(VERSION)
	rm -rf $(PACKAGE)-$(VERSION)

rpm: $(TARBALL)
	rpmbuild -tb $<

# --------------------------

# -- rules for prerequisites

$(CWD)/prereqs/ve-urpc/.git:
	git clone https://github.com/SX-Aurora/ve-urpc.git prereqs/ve-urpc

ve-urpc: $(CWD)/prereqs/ $(CWD)/prereqs/ve-urpc/.git
	make -C prereqs/ve-urpc BUILD=$(BUILD) DEST=$(URPC_INST_DIR)

ve-urpc-install: ve-urpc
	make -C prereqs/ve-urpc install BUILD=$(BUILD) DEST=$(URPC_INST_DIR) PREF=$(PREF)

# --------------------------

clean:
	make -C src clean $(MAKEVARS)
	make -C test clean $(MAKEVARS)
	make -C doc clean 
	rm -f $(TARBALL)
	rm -f aveo.spec
realclean: clean
	make -C prereqs/ve-urpc clean BUILD=$(BUILD) DEST=$(URPC_INST_DIR)
	rm -rf $(BUILD) prereqs

.PHONY: aveo tests test install ve-urpc clean realclean doc
