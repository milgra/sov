#ifndef mt_vector_test_h
#define mt_vector_test_h

void mt_vector_test_main();

#endif

#if __INCLUDE_LEVEL__ == 0

#include "mt_log.c"
#include "mt_vector.c"

void mt_vector_test_main()
{
    char* text1 = "Test text 1";
    char* text2 = "Test text 2";
    char* text3 = "Test text 3";
    char* text4 = "Test text 4";

    char* ttext1 = mt_memory_stack_to_heap(strlen(text1) + 1, NULL, NULL, (char*) text1);
    char* ttext2 = mt_memory_stack_to_heap(strlen(text2) + 1, NULL, NULL, (char*) text2);
    char* ttext3 = mt_memory_stack_to_heap(strlen(text3) + 1, NULL, NULL, (char*) text3);
    char* ttext4 = mt_memory_stack_to_heap(strlen(text4) + 1, NULL, NULL, (char*) text4);

    /* checking if vector allocated correctly */

    mt_log_debug("testing mt_vector_new");

    mt_vector_t* vec1 = mt_vector_new();

    assert(vec1 != NULL);
    assert(vec1->length == 0);

    /* checking if add works correctly */

    mt_log_debug("testing mt_vector_add");

    mt_vector_add(vec1, ttext1);

    assert(vec1->length == 1);
    assert(vec1->data[0] == ttext1);

    /* checking if add release works correctly */

    mt_log_debug("testing mt_vector_add_rel");

    mt_vector_add_rel(vec1, ttext2);

    assert(vec1->length == 2);
    assert(vec1->data[1] == ttext2);
    assert(mt_memory_retaincount(ttext2) == 1);

    /* checking if insert works correctly */

    mt_log_debug("testing mt_vector_insert");

    mt_vector_ins(vec1, ttext3, 1);

    assert(vec1->length == 3);
    assert(vec1->data[1] == ttext3);

    /* checking if insert release works correctly */

    mt_log_debug("testing mt_vector_insert_rel");

    mt_vector_ins_rel(vec1, ttext4, 2);

    assert(vec1->length == 4);
    assert(vec1->data[2] == ttext4);
    assert(mt_memory_retaincount(ttext4) == 1);

    RET(ttext4);

    /* checking if remove data works correctly */

    mt_log_debug("testing mt_vector_rem");

    mt_vector_rem(vec1, ttext1);

    assert(vec1->length == 3);
    assert(vec1->data[0] == ttext3);

    /* checking if remove index works correctly */

    mt_log_debug("testing mt_vector_rem_index");

    mt_vector_rem_index(vec1, 1);

    assert(vec1->length == 2);
    assert(vec1->data[1] == ttext2);

    /* checking if remove range works correctly */

    mt_log_debug("testing mt_vector_rem_range");

    mt_vector_rem_range(vec1, 0, 1);

    assert(vec1->length == 0);

    /* checking add in vector */

    mt_vector_t* vec2 = mt_vector_new();
    mt_vector_add(vec2, ttext1);
    mt_vector_add(vec2, ttext2);
    mt_vector_add(vec2, ttext3);
    mt_vector_add(vec2, ttext4);

    mt_log_debug("testing mt_vector_add_in_vector");

    mt_vector_add_in_vector(vec1, vec2);

    assert(vec1->length == 4);
    assert(vec1->data[3] == ttext4);

    /* checking remove in vector */

    mt_log_debug("testing mt_vector_rem_in_vector");

    mt_vector_rem_in_vector(vec1, vec2);

    assert(vec1->length == 0);

    /* checking head and tail */

    mt_vector_add(vec1, ttext1);
    mt_vector_add(vec1, ttext2);
    mt_vector_add(vec1, ttext3);
    mt_vector_add(vec1, ttext4);

    mt_log_debug("testing mt_vector_head");

    assert(mt_vector_head(vec1) == ttext1);

    mt_log_debug("testing mt_vector_tail");

    assert(mt_vector_tail(vec1) == ttext4);

    /* checking reverse */

    mt_log_debug("testing mt_vector_reverse");

    mt_vector_reverse(vec1);

    assert(vec1->length == 4);
    assert(vec1->data[0] == ttext4);
    assert(vec1->data[3] == ttext1);

    /* checking sort */

    mt_log_debug("testing mt_vector_sort");

    mt_vector_sort(vec1, (int (*)(void*, void*)) strcmp);

    assert(vec1->length == 4);
    assert(vec1->data[0] == ttext1);
    assert(vec1->data[3] == ttext4);

    /* checking index of */

    assert(mt_vector_index_of_data(vec1, ttext2) == 1);

    REL(vec1);
    REL(vec2);
    REL(ttext1);
    REL(ttext2);
    REL(ttext3);
    REL(ttext4);
    REL(ttext4);
}

#endif
