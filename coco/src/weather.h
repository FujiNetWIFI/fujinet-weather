#ifndef	WEATHER_H
#define WEATHER_H
#include <fujinet-fuji.h>
#include "weatherdefs.h"
void weather(WEATHER  *wi);
void forecast(FORECAST *fc, char p);
void decode_description(char code, char *buf);
byte * icon_code(char code);
char padding_center(char *s);
#endif
