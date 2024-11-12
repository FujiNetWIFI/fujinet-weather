/**
 * Weather / forecast.c
s *
 * Based on @bocianu's code
 *
 * @author Thomas Cherryhomes
 * @email thom dot cherryhomes at gmail dot com
 *
 */

#include <conio.h>
#include "constants.h"
#include "options.h"
#include "location.h"
#include "screen.h"
#include "io.h"
#include "sprite.h"
#include "direction.h"
#include "sprite.h"
#include "ftime.h"
#include "input.h"
#include "forecast.h"
#include "screen.h"
#include "utils.h"
#include "weather.h"

ForecastData forecastData;
unsigned char forecast_offset = 0;

extern unsigned short timer;
extern bool forceRefresh;

extern OptionsData optData;
extern Location locData;
extern char timezone[48];
extern long timezone_offset;;
extern unsigned long dt;

char request[512];

/*** FORECAST ***/


/*
forecast_open 
- Open the url containing the json data for the forecastd 

Returns
    true: Success
   false: Could not open the website
 
*/

#ifdef USE_METEO

// https://api.open-meteo.com/v1/forecast?latitude=52.52&longitude=13.41&daily=weather_code,temperature_2m_max,temperature_2m_min,uv_index_max,precipitation_sum,precipitation_probability_max,wind_speed_10m_max,wind_direction_10m_dominant&temperature_unit=fahrenheit&wind_speed_unit=mph&precipitation_unit=inch&timeformat=unixtime&forecast_days=16

bool forecast_open(void) // METEO
{
    Timestamp ts;
    char url[256];
    char units[80];

    get_timezone_from_position(timezone, sizeof(timezone), locData.latitude, locData.longitude);

    if (optData.units == METRIC) 
      strncpy2(units, "", sizeof(units));
    else
      strncpy2(units, "temperature_unit=fahrenheit&wind_speed_unit=mph&precipitation_unit=inch&", sizeof(units));

    snprintf(url, sizeof(url),
              "N:HTTP://" ME_API "//v1//forecast?"
              "latitude=%s&"
              "longitude=%s&"
              "daily=weather_code,temperature_2m_max,temperature_2m_min,uv_index_max,precipitation_sum,wind_speed_10m_max,wind_direction_10m_dominant&"
              "%s"
              "timeformat=unixtime&"
              "forecast_days=16&"
              "timezone=%s",
              locData.latitude, locData.longitude, units, timezone);

    if (io_json_open(url) == 0)
        return false;
    else
        return true;
}

#else

// HTTP://api.openweathermap.org/data/2.5/onecall?lat=44.62335968017578&lon=-63.57278060913086&exclude=minutely,hourly,alerts,current&units=%s&appid=2e8616654c548c26bc1c86b1615ef7f1

bool forecast_open(void) // OPEN_WEATHER
{
    Timestamp ts;
    char url[256];
    char units[10];

    if (optData.units == METRIC)
        strncpy2(units, "metric", sizeof(units));
    else if (optData.units == IMPERIAL)
        strncpy2(units, "imperial", sizeof(units));

    snprintf(url, sizeof(url), "N:HTTP://%s//data/2.5/onecall?lat=%s&lon=%s&exclude=minutely,hourly,alerts,current&units=%s&appid=%s", 
                OW_API, locData.latitude, locData.longitude, units,
                OW_KEY);

    if (io_json_open(url) == 0)
        return false;
    else
        return true;
}

#endif


bool forecast_close(void)
{
    io_json_close();

    return true;
}

/*
Collect the i-th forecast and parse the collected json 
and storing the information in the ForecastData structure
*/

#ifdef USE_METEO

