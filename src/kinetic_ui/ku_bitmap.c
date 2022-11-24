/*
  Kinetic UI bitmap
  Stores bitmap data in a uint8_t array.
  Can be ARGB and RGBA, it is ARGB currently because of Wayland.
  Does highly efficient inserting/cutting and alpha blending ( couldn't make it faster with pixman )
  Could be faster with SSE/MMX extensions.
*/

#ifndef ku_bitmap_h
#define ku_bitmap_h

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

/* TODO write test */

/* TODO use this */
enum ku_bitmap_format
{
    KU_BITMAP_ARGB,
    KU_BITMAP_RGBA
};

typedef struct _bmr_t bmr_t;
struct _bmr_t
{
    int x;
    int y;
    int z;
    int w;
};

typedef struct _ku_bitmap_t ku_bitmap_t;
struct _ku_bitmap_t
{
    int      w;
    int      h;
    uint32_t size;

    int      dw; // backing buffer row width
    int      dh; // backing buffer row height
    uint32_t dsize;

    uint8_t*              data;
    enum ku_bitmap_format type;
};

ku_bitmap_t* ku_bitmap_new(int the_w, int the_h);
ku_bitmap_t* ku_bitmap_new_clone(ku_bitmap_t* bm);
ku_bitmap_t* ku_bitmap_new_aligned(int the_w, int the_h, int align);
void         ku_bitmap_reset(ku_bitmap_t* bm);
void         ku_bitmap_describe(void* p, int level);
bmr_t        ku_bitmap_is(bmr_t l, bmr_t r);

void ku_bitmap_insert(
    ku_bitmap_t* dst,
    bmr_t        dstmsk,
    ku_bitmap_t* src,
    bmr_t        srcmsk,
    int          sx,
    int          sy);

void ku_bitmap_blend(
    ku_bitmap_t* dst,
    bmr_t        dstmsk,
    ku_bitmap_t* src,
    bmr_t        srcmsk,
    int          sx,
    int          sy);

void ku_bitmap_blend_with_alpha(
    ku_bitmap_t* dst,
    bmr_t        dstmsk,
    ku_bitmap_t* src,
    bmr_t        srcmsk,
    int          sx,
    int          sy,
    int          alpha);

void ku_bitmap_blend_rect(ku_bitmap_t* dst, int x, int y, int w, int h, uint32_t c);

#endif

#if __INCLUDE_LEVEL__ == 0

#include "mt_memory.c"
#include <assert.h>
#include <string.h>

#define BMMIN(X, Y) (((X) < (Y)) ? (X) : (Y))
#define BMMAX(X, Y) (((X) > (Y)) ? (X) : (Y))

void ku_bitmap_describe_data(void* p, int level);

void ku_bitmap_del(void* pointer)
{
    ku_bitmap_t* bm = pointer;

    /* if (bm->data != NULL) REL(bm->data); // REL 1 */
    if (bm->data != NULL) free(bm->data); // REL 1
}

ku_bitmap_t* ku_bitmap_new(int the_w, int the_h)
{
    assert(the_w > 0 && the_h > 0);

    ku_bitmap_t* bm = CAL(sizeof(ku_bitmap_t), ku_bitmap_del, ku_bitmap_describe); // REL 0

    bm->w = the_w;
    bm->h = the_h;

    bm->dw = the_w;
    bm->dh = the_h;

    bm->size  = 4 * the_w * the_h;
    bm->dsize = bm->size;
    bm->data  = calloc(sizeof(unsigned char), bm->size); // REL 1
    bm->type  = KU_BITMAP_ARGB;

    return bm;
}

ku_bitmap_t* ku_bitmap_new_aligned(int the_w, int the_h, int align)
{
    assert(the_w > 0 && the_h > 0);

    ku_bitmap_t* bm = CAL(sizeof(ku_bitmap_t), ku_bitmap_del, ku_bitmap_describe); // REL 0

    bm->w = the_w;
    bm->h = the_h;

    bm->dw = align;
    bm->dh = bm->h;
    while (bm->dw < bm->w * 4) bm->dw += 16;

    bm->size  = 4 * bm->w * bm->h;
    bm->dsize = bm->dw * bm->dh + 16 + 16 - 1; // alignment + stride for sws scale
    /* bm->data = CAL(bm->size * sizeof(unsigned char), NULL, ku_bitmap_describe_data); // REL 1 */

    posix_memalign((void**) &bm->data, 16, bm->dsize * sizeof(unsigned char));
    memset(bm->data, 0, bm->dsize);
    bm->type = KU_BITMAP_ARGB;

    return bm;
}

