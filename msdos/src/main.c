#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <conio.h>

#include "weatherdefs.h"
#include "ipapi.h"
#include "openmeteo.h"
#include "weather.h"
#include "gfx.h"

LOCATION loc;
LOCATION current;
WEATHER  wi;
FORECAST fc;
char	current_screen;
int	err;

typedef enum command {
	COM_REFRESH,
	COM_WEATHER,
	COM_FORECAST,
	COM_NONE
} COMMAND;

enum unit_option unit_opt = METRIC;

static void auto_units(const char *cc)
{
	if (strcmp(cc, "US") == 0 || strcmp(cc, "LR") == 0 || strcmp(cc, "MM") == 0)
		unit_opt = IMPERIAL;
	else
		unit_opt = METRIC;
}

char weather_menu[] = "\x01" "F" "\x01" "orecast \x01" "R" "\x01" "ef \x01" "U" "\x01" "nit \x01" "L" "\x01" "oc \x01" "Q" "\x01" "uit";
char *forecast_menu[3] = {"???",
	" \x01" "N" "\x01" "ext  \x01" "W" "\x01" "eather",
	" \x01" "B" "\x01" "ack  \x01" "W" "\x01" "eather"};

int main(void)
{
	bool	quit = false;
	char	forecast_page;
	char	ch;
	COMMAND com = COM_REFRESH;

	initGraphics();
	/* disp_icon_test(); // icon test page - uncomment for testing */
	resetScreen();

	disp_message("Fetching location data");
	get_location(&loc);
	auto_units(loc.countryCode);
	current = loc;
	forecast_page = 0;

	while (!quit) {
		switch (com) {
			case COM_REFRESH:
				disp_message("Fetching weather data");
				get_om_info(&loc, &wi, &fc);
				current_screen = SCREEN_INIT;
			case COM_WEATHER:
				disp_weather(&wi);
				disp_menu(weather_menu);
				break;
			case COM_FORECAST:
				disp_forecast(&fc, forecast_page);
				disp_menu(forecast_menu[forecast_page]);
				break;
			default:
				;
		}
		ch = getch();
		if (current_screen == SCREEN_WEATHER)
		{
			switch (ch)
			{
				case 'r':
				case 'R':
					com = COM_REFRESH;
					break;
				case 'u':
				case 'U':
					unit_opt = (unit_opt == METRIC) ? IMPERIAL : METRIC;
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
		else
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
				if (forecast_page > 2)
				{
					forecast_page = 2;
				}
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

	resetGraphics();
	return 0;
}
