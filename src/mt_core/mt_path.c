#ifndef mt_path_h
#define mt_path_h

/* TODO separate unit tests */

#include "mt_string.c"

char* mt_path_new_append(char* root, char* component);
char* mt_path_new_remove_last_component(char* path);
char* mt_path_new_extension(char* path);
char* mt_path_new_filename(char* path);
char* mt_path_new_normalize(char* path, char* execpath);
char* mt_path_new_normalize1(char* path);

#endif

#if __INCLUDE_LEVEL__ == 0

#include <limits.h>
#include <string.h>
#ifdef __linux__ 
#include <linux/limits.h>
#endif

#include "mt_memory.c"

char* mt_path_new_append(char* root, char* component)
{
    if (root[strlen(root) - 1] == '/')
	return mt_string_new_format(PATH_MAX + NAME_MAX, "%s%s", root, component);
    else
	return mt_string_new_format(PATH_MAX + NAME_MAX, "%s/%s", root, component);
}

char* mt_path_new_remove_last_component(char* path)
{
    int index;
    for (index = strlen(path) - 2; index > 0; index--)
    {
	if (path[index] == '/')
	{
	    index++;
	    break;
	}
    }

    if (index > -1)
    {
	char* str = CAL(index + 1, NULL, mt_string_describe);
	memcpy(str, path, index);
	return str;
    }
    else return mt_string_new_cstring("/");
}

char* mt_path_new_extension(char* path)
{
    int index;
    for (index = strlen(path) - 1; index > -1; --index)
    {
	if (path[index] == '.')
	{
	    index++;
	    break;
	}
    }

    int   len = strlen(path) - index;
    char* ext = CAL(len + 1, NULL, mt_string_describe);
    memcpy(ext, path + index, len);
    return ext;
}

char* mt_path_new_filename(char* path)
{
    int dotindex;
    for (dotindex = strlen(path) - 1; dotindex > -1; --dotindex)
    {
	if (path[dotindex] == '.') break;
    }

    if (dotindex == -1) dotindex = strlen(path) - 1;

    int slashindex;
    for (slashindex = strlen(path) - 1; slashindex > -1; --slashindex)
    {
	if (path[slashindex] == '/')
	{
	    slashindex++;
	    break;
	}
    }
    if (slashindex == -1) slashindex = 0;
    int   len   = dotindex - slashindex;
    char* title = CAL(len + 1, NULL, mt_string_describe);
    memcpy(title, path + slashindex, len);
    return title;
}

char* mt_path_new_normalize(char* path, char* execpath)
{
    char* result = NULL;

    if (path[0] == '~') // if starts with tilde, insert home dir
	result = mt_string_new_format(PATH_MAX + NAME_MAX, "%s%s", getenv("HOME"), path + 1);
    else if (path[0] != '/') // if doesn't start with / insert base dir
	result = mt_string_new_format(PATH_MAX + NAME_MAX, "%s/%s", execpath, path);
    else
	result = mt_string_new_cstring(path);

    // if ends with '/' remove it
    if (result[strlen(result) - 1] == '/') result[strlen(result) - 1] = '\0';

    return result;
}

char* mt_path_new_normalize1(char* path)
{
    mt_vector_t* tokens = mt_string_tokenize(path, "/");
    char*        result = NULL;

    if (tokens->length > 0)
    {
	result              = mt_string_new_cstring("");
	mt_vector_t* newtok = VNEW();

	for (int index = 0; index < tokens->length; index++)
	{
	    char* token = tokens->data[index];
	    if (token[0] == '~')
	    {
		// replace tilde with home
		VADDR(newtok, mt_string_new_cstring(getenv("HOME")));
	    }
	    else if (strlen(token) == 2 && token[0] == '.' && token[1] == '.')
	    {
		// delete last token
		mt_vector_rem_at_index(newtok, newtok->length - 1);
	    }
	    else if (strlen(token) == 1 && token[0] == '.')
	    {
		// do nothing at current dir
	    }
	    else
	    {
		VADD(newtok, token);
	    }
	}

	/* assemble new tokens */

	for (int index = 0; index < newtok->length; index++)
	{
	    char* token = newtok->data[index];
	    result      = mt_string_append(result, "/");
	    result      = mt_string_append(result, token);
	}

	if (newtok->length == 0) result = mt_string_new_cstring("/");

	REL(newtok);
    }
    else
    {
	result = mt_string_new_cstring("/");
    }

    REL(tokens);
    return result;
}

#endif
