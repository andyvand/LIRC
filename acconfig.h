/*      $Id: acconfig.h,v 1.1.1.1 1999/04/29 21:16:40 columbus Exp $      */

/*
 *  are you editing the correct file?
 *  
 *  acconfig.h  - changes for distribution
 *                you must run autoheader / configure
 *  config.h.in - changes specific to your installation
 *                these will be lost if you run autoheader
 *  config.h    - changes to this configuration
 *                these will be lost if you run configure
 */

/* note.
 * if you want to change silly things like the device file names or the
 * configuration file names then remember you may also need to change
 * the Makefile.am files.
 */

/* device file names - beneath DEVDIR (default /dev) */
#define DEV_LIRC	"lirc"
#define DEV_LIRCD	"lircd"
#define DEV_LIRCM	"lircm"

/* config file names - beneath SYSCONFDIR (default /usr/local/etc) */
#define CFG_LIRCD	"lircd.conf"
#define CFG_LIRCM	"lircmd.conf"

/* config file names - beneath $HOME */
#define CFG_USER	".lircrc"

/* log files */
#define LOGDIR		"/var/log"
#define LOG_LIRCD	"lircd"
#define LOG_LIRMAND	"lirmand"

/*
 * below here are defines managed by autoheader / autoconf
 */

@TOP@

/* Define to use long long IR codes */
#define LONG_IR_CODE

/* Define to enable debugging output */
#undef DEBUG

/* Define to run daemons as daemons */
#undef DAEMONIZE

/* Define if the libirman library is installed */
#undef HAVE_LIBIRMAN

/* Define if the software only test version of libirman is installed */
#undef HAVE_LIBIRMAN_SW

/* Define if the complete vga libraries (vga, vgagl) are installed */
#undef HAVE_LIBVGA

/* Text string signifying which driver is configured */
#define LIRC_DRIVER		"unknown"

/* Set the device major for the lirc driver */
#define LIRC_MAJOR		61

/* Set the IRQ for the lirc driver */
#define LIRC_IRQ		4

/* Set the port address for the lirc driver */
#define LIRC_PORT		0x3f8

/* Set the timer for the parallel port driver */
#define LIRC_TIMER		65536

/* Set the default tty used by the irman driver */
#define LIRC_IRMAN_TTY		"/dev/ttyS0"

/* Define if you have an animax serial port receiver */
#undef LIRC_SERIAL_ANIMAX

/* Define if the software needs to generate the carrier frequency */
#undef LIRC_SERIAL_SOFTCARRIER

/* system configuration directory */
#define SYSCONFDIR		"/etc"

/* device files directory */
#define DEVDIR			"/dev"

/* This should only be set by configure */
#define PACKAGE			"unset"

/* This should only be set by configure */
#define VERSION			"0.0.0"

@BOTTOM@

/*
 * compatibility and useability defines
 */

#define LIRC_DRIVER_DEVICE	DEVDIR "/" DEV_LIRC
#define LIRCD			DEVDIR "/" DEV_LIRCD
#define LIRCM			DEVDIR "/" DEV_LIRCM

#define LIRCDCFGFILE		SYSCONFDIR "/" CFG_LIRCD
#define LIRCMDCFGFILE		SYSCONFDIR "/" CFG_LIRCM

#define LIRCCFGFILE		CFG_USER

#define LOGFILE			LOGDIR "/" LOG_LIRCD
#define LIRMAND_LOGFILE		LOGDIR "/" LOG_LIRMAND

/* end of acconfig.h */
