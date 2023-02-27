#ifndef mt_memory_test_h
#define mt_memory_test_h

void mt_memory_test_main();

#endif

#if __INCLUDE_LEVEL__ == 0

#include "mt_log.c"
#include "mt_memory.c"
#include <stdio.h>

void test_alloc()
{
    mt_log_debug("testing mt_memory_alloc");

    char* atext = NULL;

    atext = mt_memory_alloc(sizeof(char) * 80, NULL, NULL);

    assert(atext != NULL);
    assert(mt_memory_retaincount(atext) == 1);

    mt_memory_release(atext);
}

void test_calloc()
{
    mt_log_debug("testing mt_memory_calloc");

    char* ctext = NULL;

    ctext = mt_memory_calloc(sizeof(char) * 80, NULL, NULL);

    assert(ctext != NULL);
    assert(mt_memory_retaincount(ctext) == 1);

    mt_memory_release(ctext);
}

void test_stack_to_heap()
{
    mt_log_debug("testing mt_memory_stack_to_heap");

    char* text  = "This is a test string";
    char* ttext = mt_memory_stack_to_heap(strlen(text) + 1, NULL, NULL, (char*) text);

    assert(ttext != NULL);
    assert(mt_memory_retaincount(ttext) == 1);
    assert(strcmp(ttext, text) == 0);

    mt_memory_release(ttext);
}

void test_realloc()
{
    mt_log_debug("testing mt_memory_realloc");

    char* text0 = "This is a test string";
    char* text1 = "Another text string";
    char* ttext = mt_memory_calloc(strlen(text0) + 1, NULL, NULL);
    memcpy(ttext, text0, strlen(text0));

    ttext = mt_memory_realloc(ttext, strlen(text0) + strlen(text1) + 1);
    memcpy(ttext + strlen(text0), text1, strlen(text1));

    assert(ttext != NULL);
    assert(mt_memory_retaincount(ttext) == 1);
    assert(strstr(ttext, text0) != NULL);
    assert(strstr(ttext, text1) != NULL);

    mt_memory_release(ttext);
}

int destruct_count = 1;

void destruct()
{
    destruct_count -= 1;
}

void test_destructor()
{
    mt_log_debug("testing mt_memory descructor");

    char* atext = NULL;

    atext = mt_memory_alloc(sizeof(char) * 80, destruct, NULL);

    assert(atext != NULL);
    assert(mt_memory_retaincount(atext) == 1);

    mt_memory_release(atext);

    assert(destruct_count == 0);
}

int describe_count = 1;

void describe()
{
    describe_count -= 1;
}

void test_descriptor()
{
    mt_log_debug("testing mt_memory descriptor");

    char* atext = NULL;

    atext = mt_memory_alloc(sizeof(char) * 80, NULL, describe);

    assert(atext != NULL);
    assert(mt_memory_retaincount(atext) == 1);

    mt_memory_describe(atext, 0);
    mt_memory_release(atext);

    assert(describe_count == 0);
}

/* run test with clang leak sanitizer */

void mt_memory_test_main()
{
    test_alloc();
    test_calloc();
    test_stack_to_heap();
    test_realloc();
    test_destructor();
    test_descriptor();
}

#endif
