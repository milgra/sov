#ifndef mt_vector_4d_h
#define mt_vector_4d_h

#include "mt_vector_3d.c"

typedef struct _v4_t v4_t;
struct _v4_t
{
    float x, y, z, w;
};

v4_t v4_init(float x, float y, float z, float w);
v4_t v4_add(v4_t a, v4_t b);
v4_t v4_sub(v4_t a, v4_t b);
v4_t v4_scale(v4_t a, float f);
void v4_describe(v4_t vector);

#endif
#if __INCLUDE_LEVEL__ == 0

#include <stdio.h>

/* creates vector */

v4_t v4_init(float x, float y, float z, float w)
{
    v4_t result;

    result.x = x;
    result.y = y;
    result.z = z;
    result.w = w;

    return result;
}

/* adds two vectors */

v4_t v4_add(v4_t a, v4_t b)
{
    v4_t result;

    result.x = a.x + b.x;
    result.y = a.y + b.y;
    result.z = a.z + b.z;
    result.w = a.w + b.w;

    return result;
}

/* substract b from a */

v4_t v4_sub(v4_t a, v4_t b)
{
    v4_t result;

    result.x = a.x - b.x;
    result.y = a.y - b.y;
    result.z = a.z - b.z;
    result.w = a.w - b.w;

    return result;
}

/* scales vector */

v4_t v4_scale(v4_t a, float f)
{
    v4_t result;

    result.x = a.x * f;
    result.y = a.y * f;
    result.z = a.z * f;
    result.w = a.w * f;

    return result;
}

/* describes vector4 */

void v4_describe(v4_t vector)
{
    printf("x : %f y : %f z : %f w : %f", vector.x, vector.y, vector.z, vector.w);
}

#endif
