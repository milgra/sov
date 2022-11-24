#ifndef mt_memory_h
#define mt_memory_h

#include <assert.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

/* TODO separate unit tests */

#undef MT_MEMORY_DEBUG
#ifdef MT_MEMORY_DEBUG
    #define MT_MEMORY_DEBUG_SIZE 1000000 /* maximum areas to store */
    #define MT_MEMORY_DEBUG_INDEX 0      /* head index to stop at error */
#endif

#define CAL(X, Y, Z) mt_memory_calloc(X, Y, Z);
#define RET(X) mt_memory_retain(X)
#define REL(X) mt_memory_release(X)
#define HEAP(X) mt_memory_stack_to_heap(sizeof(X), NULL, NULL, (uint8_t*) &X)

void*  mt_memory_alloc(size_t size, void (*destructor)(void*), void (*descriptor)(void*, int));
void*  mt_memory_calloc(size_t size, void (*destructor)(void*), void (*descriptor)(void*, int));
void*  mt_memory_realloc(void* pointer, size_t size);
void*  mt_memory_retain(void* pointer);
char   mt_memory_release(void* pointer);
size_t mt_memory_retaincount(void* pointer);
void*  mt_memory_stack_to_heap(size_t size, void (*destructor)(void*), void (*descriptor)(void*, int), uint8_t* data);
void   mt_memory_describe(void* pointer, int level);

#ifdef MT_MEMORY_DEBUG
void mt_memory_stats();
#endif

#endif

#if __INCLUDE_LEVEL__ == 0

#include <execinfo.h>
#include <string.h>

struct mt_memory_head
{
#ifdef MT_MEMORY_DEBUG
    uint32_t index; /* allocation index for debugging/statistics */
#endif
    void (*destructor)(void*);
    void (*descriptor)(void*, int);
    int32_t retaincount;
};

#ifdef MT_MEMORY_DEBUG

struct mt_memory_head* mt_memory_heads[MT_MEMORY_DEBUG_SIZE] = {0};
static uint32_t        mt_memory_index                       = 1; /* live object counter for debugging */

void mt_memory_trace(
    char* id, struct mt_memory_head* head)
{
    printf("*** %i %s : %i ***\n", head->index, id, head->retaincount);

    void*  array[128];
    int    size    = backtrace(array, 128);
    char** strings = backtrace_symbols(array, size);
    for (int i = 0; i < size; ++i) printf("%s\n", strings[i]);
    free(strings);
}

#endif

void* mt_memory_alloc(
    size_t size,                    /* size of data to store */
    void (*destructor)(void*),      /* optional destructor */
    void (*descriptor)(void*, int)) /* optional descriptor for describing memory area */
{
    uint8_t* bytes = malloc(sizeof(struct mt_memory_head) + size);
    if (bytes != NULL)
    {
	struct mt_memory_head* head = (struct mt_memory_head*) bytes;

	head->destructor  = destructor;
	head->descriptor  = descriptor;
	head->retaincount = 1;

#ifdef MT_MEMORY_DEBUG
	head->index                      = mt_memory_index;
	mt_memory_heads[mt_memory_index] = head;
	if (head->index == MT_MEMORY_DEBUG_INDEX) mt_memory_trace("ALLOC", head);
	mt_memory_index++;
#endif

	return bytes + sizeof(struct mt_memory_head);
    }
    else return NULL;
}

void* mt_memory_calloc(
    size_t size,                    /* size of data to store */
    void (*destructor)(void*),      /* optional destructor */
    void (*descriptor)(void*, int)) /* optional descriptor for describing memory area */
{
    uint8_t* bytes = calloc(1, sizeof(struct mt_memory_head) + size);
    if (bytes != NULL)
    {
	struct mt_memory_head* head = (struct mt_memory_head*) bytes;

	head->destructor  = destructor;
	head->descriptor  = descriptor;
	head->retaincount = 1;

#ifdef MT_MEMORY_DEBUG
	head->index                      = mt_memory_index;
	mt_memory_heads[mt_memory_index] = head;
	if (head->index == MT_MEMORY_DEBUG_INDEX) mt_memory_trace("CALLOC", head);
	mt_memory_index++;
#endif

	return bytes + sizeof(struct mt_memory_head);
    }
    else return NULL;
}

