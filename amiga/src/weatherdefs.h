/*
 * fnweather for the Amiga (Workbench 1.3+) over FujiNet NIO.
 *
 * Values are kept as the strings Open-Meteo returned: the program never
 * needs arithmetic on them, and avoiding floats keeps it small and free of
 * the math libraries a stock 1.3 system may lack.
 */
#ifndef WEATHERDEFS_H
#define WEATHERDEFS_H

#define VAL_LEN   12
#define NAME_LEN  40
#define DAYS      8

typedef enum { METRIC = 0, IMPERIAL = 1 } UNITOPT;

typedef struct {
	char lat[VAL_LEN + 4];
	char lon[VAL_LEN + 4];
	char city[NAME_LEN];
	char country[VAL_LEN];
} LOCATION;

typedef struct {
	long time;              /* unix time, UTC */
	long utc_offset;        /* seconds */
	char tz_abbr[VAL_LEN];
	int  code;              /* WMO weather code */
	char temp[VAL_LEN];
	char feels_like[VAL_LEN];
	char humidity[VAL_LEN];
	char dew_point[VAL_LEN];
	char pressure[VAL_LEN];
	char clouds[VAL_LEN];
	char wind_speed[VAL_LEN];
	char wind_deg[VAL_LEN];
} CURRENT;

typedef struct {
	long time;
	long sunrise;
	long sunset;
	int  code;
	char temp_max[VAL_LEN];
	char temp_min[VAL_LEN];
	char precip[VAL_LEN];
	char uv_max[VAL_LEN];
	char wind_speed[VAL_LEN];
	char wind_deg[VAL_LEN];
} DAY;

typedef struct {
	LOCATION loc;
	CURRENT  now;
	DAY      day[DAYS];
	UNITOPT  units;
} WEATHER;

#endif
