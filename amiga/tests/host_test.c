/*
 * Host-side tests for fnweather's portable code: json.c, timeutil.c and the
 * request/parse logic in meteo.c.  http_get() is stubbed with responses
 * saved in tests/fixtures/.  FNW_LIVE=1 fetches them with curl instead (and
 * rewrites the fixtures), which checks every URL against the real services.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "json.h"
#include "meteo.h"
#include "net.h"
#include "timeutil.h"

static int failures;
static int max_url;

#define CHECK(cond) do { if (!(cond)) { \
	printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond); ++failures; } } while (0)
#define CHECK_STR(got, want) do { if (strcmp((got), (want)) != 0) { \
	printf("FAIL %s:%d: \"%s\" != \"%s\"\n", __FILE__, __LINE__, (got), (want)); \
	++failures; } } while (0)

/* ---- http_get stub ------------------------------------------------------ */

static char body[BODY_MAX + 1];
/* Set while probing URL lengths only: serve the last body, fetch nothing,
 * so the probe cannot overwrite fixtures in FNW_LIVE mode. */
static int url_only;

const char *net_error(void) { return "stub error"; }

static const char *fixture_for(const char *url)
{
	static const struct { const char *needle, *file; } map[] = {
		{"ip-api.com", "ip"},
		{"geocoding-api", "geo"},
		{"current=weather_code", "current1"},
		{"current=temperature_2m", "current2"},
		{"hourly=dew_point_2m", "hourly"},
		{"daily=weather_code", "daily1"},
		{"daily=precipitation_sum", "daily2"},
	};
	size_t i;

	for (i = 0; i < sizeof(map) / sizeof(map[0]); ++i)
		if (strstr(url, map[i].needle))
			return map[i].file;
	return NULL;
}

const char *http_get(const char *url)
{
	char path[128], cmd[512];
	const char *name = fixture_for(url);
	FILE *f;
	size_t n;

	if ((int)strlen(url) > max_url)
		max_url = (int)strlen(url);
	if (strlen(url) >= 256) {
		printf("FAIL URL over the Amiga 256-byte limit (%u): %s\n",
		       (unsigned)strlen(url), url);
		++failures;
	}
	if (url_only)
		return body;
	if (!name) {
		printf("FAIL no fixture for %s\n", url);
		++failures;
		return NULL;
	}
	sprintf(path, "tests/fixtures/%s.json", name);
	if (getenv("FNW_LIVE")) {
		sprintf(cmd, "curl -sf '%s' -o %s", url, path);
		if (system(cmd) != 0) {
			printf("FAIL live fetch %s\n", url);
			++failures;
			return NULL;
		}
	}
	f = fopen(path, "rb");
	if (!f) {
		printf("FAIL missing %s (run with FNW_LIVE=1)\n", path);
		++failures;
		return NULL;
	}
	n = fread(body, 1, BODY_MAX, f);
	fclose(f);
	body[n] = '\0';
	CHECK(n < BODY_MAX);          /* every response must fit the Amiga buffer */
	return body;
}

/* ---- tests -------------------------------------------------------------- */

static void test_json(void)
{
	static const char doc[] =
		" {\"a\": 1, \"s\": \"he said \\\"hi\\\"\", \"n\": null,"
		"  \"o\": {\"x\": [10, [20, 21], {\"y\": \"deep\"}], \"t\": true},"
		"  \"skip\": {\"k\": \"}]\"}, \"neg\": -3.5e2 }";
	char v[32];

	CHECK(json_get(doc, "/a", v, sizeof v)); CHECK_STR(v, "1");
	CHECK(json_get(doc, "/s", v, sizeof v)); CHECK_STR(v, "he said \"hi\"");
	CHECK(json_get(doc, "/n", v, sizeof v)); CHECK_STR(v, "");
	CHECK(json_get(doc, "/o/x/0", v, sizeof v)); CHECK_STR(v, "10");
	CHECK(json_get(doc, "/o/x/1/1", v, sizeof v)); CHECK_STR(v, "21");
	CHECK(json_get(doc, "/o/x/2/y", v, sizeof v)); CHECK_STR(v, "deep");
	CHECK(json_get(doc, "/o/t", v, sizeof v)); CHECK_STR(v, "true");
	CHECK(json_get(doc, "/neg", v, sizeof v)); CHECK_STR(v, "-3.5e2");
	CHECK(json_get(doc, "/skip/k", v, sizeof v)); CHECK_STR(v, "}]");
	CHECK(!json_get(doc, "/missing", v, sizeof v)); CHECK_STR(v, "");
	CHECK(!json_get(doc, "/o/x/3", v, sizeof v));
	CHECK(!json_get(doc, "/a/b", v, sizeof v));
	CHECK(!json_get("{\"a\": [1, 2", "/a/5", v, sizeof v));   /* truncated */
	CHECK(json_get(doc, "/s", v, 4)); CHECK_STR(v, "he ");     /* bounded */
}

