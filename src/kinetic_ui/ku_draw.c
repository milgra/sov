#ifndef ku_draw_h
#define ku_draw_h

/* TODO cleanup, rewrite, move functions into ku_bitmap, write tests */

#include "ku_bitmap.c"
#include <math.h>
#include <stdint.h>

void ku_draw_circle(ku_bitmap_t* bitmap, float cx, float cy, float r, float edge, uint32_t c);
void ku_draw_arc(ku_bitmap_t* bitmap, float cx, float cy, float r, float edge, uint32_t c, float as, float ae);
void ku_draw_rounded_rect(ku_bitmap_t* bitmap, int x, int y, int w, int h, int r, float edge, uint32_t c1, uint32_t c2);

void ku_draw_border(ku_bitmap_t* bitmap, int x, int y, int w, int h, int r, float edge, uint32_t c1);

void ku_draw_tile(ku_bitmap_t* bitmap);
void ku_draw_arc_grad(ku_bitmap_t* bm, float cx, float cy, float d1, float d2, float a1, float a2, uint32_t c1, uint32_t c2);
void ku_draw_grad_v(ku_bitmap_t* bm, int sx, int sy, int w, int h, uint32_t c1, uint32_t c2);
void ku_draw_grad_h(ku_bitmap_t* bm, int sx, int sy, int w, int h, uint32_t c1, uint32_t c2);
void ku_draw_rect(ku_bitmap_t* bm, int sx, int sy, int w, int h, uint32_t color,
		  char la); // leave alpha channel untouched
void ku_draw_blend_argb(ku_bitmap_t* bm, int nx, int ny, ku_bitmap_t* nbm);
void ku_draw_insert(ku_bitmap_t* base, ku_bitmap_t* src, int sx, int sy);
void ku_draw_insert_argb(ku_bitmap_t* base, uint8_t* src, int w, int h, int sx, int sy);
void ku_draw_insert_rgba(ku_bitmap_t* base, uint8_t* src, int w, int h, int sx, int sy);
void ku_draw_insert_rgb(ku_bitmap_t* base, uint8_t* src, int w, int h, int sx, int sy);
void ku_draw_blend_8(ku_bitmap_t* bm, int nx, int ny, uint32_t color, unsigned char* ndata, int nw, int nh);
void ku_draw_blend_8_1(ku_bitmap_t* bm, int nx, int ny, uint32_t color, unsigned char* ndata, int nw, int nh);
void ku_draw_blend_pixel(ku_bitmap_t* bm, int x, int y, uint32_t color);
void ku_draw_blend_bitmap(ku_bitmap_t* bm, ku_bitmap_t* sbm, int sx, int sy);
void ku_draw_blend_argb1(ku_bitmap_t* base, uint8_t* src, int w, int h, int sx, int sy);
void ku_draw_scale(ku_bitmap_t* srcbmp, ku_bitmap_t* dstbmp);
void ku_draw_insert_bitmap(ku_bitmap_t* base, ku_bitmap_t* src, int sx, int sy);

#endif

#if __INCLUDE_LEVEL__ == 0

#include "mt_log.c"
#include <string.h>

void ku_draw_circle(ku_bitmap_t* bitmap, float cx, float cy, float r, float edge, uint32_t c)
{
    float m = r;
    for (int x = 0; x < bitmap->w; x++)
    {
	for (int y = 0; y < bitmap->h; y++)
	{
	    int r = (c >> 24) & 0xFF;
	    int g = (c >> 16) & 0xFF;
	    int b = (c >> 8) & 0xFF;
	    int a = (c >> 0) & 0xFF;

	    float    dx = cx - (float) x;
	    float    dy = cy - (float) y;
	    float    d  = sqrt(dx * dx + dy * dy);
	    float    sa = (float) (c & 0xFF) / 255.0;
	    uint8_t  ra = a;
	    uint32_t fi = 0;
	    if (d < m)
	    {
		if (d > m - edge)
		{
		    float delta = m - d; // (edge - (d - (m - edge)));
		    float ratio = delta / edge;
		    ra          = (uint8_t) (ratio * sa * 255.0);
		}
		fi = (r << 24) | (g << 16) | (b << 8) | ra;
		ku_draw_blend_pixel(bitmap, x, y, fi);
	    }
	}
    }
}

