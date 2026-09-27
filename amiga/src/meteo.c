#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "json.h"
#include "meteo.h"
#include "net.h"

/* FujiNet does TLS itself, so https would cost the Amiga nothing, but the
 * macOS POSIX NIO build currently fails every https fetch (its libcurl and
 * OpenSSL disagree).  Open-Meteo serves http as well; build with
 * -DOM_SCHEME='"https://"' where the FujiNet handles TLS.  ip-api's free
 * tier is http only. */
#ifndef OM_SCHEME
#define OM_SCHEME "http://"
#endif

#define URL_MAX 256   /* FN_MAX_URL_LEN on non-BBC, non-cc65 targets */

static char url[URL_MAX + 32];
static char val[48];
static char error_text[64];

static const char ip_url[] =
	"http://ip-api.com/json/?fields=status,message,city,countryCode,lat,lon";
static const char om_base[] = OM_SCHEME "api.open-meteo.com/v1/forecast";
static const char geo_base[] = OM_SCHEME "geocoding-api.open-meteo.com/v1/search?name=";

/* Open-Meteo's URL does not fit the Amiga's 256-byte FujiNet URL limit in
 * one request, so it is split.  Unit parameters are appended only to the
 * requests that return that kind of value. */
enum { U_TEMP = 1, U_WIND = 2, U_PRECIP = 4 };
static const struct {
	const char *query;
	int units;
} requests[] = {
	{"&timezone=auto&timeformat=unixtime"
	 "&current=weather_code,relative_humidity_2m,cloud_cover,surface_pressure", 0},
	{"&current=temperature_2m,apparent_temperature,wind_speed_10m,wind_direction_10m",
	 U_TEMP | U_WIND},
	{"&hourly=dew_point_2m&forecast_hours=1", U_TEMP},
	{"&timezone=auto&timeformat=unixtime&forecast_days=8"
	 "&daily=weather_code,temperature_2m_max,temperature_2m_min,sunrise,sunset", U_TEMP},
	/* timezone=auto again, so its days line up with request 3's. */
	{"&timezone=auto&forecast_days=8"
	 "&daily=precipitation_sum,uv_index_max,wind_speed_10m_max,wind_direction_10m_dominant",
	 U_WIND | U_PRECIP},
};
#define REQUESTS (int)(sizeof(requests) / sizeof(requests[0]))

const char *meteo_error(void)
{
	return error_text[0] ? error_text : net_error();
}

static void get(const char *json, const char *path, char *out, int len)
{
	json_get(json, path, out, len);
}

static long get_long(const char *json, const char *path)
{
	json_get(json, path, val, sizeof(val));
	return atol(val);
}

int locate_by_ip(LOCATION *loc)
{
	const char *json = http_get(ip_url);

	error_text[0] = '\0';
	if (!json)
		return 1;
	get(json, "/status", val, sizeof(val));
	if (strcmp(val, "success") != 0) {
		get(json, "/message", val, sizeof(val));
		snprintf(error_text, sizeof error_text, "ip-api: %s", val[0] ? val : "no location");
		return 1;
	}
	get(json, "/city", loc->city, sizeof(loc->city));
	get(json, "/countryCode", loc->country, sizeof(loc->country));
	get(json, "/lat", loc->lat, sizeof(loc->lat));
	get(json, "/lon", loc->lon, sizeof(loc->lon));
	return 0;
}

/* Append name to url, percent-encoding anything that is not unreserved. */
static int append_encoded(char *dst, const char *name, int room)
{
	static const char hex[] = "0123456789ABCDEF";
	int n = (int)strlen(dst);

	for (; *name; ++name) {
		unsigned char c = (unsigned char)*name;

		if (n + 4 >= room)
			return 0;
		if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
		    (c >= '0' && c <= '9') || c == '-' || c == '_' || c == '.') {
			dst[n++] = (char)c;
		} else {
			dst[n++] = '%';
			dst[n++] = hex[c >> 4];
			dst[n++] = hex[c & 15];
		}
	}
	dst[n] = '\0';
	return 1;
}

