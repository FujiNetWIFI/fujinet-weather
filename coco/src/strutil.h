#ifndef STRUTIL_H
#define STRUTIL_H

#include <cmoc.h>
#include <coco.h>
#include <fujinet-network.h>

// Checks if the character is printable.
int isprint(int c);

// Truncates the string at the first non-printable character.
void truncate_at_first_non_printable(char *str);

// Performs a network JSON query and trims the result at the first non-printable character.
int16_t network_json_query_trim(const char *devicespec, const char *query, char *s);

// Function to replace spaces with "%20" in a C-style string
// returns a pointer to a temporary buffer containing the modified string
char * replaceSpaces(char *str);

// Returns the day of the week for a given date string in "YYYY-MM-DD" format.
const char *dayOfWeek(char *date);

// Returns the month name for a given date string in "YYYY-MM-DD" format.
const char *monthName(const char *date);

// Parses a date string in "YYYY-MM-DD" format into year, month, and day integers.
int parse_date(const char *date, int *year, int *month, int *day);

#endif // STRUTIL_H