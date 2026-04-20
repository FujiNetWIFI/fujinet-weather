#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

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
	"&current=relative_humidity_2m,weather_code,cloud_cover,surface_pressure",
	"&current=temperature_2m,apparent_temperature,wind_speed_10m,wind_direction_10m",
	"&hourly=dew_point_2m,visibility,precipitation_probability&forecast_hours=1"
};

const char om_forecast_days_hours_daily[] = "&forecast_days=8&daily=";

const char *om_tail_forecast[] = {
	"temperature_2m_max,temperature_2m_min",
	"wind_speed_10m_max,wind_direction_10m_dominant",
	"precipitation_sum,uv_index_max",
	"weather_code,sunrise,sunset"
};

const char *unit_str[] = {"&wind_speed_unit=ms", "&temperature_unit=fahrenheit&wind_speed_unit=mph&precipitation_unit=inch"};

const char om_geocoding_head[] = "N:https://geocoding-api.open-meteo.com/v1/search?name=";
const char om_geocoding_tail[] = "&count=1&language=en&format=json";

static const struct {
	const char *iana;
	const char *std_name;
	const char *dst_name;
	long std_offset;
} tz_table[] = {
	{"America/New_York",      "EST",  "EDT",  -18000L},
	{"America/Chicago",       "CST",  "CDT",  -21600L},
	{"America/Denver",        "MST",  "MDT",  -25200L},
	{"America/Los_Angeles",   "PST",  "PDT",  -28800L},
	{"America/Anchorage",     "AKST", "AKDT", -32400L},
	{"America/Phoenix",       "MST",  "MST",  -25200L},
	{"Pacific/Honolulu",      "HST",  "HST",  -36000L},
	{"America/Halifax",       "AST",  "ADT",  -14400L},
	{"America/St_Johns",      "NST",  "NDT",  -12600L},
	{"America/Winnipeg",      "CST",  "CDT",  -21600L},
	{"America/Edmonton",      "MST",  "MDT",  -25200L},
	{"America/Vancouver",     "PST",  "PDT",  -28800L},
	{"Europe/London",         "GMT",  "BST",  0L},
	{"Europe/Paris",          "CET",  "CEST", 3600L},
	{"Europe/Berlin",         "CET",  "CEST", 3600L},
	{"Europe/Helsinki",       "EET",  "EEST", 7200L},
	{"Europe/Moscow",         "MSK",  "MSK",  10800L},
	{"Asia/Kolkata",          "IST",  "IST",  19800L},
	{"Asia/Shanghai",         "CST",  "CST",  28800L},
	{"Asia/Tokyo",            "JST",  "JST",  32400L},
	{"Australia/Sydney",      "AEST", "AEDT", 36000L},
	{"Pacific/Auckland",      "NZST", "NZDT", 43200L},
	{NULL, NULL, NULL, 0L}
};

