#ifndef mt_number_h
#define mt_number_h

#include <stdint.h>

typedef union
{
    float    floatv;
    int      intv;
    uint32_t uint32v;
} mt_number_t;

mt_number_t* mt_number_new_float(float val);
mt_number_t* mt_number_new_int(int val);
mt_number_t* mt_number_new_uint32(uint32_t val);

#endif

#if __INCLUDE_LEVEL__ == 0

#include "mt_memory.c"

void mt_number_describe(void* p, int level)
{
    mt_number_t* num = p;
    printf("num %f %i %u", num->floatv, num->intv, num->uint32v);
}

mt_number_t* mt_number_new_float(float val)
{
    mt_number_t* res = CAL(sizeof(mt_number_t), NULL, mt_number_describe);
    res->floatv      = val;
    return res;
}

mt_number_t* mt_number_new_int(int val)
{
    mt_number_t* res = CAL(sizeof(mt_number_t), NULL, mt_number_describe);
    res->intv        = val;
    return res;
}

mt_number_t* mt_number_new_uint32(uint32_t val)
{
    mt_number_t* res = CAL(sizeof(mt_number_t), NULL, mt_number_describe);
    res->uint32v     = val;
    return res;
}

#endif
