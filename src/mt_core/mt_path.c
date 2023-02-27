#ifndef mt_path_h
#define mt_path_h

#include "mt_string.c"

char* mt_path_new_append(char* root, char* component);
char* mt_path_new_remove_last_component(char* path);
char* mt_path_new_extension(char* path);
char* mt_path_new_filename(char* path);
char* mt_path_new_normalize(char* path);

#endif

#if __INCLUDE_LEVEL__ == 0

#include <limits.h>
#include <string.h>
#include <unistd.h>
#ifdef __linux__
    #include <linux/limits.h>
#endif

#include "mt_log.c"
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
    /* TODO use POSIX dirname */
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
    else
	return mt_string_new_cstring("/");
}

char* mt_path_new_extension(char* path)
{
    char* ext = strrchr(path, '.');
    char* res = NULL;
    if (ext)
	res = mt_string_new_cstring(ext + 1);

    /* int index; */
    /* for (index = strlen(path) - 1; index > -1; --index) */
    /* { */
    /* 	if (path[index] == '.') */
    /* 	{ */
    /* 	    index++; */
    /* 	    break; */
    /* 	} */
    /* } */

    /* int   len = strlen(path) - index; */
    /* char* ext = CAL(len + 1, NULL, mt_string_describe); */
    /* memcpy(ext, path + index, len); */
    return res;
}

char* mt_path_new_filename(char* path)
{
    /* TODO use POSIX basename */

    int dotindex;
    for (dotindex = strlen(path) - 1; dotindex > -1; --dotindex)
    {
	if (path[dotindex] == '.')
	    break;
    }

    if (dotindex == -1)
	dotindex = strlen(path) - 1;

    int slashindex;
    for (slashindex = strlen(path) - 1; slashindex > -1; --slashindex)
    {
	if (path[slashindex] == '/')
	{
	    slashindex++;
	    break;
	}
    }
    if (slashindex == -1)
	slashindex = 0;
    int   len   = dotindex - slashindex;
    char* title = CAL(len + 1, NULL, mt_string_describe);
    memcpy(title, path + slashindex, len);
    return title;
}

char* mt_path_new_normalize(char* path)
{
    char* extpath = mt_string_new_cstring("");
    char* newpath = CAL(PATH_MAX, NULL, NULL);

    if (path[0] == '~')
    {
	/* replace tilde with home dir */
	extpath = mt_string_append(extpath, getenv("HOME"));
	extpath = mt_string_append_sub(extpath, path, 1, strlen(path) - 1);
    }
    else
	extpath = mt_string_append(extpath, path);

    realpath(extpath, newpath);

    REL(extpath);

    /* char cwd[PATH_MAX] = {"~"}; */
    /* getcwd(cwd, sizeof(cwd)); */

    /* char* newpath = NULL; */

    /* if (path[0] == '~') */
    /* { */
    /* 	/\* replace tilde with home dir *\/ */
    /* 	newpath = mt_string_new_cstring(getenv("HOME")); */
    /* 	newpath = mt_string_append_sub(newpath, path, 1, strlen(path) - 1); */
    /* } */
    /* else if (path[0] == '\0') */
    /* { */
    /* 	/\* empty path to root path *\/ */
    /* 	newpath = mt_string_new_cstring("/"); */
    /* } */
    /* else if (path[0] != '/') */
    /* { */
    /* 	/\* insert working directory in case of relative path *\/ */
    /* 	newpath = mt_string_new_format(PATH_MAX, "%s/%s", cwd, path); */
    /* } */
    /* else */
    /* { */
    /* 	newpath = mt_string_new_cstring(path); */
    /* } */

    /* /\* remove last component in case of double dot *\/ */

    /* size_t len = strlen(newpath); */
    /* if (len > 3 && newpath[len - 1] == '.' && newpath[len - 2] == '.') */
    /* { */
    /* 	for (size_t index = len - 3; index-- > 0;) */
    /* 	{ */
    /* 	    if (newpath[index] == '/') */
    /* 	    { */
    /* 		newpath[index] = '\0'; */
    /* 		break; */
    /* 	    } */
    /* 	} */
    /* } */

    return newpath;
}

#endif