void ku_draw_arc(ku_bitmap_t* bitmap, float cx, float cy, float r, float edge, uint32_t c, float as, float ae)
{
    float m = r;
    for (int x = 0; x < bitmap->w; x++)
    {
	for (int y = 0; y < bitmap->h; y++)
	{
	    float   dx = (float) x - cx;
	    float   dy = (float) y - cy;
	    float   d  = sqrt(dx * dx + dy * dy);
	    float   sa = (float) (c & 0xFF) / 255.0;
	    uint8_t ra = c & 0xFF;
	    if (d < m)
	    {
		float r = atan2(dy, dx);
		if (r < 0) r += 6.28;
		if (r > as && r < ae)
		{
		    if (d > m - edge)
		    {
			float delta = m - d; // (edge - (d - (m - edge)));
			float ratio = delta / edge;
			ra          = (uint8_t) (ratio * sa * 255.0);
		    }
		    ku_draw_blend_pixel(bitmap, x, y, (c & 0xFFFFFF00) | ra);
		}
	    }
	}
    }
}

void ku_draw_arc_grad(
    ku_bitmap_t* bm,
    float        cx, // center x
    float        cy, // center y
    float        d1, // distance 1
    float        d2, // distance 2
    float        a1, // angle 1
    float        a2, // angle 2
    uint32_t     c1, // color 1
    uint32_t     c2)     // color 2
{
    int sx = (int) cx - d2 - 1;
    int sy = (int) cy - d2 - 1;
    int ex = (int) cx + d2 + 1;
    int ey = (int) cy + d2 + 1;

    if (sx < 0) sx = 0;
    if (ex > bm->w) ex = bm->w;
    if (sy < 0) sy = 0;
    if (ey > bm->h) ey = bm->h;

    int r1 = (c1 >> 24) & 0xFF;
    int g1 = (c1 >> 16) & 0xFF;
    int b1 = (c1 >> 8) & 0xFF;
    int p1 = c1 & 0xFF;

    int r2 = (c2 >> 24) & 0xFF;
    int g2 = (c2 >> 16) & 0xFF;
    int b2 = (c2 >> 8) & 0xFF;
    int p2 = c2 & 0xFF;

    float ds = d2 - d1;
    float dr = (float) (r2 - r1) / ds;
    float dg = (float) (g2 - g1) / ds;
    float db = (float) (b2 - b1) / ds;
    float dp = (float) (p2 - p1) / ds;

    for (int x = sx; x < ex; x++)
    {
	for (int y = sy; y < ey; y++)
	{
	    float dx = (float) x - cx;
	    float dy = (float) y - cy;
	    float di = sqrt(dx * dx + dy * dy); // distance from center

	    if (di > d1 && di < d2) // we are between the two distances
	    {
		float an = atan2(dy, dx);
		if (an < 0) an += 6.28;
		if (an > a1 && an < a2) // we are between the two angles
		{
		    float di1 = di - d1;
		    float di2 = d2 - di;

		    float pr = 1.0; // alpha ratio

		    if (di1 < 1.0) pr = di1;
		    if (di2 < 1.0) pr = di2;

		    int r = r1 + (int) (di1 * dr);
		    int g = g1 + (int) (di1 * dg);
		    int b = b1 + (int) (di1 * db);
		    int p = (int) ((float) (p1 + (int) (di1 * dp)) * pr);

		    uint32_t fi = ((r & 0xFF) << 24) | ((g & 0xFF) << 16) | ((b & 0xFF) << 8) | (p & 0xFF);

		    ku_draw_blend_pixel(bm, x, y, fi);
		}
	    }
	}
    }
}

// tiled bitmap, mainly for testing opengl rendering
void ku_draw_tile(ku_bitmap_t* bitmap)
{
    for (int col = 0; col < bitmap->w; col++)
    {
	for (int row = 0; row < bitmap->h; row++)
	{
	    uint32_t color = (row % 2 == 0 && col % 2 == 1) ? 0xFFFFFFFF : 0x000000FF;
	    ku_draw_rect(bitmap, color, row, 1, 1, color, 0);
	}
    }
}

