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
char *speed_unit[] = {" m/s", "mph"};
char *vis_unit[] = {" km", " mi"};

char *wind_deg[] = {" N", " NE", " E", " SE", " S", " SW", " W", " NW"};

void disp_weather(WEATHER *wi)
{
	char prbuf[LINE_LEN];
	char date_buf[DATE_LEN];
	char time_buf[TIME_LEN];
	int wind_idx;
	long visi;
	float visi_tmp_km;
	float visi_tmp_mi;

	if (current_screen == SCREEN_WEATHER)
	{
		return;
	}

	current_screen = SCREEN_WEATHER;
	current_forecast_page = 0;

	gfx_cls(CYAN);

	//"2025-10-13T21:15" is the source string.
	strncpy(date_buf, wi->datetime, DATE_LEN-1);
	date_buf[DATE_LEN-1] = 0x00;
	strncpy(time_buf, wi->datetime + 11, TIME_LEN-1);
	time_buf[TIME_LEN-1] = 0x00;

	// 2023-12-31 18:25 is what we display.
	sprintf(prbuf, "%s %s", date_buf, time_buf);
	puts(0, 4, PURPLE, prbuf);
	

	// weather conditions icon
	put_icon(8, 16, icon_code(wi->icon));
	
	// Temperature
	sprintf(prbuf, "%s%s", wi->temp, temp_unit[unit_opt]);
	puts_dbl(40, 16, WHITE, prbuf);
	
	// Pressure
	sprintf(prbuf, "%s%s", wi->pressure, " hPa");
	puts_dbl(40, 40, WHITE, prbuf);
	
	// Condition
	decode_description(wi->icon, prbuf);
	puts(0, 72, WHITE, prbuf);
	
	// Region
	if( strlen(wi->state) > 0 )
	{
		sprintf(prbuf, "%s, %s, %s", wi->name, wi->state, wi->country);
	}
	else
	{
		sprintf(prbuf, "%s, %s", wi->name, wi->country);
	}
	puts(0, 84, WHITE, prbuf);
	
	// Humidity
	puts(0 + 4, 100, WHITE, "Humidity:");
	sprintf(prbuf, "%s %%", wi->humidity);
	puts(40 + 4, 100, PURPLE, prbuf);
	
	// Dew Point
	sprintf(prbuf, "%s%s", wi->dew_point, temp_unit[unit_opt]);
	puts(0 + 4, 112, WHITE, "Dew Point:");
	puts(44 + 4, 112, PURPLE, prbuf);
	
	// Clouds
	puts(0 + 4, 124, WHITE, "Clouds:");
	sprintf(prbuf, "%s %%", wi->clouds);
	puts(32 + 4, 124, PURPLE, prbuf);
	
	// Visibility
	puts(0 + 4, 136, WHITE, "Visibility:");
	visi_tmp_km = (double)atol(wi->visibility) / 1000.0;
	if (unit_opt == METRIC) 
	{
		visi = (long) (visi_tmp_km + 0.5); // round to nearest
	}
	else 
	{
		visi_tmp_mi = visi_tmp_km * 0.621371;
		visi = (long) (visi_tmp_mi + 0.5); // round to nearest
	}
	sprintf(prbuf, "%ld%s", visi, vis_unit[unit_opt]);
	puts(48 + 4, 136, PURPLE, prbuf);
	
	// Wind
	wind_idx = (atoi(wi->wind_deg) % 360) / 45;
	puts(0 + 4, 148, WHITE, "Wind:");
	sprintf(prbuf, "%s%s%s", wi->wind_speed, speed_unit[unit_opt], wind_deg[wind_idx]);
	puts(24 + 4, 148, PURPLE, prbuf);
	
	// Sunrise
	memset(time_buf, 0, TIME_LEN);
	strncpy(time_buf, wi->sunrise + 11, 5);
	puts(0 + 4, 160, WHITE, "Sunrise:");
	puts(36 + 4, 160, PURPLE, time_buf);
	
	// Sunset
	memset(time_buf, 0, TIME_LEN);
	strncpy(time_buf, wi->sunset + 11, 5);
	puts(0 + 4 , 172, WHITE, "Sunset:");
	puts(32 + 4, 172, PURPLE, time_buf);
}

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
	gfx_cls(CYAN);

	//	draw header
	puts(0, 92, WHITE, max_str);
	puts(0, 100, WHITE, min_str);
	puts(0, 112, WHITE, "UV");
	puts(0, 144, WHITE, precip_str);

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
		puts(((i * 10) +5) * 4, 8, WHITE, prbuf);

		//   month
		puts(((i * 10) +5) * 4, 16, WHITE, monthName(fc->day[i + start_idx].date));

		//   weather icon
		put_icon((i*10 +4) * 4, 26, icon_code(fc->day[i+start_idx].icon));

		//   weekday
		puts(((i * 10) +5) * 4, 80, WHITE, dayOfWeek(fc->day[i + start_idx].date));

		// max temp
		sprintf(prbuf, "%s%s", fc->day[i+start_idx].temp_max, temp_unit[unit_opt]);
		puts(((i * 10) +4) * 4, 92, WHITE, prbuf);

		// min temp
		sprintf(prbuf, "%s%s", fc->day[i+start_idx].temp_min, temp_unit[unit_opt]);
		puts(((i * 10) +4) * 4, 100, WHITE, prbuf);

		//   uv index max
		sprintf(prbuf, " %s ", fc->day[i+start_idx].uv_index_max);
		puts(((i * 10) +4) * 4, 112, WHITE, prbuf);

		// //   wind degree
		wind_idx = (atoi(fc->day[i+start_idx].wind_deg) % 360) / 45;
		sprintf(prbuf, "Wind:%s", wind_deg[wind_idx]);
		puts(((i*10)+3) * 4, 124, WHITE, prbuf);

		//   wind speed
		sprintf(prbuf, "%s%s", fc->day[i+start_idx].wind_speed, speed_unit[unit_opt]);
		puts(((i*10)+3) * 4, 132, WHITE, prbuf);

		//   precipitation sum
		sprintf(prbuf, "%s mm", fc->day[i+start_idx].precipitation_sum);
		puts(((i*10)+5) * 4, 144, WHITE, prbuf);
	}
}

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

