#ifndef	OPENMETEO_H
#define OPENMETEO_H
#include <fujinet-fuji.h>
#include "weatherdefs.h"

// function
void get_location(LOCATION *loc);

// OpenMeteo 
void setup_omurl();
char *time_str(char *buf);
bool om_geocoding(LOCATION *loc, char *city);
void get_om_info(LOCATION *loc, WEATHER *wi, FORECAST *fc);
void set_weather(WEATHER *wi, int segment);
void set_forecast(FORECAST *fc, int segment);
#endif // OPENMETEO_H