int locate_by_name(LOCATION *loc, const char *name)
{
	const char *json;

	error_text[0] = '\0';
	strcpy(url, geo_base);
	if (!append_encoded(url, name, URL_MAX - 40)) {
		strcpy(error_text, "place name too long");
		return 1;
	}
	strcat(url, "&count=1&language=en&format=json");
	json = http_get(url);
	if (!json)
		return 1;
	if (!json_get(json, "/results/0/name", loc->city, sizeof(loc->city)) ||
	    !loc->city[0]) {
		snprintf(error_text, sizeof error_text, "no place called \"%.30s\"", name);
		return 1;
	}
	get(json, "/results/0/country_code", loc->country, sizeof(loc->country));
	get(json, "/results/0/latitude", loc->lat, sizeof(loc->lat));
	get(json, "/results/0/longitude", loc->lon, sizeof(loc->lon));
	return 0;
}

static const char *fetch(const WEATHER *w, int i)
{
	snprintf(url, sizeof url, "%s?latitude=%s&longitude=%s%s", om_base, w->loc.lat, w->loc.lon,
	        requests[i].query);
	if (w->units == IMPERIAL) {
		if (requests[i].units & U_TEMP)
			strcat(url, "&temperature_unit=fahrenheit");
		if (requests[i].units & U_WIND)
			strcat(url, "&wind_speed_unit=mph");
		if (requests[i].units & U_PRECIP)
			strcat(url, "&precipitation_unit=inch");
	}
	return http_get(url);
}

/* Open-Meteo abbreviates many zones as "GMT-4"; name the common ones, as the
 * msdos port does.  The zone is on daylight time when the offset differs
 * from its standard offset. */
static const struct {
	const char *iana, *std_name, *dst_name;
	long std_offset;
} zones[] = {
	{"America/New_York", "EST", "EDT", -18000L},
	{"America/Chicago", "CST", "CDT", -21600L},
	{"America/Denver", "MST", "MDT", -25200L},
	{"America/Phoenix", "MST", "MST", -25200L},
	{"America/Los_Angeles", "PST", "PDT", -28800L},
	{"America/Anchorage", "AKST", "AKDT", -32400L},
	{"Pacific/Honolulu", "HST", "HST", -36000L},
	{"America/Halifax", "AST", "ADT", -14400L},
	{"America/Toronto", "EST", "EDT", -18000L},
	{"America/Vancouver", "PST", "PDT", -28800L},
	{"Europe/London", "GMT", "BST", 0L},
	{"Europe/Dublin", "GMT", "IST", 0L},
	{"Europe/Paris", "CET", "CEST", 3600L},
	{"Europe/Berlin", "CET", "CEST", 3600L},
	{"Europe/Amsterdam", "CET", "CEST", 3600L},
	{"Europe/Rome", "CET", "CEST", 3600L},
	{"Europe/Madrid", "CET", "CEST", 3600L},
	{"Europe/Stockholm", "CET", "CEST", 3600L},
	{"Europe/Helsinki", "EET", "EEST", 7200L},
	{"Asia/Tokyo", "JST", "JST", 32400L},
	{"Australia/Sydney", "AEST", "AEDT", 36000L},
	{"Pacific/Auckland", "NZST", "NZDT", 43200L},
};

static void name_zone(const char *iana, CURRENT *c)
{
	unsigned i;

	if (strncmp(c->tz_abbr, "GMT", 3) != 0 || c->tz_abbr[3] == '\0')
		return;         /* already a name, or plain GMT */
	for (i = 0; i < sizeof(zones) / sizeof(zones[0]); ++i) {
		if (strcmp(iana, zones[i].iana) == 0) {
			strcpy(c->tz_abbr, c->utc_offset == zones[i].std_offset
			                   ? zones[i].std_name : zones[i].dst_name);
			return;
		}
	}
}

static void daily(const char *json, const char *field, int day, char *out, int len)
{
	char path[48];

	snprintf(path, sizeof path, "/daily/%s/%d", field, day);
	get(json, path, out, len);
}

static long daily_long(const char *json, const char *field, int day)
{
	daily(json, field, day, val, sizeof(val));
	return atol(val);
}