void ku_draw_grad_h(ku_bitmap_t* bm, int sx, int sy, int w, int h, uint32_t c1, uint32_t c2)
{
    int r1 = (c1 >> 24) & 0xFF;
    int g1 = (c1 >> 16) & 0xFF;
    int b1 = (c1 >> 8) & 0xFF;
    int a1 = c1 & 0xFF;

    int r2 = (c2 >> 24) & 0xFF;
    int g2 = (c2 >> 16) & 0xFF;
    int b2 = (c2 >> 8) & 0xFF;
    int a2 = c2 & 0xFF;

    float dr = (float) (r2 - r1) / (float) w;
    float dg = (float) (g2 - g1) / (float) w;
    float db = (float) (b2 - b1) / (float) w;
    float da = (float) (a2 - a1) / (float) w;

    int ex = sx + w;
    if (ex > bm->w) ex = bm->w;
    int ey = sy + h;
    if (ey > bm->h) ey = bm->h;

    uint8_t* data = bm->data;

    for (int y = sy; y < ey; y++)
    {
	for (int x = sx; x < ex; x++)
	{
	    int s = x - sx;
	    int i = (y * bm->w + x) * 4;
	    int r = r1 + (int) (s * dr);
	    int g = g1 + (int) (s * dg);
	    int b = b1 + (int) (s * db);
	    int a = a1 + (int) (s * da);

	    data[i]     = r;
	    data[i + 1] = g;
	    data[i + 2] = b;
	    data[i + 3] = a;
	}
    }
}

void ku_draw_grad_v(ku_bitmap_t* bm, int sx, int sy, int w, int h, uint32_t c1, uint32_t c2)
{
    int r1 = (c1 >> 24) & 0xFF;
    int g1 = (c1 >> 16) & 0xFF;
    int b1 = (c1 >> 8) & 0xFF;
    int a1 = c1 & 0xFF;

    int r2 = (c2 >> 24) & 0xFF;
    int g2 = (c2 >> 16) & 0xFF;
    int b2 = (c2 >> 8) & 0xFF;
    int a2 = c2 & 0xFF;

    float dr = (float) (r2 - r1) / (float) h;
    float dg = (float) (g2 - g1) / (float) h;
    float db = (float) (b2 - b1) / (float) h;
    float da = (float) (a2 - a1) / (float) h;

    int ex = sx + w;
    if (ex > bm->w) ex = bm->w;
    int ey = sy + h;
    if (ey > bm->h) ey = bm->h;

    uint8_t* data = bm->data;

    for (int y = sy; y < ey; y++)
    {
	for (int x = sx; x < ex; x++)
	{
	    int s = y - sy;
	    int i = (y * bm->w + x) * 4;
	    int r = r1 + (int) (s * dr);
	    int g = g1 + (int) (s * dg);
	    int b = b1 + (int) (s * db);
	    int a = a1 + (int) (s * da);

	    data[i]     = r;
	    data[i + 1] = g;
	    data[i + 2] = b;
	    data[i + 3] = a;
	}
    }
}

void ku_draw_rounded_rect(ku_bitmap_t* bitmap, int x, int y, int w, int h, int r, float edge, uint32_t c1, uint32_t c2)
{
    float e = edge;

    if (r > 0)
    {
	// corners
	ku_draw_arc_grad(bitmap, x + e + r, y + e + r, 0, r + 1, 3.14, 3.14 * 3 / 2.0, c1, c1);             // left top
	ku_draw_arc_grad(bitmap, x + w - e - r - 1, y + e + r, 0, r + 1, 3.14 * 3 / 2.0, 3.14 * 2, c1, c1); // right top
	ku_draw_arc_grad(bitmap, x + e + r, y + h - e - r - 1, 0, r + 1, 3.14 / 2.0, 3.14, c1, c1);         // left bottom
	ku_draw_arc_grad(bitmap, x + w - e - r - 1, y + h - e - r - 1, 0, r + 1, 0, 3.14 / 2.0, c1, c1);    // right bottom

	if (e > 0)
	{
	    // shadows
	    ku_draw_arc_grad(bitmap, x + e + r, y + e + r, r, r + e, 3.14, 3.14 * 3 / 2.0, c2, c2 & 0xFFFFFF00);
	    ku_draw_arc_grad(bitmap, x + w - e - r - 1, y + e + r, r, r + e, 3.14 * 3 / 2.0, 3.14 * 2, c2, c2 & 0xFFFFFF00);
	    ku_draw_arc_grad(bitmap, x + e + r, y + h - e - r - 1, r, r + e, 3.14 / 2.0, 3.14, c2, c2 & 0xFFFFFF00);
	    ku_draw_arc_grad(bitmap, x + w - e - r - 1, y + h - e - r - 1, r, r + e, 0, 3.14 / 2.0, c2, c2 & 0xFFFFFF00);
	}
    }

    ku_draw_grad_h(bitmap, x, y + e + r, e, h - 2 * e - 2 * r, c2 & 0xFFFFFF00, c2);             // left vertical grad
    ku_draw_grad_h(bitmap, x + w - e, y + e + r, e - 1, h - 2 * e - 2 * r, c2, c2 & 0xFFFFFF00); // right vertical grad
    ku_draw_grad_v(bitmap, x + e + r, y, w - 2 * e - 2 * r, e, c2 & 0xFFFFFF00, c2);             // top horizontal grad
    ku_draw_grad_v(bitmap, x + e + r, y + h - e, w - 2 * e - 2 * r, e - 1, c2, c2 & 0xFFFFFF00); // bottom horizontal grad

    ku_draw_rect(bitmap, x + e, y + e + r, w - 2 * e, h - 2 * e - 2 * r, c1, 0);
    ku_draw_rect(bitmap, x + e + r, y + e, w - 2 * e - 2 * r, h - 2 * e, c1, 0);
}

