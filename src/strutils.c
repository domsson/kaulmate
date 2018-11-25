#include <stdlib.h>	// NULL, malloc()
#include <string.h>	// strlen(), strcmp()
#include "strutils.h"

/*
 * Returns 1 if both input strings are equal, otherwise 0.
 */
int str_equals(const char *str1, const char *str2)
{
	return strcmp(str1, str2) == 0;
}

/*
 * Returns 1 if the input string is quoted, otherwise 0.
 */
int str_quoted(const char *str)
{
	size_t len = strlen(str); // Length without null terminator
	if (len < 2) return 0;    // We need at least two quotes (empty string)
	char first = str[0];
	char last  = str[len - 1];
	if (first == '\'' && last == '\'') return 1; // Single-quoted string
	if (first == '"' && last == '"') return 1;   // Double-quoted string
	return 0;
}

/*
 * Returns a pointer to a string that is the same as the input string, 
 * minus the enclosing quotation chars (either single or double quotes).
 * The pointer is allocated with malloc(), the caller needs to free it.
 */
char *str_unquote(const char *str)
{
	char *trimmed = NULL;
	size_t len = strlen(str);
	if (len < 2) // Prevent zero-length allocation
	{
		trimmed = malloc(1); // Make space for null terminator
		trimmed[0] = '\0';   // Add the null terminator
	}
	else
	{
		trimmed = malloc(len-2+1);        // No quotes, null terminator
		strncpy(trimmed, &str[1], len-2); // Copy everything in between
		trimmed[len-2] = '\0';            // Add the null terminator
	}
	return trimmed;
}
