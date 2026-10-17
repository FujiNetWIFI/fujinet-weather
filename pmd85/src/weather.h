#ifndef	WEATHER_H
#define WEATHER_H
#include <fujinet-fuji.h>
#include "weatherdefs.h"
void disp_weather(WEATHER  *wi);
void disp_forecast(FORECAST *fc, char p);
void decode_description(char code, char *buf);
unsigned char* get_weather_icon(char code, char is_day);
#endif