void ku_draw_border(ku_bitmap_t* bitmap, int x, int y, int w, int h, int r, float edge, uint32_t c1)
{
    float e = edge;

    if (r > 0)
    {
	ku_draw_arc_grad(
	    bitmap,
	    x + e + r,
	    y + e + r,
	    r,
	    r + e + 1.5,
	    3.14,
	    3.14 * 3 / 2.0,
	    c1,
	    c1); // left top
	ku_draw_arc_grad(
	    bitmap,
	    x + w - e - r - 1,
	    y + e + r,
	    r,
	    r + e + 1.5,
	    3.14 * 3 / 2.0,
	    3.14 * 2,
	    c1,
	    c1); // right top
	ku_draw_arc_grad(
	    bitmap,
	    x + e + r,
	    y + h - e - r - 1,
	    r,
	    r + e + 1.5,
	    3.14 / 2.0,
	    3.14,
	    c1,
	    c1); // left bottom
	ku_draw_arc_grad(
	    bitmap,
	    x + w - e - r - 1,
	    y + h - e - r - 1,
	    r,
	    r + e + 1.5,
	    0,
	    3.14 / 2.0,
	    c1,
	    c1); // right bottom
    }

    ku_draw_rect(
	bitmap,
	x,
	y + e + r,
	e,
	h - 2 * e - 2 * r,
	c1,
	0); // left vertical grad
    ku_draw_rect(
	bitmap,
	x + w - e,
	y + e + r,
	e,
	h - 2 * e - 2 * r,
	c1,
	0); // right vertical grad
    ku_draw_rect(
	bitmap,
	x + e + r,
	y,
	w - 2 * e - 2 * r,
	e,
	c1,
	0); // top horizontal grad
    ku_draw_rect(
	bitmap,
	x + e + r,
	y + h - e,
	w - 2 * e - 2 * r,
	e,
	c1,
	0); // bottom horizontal grad
}

void ku_draw_rect(ku_bitmap_t* bm, int sx, int sy, int w, int h, uint32_t color,
		  char la) // leave alpha channel untouched
{
    int ex = sx + w;
    if (ex > bm->w) ex = bm->w;
    int ey = sy + h;
    if (ey > bm->h) ey = bm->h;
    if (sx < 0) sx = 0;
    if (sy < 0) sy = 0;
    if (ex < 0) ex = 0;
    if (ey < 0) ey = 0;

    int r = color >> 24 & 0xFF;
    int g = color >> 16 & 0xFF;
    int b = color >> 8 & 0xFF;
    int a = color & 0xFF;

    for (int y = sy; y < ey; y++)
    {
	for (int x = sx; x < ex; x++)
	{
	    int position = (y * bm->w + x) * 4;

	    bm->data[position]     = r;
	    bm->data[position + 1] = g;
	    bm->data[position + 2] = b;
	    if (!la) bm->data[position + 3] = a;
	}
    }
}

