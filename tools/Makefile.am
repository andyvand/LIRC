
## Process this file with automake to produce Makefile.in
ACLOCAL_AMFLAGS             = -I m4

AM_CPPFLAGS = -I @kerneldir@ -I../lib -I$(top_srcdir) @X_CFLAGS@
AM_LDFLAGS  =  -L../lib -llirc -llirc_client

EXTRA_PROGRAMS =

if BUILD_XTOOLS
EXTRA_PROGRAMS += xmode2 irxevent
endif

if BUILD_SMODE2
EXTRA_PROGRAMS += smode2
endif

bin_PROGRAMS = irw irpty irexec ircat mode2 irsend \
        lircrcd \
        @vga_progs@ @x_progs@

## simple programs
irw_SOURCES = irw.c
irpty_SOURCES = irpty.c
irpty_LDADD = @forkpty@ -lpthread
irexec_SOURCES = irexec.c
irexec_LDADD = @daemon@
ircat_SOURCES = ircat.c
mode2_SOURCES = mode2.c
irsend_SOURCES = irsend.c


## vga programs
smode2_SOURCES = smode2.c
smode2_LDADD = -lvga -lvgagl

## x programs
irxevent_SOURCES = irxevent.c
irxevent_LDADD = @X_LIBS@ @X_PRE_LIBS@ -lX11 @X_EXTRA_LIBS@
xmode2_SOURCES = xmode2.c
xmode2_LDADD =  @X_LIBS@ @X_PRE_LIBS@ -lX11 @X_EXTRA_LIBS@

## lircrcd

lircrcd_SOURCES = lircrcd.c

if HAVE_PYTHON
dist_bin_SCRIPTS = pronto2lirc
dist_sbin_SCRIPTS = lirc-config-tool \
                    lirc-setup \
                    lirc-install-configuration \
                    lircd-modprobe
endif