void* mt_memory_stack_to_heap(
    size_t size,
    void (*destructor)(void*),
    void (*descriptor)(void*, int),
    uint8_t* data)
{
    uint8_t* bytes = mt_memory_alloc(size, destructor, descriptor);

    if (bytes != NULL)
    {
	memcpy(bytes, data, size);
	return bytes;
    }
    else return NULL;
}

void* mt_memory_realloc(void* pointer, size_t size)
{
    assert(pointer != NULL);

    uint8_t* bytes = (uint8_t*) pointer;
    bytes -= sizeof(struct mt_memory_head);
    bytes = realloc(bytes, sizeof(struct mt_memory_head) + size);
    if (bytes != NULL)
    {
#ifdef MT_MEMORY_DEBUG
	struct mt_memory_head* head  = (struct mt_memory_head*) bytes;
	mt_memory_heads[head->index] = head;
#endif
	return bytes + sizeof(struct mt_memory_head);
    }
    else return NULL;
}

void* mt_memory_retain(void* pointer)
{
    assert(pointer != NULL);

    uint8_t* bytes = (uint8_t*) pointer;
    bytes -= sizeof(struct mt_memory_head);
    struct mt_memory_head* head = (struct mt_memory_head*) bytes;

    if (head->retaincount < SIZE_MAX)
    {
	head->retaincount += 1;
#ifdef MT_MEMORY_DEBUG
	if (head->index == MT_MEMORY_DEBUG_INDEX) mt_memory_trace("CALLOC", head);
#endif
	return pointer;
    }
    else return NULL;
}

char mt_memory_release(void* pointer)
{
    assert(pointer != NULL);

    uint8_t* bytes = (uint8_t*) pointer;
    bytes -= sizeof(struct mt_memory_head);
    struct mt_memory_head* head = (struct mt_memory_head*) bytes;

    head->retaincount -= 1;

#ifdef MT_MEMORY_DEBUG
    if (head->index == MT_MEMORY_DEBUG_INDEX) mt_memory_trace("RELEASE", head);
#endif

    assert(head->retaincount > -1);

    if (head->retaincount == 0)
    {
	if (head->destructor != NULL) head->destructor(pointer);
	    /* don't clean up to catch overrelease or leaks */
#ifndef MT_MEMORY_DEBUG
	free(bytes);
#endif
	return 1;
    }

    return 0;
}

size_t mt_memory_retaincount(void* pointer)
{
    assert(pointer != NULL);

    uint8_t* bytes = (uint8_t*) pointer;
    bytes -= sizeof(struct mt_memory_head);
    struct mt_memory_head* head = (struct mt_memory_head*) bytes;

    return head->retaincount;
}

void mt_memory_describe(void* pointer, int level)
{
    assert(pointer != NULL);

    uint8_t* bytes = (uint8_t*) pointer;
    bytes -= sizeof(struct mt_memory_head);
    struct mt_memory_head* head = (struct mt_memory_head*) bytes;

    if (head->descriptor != NULL)
    {
	head->descriptor(pointer, ++level);
    }
    else
    {
	printf("no descriptor");
    }
}

#ifdef MT_MEMORY_DEBUG

void mt_memory_stats()
{
    printf("\n***MEM STATS***\n");

    // print block statistics

    int problem = 0;

    for (int index = 1; index < mt_memory_index; index++)
    {
	if (MT_MEMORY_DEBUG_INDEX == 0 || MT_MEMORY_DEBUG_INDEX == index)
	{
	    if (mt_memory_heads[index]->retaincount < 0)
	    {
		problem = 1;
		printf("OVERRELEASE at %i : %i\n", index, mt_memory_heads[index]->retaincount);
	    }
	    if (mt_memory_heads[index]->retaincount > 0)
	    {
		problem = 1;
		printf("LEAK at %i : %i\n", index, mt_memory_heads[index]->retaincount);
	    }
	}
    }

    if (problem == 0) printf("Everything seems all right\n");
}

#endif

#endif
