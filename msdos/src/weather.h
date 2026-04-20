#ifndef	WEATHER_H
#define WEATHER_H

#include "weatherdefs.h"

void disp_weather(WEATHER *wi);
void disp_forecast(FORECAST *fc, char p);
void decode_description(char code, char *buf);
unsigned char icon_code(char code);

#endif