void forecast_parse(unsigned char i, ForecastData *f) // METEO
{
    Timestamp ts;
    char prefix[20];
    char postfix[20];
    char json_part[30];
    char wmo[10];
    /*
{
  "latitude": 52.52,
  "longitude": 13.419998,
  "generationtime_ms": 0.534892082214356,
  "utc_offset_seconds": 0,
  "timezone": "GMT",
  "timezone_abbreviation": "GMT",
  "elevation": 38,
  "daily_units": {
    "time": "unixtime",
    "weather_code": "wmo code",
    "temperature_2m_max": "°F",
    "temperature_2m_min": "°F",
    "uv_index_max": "",
    "precipitation_sum": "inch",
    "precipitation_probability_max": "%",
    "wind_speed_10m_max": "mp/h",
    "wind_direction_10m_dominant": "°"
  },
  "daily": {
    "time": [1731283200, 1731369600, 1731456000, 1731542400, 1731628800, 1731715200, 1731801600, 1731888000, 1731974400, 1732060800, 1732147200, 1732233600, 1732320000, 1732406400, 1732492800,1732579200], 
    "weather_code": [45, 61, 61, 3, 3, 3, 3, 61, 3, 45, 3, 51, 3, 45, 45, 3], 
    "temperature_2m_max": [42.3, 40.9, 46.6, 45.7, 45.6, 46.4, 42.9, 41.2, 38.1, 37.2, 36.5, 36.3, 36.7, 36.1, 42.7, 46.8], 
    "temperature_2m_min": [35.1, 38.8, 37.3, 36.8, 41.2, 39.1, 36.8, 37.4, 32.8, 34.5, 34.1, 30.8, 30, 30, 32, 42.9], 
    "uv_index_max": [0.5, 1, 1.4, 1.5, 1.55, 1.45, 0.9, 0.75, 0.75, 0.1, 0.7, 1.05, 1.2, 1.2, 0.25, 0.15], 
    "precipitation_sum": [0, 0, 0.028, 0, 0, 0, 0, 0.283, 0, 0, 0, 0.059, 0, 0, 0, null],
    "precipitation_probability_max": [10, 18, 28, 15, 0, 5, 15, 23, 26, 23, 41, 42, 23, 19, 20, 30],
    "wind_speed_10m_max": [5.7, 5.3, 6, 6, 5.4, 6, 5.7, 7.5, 11.8, 14.8, 13.9, 11.2, 9.3, 6.8, 12.5, 8],
    "wind_direction_10m_dominant": [127, 52, 304, 250, 256, 206, 195, 239, 167, 247, 248, 220, 226, 270, 191, 169]
  }
}
    */

    io_json_query("/utc_offset_seconds", json_part, sizeof(json_part));
    timezone_offset = atol(json_part);

    snprintf(prefix, sizeof(prefix), "%s", "/daily/");
    snprintf(postfix, sizeof(postfix), "/%c", '0' + i);

    snprintf(request, sizeof(request), "%sweather_code%s", prefix, postfix);

    io_json_query(request, wmo, sizeof(wmo));

    snprintf(request, sizeof(request), "%stime%s", prefix,postfix);
    io_json_query(request, json_part, sizeof(json_part));
    timestamp(atol(json_part)+timezone_offset, &ts);

    snprintf(f->date, 8, "%d %s", ts.day, time_month(ts.month));
    snprintf(f->dow, 4, "%s", time_dow(ts.dow));

    get_description(wmo, f->desc);

    f->icon = get_sprite(wmo);

    snprintf(request, sizeof(request), "%stemperature_2m_min%s", prefix, postfix);
    io_json_query(request, f->lo, sizeof(f->lo));
    io_decimals(f->lo, optData.maxPrecision);

    snprintf(request, sizeof(request), "%stemperature_2m_max%s", prefix, postfix);
    io_json_query(request, f->hi, sizeof(f->hi));
    io_decimals(f->hi, optData.maxPrecision);

    //snprintf(request, sizeof(request), "%spressure", prefix);
    //io_json_query(request, f->pressure, sizeof(f->pressure));
    strncpy2(f->pressure, "", sizeof(f->pressure));

    snprintf(request, sizeof(request), "%swind_speed_10m_max%s", prefix, postfix);
    io_json_query(request, json_part, sizeof(json_part));
    io_decimals(json_part, optData.maxPrecision);
    snprintf(f->wind, sizeof(f->wind), "WIND:%s %s ", json_part, optData.units == IMPERIAL ? "mph" : "kph");

    snprintf(request, sizeof(request), "%swind_direction_10m_dominant%s", prefix, postfix);
    io_json_query(request, json_part, sizeof(json_part));
    strcat(f->wind, degToDirection(atoi(json_part)));

    snprintf(request, sizeof(request), "%sprecipitation_probability_max%s", prefix, postfix);
    io_json_query(request, f->pop, sizeof(f->pop));

    snprintf(request, sizeof(request), "%sprecipitation_sum%s", prefix, postfix);
    io_json_query(request, f->rain, sizeof(f->rain));

    // snprintf(request,  sizeof(request),"%ssnow", prefix);
    // io_json_query(request, f->snow, sizeof(f->snow));
    strncpy2(f->snow, "", sizeof(f->snow));
}

