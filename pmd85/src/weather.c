/**
 * @brief FujiNet weather for CoCo
 * @author Thomas Cherryhomes
 * @email thom dot cherryhomes at gmail dot com
 * @license gpl v. 3, see LICENSE.md, for details.
 * @verbose Weather display
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <conio.h>
#include <fujinet-fuji.h>

#include "weatherdefs.h"
#include "strutil.h"
#include "weather.h"
#include "gfx.h"
#include "draw.h"
#include "bitmap.h"
#include "utf.h"

extern enum unit_option unit_opt;
extern char disp_page;
extern char current_screen;

char current_forecast_page = 0;

// char min_str[] = "Lo";
// char max_str[] = "Hi";
// char precip_str[] = "Pr";

char *temp_unit[] = {"^C", "^F"};
char *speed_unit[] = {"m/s", "mph"};
char *vis_unit[] = {"km", "mi"};
char *precip_unit[] = {"mm", "in"};

char *wind_deg[] = {"N", "NE", "E", "SE", "S", "SW", "W", "NW"};

void put_text(unsigned short x, unsigned short y, const char *s)
{
	set_position(x, y);
	draw_text(s);
}

void put_text_2(unsigned short x, unsigned short y, const char *s1, const char *s2)
{
	set_position(x, y);
	draw_text(s1);
	draw_text(s2);
}

void put_text_3(unsigned short x, unsigned short y, const char *s1, const char *s2, const char *s3)
{
	put_text_2(x, y, s1, s2);
	draw_text(s3);
}

void put_bitmap(unsigned short x, unsigned short y, const unsigned char *bmp)
{
	set_position(x, y);
	draw_bitmap(bmp);
}

char * strceil(char *s)
{
	char *p = s;
	while (*p) {
		if (*p == '.') {
			*p = '\0';
			break;
		}
		p++;
	}
	return s;
}

void put_temp(const char *temp)
{
	char c;
	unsigned short xpos, ypos;

	for(char i=0; i<5; i++) {
		c = *temp++;
		switch (c)
		{
		case '0':
		case '1':
		case '2':
		case '3':
		case '4':
		case '5':
		case '6':
		case '7':
		case '8':
		case '9':
			draw_bitmap(digit_bitmaps[c - '0']);
			break;
		case '.':
			get_position(&xpos, &ypos);
			put_bitmap(xpos, ypos+34, bmp_dot);
			set_position(xpos+12, ypos);
			break;
		case '-':
			get_position(&xpos, &ypos);
			put_bitmap(xpos, ypos+22, bmp_dot);
			set_position(xpos+12, ypos);
			break;
		default:
			c = 0;
		}
		if (!c)	// stop drawing on nul or any unknown char
			break;
	}
	// Degree symbol
	draw_bitmap(bmp_degree);
	// Unit
	if (unit_opt == IMPERIAL) {
    	draw_bitmap(bmp_fahrenheit);	// F
	} else {
		draw_bitmap(bmp_celsius);    	// C
	}
}


void disp_weather(WEATHER *wi)
{
	char prbuf[2 * LINE_LEN + QUARTER_LEN]; // 2*LINE_LEN+QUARTER_LEN for "City, State, Country"
	int wind_idx;
	long visi;
	int y;
	char m, d;

	if (current_screen == SCREEN_WEATHER) return;

	current_screen = SCREEN_WEATHER;
	current_forecast_page = 0;

	clear_screen();

	// "2025-10-13T21:15" is the source string
	// Convert 2026-09-02 to "2 Oct 2026, Fri"
	parse_date(wi->datetime, &y, &m, &d);
	sprintf(prbuf, "%d %s %d, %s", d, monthName(m), y, dayOfWeek(y, m, d));
	set_color(MONO_GRAY);
	set_position(18*8-(strlen(prbuf)<<3), 16);
	draw_text_double(prbuf);
	// Time Zone and Time
	put_text_2(8, 220, "Time Zone: ", wi->timezone);
	put_text_2(21*8, 220, "Time:    ", &wi->datetime[DATE_LEN]);

	// Weather conditions icon
	put_bitmap(6, 52, get_weather_icon(wi->icon, wi->is_day));

	// Temperature
	set_position(90, 48);
	put_temp(wi->temp);

	// Pressure
	set_color(MONO_WHITE);
	strceil(wi->pressure);
    set_position(35*8-(strlen(wi->pressure)<<4), 56);
    draw_text_double(wi->pressure);
    set_color(MONO_GRAY);
    set_position(29*8, 72);
    draw_text_double("hPa");

	// Condition
	decode_description(wi->icon, prbuf);
	prbuf[18] = '\0';	// limit, 18 double width chars fill the screen
	set_color(MONO_WHITE);
	// centered
	set_position(18*8-(strlen(prbuf)<<3), 112);
	draw_text_double(prbuf);

	// Region
	strcpy(prbuf, wi->name);
	if (strcmp(wi->country, "US") == 0 || strcmp(wi->country, "CA") == 0)
	{
		strcat(strcat(prbuf, ", "), wi->state); // append ", state"
	}
	strcat(strcat(prbuf, ", "), wi->country);   // append ", country"
	prbuf[36] = '\0';	// limit, 36 chars fill the screen
	set_color(MONO_GRAY);
	// centered
	put_text(18*8-(strlen(prbuf)<<2), 136, prbuf);

	// Wind
	wind_idx = ((atoi(wi->wind_deg) + 22) / 45) % 8;
	sprintf(prbuf, "%s %s %s", wi->wind_speed, speed_unit[unit_opt], wind_deg[wind_idx]);
	set_color(MONO_WHITE);
	put_text_2(8, 160, "Wind: ", prbuf);

	// Apparent temp
	put_text_3(8, 170, "Feels Like: ", wi->feels_like, temp_unit[unit_opt]);

	// Dew Point
	put_text_3(8, 190, "Dew Point:  ", wi->dew_point, temp_unit[unit_opt]);

	// Visibility
	visi = (atol(wi->visibility));
	if (unit_opt == METRIC)
	{
		// Convert meters to kilometers, rounded to nearest
		visi = (visi + 500) / 1000;
	}
	else
	{
		// Convert feet to miles, rounded to nearest
		visi = (visi + 2640) / 5280;
	}
	// Model visibility often reads 25-50 km in clear air; cap it at 10 km / 10 mi
	// like airport reports (METAR) do, which is what people are used to seeing
	if (visi > 10)
	{
		visi = 10;
	}
	sprintf(prbuf, "%ld%s", visi, vis_unit[unit_opt]);
	put_text_2(8, 200, "Visibility: ", prbuf);

	// Sunrise
	put_text_2(21*8, 160, "Sunrise: ", strlen(wi->sunrise)>10 ? &wi->sunrise[11] : "");

	// // Sunset
	put_text_2(21*8, 170, "Sunset:  ", strlen(wi->sunset)>10 ? &wi->sunset[11] : "");
	// Humidity
	put_text_3(21*8, 190, "Humidity: ", wi->humidity, "%");

	// Clouds
	put_text_3(21*8, 200, "Clouds:   ", wi->clouds, "%");
}

// snow fall, snow grains and snow showers (WMO weather codes)
static char is_snow(char code)
{
	return (code >= 71 && code <= 77) || code == 85 || code == 86;
}

void disp_forecast(FORECAST *fc, char p)
{
	char i;
	char start_idx;
	char prbuf[QUARTER_LEN];
	unsigned short cx;
	int wind_idx;
	int y;
	char m, d;
	DAYINFO *day;

	if (p == current_forecast_page) return;
	current_screen = SCREEN_FORECAST;
	current_forecast_page = p;

	start_idx = (p - 1) * 4;
	clear_screen();

	for (i = 0; i <= 3; i++)
	{
		if ((start_idx + i) > 7) break;

		day = &fc->day[i + start_idx];
		cx = (unsigned short)(i * 72);

		parse_date(day->date, &y, &m, &d);

		// Day
		sprintf(prbuf, "%d", d);
		set_position(cx+36-(strlen(prbuf)<<3), 16);
		set_color(MONO_WHITE);
		draw_text_double(prbuf);
		// Month
		set_position(cx+12, 32);
		set_color(MONO_GRAY);
		draw_text_double(monthName(m));
		// Weather icon
		put_bitmap(cx, 52, get_weather_icon(day->icon, 1));
		// Weekday
		set_position(cx+12, 112);
		draw_text_double(dayOfWeek(y, m, d));

		//draw_desc_wrapped(cx, 9, day->icon, 8);

		// Temperature max
		set_color(MONO_WHITE);
		put_text_2(cx+8, 136, day->temp_max, temp_unit[unit_opt]);
		// Temperature min
		set_color(MONO_GRAY);
		put_text_2(cx+8, 146, day->temp_min, temp_unit[unit_opt]);
		// UV index
		set_color(MONO_WHITE);
		put_text_2(cx+8, 166, "UV ", day->uv_index_max);

		// wind_deg is where the wind blows from; +4 (180 deg) makes the arrow
		// point with the flow. Font chars 0..7 are arrows N, NE .. NW.
		wind_idx = ((atoi(day->wind_deg) + 22) / 45 + 4) % 8;
		put_text(cx+8, 180, "Wind: ");
		draw_char((unsigned char)wind_idx);
		// Wind speed
		put_text_2(cx+8, 190, day->wind_speed, speed_unit[unit_opt]);

		// Precipitation probability and sum, shown together when some precipitation
		// is expected or the chance is at least 30% (smaller chances are just noise);
		// snow flake (\013) on snow days, rain drop (\012) otherwise
		if (strcmp(day->precipitation_sum, "0") || atoi(day->precip_prob) >= 30) {
			put_text_3(cx+8, 204, is_snow(day->icon) ? "\013 " : "\012 ", day->precip_prob, "%");
			put_text_2(cx+8, 214, day->precipitation_sum, precip_unit[unit_opt]);
		}
	}
}

//
// decode description string from weather code
//
void decode_description(char code, char *buf)
{
	// OpenWeather wording (as shown by the Atari version), lower case label style,
	// max 18 chars to fit the double width line on the weather screen
	const char *s;

	switch (code)
	{
		case 0:  s = "clear sky"; break;
		case 1:  s = "few clouds"; break;
		case 2:  s = "scattered clouds"; break;
		case 3:  s = "overcast clouds"; break;
		case 45: s = "fog"; break;
		case 48: s = "freezing fog"; break;
		case 51: s = "light drizzle"; break;
		case 53: s = "drizzle"; break;
		case 55: s = "heavy drizzle"; break;
		case 56: s = "freezing drizzle"; break;
		case 57: s = "heavy frz drizzle"; break;
		case 61: s = "light rain"; break;
		case 63: s = "moderate rain"; break;
		case 65: s = "heavy rain"; break;
		case 66: s = "freezing rain"; break;
		case 67: s = "heavy frz rain"; break;
		case 71: s = "light snow"; break;
		case 73: s = "snow"; break;
		case 75: s = "heavy snow"; break;
		case 77: s = "snow grains"; break;
		case 80: s = "light shower rain"; break;
		case 81: s = "shower rain"; break;
		case 82: s = "heavy shower rain"; break;
		case 85: s = "light shower snow"; break;
		case 86: s = "heavy shower snow"; break;
		case 95: s = "thunderstorm"; break;
		case 96:
		case 99: s = "storm with hail"; break;
		default: s = "unknown";
	}
	strcpy(buf, s);
}

unsigned char * get_weather_icon(char code, char is_day)
{
	unsigned char *result;

	switch (code)
	{
// clear (night: moon)
		case	0:
			result = is_day ? bmp_weather0 : bmp_weather9;
			break;
// mainly clear (night: moon)
		case	1:
			result = is_day ? bmp_weather1 : bmp_weather10;
			break;
// partly cloudy
		case	2:
			result = is_day ? bmp_weather2 : bmp_weather11;
			break;
// cloud
		case	3:
			result = bmp_weather3;
			break;
// drizzle
		case	51:
		case	53:
		case	55:
		case	56:
		case	57:
		case	80:
		case	81:
		case	82:
			result = bmp_weather4;
			break;
// rain
		case	61:
		case	63:
		case	65:
			result = bmp_weather5;
			break;
// thunderstorm
		case	95:
		case	96:
		case	99:
			result = bmp_weather6;
			break;
// snow, freezing rain
		case	66:
		case	67:
		case	71:
		case	73:
		case	75:
		case	77:
		case	85:
		case	86:
			result = bmp_weather7;
			break;
// fog
		case	45:
		case	48:
			result = bmp_weather8;
			break;
		default:
			result = bmp_weather1;
	}
	return (result);
}

