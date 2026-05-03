/**
 * @brief   weather
 * @author  Shunichi Kitahara
 * @license gpl v. 3
 * @verbose main
 */

#include <cmoc.h>
#include <coco.h>

#include "weatherdefs.h"
#include "ipapi.h"
#include "openmeteo.h"
#include "weather.h"
#include "gfx.h"

#ifdef COCO3
const char build_target[] = "coco3";
#else
const char build_target[] = "coco";
#endif

LOCATION loc;
LOCATION current;
WEATHER  wi;
FORECAST fc;
char	current_screen;

typedef enum command {
	COM_REFRESH,
	COM_WEATHER,
	COM_FORECAST,
	COM_NONE
} COMMAND;

enum unit_option unit_opt = IMPERIAL;

static void auto_units(const char *cc)
{
	if (strcmp(cc, "US") == 0 || strcmp(cc, "LR") == 0 || strcmp(cc, "MM") == 0)
		unit_opt = IMPERIAL;
	else
		unit_opt = METRIC;
}

/* \001 toggles inverse video around the active key letter. On CoCo 1/2 the
 * toggles are skipped so the strings still read sensibly.
 * Octal (\001) is used instead of hex (\x01) because cmoc concatenates
 * adjacent string literals before parsing escapes, so "\x01""F" would
 * incorrectly merge into a single byte \x1F (F eaten as a hex digit). */
char weather_menu[] =
  "\001F\001orecast \001R\001ef "
  "\001U\001nit \001L\001oc \001Q\001uit";
#ifdef COCO3
/* CoCo 3: 2 pages of 4 days, mirroring MSDOS. */
char *forecast_menu[3] = {
  "???",
  " \001N\001ext  \001W\001eather",
  " \001B\001ack  \001W\001eather"};
#else
char *forecast_menu[4] = {
  "???",
  " \001N\001ext  \001W\001eather",
  " \001N\001ext  \001B\001ack  \001W\001eather",
  " \001B\001ack  \001W\001eather"};
#endif

int main(void)
{
	bool	quit = false;
	char	forecast_page;
	char	ch;
	COMMAND com = COM_REFRESH;

	// Initialize graphics and clear screen
    gfx(1);
    gfx_cls(BGCOLOR);

	disp_message("  Fetching location data...");
	get_location(&loc);
	auto_units(loc.countryCode);
	current = loc;
	forecast_page = 0;

	while (!quit) {
		switch (com) {
			case COM_REFRESH:		// refresh
				disp_message("  Fetching weather data...");
				get_om_info(&loc, &wi, &fc);
				current_screen = SCREEN_INIT;
			case COM_WEATHER:		// weather
				disp_weather(&wi);
				disp_menu(weather_menu);
				break;
			case COM_FORECAST:		// forecast
				disp_forecast(&fc, forecast_page);
				disp_menu(forecast_menu[forecast_page]);
				break;
			default:
				;
		}
		ch = waitkey(0);
		if (current_screen == SCREEN_WEATHER) // weather
		{			
			switch (ch) 
			{
				case 'r':
				case 'R':
					com = COM_REFRESH;
					break;
				case 'u':
				case 'U':
					unit_opt = (unit_opt ==  METRIC) ? IMPERIAL : METRIC;
					com = COM_REFRESH;
					break;
				case 'l':
				case 'L':
					change_location(&loc);
					auto_units(loc.countryCode);
					com = COM_REFRESH;
					break;
				case 'q':
				case 'Q':
					quit = true;
					break;
				case 'f':
				case 'F':
					com = COM_FORECAST;
					forecast_page = 1;
					break;
				default:
					com = COM_NONE;
			}
		}
		else // forecast
		{								
			switch (ch) 
			{
			case 'w':
			case 'W':
				com = COM_WEATHER;
				break;
			case 'n':
			case 'N':
				com = COM_FORECAST;
				forecast_page++;
#ifdef COCO3
				if (forecast_page > 2) forecast_page = 2;
#else
				if (forecast_page > 3) forecast_page = 3;
#endif
				break;
			case 'b':
			case 'B':
				com = COM_FORECAST;
				forecast_page--;
				if (forecast_page < 1)
				{
					forecast_page = 1;
				}
				break;
			default:
				com = COM_NONE;
			}
		}
	}
	// Restore graphics + reset FujiNet, then cold-start the CoCo so it
	// reboots and FujiNet's autorun.dsk (the config) gets loaded.
	gfx_shutdown();
	fuji_reset();
	coldStart();
	return 0;
}
