/**
 * Weather
 *
 * Constants
 */

#ifndef CONSTANTS_H
#define CONSTANTS_H

#define CLIENT_VERSION "1.1"

#define USE_METEO

#define MAX_URL      (512)
#define DEBUG_DELAY  (400)
#define MAX_APP_DATA (128)
#define MAX_QUERY    (128)

#define APPKEY_CREATOR_ID   0xb0c1
#define APPKEY_APP_ID       0x1
#define APPKEY_CONFIG_KEY   0xC0
#define APPKEY_LOCATION_KEY 0x10

#define DEFAULT_REFRESH 10

#ifdef USE_METEO

#define ME_API "api.open-meteo.com"
#define TMZ_API "api.geotimezone.com"

#else

#define OW_API "api.openweathermap.org"
#define OC_API "api.opencagedata.com"

#endif

#define IP_API "api.ipstack.com"
#define LIQ_API "locationiq.com"

#define NET_DEV  0x09
#define FUJI_DEV 0x0F
#define CHANNEL_MODE_JSON 0x01
#define ACK 0x80

#define READ_WRITE 12

#ifndef USE_METEO

#define OW_KEY "2e8616654c548c26bc1c86b1615ef7f1"

#endif


#define IP_KEY "d05a249425c32e0b231ba79e3f9af478"
#define LIQ_KEY "pk.e5e4ed6155f82872e7303a83744048a1"

#endif /* CONSTANTS_H */
