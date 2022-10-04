#ifndef zc_wrapper_h
#define zc_wrapper_h

#include <stdint.h>

typedef struct
{
  void* data;
} wrapper_t;

wrapper_t* wrapper_new(void* pointer);

#endif

#if __INCLUDE_LEVEL__ == 0

#include "zc_memory.c"

wrapper_t* wrapper_new(void* pointer)
{
  wrapper_t* res = CAL(sizeof(wrapper_t), NULL, NULL);
  res->data      = pointer;
  return res;
}

#endif
