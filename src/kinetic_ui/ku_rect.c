#ifndef ku_rect_h
#define ku_rect_h

/* TODO write tests */

typedef struct _ku_rect_t ku_rect_t;
struct _ku_rect_t
{
    float x;
    float y;
    float w;
    float h;
};

int       ku_rect_equals(ku_rect_t r1, ku_rect_t r2);
ku_rect_t ku_rect_add(ku_rect_t r1, ku_rect_t r2);
ku_rect_t ku_rect_is(ku_rect_t l, ku_rect_t r);
void      ku_rect_describe(ku_rect_t rect);

#endif

#if __INCLUDE_LEVEL__ == 0

#include <stdio.h>

#define VMIN(X, Y) (((X) < (Y)) ? (X) : (Y))
#define VMAX(X, Y) (((X) > (Y)) ? (X) : (Y))

int ku_rect_equals(ku_rect_t r1, ku_rect_t r2)
{
    return (r1.x == r2.x && r1.y == r2.y && r1.w == r2.w && r1.h == r2.h);
}

ku_rect_t ku_rect_add(ku_rect_t r1, ku_rect_t r2)
{
    if (r1.w == 0 || r1.h == 0) return r2;
    if (r2.w == 0 || r2.h == 0) return r1;

    ku_rect_t res;

    res.x = VMIN(r1.x, r2.x);
    res.y = VMIN(r1.y, r2.y);

    float r1cx = r1.x + r1.w;
    float r1cy = r1.y + r1.h;
    float r2cx = r2.x + r2.w;
    float r2cy = r2.y + r2.h;

    res.w = r1cx < r2cx ? (r2cx - res.x) : (r1cx - res.x);
    res.h = r1cy < r2cy ? (r2cy - res.y) : (r1cy - res.y);

    return res;
}

ku_rect_t ku_rect_is(ku_rect_t l, ku_rect_t r)
{
    ku_rect_t f = {0};
    if (!(l.x + l.w < r.x || r.x + r.w < l.x || l.y + l.h < r.y || r.y + r.h < l.y))
    {
	f.x = VMAX(l.x, r.x);
	f.y = VMAX(l.y, r.y);
	f.w = VMIN(r.x + r.w - f.x, l.x + l.w - f.x);
	f.h = VMIN(r.y + r.h - f.y, l.y + l.h - f.y);
    }

    return f;
}

void ku_rect_describe(ku_rect_t rect)
{
    printf("%f %f %f %f\n", rect.x, rect.y, rect.w, rect.h);
}

#endif
