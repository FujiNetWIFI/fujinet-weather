#include <stdio.h>
#include <string.h>

#include "display.h"
#include "meteo.h"
#include "timeutil.h"

#ifdef HOST_TEST
#include <unistd.h>
#define WRITE(h, s, n) write((int)(h), (s), (n))
#else
#include <proto/dos.h>
#define WRITE(h, s, n) Write((BPTR)(h), (APTR)(s), (n))
#endif

/* Amiga console pens on the Workbench 1.3 palette. */
#define PEN_BG     "30"   /* blue   */
#define PEN_TEXT   "31"   /* white  */
#define PEN_DARK   "32"   /* black  */
#define PEN_ACCENT "33"   /* orange */

#define ROW_TITLE    1
#define ROW_ICON     3
#define ROW_FORECAST 9
#define ROW_MENU     19
#define ROW_STATUS   20

static long handle;
static int ansi;
static char line[128];

void display_init(long h, int use_ansi)
{
	handle = h;
	ansi = use_ansi;
}

void out(const char *s)
{
	WRITE(handle, s, (long)strlen(s));
}

/* ANSI helpers; all expand to nothing in plain-text mode. */
static void sgr(const char *codes)
{
	if (ansi) {
		out("\x1b[");
		out(codes);
		out("m");
	}
}

static void move_to(int row)
{
	if (ansi) {
		snprintf(line, sizeof line, "\x1b[%d;1H\x1b[K", row);   /* column 1, clear line */
		out(line);
	}
}

/* 5 x 11 ASCII icons, like the art on the other ports' screens. */
enum { ICON_SUN, ICON_PARTLY, ICON_CLOUD, ICON_FOG, ICON_RAIN, ICON_SNOW, ICON_STORM };
static const char *const icons[][5] = {
	{"  \\   /    ", "   .-.     ", "- (   ) -  ", "   `-'     ", "  /   \\    "},
	{"  \\  /     ", "_ /\"\".-.   ", "  \\_(   ). ", "  /(___(__)", "           "},
	{"           ", "    .--.   ", " .-(    ). ", "(___.__)__)", "           "},
	{"           ", " _ - _ - _ ", "  _ - _ - _", " _ - _ - _ ", "           "},
	{"    .-.    ", "   (   ).  ", "  (___(__) ", "   ' ' ' ' ", "  ' ' ' '  "},
	{"    .-.    ", "   (   ).  ", "  (___(__) ", "   *  *  * ", "  *  *  *  "},
	{"    .-.    ", "   (   ).  ", "  (___(__) ", "    ,/ ,/  ", "   /  /    "},
};

static int icon_for(int code)
{
	if (code <= 1)
		return ICON_SUN;
	if (code == 2)
		return ICON_PARTLY;
	if (code == 3)
		return ICON_CLOUD;
	if (code == 45 || code == 48)
		return ICON_FOG;
	if ((code >= 71 && code <= 77) || code == 85 || code == 86)
		return ICON_SNOW;
	if (code >= 95)
		return ICON_STORM;
	return ICON_RAIN;
}

static const char *icon_pen(int icon)
{
	switch (icon) {
	case ICON_SUN:   return PEN_ACCENT;
	case ICON_RAIN:
	case ICON_STORM: return PEN_DARK;
	default:         return PEN_TEXT;
	}
}

/* buf holds at least 8 bytes. */
static void hhmm(long unix_utc, long offset, char *buf)
{
	LTIME t;

	to_ltime(unix_utc, offset, &t);
	snprintf(buf, 8, "%02d:%02d", t.hour, t.min);
}

