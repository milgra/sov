#ifndef fontconfig_h
#define fontconfig_h

char* fontconfig_new_path(char* face_desc);

#endif

#if __INCLUDE_LEVEL__ == 0

#define _POSIX_C_SOURCE 200112L
#include "zc_cstring.c"
#include "zc_memory.c"
#include <limits.h>
#ifdef __linux__
    #include <linux/limits.h>
#endif
#include <stdio.h>
#include <string.h>

char* fontconfig_new_path(char* face_desc)
{
    char  buff[PATH_MAX];
    char* filename = cstr_new_cstring("");                                                // REL 0
    char* command  = cstr_new_format(80, "fc-match \"%s\" --format=%%{file}", face_desc); // REL 1
    FILE* pipe     = popen(command, "r");                                                 // CLOSE 0
    while (fgets(buff, sizeof(buff), pipe) != NULL) filename = cstr_append(filename, buff);
    pclose(pipe); // CLOSE 0
    REL(command); // REL 1

    return filename;
}

#endif
