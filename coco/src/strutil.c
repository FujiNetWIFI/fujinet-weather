#include <cmoc.h>
#include <coco.h>
#include <fujinet-network.h>
#include "weatherdefs.h"

char tmp_buf[LINE_LEN * 2];

int isprint(int c)
{
    // Check if c falls within the ASCII range for printable characters (0x20 to 0x7E)
    // This range includes space (0x20) up to tilde (0x7E).
    if (c >= 0x20 && c <= 0x7E)
    {
        return 1; // It is a printable character
    }
    else
    {
        return 0; // It is not a printable character
    }
}

void truncate_at_first_non_printable(char *str)
{
    if (str == NULL)
    {
        return; // Handle null pointer case
    }

    int i = 0;
    while (str[i] != '\0')
    {
        if (!isprint((unsigned char)str[i]))
        {
            str[i] = '\0'; // Truncate the string
            return;
        }
        i++;
    }
}

// Function to replace spaces with "%20" in a C-style string
// returns a pointer to a temporary buffer containing the modified string
char * replaceSpaces(char *str)
{
    int trueLength = 0;
    memset(tmp_buf, 0, sizeof(tmp_buf));

    if (str == NULL)
    {
        return NULL; 
    }

    trueLength = strlen(str);

    if (trueLength == 0)
    {
        return tmp_buf ; 
    }   

    strncpy(tmp_buf, str, trueLength);

    int spaceCount = 0;
    for (int i = 0; i < trueLength; i++)
    {
        if (tmp_buf[i] == ' ')
        {
            spaceCount++;
        }
    }

    int newLength = trueLength + spaceCount * 2; // Each space becomes "%20" (3 chars, 2 extra)

    // Set null terminator at the new end
    tmp_buf[newLength] = '\0';

    // Iterate from the end of the original string backwards
    // and copy characters to their new positions
    for (int i = trueLength - 1; i >= 0; i--)
    {
        if (str[i] == ' ')
        {
            tmp_buf[newLength - 1] = '0';
            tmp_buf[newLength - 2] = '2';
            tmp_buf[newLength - 3] = '%';
            newLength -= 3;
        }
        else
        {
            tmp_buf[newLength - 1] = str[i];
            newLength--;
        }
    }
    
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

const char *dayOfWeek(char *date)
{
    static const char *days[] = {
        "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"
    };
    static char result[4];

    int y, m, d;

    // Parse "YYYY-MM-DD"
    parse_date(date, &y, &m, &d);

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

int16_t network_json_query_trim(const char *devicespec, const char *query, char *s)
{
    int16_t ret = network_json_query(devicespec, query, s);
    truncate_at_first_non_printable(s);
    return ret;
}  