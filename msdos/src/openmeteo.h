#ifndef	OPENMETEO_H
#define OPENMETEO_H

#include "weatherdefs.h"

void setup_omurl(LOCATION *loc, const char *param, bool isforecast, bool needtz);
bool om_geocoding(LOCATION *loc, char *city);
void get_om_info(LOCATION *loc, WEATHER *wi, FORECAST *fc);
void set_weather(WEATHER *wi, int segment);
void set_forecast(FORECAST *fc, int segment);

#endif
