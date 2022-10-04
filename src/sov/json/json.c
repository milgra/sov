
#ifndef json_h
#define json_h

#include "zc_vector.c"

void json_parse(char* string, vec_t* result);

#endif

#if __INCLUDE_LEVEL__ == 0

#include "jsmn.c"
#include "zc_cstring.c"
#include "zc_memory.c"
#include "zc_string.c"
#include <string.h>

char* json_build_path(char* string, jsmntok_t* tokens, char* path, int index)
{
    // go to the upmost node
    if (tokens[index].parent > -1) path = json_build_path(string, tokens, path, tokens[index].parent);
    // if token is string or primitive, add to path
    if (tokens[index].type == JSMN_STRING || tokens[index].type == JSMN_PRIMITIVE)
    {
	path = cstr_append(path, "/");
	path = cstr_append_sub(path, string, tokens[index].start, tokens[index].end - tokens[index].start);
    }
    // if token is array, add index counter to path and increase counter
    if (tokens[index].type == JSMN_ARRAY)
    {
	char istr[10] = {0};
	snprintf(istr, 10, "/%i", tokens[index].aind);
	path = cstr_append(path, istr);
    }

    return path;
}

/* parses c string into an map */

void json_parse(char* string, vec_t* result)
{
    jsmn_parser parser;

    jsmn_init(&parser);

    int        tokcount = 128;
    jsmntok_t* tokens   = CAL(sizeof(jsmntok_t) * tokcount, NULL, NULL); // REL 0
    int        actcount = 0;

    while (1)
    {
	actcount = jsmn_parse(&parser, string, strlen(string), tokens, tokcount);
	if (actcount != JSMN_ERROR_NOMEM)
	{
	    break;
	}
	else
	{
	    tokcount *= 2;
	    tokens = mem_realloc(tokens, sizeof(jsmntok_t) * tokcount);
	}
    }

    if (actcount > 0)
    {
	tokens[0].parent = -1;

	for (int index = 1; index < actcount; index++)
	{
	    jsmntok_t cur_tok = tokens[index];
	    jsmntok_t par_tok = tokens[cur_tok.parent];

	    // printf("TOKEN index %i type %i par %i cont %s\n", index, cur_tok.type, cur_tok.parent, strndup(string + cur_tok.start, cur_tok.end - cur_tok.start));

	    // initiate path creation and storage when needed
	    if (cur_tok.type == JSMN_STRING || cur_tok.type == JSMN_PRIMITIVE)
	    {
		if (par_tok.type == JSMN_STRING || par_tok.type == JSMN_ARRAY)
		{
		    char* val = cstr_new_cstring("");
		    char* esc = NULL;
		    char* key = cstr_new_cstring("");

		    val = cstr_append_sub(val, string, cur_tok.start, cur_tok.end - cur_tok.start);
		    esc = cstr_unescape(val);
		    REL(val);
		    key = json_build_path(string, tokens, key, cur_tok.parent);

		    VADDR(result, key);
		    VADDR(result, esc);

		    // printf("%s - %s\n", key, val);
		}
	    }
	    // increase array counters
	    if (par_tok.type == JSMN_ARRAY)
	    {
		tokens[cur_tok.parent].aind += 1;
	    }
	}
    }

    REL(tokens);
}

#endif
