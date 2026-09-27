#ifndef METEO_H
#define METEO_H

#include "weatherdefs.h"

/* Each returns 0 on success, else a message is available from net_error()
 * or meteo_error(). */
int locate_by_ip(LOCATION *loc);
int locate_by_name(LOCATION *loc, const char *name);
int fetch_weather(WEATHER *w);
const char *meteo_error(void);

/* Display helpers shared by every screen. */
const char *wmo_text(int code);
const char *wind_dir(const char *degrees);
const char *temp_unit(UNITOPT u);
const char *speed_unit(UNITOPT u);
const char *precip_unit(UNITOPT u);

#endif
