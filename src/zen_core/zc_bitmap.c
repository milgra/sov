#ifndef zc_bitmap_h
#define zc_bitmap_h

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

typedef struct _bm_t bm_t;
struct _bm_t
{
  int w;
  int h;

  uint8_t* data;
  uint32_t size;
};

bm_t* bm_new(int the_w, int the_h);
bm_t* bm_new_clone(bm_t* bm);
bm_t* bm_new_flip_y(bm_t* bm);
void  bm_reset(bm_t* bm);
void  bm_describe(void* p, int level);
void  bm_write(bm_t* bm, char* path);

#endif

#if __INCLUDE_LEVEL__ == 0

#include "zc_memory.c"
#include <assert.h>
#include <string.h>

void bm_describe_data(void* p, int level);

void bm_del(void* pointer)
{
  bm_t* bm = pointer;

  if (bm->data != NULL) REL(bm->data); // REL 1
}

bm_t* bm_new(int the_w, int the_h)
{
  assert(the_w > 0 && the_h > 0);

  bm_t* bm = CAL(sizeof(bm_t), bm_del, bm_describe); // REL 0

  bm->w = the_w;
  bm->h = the_h;

  bm->size = 4 * the_w * the_h;
  bm->data = CAL(bm->size * sizeof(unsigned char), NULL, bm_describe_data); // REL 1

  return bm;
}

bm_t* bm_new_clone(bm_t* the_bm)
{
  bm_t* bm = bm_new(the_bm->w, the_bm->h);
  memcpy(bm->data, the_bm->data, the_bm->size);
  return bm;
}

bm_t* bm_new_flip_y(bm_t* bm)
{
  bm_t* tmp = bm_new(bm->w, bm->h);
  for (int y = 0; y < bm->h; y++)
  {
    int src_y = bm->h - y - 1;
    memcpy(tmp->data + y * bm->w * 4, bm->data + src_y * bm->w * 4, bm->w * 4);
  }
  return tmp;
}

void bm_reset(bm_t* bm)
{
  memset(bm->data, 0, bm->size);
}

void bm_describe(void* p, int level)
{
  bm_t* bm = p;
  printf("width %i height %i size %u", bm->w, bm->h, bm->size);
}

void bm_describe_data(void* p, int level)
{
  printf("bm data\n");
}

void bm_write(bm_t* bm, char* path)
{
  int w = bm->w;
  int h = bm->h;

  FILE*          f;
  unsigned char* img      = NULL;
  int            filesize = 54 + 3 * w * h; // w is your image width, h is image height, both int

  img = (unsigned char*)malloc(3 * w * h);
  memset(img, 0, 3 * w * h);

  for (int i = 0; i < w; i++)
  {
    for (int j = 0; j < h; j++)
    {
      int index = j * w * 4 + i * 4;

      int x = i;
      int y = j;

      int r = bm->data[index];
      int g = bm->data[index + 1];
      int b = bm->data[index + 2];

      if (r > 255) r = 255;
      if (g > 255) g = 255;
      if (b > 255) b = 255;

      img[(x + y * w) * 3 + 2] = (unsigned char)(r);
      img[(x + y * w) * 3 + 1] = (unsigned char)(g);
      img[(x + y * w) * 3 + 0] = (unsigned char)(b);
    }
  }

  unsigned char bmpfileheader[14] = {'B', 'M', 0, 0, 0, 0, 0, 0, 0, 0, 54, 0, 0, 0};
  unsigned char bmpinfoheader[40] = {40, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 24, 0};
  unsigned char bmppad[3]         = {0, 0, 0};

  bmpfileheader[2] = (unsigned char)(filesize);
  bmpfileheader[3] = (unsigned char)(filesize >> 8);
  bmpfileheader[4] = (unsigned char)(filesize >> 16);
  bmpfileheader[5] = (unsigned char)(filesize >> 24);

  bmpinfoheader[4]  = (unsigned char)(w);
  bmpinfoheader[5]  = (unsigned char)(w >> 8);
  bmpinfoheader[6]  = (unsigned char)(w >> 16);
  bmpinfoheader[7]  = (unsigned char)(w >> 24);
  bmpinfoheader[8]  = (unsigned char)(h);
  bmpinfoheader[9]  = (unsigned char)(h >> 8);
  bmpinfoheader[10] = (unsigned char)(h >> 16);
  bmpinfoheader[11] = (unsigned char)(h >> 24);

  f = fopen(path, "wb");
  fwrite(bmpfileheader, 1, 14, f);
  fwrite(bmpinfoheader, 1, 40, f);
  for (int i = 0; i < h; i++)
  {
    fwrite(img + (w * (h - i - 1) * 3), 3, w, f);
    fwrite(bmppad, 1, (4 - (w * 3) % 4) % 4, f);
  }

  free(img);
  fclose(f);
}

#endif
