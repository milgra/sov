/*
  Milan Toth's communication channel
  One-way non-locking communication channel between threads
  If mtch_send returns 0, channel is full, send data again later
  If mtch_recv returns 0, channel is empty
 */

#ifndef zc_channel_h
#define zc_channel_h

#include "zc_memory.c"
#include <assert.h>
#include <pthread.h>
#include <stdio.h>
#include <time.h>

typedef struct ch_t ch_t;
struct ch_t
{
  char*  flags;
  void** boxes;

  uint32_t size;
  uint32_t rpos; // read position
  uint32_t wpos; // write position
};

ch_t* ch_new(uint32_t size);
void  ch_del(void* pointer);
char  ch_send(ch_t* ch, void* data);
void* ch_recv(ch_t* ch);
void  ch_test(void);

#endif

#if __INCLUDE_LEVEL__ == 0

void ch_del(void* pointer)
{
  assert(pointer != NULL);

  ch_t* ch = pointer;

  REL(ch->flags);
  REL(ch->boxes);
}

void ch_describe(void* p, int level)
{
  ch_t* ch = p;
  printf("zc_channel");
}

void ch_describe_flags(void* p, int level)
{
  printf("zc_channel flags");
}

void ch_describe_boxes(void* p, int level)
{
  printf("zc_channel boxes");
}

ch_t* ch_new(uint32_t size)
{
  ch_t* ch = CAL(sizeof(ch_t), ch_del, ch_describe);

  ch->flags = CAL(sizeof(char) * size, NULL, ch_describe_flags);
  ch->boxes = CAL(sizeof(void*) * size, NULL, ch_describe_boxes);
  ch->size  = size;
  ch->rpos  = 0;
  ch->wpos  = 0;

  return ch;
}

char ch_send(ch_t* ch, void* data)
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

void* ch_recv(ch_t* ch)
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

//
//  TEST
//

#define kChTestThreads 10

void send_test(ch_t* ch)
{
  uint32_t counter = 0;
  while (1)
  {
    uint32_t* number = CAL(sizeof(uint32_t), NULL, NULL);
    *number          = counter;
    char success     = ch_send(ch, number);
    if (success == 0)
      REL(number);
    else
      counter += 1;
    if (counter == UINT32_MAX - 1)
      counter = 0;
    //            struct timespec time;
    //            time.tv_sec = 0;
    //            time.tv_nsec = rand() % 100000;
    //            nanosleep(&time , (struct timespec *)NULL);
  }
}

void recv_test(ch_t* ch)
{
  uint32_t last = 0;
  while (1)
  {
    uint32_t* number = ch_recv(ch);
    if (number != NULL)
    {
      if (*number != last)
        printf("index error!!!");
      REL(number);
      last += 1;
      if (last == UINT32_MAX - 1)
        last = 0;
      if (last % 100000 == 0)
        printf("%zx OK %u %u", (size_t)ch, last, UINT32_MAX);
      //                struct timespec time;
      //                time.tv_sec = 0;
      //                time.tv_nsec = rand() % 100000;
      //                nanosleep(&time , (struct timespec *)NULL);
    }
  }
}

ch_t** testarray;

void ch_test()
{
  testarray = CAL(sizeof(ch_t) * kChTestThreads, NULL, NULL);

  for (int index = 0; index < kChTestThreads; index++)
  {
    testarray[index] = ch_new(100);
    pthread_t thread;
    /* pthread_create(&thread, NULL, (void*)send_test, testarray[index]); */
    /* pthread_create(&thread, NULL, (void*)recv_test, testarray[index]); */
  }
}

#endif
