#ifndef config_h
#define config_h

#include "mt_map.c"

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
#include "mt_string.c"
#include <limits.h>

mt_map_t* confmap;

void config_init()
{
    confmap = MNEW(); // REL 0

    MPUTR(confmap, "meta_code", STRNC("133"));
    /* secondary_code 23 */
    MPUTR(confmap, "gap", STRNC("25"));
    MPUTR(confmap, "columns", STRNC("5"));
    MPUTR(confmap, "ratio", STRNC("8"));
    MPUTR(confmap, "font_face", STRNC("Terminus (TTF):style=Bold"));
    MPUTR(confmap, "text_margin_size", STRNC("7"));
    MPUTR(confmap, "text_margin_top_size", STRNC("4"));
    MPUTR(confmap, "text_title_size", STRNC("14"));
    MPUTR(confmap, "text_title_color", STRNC("#FFFFFFFF"));
    MPUTR(confmap, "text_description_size", STRNC("12"));
    MPUTR(confmap, "text_description_color", STRNC("#AADDFFFF"));
    MPUTR(confmap, "text_workspace_size", STRNC("20"));
    MPUTR(confmap, "text_workspace_color", STRNC("#FFFFFFFF"));
    MPUTR(confmap, "text_workspace_xshift", STRNC("11"));
    MPUTR(confmap, "text_workspace_yshift", STRNC("-17"));
    MPUTR(confmap, "border_color", STRNC("#AADDFF6FF"));
    MPUTR(confmap, "background_color", STRNC("#000022FF"));
    MPUTR(confmap, "background_color_focused", STRNC("#222266FF"));
    MPUTR(confmap, "window_border_radius", STRNC("20"));
    MPUTR(confmap, "window_border_size", STRNC("1"));
    MPUTR(confmap, "window_border_color", STRNC("#000022FF"));
    MPUTR(confmap, "workspace_border_radius", STRNC("7"));
    MPUTR(confmap, "workspace_border_size", STRNC("1"));
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
    mt_memory_describe(confmap, 0);
    printf("\n");
}

#endif
