/*
 * Methods to get weather data from Open-Meteo API
 */
#include <cmoc.h>
#include <coco.h>

#include "fujinet-network.h"
#include "weatherdefs.h"
#include "strutil.h"
#include "gfx.h"
#include "openmeteo.h"

extern UNITOPT unit_opt;
extern int	err;


char omurl[384];
char om_head[] = "N:https://api.open-meteo.com/v1/forecast?latitude=";
char om_lon[] = "&longitude=";

char om_tail_weather1[] ="&timezone=auto&current=relative_humidity_2m,weather_code,cloud_cover,surface_pressure";
char om_tail_weather2[] ="&current=temperature_2m,apparent_temperature,,wind_speed_10m,wind_direction_10m&hourly=dew_point_2m,visibility&forecast_hours=1";

char om_tail_forecast1[] ="&timezone=auto&forecast_days=8&forecast_hours=1&daily=weather_code,temperature_2m_max,temperature_2m_min,sunrise,sunset";
char om_tail_forecast2[] ="&forecast_days=8&forecast_hours=1&daily=wind_speed_10m_max,wind_direction_10m_dominant,precipitation_sum,uv_index_max";

/* unit option string */
char *unit_str[] = {"&wind_speed_unit=ms", "&temperature_unit=fahrenheit&wind_speed_unit=mph"};

/* geocoding api */
char om_geocoding_head[] = "N:https://geocoding-api.open-meteo.com/v1/search?name=";
char om_geocoding_tail[] = "&count=1&language=en&format=json";

char pdot = 0;

char increment_dot()
{
	char ret = pdot;
	pdot++;
	if (pdot > 5)
	{
		pdot = 0;
	}
	return (ret);
}

bool om_geocoding(LOCATION *loc, char *city)
{
	char temp_buf[LINE_LEN];

	strcpy(omurl, om_geocoding_head);
	strcat(omurl, city);
	strcat(omurl, om_geocoding_tail);

	err = network_open(omurl, OPEN_MODE_READ, OPEN_TRANS_NONE);
	handle_err("open meteo open");
	err = network_json_parse(omurl);
	handle_err("open meteo geocoding parse");

	memset(temp_buf, 0, LINE_LEN);
	network_json_query_trim(omurl, "/results/0/name", temp_buf);
	if (temp_buf == NULL || strlen(temp_buf) == 0)
	{
		return false;
	}
	strcpy(loc->city, temp_buf);
	network_json_query_trim(omurl, "/results/0/longitude", loc->lon);
	network_json_query_trim(omurl, "/results/0/latitude", loc->lat);
	network_json_query_trim(omurl, "/results/0/country_code", loc->countryCode);
	network_json_query_trim(omurl, "/results/0/admin1", temp_buf);

	// The API sometimes returns the city name in admin1.
	// If so, ignore it.
	if (strcmp(temp_buf, city) != 0)
	{
		strcpy(loc->state, temp_buf);
	}
	else
	{
		strcpy(loc->state, "");
	}

	network_close(omurl);

	return true;
}