static void test_time(void)
{
	LTIME t;

	to_ltime(0, 0, &t);
	CHECK(t.year == 1970 && t.month == 1 && t.mday == 1 && t.wday == 4);
	/* 2026-09-27 20:05 UTC is 16:05 EDT (-4h), a Sunday. */
	to_ltime(1790539500L, -14400L, &t);
	CHECK(t.year == 2026 && t.month == 9 && t.mday == 27);
	CHECK(t.hour == 16 && t.min == 5 && t.wday == 0);
	to_ltime(951782400L, 0, &t);          /* 2000-02-29, leap day */
	CHECK(t.year == 2000 && t.month == 2 && t.mday == 29 && t.wday == 2);
	CHECK_STR(wday_name(t.wday), "Tue");
	CHECK_STR(month_name(12), "Dec");
}

static void test_helpers(void)
{
	CHECK_STR(wind_dir("0"), "N");
	CHECK_STR(wind_dir("22"), "N");
	CHECK_STR(wind_dir("23"), "NE");
	CHECK_STR(wind_dir("315"), "NW");
	CHECK_STR(wind_dir("359"), "N");
	CHECK_STR(wind_dir("360"), "N");
	CHECK_STR(wmo_text(63), "Rain");
	CHECK_STR(wmo_text(42), "Unknown");
}

static void check_weather(const WEATHER *w)
{
	int d;

	CHECK(w->now.time > 1700000000L);
	CHECK(w->now.tz_abbr[0] != '\0');
	CHECK(strncmp(w->now.tz_abbr, "GMT-", 4) != 0);   /* named via the table */
	CHECK(w->now.temp[0] && w->now.feels_like[0] && w->now.humidity[0]);
	CHECK(w->now.dew_point[0] && w->now.pressure[0] && w->now.wind_speed[0]);
	for (d = 0; d < DAYS; ++d) {
		CHECK(w->day[d].time > 1700000000L);
		CHECK(w->day[d].temp_max[0] && w->day[d].temp_min[0]);
		CHECK(w->day[d].precip[0] && w->day[d].wind_deg[0]);
		if (d)
			CHECK(w->day[d].time - w->day[d - 1].time >= 82800L);  /* ~1 day */
	}
	CHECK(w->day[0].sunrise > w->day[0].time && w->day[0].sunset > w->day[0].sunrise);
}

static void test_meteo(void)
{
	WEATHER w;
	LOCATION far;

	memset(&w, 0, sizeof w);
	CHECK(locate_by_ip(&w.loc) == 0);
	CHECK(w.loc.city[0] && w.loc.lat[0] && w.loc.lon[0]);
	CHECK(fetch_weather(&w) == 0);
	check_weather(&w);

	/* Worst-case coordinate lengths and imperial units: URL limit check. */
	strcpy(w.loc.lat, "-33.868820");
	strcpy(w.loc.lon, "-151.209290");
	w.units = IMPERIAL;
	url_only = 1;
	CHECK(fetch_weather(&w) == 0);
	url_only = 0;

	CHECK(locate_by_name(&far, "New York") == 0);
	CHECK(far.city[0] && far.lat[0] && far.lon[0] && far.country[0]);
}

int main(void)
{
	test_json();
	test_time();
	test_helpers();
	test_meteo();
	printf("longest URL %d bytes (limit 255)\n", max_url);
	printf(failures ? "%d FAILED\n" : "all host tests passed\n", failures);
	return failures != 0;
}
