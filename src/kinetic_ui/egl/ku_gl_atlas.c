#ifndef _ku_gl_atlas_h
#define _ku_gl_atlas_h

#include "mt_map.c"
#include "mt_matrix_4d.c"
#include "mt_vector_4d.c"

typedef struct _ku_gl_atlas_coords_t ku_gl_atlas_coords_t;
struct _ku_gl_atlas_coords_t
{
    float ltx;
    float lty;
    float rbx;
    float rby;
    int   x;
    int   y;
    int   w;
    int   h;
};

typedef struct _ku_gl_atlas_t ku_gl_atlas_t;
struct _ku_gl_atlas_t
{
    mt_map_t* coords;
    char*     blocks;
    char      is_full;
    int       width;
    int       height;
    int       cols;
    int       rows;
};

ku_gl_atlas_t*       ku_gl_atlas_new(int w, int h);
void                 ku_gl_atlas_del(void* p);
char                 ku_gl_atlas_has(ku_gl_atlas_t* tm, char* id);
ku_gl_atlas_coords_t ku_gl_atlas_get(ku_gl_atlas_t* tm, char* id);
int                  ku_gl_atlas_put(ku_gl_atlas_t* tm, char* id, int w, int h);

#endif

#if __INCLUDE_LEVEL__ == 0

#include "mt_memory.c"
#include <math.h>

void ku_gl_atlas_desc(void* p, int level)
{
    ku_gl_atlas_t* tm = p;
    printf("tm w %i h %i", tm->width, tm->height);
}

void ku_gl_atlas_desc_blocks(void* p, int level)
{
    printf("tm blocks\n");
}

ku_gl_atlas_t* ku_gl_atlas_new(int w, int h)
{
    int cols = w / 32;
    int rows = h / 32;

    ku_gl_atlas_t* tm = CAL(sizeof(ku_gl_atlas_t), ku_gl_atlas_del, ku_gl_atlas_desc);
    tm->coords        = mt_map_new();
    tm->blocks        = CAL(sizeof(char) * cols * rows, NULL, ku_gl_atlas_desc_blocks);
    tm->width         = w;
    tm->height        = h;
    tm->cols          = cols;
    tm->rows          = rows;

    return tm;
}

void ku_gl_atlas_del(void* p)
{
    ku_gl_atlas_t* tm = (ku_gl_atlas_t*) p;
    REL(tm->coords);
    REL(tm->blocks);
}

char ku_gl_atlas_has(ku_gl_atlas_t* tm, char* id)
{
    v4_t* coords = mt_map_get(tm->coords, id);
    if (coords) return 1;
    return 0;
}

ku_gl_atlas_coords_t ku_gl_atlas_get(ku_gl_atlas_t* tm, char* id)
{
    ku_gl_atlas_coords_t* coords = mt_map_get(tm->coords, id);
    if (coords) return *coords;
    return (ku_gl_atlas_coords_t){0};
}

int ku_gl_atlas_put(ku_gl_atlas_t* tm, char* id, int w, int h)
{
    if (w > tm->width || h > tm->height) return -1; // too big bitmap

    // get size of incoming rect
    int sx = ceil((float) w / 32.0);
    int sy = ceil((float) h / 32.0);

    int r = 0; // row
    int c = 0; // col
    int s = 0; // success

    for (r = 0; r < tm->rows; r++)
    {
	for (c = 0; c < tm->cols; c++)
	{
	    int i = r * tm->cols + c;

	    // if block is free, check if width and height of new rect fits

	    if (tm->blocks[i] == 0)
	    {
		s = 1; // assume success
		if (c + sx < tm->cols)
		{
		    for (int tc = c; tc < c + sx; tc++)
		    {
			int ti = r * tm->cols + tc; // test index
			if (tm->blocks[ti] == 1)
			{
			    s = 0; // if block is occupied test is failed
			    break;
			}
		    }
		}
		else
		    s = 0; // doesn't fit

		if (r + sy < tm->rows && s == 1)
		{
		    for (int tr = r; tr < r + sy; tr++)
		    {
			int ti = tr * tm->cols + c; // test index
			if (tm->blocks[ti] == 1)
			{
			    s = 0; // if block is occupied test is failed
			    break;
			}
		    }
		}
		else
		    s = 0; // doesn't fit
	    }
	    if (s == 1) break;
	}
	if (s == 1) break;
    }

    if (s == 1)
    {
	// flip blocks to occupied
	for (int nr = r; nr < r + sy; nr++)
	{
	    for (int nc = c; nc < c + sx; nc++)
	    {
		int ni         = nr * tm->cols + nc;
		tm->blocks[ni] = 1;
	    }
	}

	int ncx = c * 32;
	int ncy = r * 32;
	int rbx = ncx + w;
	int rby = ncy + h;

	ku_gl_atlas_coords_t* coords = HEAP(
	    ((ku_gl_atlas_coords_t){
		.ltx = (float) ncx / (float) tm->width,
		.lty = (float) ncy / (float) tm->height,
		.rbx = (float) rbx / (float) tm->width,
		.rby = (float) rby / (float) tm->height,
		.x   = ncx,
		.y   = ncy,
		.w   = w,
		.h   = h}));

	MPUTR(tm->coords, id, coords);
    }
    else
	return -2; // texmap is full

    return 0; // success
}

#endif
