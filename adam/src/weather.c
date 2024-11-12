/**
 * Weather / weather.c
 *
 * Based on @bocianu's code
 *
 * @author Thomas Cherryhomes
 * @email thom dot cherryhomes at gmail dot com
 *
 */

#include <conio.h>
#include "constants.h"
#include "weather.h"
#include "options.h"
#include "location.h"
#include "screen.h"
#include "io.h"
#include "sprite.h"
#include "direction.h"
#include "ftime.h"
#include "input.h"
#include "utils.h"
#include "state.h"

extern OptionsData optData;
extern Location locData;

unsigned long dt, sunrise, sunset;
extern bool forceRefresh;
extern unsigned int timer;

char date_txt[32];
char sunrise_txt[16];
char sunset_txt[16];
char time_txt[16];
char feels_like[16];
long timezone_offset;
char pressure[14];
char humidity[16];
char dew_point[16];
char clouds[16];
char visibility[16];
char wind_speed[16];
char wind_dir[3];
char wind_txt[16];
char description[24];
char loc[48];
char timezone[48];
char temp[24];
unsigned char icon;
char url[MAX_URL];

/*
  {"lat":33.1451,"lon":-97.088,"timezone":"America/Chicago","timezone_offset":-21600,"current":{"dt":1646341760,"sunrise":1646312048,"sunset":1646353604,"temp":25.42,"feels_like":24.7,"pressure":1018,"humidity":26,"dew_point":4.54,"uvi":3.66,"clouds":0,"visibility":10000,"wind_speed":5.14,"wind_deg":170,"weather":[{"id":800,"main":"Clear","description":"clear
  sky","icon":"01d"}]}}
 */

void weather_hpa_to_inhg(char *p)
{
    unsigned short w = atoi(p);

    w *= 3;
    itoa(w, p, 10);
    p[5] = p[4];
    p[4] = p[3];
    p[2] = '.';
}

void weather_date(char *c, unsigned long d, long offset)
{
    Timestamp ts;

    if (d != 0)
    {
        timestamp(d + offset, &ts);

        sprintf(c, "%u %s %u, %s", ts.day, time_month(ts.month), ts.year, time_dow(ts.dow));
    } else
        strcpy(c, "Unknown");
}

void weather_time(char *c, unsigned long d, long offset)
{
    Timestamp ts;

    if (d != 0)
    {
        timestamp(d + offset, &ts);

        sprintf(c, "%02u:%02u", ts.hour, ts.min);
    } else
        strcpy(c, "Unknown");
}
/*
weather_parse 
- Open the url containing the json data for the daily weather 

Returns
    true: Success
   false: Could not open the website, or could not find one of the json elements
 
*/

#ifdef USE_METEO


bool get_timezone_from_position(char *tm, size_t tm_size, char *latitude, char *longitude)
{
    unsigned char tmp[128];
    bool success = true;

    snprintf(url, sizeof(url), "N:HTTPS://" TMZ_API "//public/timezone?latitude=%s&longitude=%s", latitude, longitude);

    if (io_json_open(url))
    {
        return false;
    }

    if (io_json_query("/iana_timezone", tmp, sizeof(tmp)))
    {
        success = false;
        strncpy2(tm, "EST", tm_size);
    } else
        strncpy2(tm, tmp, tm_size);

    io_json_close();



    return success;
}

