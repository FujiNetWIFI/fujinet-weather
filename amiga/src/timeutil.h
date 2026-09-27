#ifndef TIMEUTIL_H
#define TIMEUTIL_H

/* Broken-down local time from unix seconds plus a UTC offset.  Integer-only
 * civil-date arithmetic, so no dependency on the runtime's gmtime/TZ. */
typedef struct {
	int year, month, mday;  /* month 1..12 */
	int wday;               /* 0 = Sunday */
	int hour, min;
} LTIME;

void to_ltime(long unix_utc, long offset, LTIME *t);
const char *wday_name(int wday);     /* "Sun".."Sat" */
const char *month_name(int month);   /* "Jan".."Dec" */

#endif