ku_bitmap_t* ku_bitmap_new_clone(ku_bitmap_t* the_bm)
{
    ku_bitmap_t* bm = ku_bitmap_new(the_bm->w, the_bm->h);
    memcpy(bm->data, the_bm->data, the_bm->size);
    return bm;
}

void ku_bitmap_reset(ku_bitmap_t* bm)
{
    memset(bm->data, 0, bm->size);
}

bmr_t ku_bitmap_is(bmr_t l, bmr_t r)
{
    bmr_t f = {0};
    if (!(l.z < r.x || r.z < l.x || l.w < r.y || r.w < l.y))
    {
	f.x = BMMAX(l.x, r.x);
	f.y = BMMAX(l.y, r.y);
	f.z = BMMIN(r.z, l.z);
	f.w = BMMIN(r.w, l.w);
    }

    return f;
}

void ku_bitmap_insert(
    ku_bitmap_t* dst,
    bmr_t        dstmsk,
    ku_bitmap_t* src,
    bmr_t        srcmsk,
    int          sx,
    int          sy)
{
    srcmsk.x += sx;
    srcmsk.z += sx;
    srcmsk.y += sy;
    srcmsk.w += sy;

    bmr_t f = ku_bitmap_is(srcmsk, dstmsk); // final source rectangle

    if (f.x == 0 && f.w == 0) return;
    if (f.y == 0 && f.z == 0) return;

    int sox = f.x - sx; // src offset
    int soy = f.y - sy;

    int dox = f.x; // dst offset
    int doy = f.y;

    int dex = f.z; // dst endpoints
    int dey = f.w;

    uint32_t* d = (uint32_t*) dst->data;
    uint32_t* s = (uint32_t*) src->data;
    uint32_t  l = (dex - dox) * 4; // length

    d += (doy * dst->w) + dox;
    s += (soy * src->w) + sox;

    for (int y = doy; y < dey; y++)
    {
	memcpy(d, s, l);

	d += dst->w;
	s += src->w;
    }
}

void ku_bitmap_blend(
    ku_bitmap_t* dst,
    bmr_t        dstmsk,
    ku_bitmap_t* src,
    bmr_t        srcmsk,
    int          sx,
    int          sy)
{
    srcmsk.x += sx;
    srcmsk.z += sx;
    srcmsk.y += sy;
    srcmsk.w += sy;

    bmr_t f = ku_bitmap_is(srcmsk, dstmsk); // final source rectangle

    if (f.x == 0 && f.w == 0) return;
    if (f.y == 0 && f.z == 0) return;

    int sox = f.x - sx; // src offset
    int soy = f.y - sy;

    int dox = f.x; // dst offset
    int doy = f.y;

    int dex = f.z; // dst endpoints
    int dey = f.w;

    uint32_t* d  = (uint32_t*) dst->data; // dest bytes
    uint32_t* s  = (uint32_t*) src->data; // source byes
    uint32_t* td = (uint32_t*) dst->data; // temporary dst bytes
    uint32_t* ts = (uint32_t*) src->data; // temporary src bytes

    d += (doy * dst->w) + dox;
    s += (soy * src->w) + sox;

    for (int y = doy; y < dey; y++)
    {
	td = d;
	ts = s;

	for (int x = dox; x < dex; x++)
	{
	    static const int AMASK    = 0xFF000000;
	    static const int RBMASK   = 0x00FF00FF;
	    static const int GMASK    = 0x0000FF00;
	    static const int AGMASK   = AMASK | GMASK;
	    static const int ONEALPHA = 0x01000000;

	    uint32_t dc = *td;
	    uint32_t sc = *ts;
	    int      sa = (sc & AMASK) >> 24;

	    if (sa == 256)
	    {
		*td = sc; // dest color is source color
	    }
	    else if (sa > 0)
	    {

		unsigned int na = 255 - sa;
		unsigned int rb = ((na * (dc & RBMASK)) + (sa * (sc & RBMASK))) >> 8;
		unsigned int ag = (na * ((dc & AGMASK) >> 8)) + (sa * (ONEALPHA | ((sc & GMASK) >> 8)));
		*td             = ((rb & RBMASK) | (ag & AGMASK));
	    }
	    // else dst remains the same

	    td++;
	    ts++;
	}
	d += dst->w;
	s += src->w;
    }
}

