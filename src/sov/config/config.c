#ifndef config_h
#define config_h

#include "zc_map.c"

void  config_init();
void  config_destroy();
void  config_describe();
int   config_read(char* path);
void  config_set(char* key, char* value);
char* config_get(char* key);
int   config_get_int(char* key);
int   config_get_bool(char* key);

#endif

#if __INCLUDE_LEVEL__ == 0

#include "kvlines.c"
#include "zc_cstring.c"
#include <limits.h>

map_t* confmap;

void config_init()
{
  confmap = MNEW(); // REL 0

  MPUTR(confmap, "meta_code", cstr_new_cstring("133"));
  /* secondary_code 23 */
  MPUTR(confmap, "gap", cstr_new_cstring("25"));
  MPUTR(confmap, "columns", cstr_new_cstring("5"));
  MPUTR(confmap, "ratio", cstr_new_cstring("8"));
  MPUTR(confmap, "font_face", cstr_new_cstring("Terminus (TTF):style=Bold"));
  MPUTR(confmap, "text_margin_size", cstr_new_cstring("7"));
  MPUTR(confmap, "text_margin_top_size", cstr_new_cstring("4"));
  MPUTR(confmap, "text_title_size", cstr_new_cstring("14"));
  MPUTR(confmap, "text_title_color", cstr_new_cstring("#FFFFFFFF"));
  MPUTR(confmap, "text_description_size", cstr_new_cstring("12"));
  MPUTR(confmap, "text_description_color", cstr_new_cstring("#AADDFFFF"));
  MPUTR(confmap, "text_workspace_size", cstr_new_cstring("20"));
  MPUTR(confmap, "text_workspace_color", cstr_new_cstring("#FFFFFFFF"));
  MPUTR(confmap, "text_workspace_xshift", cstr_new_cstring("11"));
  MPUTR(confmap, "text_workspace_yshift", cstr_new_cstring("-17"));
  MPUTR(confmap, "border_color", cstr_new_cstring("#AADDFF6FF"));
  MPUTR(confmap, "background_color", cstr_new_cstring("#000022FF"));
  MPUTR(confmap, "background_color_focused", cstr_new_cstring("#222266FF"));
  MPUTR(confmap, "background_corner_radius", cstr_new_cstring("20"));
  MPUTR(confmap, "tree_corner_radius", cstr_new_cstring("7"));
}

void config_destroy()
{
  REL(confmap); // REL 0
}

int config_read(char* path)
{
  return kvlines_read(path, confmap);
}

void config_set(char* key, char* value)
{
  MPUT(confmap, key, value);
}

char* config_get(char* key)
{
  return MGET(confmap, key);
}

int config_get_bool(char* key)
{
  char* val = MGET(confmap, key);
  if (val && strcmp(val, "true") == 0)
    return 1;
  else
    return 0;
}

int config_get_int(char* key)
{
  char* val = MGET(confmap, key);
  if (val)
    return atoi(val);
  else
    return INT_MAX;
}

void config_describe()
{
  mem_describe(confmap, 0);
  printf("\n");
}

#endif
