#include <cmoc.h>
#include <coco.h>
#include <fujinet-network.h>
#include "weatherdefs.h"

char tmp_buf[HALF_LEN * 2];

// Function to replace spaces with "%20" in a C-style string
// returns a pointer to a temporary buffer containing the modified string
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
    if (strlen((char *)date) < 10)
    {
        return -1; // Invalid date string
    }

    // Extract year, month, and day from "YYYY-MM-DD"
    *year = (date[0]-'0')*1000 + (date[1]-'0')*100
          + (date[2]-'0')*10   + (date[3]-'0');
    *month = (date[5]-'0')*10 + (date[6]-'0');
    *day = (date[8]-'0')*10 + (date[9]-'0');

    return 0; // Success
}

const char *dayOfWeek(int y, int m, int d)
{
    static const char *days[] = {
        "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"
    };
    
    static char result[4];

    // Adjust for Zeller’s congruence (months Jan/Feb are counted as 13/14 of previous year)
    if (m < 3) {
        m += 12;
        y--;
    }

    int K = y % 100;
    int J = y / 100;

    // Zeller’s formula
    int h = (d + (13*(m + 1))/5 + K + K/4 + J/4 + 5*J) % 7;

    // Convert Zeller’s output (0=Saturday) to 0=Sunday
    int dow = (h + 6) % 7;

    strcpy(result, days[dow]);
    return result;
}

const char *monthName(char *date)
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
        strcpy(result, "???");  // fallback for invalid month

    return result;
}
  