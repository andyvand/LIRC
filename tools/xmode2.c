
/****************************************************************************
 ** xmode2.c ****************************************************************
 ****************************************************************************
 *
 * xmode2 - shows the ir waveform of an IR signal
 *
 * patched together on Feb. 18th 1999 by
 * Heinrich Langos <heinrich@mad.scientist.com>
 *
 * This program is based on the smode2.c file by Sinkovics Zoltan
 * <sinko@szarvas.hu> which is a part of the LIRC distribution. It is
 * just a conversion from svga to X with some basic support for resizing.
 * I copied most of this comment.
 *
 * This program is based on the mode2.c file which is a part of the
 * LIRC distribution. The main purpose of this program is to check
 * operation of LIRC receiver hardware, and to see the IR waveform of
 * the remote controller without an expensive oscilloscope. The time
 * division is variable from 1 ms/div to extremely high values (integer
 * type) but there is no point increasing this value above 20 ms/div,
 * because one pulse is about 1 ms. I think this kind of presentation
 * is much more exciting than the simple pulse&space output showed by
 * mode2.
 *
 * Usage: xmode2 [-t (ms/div)] , default division is 5 ms/div
 *
 *
 * compile: gcc -o xmode2 xmode2.c -L/usr/X11R6/lib -lX11
 *
 * version 0.01  Feb 18 1999
 *   initial release
 *
 * version 0.02  Aug 24 1999
 *   using select() to make the whole thing more responsive
 * */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <unistd.h>
#include <fcntl.h>
#include <getopt.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <limits.h>
#include <errno.h>
#include <syslog.h>
#include <time.h>

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/cursorfont.h>

#include "lirc_private.h"

Display *d1;
Window w0, w1;			/*w0 = root */
char w1_wname[] = "xmode2";
char w1_iname[] = "xmode2";
char font1_name[]= "-misc-fixed-*-r-*-*-12-*-*-*-*-*-iso8859-1";

int w1_x = 0, w1_y = 0;
unsigned int w1_w = 640, w1_h = 480, w1_border = 0;

XFontStruct *f1_str;
XColor xc1, xc2;
Colormap cm1;
XGCValues gcval1;
GC gc1, gc2;
XSetWindowAttributes winatt1;

long event_mask1;
XEvent event_return1;

static int div_ = 5;
static int dmode = 0;
static struct stat s;
static int use_stdin = 0;
static int use_raw_access = 0;

static char *device = "";
static char *geometry = NULL;

static struct option options[] = {
	{"help", no_argument, NULL, 'h'},
	{"version", no_argument, NULL, 'v'},
	{"device", required_argument, NULL, 'd'},
	{"driver", required_argument, NULL, 'H'},
	{"keep-root", required_argument, NULL, 'k'},
	{"geometry", required_argument, NULL, 'g'},
	{"timediv", required_argument, NULL, 't'},
	{"mode", no_argument, NULL, 'm'},
	{"raw", no_argument, NULL, 'r'},
	{"driver-options", required_argument, NULL, 'A'},
	{0, 0, 0, 0}
};

static const char* const help =
"\nUsage: xmode2 [options]\n"
"\nOptions:\n"
"    -d --device=device\t\tread from given device\n"
"    -H --driver=driver\t\tuse given driver\n"
"    -U --plugindir=dir\t\tLoad drivers from given dir\n"
"    -g --geometry=geometry\twindow geometry\n"
"    -t --timediv=value\t\tms per unit\n"
"    -m --mode\t\t\tenable alternative display mode\n"
"    -k --keep-root\t\tkeep root privileges\n"
"    -r --raw\t\t\taccess device directly\n"
"    -h --help\t\t\tdisplay usage summary\n"
"    -v --version\t\tdisplay version\n"
"    -A --driver-options=key:value[|key:value...]\n"
"\t\t\t\tSet driver options\n";


static void add_defaults(void)
{
        char level[4];
        snprintf(level, sizeof(level), "%d", lirc_log_defaultlevel());
	const char* const defaults[] = {
		"lircd:plugindir",            PLUGINDIR,
		"lircd:debug",                level,
		"xmode2:driver",              "devinput",
		"xmode2:analyse",             "False",
		"xmode2:force",               "False",
		"xmode2:disable-namespace",   "False",
		"xmode2:dynamic-codes",       "False",
		"xmode2:list_namespace",      "False",
		(const char*)NULL,      (const char*)NULL
	};
	options_add_defaults(defaults);
}


