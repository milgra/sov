#ifndef zc_cstring_h
#define zc_cstring_h

#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>

char* cstr_new_format(int size, char* format, ...);
char* cstr_new_cstring(char* string);
char* cstr_append(char* str, char* add);
char* cstr_append_sub(char* str, char* add, int from, int len);
void  cstr_describe(void* p, int level);

#endif

#if __INCLUDE_LEVEL__ == 0

#include "zc_memory.c"
#include <ctype.h>
#include <string.h>

char* cstr_new_format(int size, char* format, ...)
{
  char*   result = CAL(sizeof(char) * size, NULL, cstr_describe);
  va_list args;

  va_start(args, format);
  vsnprintf(result, size, format, args);
  va_end(args);

  return result;
}

char* cstr_new_cstring(char* string)
{
  char* result = NULL;
  if (string != NULL)
  {
    result = CAL((strlen(string) + 1) * sizeof(char), NULL, cstr_describe);
    memcpy(result, string, strlen(string));
  }
  return result;
}

char* cstr_append(char* str, char* add)
{
  size_t needed = strlen(str) + strlen(add) + 1;

  if (strlen(str) < needed) str = mem_realloc(str, needed);
  strcat(str, add);

  return str;
}

char* cstr_append_sub(char* str, char* add, int from, int len)
{
  size_t needed  = strlen(str) + len + 1;
  int    oldsize = strlen(str);

  if (strlen(str) < needed) str = mem_realloc(str, needed);
  memcpy(str + oldsize, add + from, len);
  str[needed - 1] = '\0';

  return str;
}

void cstr_describe(void* p, int level)
{
  printf("%s", (char*)p);
}

#endif
