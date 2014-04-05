[NAME]
lirc-setup - Interactive lirc setup tool

[DESCRIPTION]
A simple tool which creates a lirc configuration after
some user dialogs which defines driver, device, modprobe
actions, etc.

[FILES]
.TP 4
.B hardware.yaml
The lirc hardware database. By default the file is searched for
in ., .., /usr/share/lirc and the dir where lirc-setup lives.
.TP 4
.B configuration.conf
The output file. This can be inspected and edited manually.

[SEE ALSO]
.TP 4
lirc-install-configuration.
Script which installs configuration.conf as root.
