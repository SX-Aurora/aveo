CWD = $(shell pwd)
DEST ?= $(CWD)/install
BUILD ?= $(CWD)/build

ifeq ($(URPC_INST_DIR),)
PREPARE := ve-urpc
PREPAREINSTALL := ve-urpc-install
PREPAREVE := ve-urpc-ve
PREPAREINSTALLVE := ve-urpc-install-ve
PREPAREVH := ve-urpc-vh
PREPAREINSTALLVH := ve-urpc-install-vh
URPC_INST_DIR := $(DEST)
endif

PACKAGE := aveo
VENDOR := NEC HPC Europe
VERSION := $(shell cat VERSION)
TARBALL := $(PACKAGE)-$(VERSION).tar.gz
RELEASE := 1
RPMBUILD_DIR := $(HOME)/rpmbuild
include make_aveo.inc

MAKEVARS = DEST=$(DEST) BUILD=$(BUILD) URPC_INST_DIR=$(URPC_INST_DIR) PREF=$(PREF)

ALL: $(PREPARE) aveo tests doc

all-ve: $(PREPAREVE) aveo-ve

all-vh: $(PREPAREVH) aveo-vh

aveo:
	make -C src $(MAKEVARS)

aveo-ve:
	make -C src all-ve $(MAKEVARS)

aveo-vh:
	make -C src all-vh $(MAKEVARS)

tests:
	make -C test $(MAKEVARS)

test:
	make -C test test $(MAKEVARS)

doc:
	make -C doc 

install: ALL $(PREPAREINSTALL)
	make -C src install $(MAKEVARS)
	make -C test install $(MAKEVARS)

install-ve: all-ve $(PREPAREINSTALLVE)
	make -C src install-ve $(MAKEVARS)

install-vh: all-vh $(PREPAREINSTALLVH)
	make -C src install-vh $(MAKEVARS)

# -- rules for RPM build

edit = sed -e "s,@PACKAGE@,$(PACKAGE)," -e "s,@VERSION@,$(VERSION)," \
		-e "s,@VENDOR@,$(VENDOR)," \
		-e "s,@PREFIX@,/opt/nec/ve/veos," \
		-e "s,@SUBPREFIX@,/opt/nec/ve," \
		-e "s,@RELEASE@,$(RELEASE),"
#		-e "s,@PREFIX@,/usr/local/ve/$(PACKAGE)-$(VERSION),"

aveo.spec: aveo.spec.in
	$(edit) $< > $@

aveorun.spec: aveorun.spec.in
	$(edit) $< > $@

$(TARBALL): aveo.spec $(CWD)/prereqs/ve-urpc/.git
	if [ -d $(PACKAGE)-$(VERSION) ]; then rm -rf $(PACKAGE)-$(VERSION); fi
	mkdir -p $(PACKAGE)-$(VERSION)
	cp -p aveo.spec $(PACKAGE)-$(VERSION)
	cp -rl Makefile make_aveo.inc prereqs README.md scripts src doc test VERSION COPYING $(PACKAGE)-$(VERSION)
	rm -rf $(PACKAGE)-$(VERSION)/prereqs/ve-urpc/.git/*
	tar czvf $(TARBALL) $(PACKAGE)-$(VERSION)
	rm -rf $(PACKAGE)-$(VERSION)

rpm: $(TARBALL) aveo.spec aveorun.spec
	mkdir -p ${RPMBUILD_DIR}/SOURCES
	cp $(TARBALL) ${RPMBUILD_DIR}/SOURCES
	rpmbuild -D "%_topdir $(RPMBUILD_DIR)" -ba aveo.spec
	rpmbuild -D "%_topdir $(RPMBUILD_DIR)" -ba aveorun.spec

.PHONY: aveo.spec aveorun.spec $(TARBALL) rpm

# --------------------------

# -- rules for prerequisites

$(CWD)/prereqs/ve-urpc/.git:
	git clone https://github.com/SX-Aurora/ve-urpc.git prereqs/ve-urpc

ve-urpc: $(CWD)/prereqs/ $(CWD)/prereqs/ve-urpc/.git
	make -C prereqs/ve-urpc BUILD=$(BUILD) DEST=$(URPC_INST_DIR)

ve-urpc-install: ve-urpc
	make -C prereqs/ve-urpc install BUILD=$(BUILD) DEST=$(URPC_INST_DIR) PREF=$(PREF)

ve-urpc-ve: $(CWD)/prereqs/ $(CWD)/prereqs/ve-urpc/.git
	make -C prereqs/ve-urpc all-ve BUILD=$(BUILD) DEST=$(URPC_INST_DIR)

ve-urpc-install-ve: ve-urpc-ve
	make -C prereqs/ve-urpc install-ve BUILD=$(BUILD) DEST=$(URPC_INST_DIR) PREF=$(PREF)

ve-urpc-vh: $(CWD)/prereqs/ $(CWD)/prereqs/ve-urpc/.git
	make -C prereqs/ve-urpc all-vh BUILD=$(BUILD) DEST=$(URPC_INST_DIR)

ve-urpc-install-vh: ve-urpc-vh
	make -C prereqs/ve-urpc install-vh BUILD=$(BUILD) DEST=$(URPC_INST_DIR) PREF=$(PREF)

# --------------------------

clean:
	make -C src clean $(MAKEVARS)
	make -C test clean $(MAKEVARS)
	make -C doc clean 
	make -C prereqs/ve-urpc clean BUILD=$(BUILD) DEST=$(URPC_INST_DIR)
	rm -f $(TARBALL)
	rm -f aveo.spec
	rm -f aveorun.spec
realclean: clean
	rm -rf $(BUILD) prereqs

.PHONY: aveo tests test install ve-urpc clean realclean doc
