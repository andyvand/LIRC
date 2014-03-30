
## Process this file with automake to produce Makefile.in
## Makefile.am, (c)1999 Tom Wheeley <tw104@york.ac.uk>
ACLOCAL_AMFLAGS             = -I m4

## we need version 1.5 of automake for DIST_SUBDIRS to work and dist-bzip2.
AUTOMAKE_OPTIONS            = 1.5 check-news dist-bzip2

EXTRA_DIST                  = ANNOUNCE TODO ChangeLog

BUILT_SOURCES               = paths.h

## Flags to pass to ./configure when calling "make distcheck".
## Try to check as many features as possible! Also
## need to give enable-sandboxed, so that staged install does not fail.

DISTCHECK_FLAGS             = --enable-sandboxed
DISTCHECK_CONFIGURE_FLAGS   = ${DISTCHECK_FLAGS}

DISTCLEANFILES              = configure.sh  ChangeLog paths.h
CLEANFILES                  = *~ remotes/*~ contrib/*~


## daemons/ and tools/ should appear before doc/ for help2man to work.
if SYSTEMD_INSTALL
    SYSTEMD_DIR = systemd
endif

SUBDIRS                     = lib daemons tools doc $(SYSTEMD_DIR) plugins


lirc_confdir                = $(sysconfdir)/lirc
dist_lirc_conf_DATA         = lircd.conf lirc_options.conf lircmd.conf

pkgconfigdir                = $(libdir)/pkgconfig
dist_pkgconfig_DATA         = lirc.pc lirc-driver.pc

dist_pkgdata_DATA           = remotes.tar.gz

# ----------------------------------------------------------------------

ChangeLog: .phony

.phony:
	test -s ChangeLog || git log > ChangeLog || \
	    echo "See http://http://sourceforge.net/p/lirc/git/ci//tree/" > \
	    ChangeLog


# ----------------------------------------------------------------------

dist-hook:
	(cd $(srcdir)/contrib/hal/ && ./gen-hal-fdi.pl)
	for f in remotes contrib; do \
	    cp -pr $(srcdir)/$$f $(distdir)/$$f; done

install-data-hook:
	cd $(DESTDIR)$(pkgdatadir); \
	    tar xzf remotes.tar.gz; rm -f remotes.tar.gz

uninstall-local:
	cd $(DESTDIR)$(pkgdatadir); rm -rf remotes

remotes.tar.gz:
	tar czf $@ remotes

paths.h: Makefile
	@echo '#define DATADIR          "$(datadir)"' >$@
	@echo '#define SYSCONFDIR       "$(sysconfdir)"' >>$@
	@echo '#define VARRUNDIR        "$(localstatedir)/run"' >>$@
	@echo '#define LOCALSTATEDIR    "$(localstatedir)"' >>$@