bool weather_parse(void) // METEO
{
    bool success = true;
    char cc = 'C';
    unsigned char res;
    char failure[2] = {"?"};
    unsigned char tmp[128];
    unsigned char units[10];
    char *p;


    get_timezone_from_position(timezone, sizeof(timezone), locData.latitude, locData.longitude);

    if (optData.units == IMPERIAL)
        strncpy2(tmp, "temperature_unit=fahrenheit&wind_speed_unit=mph&", sizeof(tmp));
    else
        strcpy(tmp, "");

    // F
    // https://api.open-meteo.com/v1/forecast?latitude=46.2618&longitude=-63.6726&current=temperature_2m,relative_humidity_2m,apparent_temperature,is_day,weather_code,cloud_cover,pressure_msl,wind_speed_10m,wind_direction_10m&hourly=dew_point_2m,visibility&daily=sunrise,sunset&temperature_unit=fahrenheit&wind_speed_unit=mph&precipitation_unit=inch&timeformat=unixtime&timezone=Pacific%2FAuckland&forecast_days=3&forecast_hours=1
    // C
    // https://api.open-meteo.com/v1/forecast?"
    // latitude=46.2618&
    // longitude=-63.6726&
    // current=temperature_2m,relative_humidity_2m,apparent_temperature,is_day,weather_code,cloud_cover,pressure_msl,wind_speed_10m,wind_direction_10m&
    // hourly=dew_point_2m,visibility&"
    // daily=sunrise,sunset&
    // timeformat=unixtime&
    // timezone=Pacific%2FAuckland&
    // forecast_days=3&forecast_hours=1

    snprintf(
        url, sizeof(url),
        "N:https://" ME_API "/v1//forecast?"
        "latitude=%s&"
        "longitude=%s&"
        "current=temperature_2m,relative_humidity_2m,apparent_temperature,weather_code,cloud_cover,pressure_msl,wind_speed_10m,wind_direction_10m&"
        "hourly=dew_point_2m,visibility&"
        "daily=sunrise,sunset&"
        "timeformat=unixtime&"
        "%s"
        "forecast_days=1&"
        "timezone=%s&"
        "forecast_days=1&"
        "forecast_hours=1",
        locData.latitude, locData.longitude, tmp, timezone);

    if (io_json_open(url))
    {
        return false;
    }

    /*
{
  "latitude": 46.26653,
  "longitude": -63.662872,
  "generationtime_ms": 0.0770092010498047,
  "utc_offset_seconds": 46800,
  "timezone": "Pacific/Auckland",
  "timezone_abbreviation": "NZDT",
  "elevation": 18,
  "current_units": {
    "time": "unixtime",
    "interval": "seconds",
    "temperature_2m": "°C",
    "relative_humidity_2m": "%",
    "apparent_temperature": "°C",
    "is_day": "",
    "weather_code": "wmo code",
    "cloud_cover": "%",
    "pressure_msl": "hPa",
    "wind_speed_10m": "km/h",
    "wind_direction_10m": "°"
  },
  "current": {
    "time": 1731206700,
    "interval": 900,
    "temperature_2m": 0.7,
    "relative_humidity_2m": 72,
    "apparent_temperature": -4.8,
    "is_day": 0,
    "weather_code": 3,
    "cloud_cover": 100,
    "pressure_msl": 1021.2,
    "wind_speed_10m": 19.6,
    "wind_direction_10m": 296
  },
  "hourly_units": {
    "time": "unixtime",
    "dew_point_2m": "°C",
    "visibility": "m"
  },
  "hourly": {
    "time": [1731204000],
    "dew_point_2m": [-3.2],
    "visibility": [21600]
  },
  "daily_units": {
    "time": "unixtime",
    "sunrise": "unixtime",
    "sunset": "unixtime"
  },
  "daily": {
    "time": [1731150000, 1731236400, 1731322800],
    "sunrise": [1731236984, 1731323470, 1731409956],
    "sunset": [1731271651, 1731357980, 1731444311]
  }
}
    */

    // Grab the relevant bits
    if (io_json_query("/current/time", tmp, sizeof(tmp)))
    {
        success = false;
        strncpy2(tmp, "0", sizeof(tmp));
    }
    dt = atol(tmp);

    // ellipsizeString( (char *) &tmp[0], &timezone[0], sizeof(timezone));

    if (io_json_query("/daily/sunrise/0", tmp, sizeof(tmp)))
    {
        success = false;
        strncpy2(tmp, "0", sizeof(tmp));
    }
    sunrise = atol(tmp);


    if (io_json_query("/daily/sunset/0", tmp, sizeof(tmp)))
    {
        success = false;
        strncpy2(tmp, "0", sizeof(tmp));
    }
    sunset = atol(tmp);

    if (io_json_query("/current/temperature_2m", tmp, sizeof(tmp)))
    {
        success = false;
        strncpy2(tmp, failure, sizeof(tmp));
    }
    io_decimals(tmp,optData.maxPrecision);   
    snprintf(temp, sizeof(temp), "%s*%c", tmp, optData.units == IMPERIAL ? 'F' : 'C');

    if (io_json_query("/current/apparent_temperature", tmp, sizeof(tmp)))
    {
        success = false;
        strncpy2(tmp, failure, sizeof(tmp));
    }
    io_decimals(tmp,optData.maxPrecision);
    snprintf(feels_like, sizeof(feels_like), "%s *%c", tmp, optData.units == IMPERIAL ? 'F' : 'C');

    if (io_json_query("/utc_offset_seconds", tmp, sizeof(tmp)))
    {
        success = false;
        strncpy2(tmp, "", sizeof(tmp));
    }

    timezone_offset = atol(tmp);

    if (io_json_query("/current_units/pressure_msl", tmp, sizeof(tmp)))
    {
        success = false;
        strncpy2(units, "hPa", sizeof(units));
    } else
        strncpy2(units, tmp, sizeof(units));

    if (io_json_query("/current/pressure_msl", tmp, sizeof(tmp)))
    {
        success = false;
        strncpy2(tmp, "", sizeof(tmp));
    }
    //if (optData.units == IMPERIAL)
    //    weather_hpa_to_inhg(tmp);
    snprintf(pressure, sizeof(pressure), "%s %s", tmp, units);


    if (io_json_query("/current/relative_humidity_2m", tmp, sizeof(tmp)))
    {
        success = false;
        strncpy2(tmp, failure, sizeof(tmp));
    }
    snprintf(humidity, sizeof(humidity), "%s%%", tmp);

    if (io_json_query("/hourly/dew_point_2m/0", tmp, sizeof(tmp)))
    {
        success = false;
        strncpy2(tmp, failure, sizeof(tmp));
    }
    snprintf(dew_point, sizeof(dew_point), "%s *%c", tmp, optData.units == IMPERIAL ? 'F' : 'C');

    if (io_json_query("/current/cloud_cover", tmp, sizeof(tmp)))
    {
        success = false;
        strncpy2(tmp, failure, sizeof(tmp));
    }
    snprintf(clouds, sizeof(clouds), "%s%%", tmp);


    if (io_json_query("/hourly/visibility/0", tmp, sizeof(tmp)))
    {
        success = false;
        strncpy2(tmp, "0", sizeof(tmp));
    }
    snprintf(visibility, sizeof(visibility), "%d %s", atoi(tmp) / 1000, optData.units == IMPERIAL ? "mi" : "km");

    if (io_json_query("/current/wind_speed_10m", tmp, sizeof(tmp)))
    {
        success = false;
        strncpy2(tmp, failure, sizeof(tmp));
    }
    snprintf(wind_speed, sizeof(wind_speed), "%s %s", tmp, optData.units == IMPERIAL ? "mph" : "kph");

    if (io_json_query("/current/wind_direction_10m", tmp, sizeof(tmp)))
    {
        success = false;
        strncpy2(tmp, failure, sizeof(tmp));
    }
    snprintf(wind_dir, sizeof(wind_dir), "%s", degToDirection(atoi(tmp)));

/*
    if (io_json_query("/current/weather/0/description", tmp, sizeof(tmp)))
    {
        success = false;
        strncpy2(tmp, failure, sizeof(tmp));
    }
    snprintf(description, sizeof(description), "%s", strupr(tmp));
*/

    if (io_json_query("/current/weather_code", tmp, sizeof(tmp)))
    {
        success = false;
        strncpy2(tmp, "1", sizeof(tmp));
    }
    get_description(tmp, description);

    icon = get_sprite(tmp);


    // Close connection
    io_json_close();

    return success;
}

