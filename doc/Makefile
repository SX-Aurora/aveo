dist_noinst_DATA = GettingStarted.md Restriction.md Doxyfile
EXTRA_DIST = Doxyfile.in
CLEANFILES = Doxyfile doxygen
PACKAGE = aveo
VERSION = $(shell cat ../VERSION)
TOP_SRCDIR = $(abspath ../src/ ./)

edit = sed -e "s,@PACKAGE@,$(PACKAGE)," \
	-e "s,@VERSION@,$(VERSION)," \
	-e "s,@top_srcdir@,$(TOP_SRCDIR),"

Doxyfile: Doxyfile.in
	echo $(SRCDIR)
	$(edit) $< > $@

clean:
	rm -rf $(CLEANFILES)
