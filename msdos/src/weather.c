#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <time.h>

#include "weatherdefs.h"
#include "strutil.h"
#include "weather.h"
#include "gfx.h"

extern enum unit_option unit_opt;
extern char current_screen;

char current_forecast_page = 0;

char *temp_unit[] = {"\xF8""C", "\xF8""F"};
char *speed_unit[] = {" m/s", " mph"};
char *vis_unit[] = {" km", " mi"};
char *precip_unit[] = {" mm", " in"};

char *wind_deg_str[] = {" N", " NE", " E", " SE", " S", " SW", " W", " NW"};

static const char *day_names[] = {"Sun","Mon","Tue","Wed","Thu","Fri","Sat"};
static const char *mon_names[] = {"Jan","Feb","Mar","Apr","May","Jun",
                                  "Jul","Aug","Sep","Oct","Nov","Dec"};

static void draw_desc_wrapped(unsigned char x, unsigned char y, char code, unsigned char maxw)
{
	char desc[LINE_LEN];
	char line[14];
	char *p, *word, *lp;
	unsigned char llen, wlen, row;

	decode_description(code, desc);
	if (strncmp(desc, "Thunderstorm", 12) == 0)
	{
		if (desc[12] == 's')
		{
			memmove(desc + 8, desc + 13, strlen(desc + 13) + 1);
			memcpy(desc, "T-Storms", 8);
		}
		else
		{
			memmove(desc + 7, desc + 12, strlen(desc + 12) + 1);
			memcpy(desc, "T-Storm", 7);
		}
	}
	p = desc;
	row = 0;

	while (*p && row < 3)
	{
		lp = line;
		llen = 0;

		while (*p)
		{
			word = p;
			wlen = 0;
			while (*p && *p != ' ')
			{
				p++;
				wlen++;
			}

			if (llen == 0)
			{
				memcpy(lp, word, wlen);
				lp += wlen;
				llen = wlen;
			}
			else if (llen + 1 + wlen <= maxw)
			{
				*lp++ = ' ';
				memcpy(lp, word, wlen);
				lp += wlen;
				llen += 1 + wlen;
			}
			else
			{
				p = word;
				break;
			}

			if (*p == ' ')
				p++;
		}

		*lp = '\0';
		drawText(x, y + row, line);
		row++;
	}
}

static struct tm *local_tm(long ts, long tz)
{
	time_t t = (time_t)(ts + tz);
	return gmtime(&t);
}

void disp_weather(WEATHER *wi)
{
	char prbuf[LINE_LEN];
	struct tm *tm;
	int wind_idx;
	unsigned char tx;
	long visi;

	if (current_screen == SCREEN_WEATHER)
	{
		return;
	}

	current_screen = SCREEN_WEATHER;
	current_forecast_page = 0;

	setBgColor(0x09);
	resetScreen();
	draw_border();

	tm = local_tm(wi->datetime, wi->tz);
	sprintf(prbuf, "%s %s %d %02d:%02d %s",
		day_names[tm->tm_wday], mon_names[tm->tm_mon],
		tm->tm_mday, tm->tm_hour, tm->tm_min, wi->timezone);
	drawText(1 + (38 - strlen(prbuf)) / 2, 1, prbuf);

	draw_hdiv(2);

	drawIcon(1, 3, icon_code(wi->icon));

	sprintf(prbuf, "%s%s", wi->temp, temp_unit[unit_opt]);
	drawTextDouble(9, 3, prbuf);
	tx = 9 + strlen(prbuf) + 1;
	drawText(tx, 3, "Feels");
	drawText(tx, 4, "Like:");
	tx += 6;
	sprintf(prbuf, "%s%s", wi->feels_like, temp_unit[unit_opt]);
	drawTextDouble(tx, 3, prbuf);

	sprintf(prbuf, "%s hPa", wi->pressure);
	drawTextDouble(9, 5, prbuf);

	decode_description(wi->icon, prbuf);
	drawText(1, 8, prbuf);

	if (strlen(wi->state) > 0)
	{
		sprintf(prbuf, "%s, %s, %s", wi->name, wi->state, wi->country);
	}
	else
	{
		sprintf(prbuf, "%s, %s", wi->name, wi->country);
	}
	drawText(1, 9, prbuf);

	draw_hdiv(10);

	drawText(2, 11, "Humidity:");
	sprintf(prbuf, "%s %%", wi->humidity);
	drawText(12, 11, prbuf);

	sprintf(prbuf, "%s %s", wi->dew_point, temp_unit[unit_opt]);
	drawText(2, 12, "Dew Point:");
	drawText(13, 12, prbuf);

	drawText(2, 13, "Clouds:");
	sprintf(prbuf, "%s %%", wi->clouds);
	drawText(10, 13, prbuf);

	drawText(2, 14, "Visibility:");
	visi = atol(wi->visibility);
	if (unit_opt == METRIC)
	{
		visi = (visi + 500) / 1000;
	}
	else
	{
		visi = (visi + 2640) / 5280;
	}
	sprintf(prbuf, "%ld%s", visi, vis_unit[unit_opt]);
	drawText(14, 14, prbuf);

	wind_idx = (atoi(wi->wind_deg) % 360) / 45;
	drawText(2, 15, "Wind:");
	sprintf(prbuf, "%s%s%s", wi->wind_speed, speed_unit[unit_opt], wind_deg_str[wind_idx]);
	drawText(8, 15, prbuf);

	drawText(2, 16, "Precip:");
	sprintf(prbuf, "%s %%", wi->precip_prob);
	drawText(10, 16, prbuf);

	draw_hdiv(17);

	tm = local_tm(wi->sunrise, wi->tz);
	sprintf(prbuf, "%02d:%02d %s", tm->tm_hour, tm->tm_min, wi->timezone);
	drawText(2, 18, "Sunrise:");
	drawText(11, 18, prbuf);

	tm = local_tm(wi->sunset, wi->tz);
	sprintf(prbuf, "%02d:%02d %s", tm->tm_hour, tm->tm_min, wi->timezone);
	drawText(2, 19, "Sunset:");
	drawText(11, 19, prbuf);

	draw_hdiv(22);
}

