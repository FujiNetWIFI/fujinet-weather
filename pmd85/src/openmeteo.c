/*
 * Methods to get weather data from Open-Meteo API
 */
#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <conio.h>

#include "fujinet-network.h"
#include "jsonquery.h"
#include "weatherdefs.h"
//#include "strutil.h"
#include "gfx.h"
#include "openmeteo.h"

extern UNITOPT unit_opt;
extern int err;

char omurl[256];
char msgbuf[LINE_LEN];

const char om_head[] = "N:https://api.open-meteo.com/v1/forecast?latitude=";
const char om_lon[] = "&longitude=";

const char *om_tail_weather[] = {
	"&timezone=auto&current=relative_humidity_2m,weather_code,cloud_cover,pressure_msl,is_day",
	"&current=temperature_2m,apparent_temperature,wind_speed_10m,wind_direction_10m",
	// sunrise/sunset exist only as daily values, today's are fetched with the weather
	"&timezone=auto&current=dew_point_2m,visibility&daily=sunrise,sunset&forecast_days=1"
};

const char om_forecast_days_hours_daily[] = "&timezone=auto&forecast_days=8&daily=";

const char *om_tail_forecast[] = {
	"temperature_2m_max,temperature_2m_min",
	"wind_speed_10m_max,wind_direction_10m_dominant",
	"precipitation_sum,uv_index_max,precipitation_probability_max",
	"weather_code"
};

/* unit option string */
const char *unit_str[] = {"&wind_speed_unit=ms", "&temperature_unit=fahrenheit&wind_speed_unit=mph&precipitation_unit=inch"};

/* geocoding api */
const char om_geocoding_head[] = "N:https://geocoding-api.open-meteo.com/v1/search?name=";
const char om_geocoding_tail[] = "&count=1&language=en&format=json";
static unsigned char pdot = 0;

char increment_dot()
{
	return (pdot++);
}

bool om_geocoding(LOCATION *loc, char *city)
{
	strcpy(omurl, om_geocoding_head);
	strcat(omurl, city);
	strcat(omurl, om_geocoding_tail);

	err = network_open(omurl, OPEN_MODE_HTTP_GET_H, OPEN_TRANS_NONE);
	handle_err("open meteo open");
	err = network_json_parse(omurl);
	handle_err("open meteo geocoding parse");

	JSON_QUERY(omurl, "/results/0/name", loc->city, sizeof(loc->city));
	if (loc->city[0] == '\0')
	{
		network_close(omurl);
		return false;
	}
	JSON_QUERY(omurl, "/results/0/longitude", loc->lon, sizeof(loc->lon));
	JSON_QUERY(omurl, "/results/0/latitude", loc->lat, sizeof(loc->lat));
	JSON_QUERY(omurl, "/results/0/country_code", loc->countryCode, sizeof(loc->countryCode));
	JSON_QUERY(omurl, "/results/0/admin1", loc->state, sizeof(loc->state));

	// The API sometimes returns the city name in admin1.
	// If so, ignore it.
	if (strcmp(loc->state, city) == 0)
	{
		loc->state[0] = '\0';
	}

	network_close(omurl);

	return true;
}

//
// setup Open-Metro URL
//
void setup_omurl(LOCATION *loc, char *param, bool isforecast)
{
	char prbuf[QUARTER_LEN];

	strcpy(omurl, om_head);
	strcat(omurl, loc->lat);
	strcat(omurl, om_lon);
	strcat(omurl, loc->lon);
	if (isforecast)
	{
		strcat(omurl, om_forecast_days_hours_daily);
	}
	strcat(omurl, param);
	strcat(omurl, unit_str[unit_opt]);

	sprintf(prbuf, "%03d", strlen(omurl));
}

//
// open and parse one open-meteo request, what + n identify it in error messages
//
static void om_request(LOCATION *loc, const char *param, bool isforecast, const char *what, int n)
{
	setup_omurl(loc, (char *)param, isforecast);

	disp_status("Reading...");
	err = network_open(omurl, OPEN_MODE_HTTP_GET_H, OPEN_TRANS_NONE);
	sprintf(msgbuf, "%s open %d", what, n);
	handle_err(msgbuf);

	progress_dots(increment_dot());

	err = network_json_parse(omurl);
	sprintf(msgbuf, "%s parse %d", what, n);
	handle_err(msgbuf);

	progress_dots(increment_dot());

	disp_status("Processing...");
}

//
// fetch current weather, including today's sunrise/sunset
//
void get_weather(LOCATION *loc, WEATHER *wi)
{
	pdot = 0;

	// city name, state, country code
	strcpy(wi->name, utf8_to_ascii(loc->city));
	strcpy(wi->state, loc->state);
	strcpy(wi->country, loc->countryCode);

	for (int i = 0; i < 3; i++)
	{
		om_request(loc, om_tail_weather[i], false, "om", i + 1);
		set_weather(wi, i + 1);
		network_close(omurl);
	}
}

