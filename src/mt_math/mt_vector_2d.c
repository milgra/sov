#ifndef mt_vector_2d_h
#define mt_vector_2d_h

#include <math.h>
#include <stdio.h>

typedef struct _v2_t v2_t;
struct _v2_t
{
    float x, y;
};

v2_t v2_init(float x, float y);
v2_t v2_add(v2_t a, v2_t b);
v2_t v2_sub(v2_t a, v2_t b);
v2_t v2_scale(v2_t vector, float ratio);
v2_t v2_resize(v2_t a, float size);
v2_t v2_rotate(v2_t vector, float newangle);
v2_t v2_rotate_90_left(v2_t vector);
v2_t v2_rotate_90_right(v2_t vector);

v2_t  v2_midpoints(v2_t pointa, v2_t pointb);
float v2_length(v2_t a);
float v2_angle_x(v2_t a);
float v2_circular_angle_between(v2_t a, v2_t b);
char  v2_equals(v2_t a, v2_t b);
void  v2_describe(v2_t vector);
float v2_longside(v2_t a);

#endif

#if __INCLUDE_LEVEL__ == 0

#ifndef M_PI
    #define M_PI 3.14159265358979323846
#endif

/* creates vector2 */

v2_t v2_init(float x, float y)
{
    v2_t result;

    result.x = x;
    result.y = y;

    return result;
}

/* adds two vectors */

v2_t v2_add(v2_t a, v2_t b)
{
    v2_t result;

    result.x = a.x + b.x;
    result.y = a.y + b.y;

    return result;
}

/* substracts b from a */

v2_t v2_sub(v2_t a, v2_t b)
{
    v2_t result;

    result.x = a.x - b.x;
    result.y = a.y - b.y;

    return result;
}

/* multiplies vector with ratio */

v2_t v2_scale(v2_t vector, float ratio)
{
    v2_t result;

    result.x = vector.x * ratio;
    result.y = vector.y * ratio;

    return result;
}

/* resizes vector */

v2_t v2_resize(v2_t a, float size)
{
    float length = sqrtf(a.x * a.x + a.y * a.y);
    float ratio  = size / length;
    a.x *= ratio;
    a.y *= ratio;
    return a;
}

/* rotates vector with given radians */

v2_t v2_rotate(v2_t vector, float newangle)
{
    float angle  = v2_angle_x(vector);
    float length = v2_length(vector);
    angle += newangle;
    return v2_init(cos(angle) * length, sin(angle) * length);
}

/* rotates vector to left with 90 degrees */

v2_t v2_rotate_90_left(v2_t vector)
{
    return v2_init(-vector.y, vector.x);
}

/* rotates vector to right with 90 degrees */

v2_t v2_rotate_90_right(v2_t vector)
{
    return v2_init(vector.y, -vector.x);
}

/* returns middle point */

v2_t v2_midpoints(v2_t pointa, v2_t pointb)
{
    v2_t pointc = v2_sub(pointb, pointa);

    pointc = v2_scale(pointc, 0.5);
    pointc = v2_add(pointa, pointc);

    return pointc;
}

/* returns vector length */

float v2_length(v2_t a)
{
    return sqrtf(a.x * a.x + a.y * a.y);
}

/* returns vector angel on x axis */

float v2_angle_x(v2_t a)
{
    return atan2(a.y, a.x);
}

/* returns angle between two vector as a circular CCW angle where angle 0 is
 * vector a */

float v2_circular_angle_between(v2_t a, v2_t b)
{
    float anglea = v2_angle_x(a);
    float angleb = v2_angle_x(b);

    float anglere = angleb - anglea;
    if (anglere < 0.0)
	anglere += 2 * M_PI;
    return anglere;
}

/* compares two vectors */

char v2_equals(v2_t a, v2_t b)
{
    return a.x == b.x && a.y == b.y;
}

/* describes vector */

void v2_describe(v2_t vector)
{
    printf("%f %f ", vector.x, vector.y);
}

/* returns longer side projected to x */

float v2_longside(v2_t a)
{
    float x = fabs(a.x);
    float y = fabs(a.y);
    return x > y ? x : y;
}

#endif