void disp_forecast(FORECAST *fc, char p)
{
	char i;
	char start_idx;
	char prbuf[QUARTER_LEN];
	struct tm *tm;
	int wind_idx;
	unsigned char cx;
	extern WEATHER wi;

	if (p == current_forecast_page)
	{
		return;
	}

	current_screen = SCREEN_FORECAST;
	current_forecast_page = p;

	start_idx = (p - 1) * 4;
	setBgColor(0x09);
	resetScreen();
	draw_forecast_border();

	for (i = 0; i <= 3; i++)
	{
		if ((start_idx + i) > 7)
		{
			break;
		}

		cx = 1 + i * 10 + 1;

		tm = local_tm(fc->day[(int)(i + start_idx)].date, wi.tz);

		sprintf(prbuf, "%2d", tm->tm_mday);
		drawText(cx, 1, prbuf);

		drawText(cx, 2, mon_names[tm->tm_mon]);

		drawIcon(cx, 3, icon_code(fc->day[(int)(i + start_idx)].icon));

		drawText(cx, 7, day_names[tm->tm_wday]);

		draw_desc_wrapped(cx, 9, fc->day[(int)(i + start_idx)].icon, 8);

		sprintf(prbuf, "%s%s", fc->day[(int)(i+start_idx)].temp_max, temp_unit[unit_opt]);
		drawText(cx, 13, prbuf);

		sprintf(prbuf, "%s%s", fc->day[(int)(i+start_idx)].temp_min, temp_unit[unit_opt]);
		drawText(cx, 14, prbuf);

		sprintf(prbuf, "UV %s", fc->day[(int)(i+start_idx)].uv_index_max);
		drawText(cx, 15, prbuf);

		wind_idx = (atoi(fc->day[(int)(i+start_idx)].wind_deg) % 360) / 45;
		sprintf(prbuf, "W:%s", wind_deg_str[wind_idx]);
		drawText(cx, 17, prbuf);

		sprintf(prbuf, "%s%s", fc->day[(int)(i+start_idx)].wind_speed, speed_unit[unit_opt]);
		drawText(cx, 18, prbuf);

		sprintf(prbuf, "%s%s", fc->day[(int)(i+start_idx)].precipitation_sum, precip_unit[unit_opt]);
		drawText(cx, 20, prbuf);
	}
}

void decode_description(char code, char *buf)
{
	switch (code)
	{
		case 0:
			strcpy(buf, "Sunny");
			break;
		case 1:
			strcpy(buf, "Mainly sunny");
			break;
		case 2:
			strcpy(buf, "Partly cloudy");
			break;
		case 3:
			strcpy(buf, "Cloudy");
			break;
		case 45:
			strcpy(buf, "Foggy");
			break;
		case 48:
			strcpy(buf, "Rime fog");
			break;
		case 51:
			strcpy(buf, "Light drizzle");
			break;
		case 53:
			strcpy(buf, "Drizzle");
			break;
		case 55:
			strcpy(buf, "Heavy drizzle");
			break;
		case 56:
			strcpy(buf, "Light freezing drizzle");
			break;
		case 57:
			strcpy(buf, "Freezing drizzle");
			break;
		case 61:
			strcpy(buf, "Light rain");
			break;
		case 63:
			strcpy(buf, "Rain");
			break;
		case 65:
			strcpy(buf, "Heavy rain");
			break;
		case 66:
			strcpy(buf, "Light freezing rain");
			break;
		case 67:
			strcpy(buf, "Freezing rain");
			break;
		case 71:
			strcpy(buf, "Light snow");
			break;
		case 73:
			strcpy(buf, "Snow");
			break;
		case 75:
			strcpy(buf, "Heavy snow");
			break;
		case 77:
			strcpy(buf, "Snow grains");
			break;
		case 80:
			strcpy(buf, "Light showers");
			break;
		case 81:
			strcpy(buf, "Showers");
			break;
		case 82:
			strcpy(buf, "Heavy showers");
			break;
		case 85:
			strcpy(buf, "Light snow showers");
			break;
		case 86:
			strcpy(buf, "Snow showers");
			break;
		case 95:
			strcpy(buf, "Thunderstorm");
			break;
		case 96:
			strcpy(buf, "Thunderstorms w/ hail");
			break;
		case 99:
			strcpy(buf, "Thunderstorm w/ hail");
			break;
		default:
			strcpy(buf, "???");
	}
}

unsigned char icon_code(char code)
{
	switch (code)
	{
		case 0:  return 0;
		case 1:  return 1;
		case 2:  return 2;
		case 3:  return 3;
		case 51:
		case 53:
		case 55:
		case 56:
		case 57:
		case 80:
		case 81:
		case 82: return 4;
		case 61:
		case 63:
		case 65:
		case 66:
		case 67: return 5;
		case 95:
		case 96:
		case 99: return 6;
		case 71:
		case 73:
		case 75:
		case 77:
		case 85:
		case 86: return 7;
		case 45:
		case 48: return 8;
		default: return 1;
	}
}
