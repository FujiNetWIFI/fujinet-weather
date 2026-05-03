/**
 * @brief FujiNet weather for CoCo
 * @author Thomas Cherryhomes
 * @email thom dot cherryhomes at gmail dot com
 * @license gpl v. 3, see LICENSE.md, for details.
 * @verbose Weather display
 */

#include <cmoc.h>
#include <coco.h>
#include <fujinet-fuji.h>
#include "weatherdefs.h"
#include "strutil.h"
#include "weather.h"
#include "gfx.h"
#include "broken_clouds.h"
#include "clear.h"
#include "few_clouds.h"
#include "mist.h"
#include "rain.h"
#include "scattered_clouds.h"
#include "shower_rain.h"
#include "snow.h"
#include "thunderstorm.h"

extern enum unit_option unit_opt;
extern char disp_page;
extern char current_screen;

char current_forecast_page = 0;

char c_str[] = {0x7F, 0x43, 0x00};
char f_str[] = {0x7F, 0x46,0x00};
char min_str[] = {'L','o',0x00};
char max_str[] = {'H','i', 0x00};
char precip_str[] = {'P', 'r', 0x00};

char *temp_unit[] = {c_str, f_str};
char *speed_unit[] = {" m/s", " mph"};
char *vis_unit[] = {" km", " mi"};
char *precip_unit[] = {" mm", " in"};

char *wind_deg[] = {" N", " NE", " E", " SE", " S", " SW", " W", " NW"};

#ifdef COCO3
/* Toggle the foreground slot of text_palette so subsequent text renders
 * in either label-white or data-yellow.  The 2bpp source font only ever
 * uses 0 (off) and 3 (on), so we just swap text_palette[3]. */
#define TEXT_LABEL  3   /* white */
#define TEXT_DATA   5   /* light blue */
static void use_color(unsigned char c) { text_palette[3] = c; }