void ku_bitmap_blend_with_alpha(
    ku_bitmap_t* dst,
    bmr_t        dstmsk,
    ku_bitmap_t* src,
    bmr_t        srcmsk,
    int          sx,
    int          sy,
    int          alpha)
{
    if (alpha == 255) return;

    srcmsk.x += sx;
    srcmsk.z += sx;
    srcmsk.y += sy;
    srcmsk.w += sy;

    bmr_t f = ku_bitmap_is(srcmsk, dstmsk); // final source rectangle

    if (f.x == 0 && f.w == 0) return;
    if (f.y == 0 && f.z == 0) return;

    int sox = f.x - sx; // src offset
    int soy = f.y - sy;

    int dox = f.x; // dst offset
    int doy = f.y;

    int dex = f.z; // dst endpoints
    int dey = f.w;

    uint32_t* d  = (uint32_t*) dst->data; // dest bytes
    uint32_t* s  = (uint32_t*) src->data; // source byes
    uint32_t* td = (uint32_t*) dst->data; // temporary dst bytes
    uint32_t* ts = (uint32_t*) src->data; // temporary src bytes

    d += (doy * dst->w) + dox;
    s += (soy * src->w) + sox;

    for (int y = doy; y < dey; y++)
    {
	td = d;
	ts = s;

	for (int x = dox; x < dex; x++)
	{
	    static const int AMASK    = 0xFF000000;
	    static const int RBMASK   = 0x00FF00FF;
	    static const int GMASK    = 0x0000FF00;
	    static const int AGMASK   = AMASK | GMASK;
	    static const int ONEALPHA = 0x01000000;

	    uint32_t dc = *td;
	    uint32_t sc = *ts;
	    int      sa = ((sc & AMASK) >> 24) - alpha;

	    if (sa == 256)
	    {
		*td = sc; // dest color is source color
	    }
	    else if (sa > 0)
	    {
		unsigned int na = 255 - sa;
		unsigned int rb = ((na * (dc & RBMASK)) + (sa * (sc & RBMASK))) >> 8;
		unsigned int ag = (na * ((dc & AGMASK) >> 8)) + (sa * (ONEALPHA | ((sc & GMASK) >> 8)));
		*td             = ((rb & RBMASK) | (ag & AGMASK));
	    }
	    // else dst remains the same

	    td++;
	    ts++;
	}
	d += dst->w;
	s += src->w;
    }
}

void ku_bitmap_blend_rect(ku_bitmap_t* dst, int x, int y, int w, int h, uint32_t c)
{
    if (x > dst->w) return;
    if (y > dst->h) return;
    if (x + w < 0) return;
    if (y + h < 0) return;

    int dox = x; // dst offset
    int doy = y;

    if (x < 0)
    {
	dox = 0;
    }
    if (y < 0)
    {
	doy = 0;
    }

    int dex = x + w; // dst endpoints
    int dey = y + h;

    if (dex >= dst->w) dex = dst->w;
    if (dey >= dst->h) dey = dst->h;

    uint32_t* d  = (uint32_t*) dst->data; // dest bytes
    uint32_t* td = (uint32_t*) dst->data; // temporary dst bytes

    d += (doy * dst->w) + dox;

    for (int y = doy; y < dey; y++)
    {
	td = d;

	for (int x = dox; x < dex; x++)
	{
	    static const int AMASK    = 0xFF000000;
	    static const int RBMASK   = 0x00FF00FF;
	    static const int GMASK    = 0x0000FF00;
	    static const int AGMASK   = AMASK | GMASK;
	    static const int ONEALPHA = 0x01000000;

	    uint32_t dc = *td;
	    uint32_t sc = c;
	    int      sa = (sc & AMASK) >> 24;

	    if (sa == 256)
	    {
		*td = sc; // dest color is source color
	    }
	    else if (sa > 0)
	    {
		unsigned int na = 255 - sa;
		unsigned int rb = ((na * (dc & RBMASK)) + (sa * (sc & RBMASK))) >> 8;
		unsigned int ag = (na * ((dc & AGMASK) >> 8)) + (sa * (ONEALPHA | ((sc & GMASK) >> 8)));
		*td             = ((rb & RBMASK) | (ag & AGMASK));
	    }
	    // else dst remains the same

	    td++;
	}
	d += dst->w;
    }
}

void ku_bitmap_describe(void* p, int level)
{
    ku_bitmap_t* bm = p;
    printf("width %i height %i size %u", bm->w, bm->h, bm->size);
}

void ku_bitmap_describe_data(void* p, int level)
{
    printf("bm data\n");
}

#endif