void ku_draw_blend_argb(ku_bitmap_t* bm, int nx, int ny, ku_bitmap_t* nbm)
{
    int ex = nx + nbm->w;
    if (ex > bm->w) ex = bm->w;
    int ey = ny + nbm->h;
    if (ey > bm->h) ey = bm->h;

    uint8_t* odata = bm->data;
    uint8_t* ndata = nbm->data;

    for (int y = ny; y < ey; y++)
    {
	for (int x = nx; x < ex; x++)
	{
	    if (x > -1 && y > -1)
	    {
		int ni = ((y - ny) * nbm->w + (x - nx)) * 4; // new map index
		int oi = (y * bm->w + x) * 4;                // old map index

		int nr = ndata[ni];
		int ng = ndata[ni + 1];
		int nb = ndata[ni + 2];
		int na = ndata[ni + 3];

		int or = odata[oi];
		int og = odata[oi + 1];
		int ob = odata[oi + 2];
		int oa = odata[oi + 3];

		int a = na + oa * (255 - na) / 255;
		int r = nr * na / 255 + or *oa / 255 * (255 - na) / 255;
		int g = ng * na / 255 + og * oa / 255 * (255 - na) / 255;
		int b = nb * na / 255 + ob * oa / 255 * (255 - na) / 255;

		if (a > 0)
		{
		    r = r * 255 / a;
		    g = g * 255 / a;
		    b = b * 255 / a;
		}

		odata[oi]     = (uint8_t) (r & 0xFF);
		odata[oi + 1] = (uint8_t) (g & 0xFF);
		odata[oi + 2] = (uint8_t) (b & 0xFF);
		odata[oi + 3] = (uint8_t) (a & 0xFF);
	    }
	}
    }
}

void ku_draw_blend_argb1(ku_bitmap_t* base, uint8_t* src, int w, int h, int sx, int sy)
{
    int bx = sx + w;
    if (bx > base->w) bx = base->w;
    int by = sy + h;
    if (by > base->h) by = base->h;

    uint8_t* sdata = src;        // src data
    uint8_t* bdata = base->data; // base data

    for (int y = sy; y < by; y++)
    {
	for (int x = sx; x < bx; x++)
	{
	    if (x > -1 && y > -1)
	    {
		int si = ((y - sy) * w + (x - sx)) * 4; // src index
		int bi = (y * base->w + x) * 4;         // base index

		uint8_t nr = sdata[si];
		uint8_t ng = sdata[si + 1];
		uint8_t nb = sdata[si + 2];
		uint8_t na = sdata[si + 3];

		int or = bdata[bi];
		int og = bdata[bi + 1];
		int ob = bdata[bi + 2];
		int oa = bdata[bi + 3];

		int a = na + oa * (255 - na) / 255;
		int r = nr * na / 255 + or *oa / 255 * (255 - na) / 255;
		int g = ng * na / 255 + og * oa / 255 * (255 - na) / 255;
		int b = nb * na / 255 + ob * oa / 255 * (255 - na) / 255;

		if (a > 0)
		{
		    bdata[bi]     = (uint8_t) (r & 0xFF);
		    bdata[bi + 1] = (uint8_t) (g & 0xFF);
		    bdata[bi + 2] = (uint8_t) (b & 0xFF);
		    bdata[bi + 3] = (uint8_t) (a & 0xFF);
		}
	    }
	}
    }
}

void ku_draw_insert(ku_bitmap_t* base, ku_bitmap_t* src, int sx, int sy)
{
    int bx = sx + src->w;
    if (bx > base->w) bx = base->w;
    int by = sy + src->h;
    if (by > base->h) by = base->h;

    uint8_t* sdata = src->data;  // src data
    uint8_t* bdata = base->data; // base data

    for (int y = sy; y < by; y++)
    {
	for (int x = sx; x < bx; x++)
	{
	    if (x > -1 && y > -1)
	    {
		int si = ((y - sy) * src->w + (x - sx)) * 4; // src index
		int bi = (y * base->w + x) * 4;              // base index

		uint8_t r = sdata[si];
		uint8_t g = sdata[si + 1];
		uint8_t b = sdata[si + 2];
		uint8_t a = sdata[si + 3];

		bdata[bi]     = r;
		bdata[bi + 1] = g;
		bdata[bi + 2] = b;
		bdata[bi + 3] = a;
	    }
	}
    }
}

