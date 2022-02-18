#ifndef zc_string_h
#define zc_string_h

#include "zc_map.c"
#include "zc_vector.c"
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

typedef struct _str_t str_t;
struct _str_t
{
  uint32_t  length;       // current length of codepoint array
  uint32_t  length_real;  // backing length of codepoint array
  uint32_t  length_bytes; // needed length of byte array for all codepoints
  uint32_t* codepoints;
};

str_t* str_new(void);
void   str_reset(str_t* string);
void   str_describe(void* p, int level);

str_t* str_new_substring(str_t* string, int start, int end);
char*  str_new_cstring(str_t* string);

void str_add_string(str_t* stra, str_t* strb);
void str_add_bytearray(str_t* string, char* bytearray);
void str_add_codepoint(str_t* string, uint32_t codepoint);

void str_remove_codepoint_at_index(str_t* string, uint32_t index);
void str_remove_codepoints_in_range(str_t* string, uint32_t start, uint32_t end);

int8_t str_compare(str_t* stra, str_t* strb);

#endif
#if __INCLUDE_LEVEL__ == 0

#include "zc_memory.c"
#include <string.h>

#define UTF8_BOM "\xEF\xBB\xBF"

void str_del(void* pointer)
{
  str_t* string = pointer;
  REL(string->codepoints);
}

void str_describe_codepoints(void* p, int level)
{
  printf("str codepoints\n");
}

str_t* str_new()
{
  str_t* string = CAL(sizeof(str_t), str_del, str_describe);

  string->length       = 0;  // current length of codepoint array
  string->length_real  = 10; // backing length of codepoint array
  string->length_bytes = 0;  // needed length of byte array for all codepoints
  string->codepoints   = CAL(string->length_real * sizeof(uint32_t), NULL, NULL);

  return string;
}

void str_reset(str_t* string)
{
  string->length       = 0;
  string->length_real  = 10;
  string->length_bytes = 0;
  string->codepoints   = mem_realloc(string->codepoints, string->length_real * sizeof(uint32_t));
  memset(string->codepoints, 0, string->length_real * sizeof(uint32_t));
}

void str_describe(void* p, int level)
{
  str_t* str = p;
  printf("length %u", str->length);
}

str_t* str_new_substring(str_t* string, int start, int end)
{
  str_t* result = str_new();

  for (int index = start; index < end; index++)
  {
    str_add_codepoint(result, string->codepoints[index]);
  }

  return result;
}

void str_desc_cstr(void* p, int level)
{
  printf("%s", (char*)p);
}

char* str_new_cstring(str_t* string)
{
  if (string == NULL) return NULL;
  char*    bytes    = CAL((string->length_bytes + 1) * sizeof(char), NULL, str_desc_cstr);
  uint32_t position = 0;
  for (int index = 0; index < string->length; index++)
  {
    uint32_t codepoint = string->codepoints[index];
    if (codepoint < 0x80)
    {
      bytes[position++] = codepoint;
    }
    else if (codepoint < 0x800)
    {
      bytes[position++] = (codepoint >> 6) | 0xC0;
      bytes[position++] = (codepoint & 0x3F) | 0x80;
    }
    else if (codepoint < 0x1000)
    {
      bytes[position++] = (codepoint >> 12) | 0xE0;
      bytes[position++] = ((codepoint >> 6) & 0x3F) | 0x80;
      bytes[position++] = (codepoint & 0x3F) | 0x80;
    }
    else
    {
      bytes[position++] = (codepoint >> 18) | 0xF0;
      bytes[position++] = ((codepoint >> 12) & 0x3F) | 0x80;
      bytes[position++] = ((codepoint >> 6) & 0x3F) | 0x80;
      bytes[position++] = (codepoint & 0x3F) | 0x80;
    }
  }
  return bytes;
}

void str_add_string(str_t* stra, str_t* strb)
{
  if (strb != NULL)
  {
    uint32_t newlength       = stra->length + strb->length;
    uint32_t newlength_real  = stra->length_real + strb->length_real;
    uint32_t newlength_bytes = stra->length_bytes + strb->length_bytes;

    stra->codepoints = mem_realloc(stra->codepoints, sizeof(uint32_t) * newlength_real);
    memcpy((void*)(stra->codepoints + stra->length), (void*)strb->codepoints, strb->length * sizeof(uint32_t));

    stra->length       = newlength;
    stra->length_real  = newlength_real;
    stra->length_bytes = newlength_bytes;
  }
}

