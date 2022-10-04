#ifndef cstr_util_h
#define cstr_util_h

#include "zc_cstring.c"
#include "zc_vector.c"
#include <ctype.h>
#include <string.h>

char*    cstr_new_file(char* path);
uint32_t cstr_color_from_cstring(char* string);
char*    cstr_new_readablec(uint32_t length);
char*    cstr_new_alphanumeric(uint32_t length);
void     cstr_tolower(char* str);
vec_t*   cstr_split(char* str, char* del);
char*    cstr_unescape(char* str);

#endif

#if __INCLUDE_LEVEL__ == 0

#include "zc_memory.c"

static char hexa[] =
    {
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // 0..9
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0, // 10..19
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0, // 20..29
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0, // 30..39
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	1, // 40..49
	2,
	3,
	4,
	5,
	6,
	7,
	8,
	9,
	0,
	0, // 50..59
	0,
	0,
	0,
	0,
	0,
	10,
	11,
	12,
	13,
	14, // 60..69
	15,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0, // 70..79
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0, // 80..89
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	10,
	11,
	12, // 90..99
	13,
	14,
	15,
	0,
	0,
	0,
	0,
	0,
	0,
	0 // 100..109
};

/* returns uint value based on digits */

uint32_t cstr_color_from_cstring(char* string)
{
    uint32_t result = 0;
    while (*string != 0)
	result = result << 4 | hexa[(int) *string++];
    return result;
}

/* reads up text file */

char* cstr_new_file(char* path)
{
    char* buffer = NULL;
    int   string_size, read_size;
    FILE* handler = fopen(path, "r");

    if (handler)
    {
	// Seek the last byte of the file
	fseek(handler, 0, SEEK_END);
	// Offset from the first to the last byte, or in other words, filesize
	string_size = ftell(handler);
	// go back to the start of the file
	rewind(handler);

	// Allocate a string that can hold it all
	buffer = (char*) CAL(sizeof(char) * (string_size + 1), NULL, cstr_describe);

	// Read it all in one operation
	read_size = fread(buffer, sizeof(char), string_size, handler);

	// fread doesn't set it so put a \0 in the last position
	// and buffer is now officially a string
	buffer[string_size] = '\0';

	if (string_size != read_size)
	{
	    // Something went wrong, throw away the memory and set
	    // the buffer to NULL
	    free(buffer);
	    buffer = NULL;
	}

	// Always remember to close the file.
	fclose(handler);
    }

    return buffer;
}

/* generates readable string */

char* vowels     = "aeiou";
char* consonants = "bcdefghijklmnpqrstvwxyz";

char* cstr_new_readablec(uint32_t length)
{
    char* result = CAL(sizeof(char) * (length + 1), NULL, cstr_describe);
    for (int index = 0; index < length; index += 2)
    {
	result[index] = consonants[rand() % strlen(consonants)];
	if (index + 1 < length) result[index + 1] = vowels[rand() % strlen(vowels)];
    }
    return result;
}

void cstr_tolower(char* str)
{
    for (int index = 0; index < strlen(str); index++)
    {
	str[index] = tolower(str[index]);
    }
}

/* generates alphanumeric string */

char* cstr_alphanumeric =
    "0123456789"
    "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
    "abcdefghijklmnopqrstuvwxyz";

char* cstr_new_alphanumeric(uint32_t length)
{
    char* result = CAL(sizeof(char) * (length + 1), NULL, cstr_describe);
    for (int index = 0; index < length; index++)
    {
	result[index] = cstr_alphanumeric[rand() % strlen(cstr_alphanumeric)];
    }
    return result;
}

vec_t* cstr_split(char* str, char* del)
{
    char*  token;
    vec_t* result = VNEW();

    token = strtok(str, del);

    while (token != NULL)
    {
	char* txt = CAL(strlen(token) + 1, NULL, cstr_describe);
	strcpy(txt, token);

	VADDR(result, txt);

	token = strtok(NULL, del);
    }

    return result;
}

char* cstr_unescape(char* str)
{
    size_t length = strlen(str);
    char*  result = CAL((length + 1) * sizeof(char), NULL, cstr_describe);
    int    ni     = 0;
    for (int index = 0; index < length; index++)
    {
	if (str[index] == '\\')
	{
	    if (index < length - 1)
	    {
		char n = str[index + 1];
		if (n == '\\' || n == '"' || n == '\'' || n == '/' || n == '?')
		{
		    result[ni++] = n;
		}
		index += 1;
	    }
	}
	else
	{
	    result[ni++] = str[index];
	}
    }
    return result;
}

#endif
