## Process this file with automake to produce Makefile.in

## set automake strictness to 'foreign'
AUTOMAKE_OPTIONS = foreign

SUBDIRS = src scripts

sysconf_DATA = SCEd.cfg

EXTRA_DIST = $(sysconf_DATA) CREDITS contrib


dist-hook:
	rm -rf `find $(distdir)/contrib -name CVS`


## convenience targets

.PHONY: $(SUBDIRS) install-server

$(SUBDIRS):
	$(MAKE) -C $@

install-server:
	$(MAKE) -C src install



.PHONY: topleveltags


topleveltags:
	$(CTAGS) --format=1 -f - --languages=C --c-kinds=f --recurse=yes  server shared clients \
	| perl -p -e 's/^([^\t]+)\t[^\t]+\/([^\t\/]+)\t(.*)$/$1\t$2\t$3/' > tags

## EOF
