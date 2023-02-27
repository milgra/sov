/*
  CSS texture generator
  Generates texture based on css style
 */

#ifndef texgen_css_h
#define texgen_css_h

#include "ku_bitmap.c"
#include "ku_view.c"

typedef struct _tg_css_t
{
    char*        id;
    char*        path;
    ku_bitmap_t* bitmap;
} tg_bitmap_t;

void tg_css_add(ku_view_t* view);

#endif

#if __INCLUDE_LEVEL__ == 0

#include "ku_draw.c"
#include "ku_png.c"
#include "mt_log.c"
#include "mt_string.c"

int tg_css_gen(ku_view_t* view)
{
    if (view->frame.local.w >= 1.0 &&
	view->frame.local.h >= 1.0)
    {
	if (strlen(view->style.background_image) > 0)
	{
	    ku_bitmap_t* bm = view->texture.bitmap;

	    if (bm == NULL ||
		bm->w != (int) view->frame.local.w ||
		bm->h != (int) view->frame.local.h)
	    {
		bm = ku_bitmap_new((int) view->frame.local.w, (int) view->frame.local.h); // REL 0
		ku_view_set_texture_bmp(view, bm);
		REL(bm);
	    }

	    ku_png_load_into(view->style.background_image, bm);

	    view->texture.transparent = 1;
	    view->texture.changed     = 0;
	    view->texture.ready       = 1;
	}
	else if (view->style.background_color)
	{
	    uint32_t color = view->style.background_color;

	    if ((color & 0xFF) < 0xFF || view->style.shadow_blur > 0 || view->style.border_radius > 0 || view->style.border_color > 0)
		view->texture.transparent = 1;
	    else
		view->texture.transparent = 0;

	    float w = view->frame.local.w + 2 * view->style.shadow_blur;
	    float h = view->frame.local.h + 2 * view->style.shadow_blur;

	    ku_bitmap_t* bm = view->texture.bitmap;

	    if (bm == NULL ||
		bm->w != (int) w ||
		bm->h != (int) h)
	    {
		bm = ku_bitmap_new((int) w, (int) h); // REL 0
		ku_view_set_texture_bmp(view, bm);
		REL(bm);
	    }

	    ku_bitmap_reset(bm);

	    if (color > 0)
		ku_draw_rounded_rect(bm, 0, 0, w, h, view->style.border_radius, view->style.shadow_blur, color, view->style.shadow_color);

	    if (view->style.border_width > 0)
	    {
		ku_draw_border(
		    bm,
		    0 + view->style.shadow_blur,
		    0 + view->style.shadow_blur,
		    w - 2 * view->style.shadow_blur,
		    h - 2 * view->style.shadow_blur,
		    view->style.border_radius,
		    view->style.border_width,
		    view->style.border_color);
	    }

	    view->texture.changed = 1;
	    view->texture.ready   = 1;
	}
    }

    return 1;
}

void tg_css_add(ku_view_t* view)
{
    if (view->tex_gen != NULL)
	mt_log_debug("Text generator already exist for view, cannot create a new one : %s", view->id);
    else
    {
	view->tex_gen = tg_css_gen;
    }
}

#endif