//
// setup Open-Metro URL
//
void setup_omurl(LOCATION *loc, char *param) {
	strcpy(omurl, om_head);
    strcat(omurl, loc->lat);
    strcat(omurl, om_lon);
    strcat(omurl, loc->lon);
    strcat(omurl, param);
	strcat(omurl, unit_str[unit_opt]);
}
//
//
//
void get_om_info(LOCATION *loc, WEATHER *wi, FORECAST *fc) {
	char querybuf[LINE_LEN * 2];
 
	pdot = 0;
// weather 1 query
	setup_omurl(loc, om_tail_weather1);

	progress_dots(increment_dot());

    err = network_open(omurl, OPEN_MODE_READ, OPEN_TRANS_NONE);
    handle_err("om open");

    err = network_json_parse(omurl);
    handle_err("om parse");

//	city name, state, country code
	strcpy(wi->name, loc->city);
	strcpy(wi->state, loc->state);
	strcpy(wi->country, loc->countryCode);

//  date & time  
    network_json_query_trim(omurl, "/current/time", querybuf);
	strcpy(wi->datetime, querybuf);
//  timezone(offset) 
    network_json_query_trim(omurl, "/utc_offset_seconds", querybuf);
	wi->tz = atol(querybuf);
// timezone
    network_json_query_trim(omurl, "/timezone", wi->timezone);
//  pressure
    network_json_query_trim(omurl, "/current/surface_pressure", wi->pressure);
//  humidity
    network_json_query_trim(omurl, "/current/relative_humidity_2m", wi->humidity);
// weather code (icon)
    network_json_query_trim(omurl, "/current/weather_code", querybuf);
	wi->icon = (char) atoi(querybuf);
//  clouds
    network_json_query_trim(omurl, "/current/cloud_cover", wi->clouds);

	network_close(omurl);	// of weather1

// weather 2 query
	setup_omurl(loc, om_tail_weather2);

	progress_dots(increment_dot());

    err = network_open(omurl, OPEN_MODE_READ, OPEN_TRANS_NONE);
    handle_err("omurl open 2");
	
    err = network_json_parse(omurl);
    handle_err("omurl parse 2");
	
//  temperature
    network_json_query_trim(omurl, "/current/temperature_2m", wi->temp);
//  feels_like
    network_json_query_trim(omurl, "/current/apparent_temperature", wi->feels_like);
//  dew_point
    network_json_query_trim(omurl, "/hourly/dew_point_2m/0", wi->dew_point);
//  visibility
    network_json_query_trim(omurl, "/hourly/visibility/0", wi->visibility);
//  wind_speed
    network_json_query_trim(omurl, "/current/wind_speed_10m", wi->wind_speed);
//  wind_deg
    err = network_json_query_trim(omurl, "/current/wind_direction_10m", wi->wind_deg);

	network_close(omurl);	// of weather2

//	forecast
//  part 1
	setup_omurl(loc, om_tail_forecast1);

	progress_dots(increment_dot());

    err = network_open(omurl, OPEN_MODE_READ, OPEN_TRANS_NONE);
    handle_err("forecast 1 open");

	progress_dots(increment_dot());

    err = network_json_parse(omurl);
    handle_err("forecast 1 parse");

	set_forecast1(fc);
	network_close(omurl);	// of forecast part 1

//  copy today's sunrise/sunset from forecat data to weather data
	strcpy(wi->sunrise, fc->day[0].sunrise); 
	strcpy(wi->sunset, fc->day[0].sunset); 

//  part 2
	setup_omurl(loc, om_tail_forecast2);

	progress_dots(increment_dot());

    err = network_open(omurl, OPEN_MODE_READ, OPEN_TRANS_NONE);
    handle_err("forecast 2 open");

    err = network_json_parse(omurl);
    handle_err("forecast 2 parse");

	progress_dots(increment_dot());

	set_forecast2(fc);

	network_close(omurl);	// of forecast part 2
}
//
// set forecast data part1
//
void set_forecast1(FORECAST *fc) {
	char	i;
	char querybuf[LINE_LEN * 2];
	char prbuf[LINE_LEN];

	for (i = 0; i <= 7; i++)
	{
		// date & time
		sprintf(querybuf, "/daily/time/%d", i);
		network_json_query_trim(omurl, querybuf, prbuf);
		strcpy(fc->day[i].date, prbuf);
		// sunrise
		sprintf(querybuf, "/daily/sunrise/%d", i);
		network_json_query_trim(omurl, querybuf, prbuf);
		strcpy(fc->day[i].sunrise, prbuf);
		// sunset
		sprintf(querybuf, "/daily/sunset/%d", i);
		network_json_query_trim(omurl, querybuf, prbuf);
		strcpy(fc->day[i].sunset, prbuf);
		// temp min
		sprintf(querybuf, "/daily/temperature_2m_min/%d", i);
		network_json_query_trim(omurl, querybuf, fc->day[i].temp_min);
		// temp max
		sprintf(querybuf, "/daily/temperature_2m_max/%d", i);
		network_json_query_trim(omurl, querybuf, fc->day[i].temp_max);
		// icon
		sprintf(querybuf, "/daily/weather_code/%d", i);
		network_json_query_trim(omurl, querybuf, prbuf);
		fc->day[i].icon = (char)atoi(prbuf);

		progress_dots(increment_dot());
	}
}
//
// set forecast data part 2
//
void set_forecast2(FORECAST *fc)
{
	char i;
	char querybuf[LINE_LEN * 2];

	for (i = 0; i <= 7; i++)
	{
		// precipitation sum
		sprintf(querybuf, "/daily/precipitation_sum/%d", i);
		network_json_query_trim(omurl, querybuf, fc->day[i].precipitation_sum);
		// uv index  max
		sprintf(querybuf, "/daily/uv_index_max/%d", i);
		network_json_query_trim(omurl, querybuf, fc->day[i].uv_index_max);
		// wind  speed
		sprintf(querybuf, "/daily/wind_speed_10m_max/%d", i);
		network_json_query_trim(omurl, querybuf, fc->day[i].wind_speed);
		// wind  deg
		sprintf(querybuf, "/daily/wind_direction_10m_dominant/%d", i);
		network_json_query_trim(omurl, querybuf, fc->day[i].wind_deg);

		progress_dots(increment_dot());
	}
}
