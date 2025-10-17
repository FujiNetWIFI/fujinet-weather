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

int isspace(int c)
{
    // Standard whitespace characters include:
    // space (0x20), form feed (0x0c), line feed (0x0a), carriage return (0x0d),
    // horizontal tab (0x09), vertical tab (0x0b)

    // It's important to cast 'c' to unsigned char before comparison to handle negative char values correctly,
    // unless 'c' is EOF.
    unsigned char uc = (unsigned char)c;

    return (uc == ' ' || uc == '\f' || uc == '\n' || uc == '\r' || uc == '\t' || uc == '\v');
}

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

void truncate_at_first_non_printable(char *str)
{
    if (str == NULL)
    {
        return; // Handle null pointer case
    }

    int i = 0;
    while (str[i] != '\0')
    {
        if (!isprint((unsigned char)str[i]))
        {
            str[i] = '\0'; // Truncate the string
            return;
        }
        i++;
    }
}

int16_t network_json_query_trim(const char *devicespec, const char *query, char *s)
{
    int16_t ret = network_json_query(devicespec, query, s);
    truncate_at_first_non_printable(s);
    return ret;
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

    printf("ip-api open: %s\n", ip_url);
    err = network_open(ip_url, OPEN_MODE_READ, OPEN_TRANS_NONE);
    sprintf(linebuf, "ip-api open ret: %02X", err);
    printstr(linebuf);
    putchar('\n');

    putstr("ip-api parse\n", 13);
    err = network_json_parse(ip_url);
    sprintf(linebuf, "ip-api parse ret: %02X", err);
    printstr(linebuf);
    putchar('\n');

    memset(buf, 0, LINE_LEN);

    printf("ip-api query status\n");
    network_json_query_trim(ip_url, "/status", buf);
    sprintf(linebuf, "status: %s", buf);
    printstr(linebuf);
    putchar('\n');
    
    if (strcmp(buf, "success") != 0)
    {
        memset(buf, 0, LINE_LEN);
        network_json_query_trim(ip_url, "/message", buf);
        network_close(ip_url);
        sprintf(message, "ip-api(%s)", buf);
        err = 0xff; // set unknown error
    }

    printf("ip-api query city\n");
    network_json_query_trim(ip_url, "/city", loc->city);
    sprintf(linebuf, "city; %s", loc->city);
    printstr(linebuf);
    putchar('\n');
    printf("ip-api query countryCode\n");
    network_json_query_trim(ip_url, "/countryCode", loc->countryCode);
    sprintf(linebuf, "countryCode: %s", loc->countryCode);
    printstr(linebuf);
    putchar('\n');
    printf("ip-api query lon\n");
    network_json_query_trim(ip_url, "/lon", loc->lon);
    sprintf(linebuf, "lon: %s", loc->lon);
    printstr(linebuf);
    putchar('\n');
    printf("ip-api query lat\n");
    network_json_query_trim(ip_url, "/lat", loc->lat);
    sprintf(linebuf, "lat: %s", loc->lat);
    printstr(linebuf);
    putchar('\n');

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
        printf("query date (day %d) %s\n", i, querybuf);
        network_json_query_trim(omurl, querybuf, prbuf);
        sprintf(linebuf, "date: %s", prbuf);
        printstr(linebuf);
        putchar('\n');
        strcpy(fc->day[i].date, prbuf);
        // sunrise
        sprintf(querybuf, "/daily/sunrise/%d", i);
        printf("query sunrise (day %d) %s\n", i, querybuf);
        network_json_query_trim(omurl, querybuf, prbuf);
        sprintf(linebuf, "sunrise: %s", prbuf);
        printstr(linebuf);
        putchar('\n');
        strcpy(fc->day[i].sunrise, prbuf);
        // sunset
        sprintf(querybuf, "/daily/sunset/%d", i);
        printf("query sunset (day %d) %s\n", i, querybuf);
        network_json_query_trim(omurl, querybuf, prbuf);
        sprintf(linebuf, "sunset: %s", prbuf);
        printstr(linebuf);
        putchar('\n');  
        strcpy(fc->day[i].sunset, prbuf);
        // temp min
        sprintf(querybuf, "/daily/temperature_2m_min/%d", i);
        printf("query temp min (day %d) %s\n", i, querybuf);
        network_json_query_trim(omurl, querybuf, fc->day[i].temp_min);
        sprintf(linebuf, "temp_min: %s", fc->day[i].temp_min);
        printstr(linebuf);
        putchar('\n');
        // temp max
        sprintf(querybuf, "/daily/temperature_2m_max/%d", i);
        printf("query temp max (day %d) %s\n", i, querybuf);
        network_json_query_trim(omurl, querybuf, fc->day[i].temp_max);
        sprintf(linebuf, "temp_max: %s", fc->day[i].temp_max);
        printstr(linebuf);
        putchar('\n');
        // icon
        sprintf(querybuf, "/daily/weather_code/%d", i);
        printf("query weather_code (day %d) %s\n", i, querybuf);
        network_json_query_trim(omurl, querybuf, prbuf);
        sprintf(linebuf, "weather_code: %s", prbuf);
        printstr(linebuf);
        putchar('\n');
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
        printf("query precipitation_sum (day %d) %s\n", i, querybuf);
        network_json_query_trim(omurl, querybuf, fc->day[i].precipitation_sum);
        sprintf(linebuf, "precipitation_sum: %s", fc->day[i].precipitation_sum);
        printstr(linebuf);
        putchar('\n');
        // uv index  max
        sprintf(querybuf, "/daily/uv_index_max/%d", i);
        printf("query uv_index_max (day %d) %s\n", i, querybuf);
        network_json_query_trim(omurl, querybuf, fc->day[i].uv_index_max);
        sprintf(linebuf, "uv_index_max: %s", fc->day[i].uv_index_max);
        printstr(linebuf);
        putchar('\n');
        // wind  speed
        sprintf(querybuf, "/daily/wind_speed_10m_max/%d", i);
        printf("query wind_speed_10m_max (day %d) %s\n", i, querybuf);  
        network_json_query_trim(omurl, querybuf, fc->day[i].wind_speed);
        sprintf(linebuf, "wind_speed_10m_max: %s", fc->day[i].wind_speed);
        printstr(linebuf);
        putchar('\n');
        // wind  deg
        sprintf(querybuf, "/daily/wind_direction_10m_dominant/%d", i);
        printf("query wind_direction_10m_dominant (day %d) %s\n", i, querybuf);
        network_json_query_trim(omurl, querybuf, fc->day[i].wind_deg);
        sprintf(linebuf, "wind_direction_10m_dominant: %s", fc->day[i].wind_deg);
        printstr(linebuf);
        putchar('\n');
    }
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

    putstr("om parse\n", 9);
    err = network_json_parse(omurl);
    sprintf(linebuf, "om parse ret: %02X", err);
    printstr(linebuf);
    putchar('\n');

    //	city name, country code
    strcpy(wi->name, loc->city);
    strcpy(wi->country, loc->countryCode);

    //  date & time
    putstr("query time\n", 11);
    network_json_query_trim(omurl, "/current/time", querybuf);
    sprintf(linebuf, "time: %s", querybuf);
    printstr(linebuf);
    putchar('\n');
    strcpy(wi->datetime, querybuf);
    //  timezone(offset)
    putstr("query utc_offset_seconds\n", 25);
    network_json_query_trim(omurl, "/utc_offset_seconds", querybuf);
    sprintf(linebuf, "utc_offset_seconds: %s", querybuf);
    printstr(linebuf);
    putchar('\n');
    wi->tz = atol(querybuf);
    // timezone
    putstr("query timezone\n", 15);
    network_json_query_trim(omurl, "/timezone", wi->timezone);
    sprintf(linebuf, "timezone: %s", wi->timezone);
    printstr(linebuf);
    putchar('\n');
    //  pressure
    putstr("query surface_pressure\n", 23);
    network_json_query_trim(omurl, "/current/surface_pressure", wi->pressure);
    sprintf(linebuf, "surface_pressure: %s", wi->pressure);
    printstr(linebuf);
    putchar('\n');
    //  humidity
    putstr("query relative_humidity_2m\n", 28);
    network_json_query_trim(omurl, "/current/relative_humidity_2m", wi->humidity);
    sprintf(linebuf, "relative_humidity_2m: %s", wi->humidity);
    printstr(linebuf);
    putchar('\n');
    // weather code (icon)
    putstr("query weather_code\n", 19);
    network_json_query_trim(omurl, "/current/weather_code", querybuf);
    sprintf(linebuf, "weather_code: %s", querybuf);
    printstr(linebuf);
    putchar('\n');
    wi->icon = (char)atoi(querybuf);
    //  clouds
    putstr("query cloud_cover\n", 19);
    network_json_query_trim(omurl, "/current/cloud_cover", wi->clouds);
    sprintf(linebuf, "cloud_cover: %s", wi->clouds);
    printstr(linebuf);
    putchar('\n');

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

    putstr("om parse 2\n", 11);
    err = network_json_parse(omurl);
    sprintf(linebuf, "om parse ret 2: %02X", err);
    printstr(linebuf);
    putchar('\n');

    //  temperature
    putstr("query temperature_2m\n", 21);
    network_json_query_trim(omurl, "/current/temperature_2m", wi->temp);
    sprintf(linebuf, "temperature_2m: %s", wi->temp);
    printstr(linebuf);
    putchar('\n');
    //  feels_like
    putstr("query apparent_temperature\n", 27);
    network_json_query_trim(omurl, "/current/apparent_temperature", wi->feels_like);
    sprintf(linebuf, "apparent_temperature: %s", wi->feels_like);
    printstr(linebuf);
    putchar('\n');

    //  dew_point
    putstr("query dew_point_2m\n", 19);
    network_json_query_trim(omurl, "/hourly/dew_point_2m/0", wi->dew_point);
    sprintf(linebuf, "dew_point_2m: %s", wi->dew_point);
    printstr(linebuf);
    putchar('\n');

    //  visibility
    putstr("query visibility\n", 17);
    network_json_query_trim(omurl, "/hourly/visibility/0", wi->visibility);
    sprintf(linebuf, "visibility: %s", wi->visibility);
    printstr(linebuf);
    putchar('\n');

    //  wind_speed
    putstr("query wind_speed_10m\n", 21);
    network_json_query_trim(omurl, "/current/wind_speed_10m", wi->wind_speed);
    sprintf(linebuf, "wind_speed_10m: %s", wi->wind_speed);
    printstr(linebuf);
    putchar('\n');

    //  wind_deg
    putstr("query wind_direction_10m\n", 25);
    err = network_json_query_trim(omurl, "/current/wind_direction_10m", wi->wind_deg);
    sprintf(linebuf, "wind_direction_10m: %s", wi->wind_deg);
    printstr(linebuf);
    putchar('\n');
 

    putstr("om close 2\n", 11);
    network_close(omurl); // of weather2
    waitkey(1);

    //	forecast
    //  part 1
    setup_omurl(loc, om_tail_forecast1);

    printf("om open forecast 1: %s\n", omurl);
    err = network_open(omurl, OPEN_MODE_READ, OPEN_TRANS_NONE);
    sprintf(linebuf, "om open ret forecast 1: %02X", err);
    printstr(linebuf);
    putchar('\n');

    putstr("om parse forecast 1\n", 20);
    err = network_json_parse(omurl);
    sprintf(linebuf, "om parse ret forecast 1: %02X", err);
    printstr(linebuf);
    putchar('\n');

    set_forecast1(fc);

    printf("om close forecast 1\n");    
    network_close(omurl); // of forecast part 1
    waitkey(1);

    //  copy today's sunrise/sunset from forecat data to weather data
    strcpy(wi->sunrise, fc->day[0].sunrise);
    strcpy(wi->sunset, fc->day[0].sunset);

    //  part 2
    setup_omurl(loc, om_tail_forecast2);

    printf("om open forecast 2: %s\n", omurl);
    err = network_open(omurl, OPEN_MODE_READ, OPEN_TRANS_NONE);
    sprintf(linebuf, "om open ret forecast 2: %02X", err);
    printstr(linebuf);
    putchar('\n');

    putstr("om parse forecast 2\n", 20);
    err = network_json_parse(omurl);
    sprintf(linebuf, "om parse ret forecast 2: %02X", err);
    printstr(linebuf);
    putchar('\n');

    set_forecast2(fc);

    printf("om close forecast 2\n");
    network_close(omurl); // of forecast part 2
    waitkey(1);
}

int main(void)
{
    initCoCoSupport();
    width(80);
    cls(1);

    get_location(&loc);
    waitkey(1);
    cls(1);
    get_om_info(&loc, &wi, &fc);
    width(32);
    return(0);
}