int fetch_weather(WEATHER *w)
{
	CURRENT *c = &w->now;
	const char *json;
	int i, d;

	error_text[0] = '\0';
	for (i = 0; i < REQUESTS; ++i) {
		json = fetch(w, i);
		if (!json)
			return 1;
		if (json_get(json, "/error", val, sizeof(val)) && strcmp(val, "true") == 0) {
			get(json, "/reason", val, sizeof(val));
			snprintf(error_text, sizeof error_text, "open-meteo: %.40s", val);
			return 1;
		}
		switch (i) {
		case 0:
			c->time = get_long(json, "/current/time");
			c->utc_offset = get_long(json, "/utc_offset_seconds");
			get(json, "/timezone_abbreviation", c->tz_abbr, sizeof(c->tz_abbr));
			get(json, "/timezone", val, sizeof(val));
			name_zone(val, c);
			c->code = (int)get_long(json, "/current/weather_code");
			get(json, "/current/relative_humidity_2m", c->humidity, sizeof(c->humidity));
			get(json, "/current/cloud_cover", c->clouds, sizeof(c->clouds));
			get(json, "/current/surface_pressure", c->pressure, sizeof(c->pressure));
			break;
		case 1:
			get(json, "/current/temperature_2m", c->temp, sizeof(c->temp));
			get(json, "/current/apparent_temperature", c->feels_like, sizeof(c->feels_like));
			get(json, "/current/wind_speed_10m", c->wind_speed, sizeof(c->wind_speed));
			get(json, "/current/wind_direction_10m", c->wind_deg, sizeof(c->wind_deg));
			break;
		case 2:
			get(json, "/hourly/dew_point_2m/0", c->dew_point, sizeof(c->dew_point));
			break;
		case 3:
			for (d = 0; d < DAYS; ++d) {
				DAY *day = &w->day[d];

				day->time = daily_long(json, "time", d);
				day->code = (int)daily_long(json, "weather_code", d);
				daily(json, "temperature_2m_max", d, day->temp_max, sizeof(day->temp_max));
				daily(json, "temperature_2m_min", d, day->temp_min, sizeof(day->temp_min));
				day->sunrise = daily_long(json, "sunrise", d);
				day->sunset = daily_long(json, "sunset", d);
			}
			break;
		case 4:
			for (d = 0; d < DAYS; ++d) {
				DAY *day = &w->day[d];

				daily(json, "precipitation_sum", d, day->precip, sizeof(day->precip));
				daily(json, "uv_index_max", d, day->uv_max, sizeof(day->uv_max));
				daily(json, "wind_speed_10m_max", d, day->wind_speed, sizeof(day->wind_speed));
				daily(json, "wind_direction_10m_dominant", d, day->wind_deg,
				      sizeof(day->wind_deg));
			}
			break;
		}
	}
	return 0;
}

/* WMO weather interpretation codes, worded as in the msdos port. */
const char *wmo_text(int code)
{
	switch (code) {
	case 0:  return "Clear sky";
	case 1:  return "Mainly clear";
	case 2:  return "Partly cloudy";
	case 3:  return "Overcast";
	case 45: return "Fog";
	case 48: return "Rime fog";
	case 51: return "Light drizzle";
	case 53: return "Drizzle";
	case 55: return "Heavy drizzle";
	case 56: return "Light freezing drizzle";
	case 57: return "Freezing drizzle";
	case 61: return "Light rain";
	case 63: return "Rain";
	case 65: return "Heavy rain";
	case 66: return "Light freezing rain";
	case 67: return "Freezing rain";
	case 71: return "Light snow";
	case 73: return "Snow";
	case 75: return "Heavy snow";
	case 77: return "Snow grains";
	case 80: return "Light showers";
	case 81: return "Showers";
	case 82: return "Heavy showers";
	case 85: return "Light snow showers";
	case 86: return "Snow showers";
	case 95: return "Thunderstorm";
	case 96: return "Thunderstorm, hail";
	case 99: return "Thunderstorm, heavy hail";
	default: return "Unknown";
	}
}

const char *wind_dir(const char *degrees)
{
	static const char *const dirs[] = {"N", "NE", "E", "SE", "S", "SW", "W", "NW"};
	long deg = atol(degrees);

	/* Round to the nearest 45-degree sector; 360 wraps to N. */
	return dirs[((deg % 360 + 360) % 360 + 22) / 45 % 8];
}

const char *temp_unit(UNITOPT u)
{
	return u == IMPERIAL ? "\xb0" "F" : "\xb0" "C";
}

const char *speed_unit(UNITOPT u)
{
	return u == IMPERIAL ? "mph" : "km/h";
}

const char *precip_unit(UNITOPT u)
{
	return u == IMPERIAL ? "in" : "mm";
}
