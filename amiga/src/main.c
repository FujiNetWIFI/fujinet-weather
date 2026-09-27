/*
 * fnweather -- FujiNet weather for the Amiga, Workbench 1.3 and later.
 *
 * Usage:  fnweather [-t] [-i] [place name]
 *
 *   (no args)   locate by IP address, open a window, keys R U L Q
 *   place name  look the place up with Open-Meteo's geocoder instead
 *   -t          print one report to the Shell and exit (scriptable)
 *   -i          imperial units (°F, mph, in)
 *
 * From Workbench, double-click the icon.  Its Tool Types PLACE=<name> and
 * UNITS=IMPERIAL do what the arguments do.
 *
 * Needs the FujiNet NIO driver resident; if it is not, fnweather loads it
 * with fujinet-load-resident from C:/DEVS: or, failing that, from NIO:.
 *
 * Ported from the Apple2/MS-DOS fnweather (Open-Meteo + ip-api).  Other
 * ports ask FujiNet to parse the JSON; NIO has no such service on the
 * Amiga, so json.c parses the responses here.
 *
 * Use snprintf(), never sprintf(): -lamiga precedes libnix on the link line,
 * so sprintf() resolves to amiga.lib's RawDoFmt() wrapper, which has no %u
 * and reads %d as a 16-bit WORD.  snprintf() exists only in libnix.
 */
#include <stdio.h>
#include <string.h>

#include <dos/dos.h>
#include <workbench/startup.h>
#include <workbench/workbench.h>
#include <proto/dos.h>
#include <proto/exec.h>
#include <proto/icon.h>

#include "display.h"
#include "meteo.h"
#include "net.h"

static const char version_tag[] __attribute__((used)) =
	"$VER: fnweather 1.0 (27.9.2026) FujiNet NIO";

#define WINDOW_SPEC "RAW:0/0/640/200/FujiNet Weather"
/* WaitForChar() takes microseconds; refresh after 15 idle minutes, the
 * same pace as the ADAM port, and within Open-Meteo's update interval. */
#define IDLE_SLICE_US  10000000L
#define IDLE_SLICES    90

/* Defined here and opened explicitly, rather than left to libnix's
 * auto-open, so a system without icon.library still runs from the Shell. */
struct Library *IconBase = NULL;

static WEATHER weather;
static char place[48];

static int refresh(void)
{
	show_status("Fetching weather from Open-Meteo...");
	if (fetch_weather(&weather) != 0) {
		show_status(meteo_error());
		return 1;
	}
	show_weather(&weather);
	show_menu(&weather);
	show_status("");
	return 0;
}

/* Read a line in the RAW: window with echo; returns 0 on Esc. */
static int read_line(BPTR con, char *buf, int len)
{
	int n = 0;
	char c;

	for (;;) {
		if (Read(con, &c, 1) != 1)
			return 0;
		if (c == '\r' || c == '\n') {
			buf[n] = '\0';
			return n > 0;
		}
		if (c == 0x1b)
			return 0;
		if ((c == 0x08 || c == 0x7f) && n > 0) {
			--n;
			out("\x08 \x08");
		} else if ((unsigned char)c >= 0x20 && c != 0x7f && n < len - 1) {
			char echo[2] = {c, '\0'};

			buf[n++] = c;
			out(echo);
		}
	}
}

static void change_location(BPTR con)
{
	LOCATION found;

	show_status("Place name (Esc cancels): ");
	if (!read_line(con, place, sizeof(place))) {
		show_status("");
		return;
	}
	show_status("Looking up place...");
	if (locate_by_name(&found, place) != 0) {
		show_status(meteo_error());
		return;
	}
	weather.loc = found;
	refresh();
}

static void interactive(BPTR con)
{
	int idle = 0;
	char c;

	refresh();
	for (;;) {
		if (!WaitForChar(con, IDLE_SLICE_US)) {
			if (++idle >= IDLE_SLICES) {
				idle = 0;
				refresh();
			}
			continue;
		}
		idle = 0;
		if (Read(con, &c, 1) != 1)
			return;
		switch (c) {
		case 'r': case 'R':
			refresh();
			break;
		case 'u': case 'U':
			weather.units = weather.units == METRIC ? IMPERIAL : METRIC;
			refresh();
			break;
		case 'l': case 'L':
			change_location(con);
			break;
		case 'q': case 'Q': case 0x1b:
			return;
		}
	}
}

static void usage(void)
{
	out("Usage: fnweather [-t] [-i] [place name]\n"
	    "  -t  print one report and exit   -i  imperial units\n");
}

