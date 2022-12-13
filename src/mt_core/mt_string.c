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
char*        mt_string_reset(char* str);
char*        mt_string_append(char* str, char* add);
char*        mt_string_append_cp(char* str, uint32_t cp);
char*        mt_string_append_sub(char* str, char* add, int from, int len);
char*        mt_string_delete_utf_codepoints(char* str, int from, int len);
mt_vector_t* mt_string_tokenize(char* str, char* del);
void         mt_string_describe(void* p, int level);
void         mt_string_describe_utf(char* str);

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

char* mt_string_reset(char* str)
{
    str[0] = '\0';
    return str;
}

char* mt_string_append(char* str, char* add)
{
    size_t needed = strlen(str) + strlen(add) + 1;

    if (strlen(str) < needed) str = mt_memory_realloc(str, needed);

    str = utf8cat(str, add);

    return str;
}
char* mt_string_append_cp(char* str, uint32_t cp)
{
    size_t size   = utf8size(str);
    size_t cpsize = utf8codepointsize(cp);
    size_t needed = strlen(str) + size + 1;

    if (strlen(str) < needed) str = mt_memory_realloc(str, needed);

    char* end = str + size - 1;

    end    = utf8catcodepoint(end, cp, cpsize);
    end[0] = '\0';

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

char* mt_string_delete_utf_codepoints(char* str, int from, int len)
{
    char* res = STRNC("");

    char*        curr = str;
    utf8_int32_t cp;

    for (int index = 0; index < utf8len(str); index++)
    {
	curr = utf8codepoint(curr, &cp);

	if (index < from || index > from + len) res = mt_string_append_cp(res, cp);
    }

    strcpy(str, res);
    REL(res);

    return str;
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
    printf("STR: %s", (char*) p);
}

void mt_string_describe_utf(char* str)
{
    char*        part = str;
    utf8_int32_t cp;
    for (int i = 0; i < utf8len(str); i++)
    {
	part = utf8codepoint(part, &cp);
	printf("%.4i \t| ", cp);
    }
    printf("\n");
    part = str;
    for (int i = 0; i < utf8len(str); i++)
    {
	part = utf8codepoint(part, &cp);
	printf("%c \t| ", cp);
    }
    part = str;
    printf("\n");
    for (int i = 0; i < utf8len(str); i++)
    {
	part = utf8codepoint(part, &cp);
	printf("%zu \t| ", utf8codepointsize(cp));
    }
    printf("\n");
}

#endif