//
// fetch 8 day forecast
//
void get_forecast(LOCATION *loc, FORECAST *fc)
{
	pdot = 0;

	for (int i = 0; i < 4; i++)
	{
		om_request(loc, om_tail_forecast[i], true, "forecast", i + 1);
		set_forecast(fc, i + 1);
		network_close(omurl);
	}
}

//
// set weather data
//
void set_weather(WEATHER *wi, int segment)
{
	switch(segment)
	{
		case 1:
			//  date & time
			JSON_QUERY(omurl, "/current/time", wi->datetime, sizeof(wi->datetime));
			//  timezone abbreviation (e.g. "EDT" or "GMT-4" depending
			//  on what open-meteo decides to return)
			JSON_QUERY(omurl, "/timezone_abbreviation", wi->timezone, sizeof(wi->timezone));
			//  pressure
			JSON_QUERY(omurl, "/current/pressure_msl", wi->pressure, sizeof(wi->pressure));
			//  humidity
			JSON_QUERY(omurl, "/current/relative_humidity_2m", wi->humidity, sizeof(wi->humidity));
			// weather code (icon)
			JSON_QUERY(omurl, "/current/weather_code", msgbuf, sizeof(msgbuf));
			wi->icon = (char)atoi(msgbuf);
			// day/night; anything but "0" (including a failed query) counts as day
			JSON_QUERY(omurl, "/current/is_day", msgbuf, sizeof(msgbuf));
			wi->is_day = (msgbuf[0] != '0');
			//  clouds
			JSON_QUERY(omurl, "/current/cloud_cover", wi->clouds, sizeof(wi->clouds));
			break;
		case 2:
			//  temperature
			JSON_QUERY(omurl, "/current/temperature_2m", wi->temp, sizeof(wi->temp));
			//  feels_like
			JSON_QUERY(omurl, "/current/apparent_temperature", wi->feels_like, sizeof(wi->feels_like));
			//  wind_speed
			JSON_QUERY(omurl, "/current/wind_speed_10m", wi->wind_speed, sizeof(wi->wind_speed));
			//  wind_deg
			JSON_QUERY(omurl, "/current/wind_direction_10m", wi->wind_deg, sizeof(wi->wind_deg));
			break;
		case 3:
			//  dew_point
			JSON_QUERY(omurl, "/current/dew_point_2m", wi->dew_point, sizeof(wi->dew_point));
			//  visibility
			JSON_QUERY(omurl, "/current/visibility", wi->visibility, sizeof(wi->visibility));
			//  today's sunrise and sunset
			JSON_QUERY(omurl, "/daily/sunrise/0", wi->sunrise, sizeof(wi->sunrise));
			JSON_QUERY(omurl, "/daily/sunset/0", wi->sunset, sizeof(wi->sunset));
			break;
	}	

	progress_dots(increment_dot());
}

//
// set forecast data
//
void set_forecast(FORECAST *fc, int segment)
{
	char i;
	char querybuf[HALF_LEN * 2];
	char prbuf[QUARTER_LEN];

	for (i = 0; i <= 7; i++)
	//for (i = 0; i <=0 ; i++)
	{
		DAYINFO *day = &fc->day[i];

		switch (segment)
		{
			case 1:
				// date & time
				sprintf(querybuf, "/daily/time/%d", i);
				JSON_QUERY(omurl, querybuf, day->date, sizeof(day->date));
				// temp min
				sprintf(querybuf, "/daily/temperature_2m_min/%d", i);
				JSON_QUERY(omurl, querybuf, day->temp_min, sizeof(day->temp_min));
				// temp max
				sprintf(querybuf, "/daily/temperature_2m_max/%d", i);
				JSON_QUERY(omurl, querybuf, day->temp_max, sizeof(day->temp_max));
				break;
			case 2:


				// wind  speed
				sprintf(querybuf, "/daily/wind_speed_10m_max/%d", i);
				JSON_QUERY(omurl, querybuf, day->wind_speed, sizeof(day->wind_speed));
				// wind  deg
				sprintf(querybuf, "/daily/wind_direction_10m_dominant/%d", i);
				JSON_QUERY(omurl, querybuf, day->wind_deg, sizeof(day->wind_deg));
				break;
			case 3:
				// precipitation sum
				sprintf(querybuf, "/daily/precipitation_sum/%d", i);
				JSON_QUERY(omurl, querybuf, day->precipitation_sum, sizeof(day->precipitation_sum));
				// uv index  max
				sprintf(querybuf, "/daily/uv_index_max/%d", i);
				JSON_QUERY(omurl, querybuf, day->uv_index_max, sizeof(day->uv_index_max));
				// precipitation probability max (longest query: 38 chars + '\0' fits querybuf[40])
				sprintf(querybuf, "/daily/precipitation_probability_max/%d", i);
				JSON_QUERY(omurl, querybuf, day->precip_prob, sizeof(day->precip_prob));
				break;
			case 4:
				// icon
				sprintf(querybuf, "/daily/weather_code/%d", i);
				JSON_QUERY(omurl, querybuf, prbuf, sizeof(prbuf));
				day->icon = (char)atoi(prbuf);
				break;
		}

		progress_dots(increment_dot());
	}
}
