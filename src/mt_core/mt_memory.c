#ifndef mt_memory_h
#define mt_memory_h

#include <assert.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CAL(X, Y, Z) mt_memory_calloc(X, Y, Z);
#define RET(X) mt_memory_retain(X)
#define REL(X) mt_memory_release(X)
#define HEAP(X) mt_memory_stack_to_heap(sizeof(X), NULL, NULL, (char*) &X)

void*  mt_memory_alloc(size_t size, void (*destructor)(void*), void (*descriptor)(void*, int));
void*  mt_memory_calloc(size_t size, void (*destructor)(void*), void (*descriptor)(void*, int));
void*  mt_memory_realloc(void* pointer, size_t size);
void*  mt_memory_retain(void* pointer);
char   mt_memory_release(void* pointer);
size_t mt_memory_retaincount(void* pointer);
void*  mt_memory_stack_to_heap(size_t size, void (*destructor)(void*), void (*descriptor)(void*, int), char* data);
void   mt_memory_describe(void* pointer, int level);

#endif

#if __INCLUDE_LEVEL__ == 0

struct mt_memory_head
{
    char id[2];
    void (*destructor)(void*);
    void (*descriptor)(void*, int);
    size_t retaincount;
};

void* mt_memory_alloc(
    size_t size,                    /* size of data to store */
    void (*destructor)(void*),      /* optional destructor */
    void (*descriptor)(void*, int)) /* optional descriptor for describing memory area */
{
    char* bytes = malloc(sizeof(struct mt_memory_head) + size);
    if (bytes != NULL)
    {
	struct mt_memory_head* head = (struct mt_memory_head*) bytes;

	head->id[0]       = 'm';
	head->id[1]       = 't';
	head->destructor  = destructor;
	head->descriptor  = descriptor;
	head->retaincount = 1;

	return bytes + sizeof(struct mt_memory_head);
    }
    else
	return NULL;
}

void* mt_memory_calloc(
    size_t size,                    /* size of data to store */
    void (*destructor)(void*),      /* optional destructor */
    void (*descriptor)(void*, int)) /* optional descriptor for describing memory area */
{
    char* bytes = calloc(1, sizeof(struct mt_memory_head) + size);

    if (bytes != NULL)
    {
	struct mt_memory_head* head = (struct mt_memory_head*) bytes;

	head->id[0]       = 'm';
	head->id[1]       = 't';
	head->destructor  = destructor;
	head->descriptor  = descriptor;
	head->retaincount = 1;

	return bytes + sizeof(struct mt_memory_head);
    }
    else
	return NULL;
}

void* mt_memory_stack_to_heap(
    size_t size,
    void (*destructor)(void*),
    void (*descriptor)(void*, int),
    char* data)
{
    char* bytes = mt_memory_alloc(size, destructor, descriptor);

    if (bytes != NULL)
    {
	memcpy(bytes, data, size);
	return bytes;
    }
    else
	return NULL;
}

void* mt_memory_realloc(void* pointer, size_t size)
{
    assert(pointer != NULL);

    char* bytes = (char*) pointer;
    bytes -= sizeof(struct mt_memory_head);
    bytes = realloc(bytes, sizeof(struct mt_memory_head) + size);
    if (bytes != NULL)
    {
	return bytes + sizeof(struct mt_memory_head);
    }
    else
	return NULL;
}

void* mt_memory_retain(void* pointer)
{
    assert(pointer != NULL);

    char* bytes = (char*) pointer;
    bytes -= sizeof(struct mt_memory_head);
    struct mt_memory_head* head = (struct mt_memory_head*) bytes;

    assert(head->id[0] == 'm');
    assert(head->id[1] == 't');

    if (head->retaincount < SIZE_MAX)
    {
	head->retaincount += 1;
	return pointer;
    }
    else
	return NULL;
}

char mt_memory_release(void* pointer)
{
    assert(pointer != NULL);

    char* bytes = (char*) pointer;
    bytes -= sizeof(struct mt_memory_head);
    struct mt_memory_head* head = (struct mt_memory_head*) bytes;

    assert(head->id[0] == 'm');
    assert(head->id[1] == 't');
    assert(head->retaincount > 0);

    head->retaincount -= 1;

    if (head->retaincount == 0)
    {
	if (head->destructor != NULL)
	    head->destructor(pointer);

	head->id[0] = '\0';
	head->id[1] = '\0';

	free(bytes);

	return 1;
    }

    return 0;
}

size_t mt_memory_retaincount(void* pointer)
{
    assert(pointer != NULL);

    char* bytes = (char*) pointer;
    bytes -= sizeof(struct mt_memory_head);
    struct mt_memory_head* head = (struct mt_memory_head*) bytes;

    assert(head->id[0] == 'm');
    assert(head->id[1] == 't');

    return head->retaincount;
}

void mt_memory_describe(void* pointer, int level)
{
    assert(pointer != NULL);

    char* bytes = (char*) pointer;
    bytes -= sizeof(struct mt_memory_head);
    struct mt_memory_head* head = (struct mt_memory_head*) bytes;

    assert(head->id[0] == 'm');
    assert(head->id[1] == 't');

    if (head->descriptor != NULL)
    {
	head->descriptor(pointer, ++level);
    }
    else
    {
	printf("no descriptor");
    }
}

#endif
