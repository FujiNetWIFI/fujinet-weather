/**
 * Weather
 *
 * Based on @bocianu's code
 *
 * @author Thomas Cherryhomes
 * @email thom dot cherryhomes at gmail dot com
 *
 */

#ifndef WEATHER_H
#define WEATHER_H

#include <stdbool.h>

void weather(void);
void weather_hpa_to_inhg(char *p);
extern char timezone[48];
bool get_timezone_from_position(char *tm, size_t tm_size, char *latitude, char *longitude);

#endif /* WEATHER_H */