uint8_t str_get_code_bytelength(uint32_t codepoint)
{
  uint8_t codelength = 4;
  if (codepoint < 0x80)
    codelength = 1;
  else if (codepoint < 0x800)
    codelength = 2;
  else if (codepoint < 0x1000)
    codelength = 3;
  return codelength;
}

void str_add_buffer(str_t* string, char* buffer, char length)
{
  // filter byte order mark
  if (strcmp(buffer, UTF8_BOM) != 0)
  {
    uint32_t codepoint = 0;
    // extract codepoint
    if (length == 1)
      codepoint = buffer[0] & 0x7F;
    else if (length == 2)
      codepoint = (buffer[0] & 0x1F) << 6 | (buffer[1] & 0x3F);
    else if (length == 3)
      codepoint = (buffer[0] & 0xF) << 12 | (buffer[1] & 0x3F) << 6 | (buffer[2] & 0x3F);
    else if (length == 4)
      codepoint = (buffer[0] & 0x7) << 18 | (buffer[1] & 0x3F) << 12 | (buffer[2] & 0x3F) << 6 | (buffer[3] & 0x3F);
    // add codepoint
    str_add_codepoint(string, codepoint);
  }
}

void str_add_bytearray(str_t* string, char* bytearray)
{
  assert(bytearray != NULL);

  char buffer[4]       = {0};
  char buffer_position = 0;
  while (*bytearray != 0)
  {
    // checking unicode closing characters or last character
    if ((*bytearray & 0xFF) >> 7 == 0 || (*bytearray & 0xFF) >> 6 == 3)
    {
      if (buffer_position > 0)
      {
        str_add_buffer(string, buffer, buffer_position);
        // reset unicode buffer
        memset(&buffer, 0, 4);
        buffer_position = 0;
      }
    }
    // storing actual byte in unicode codepoint buffer
    buffer[buffer_position++] = *bytearray;
    // step in byte array
    bytearray += 1;
    // invalid utf sequence, aborting
    if (buffer_position == 5) return;
  }
  // add remaining buffer content
  if (buffer_position > 0) str_add_buffer(string, buffer, buffer_position);
}

void str_add_codepoint(str_t* string, uint32_t codepoint)
{
  uint8_t codelength = str_get_code_bytelength(codepoint);

  // expand
  if (string->length_real == string->length)
  {
    string->codepoints = mem_realloc(string->codepoints, sizeof(uint32_t) * (string->length_real + 10));
    string->length_real += 10;
  }

  string->codepoints[string->length] = codepoint;
  string->length += 1;
  string->length_bytes += codelength;
}

void str_remove_codepoint_at_index(str_t* string, uint32_t index)
{
  uint32_t codepoint  = string->codepoints[index];
  uint8_t  codelength = str_get_code_bytelength(codepoint);

  string->length_bytes -= codelength;
  memmove(string->codepoints + index, string->codepoints + index + 1, (string->length - index - 1) * sizeof(uint32_t));
  string->length -= 1;
}

void str_remove_codepoints_in_range(str_t* string, uint32_t start, uint32_t end)
{
  if (end > string->length) end = string->length;

  for (int index = start; index < end; index++)
  {
    uint32_t codepoint  = string->codepoints[index];
    uint8_t  codelength = str_get_code_bytelength(codepoint);
    string->length_bytes -= codelength;
  }

  if (end < string->length)
  {
    memmove(
        string->codepoints + start,
        string->codepoints + end + 1,
        (string->length - end - 1) * sizeof(uint32_t));
  }

  string->length -= end - start + 1;
}

int8_t str_compare(str_t* stra, str_t* strb)
{
  char* bytes_a = str_new_cstring(stra);
  char* bytes_b = str_new_cstring(strb);

  int8_t result = strcmp(bytes_a, bytes_b);

  REL(bytes_a);
  REL(bytes_b);

  return result;
}

#endif
