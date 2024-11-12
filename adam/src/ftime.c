/**
 * timestamp function
 *
 * Based on @bocianu's code
 *
 * @author Thomas Cherryhomes
 * @email thom dot cherryhomes at gmail dot com
 *
 */

#include <stdbool.h>
#include "ftime.h"
#include <time.h>

const unsigned char daysInMonth[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

const char *monthNames[] = {"JAN", "FEB", "MAR", "APR", "MAY", "JUN", "JUL", "AUG", "SEP", "OCT", "NOV", "DEC"};

const char *dowNames[] = {"SUN", "MON", "TUE", "WED", "THU", "FRI", "SAT"};

/*
void timestamp(unsigned long t, Timestamp *ts)
{
    bool leap;
    unsigned short daysInYear;
    bool done = false;
    unsigned long s = t;
    unsigned long m = t / 60;
    unsigned long h;
    unsigned long d;
    unsigned short y = 1970;
    unsigned char dow = 4;
    unsigned char dim = 0;

    h = m / 60;
    m = m - h * 60;
    d = h / 24;
    h = h - d * 24;

    do
    {
        leap = ((y & 3) == 0 && (y % 100 != 0 || y % 400 == 0));
        daysInYear = 365;
        if (leap)
            daysInYear++;
        if (d >= daysInYear)
        {
            dow++;
            if (leap)
                dow++;
            d -= daysInYear;
            if (dow >= 7)
                dow -= 7;
            y++;
        }
        else
        {
            ts->day = d;
            dow += d;
            dow %= 7;
            m = 0;
            do
            {
                dim = daysInMonth[m];
                if (m == 1 && leap)
                    dim++;
                if (d >= dim)
                    d -= dim;
                else
                    done = true;
                m++;
            } while (!(done || m == 12));
            done = true;
        }
    } while (!done);

    ts->sec = s;
    ts->min = m;
    ts->hour = h;
    ts->day = d + 1;
    ts->month = m;
    ts->year = y;
    ts->dow = dow;
}
*/

void timestamp(unsigned long t, Timestamp *ts)
{
    struct tm *tm2;
    
    tm2 = gmtime(&t);

    ts->sec   = tm2->tm_sec;
    ts->min   = tm2->tm_min;
    ts->hour  = tm2->tm_hour;
    ts->day   = tm2->tm_mday;
    ts->month = tm2->tm_mon+1;
    ts->year  = tm2->tm_year+1900;
    ts->dow   = tm2->tm_wday;

}
const char *time_month(unsigned char month) { return monthNames[month - 1]; }

const char *time_dow(unsigned char dow) { return dowNames[dow]; }