/* CoCo 3: 40x25 char grid layout mirroring the MSDOS port. */
void disp_weather(WEATHER *wi)
{
	char prbuf[LINE_LEN];
	char date_buf[DATE_LEN];
	char time_buf[TIME_LEN];
	unsigned char tx, len;
	int wind_idx;
	long visi;

	if (current_screen == SCREEN_WEATHER) return;
	current_screen = SCREEN_WEATHER;
	current_forecast_page = 0;

	gfx_cls(BGCOLOR);
	draw_border();

	/* Date/time, centered on row 1 -- data. */
	strncpy(date_buf, wi->datetime, DATE_LEN-1);
	date_buf[DATE_LEN-1] = 0;
	strncpy(time_buf, wi->datetime + 11, TIME_LEN-1);
	time_buf[TIME_LEN-1] = 0;
	sprintf(prbuf, "%s %s %s", date_buf, time_buf, wi->timezone);
	len = (unsigned char)strlen(prbuf);
	tx = len < 38 ? 1 + (38 - len) / 2 : 1;
	use_color(TEXT_DATA);
	drawText(tx, 1, prbuf);

	use_color(TEXT_LABEL);
	draw_hdiv(2);

	/* Icon centered in the cols 1-7 area (matches MSDOS 7-cell icon slot). */
	put_icon(3, 3, icon_code(wi->icon));

	/* Temp double-height at (9, 3) -- data. */
	use_color(TEXT_DATA);
	sprintf(prbuf, "%s%s", wi->temp, temp_unit[unit_opt]);
	drawTextDouble(9, 3, prbuf);
	tx = 9 + (unsigned char)strlen(prbuf) + 1;
	use_color(TEXT_LABEL);
	drawText(tx, 3, "Feels");
	drawText(tx, 4, "Like:");
	tx += 6;
	use_color(TEXT_DATA);
	sprintf(prbuf, "%s%s", wi->feels_like, temp_unit[unit_opt]);
	drawTextDouble(tx, 3, prbuf);

	sprintf(prbuf, "%s hPa", wi->pressure);
	drawTextDouble(9, 5, prbuf);

	/* Description and location: keep white (not values per se). */
	use_color(TEXT_LABEL);
	decode_description(wi->icon, prbuf);
	drawText(1, 8, prbuf);

	if (strlen(wi->state) > 0)
		sprintf(prbuf, "%s, %s, %s", wi->name, wi->state, wi->country);
	else
		sprintf(prbuf, "%s, %s", wi->name, wi->country);
	drawText(1, 9, prbuf);

	draw_hdiv(10);

	use_color(TEXT_LABEL);
	drawText(2, 11, "Humidity:");
	use_color(TEXT_DATA);
	sprintf(prbuf, "%s %%", wi->humidity);
	drawText(12, 11, prbuf);

	use_color(TEXT_LABEL);
	drawText(2, 12, "Dew Point:");
	use_color(TEXT_DATA);
	sprintf(prbuf, "%s %s", wi->dew_point, temp_unit[unit_opt]);
	drawText(13, 12, prbuf);

	use_color(TEXT_LABEL);
	drawText(2, 13, "Clouds:");
	use_color(TEXT_DATA);
	sprintf(prbuf, "%s %%", wi->clouds);
	drawText(10, 13, prbuf);

	use_color(TEXT_LABEL);
	drawText(2, 14, "Visibility:");
	visi = atol(wi->visibility);
	if (unit_opt == METRIC)
		visi = (visi + 500) / 1000;
	else
		visi = (visi + 2640) / 5280;
	use_color(TEXT_DATA);
	sprintf(prbuf, "%ld%s", visi, vis_unit[unit_opt]);
	drawText(14, 14, prbuf);

	use_color(TEXT_LABEL);
	drawText(2, 15, "Wind:");
	wind_idx = (atoi(wi->wind_deg) % 360) / 45;
	sprintf(prbuf, "%s%s%s", wi->wind_speed, speed_unit[unit_opt], wind_deg[wind_idx]);
	use_color(TEXT_DATA);
	drawText(8, 15, prbuf);

	use_color(TEXT_LABEL);
	draw_hdiv(17);

	use_color(TEXT_LABEL);
	drawText(2, 18, "Sunrise:");
	use_color(TEXT_DATA);
	memset(time_buf, 0, TIME_LEN);
	strncpy(time_buf, wi->sunrise + 11, 5);
	sprintf(prbuf, "%s %s", time_buf, wi->timezone);
	drawText(11, 18, prbuf);

	use_color(TEXT_LABEL);
	drawText(2, 19, "Sunset:");
	use_color(TEXT_DATA);
	memset(time_buf, 0, TIME_LEN);
	strncpy(time_buf, wi->sunset + 11, 5);
	sprintf(prbuf, "%s %s", time_buf, wi->timezone);
	drawText(11, 19, prbuf);

	use_color(TEXT_LABEL);
	draw_hdiv(22);
}
#else
void disp_weather(WEATHER *wi)
{
	char prbuf[LINE_LEN];
	char date_buf[DATE_LEN];
	char time_buf[TIME_LEN];
	int wind_idx;
	long visi;

	if (current_screen == SCREEN_WEATHER)
	{
		return;
	}

	current_screen = SCREEN_WEATHER;
	current_forecast_page = 0;

	gfx_cls(BGCOLOR);

	//"2025-10-13T21:15" is the source string.
	strncpy(date_buf, wi->datetime, DATE_LEN-1);
	date_buf[DATE_LEN-1] = 0x00;
	strncpy(time_buf, wi->datetime + 11, TIME_LEN-1);
	time_buf[TIME_LEN-1] = 0x00;

	// 2023-12-31 18:25 is what we display.
	sprintf(prbuf, "%s %s %s", date_buf, time_buf, wi->timezone);
	puts(0, 4, PURPLE, prbuf, false);

	// weather conditions icon
	put_icon(8, 16, icon_code(wi->icon));

	// Temperature
	sprintf(prbuf, "%s%s", wi->temp, temp_unit[unit_opt]);
	puts(40, 16, WHITE, prbuf, true);


	// Apparent temp
	puts(72, 16, WHITE, "Feels", false);
	puts(72, 24, WHITE, "Like:", false);
	sprintf(prbuf, "%s%s", wi->feels_like, temp_unit[unit_opt]);
	puts(96, 16, WHITE, prbuf, true);

	// Pressure
	sprintf(prbuf, "%s%s", wi->pressure, " hPa");
	puts(40, 40, WHITE, prbuf, true);

	// Condition
	decode_description(wi->icon, prbuf);
	puts(0, 72, WHITE, prbuf, false);

	// Region
	if( strlen(wi->state) > 0 )
	{
		sprintf(prbuf, "%s, %s, %s", wi->name, wi->state, wi->country);
	}
	else
	{
		sprintf(prbuf, "%s, %s", wi->name, wi->country);
	}
	puts(0, 84, WHITE, prbuf, false);

	// Humidity
	puts(0 + 4, 100, WHITE, "Humidity:", false);
	sprintf(prbuf, "%s %%", wi->humidity);
	puts(40 + 4, 100, PURPLE, prbuf, false);

	// Dew Point
	sprintf(prbuf, "%s%s", wi->dew_point, temp_unit[unit_opt]);
	puts(0 + 4, 112, WHITE, "Dew Point:", false);
	puts(44 + 4, 112, PURPLE, prbuf, false);

	// Clouds
	puts(0 + 4, 124, WHITE, "Clouds:", false);
	sprintf(prbuf, "%s %%", wi->clouds);
	puts(32 + 4, 124, PURPLE, prbuf, false);

	// Visibility
	puts(0 + 4, 136, WHITE, "Visibility:", false);
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
	sprintf(prbuf, "%ld%s", visi, vis_unit[unit_opt]);
	puts(48 + 4, 136, PURPLE, prbuf, false);

	// Wind
	wind_idx = (atoi(wi->wind_deg) % 360) / 45;
	puts(0 + 4, 148, WHITE, "Wind:", false);
	sprintf(prbuf, "%s%s%s", wi->wind_speed, speed_unit[unit_opt], wind_deg[wind_idx]);
	puts(24 + 4, 148, PURPLE, prbuf, false);

	// Sunrise
	memset(time_buf, 0, TIME_LEN);
	strncpy(time_buf, wi->sunrise + 11, 5);
	puts(0 + 4, 160, WHITE, "Sunrise:", false);
	sprintf(prbuf, "%s %s", time_buf, wi->timezone);
	puts(36 + 4, 160, PURPLE, prbuf, false);

	// Sunset
	memset(time_buf, 0, TIME_LEN);
	strncpy(time_buf, wi->sunset + 11, 5);
	puts(0 + 4 , 172, WHITE, "Sunset:", false);
	sprintf(prbuf, "%s %s", time_buf, wi->timezone);
	puts(32 + 4, 172, PURPLE, prbuf, false);
}
#endif

