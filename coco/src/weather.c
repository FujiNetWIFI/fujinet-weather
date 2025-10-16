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

char c_str[] = {0x22, 0x43, 0x00};
char f_str[] = {0x22, 0x46,0x00};
char min_str[] = {0x23, 0x24,0x00};
char max_str[] = {0x26, 0x27,0x00};
char precip_str[] = {0x28, 0x29, 0x00};

char *temp_unit[] = {c_str, f_str};
char *speed_unit[] = {" m/s", "mph"};

char *wind_deg[] = {" N", " NE", " E", " SE", " S", " SW", " W", " NW"};

char current_forecast_page = 0;

void weather(WEATHER *wi)
{
	char year[5];
	char month[3];
	char day[3];
	char hour[3];
	char min[3];
	char prbuf[LINE_LEN];
	char time_buf[TIME_LEN];
	int wind_idx;

	if (current_screen == 1)
	{
		return;
	}

	current_screen = 1;
	current_forecast_page = 0;

	gfx_cls(CYAN);

	//"2025-10-13T21:15" is the source string.
	strncpy(year, wi->datetime, 4);
	strncpy(month, wi->datetime + 5, 2);
	strncpy(day, wi->datetime + 8, 2);
	strncpy(hour, wi->datetime + 11, 2);
	strncpy(min, wi->datetime + 14, 2);
	// 2023-12-31 18:25 is what we display.
	puts(0, 4, 2, year);
	puts(32, 4, 2, "-");
	puts(40, 4, 2, month);
	puts(56, 4, 2, "-");
	puts(64, 4, 2, day);
	puts(88, 4, 2, hour);
	puts(104, 4, 2, ":");
	puts(112, 4, 2, min);

	// weather conditions icon
	put_icon(0, 0, icon_code(wi->icon));

	// Temperature
	puts_dbl(40, 16, WHITE, wi->temp);

	// Pressure
	puts_dbl(40, 40, WHITE, wi->pressure);

	// Condition
	decode_description(wi->icon, prbuf);
	puts(0, 72, WHITE, prbuf);

	// Region
	sprintf(prbuf, "%s, %s", wi->name, wi->country);
	puts(0, 88, WHITE, prbuf);

	// Humidity
	puts(0 + 4, 104, WHITE, "HUMIDITY:");
	puts(88 + 4, 104, PURPLE, wi->humidity);

	// Dew Point
	puts(0 + 4, 116, WHITE, "DEW PT:");
	puts(64 + 4, 116, PURPLE, wi->dew_point);

	// Clouds
	puts(0 + 4, 128, WHITE, "CLOUDS:");
	puts(88 + 4, 128, PURPLE, wi->clouds);

	// Visibility
	puts(0 + 4, 140, WHITE, "VISIBILITY:");
	puts(88 + 4, 140, PURPLE, wi->visibility);

	// Wind
	wind_idx = (atoi(wi->wind_deg) % 360) / 45;
	sprintf(prbuf, "W: %s%s%s", wi->wind_speed, speed_unit[unit_opt], wind_deg[wind_idx]);
	puts(0 + 4, 152, WHITE, prbuf);

	// Sunrise
	strncpy(time_buf, wi->sunrise + 11, 5);
	puts(0 + 4, 164, WHITE, "SUNRISE:");
	puts(64 + 4, 164, PURPLE, time_buf);

	// Sunset
	strncpy(time_buf, wi->sunset + 11, 5);
	puts(0 + 4 , 176, WHITE, "SUNSET:");
	puts(64 + 4, 176, PURPLE, time_buf);
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

/**
 * @brief Test harness, remove.
 */
/* int main(void) */
/* { */
/*   initCoCoSupport(0); */
/*   rgb(); */
/*   width(32); */
/*   gfx(1); */
  
/*   weather("2023","12","31", */
/* 	  "18","25", */
/* 	  "   Denton, US   ", */
/* 	  "53.0*F", */
/* 	  "  thunderstorm  ", */
/* 	  "30.60 \"Hg", */
/* 	  " 61%", */
/* 	  "39.99*F", */
/* 	  "  0%", */
/* 	  "10km", */
/* 	  " 6.91 mph NE", */
/* 	  " 7:31AM", */
/* 	  " 5:30PM", */
/* 	  thunderstorm); */

/*   while(1); */
  
/*   return 0; */
/* } */
