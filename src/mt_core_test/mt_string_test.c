#ifndef mt_string_test_h
#define mt_string_test_h

void mt_string_test_main();

#endif

#if __INCLUDE_LEVEL__ == 0

#include "mt_log.c"
#include "mt_string.c"

void mt_string_test_main()
{
    /* char* str = "000 Állítólag svájcban"; */

    /* checking if mt_string_new_format works */

    mt_log_debug("testing mt_string_new_format");

    char* str1 = mt_string_new_format(100, "%s-%i-%x", "milcsi", 100, 64);

    assert(strcmp(str1, "milcsi-100-40") == 0);

    /* checking if mt_string_new_cstring works */

    mt_log_debug("testing mt_string_new_cstring");

    char* str2 = mt_string_new_cstring("another cstring");

    assert(strcmp(str2, "another cstring") == 0);

    /* checking if mt_string_new_substring works */

    mt_log_debug("testing mt_string_new_substring");

    char* str3 = mt_string_new_substring("another cstring", 3, 6);

    assert(strcmp(str3, "ther c") == 0);

    /* checking if mt_string_reset works */

    mt_log_debug("testing mt_string_reset");

    str1 = mt_string_reset(str1);

    assert(strlen(str1) == 0);

    /* checking if mt_string_append works */

    mt_log_debug("testing mt_string_append");

    str1 = mt_string_append(str1, "new string");
    str1 = mt_string_append(str1, "|old string");

    assert(strcmp(str1, "new string|old string") == 0);

    /* checking if mt_string_append works */

    mt_log_debug("testing mt_string_append_cp");

    str1 = mt_string_append_cp(str1, 588);

    assert(strcmp(str1, "new string|old stringɌ") == 0);

    /* checking if mt_string_append_sub works */

    mt_log_debug("testing mt_string_append_sub");

    str2 = mt_string_append_sub(str2, "WHAT THE", 2, 3);

    assert(strcmp(str2, "another cstringAT ") == 0);

    /* checking if mt_string_delete_utf_codepoints works */

    mt_log_debug("testing mt_string_delete_utf_codepoints");

    str1 = mt_string_delete_utf_codepoints(str1, 4, 18);

    assert(strcmp(str1, "new ") == 0);

    /* checking if mt_string_tokenize works */

    mt_log_debug("testing mt_string_tokenize");

    mt_vector_t* vec = mt_string_tokenize("ONE TWO THREE", " ");

    assert(vec->length == 3);
    assert(strcmp(vec->data[0], "ONE") == 0);
    assert(strcmp(vec->data[1], "TWO") == 0);
    assert(strcmp(vec->data[2], "THREE") == 0);

    REL(vec);
    REL(str1);
    REL(str2);
    REL(str3);
}

#endif