#else

void forecast_parse(unsigned char i, ForecastData *f) // OPEN_WEATHER
{
    Timestamp ts;
    char prefix[20];
    char request[60];
    char json_part[30];

    snprintf(prefix, sizeof(prefix), "/daily/%c/", '0' + i);

    snprintf(request, sizeof(request), "%sdt", prefix);
    io_json_query(request, json_part, sizeof(json_part));
    timestamp(atol(json_part), &ts);

    snprintf(f->date, 8, "%d %s", ts.day, time_month(ts.month));
    snprintf(f->dow, 4, "%s", time_dow(ts.dow));

    snprintf(request, sizeof(request), "%sweather/0/description", prefix);
    io_json_query(request, f->desc, sizeof(f->desc));

    snprintf(request, sizeof(request), "%sweather/0/icon", prefix);
    io_json_query(request, json_part, sizeof(json_part));
    f->icon = get_sprite(json_part);

    snprintf(request, sizeof(request), "%stemp/min", prefix);
    io_json_query(request, f->lo, sizeof(f->lo));
    io_decimals(f->lo, optData.maxPrecision);

    snprintf(request, sizeof(request), "%stemp/max", prefix);
    io_json_query(request, f->hi, sizeof(f->hi));
    io_decimals(f->hi, optData.maxPrecision);

    snprintf(request, sizeof(request), "%spressure", prefix);
    io_json_query(request, f->pressure, sizeof(f->pressure));

    snprintf(request, sizeof(request), "%swind_speed", prefix);
    io_json_query(request, json_part, sizeof(json_part));
    io_decimals(json_part, optData.maxPrecision);
    snprintf(f->wind, sizeof(f->wind), "WIND:%s %s ", json_part, optData.units == IMPERIAL ? "mph" : "kph");

    snprintf(request, sizeof(request), "%swind_deg", prefix);
    io_json_query(request, json_part, sizeof(json_part));
    strcat(f->wind, degToDirection(atoi(json_part)));

    snprintf(request, sizeof(request), "%spop", prefix);
    io_json_query(request, f->pop, sizeof(f->pop));

    snprintf(request, sizeof(request), "%srain", prefix);
    io_json_query(request, f->rain, sizeof(f->rain));

    // snprintf(request,  sizeof(request),"%ssnow", prefix);
    // io_json_query(request, f->snow, sizeof(f->snow));
    strncpy2(f->snow, "", sizeof(f->snow));
}

#endif

void forecast(void)
{
  unsigned char bg, fg;
  bool  dayNight;
static  bool firstTime = true;    
static  FUJI_TIME future_time;
static  FUJI_TIME adjust_time;

  if (firstTime)
  {
      firstTime = false;
      io_time(&future_time);
  }

  if (time_reached(&future_time) || forceRefresh)
  {
    forceRefresh = false;

    memset(&adjust_time, 0, sizeof(FUJI_TIME));

    adjust_time.minute = (unsigned char) optData.refreshIntervalMinutes;
    add_time(&future_time, &future_time, &adjust_time);

    screen_forecast_init();
    
    screen_colors(dt, timezone_offset, &fg, &bg, &dayNight);
    
    if (forecast_open())
      screen_weather_could_not_get();
    else
    { 

      clear_all_sprites();  
      for (int i=0;i<4;i++)
      {
        forecast_parse((unsigned char) (i + forecast_offset), &forecastData);
        screen_forecast((unsigned char) i, &forecastData, fg, bg, true, &future_time);
      }
      display_sprites();
      forecast_close();
      
    }

  }

  input_init();

  screen_forecast_keys();

  timer = 65535;
  while (timer > 0)
  {
    if (input_forecast())
    {
      if (forceRefresh)
        io_time(&future_time);
      
      break; // allow input_init to be called again
    }

    if ((timer % CHECK_TIME_FREQUENCY) == 0)
    {
      if (time_reached(&future_time))
        break;
    }
    
    csleep(1);
    timer--;
   }

}
