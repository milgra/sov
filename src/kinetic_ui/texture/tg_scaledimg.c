/*
  CSS texture generator
  Generates texture based on css style
 */

#ifndef texgen_scaledimg_h
#define texgen_scaledimg_h

#include "ku_bitmap.c"
#include "ku_view.c"

typedef struct _tg_scaledimg_t
{
    int w;
    int h;
} tg_scaledimg_t;

void tg_scaledimg_add(ku_view_t* view, int w, int h);
int  tg_scaledimg_gen(ku_view_t* view);
void tg_scaledimg_set_content_size(ku_view_t* view, int w, int h);

#endif

#if __INCLUDE_LEVEL__ == 0

#include "ku_draw.c"
#include "mt_string.c"

int tg_scaledimg_gen(ku_view_t* view)
{
    tg_scaledimg_t* gen = view->tex_gen_data;
    ku_bitmap_t*    bm  = view->texture.bitmap;

    /* just resize texture bitmap with the view */

    if (bm == NULL ||
	bm->w != (int) gen->w ||
	bm->h != (int) gen->h)
    {
	bm = ku_bitmap_new_aligned((int) gen->w, (int) gen->h, 16); // REL 0

	ku_view_set_texture_bmp(view, bm);

	ku_draw_rect(bm, 0, 0, bm->w, bm->h, 0x00000000, 0);

	REL(bm);
    }

    view->texture.ready = 1;

    return 1;
}

void tg_scaledimg_set_content_size(ku_view_t* view, int w, int h)
{
    tg_scaledimg_t* gen = view->tex_gen_data;

    gen->w = w;
    gen->h = h;
}

void tg_scaledimg_add(ku_view_t* view, int w, int h)
{
    assert(view->tex_gen == NULL);

    tg_scaledimg_t* gen = CAL(sizeof(tg_scaledimg_t), NULL, NULL);
    gen->w              = w;
    gen->h              = h;

    view->tex_gen_data = gen;
    view->tex_gen      = tg_scaledimg_gen;
}

#endif
