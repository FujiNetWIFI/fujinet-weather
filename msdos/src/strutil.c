#include <string.h>
#include <stdlib.h>
#include "weatherdefs.h"

char tmp_buf[HALF_LEN * 2];

char *replaceSpaces(const char *str)
{
    char *dst = tmp_buf;

    if (!str) return NULL;

    while (*str)
    {
        if (*str == ' ')
        {
            *dst++ = '%';
            *dst++ = '2';
            *dst++ = '0';
        }
        else
        {
            *dst++ = *str;
        }
        str++;
    }
    *dst = '\0';
    return tmp_buf;
}

int parse_date(const char *date, int *year, int *month, int *day)
{
    if (strlen(date) < 10)
    {
        return -1;
    }

    *year = (date[0]-'0')*1000 + (date[1]-'0')*100
          + (date[2]-'0')*10   + (date[3]-'0');
    *month = (date[5]-'0')*10 + (date[6]-'0');
    *day = (date[8]-'0')*10 + (date[9]-'0');

    return 0;
}

const char *dayOfWeek(int y, int m, int d)
{
    static const char *days[] = {
        "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"
    };

    static char result[4];
    int K, J, h, dow;

    if (m < 3) {
        m += 12;
        y--;
    }

    K = y % 100;
    J = y / 100;

    h = (d + (13*(m + 1))/5 + K + K/4 + J/4 + 5*J) % 7;
    dow = (h + 6) % 7;

    strcpy(result, days[dow]);
    return result;
}

const char *monthName(const char *date)
{
    static const char *months[] = {
        "Jan", "Feb", "Mar", "Apr", "May", "Jun",
        "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
    };
    static char result[4];

    int m = (date[5]-'0')*10 + (date[6]-'0');

    if (m >= 1 && m <= 12)
        strcpy(result, months[m - 1]);
    else
        strcpy(result, "???");

    return result;
}
