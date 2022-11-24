#ifndef mt_string_h
#define mt_string_h

/* TODO separate unit tests */

#include "mt_vector.c"
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>

#define STRNC(x) mt_string_new_cstring(x)
#define STRNF(x, ...) mt_string_new_format(x, __VA_ARGS__)

char*        mt_string_new_format(int size, char* format, ...);
char*        mt_string_new_cstring(char* string);
char*        mt_string_append(char* str, char* add);
char*        mt_string_append_sub(char* str, char* add, int from, int len);
char*        mt_string_new_delete_utf_codepoints(char* str, int from, int len);
mt_vector_t* mt_string_tokenize(char* str, char* del);
void         mt_string_describe(void* p, int level);

#endif

#if __INCLUDE_LEVEL__ == 0

#include "mt_memory.c"
#include "utf8.h"
#include <ctype.h>
#include <string.h>

void mt_string_del(void* p)
{
    /* printf("DEL %s\n", (char*) p); */
}

char* mt_string_new_format(int size, char* format, ...)
{
    char*   result = CAL(sizeof(char) * size, mt_string_del, mt_string_describe);
    va_list args;

    va_start(args, format);
    vsnprintf(result, size, format, args);
    va_end(args);

    return result;
}

char* mt_string_new_cstring(char* string)
{
    char* result = NULL;
    if (string != NULL)
    {
	result = CAL((strlen(string) + 1) * sizeof(char), mt_string_del, mt_string_describe);
	memcpy(result, string, strlen(string));
    }
    return result;
}

char* mt_string_append(char* str, char* add)
{
    size_t needed = strlen(str) + strlen(add) + 1;

    if (strlen(str) < needed) str = mt_memory_realloc(str, needed);
    strcat(str, add);

    return str;
}

char* mt_string_append_sub(char* str, char* add, int from, int len)
{
    size_t needed  = strlen(str) + len + 1;
    int    oldsize = strlen(str);

    if (strlen(str) < needed) str = mt_memory_realloc(str, needed);
    memcpy(str + oldsize, add + from, len);
    str[needed - 1] = '\0';

    return str;
}

char* mt_string_new_delete_utf_codepoints(char* str, int from, int len)
{
    size_t       count = utf8len(str);
    const void*  part  = str;
    utf8_int32_t cp;
    char*        new_text = CAL(count, NULL, NULL);
    char*        new_part = new_text;

    // remove last codepoint
    for (int index = 0; index < count - 1; index++)
    {
	part = utf8codepoint(part, &cp);
	if (index < from || index > from + len) new_part = utf8catcodepoint(new_part, cp, 4);
    }

    return new_text;
}

mt_vector_t* mt_string_tokenize(char* str, char* del)
{
    char*        token;
    char*        copy   = mt_string_new_cstring(str);
    mt_vector_t* result = VNEW();

    token = strtok(copy, del);

    while (token != NULL)
    {
	char* txt = CAL(strlen(token) + 1, NULL, mt_string_describe);
	memcpy(txt, token, strlen(token));

	VADDR(result, txt);

	token = strtok(NULL, del);
    }

    REL(copy);

    return result;
}

void mt_string_describe(void* p, int level)
{
    printf("%s", (char*) p);
}

#endif
