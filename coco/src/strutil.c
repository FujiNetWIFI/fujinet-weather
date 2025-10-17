#include <cmoc.h>
#include <coco.h>
#include <fujinet-network.h>

int isspace(int c)
{
    // Standard whitespace characters include:
    // space (0x20), form feed (0x0c), line feed (0x0a), carriage return (0x0d),
    // horizontal tab (0x09), vertical tab (0x0b)

    // It's important to cast 'c' to unsigned char before comparison to handle negative char values correctly,
    // unless 'c' is EOF.
    unsigned char uc = (unsigned char)c;

    return (uc == ' ' || uc == '\f' || uc == '\n' || uc == '\r' || uc == '\t' || uc == '\v');
}

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
// Assumes 'str' has sufficient allocated memory
void replaceSpaces(char *str)
{
    int trueLength = 0;

    if (str == NULL)
    {
        return;
    }

    trueLength = strlen(str);

    if (trueLength == 0)
    {
        return; 
    }   

    int spaceCount = 0;
    for (int i = 0; i < trueLength; i++)
    {
        if (str[i] == ' ')
        {
            spaceCount++;
        }
    }

    int newLength = trueLength + spaceCount * 2; // Each space becomes "%20" (3 chars, 2 extra)

    // Set null terminator at the new end
    str[newLength] = '\0';

    // Iterate from the end of the original string backwards
    // and copy characters to their new positions
    for (int i = trueLength - 1; i >= 0; i--)
    {
        if (str[i] == ' ')
        {
            str[newLength - 1] = '0';
            str[newLength - 2] = '2';
            str[newLength - 3] = '%';
            newLength -= 3;
        }
        else
        {
            str[newLength - 1] = str[i];
            newLength--;
        }
    }
}

int16_t network_json_query_trim(const char *devicespec, const char *query, char *s)
{
    int16_t ret = network_json_query(devicespec, query, s);
    truncate_at_first_non_printable(s);
    return ret;
}  