static void resolve_timezone(const char *api_abbr, const char *iana,
                             long offset, char *buf)
{
	int i;
	long hrs, mins;

	if (strncmp(api_abbr, "GMT", 3) != 0)
	{
		strcpy(buf, api_abbr);
		return;
	}

	for (i = 0; tz_table[i].iana != NULL; i++)
	{
		if (strcmp(iana, tz_table[i].iana) == 0)
		{
			if (offset == tz_table[i].std_offset)
				strcpy(buf, tz_table[i].std_name);
			else
				strcpy(buf, tz_table[i].dst_name);
			return;
		}
	}

	strcpy(buf, api_abbr);
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
	if (temp_buf[0] == '\0' || strlen(temp_buf) == 0)
	{
		return false;
	}
	strcpy(loc->city, temp_buf);
	network_json_query(omurl, "/results/0/longitude", loc->lon);
	network_json_query(omurl, "/results/0/latitude", loc->lat);
	network_json_query(omurl, "/results/0/country_code", loc->countryCode);
	network_json_query(omurl, "/results/0/admin1", temp_buf);

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

void setup_omurl(LOCATION *loc, const char *param, bool isforecast, bool needtz)
{
	strcpy(omurl, om_head);
	strcat(omurl, loc->lat);
	strcat(omurl, om_lon);
	strcat(omurl, loc->lon);
	if (needtz)
	{
		strcat(omurl, "&timezone=auto");
	}
	if (isforecast)
	{
		strcat(omurl, om_forecast_days_hours_daily);
	}
	strcat(omurl, param);
	strcat(omurl, unit_str[unit_opt]);
	strcat(omurl, "&timeformat=unixtime");
}

void get_om_info(LOCATION *loc, WEATHER *wi, FORECAST *fc)
{
	char querybuf[HALF_LEN * 2];
	int i;

	for (i = 0; i < 3; i++)
	{
		setup_omurl(loc, om_tail_weather[i], false, i == 0);

		err = network_open(omurl, OPEN_MODE_READ, OPEN_TRANS_NONE);
		sprintf(msgbuf, "om open %d", i + 1);
		handle_err(msgbuf);

		progress_step();

		err = network_json_parse(omurl);
		sprintf(msgbuf, "om parse %d", i + 1);
		handle_err(msgbuf);

		progress_step();

		strcpy(wi->name, loc->city);
		strcpy(wi->state, loc->state);
		strcpy(wi->country, loc->countryCode);

		set_weather(wi, i + 1);

		network_close(omurl);
	}

	for (i = 0; i < 4; i++)
	{
		setup_omurl(loc, om_tail_forecast[i], true, true);

		err = network_open(omurl, OPEN_MODE_READ, OPEN_TRANS_NONE);
		sprintf(msgbuf, "forecast open %d", i + 1);
		handle_err(msgbuf);

		progress_step();

		err = network_json_parse(omurl);
		sprintf(msgbuf, "forecast parse %d", i + 1);
		handle_err(msgbuf);

		progress_step();

		set_forecast(fc, i+1);
		network_close(omurl);
	}

	wi->sunrise = fc->day[0].sunrise;
	wi->sunset = fc->day[0].sunset;
}

void set_weather(WEATHER *wi, int segment)
{
	char tbuf[HALF_LEN];
	char iana[LINE_LEN];
	char api_abbr[QUARTER_LEN];

	switch(segment)
	{
		case 1:
			network_json_query(omurl, "/current/time", tbuf);
			wi->datetime = atol(tbuf);
			network_json_query(omurl, "/utc_offset_seconds", tbuf);
			wi->tz = atol(tbuf);
			network_json_query(omurl, "/timezone_abbreviation", api_abbr);
			network_json_query(omurl, "/timezone", iana);
			resolve_timezone(api_abbr, iana, wi->tz, wi->timezone);
			network_json_query(omurl, "/current/surface_pressure", wi->pressure);
			network_json_query(omurl, "/current/relative_humidity_2m", wi->humidity);
			network_json_query(omurl, "/current/weather_code", msgbuf);
			wi->icon = (char)atoi(msgbuf);
			network_json_query(omurl, "/current/cloud_cover", wi->clouds);
			break;
		case 2:
			network_json_query(omurl, "/current/temperature_2m", wi->temp);
			network_json_query(omurl, "/current/apparent_temperature", wi->feels_like);
			network_json_query(omurl, "/current/wind_speed_10m", wi->wind_speed);
			network_json_query(omurl, "/current/wind_direction_10m", wi->wind_deg);
			break;
		case 3:
			network_json_query(omurl, "/hourly/dew_point_2m/0", wi->dew_point);
			network_json_query(omurl, "/hourly/visibility/0", wi->visibility);
			network_json_query(omurl, "/hourly/precipitation_probability/0", wi->precip_prob);
			break;
	}

	progress_step();
}

void set_forecast(FORECAST *fc, int segment)
{
	char i;
	char querybuf[LINE_LEN];
	char prbuf[QUARTER_LEN];
	char tbuf[HALF_LEN];

	for (i = 0; i <= 7; i++)
	{
		switch (segment)
		{
			case 1:
				sprintf(querybuf, "/daily/time/%d", i);
				network_json_query(omurl, querybuf, tbuf);
				fc->day[(int)i].date = atol(tbuf);
				sprintf(querybuf, "/daily/temperature_2m_min/%d", i);
				network_json_query(omurl, querybuf, fc->day[(int)i].temp_min);
				sprintf(querybuf, "/daily/temperature_2m_max/%d", i);
				network_json_query(omurl, querybuf, fc->day[(int)i].temp_max);
				break;
			case 2:
				sprintf(querybuf, "/daily/wind_speed_10m_max/%d", i);
				network_json_query(omurl, querybuf, fc->day[(int)i].wind_speed);
				sprintf(querybuf, "/daily/wind_direction_10m_dominant/%d", i);
				network_json_query(omurl, querybuf, fc->day[(int)i].wind_deg);
				break;
			case 3:
				sprintf(querybuf, "/daily/precipitation_sum/%d", i);
				network_json_query(omurl, querybuf, fc->day[(int)i].precipitation_sum);
				sprintf(querybuf, "/daily/uv_index_max/%d", i);
				network_json_query(omurl, querybuf, fc->day[(int)i].uv_index_max);
				break;
			case 4:
				sprintf(querybuf, "/daily/weather_code/%d", i);
				network_json_query(omurl, querybuf, prbuf);
				fc->day[(int)i].icon = (char)atoi(prbuf);
				sprintf(querybuf, "/daily/sunrise/%d", i);
				network_json_query(omurl, querybuf, tbuf);
				fc->day[(int)i].sunrise = atol(tbuf);
				sprintf(querybuf, "/daily/sunset/%d", i);
				network_json_query(omurl, querybuf, tbuf);
				fc->day[(int)i].sunset = atol(tbuf);
				break;
		}

		progress_step();
	}
}
