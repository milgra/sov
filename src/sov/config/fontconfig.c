#ifndef fontconfig_h
#define fontconfig_h

char* fontconfig_new_path(char* face_desc);

#endif

#if __INCLUDE_LEVEL__ == 0

#include "zc_cstring.c"
#include "zc_memory.c"
#include <stdio.h>
#include <string.h>

char* fontconfig_new_path(char* face_desc)
{
    char buff[100];

    char* cstr = cstr_new_cstring(""); // REL 0

	/* look for first match with fc-match */
    char* command = cstr_new_format(80, "fc-match %s --format=%%{file}", face_desc); // REL 1
    FILE* pipe    = popen(command, "r");                                    // CLOSE 0
    REL(command);                                                           // REL 1

    while (fgets(buff, sizeof(buff), pipe) != NULL) cstr = cstr_append(cstr, buff);
    pclose(pipe); // CLOSE 0

	if (strlen(cstr) == 0)
	{
	    /* if no result, get first available font */
	    command    = cstr_new_cstring("fc-list | grep ''$"); // REL 3
	    FILE* pipe = popen(command, "r");                    // CLOSE 2
	    REL(command);                                        // REL  3

	    while (fgets(buff, sizeof(buff), pipe) != NULL) cstr = cstr_append(cstr, buff);
	    pclose(pipe); // CLOSE 2
	}

    /* extract file name before double colon */

    return cstr;
}

#endif
