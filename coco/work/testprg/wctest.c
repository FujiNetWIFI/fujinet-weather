#include <cmoc.h>
#include <coco.h>

#include "fujinet-network.h"
#include "weatherdefs.h"

char linebuf[512];

char omurl[256];
char om_head[] = "N:https://api.open-meteo.com/v1/forecast?latitude=";
char om_lon[] = "&longitude=";

char om_tail_weather1[] = "&timezone=auto&current=relative_humidity_2m,weather_code,cloud_cover,surface_pressure";
char om_tail_weather2[] = "&current=temperature_2m,apparent_temperature,,wind_speed_10m,wind_direction_10m&hourly=dew_point_2m,visibility&forecast_hours=1";

char om_tail_forecast1[] = "&forecast_days=8&forecast_hours=1&daily=weather_code,temperature_2m_max,temperature_2m_min,sunrise,sunset";
char om_tail_forecast2[] = "&forecast_days=8&forecast_hours=1&daily=wind_speed_10m_max,wind_direction_10m_dominant,precipitation_sum,uv_index_max";

/* unit option string */
char *unit_str[] = {"&wind_speed_unit=ms", "&temperature_unit=fahrenheit&wind_speed_unit=mph"};

/* geocoding api */
char om_geocoding_head[] = "N:https://geocoding-api.open-meteo.com/v1/search?name=";
char om_geocoding_tail[] = "&count=1&language=en&format=json";

char city[40];

UNITOPT unit_opt;
int err;

LOCATION loc;
WEATHER wi;
FORECAST fc;

char ip_url[] = "N:http://ip-api.com/json/?fields=status,city,countryCode,lon,lat";

int isprint(int c)
{
    // Check if c falls within the ASCII range for printable characters (0x20 to 0x7E)
    // This range includes space (0x20) up to tilde (0x7E).
    if (c >= 0x20 && c <= 0x7E)
    {
        return 1; // It is a printable character
    }
    else
    {
        return 0; // It is not a printable character
    }
}

void printstr(const char *s)
{
    while (*s)
    {
        char c = *s++;
        if (isprint(c))
        {
            putchar(c);
        }
        else
        {
            // Print non-printable characters in hexadecimal format
            printf("\\x%02X", (unsigned char)c);
        }
    }
}

//
// get location info from ip
// returns true: call is success
//         false:     is not success
//
void get_location(LOCATION *loc)
{
    char buf[LINE_LEN];
    char message[LINE_LEN];

    err = network_open(ip_url, OPEN_MODE_READ, OPEN_TRANS_NONE);

    err = network_json_parse(ip_url);

    memset(buf, 0, LINE_LEN);

    network_json_query(ip_url, "/status", buf);
    if (strcmp(buf, "success\n") != 0)
    {
        memset(buf, 0, LINE_LEN);
        network_json_query(ip_url, "/message", buf);
        network_close(ip_url);
        sprintf(message, "ip-api(%s)", buf);
        err = 0xff; // set unknown error
    }

    network_json_query(ip_url, "/city", loc->city);
    network_json_query(ip_url, "/countryCode", loc->countryCode);
    network_json_query(ip_url, "/lon", loc->lon);
    network_json_query(ip_url, "/lat", loc->lat);

    network_close(ip_url);
}

