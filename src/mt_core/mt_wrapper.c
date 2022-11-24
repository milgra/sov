#ifndef mt_wrapper_h
#define mt_wrapper_h

/* TODO separate unit tests */

#include <stdint.h>

typedef struct
{
    void* data;
} mt_wrapper_t;

mt_wrapper_t* mt_wrapper_new(void* pointer);

#endif

#if __INCLUDE_LEVEL__ == 0

#include "mt_memory.c"

mt_wrapper_t* mt_wrapper_new(void* pointer)
{
    mt_wrapper_t* res = CAL(sizeof(mt_wrapper_t), NULL, NULL);
    res->data         = pointer;
    return res;
}

#endif