void ku_draw_insert_rgb(ku_bitmap_t* base, uint8_t* src, int w, int h, int sx, int sy)
{
    int bx = sx + w;
    if (bx > base->w) bx = base->w;
    int by = sy + h;
    if (by > base->h) by = base->h;

    uint8_t* sdata = src;        // src data
    uint8_t* bdata = base->data; // base data

    for (int y = sy; y < by; y++)
    {
	for (int x = sx; x < bx; x++)
	{
	    if (x > -1 && y > -1)
	    {
		int si = ((y - sy) * w + (x - sx)) * 4; // src index
		int bi = (y * base->w + x) * 4;         // base index

		uint8_t r = sdata[si];
		uint8_t g = sdata[si + 1];
		uint8_t b = sdata[si + 2];

		bdata[bi]     = r;
		bdata[bi + 1] = g;
		bdata[bi + 2] = b;
	    }
	}
    }
}

void ku_draw_insert_argb(ku_bitmap_t* base, uint8_t* src, int w, int h, int sx, int sy)
{
    int bx = sx + w;
    if (bx > base->w)
	bx = base->w;
    int by = sy + h;
    if (by > base->h)
	by = base->h;

    uint8_t* sdata = src;        // src data
    uint8_t* bdata = base->data; // base data

    for (int y = sy; y < by; y++)
    {
	for (int x = sx; x < bx; x++)
	{
	    if (x > -1 && y > -1)
	    {
		int si = ((y - sy) * w + (x - sx)) * 4; // src index
		int bi = (y * base->w + x) * 4;         // base index

		uint8_t r = sdata[si];
		uint8_t g = sdata[si + 1];
		uint8_t b = sdata[si + 2];
		uint8_t a = sdata[si + 3];

		bdata[bi]     = r;
		bdata[bi + 1] = g;
		bdata[bi + 2] = b;
		bdata[bi + 3] = a;
	    }
	}
    }
}

void ku_draw_insert_rgba(ku_bitmap_t* base, uint8_t* src, int w, int h, int sx, int sy)
{
    int bx = sx + w;
    if (bx > base->w)
	bx = base->w;
    int by = sy + h;
    if (by > base->h)
	by = base->h;

    uint8_t* sdata = src;        // src data
    uint8_t* bdata = base->data; // base data

    for (int y = sy; y < by; y++)
    {
	for (int x = sx; x < bx; x++)
	{
	    if (x > -1 && y > -1)
	    {
		int si = ((y - sy) * w + (x - sx)) * 4; // src index
		int bi = (y * base->w + x) * 4;         // base index

		uint8_t a = sdata[si];
		uint8_t r = sdata[si + 1];
		uint8_t g = sdata[si + 2];
		uint8_t b = sdata[si + 3];

		bdata[bi]     = r;
		bdata[bi + 1] = g;
		bdata[bi + 2] = b;
		bdata[bi + 3] = a;
	    }
	}
    }
}

void ku_draw_insert_bitmap(ku_bitmap_t* base, ku_bitmap_t* src, int sx, int sy)
{
    if (sx < 0)
	sx = 0;
    if (sy < 0)
	sy = 0;
    if (sx >= base->w)
	return;
    if (sy >= base->h)
	return;

    int ex = sx + src->w;
    int ey = sy + src->h;

    if (ex >= base->w)
	ex = base->w;
    if (ey >= base->h)
	ey = base->h;

    uint32_t* b = (uint32_t*) base->data;
    uint32_t* s = (uint32_t*) src->data;
    uint32_t  l = (ex - sx) * 4; // length

    b += (sy * base->w) + sx;

    for (int y = sy; y < ey; y++)
    {
	memcpy(b, s, l);

	b += base->w;
	s += src->w;
    }
}

void ku_draw_blend_pixel(ku_bitmap_t* bm, int x, int y, uint32_t color)
{
    // TODO this shouldn't return, it should do a partial blend
    if (x > bm->w) return;
    if (y > bm->h) return;

    uint8_t* data = bm->data;
    int      i    = (y * bm->w + x) * 4;

    int dr = data[i];
    int dg = data[i + 1];
    int db = data[i + 2];
    int da = data[i + 3];

    int sr = (color >> 24) & 0xFF;
    int sg = (color >> 16) & 0xFF;
    int sb = (color >> 8) & 0xFF;
    int sa = color & 0xFF;

    int a = sa + da * (255 - sa) / 255;
    int r = (sr * sa / 255 + dr * da / 255 * (255 - sa) / 255);
    int g = (sg * sa / 255 + dg * da / 255 * (255 - sa) / 255);
    int b = (sb * sa / 255 + db * da / 255 * (255 - sa) / 255);

    if (a > 0)
    {
	r = r * 255 / a;
	g = g * 255 / a;
	b = b * 255 / a;
    }

    data[i]     = (uint8_t) (r & 0xFF);
    data[i + 1] = (uint8_t) (g & 0xFF);
    data[i + 2] = (uint8_t) (b & 0xFF);
    data[i + 3] = (uint8_t) (a & 0xFF);
}

