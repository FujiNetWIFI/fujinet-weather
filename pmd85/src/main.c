/**
 * @brief   weather
 * @author  Shunichi Kitahara
 * @license gpl v. 3
 * @verbose main
 */

#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <conio.h>

#include "ipapi.h"
#include "openmeteo.h"
#include "weather.h"
#include "gfx.h"
#include "draw.h"

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

enum unit_option unit_opt = METRIC;

static void auto_units(const char *cc)
{
	if (strcmp(cc, "US") == 0 || strcmp(cc, "LR") == 0 || strcmp(cc, "MM") == 0)
		unit_opt = IMPERIAL;
	else
		unit_opt = METRIC;
}

// char weather_menu[] = "F)orecast R)ef U)nit L)oc Q)uit         ";

// char *forecast_menu[3] = {"???", " N)ext  W)eather                 ",
//                          " B)ack  W)eather                 "};

int main(void)
{
	bool	quit = false;
	char	forecast_page;
	char	ch;
	COMMAND com = COM_REFRESH;

	disp_message("Fetching location");
	get_location(&loc);
	current = loc;
	forecast_page = 0;

	while (!quit) {
		switch (com) {
			case COM_REFRESH:		// refresh
				disp_message("Fetching weather");
				get_om_info(&loc, &wi, &fc);
				current_screen = SCREEN_INIT;
			case COM_WEATHER:		// weather
				disp_weather(&wi);
				disp_weather_menu();
				break;
			case COM_FORECAST:		// forecast
				disp_forecast(&fc, forecast_page);
				disp_forecast_menu(forecast_page);
				break;
			default:
				;
		}
		ch = get_keyin();
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
					if (!change_location(&loc)) {
						current_screen = SCREEN_INIT; // force redraw of weather screen
						com = COM_WEATHER;
					} else {
						auto_units(loc.countryCode);
						com = COM_REFRESH;
					}
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
			case 'b':
			case 'B':
				com = COM_WEATHER;
				break;
			case 'n':
			case 'N':
				com = COM_FORECAST;
				forecast_page = 2;
				break;
			case 'p':
			case 'P':
				com = COM_FORECAST;
				forecast_page = 1;
				break;
			default:
				com = COM_NONE;
			}
		}
	}

	clear_screen();
	return 0;
}
