#!/bin/bash

SRCDIR=${srcdir:-.}
SOURCE_DIR="${SRCDIR}/html-source/"
MAN_HTML_DIR="${SRCDIR}/man-html/"
DEST_DIR=html/

if test ! -e ${DEST_DIR}; then
    mkdir $DEST_DIR;
fi

FILES1="index.html install.html configure.html configuration-guide.html
programs.html technical.html help.html audio.html audio-alsa.html
alsa-usb.html devinput.html imon.html imon-24g.html pronto2lirc.html
tira.html"
FILES2="irexec.html ircat.html irw.html irpty.html irrecord.html
irxevent.html lircd.html lircmd.html lircrcd.html mode2.html
smode2.html xmode2.html irsend.html"

echo -n "Pass1:"
for FILE in $FILES1; do
    {
    echo -n " $FILE"
    test "$DEST_DIR/${FILE:-foo}" -nt "$SOURCE_DIR/$FILE" && continue
    cat $SOURCE_DIR/head.html > $DEST_DIR/$FILE
    cat $SOURCE_DIR/$FILE     >> $DEST_DIR/$FILE
    cat $SOURCE_DIR/foot.html >> $DEST_DIR/$FILE
    }
done
echo

echo -n "Pass2:"
for FILE in $FILES2; do
    basename=`basename $FILE .html`
#    test -z "`find .. -name $basename`" && continue
#    test "$DEST_DIR/${FILE:-foo}" -nt "man-html/$FILE" && continue
    {
    echo -n " $FILE"
    cat $SOURCE_DIR/head.html > $DEST_DIR/$FILE
    cat $MAN_HTML_DIR/$FILE   >> $DEST_DIR/$FILE
    cat $SOURCE_DIR/foot.html >> $DEST_DIR/$FILE
    }
done
