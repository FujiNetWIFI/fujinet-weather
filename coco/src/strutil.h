#ifndef STRUTIL_H
#define STRUTIL_H

#include <cmoc.h>
#include <coco.h>
#include <fujinet-network.h>

// Checks if the character is a whitespace character.
int isspace(int c);

// Checks if the character is printable.
int isprint(int c);

// Truncates the string at the first non-printable character.
void truncate_at_first_non_printable(char *str);

// Performs a network JSON query and trims the result at the first non-printable character.
int16_t network_json_query_trim(const char *devicespec, const char *query, char *s);

// Function to replace spaces with "%20" in a C-style string
// Assumes 'str' has sufficient allocated memory
void replaceSpaces(char *str);

#endif // STRUTIL_H