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
extern int err;

char omurl[256];
char msgbuf[LINE_LEN];

const char om_head[] = "N:https://api.open-meteo.com/v1/forecast?latitude=";
const char om_lon[] = "&longitude=";

const char *om_tail_weather[] = {
	"&timezone=auto&current=relative_humidity_2m,weather_code,cloud_cover,surface_pressure",
	"&current=temperature_2m,apparent_temperature,wind_speed_10m,wind_direction_10m",
	"&hourly=dew_point_2m,visibility&forecast_hours=1"
};

const char om_forecast_days_hours_daily[] = "&timezone=auto&forecast_days=8&forecast_hours=1&daily=";

const char *om_tail_forecast[] = {
	"temperature_2m_max,temperature_2m_min",
	"wind_speed_10m_max,wind_direction_10m_dominant",
	"precipitation_sum,uv_index_max",
	"weather_code,sunrise,sunset"
};

/* unit option string */
const char *unit_str[] = {"&wind_speed_unit=ms", "&temperature_unit=fahrenheit&wind_speed_unit=mph&precipitation_unit=inch"};

/* geocoding api */
const char om_geocoding_head[] = "N:https://geocoding-api.open-meteo.com/v1/search?name=";
const char om_geocoding_tail[] = "&count=1&language=en&format=json";
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
	network_json_query(omurl, "/results/0/name", temp_buf);
	if (temp_buf == NULL || strlen(temp_buf) == 0)
	{
		return false;
	}
	strcpy(loc->city, temp_buf);
	network_json_query(omurl, "/results/0/longitude", loc->lon);
	network_json_query(omurl, "/results/0/latitude", loc->lat);
	network_json_query(omurl, "/results/0/country_code", loc->countryCode);
	network_json_query(omurl, "/results/0/admin1", temp_buf);

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
//
//
void get_om_info(LOCATION *loc, WEATHER *wi, FORECAST *fc)
{
	char querybuf[HALF_LEN * 2];

	pdot = 0;

	// weather query
	for (int i = 0; i < 3; i++) 
	{
		setup_omurl(loc, om_tail_weather[i], false);

		err = network_open(omurl, OPEN_MODE_READ, OPEN_TRANS_NONE);
		sprintf(msgbuf, "om open %d", i + 1);
		handle_err(msgbuf);

		progress_dots(increment_dot());

		err = network_json_parse(omurl);
		sprintf(msgbuf, "om parse %d", i + 1);	
		handle_err(msgbuf);

		progress_dots(increment_dot());

		// city name, state, country code
		strcpy(wi->name, loc->city);
		strcpy(wi->state, loc->state);
		strcpy(wi->country, loc->countryCode);

		set_weather(wi, i + 1);

		network_close(omurl); 
	}

	// forecast
	for (int i = 0; i < 4; i++) 
	{	
		setup_omurl(loc, om_tail_forecast[i], true);

		err = network_open(omurl, OPEN_MODE_READ, OPEN_TRANS_NONE);
		sprintf(msgbuf, "forecast open %d", i + 1);
		handle_err(msgbuf);

		progress_dots(increment_dot());

		err = network_json_parse(omurl);
		sprintf(msgbuf, "forecast parse %d", i + 1);
		handle_err(msgbuf);

		progress_dots(increment_dot());

		set_forecast(fc, i+1);
		network_close(omurl);
	}

	//  Copy today's sunrise/sunset from forecast data to weather data
	//  We don't have both until the second part of the forecast is retrieved
	strcpy(wi->sunrise, fc->day[0].sunrise);
	strcpy(wi->sunset, fc->day[0].sunset);
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
			network_json_query(omurl, "/current/time", wi->datetime);
			// timezone abbreviation
			network_json_query(omurl, "/timezone_abbreviation", wi->timezone);
			//  pressure
			network_json_query(omurl, "/current/surface_pressure", wi->pressure);
			//  humidity
			network_json_query(omurl, "/current/relative_humidity_2m", wi->humidity);
			// weather code (icon)
			network_json_query(omurl, "/current/weather_code", msgbuf);
			wi->icon = (char)atoi(msgbuf);
			//  clouds
			network_json_query(omurl, "/current/cloud_cover", wi->clouds);
			break;
		case 2:
			//  temperature
			network_json_query(omurl, "/current/temperature_2m", wi->temp);
			//  feels_like
			network_json_query(omurl, "/current/apparent_temperature", wi->feels_like);
			//  wind_speed
			network_json_query(omurl, "/current/wind_speed_10m", wi->wind_speed);
			//  wind_deg
			network_json_query(omurl, "/current/wind_direction_10m", wi->wind_deg);
			break;
		case 3:
			//  dew_point
			network_json_query(omurl, "/hourly/dew_point_2m/0", wi->dew_point);
			//  visibility
			network_json_query(omurl, "/hourly/visibility/0", wi->visibility);
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
	char querybuf[LINE_LEN];
	char prbuf[QUARTER_LEN];

	for (i = 0; i <= 7; i++)
	{
		switch (segment)
		{
			case 1:
				// date & time
				sprintf(querybuf, "/daily/time/%d", i);
				network_json_query(omurl, querybuf, fc->day[i].date);
				// temp min
				sprintf(querybuf, "/daily/temperature_2m_min/%d", i);
				network_json_query(omurl, querybuf, fc->day[i].temp_min);
				// temp max
				sprintf(querybuf, "/daily/temperature_2m_max/%d", i);
				network_json_query(omurl, querybuf, fc->day[i].temp_max);
				break;
			case 2:


				// wind  speed
				sprintf(querybuf, "/daily/wind_speed_10m_max/%d", i);
				network_json_query(omurl, querybuf, fc->day[i].wind_speed);
				// wind  deg
				sprintf(querybuf, "/daily/wind_direction_10m_dominant/%d", i);
				network_json_query(omurl, querybuf, fc->day[i].wind_deg);
				break;
			case 3:
				// precipitation sum
				sprintf(querybuf, "/daily/precipitation_sum/%d", i);
				network_json_query(omurl, querybuf, fc->day[i].precipitation_sum);
				// uv index  max
				sprintf(querybuf, "/daily/uv_index_max/%d", i);
				network_json_query(omurl, querybuf, fc->day[i].uv_index_max);
				break;
			case 4:
				// icon
				sprintf(querybuf, "/daily/weather_code/%d", i);
				network_json_query(omurl, querybuf, prbuf);
				fc->day[i].icon = (char)atoi(prbuf);
				// sunrise
				sprintf(querybuf, "/daily/sunrise/%d", i);
				network_json_query(omurl, querybuf, fc->day[i].sunrise);
				// sunset
				sprintf(querybuf, "/daily/sunset/%d", i);
				network_json_query(omurl, querybuf, fc->day[i].sunset);
				break;
		}

		progress_dots(increment_dot());
	}
}
