#ifndef ku_html_h
#define ku_html_h

/* TODO write tests */

#include "mt_string.c"
#include "mt_vector.c"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

typedef struct _html_range_t
{
    uint32_t pos;
    uint32_t len;
} html_range_t;

typedef struct _tag_t
{
    uint32_t pos;
    uint32_t len;

    uint32_t level;
    uint32_t parent;

    html_range_t id;
    html_range_t type;
    html_range_t text;
    html_range_t class;
    html_range_t script;
} tag_t;

tag_t* ku_html_new(char* path);

#endif

#if __INCLUDE_LEVEL__ == 0

uint32_t ku_html_count_tags(char* html)
{
    uint32_t t       = 0; // tag index
    char*    c       = html;
    int      in_tag  = 0;
    int      in_comm = 0;
    while (*c)
    {
	if (*c == '<')
	{
	    if (!in_comm)
	    {
		in_tag = 1;
	    }
	}
	else if (*c == '!')
	{
	    if (in_tag)
	    {
		if (*(c - 1) == '<') in_comm += 1;
	    }
	}
	else if (*c == '>')
	{
	    if (in_comm)
	    {
		if (*(c - 1) == '-') in_comm -= 1;

		if (!in_comm)
		{
		    in_tag = 0;
		}
	    }
	    else if (in_tag)
	    {
		in_tag = 0;
		t++;
	    }
	}
	c++;
    }

    return t;
}

void ku_html_extract_tags(char* html, tag_t* tags)
{
    uint32_t t       = 0; // tag index
    uint32_t i       = 0; // char index
    char*    c       = html;
    int      in_tag  = 0;
    int      in_comm = 0;
    while (*c)
    {
	if (*c == '<')
	{
	    if (!in_comm)
	    {
		tags[t].pos = i;
		in_tag      = 1;
	    }
	}
	else if (*c == '!')
	{
	    if (in_tag)
	    {
		if (*(c - 1) == '<') in_comm += 1;
	    }
	}
	else if (*c == '>')
	{
	    if (in_comm)
	    {
		if (*(c - 1) == '-') in_comm -= 1;

		if (!in_comm)
		{
		    in_tag = 0;
		}
	    }
	    else if (in_tag)
	    {
		tags[t].len = i - tags[t].pos + 1;
		in_tag      = 0;
		// printf("storing %i tag %.*s\n", t, tags[t].len, html + tags[t].pos);
		t++;
	    }
	}
	i++;
	c++;
    }
}

html_range_t ku_html_extract_string(char* str, uint32_t pos, uint32_t len)
{
    int start = 0;
    int end   = 0;
    int in    = 0;
    for (int i = pos; i < pos + len; i++)
    {
	char c = str[i];
	if (c == '"')
	{
	    if (!in)
	    {
		in    = 1;
		start = i;
	    }
	    else
	    {
		in  = 0;
		end = i;
		break;
	    }
	}
    }
    if (!in)
	return ((html_range_t){.pos = start, .len = end - start - 1});
    else
	return ((html_range_t){0});
}

html_range_t ku_html_extract_value(tag_t tag, char* key, char* html)
{
    char*    start = strstr(html + tag.pos, key);
    uint32_t len   = start - (html + tag.pos);

    if (len < tag.len)
    {
	html_range_t range = ku_html_extract_string(html, start - html, tag.len);
	return range;
    }
    return ((html_range_t){0});
}

void ku_html_analyze_tags(char* html, tag_t* tags, uint32_t count)
{
    int l = 0; // level
    for (int i = 0; i < count; i++)
    {
	tags[i].level = l++;

	int ii = i;
	while (ii-- > 0)
	{
	    if (tags[ii].level == tags[i].level - 1)
	    {
		tags[i].parent = ii;
		break;
	    }
	}

	tags[i].id     = ku_html_extract_value(tags[i], "id=\"", html);
	tags[i].type   = ku_html_extract_value(tags[i], "type=\"", html);
	tags[i].text   = ku_html_extract_value(tags[i], "text=\"", html);
	tags[i].class  = ku_html_extract_value(tags[i], "class=\"", html);
	tags[i].script = ku_html_extract_value(tags[i], "script=\"", html);

	// tag_t t = tags[i];

	if (html[tags[i].pos + 1] == '/')
	    l -= 2; // </div>
	if (html[tags[i].pos + tags[i].len - 2] == '/' || html[tags[i].pos + tags[i].len - 2] == '-')
	    l -= 1; // />
    }
}

void ku_html_tag_describe(void* p, int level)
{
    printf("html tag_t");
}

tag_t* ku_html_new(char* html)
{
    uint32_t cnt  = ku_html_count_tags(html);
    tag_t*   tags = CAL(sizeof(tag_t) * (cnt + 1), NULL, ku_html_tag_describe); // REL 0

    ku_html_extract_tags(html, tags);
    ku_html_analyze_tags(html, tags, cnt);

    /* for (int i = 0; i < cnt; i++) */
    /* { */
    /* 	tag_t t = tags[i]; */
    /* 	printf("ind %i tag %.*s lvl %i par %i\n", i, t.len, html + t.pos, t.level, t.parent); */
    /* } */

    return tags;
}

#endif
