#ifndef STRUTIL_H
#define STRUTIL_H

// Checks if the character is printable.
#undef isprint
#define isprint(c) (c>=0x20 && c<=0x8E)

// Function to replace spaces with "%20" in a C-style string
// returns a pointer to a temporary buffer containing the modified string
char * replaceSpaces(char *str);

// Parses a date string in "YYYY-MM-DD" format into year, month, and day integers.
int parse_date(const char *date, int *year, char *month, char *day);

// Returns the day of the week for a given year, month, day integers.
// (Returned from parse_date function)
const char *dayOfWeek(int y, char m, char d);

// Returns the month name for a given date string month (returend from parse_date).
const char *monthName(char m);
// // Returns the month name for a given date string in "YYYY-MM-DD" format.
// const char *monthName(const char *date);

#endif // STRUTIL_H