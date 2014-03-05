This is a temporary playground for lirc. Feel free to test, but be aware
that branches here will be rewritten any time; they are basically feature
branches.

That said, here is a first sketch for lirc which provides:
- Complete systemd support including socket activation
  and modprobing required modules.
- Dynamic loading of userspace drivers.
- Out-of-tree builds of any driver against a common lib
  (examples in drivers/).
- Standard build-install using autoreconf, configure, make install.
  The old setup.sh script is no longer needed, nor is autogen.sh.
- A config file which makes the use of command line switches
  for lircd unnecessary. Helps systemd and documentation.
- A new tool lirc-setup which aids in the process to select
  remote, driver, device, etc.
- Automatic handling of protocol selection on the rc devices. The
  "echo lirc >/sys/class/rc/rcX/protocols" is not needed anymore.
- "make distcheck" now passes.
- Debug printouts are now solely enabled in runtime which means that
  the configuration debug option could be left on in almost all cases(?).
- A number of patches from the mailing list or otherwise floating around
  has been merged, notably a blocking bugfix on ftdi and a new zotac remote
  config + userspace driver.
- A number of fresh bugs.

Note that this version does not use the config.sh script to build.
Instead, the build procedure is the standard one:

git checkout HEAD -- configure.ac NEWS lirc.pc # Don't care if building
                                               # from dist tarball.
autoreconf -fi
./configure --enable-debug --with-syslog=LOG_DAEMON
make
make [DESTDIR=/whatever] install
