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
char om_head[] = "N:https://api.open-meteo.com/v1/forecast?latitude=";
char om_lon[] = "&longitude=";

char om_tail_weather1[] = "&timezone=auto&current=relative_humidity_2m,weather_code,cloud_cover,surface_pressure";
char om_tail_weather2[] = "&current=temperature_2m,apparent_temperature,wind_speed_10m,wind_direction_10m";
char om_tail_weather3[] = "&hourly=dew_point_2m,visibility&forecast_hours=1";

char om_forecast_days_hours_daily[] = "&timezone=auto&forecast_days=8&forecast_hours=1&daily=";
                                                                                // 253
char om_tail_forecast1[] = "temperature_2m_max,temperature_2m_min";
char om_tail_forecast2[] = "wind_speed_10m_max,wind_direction_10m_dominant";
char om_tail_forecast3[] = "precipitation_sum,uv_index_max";
char om_tail_forecast4[] = "weather_code,sunrise,sunset";

/* unit option string */
char *unit_str[] = {"&wind_speed_unit=ms", "&temperature_unit=fahrenheit&wind_speed_unit=mph&precipitation_unit=inch"};

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
	// weather 1 query
	setup_omurl(loc, om_tail_weather1, false);

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
	network_json_query(omurl, "/current/time", querybuf);
	strcpy(wi->datetime, querybuf);
	// timezone abbreviation
	network_json_query(omurl, "/timezone_abbreviation", wi->timezone);
	//  pressure
	network_json_query(omurl, "/current/surface_pressure", wi->pressure);
	//  humidity
	network_json_query(omurl, "/current/relative_humidity_2m", wi->humidity);
	// weather code (icon)
	network_json_query(omurl, "/current/weather_code", querybuf);
	wi->icon = (char)atoi(querybuf);
	//  clouds
	network_json_query(omurl, "/current/cloud_cover", wi->clouds);

	network_close(omurl); // of weather1

	// weather 2 query
	setup_omurl(loc, om_tail_weather2, false);

	progress_dots(increment_dot());

	err = network_open(omurl, OPEN_MODE_READ, OPEN_TRANS_NONE);
	handle_err("omurl open 2");

	err = network_json_parse(omurl);
	handle_err("omurl parse 2");

	//  temperature
	network_json_query(omurl, "/current/temperature_2m", wi->temp);
	//  feels_like
	network_json_query(omurl, "/current/apparent_temperature", wi->feels_like);
	//  wind_speed
	network_json_query(omurl, "/current/wind_speed_10m", wi->wind_speed);
	//  wind_deg
	err = network_json_query(omurl, "/current/wind_direction_10m", wi->wind_deg);

	network_close(omurl); // of weather2

	// weather 3 query
	setup_omurl(loc, om_tail_weather3, false);

	progress_dots(increment_dot());

	err = network_open(omurl, OPEN_MODE_READ, OPEN_TRANS_NONE);
	handle_err("omurl open 3");

	err = network_json_parse(omurl);
	handle_err("omurl parse 3");

	//  dew_point
	network_json_query(omurl, "/hourly/dew_point_2m/0", wi->dew_point);
	//  visibility
	network_json_query(omurl, "/hourly/visibility/0", wi->visibility);
	
	network_close(omurl); // of weather3

	//	forecast
	//  part 1
	setup_omurl(loc, om_tail_forecast1, true);

	progress_dots(increment_dot());

	err = network_open(omurl, OPEN_MODE_READ, OPEN_TRANS_NONE);
	handle_err("forecast 1 open");

	progress_dots(increment_dot());

	err = network_json_parse(omurl);
	handle_err("forecast 1 parse");

	set_forecast(fc, 1);
	network_close(omurl); // of forecast part 1

	//  part 2
	setup_omurl(loc, om_tail_forecast2, true);

	progress_dots(increment_dot());

	err = network_open(omurl, OPEN_MODE_READ, OPEN_TRANS_NONE);
	handle_err("forecast 2 open");

	err = network_json_parse(omurl);
	handle_err("forecast 2 parse");

	progress_dots(increment_dot());

	set_forecast(fc, 2);

	network_close(omurl); // of forecast part 2

	//  part 3
	setup_omurl(loc, om_tail_forecast3, true);

	progress_dots(increment_dot());

	err = network_open(omurl, OPEN_MODE_READ, OPEN_TRANS_NONE);
	handle_err("forecast 3 open");

	err = network_json_parse(omurl);
	handle_err("forecast 3 parse");

	progress_dots(increment_dot());

	set_forecast(fc, 3);

	network_close(omurl); // of forecast part 3

	//  part 4
	setup_omurl(loc, om_tail_forecast4, true);

	progress_dots(increment_dot());

	err = network_open(omurl, OPEN_MODE_READ, OPEN_TRANS_NONE);
	handle_err("forecast 4 open");

	err = network_json_parse(omurl);
	handle_err("forecast 4 parse");

	progress_dots(increment_dot());

	set_forecast(fc, 4);

	network_close(omurl); // of forecast part 4

	//  Copy today's sunrise/sunset from forecast data to weather data
	//  We don't have both until the second part of the forecast is retrieved
	strcpy(wi->sunrise, fc->day[0].sunrise);
	strcpy(wi->sunset, fc->day[0].sunset);
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
