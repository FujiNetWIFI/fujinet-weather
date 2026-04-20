#ifndef STRUTIL_H
#define STRUTIL_H

#include <string.h>
#include <ctype.h>

char *replaceSpaces(const char *str);
const char *dayOfWeek(int y, int m, int d);
const char *monthName(const char *date);
int parse_date(const char *date, int *year, int *month, int *day);

#endif
