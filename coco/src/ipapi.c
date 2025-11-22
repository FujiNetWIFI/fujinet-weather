/*
 * # Weather application for FujiNet                     
 *
 */

#include <cmoc.h>	
#include <coco.h>

#include "fujinet-network.h"
#include "weatherdefs.h"
#include "ipapi.h"
#include "gfx.h"
#include "strutil.h"

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

	err = network_open(ip_url, OPEN_MODE_READ, OPEN_TRANS_NONE);
	handle_err("ip-api open");

	err = network_json_parse(ip_url);
	handle_err("ip-api parse");

	memset(buf, 0, LINE_LEN);

	network_json_query(ip_url, "/status", buf);
	if (strcmp(buf, "success") != 0)
	{
		memset(buf, 0, LINE_LEN);
		network_json_query(ip_url, "/message", buf);
		network_close(ip_url);
		sprintf(message, "ip-api(%s)", buf);
		err = 0xff; // set unknown error
		handle_err(message);
	}

	network_json_query(ip_url, "/city", loc->city);
	network_json_query(ip_url, "/countryCode", loc->countryCode);
	network_json_query(ip_url, "/lon", loc->lon);
	network_json_query(ip_url, "/regionName", loc->state);
	network_json_query(ip_url, "/lat", loc->lat);

	network_close(ip_url);
}
