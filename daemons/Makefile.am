## Process this file with automake to produce Makefile.in
ACLOCAL_AMFLAGS             = -I m4

AM_CPPFLAGS = -I@kerneldir@ -I$(top_srcdir) -I../lib
AM_LDFLAGS  = -L../lib -llirc -lpthread

varrundir   = $(localstatedir)/run


sbin_PROGRAMS = lircd lircmd

lircd_SOURCES = lircd.c lircd.h

lircmd_SOURCES = lircmd.c

bin_PROGRAMS = irrecord

irrecord_SOURCES = irrecord.c

## maintainer mode stuff
EXTRA_PROGRAMS = lircd.simsend lircd.simrec slinke
noinst_PROGRAMS = @maintmode_daemons_extra@

lircd_simsend_SOURCES = lircd.c
lircd_simsend_CFLAGS = -DSIM_SEND

lircd_simrec_SOURCES = lircd.c
lircd_simrec_CFLAGS = -DSIM_REC

slinke_SOURCES = slinke.c slinke.h


if SANDBOXED
else
if USE_FIFOS
install-exec-local: mkfifo
uninstall-local: rmfifo
else
install-exec-local: mkdev
uninstall-local: rmdev
endif
endif

install-data-local:
	test "$(lircd_conf)" = ""  || test -e $(DESTDIR)$(sysconfdir)/$(PACKAGE)/lircd.conf  || ($(mkinstalldirs) $(DESTDIR)$(sysconfdir)/lirc && $(INSTALL_DATA) $(top_srcdir)/remotes/$(lircd_conf) $(DESTDIR)$(sysconfdir)/$(PACKAGE)/lircd.conf)
	test "$(lircmd_conf)" = "" || test -e $(DESTDIR)$(sysconfdir)/$(PACKAGE)/lircmd.conf || ($(mkinstalldirs) $(DESTDIR)$(sysconfdir)/lirc && $(INSTALL_DATA) $(top_srcdir)/remotes/$(lircmd_conf) $(DESTDIR)$(sysconfdir)/$(PACKAGE)/lircmd.conf)

mkdev:
	test -e $(DESTDIR)$(varrundir)/$(PACKAGE)/lircd || ($(mkinstalldirs) $(DESTDIR)$(varrundir)/lirc && @mknod@ $(DESTDIR)$(varrundir)/$(PACKAGE)/lircd p)
	test -e $(DESTDIR)$(varrundir)/$(PACKAGE)/lircm || ($(mkinstalldirs) $(DESTDIR)$(varrundir)/lirc && @mknod@ $(DESTDIR)$(varrundir)/$(PACKAGE)/lircm p)

mkfifo:
	test -e $(DESTDIR)$(varrundir)/$(PACKAGE)/lircd || ($(mkinstalldirs) $(DESTDIR)$(varrundir)/lirc && @mkfifo@ $(DESTDIR)$(varrundir)/$(PACKAGE)/lircd)
	test -e $(DESTDIR)$(varrundir)/$(PACKAGE)/lircm || ($(mkinstalldirs) $(DESTDIR)$(varrundir)/lirc && @mkfifo@ $(DESTDIR)$(varrundir)/$(PACKAGE)/lircm)

rmdev:
rmfifo:
	-$(RM) $(DESTDIR)$(varrundir)/$(PACKAGE)/lircd
	-$(RM) $(DESTDIR)$(varrundir)/$(PACKAGE)/lircm


CLEANFILES = *~