void ku_draw_blend_bitmap(ku_bitmap_t* bm, ku_bitmap_t* sbm, int sx, int sy)
{
    // TODO this shouldn't return, it should do a partial blend
    if (sx + sbm->w > bm->w) return;
    if (sy + sbm->h > bm->h) return;

    uint8_t* data  = bm->data;
    uint8_t* sdata = sbm->data;

    for (int x = sx; x < sx + sbm->w; x++)
    {
	for (int y = sy; y < sy + sbm->h; y++)
	{
	    if (x > -1 && y > -1)
	    {
		int si = ((y - sy) * sbm->w + (x - sx)) * 4;
		int i  = (y * bm->w + x) * 4;

		int sr = sdata[si];
		int sg = sdata[si + 1];
		int sb = sdata[si + 2];
		int sa = sdata[si + 3];

		if (sa == 255)
		{
		    data[i]     = (uint8_t) (sr & 0xFF);
		    data[i + 1] = (uint8_t) (sg & 0xFF);
		    data[i + 2] = (uint8_t) (sb & 0xFF);
		    data[i + 3] = (uint8_t) (sa & 0xFF);
		}
		else
		{
		    int dr = data[i];
		    int dg = data[i + 1];
		    int db = data[i + 2];
		    int da = data[i + 3];

		    int a = sa + da * (255 - sa) / 255;
		    int r = (sr * sa / 255 + dr * da / 255 * (255 - sa) / 255);
		    int g = (sg * sa / 255 + dg * da / 255 * (255 - sa) / 255);
		    int b = (sb * sa / 255 + db * da / 255 * (255 - sa) / 255);

		    if (a > 0)
		    {
			r = r * 255 / a;
			g = g * 255 / a;
			b = b * 255 / a;
		    }

		    data[i]     = (uint8_t) (r & 0xFF);
		    data[i + 1] = (uint8_t) (g & 0xFF);
		    data[i + 2] = (uint8_t) (b & 0xFF);
		    data[i + 3] = (uint8_t) (a & 0xFF);
		}
	    }
	}
    }
}

void ku_draw_blend_8(ku_bitmap_t* bm, int nx, int ny, uint32_t color, unsigned char* ndata, int nw, int nh)
{
    int ex = nx + nw;
    if (ex > bm->w - 2) ex = bm->w - 2;
    int ey = ny + nh;
    if (ey > bm->h - 2) ey = bm->h - 2;

    int nr = (color >> 24) & 0xFF;
    int ng = (color >> 16) & 0xFF;
    int nb = (color >> 8) & 0xFF;

    uint8_t* odata = bm->data;

    for (int y = ny; y < ey; y++)
    {
	for (int x = nx; x < ex; x++)
	{
	    if (x > -1 && y > -1)
	    {
		int ni = (y - ny) * nw + (x - nx); // new map index
		int oi = (y * bm->w + x) * 4;      // old map index

		int or = odata[oi];
		int og = odata[oi + 1];
		int ob = odata[oi + 2];
		int oa = odata[oi + 3];

		unsigned char na = ndata[ni];

		int a = na + oa * (255 - na) / 255;
		int r = nr * na / 255 + or *oa / 255 * (255 - na) / 255;
		int g = ng * na / 255 + og * oa / 255 * (255 - na) / 255;
		int b = nb * na / 255 + ob * oa / 255 * (255 - na) / 255;

		if (a > 0)
		{
		    r = r * 255 / a;
		    g = g * 255 / a;
		    b = b * 255 / a;
		}

		odata[oi]     = (uint8_t) (r & 0xFF);
		odata[oi + 1] = (uint8_t) (g & 0xFF);
		odata[oi + 2] = (uint8_t) (b & 0xFF);
		odata[oi + 3] = (uint8_t) (a & 0xFF);
	    }
	}
    }
}