#ifdef COCO3
/* Wrap a weather description into up to 3 lines of width maxw, drawing each
 * line at (x, y+row).  Mirrors the MSDOS port. */
static void draw_desc_wrapped(unsigned char x, unsigned char y, char code,
                              unsigned char maxw)
{
	char desc[LINE_LEN];
	char line[14];
	char *p, *wstart, *lp;
	unsigned char llen, wlen, row;

	decode_description(code, desc);
	if (strncmp(desc, "Thunderstorm", 12) == 0)
	{
		memcpy(desc, "T-Storm", 7);
		strcpy(desc + 7, "");
	}

	p = desc;
	row = 0;

	while (*p && row < 3)
	{
		lp = line;
		llen = 0;

		while (*p)
		{
			wstart = p;
			wlen = 0;
			while (*p && *p != ' ')
			{
				p++;
				wlen++;
			}

			if (llen == 0)
			{
				memcpy(lp, wstart, wlen);
				lp += wlen;
				llen = wlen;
			}
			else if (llen + 1 + wlen <= maxw)
			{
				*lp++ = ' ';
				memcpy(lp, wstart, wlen);
				lp += wlen;
				llen += 1 + wlen;
			}
			else
			{
				p = wstart;
				break;
			}

			if (*p == ' ')
				p++;
		}

		*lp = '\0';
		drawText(x, y + row, line);
		row++;
	}
}

