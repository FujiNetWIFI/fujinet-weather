#include "timeutil.h"

static const char *const wdays[] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
static const char *const months[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun",
                                     "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};

void to_ltime(long unix_utc, long offset, LTIME *t)
{
	long secs = unix_utc + offset;
	long days = secs / 86400L;
	long rem = secs % 86400L;
	long era, doe, yoe, doy, mp;

	if (rem < 0) {
		rem += 86400L;
		--days;
	}
	t->hour = (int)(rem / 3600);
	t->min = (int)(rem % 3600 / 60);
	/* 1970-01-01 was a Thursday. */
	t->wday = (int)((days % 7 + 11) % 7);

	/* Howard Hinnant's days_from_civil inverse. */
	days += 719468L;
	era = (days >= 0 ? days : days - 146096L) / 146097L;
	doe = days - era * 146097L;
	yoe = (doe - doe / 1460 + doe / 36524 - doe / 146096) / 365;
	doy = doe - (365 * yoe + yoe / 4 - yoe / 100);
	mp = (5 * doy + 2) / 153;
	t->mday = (int)(doy - (153 * mp + 2) / 5 + 1);
	t->month = (int)(mp < 10 ? mp + 3 : mp - 9);
	t->year = (int)(yoe + era * 400 + (t->month <= 2));
}

const char *wday_name(int wday)
{
	return wdays[(unsigned)wday % 7];
}

const char *month_name(int month)
{
	return months[(unsigned)(month - 1) % 12];
}