//
// setup Open-Metro URL
//
void setup_omurl(LOCATION *loc, char *param)
{
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
void get_om_info(LOCATION *loc, WEATHER *wi, FORECAST *fc)
{
    char querybuf[LINE_LEN];

    // weather 1 query
    setup_omurl(loc, om_tail_weather1);

    sprintf(linebuf, "om open: %s", omurl);
    putstr(linebuf, strlen(linebuf));
    putchar('\n');
    err = network_open(omurl, OPEN_MODE_READ, OPEN_TRANS_NONE);
    sprintf(linebuf, "om open ret: %02X", err);
    printstr(linebuf);
    putchar('\n');
    waitkey(1);

    putstr("om parse\n", 9);
    err = network_json_parse(omurl);
    sprintf(linebuf, "om parse ret: %02X", err);
    printstr(linebuf);
    putchar('\n');
    waitkey(1);

    //	city name, country code
    strcpy(wi->name, loc->city);
    strcpy(wi->country, loc->countryCode);

    //  date & time
    putstr("query time\n", 11);
    network_json_query(omurl, "/current/time", querybuf);
    sprintf(linebuf, "time: %s", querybuf);
    printstr(linebuf);
    putchar('\n');
    waitkey(1);
    strcpy(wi->datetime, querybuf);
    //  timezone(offset)
    putstr("query utc_offset_seconds\n", 25);
    network_json_query(omurl, "/utc_offset_seconds", querybuf);
    sprintf(linebuf, "utc_offset_seconds: %s", querybuf);
    printstr(linebuf);
    putchar('\n');
    waitkey(1);
    wi->tz = atol(querybuf);
    // timezone
    putstr("query timezone\n", 15);
    network_json_query(omurl, "/timezone", wi->timezone);
    sprintf(linebuf, "timezone: %s", wi->timezone);
    printstr(linebuf);
    putchar('\n');
    waitkey(1);
    //  pressure
    putstr("query surface_pressure\n", 23);
    network_json_query(omurl, "/current/surface_pressure", wi->pressure);
    sprintf(linebuf, "surface_pressure: %s", wi->pressure);
    printstr(linebuf);
    putchar('\n');
    waitkey(1);
    //  humidity
    putstr("query relative_humidity_2m\n", 28);
    network_json_query(omurl, "/current/relative_humidity_2m", wi->humidity);
    sprintf(linebuf, "relative_humidity_2m: %s", wi->humidity);
    printstr(linebuf);
    putchar('\n');
    waitkey(1);
    // weather code (icon)
    putstr("query weather_code\n", 19);
    network_json_query(omurl, "/current/weather_code", querybuf);
    sprintf(linebuf, "weather_code: %s", querybuf);
    printstr(linebuf);
    putchar('\n');
    waitkey(1);
    wi->icon = (char)atoi(querybuf);
    //  clouds
    putstr("query cloud_cover\n", 19);
    network_json_query(omurl, "/current/cloud_cover", wi->clouds);
    sprintf(linebuf, "cloud_cover: %s", wi->clouds);
    printstr(linebuf);
    putchar('\n');
    waitkey(1);

    putstr("om close\n", 9);
    network_close(omurl); // of weather1
    waitkey(1);

    // weather 2 query
    setup_omurl(loc, om_tail_weather2);

    sprintf(linebuf, "om open 2: %s", omurl);
    putstr(linebuf, strlen(linebuf));   
    putchar('\n');
    err = network_open(omurl, OPEN_MODE_READ, OPEN_TRANS_NONE);
    sprintf(linebuf, "om open ret 2: %02X", err);
    printstr(linebuf);
    putchar('\n');
    waitkey(1);

    putstr("om parse 2\n", 11);
    err = network_json_parse(omurl);
    sprintf(linebuf, "om parse ret 2: %02X", err);
    printstr(linebuf);
    putchar('\n');
    waitkey(1);

    //  temperature
    putstr("query temperature_2m\n", 21);
    network_json_query(omurl, "/current/temperature_2m", wi->temp);
    sprintf(linebuf, "temperature_2m: %s", wi->temp);
    printstr(linebuf);
    putchar('\n');
    waitkey(1);
    //  feels_like
    putstr("query apparent_temperature\n", 27);
    network_json_query(omurl, "/current/apparent_temperature", wi->feels_like);
    sprintf(linebuf, "apparent_temperature: %s", wi->feels_like);
    printstr(linebuf);
    putchar('\n');
    waitkey(1);
    //  dew_point
    putstr("query dew_point_2m\n", 19);
    network_json_query(omurl, "/hourly/dew_point_2m/0", wi->dew_point);
    sprintf(linebuf, "dew_point_2m: %s", wi->dew_point);
    printstr(linebuf);
    putchar('\n');
    waitkey(1);
    //  visibility
    putstr("query visibility\n", 17);
    network_json_query(omurl, "/hourly/visibility/0", wi->visibility);
    sprintf(linebuf, "visibility: %s", wi->visibility);
    printstr(linebuf);
    putchar('\n');
    waitkey(1);
    //  wind_speed
    putstr("query wind_speed_10m\n", 21);
    network_json_query(omurl, "/current/wind_speed_10m", wi->wind_speed);
    sprintf(linebuf, "wind_speed_10m: %s", wi->wind_speed);
    printstr(linebuf);
    putchar('\n');
    waitkey(1);
    //  wind_deg
    putstr("query wind_direction_10m\n", 25);
    err = network_json_query(omurl, "/current/wind_direction_10m", wi->wind_deg);
    sprintf(linebuf, "wind_direction_10m: %s", wi->wind_deg);
    printstr(linebuf);
    putchar('\n');
    waitkey(1);

    putstr("om close 2\n", 11);
    network_close(omurl); // of weather2
    waitkey(1);

    // //	forecast
    // //  part 1
    // 	setup_omurl(loc, om_tail_forecast1);

    // 	progress_dots(2);

    //     err = network_open(omurl, OPEN_MODE_READ, OPEN_TRANS_NONE);
    //     handle_err("forecast 1 open");

    // 	progress_dots(3);

    //     err = network_json_parse(omurl);
    //     handle_err("forecast 1 parse");

    // 	set_forecast1(fc);
    // 	network_close(omurl);	// of forecast part 1

    // //  copy today's sunrise/sunset from forecat data to weather data
    // 	strcpy(wi->sunrise, fc->day[0].sunrise);
    // 	strcpy(wi->sunset, fc->day[0].sunset);

    // //  part 2
    // 	setup_omurl(loc, om_tail_forecast2);

    // 	progress_dots(4);

    //     err = network_open(omurl, OPEN_MODE_READ, OPEN_TRANS_NONE);
    //     handle_err("forecast 2 open");

    //     err = network_json_parse(omurl);
    //     handle_err("forecast 2 parse");

    // 	progress_dots(5);

    // 	set_forecast2(fc);

    // 	network_close(omurl);	// of forecast part 2

    // 	progress_dots(6);
}

//
// set forecast data part1
//
void set_forecast1(FORECAST *fc)
{
    char i;
    char querybuf[LINE_LEN];
    char prbuf[LINE_LEN];

    for (i = 0; i <= 7; i++)
    {
        // date & time
        sprintf(querybuf, "/daily/time/%d", i);
        network_json_query(omurl, querybuf, prbuf);
        strcpy(fc->day[i].date, prbuf);
        // sunrise
        sprintf(querybuf, "/daily/sunrise/%d", i);
        network_json_query(omurl, querybuf, prbuf);
        strcpy(fc->day[i].sunrise, prbuf);
        // sunset
        sprintf(querybuf, "/daily/sunset/%d", i);
        network_json_query(omurl, querybuf, prbuf);
        strcpy(fc->day[i].sunset, prbuf);
        // temp min
        sprintf(querybuf, "/daily/temperature_2m_min/%d", i);
        network_json_query(omurl, querybuf, fc->day[i].temp_min);
        // temp max
        sprintf(querybuf, "/daily/temperature_2m_max/%d", i);
        network_json_query(omurl, querybuf, fc->day[i].temp_max);
        // icon
        sprintf(querybuf, "/daily/weather_code/%d", i);
        network_json_query(omurl, querybuf, prbuf);
        fc->day[i].icon = (char)atoi(prbuf);
    }
}
//
// set forecast data part 2
//
void set_forecast2(FORECAST *fc)
{
    char i;
    char querybuf[LINE_LEN];

    for (i = 0; i <= 7; i++)
    {
        // precipitation sum
        sprintf(querybuf, "/daily/precipitation_sum/%d", i);
        network_json_query(omurl, querybuf, fc->day[i].precipitation_sum);
        // uv index  max
        sprintf(querybuf, "/daily/uv_index_max/%d", i);
        network_json_query(omurl, querybuf, fc->day[i].uv_index_max);
        // wind  speed
        sprintf(querybuf, "/daily/wind_speed_10m_max/%d", i);
        network_json_query(omurl, querybuf, fc->day[i].wind_speed);
        // wind  deg
        sprintf(querybuf, "/daily/wind_direction_10m_dominant/%d", i);
        network_json_query(omurl, querybuf, fc->day[i].wind_deg);
    }
}

int main(void)
{
    initCoCoSupport();
    width(80);
    cls(1);

    get_location(&loc);
    get_om_info(&loc, &wi, &fc);
    width(32);
    return(0);
}