/* CoCo 3: 4 days/page in 4 columns of 10 cells, mirroring the MSDOS layout. */
void disp_forecast(FORECAST *fc, char p)
{
	char i;
	char start_idx;
	char prbuf[QUARTER_LEN];
	unsigned char cx;
	int wind_idx;
	int y, m, d;

	if (p == current_forecast_page) return;
	current_screen = SCREEN_FORECAST;
	current_forecast_page = p;

	start_idx = (p - 1) * 4;
	gfx_cls(BGCOLOR);
	draw_forecast_border();

	for (i = 0; i <= 3; i++)
	{
		if ((start_idx + i) > 7) break;

		cx = (unsigned char)(1 + i * 10 + 1);

		parse_date(fc->day[i + start_idx].date, &y, &m, &d);

		/* Center each piece on column-center cx+3.  Day uses %d (no
		 * leading-space padding) so 1-digit days center cleanly. */
		sprintf(prbuf, "%d", d);
		drawText(cx + 3 - (unsigned char)strlen(prbuf) / 2, 1, prbuf);
		drawText(cx + 2, 2, monthName(fc->day[i + start_idx].date));
		put_icon(cx + 1, 3, icon_code(fc->day[i + start_idx].icon));
		drawText(cx + 2, 7, dayOfWeek(y, m, d));

		draw_desc_wrapped(cx, 9, fc->day[i + start_idx].icon, 8);

		sprintf(prbuf, "%s%s", fc->day[i+start_idx].temp_max, temp_unit[unit_opt]);
		drawText(cx, 13, prbuf);

		sprintf(prbuf, "%s%s", fc->day[i+start_idx].temp_min, temp_unit[unit_opt]);
		drawText(cx, 14, prbuf);

		sprintf(prbuf, "UV %s", fc->day[i+start_idx].uv_index_max);
		drawText(cx, 15, prbuf);

		wind_idx = (atoi(fc->day[i+start_idx].wind_deg) % 360) / 45;
		sprintf(prbuf, "W:%s", wind_deg[wind_idx]);
		drawText(cx, 17, prbuf);

		sprintf(prbuf, "%s%s", fc->day[i+start_idx].wind_speed, speed_unit[unit_opt]);
		drawText(cx, 18, prbuf);

		sprintf(prbuf, "%s%s", fc->day[i+start_idx].precipitation_sum, precip_unit[unit_opt]);
		drawText(cx, 20, prbuf);
	}
}
#else
void disp_forecast(FORECAST *fc, char p)
{
	char i;
	char start_idx;
	char tdbuf[LINE_LEN];
	char prbuf[QUARTER_LEN];
	long localtime;
	int wind_idx;
	int y, m, d;

	if (p == current_forecast_page)
	{
		return;
	}

	current_screen = SCREEN_FORECAST;
	current_forecast_page = p;

	start_idx = (p - 1) * 3;
	gfx_cls(BGCOLOR);

	//	draw header
	puts(0, 92, WHITE, max_str, false);
	puts(0, 100, WHITE, min_str, false);
	puts(0, 112, WHITE, "UV", false);
	puts(0, 144, WHITE, precip_str, false);

	for (i = 0; i <= 2; i++)
	{
		// There are only 8 days of forecast data
		// The last page has only 2 days,
		// so break out of loop if we exceed available data
		if ( (start_idx + i) > 7 )
		{
			break;
		}

		parse_date(fc->day[i + start_idx].date, &y, &m, &d);

		//   day
		sprintf(prbuf, "%2d", d);
		puts(((i * 10) +5) * 4, 8, WHITE, prbuf, false);

		//   month
		puts(((i * 10) +5) * 4, 16, WHITE, monthName(fc->day[i + start_idx].date), false);

		//   weather icon
		put_icon((i*10 +4) * 4, 26, icon_code(fc->day[i+start_idx].icon));

		//   weekday
		puts(((i * 10) +5) * 4, 80, WHITE, dayOfWeek(y, m, d), false);

		// max temp
		sprintf(prbuf, "%s%s", fc->day[i+start_idx].temp_max, temp_unit[unit_opt]);
		puts(((i * 10) +4) * 4, 92, WHITE, prbuf, false);

		// min temp
		sprintf(prbuf, "%s%s", fc->day[i+start_idx].temp_min, temp_unit[unit_opt]);
		puts(((i * 10) +4) * 4, 100, WHITE, prbuf, false);

		//   uv index max
		sprintf(prbuf, " %s ", fc->day[i+start_idx].uv_index_max);
		puts(((i * 10) +4) * 4, 112, WHITE, prbuf, false);

		// //   wind degree
		wind_idx = (atoi(fc->day[i+start_idx].wind_deg) % 360) / 45;
		sprintf(prbuf, "Wind:%s", wind_deg[wind_idx]);
		puts(((i*10)+3) * 4, 124, WHITE, prbuf, false);

		//   wind speed
		sprintf(prbuf, "%s%s", fc->day[i+start_idx].wind_speed, speed_unit[unit_opt]);
		puts(((i*10)+3) * 4, 132, WHITE, prbuf, false);

		//   precipitation sum
		sprintf(prbuf, "%s%s", fc->day[i+start_idx].precipitation_sum, precip_unit[unit_opt]);
		puts(((i*10)+3) * 4, 144, WHITE, prbuf, false);
	}
}
#endif

