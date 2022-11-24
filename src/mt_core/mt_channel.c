/*
  One-way non-locking communication channel between threads
  If mtch_send returns 0, channel is full, send data again later
  If mtch_recv returns 0, channel is empty
 */

#ifndef mt_channel_h
#define mt_channel_h

/* TODO separate unit tests */

#include "mt_memory.c"
#include <assert.h>
#include <pthread.h>
#include <stdio.h>
#include <time.h>

typedef struct mt_channel_t mt_channel_t;
struct mt_channel_t
{
    char*  flags;
    void** boxes;

    uint32_t size;
    uint32_t rpos; // read position
    uint32_t wpos; // write position
};

mt_channel_t* mt_channel_new(uint32_t size);
void          mt_channel_del(void* pointer);
char          mt_channel_send(mt_channel_t* ch, void* data);
void*         mt_channel_recv(mt_channel_t* ch);
void          mt_channel_test(void);

#endif

#if __INCLUDE_LEVEL__ == 0

void mt_channel_del(void* pointer)
{
    assert(pointer != NULL);

    mt_channel_t* ch = pointer;

    REL(ch->flags);
    REL(ch->boxes);
}

void mt_channel_describe(void* p, int level)
{
    mt_channel_t* ch = p;
    printf("mt_channel, size %u read pos %u write pos %u", ch->size, ch->rpos, ch->wpos);
}

void mt_channel_describe_flags(void* p, int level)
{
    printf("mt_channel flags");
}

void mt_channel_describe_boxes(void* p, int level)
{
    printf("mt_channel boxes");
}

mt_channel_t* mt_channel_new(uint32_t size)
{
    mt_channel_t* ch = CAL(sizeof(mt_channel_t), mt_channel_del, mt_channel_describe);

    ch->flags = CAL(sizeof(char) * size, NULL, mt_channel_describe_flags);
    ch->boxes = CAL(sizeof(void*) * size, NULL, mt_channel_describe_boxes);
    ch->size  = size;
    ch->rpos  = 0;
    ch->wpos  = 0;

    return ch;
}

char mt_channel_send(mt_channel_t* ch, void* data)
{
    assert(ch != NULL);
    assert(data != NULL);

    // wait for the box to get empty

    if (ch->flags[ch->wpos] == 0)
    {
	ch->flags[ch->wpos] = 1; // set flag, it doesn't have to be atomic, only the last bit counts
	ch->boxes[ch->wpos] = data;
	ch->wpos += 1; // increment write index, doesn't have to be atomic, this thread uses it only
	if (ch->wpos == ch->size) ch->wpos = 0;

	return 1;
    }

    return 0;
}

void* mt_channel_recv(mt_channel_t* ch)
{
    assert(ch != NULL);

    if (ch->flags[ch->rpos] == 1)
    {
	void* result = ch->boxes[ch->rpos];

	ch->boxes[ch->rpos] = NULL; // empty box
	ch->flags[ch->rpos] = 0;    // set flag, it doesn't have to be atomic, only the last bit counts
	ch->rpos += 1;              // increment read index, it doesn't have to be atomic, this thread

	if (ch->rpos == ch->size) ch->rpos = 0;

	return result;
    }

    return NULL;
}
#endif