void show_weather(const WEATHER *w)
{
	const CURRENT *c = &w->now;
	const char *tu = temp_unit(w->units);
	char info[5][80];
	char rise[8], set[8];
	LTIME t;
	int icon = icon_for(c->code);
	int i;

	if (ansi)
		out("\x0c");          /* form feed clears an Amiga console */

	/* Title line: place on the left, local time on the right. */
	to_ltime(c->time, c->utc_offset, &t);
	move_to(ROW_TITLE);
	sgr("1;" PEN_ACCENT);
	snprintf(line, sizeof line, " FujiNet Weather ");
	out(line);
	sgr("0;" PEN_TEXT);
	snprintf(line, sizeof line, " %s, %s   %s %d %s %d  %02d:%02d %s\n", w->loc.city, w->loc.country,
	        wday_name(t.wday), t.mday, month_name(t.month), t.year, t.hour, t.min,
	        c->tz_abbr);
	out(line);
	if (!ansi)
		out("\n");

	hhmm(w->day[0].sunrise, c->utc_offset, rise);
	hhmm(w->day[0].sunset, c->utc_offset, set);
	snprintf(info[0], sizeof info[0], "%s", wmo_text(c->code));
	snprintf(info[1], sizeof info[1], "Temperature %s%s   Feels like %s%s", c->temp, tu, c->feels_like, tu);
	snprintf(info[2], sizeof info[2], "Humidity %s%%   Dew point %s%s   Clouds %s%%", c->humidity,
	        c->dew_point, tu, c->clouds);
	snprintf(info[3], sizeof info[3], "Pressure %s hPa   Wind %s %s %s", c->pressure, c->wind_speed,
	        speed_unit(w->units), wind_dir(c->wind_deg));
	snprintf(info[4], sizeof info[4], "Sunrise %s   Sunset %s", rise, set);

	for (i = 0; i < 5; ++i) {
		move_to(ROW_ICON + i);
		out("  ");
		sgr(icon_pen(icon));
		out(icons[icon][i]);
		sgr(i == 0 ? "1;" PEN_TEXT : "0;" PEN_TEXT);
		out("   ");
		out(info[i]);
		sgr("0");
		out("\n");
	}

	move_to(ROW_FORECAST - 1);
	out("\n");
	move_to(ROW_FORECAST);
	sgr("1;" PEN_ACCENT);
	snprintf(line, sizeof line, " %-10s %-22s %6s %6s %8s %4s  %s\n", "Day", "Weather", "High", "Low",
	        "Precip", "UV", "Wind");
	out(line);
	sgr("0;" PEN_TEXT);
	for (i = 0; i < DAYS; ++i) {
		const DAY *d = &w->day[i];
		char when[16];

		/* Daily times are local midnight; add the offset back for the date. */
		to_ltime(d->time, c->utc_offset, &t);
		if (i == 0)
			strcpy(when, "Today");
		else
			snprintf(when, sizeof when, "%s %2d", wday_name(t.wday), t.mday);
		move_to(ROW_FORECAST + 1 + i);
		snprintf(line, sizeof line, " %-10s %-22.22s %6s %6s %6s%-2s %4s  %s %s %s\n", when,
		        wmo_text(d->code), d->temp_max, d->temp_min, d->precip,
		        precip_unit(w->units), d->uv_max, d->wind_speed, speed_unit(w->units),
		        wind_dir(d->wind_deg));
		out(line);
	}
}

void show_menu(const WEATHER *w)
{
	if (!ansi)
		return;
	move_to(ROW_MENU);
	sgr("1;" PEN_ACCENT);
	out(" R");
	sgr("0;" PEN_TEXT);
	out(")efresh  ");
	sgr("1;" PEN_ACCENT);
	out("U");
	sgr("0;" PEN_TEXT);
	out(w->units == IMPERIAL ? ")nits: imperial  " : ")nits: metric  ");
	sgr("1;" PEN_ACCENT);
	out("L");
	sgr("0;" PEN_TEXT);
	out(")ocation  ");
	sgr("1;" PEN_ACCENT);
	out("Q");
	sgr("0;" PEN_TEXT);
	out(")uit");
}

void show_status(const char *msg)
{
	if (ansi) {
		move_to(ROW_STATUS);
		sgr(PEN_DARK);
		out(" ");
		out(msg);
		sgr("0");
	} else if (msg[0]) {
		out(msg);
		out("\n");
	}
}
