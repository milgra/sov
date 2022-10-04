#ifndef zc_bm_rgba_h
#define zc_bm_rgba_h

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

typedef struct _bm_rgba_t bm_rgba_t;
struct _bm_rgba_t
{
  int w;
  int h;

  uint8_t* data;
  uint32_t size;
};

bm_rgba_t* bm_rgba_new(int the_w, int the_h);
bm_rgba_t* bm_rgba_new_clone(bm_rgba_t* bm);
void       bm_rgba_reset(bm_rgba_t* bm);
void       bm_rgba_describe(void* p, int level);

#endif

#if __INCLUDE_LEVEL__ == 0

#include "zc_memory.c"
#include <assert.h>
#include <string.h>

void bm_rgba_describe_data(void* p, int level);

void bm_rgba_del(void* pointer)
{
  bm_rgba_t* bm = pointer;

  if (bm->data != NULL) REL(bm->data); // REL 1
}

bm_rgba_t* bm_rgba_new(int the_w, int the_h)
{
  assert(the_w > 0 && the_h > 0);

  bm_rgba_t* bm = CAL(sizeof(bm_rgba_t), bm_rgba_del, bm_rgba_describe); // REL 0

  bm->w = the_w;
  bm->h = the_h;

  bm->size = 4 * the_w * the_h;
  bm->data = CAL(bm->size * sizeof(unsigned char), NULL, bm_rgba_describe_data); // REL 1

  return bm;
}

bm_rgba_t* bm_rgba_new_clone(bm_rgba_t* the_bm)
{
  bm_rgba_t* bm = bm_rgba_new(the_bm->w, the_bm->h);
  memcpy(bm->data, the_bm->data, the_bm->size);
  return bm;
}

void bm_rgba_reset(bm_rgba_t* bm)
{
  memset(bm->data, 0, bm->size);
}

void bm_rgba_describe(void* p, int level)
{
  bm_rgba_t* bm = p;
  printf("width %i height %i size %u", bm->w, bm->h, bm->size);
}

void bm_rgba_describe_data(void* p, int level)
{
  printf("bm data\n");
}

#endif