/* Tool Types from our icon when started from Workbench. */
static void read_tool_types(struct WBStartup *wb)
{
	struct WBArg *self = &wb->sm_ArgList[0];
	struct DiskObject *icon;
	BPTR old_dir;
	char *value;

	IconBase = OpenLibrary((UBYTE *)"icon.library", 33);
	if (!IconBase)
		return;
	old_dir = CurrentDir(self->wa_Lock);
	icon = GetDiskObject((UBYTE *)self->wa_Name);
	if (icon) {
		value = (char *)FindToolType((UBYTE **)icon->do_ToolTypes, (UBYTE *)"PLACE");
		if (value && strlen(value) < sizeof(place))
			strcpy(place, value);
		value = (char *)FindToolType((UBYTE **)icon->do_ToolTypes, (UBYTE *)"UNITS");
		if (value && MatchToolValue((UBYTE *)value, (UBYTE *)"IMPERIAL"))
			weather.units = IMPERIAL;
		FreeDiskObject(icon);
	}
	CurrentDir(old_dir);
	CloseLibrary(IconBase);
	IconBase = NULL;
}

/* Connect to FujiNet, loading the resident NIO driver first if needed.
 * A double-click from Workbench then works without any Shell setup. */
static int connect_fujinet(void)
{
	static const char *const loaders[][2] = {
		{"C:fujinet-load-resident",
		 "C:fujinet-load-resident DEVS:fujinet-nio.device fujinet-nio.device"},
		{"NIO:fujinet-load-resident",
		 "NIO:fujinet-load-resident NIO:fujinet-nio.device fujinet-nio.device"},
	};
	BPTR nil;
	int i;

	if (net_init() == 0)
		return 0;
	net_done();
	show_status("Loading the FujiNet NIO driver...");
	nil = Open((UBYTE *)"NIL:", MODE_NEWFILE);
	for (i = 0; i < 2; ++i) {
		BPTR lock = Lock((UBYTE *)loaders[i][0], ACCESS_READ);

		if (!lock)
			continue;
		UnLock(lock);
		Execute((UBYTE *)loaders[i][1], 0, nil);
		if (net_init() == 0)
			break;
		net_done();
	}
	if (nil)
		Close(nil);
	return i < 2 ? 0 : 1;
}

/* Show a fatal message; in a window, keep it up until a key is pressed. */
static void fail(BPTR con, const char *msg)
{
	show_status(msg);
	if (con) {
		char c;

		out("\n\n Press any key to close.");
		Read(con, &c, 1);
	}
}

int main(int argc, char **argv)
{
	BPTR con = 0;
	int from_workbench = argc == 0;
	int text_mode = 0;
	int i, rc = RETURN_OK;

	place[0] = '\0';
	if (from_workbench) {
		/* libnix passes the WBStartup message as argv; no Output() here. */
		read_tool_types((struct WBStartup *)argv);
	} else {
		display_init((long)Output(), 0);
		for (i = 1; i < argc; ++i) {
			if (strcmp(argv[i], "-t") == 0 || strcmp(argv[i], "-T") == 0) {
				text_mode = 1;
			} else if (strcmp(argv[i], "-i") == 0 || strcmp(argv[i], "-I") == 0) {
				weather.units = IMPERIAL;
			} else if (argv[i][0] == '?' || argv[i][0] == '-') {
				usage();
				return RETURN_WARN;
			} else if (strlen(place) + strlen(argv[i]) + 2 < sizeof(place)) {
				if (place[0])
					strcat(place, " ");
				strcat(place, argv[i]);
			}
		}
	}

	if (!text_mode) {
		con = Open((UBYTE *)WINDOW_SPEC, MODE_NEWFILE);
		if (con)
			display_init((long)con, 1);
		else if (from_workbench)
			return RETURN_FAIL;     /* nowhere to report anything */
		else
			text_mode = 1;          /* no window: print to the Shell */
	}

	show_status("Connecting to FujiNet...");
	if (connect_fujinet() != 0) {
		fail(con, "FujiNet NIO not available: load it with fujinet-load-resident.");
		rc = RETURN_FAIL;
	} else {
		show_status(place[0] ? "Looking up place..." : "Finding location by IP...");
		if ((place[0] ? locate_by_name(&weather.loc, place)
		              : locate_by_ip(&weather.loc)) != 0) {
			fail(con, meteo_error());
			rc = RETURN_ERROR;
		} else if (text_mode) {
			if (refresh() != 0)
				rc = RETURN_ERROR;
		} else {
			interactive(con);
		}
		net_done();
	}

	if (con)
		Close(con);
	return rc;
}
