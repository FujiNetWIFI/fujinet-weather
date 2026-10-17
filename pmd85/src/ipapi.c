/*
 * # Weather application for FujiNet                     
 *
 */

#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include <conio.h>

#include "fujinet-network.h"
#include "jsonquery.h"
#include "weatherdefs.h"
#include "gfx.h"
#include "ipapi.h"


// for raw NET_SET_PARAMETER, EOL, 0 command
// extern uint8_t __CALLEE__  dwwrite(const uint8_t *buf, uint16_t count);
// extern void bus_ready(void);

extern int	err;

char ip_url[] = "N:http://ip-api.com/json/?fields=status,city,regionName,countryCode,lon,lat";

//
// get location info from ip
// returns true: call is success
//         false:     is not success
//
void get_location(LOCATION *loc)
{
	char buf[LINE_LEN];
	char message[LINE_LEN];

	// // raw NET_SET_PARAMETER, EOL command
	// static const unsigned char set_param_packet[] = {0xE3, 0x01, 0xFB, 0x01, 0x00};

	err = network_open(ip_url, OPEN_MODE_HTTP_GET_H, OPEN_TRANS_NONE);
	handle_err("ip-api open");

	// // set parameter: ask for '\0' terminator
	// bus_ready();
	// dwwrite(set_param_packet, sizeof(set_param_packet));

	err = network_json_parse(ip_url);
	handle_err("ip-api parse");

	JSON_QUERY(ip_url, "/status", buf, sizeof(buf));
	if (strcmp(buf, "success") != 0)
	{
		JSON_QUERY(ip_url, "/message", buf, sizeof(buf));
		network_close(ip_url);
		sprintf(message, "ip-api(%s)", buf);
		err = 0xff; // set unknown error
		handle_err(message);
	}

	JSON_QUERY(ip_url, "/city", loc->city, sizeof(loc->city));
	JSON_QUERY(ip_url, "/countryCode", loc->countryCode, sizeof(loc->countryCode));
	JSON_QUERY(ip_url, "/regionName", loc->state, sizeof(loc->state));
	JSON_QUERY(ip_url, "/lon", loc->lon, sizeof(loc->lon));
	JSON_QUERY(ip_url, "/lat", loc->lat, sizeof(loc->lat));

	network_close(ip_url);
}