static void parse_options(int argc, char** const argv)
{
	int c;
	add_defaults();
	const char* const optstring =  "U:hvd:H:g:t:mrA:";
	while ((c = getopt_long(argc, argv, optstring, options, NULL)) != -1) {
		switch (c) {
		case 'h':puts(help);
			exit (EXIT_SUCCESS);
		case 'H':
			if (hw_choose_driver(optarg) != 0) {
				fprintf(stderr, "Driver `%s' not found", optarg);
				fputs(" (wrong or missing -U/--plugindir?)\n",
                                       stderr);
				hw_print_drivers(stderr);
				exit(EXIT_FAILURE);
			}
			break;
		case 'v':
			printf("%s %s\n", progname, VERSION);
			exit (EXIT_SUCCESS);
		case 'd':
			device = optarg;
			break;
		case 'g':
			geometry = optarg;
			break;
		case 't':	/* timediv */
			div_ = strtol(optarg, NULL, 10);
			break;
		case 'k':
			unsetenv("SUDO_USER");
			break;
		case 'm':
			dmode = 1;
			break;
		case 'r':
			use_raw_access = 1;
			break;
		case 'U':
			options_set_opt("lircd:plugindir", optarg);
			break;
		case 'A':
			options_set_opt("lircd:driver-options", optarg);
			break;
		default:
			printf("Usage: %s [options]\n", progname);
			exit (EXIT_FAILURE);
		}
	}
	if (optind < argc) {
		fprintf(stderr, "%s: too many arguments\n", progname);
		exit (EXIT_FAILURE);
	}
}


void initscreen(char *geometry)
{
	d1 = XOpenDisplay(0);
	if (d1 == NULL) {
		puts("Can't open display.");
		exit(0);
	}

	if (geometry != NULL) {
		XParseGeometry(geometry, &w1_x, &w1_y, &w1_w, &w1_h);
	}

	/*Aufbau der XWindowsAttribStr */
	w0 = DefaultRootWindow(d1);
	winatt1.background_pixel = BlackPixel(d1, 0);
	winatt1.backing_store = WhenMapped;
	winatt1.event_mask = KeyPressMask | StructureNotifyMask | ExposureMask;
	w1 = XCreateWindow(d1, w0, w1_x, w1_y, w1_w, w1_h, w1_border, CopyFromParent, InputOutput, CopyFromParent,
			   CWBackPixel | CWBackingStore | CWEventMask, &winatt1);

	XStoreName(d1, w1, w1_wname);
	XSetIconName(d1, w1, w1_iname);
	XMapWindow(d1, w1);

	cm1 = DefaultColormap(d1, 0);
	if (!XAllocNamedColor(d1, cm1, "blue", &xc1, &xc2))
		puts("couldn't allocate blue color");
	f1_str = XLoadQueryFont(d1, font1_name);
	if (f1_str == NULL) {
		puts("could't load font");
		exit(EXIT_FAILURE);
	}

	gcval1.foreground = xc1.pixel;
	gcval1.font = f1_str->fid;
	gcval1.line_style = LineSolid;
	gc1 = XCreateGC(d1, w1, GCForeground | GCLineStyle, &gcval1);
	gcval1.foreground = WhitePixel(d1, 0);
	gc2 = XCreateGC(d1, w1, GCForeground | GCLineStyle | GCFont, &gcval1);
}

void closescreen(void)
{
	XUnmapWindow(d1, w1);
	XCloseDisplay(d1);
}

