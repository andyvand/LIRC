#!/bin/dash
rm -rf aclocal.m4 config.guess config.h.in* config.sub \
    configure depcomp install-sh install.sh ltmain.sh missing \
    autom4te.cache
find . -name Makefile.in -exec rm {} \;
find . -name Makefile -exec rm {} \;
autoreconf -fi
(cd contrib/hal/ && ./gen-hal-fdi.pl)
