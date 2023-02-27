#ifndef mt_path_test_h
#define mt_path_test_h

void mt_path_test_main();

#endif

#if __INCLUDE_LEVEL__ == 0

#include "mt_log.c"
#include "mt_path.c"
#include <stdio.h>
#include <string.h>

void mt_path_test_main()
{
    /* checking if mt_path_new_format works */

    mt_log_debug("testing mt_path_new_append");

    char* path1 = mt_path_new_append("root", "exec/");

    assert(strcmp(path1, "root/exec/") == 0);

    /* checking if mt_path_new_remove_last_component works */

    mt_log_debug("testing mt_path_new_remove_last_component");

    char* path2 = mt_path_new_remove_last_component("/home/milgra/whatever.ext");

    assert(strcmp(path2, "/home/milgra/") == 0);

    /* checking if mt_path_new_extension works */

    mt_log_debug("testing mt_path_new_extension");

    char* path3 = mt_path_new_extension("/home/milgra/whatever.ext");

    assert(strcmp(path3, "ext") == 0);

    /* checking if mt_path_new_filename works */

    mt_log_debug("testing mt_path_new_filename");

    char* path4 = mt_path_new_filename("/home/milgra/whatever.ext");

    assert(strcmp(path4, "whatever") == 0);

    /* checking if mt_path_new_normalize works */

    mt_log_debug("testing mt_path_new_normalize");

    char* path5 = mt_path_new_normalize("//milcsi/../whatever.ext");

    assert(strcmp(path5, "//whatever.ext"));

    REL(path1);
    REL(path2);
    REL(path3);
    REL(path4);
    REL(path5);
}

#endif