int main(int argc, char **argv)
{
	fd_set rfds;
	int xfd, maxfd;

	int fd;
	__u32 mode;
	lirc_t data;
	lirc_t x1, y1, x2, y2;
	int result;
	char textbuffer[80];
	const char* new_user;
	const char* opt;

	lirc_log_open("xmode2", 0, LIRC_INFO);
	hw_choose_driver(NULL);
	options_load(argc, argv, NULL, parse_options);
	if (strcmp(device, LIRCD) == 0) {
		fprintf(stderr, "%s: refusing to connect to lircd socket\n", progname);
		return EXIT_FAILURE;
	}
	if (!isatty(STDIN_FILENO)) {
		use_stdin = 1;
		fd = STDIN_FILENO;
	} else if (use_raw_access) {
		fd = open(device, O_RDONLY);
		if (fd == -1) {
			fprintf(stderr, "%s: error opening %s\n", progname, device);
			perror(progname);
			exit(EXIT_FAILURE);
		};

		if ((fstat(fd, &s) != -1) && (S_ISFIFO(s.st_mode))) {
			/* can't do ioctls on a pipe */
		} else if ((fstat(fd, &s) != -1) && (!S_ISCHR(s.st_mode))) {
			fprintf(stderr, "%s: %s is not a character device\n", progname, device);
			fprintf(stderr, "%s: use the -d option to specify the correct device\n", progname);
			close(fd);
			exit(EXIT_FAILURE);
		} else if (ioctl(fd, LIRC_GET_REC_MODE, &mode) == -1) {
			puts("This program is only intended for receivers supporting the pulse/space layer.");
			puts("Note that this is no error, but this program "
			       "simply makes no sense for your\n" "receiver.");
			puts("In order to test your setup run lircd with "
			       "the --nodaemon option and \n" "then check if the remote works with the irw tool.");
			close(fd);
			exit(EXIT_FAILURE);
		}
	} else {
		curr_driver->open_func(device);
		opt = options_getstring("lircd:driver-options");
		if (opt != NULL)
			if (drv_handle_options(opt) != 0) {
				fprintf(stderr,
				"Cannot set driver (%s) options (%s)\n",
				curr_driver->name, opt);
				return EXIT_FAILURE;
                       }
		if (curr_driver->init_func  && !curr_driver->init_func()) {
			fputs("Cannot initialize hardware\n", stderr);
			exit(EXIT_FAILURE);
		}

		fd = curr_driver->fd;	/* please compiler */
		mode = curr_driver->rec_mode;
		if (mode != LIRC_MODE_MODE2) {
			if (strcmp(curr_driver->name, "default") == 0) {
				puts("Please use the --raw option to access "
				       "the device directly instead through\n" "the abstraction layer.");
			} else {
				puts("This program does not work for this hardware yet.");
			}
			exit(EXIT_FAILURE);
		}

	}

	new_user = drop_sudo_root(setuid);
	if (strcmp("root", new_user) == 0)
		puts("Warning: Running as root.");
	else if (strlen(new_user) == 0)
		puts("Warning: Cannot change uid.");
	else
		printf("Running as regular user %s\n", new_user);

	initscreen(geometry);
	xfd = XConnectionNumber(d1);
	maxfd = fd > xfd ? fd : xfd;
	y1 = 20;
	x1 = x2 = 0;
	sprintf(textbuffer, "%d ms/unit", div_);
	for (y2 = 0; y2 < w1_w; y2 += 10)
		XDrawLine(d1, w1, gc1, y2, 0, y2, w1_h);
	XDrawString(d1, w1, gc2, w1_w - 100, 10, textbuffer, strlen(textbuffer));
	XFlush(d1);
	while (1) {
		while (XPending(d1) > 0) {
			XNextEvent(d1, &event_return1);
			switch (event_return1.type) {
			case KeyPress:
				if (event_return1.xkey.keycode == XKeysymToKeycode(d1, XStringToKeysym("q"))) {
					closescreen();
					exit(1);
				}
				break;
			case Expose:
			case ConfigureNotify:
				switch (event_return1.type) {
				case Expose:
					break;
				case ConfigureNotify:
					if (w1_w == event_return1.xconfigure.width &&
					    w1_h == event_return1.xconfigure.height) {
						continue;
					}

					w1_w = event_return1.xconfigure.width;
					w1_h = event_return1.xconfigure.height;
					break;
				}
				XClearWindow(d1, w1);
				for (y2 = 0; y2 < w1_w; y2 += 10)
					XDrawLine(d1, w1, gc1, y2, 0, y2, w1_h);
				XDrawString(d1, w1, gc2, w1_w - 100, 10, textbuffer, strlen(textbuffer));

				XFlush(d1);
				//            printf("resize \n");
				break;
			default:
				;
			}
		}

		FD_ZERO(&rfds);
		FD_SET(fd, &rfds);
		FD_SET(xfd, &rfds);

		select(maxfd + 1, &rfds, NULL, NULL, NULL);

		if (FD_ISSET(fd, &rfds)) {
			if (use_stdin) {
				static int space = 1;
				unsigned long scan;

				if (space) {
					result = fscanf(stdin, "space %ld\n", &scan);
				} else {
					result = fscanf(stdin, "pulse %ld\n", &scan);
				}
				if (result == 1) {
					data = (lirc_t) scan;
					if (!space)
						data |= PULSE_BIT;
				} else {
					fd = STDOUT_FILENO;
				}
				space = !space;
			} else {
				result = read(fd, &data, sizeof(data));
			}
			if (result != 0) {
				//                  printf("%.8x\t",data);
				x2 = (data & PULSE_MASK) / (div_ * 50);
				if (x2 > 400) {
					if (!dmode) {
						y1 += 15;
					} else {
						y1++;
					}
					x1 = 0;
				} else {
					if (x1 == 0) {
						if (!dmode)
							XDrawLine(d1, w1, gc2, x1, y1 + 10, x1 + 10, y1 + 10);
						x1 += 10;
						if (!dmode)
							XDrawLine(d1, w1, gc2, x1, y1 + 10, x1, y1);
					}
					if (x1 < w1_w) {
						if (dmode) {
							if (data & PULSE_BIT)
								XDrawLine(d1, w1, gc2, x1, y1, x1 + x2, y1);
							x1 += x2;
						} else {
							XDrawLine(d1, w1, gc2, x1,
								  ((data & PULSE_BIT) ? y1 : y1 + 10), x1 + x2,
								  ((data & PULSE_BIT) ? y1 : y1 + 10));
							x1 += x2;
							XDrawLine(d1, w1, gc2, x1,
								  ((data & PULSE_BIT) ? y1 : y1 + 10), x1,
								  ((data & PULSE_BIT) ? y1 + 10 : y1));
						}
					}
				}
				if (y1 > w1_h) {
					y1 = 20;
					XClearWindow(d1, w1);
					for (y2 = 0; y2 < w1_w; y2 += 10)
						XDrawLine(d1, w1, gc1, y2, 0, y2, w1_h);
					XDrawString(d1, w1, gc2, w1_w - 100, 10, textbuffer, strlen(textbuffer));
				}
			}
			XFlush(d1);
		}
	}
	exit(EXIT_SUCCESS);
}