//
// decode description string from weather code
//
void decode_description(char code, char *buf) 
{
	switch (code) 
  {
		case 0:
			strcpy(buf, "Clear sky");
			break;
		case 1:
			strcpy(buf, "Mainly clear");
			break;
		case 2:
		 	strcpy(buf, "Partly cloudy");
            break;
		case 3:
		 	strcpy(buf, "Cloudy");
            break;
		case 45:
		 	strcpy(buf, "Fog");
            break;
		case 48:
		 	strcpy(buf, "Depositing rime fog");
            break;
		case 51:
		 	strcpy(buf, "Drizzle light");
            break;
		case 53:
		 	strcpy(buf, "Drizzle moderate");
            break;
		case 55:
		 	strcpy(buf, "Drizzle dense intensity");
            break;
		case 56:
		 	strcpy(buf, "Freezing Drizzle light");
            break;
		case 57:
		 	strcpy(buf, "Freezing Drizzle dense intensity");
            break;
		case 61:
		 	strcpy(buf, "Rain slight");
            break;
		case 63:
		 	strcpy(buf, "Rain moderate");
            break;
		case 65:
		 	strcpy(buf, "Rain heavy intensity");
            break;
		case 66:
		 	strcpy(buf, "Freezing rain light");
            break;
		case 67:
		 	strcpy(buf, "Freezing rain heavy intensity");
            break;
		case 71:
		 	strcpy(buf, "Snow fall slight");
            break;
		case 73:
		 	strcpy(buf, "Snow fall moderate");
            break;
		case 75:
		 	strcpy(buf, "Snow fall heavy intensity");
            break;
		case 77:
		 	strcpy(buf, "Snow grains");
            break;
		case 80:
		 	strcpy(buf, "Rain showers slight");
            break;
		case 81:
		 	strcpy(buf, "Rain showers moderate");
            break;
		case 82:
		 	strcpy(buf, "Rain showers violent");
            break;
		case 85:
		 	strcpy(buf, "Snow showers slight");
            break;
		case 86:
		 	strcpy(buf, "Snow showers heavy");
            break;
		case 95:
		case 96:
		case 99:
		 	strcpy(buf, "Thunderstorm");
            break;
		default:
		 	strcpy(buf, "???");
	}
}

//
// decode icon from weather code
//
#ifdef COCO3
/* CoCo 3: returns an index 0..8 into charset_coco3_icons.
 * Order: 0 clear, 1 few_clouds, 2 scattered, 3 broken, 4 shower_rain,
 *        5 rain, 6 thunderstorm, 7 snow, 8 fog. */
unsigned char icon_code(char code)
{
	switch (code)
	{
		case 0:  return 0;
		case 1:  return 1;
		case 2:  return 2;
		case 3:  return 3;
		case 56: case 57: case 80: case 81: case 82:
			return 4;
		case 51: case 53: case 55: case 61: case 63:
		case 65: case 66: case 67:
			return 5;
		case 95: case 96: case 99:
			return 6;
		case 71: case 73: case 75: case 77: case 85: case 86:
			return 7;
		case 45: case 48:
			return 8;
		default:
			return 1;
	}
}
#else
byte * icon_code(char code)
{
	byte *result;

	switch (code)
	{
// clear
		case	0:
			result = clear;
			break;
// mainly clear
		case	1:
			result = few_clouds;
			break;
// partly cloudy
		case	2:
			result = scattered_clouds;
			break;
// cloud
		case	3:
			result = broken_clouds;
			break;
// rain showers
		case	56:
		case	57:
		case	80:
		case	81:
		case	82:
			result = shower_rain;
			break;
// drizzle, rain
		case	51:
		case	53:
		case	55:
		case	61:
		case	63:
		case	65:
		case	66:
		case	67:
			result = rain;
			break;
// thunderstorm
		case	95:
		case	96:
		case	99:
			result = thunderstorm;
			break;
// snow
		case	71:
		case	73:
		case	75:
		case	77:
		case	85:
		case	86:
			result = snow;
			break;
// fog
		case	45:
		case	48:
			result = mist;
			break;
		default:
			result = few_clouds;
	}
	return (result);
}
#endif