#else

bool weather_parse(void) // OPEN_WEATHER
{
    bool success = true;
    char units[14];
    char cc = 'C';
    unsigned char res;
    char failure[2] = {"?"};
    unsigned char tmp[128];
    char url[256];
    char *p;

    strncpy2(units, "metric", sizeof(units));
    if (optData.units == IMPERIAL)
        strncpy2(units, "imperial", sizeof(units));


    // http://api.openweathermap.org/data/2.5/onecall?lat=44.62335968017578&lon=-63.57278060913086&exclude=minutely,hourly,alerts,daily&units=metric&appid=2e8616654c548c26bc1c86b1615ef7f1

    snprintf(url, sizeof(url),  "N:HTTP://%s//data/2.5/onecall?lat=%s&lon=%s&exclude=minutely,hourly,alerts,daily&units=%s&appid=%s", 
                                OW_API, locData.latitude, locData.longitude, units,
                                OW_KEY);
    if (io_json_open(url))
    {
        return false;
    }


    // Grab the relevant bits
    if (io_json_query("/current/dt", tmp, sizeof(tmp)))
    {
        success = false;
        strncpy2(tmp, "", sizeof(tmp));
    }
    dt = atol(tmp);


    if (io_json_query("/timezone", tmp, sizeof(tmp)))
    {
        success = false;
        strncpy2(tmp, "", sizeof(tmp));
    }
    ellipsizeString( (char *) &tmp[0], &timezone[0], sizeof(timezone));


    if (io_json_query("/current/sunrise", tmp, sizeof(tmp)))
    {
        success = false;
        strncpy2(tmp, "", sizeof(tmp));
    }
    sunrise = atol(tmp);


    if (io_json_query("/current/sunset", tmp, sizeof(tmp)))
    {
        success = false;
        strncpy2(tmp, "", sizeof(tmp));
    }
    sunset = atol(tmp);


    if (io_json_query("/current/temp", tmp, sizeof(tmp)))
    {
        success = false;
        strncpy2(tmp, failure, sizeof(tmp));
    }
    io_decimals(tmp,optData.maxPrecision);   
    snprintf(temp, sizeof(temp), "%s*%c", tmp, optData.units == IMPERIAL ? 'F' : 'C');


    if (io_json_query("/current/feels_like", tmp, sizeof(tmp)))
    {
        success = false;
        strncpy2(tmp, failure, sizeof(tmp));
    }
    io_decimals(tmp,optData.maxPrecision);
    snprintf(feels_like, sizeof(feels_like), "%s *%c", tmp, optData.units == IMPERIAL ? 'F' : 'C');


    if (io_json_query("/timezone_offset", tmp, sizeof(tmp)))
    {
        success = false;
        strncpy2(tmp, "", sizeof(tmp));
    }
    timezone_offset = atoi(tmp);


    if (io_json_query("/current/pressure", tmp, sizeof(tmp)))
    {
        success = false;
        strncpy2(tmp, "", sizeof(tmp));
    }
    if (optData.units == IMPERIAL)
        weather_hpa_to_inhg(tmp);
    snprintf(pressure, sizeof(pressure), "%s %s", tmp, optData.units == IMPERIAL ? "\"Hg" : "mPa");


    if (io_json_query("/current/humidity", tmp, sizeof(tmp)))
    {
        success = false;
        strncpy2(tmp, failure, sizeof(tmp));
    }
    snprintf(humidity, sizeof(humidity), "%s%%", tmp);


    if (io_json_query("/current/dew_point", tmp, sizeof(tmp)))
    {
        success = false;
        strncpy2(tmp, failure, sizeof(tmp));
    }
    snprintf(dew_point, sizeof(dew_point), "%s *%c", tmp, optData.units == IMPERIAL ? 'F' : 'C');


    if (io_json_query("/current/clouds", tmp, sizeof(tmp)))
    {
        success = false;
        strncpy2(tmp, failure, sizeof(tmp));
    }
    snprintf(clouds, sizeof(clouds), "%s%%", tmp);


    if (io_json_query("/current/visibility", tmp, sizeof(tmp)))
    {
        success = false;
        strncpy2(tmp, "0", sizeof(tmp));
    }
    snprintf(visibility, sizeof(visibility), "%d %s", atoi(tmp) / 1000, optData.units == IMPERIAL ? "mi" : "km");


    if (io_json_query("/current/wind_speed", tmp, sizeof(tmp)))
    {
        success = false;
        strncpy2(tmp, failure, sizeof(tmp));
    }
    snprintf(wind_speed, sizeof(wind_speed), "%s %s", tmp, optData.units == IMPERIAL ? "mph" : "kph");


    if (io_json_query("/current/wind_deg", tmp, sizeof(tmp)))
    {
        success = false;
        strncpy2(tmp, failure, sizeof(tmp));
    }
    snprintf(wind_dir, sizeof(wind_dir), "%s", degToDirection(atoi(tmp)));


    if (io_json_query("/current/weather/0/description", tmp, sizeof(tmp)))
    {
        success = false;
        strncpy2(tmp, failure, sizeof(tmp));
    }
    snprintf(description, sizeof(description), "%s", strupr(tmp));


    if (io_json_query("/current/weather/0/icon", tmp, sizeof(tmp)))
    {
        success = false;
        strncpy2(tmp, "01d", sizeof(tmp));
    }
    icon = get_sprite(tmp);


    // Close connection
    io_json_close();

    return success;
}

#endif


void weather(void)
{
bool dayNight;
unsigned char bg, fg;
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

        screen_weather_parsing();

        if (! weather_parse())
            screen_weather_could_not_get();

        weather_date(date_txt,         dt, timezone_offset);
        weather_time(time_txt,         dt, timezone_offset);
        weather_time(sunrise_txt, sunrise, timezone_offset);
        weather_time(sunset_txt,   sunset, timezone_offset);

        sprintf(wind_txt, "%s %s", wind_speed, wind_dir);

        sprintf(loc, "%s, %s %s", locData.city, locData.region_code, locData.country_code);

        screen_colors(dt, timezone_offset, &fg, &bg, &dayNight);

        screen_daily(date_txt, icon, temp, pressure, description, loc, wind_txt, feels_like, 
                     dew_point, visibility, timezone, sunrise_txt, sunset_txt, humidity, clouds, 
                     time_txt, fg, bg, dayNight, &future_time);
    }

    input_init();

    screen_weather_keys();

    timer = 65535;
    while (timer > 0)
    {
        if (input_weather())
        {
            if (forceRefresh)
                io_time(&future_time);
            
            break; // to allow input_init to be called again
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