void ku_draw_blend_8_1(ku_bitmap_t* bm, int nx, int ny, uint32_t color, unsigned char* ndata, int nw, int nh)
{
    int ex = nx + nw;
    if (ex > bm->w) ex = bm->w;
    int ey = ny + nh;
    if (ey > bm->h) ey = bm->h;

    int nr = (color >> 24) & 0xFF;
    int ng = (color >> 16) & 0xFF;
    int nb = (color >> 8) & 0xFF;

    uint8_t* odata = bm->data;

    for (int y = ny; y < ey; y++)
    {
	for (int x = nx; x < ex; x++)
	{
	    if (x > -1 && y > -1)
	    {
		int ni = (y - ny) * nw + (x - nx); // new map index
		int oi = (y * bm->w + x) * 4;      // old map index

		int or = odata[oi];
		int og = odata[oi + 1];
		int ob = odata[oi + 2];
		int oa = odata[oi + 3];

		unsigned char na = ndata[ni];
		if (na) na = 0xFF;

		int a = na + oa * (255 - na) / 255;
		int r = nr * na / 255 + or *oa / 255 * (255 - na) / 255;
		int g = ng * na / 255 + og * oa / 255 * (255 - na) / 255;
		int b = nb * na / 255 + ob * oa / 255 * (255 - na) / 255;

		if (a > 0)
		{
		    r = r * 255 / a;
		    g = g * 255 / a;
		    b = b * 255 / a;
		}

		odata[oi]     = (uint8_t) (r & 0xFF);
		odata[oi + 1] = (uint8_t) (g & 0xFF);
		odata[oi + 2] = (uint8_t) (b & 0xFF);
		odata[oi + 3] = (uint8_t) (a & 0xFF);
	    }
	}
    }
}

#include <stdint.h>
typedef struct
{
    uint32_t*    pixels;
    unsigned int w;
    unsigned int h;
} image_t;

#define getByte(value, n) (value >> (n * 8) & 0xFF)

uint32_t getpixel(image_t* image, unsigned int x, unsigned int y)
{
    return image->pixels[(y * image->w) + x];
}

float max(float a, float b)
{
    return (a < b) ? a : b;
}

float lerp(float s, float e, float t)
{
    return s + (e - s) * t;
}
float blerp(float c00, float c10, float c01, float c11, float tx, float ty)
{
    return lerp(lerp(c00, c10, tx), lerp(c01, c11, tx), ty);
}
void putpixel(image_t* image, unsigned int x, unsigned int y, uint32_t color)
{
    image->pixels[(y * image->w) + x] = color;
}

void ku_draw_scale(ku_bitmap_t* srcbmp, ku_bitmap_t* dstbmp)
{
    image_t srci =
	{
	    .pixels = (uint32_t*) srcbmp->data,
	    .w      = srcbmp->w,
	    .h      = srcbmp->h};

    image_t dsti =
	{
	    .pixels = (uint32_t*) dstbmp->data,
	    .w      = dstbmp->w,
	    .h      = dstbmp->h

	};

    image_t* src = &srci;
    image_t* dst = &dsti;

    int newWidth  = dst->w;
    int newHeight = dst->h;
    int x, y;
    for (x = 0, y = 0; y < newHeight - 1; x++)
    {
	if (x == newWidth)
	{
	    x = 0;
	    y++;
	}
	// float gx = x / (float)(newWidth) * (src->w - 1);
	// float gy = y / (float)(newHeight) * (src->h - 1);
	//  Image should be clamped at the edges and not scaled.
	float    gx     = max(x / (float) (newWidth) * (src->w) - 0.5f, src->w - 2);
	float    gy     = max(y / (float) (newHeight) * (src->h) - 0.5, src->h - 2);
	int      gxi    = (int) gx;
	int      gyi    = (int) gy;
	
	uint32_t result = 0;
	uint32_t c00    = getpixel(src, gxi, gyi);
	uint32_t c10    = getpixel(src, gxi + 1, gyi);
	uint32_t c01    = getpixel(src, gxi, gyi + 1);
	uint32_t c11    = getpixel(src, gxi + 1, gyi + 1);
	uint8_t  i;
	for (i = 0; i < 4; i++)
	{
	    //((uint8_t*)&result)[i] = blerp( ((uint8_t*)&c00)[i], ((uint8_t*)&c10)[i], ((uint8_t*)&c01)[i], ((uint8_t*)&c11)[i], gxi - gx, gyi - gy); // this is shady
	    result |= (uint8_t) blerp(getByte(c00, i), getByte(c10, i), getByte(c01, i), getByte(c11, i), gx - gxi, gy - gyi) << (8 * i);
	}

	putpixel(dst, x, y, result);
    }
}

#endif
