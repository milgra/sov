#ifndef mt_map_test_h
#define mt_map_test_h

void mt_map_test_main();

#endif

#if __INCLUDE_LEVEL__ == 0

#include "mt_log.c"
#include "mt_map.c"

void mt_map_test_main()
{
    char* text1 = "Test text 1";
    char* text2 = "Test text 2";

    char* ttext1 = mt_memory_stack_to_heap(strlen(text1) + 1, NULL, NULL, (char*) text1);
    char* ttext2 = mt_memory_stack_to_heap(strlen(text2) + 1, NULL, NULL, (char*) text2);

    /* checking if map is allocated correctly */

    mt_log_debug("testing mt_map_new");

    mt_map_t* map1 = mt_map_new();

    assert(map1 != NULL);
    assert(map1->count == 0);

    /* checking if put works correctly */

    mt_log_debug("testing mt_map_put");

    mt_map_put(map1, "text1", ttext1);

    char* ctext1 = mt_map_get(map1, "text1");

    assert(ttext1 == ctext1);

    /* checking if put release working correctly by checking retain counter */

    mt_log_debug("testing mt_map_put_rel");

    mt_map_put_rel(map1, "text2", ttext2);

    char* ctext2 = mt_map_get(map1, "text2");

    assert(ttext2 == ctext2);
    assert(mt_memory_retaincount(ttext2) == 1);

    /* checking if map_keys working correctly */

    mt_log_debug("testing mt_map_keys");

    mt_vector_t* keys = mt_vector_new();

    mt_map_keys(map1, keys);

    assert(keys->length == 2);

    for (size_t index = 0; index < keys->length; index++)
    {
	char* key = keys->data[index];

	assert(strcmp(key, "text1") == 0 || strcmp(key, "text2") == 0);
    }

    REL(keys);

    /* checking if map_values working correctly */

    mt_log_debug("testing mt_map_values");

    mt_vector_t* vals = mt_vector_new();

    mt_map_values(map1, vals);

    assert(vals->length == 2);

    for (size_t index = 0; index < vals->length; index++)
    {
	char* val = vals->data[index];

	assert(val == ttext1 || val == ttext2);
    }

    REL(vals);

    /* checking if map_del working correctly */

    mt_log_debug("testing mt_map_del");

    mt_map_del(map1, "text2");

    assert(map1->count == 1);
    assert(mt_map_get(map1, "text2") == NULL);

    /* checking if map_reset working correctly */

    mt_log_debug("testing mt_map_reset");

    mt_map_reset(map1);

    assert(map1->count == 0);

    /* cleanup, run with leak sanitizer on to check for hidden leaks */

    REL(ttext1);
    REL(map1);
}

#endif
