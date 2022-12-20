/*
  Kinetic UI CSS parser
  Does pretty dumb parsing, error handling should be improved
*/

#ifndef ku_css_h
#define ku_css_h

/* TODO write tests */

#include "mt_log.c"
#include "mt_map.c"
#include "mt_string.c"
#include "mt_vector.c"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

typedef struct _css_range_t
{
    uint32_t pos;
    uint32_t len;
} css_range_t;

typedef struct _prop_t
{
    css_range_t class;
    css_range_t key;
    css_range_t value;
} prop_t;

mt_map_t* ku_css_new(char* path);

#endif

#if __INCLUDE_LEVEL__ == 0

#include "mt_string_ext.c"

uint32_t ku_css_count_props(char* css)
{
    int   t = 0; // tag index
    char* c = css;
    while (*c)
    {
	if (*c == ':')
	    t++;
	c++;
    }
    return t;
}

void ku_css_analyze_classes(char* css, prop_t* props)
{
    int start = -1;
    // char     in_l  = 0; // in line
    uint32_t index    = 0;
    css_range_t class = {0};
    int   in_str      = 0;
    char* c           = css;
    while (*c)
    {
	if (*c == '}' || *c == ' ' || *c == '\r' || *c == '\n')
	{
	    if (c - css - 1 == start) start = c - css; // skip starting empty chars
	}
	else if (*c == '"') // class name
	{
	    in_str = 1 - in_str;
	}
	else if (*c == '{') // class name
	{
	    class.pos = start + 1;
	    class.len = c - css - start - 2;
	    while (*(css + class.pos + class.len) == ' ') class.len--;
	    class.len++;
	    start = c - css;
	}
	else if (*c == ':' && !in_str) // property name
	{
	    props[index].class   = class;
	    props[index].key.pos = start + 1;
	    props[index].key.len = c - css - start - 1;
	    start                = c - css;
	}
	else if (*c == ';') // value name
	{
	    props[index].class     = class;
	    props[index].value.pos = start + 1;
	    props[index].value.len = c - css - start - 1;

	    start = c - css;
	    index++;
	}

	c++;
    }
}

void ku_css_prop_desc(void* p, int level)
{
    printf("html prop_t");
}

prop_t* ku_css_new_parse(char* css)
{
    uint32_t cnt   = ku_css_count_props(css);
    prop_t*  props = CAL(sizeof(prop_t) * (cnt + 1), NULL, ku_css_prop_desc); // REL 1

    ku_css_analyze_classes(css, props);

    for (int i = 0; i < cnt; i++)
    {
	// prop_t p = props[i];
	// printf("extracted prop %.*s %.*s %.*s\n", p.class.len, css + p.class.pos, p.key.len, css + p.key.pos, p.value.len, css + p.value.pos);
    }

    return props;
}

mt_map_t* ku_css_new(char* filepath)
{
    mt_map_t* styles = MNEW();                       // REL 2
    char*     css    = mt_string_new_file(filepath); // REL 0

    if (css)
    {
	prop_t* view_styles = ku_css_new_parse(css); // REL 1
	prop_t* props       = view_styles;

	while ((*props).class.len > 0)
	{
	    prop_t t   = *props;
	    char*  cls = CAL(sizeof(char) * t.class.len + 1, NULL, mt_string_describe); // REL 3
	    char*  key = CAL(sizeof(char) * t.key.len + 1, NULL, mt_string_describe);   // REL 4
	    char*  val = CAL(sizeof(char) * t.value.len + 1, NULL, mt_string_describe); // REL 5

	    memcpy(cls, css + t.class.pos, t.class.len);
	    memcpy(key, css + t.key.pos, t.key.len);
	    memcpy(val, css + t.value.pos, t.value.len);

	    mt_map_t* style = MGET(styles, cls);
	    if (style == NULL)
	    {
		style = MNEW(); // REL 6
		MPUT(styles, cls, style);
		REL(style); // REL 6
	    }
	    MPUT(style, key, val);
	    props += 1;
	    REL(cls); // REL 3
	    REL(key); // REL 4
	    REL(val); // REL 5
	}

	REL(view_styles);
	REL(css);
    }
    else mt_log_error("Empty CSS descriptor");

    return styles;
}

#endif
