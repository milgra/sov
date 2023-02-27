
#ifndef texgen_knob_h
#define texgen_knob_h

#include "ku_view.c"

typedef struct _tg_knob_t
{
    float        angle;
    ku_bitmap_t* back;
    ku_bitmap_t* fore;
} tg_knob_t;

void tg_knob_add(ku_view_t* view);
void tg_knob_set_angle(ku_view_t* view, float angle);

#endif

#if __INCLUDE_LEVEL__ == 0

#include "ku_draw.c"

int tg_knob_gen(ku_view_t* view)
{
    tg_knob_t* tg = view->tex_gen_data;

    if (view->frame.local.w > 0 && view->frame.local.h > 0)
    {

	float dist0 = 5 * view->style.scale;
	float dist1 = 27 * view->style.scale;
	float dist8 = 28 * view->style.scale;
	float dist2 = 31 * view->style.scale;
	float dist3 = 35 * view->style.scale;

	if (view->texture.bitmap == NULL && view->frame.local.w > 0 && view->frame.local.h > 0)
	{
	    ku_bitmap_t* bmp = ku_bitmap_new(view->frame.local.w, view->frame.local.h); // REL 0
	    tg->back         = ku_bitmap_new(view->frame.local.w, view->frame.local.h); // REL 1
	    tg->fore         = ku_bitmap_new(view->frame.local.w, view->frame.local.h); // REL 2

	    uint32_t basecol   = 0x454545FF;
	    uint32_t outercol  = 0x343434FF;
	    uint32_t centercol = 0x676767FF;
	    uint32_t shadowcol = 0xABABAB0A;

	    /* ku_draw_arc_grad(tg->back, */
	    /*              (view->frame.local.w - 1.0) / 2.0, */
	    /*              (view->frame.local.h - 1.0) / 2.0, */
	    /*              (view->frame.local.w / 2.0) - 3.0, */
	    /*              (view->frame.local.w / 2.0), */
	    /*              0, */
	    /*              3.14 * 2, */
	    /*              0x00000044, */
	    /*              0); */

	    ku_draw_arc_grad(tg->back, (view->frame.local.w - 1.0) / 2.0, (view->frame.local.h - 1.0) / 2.0, 0, (view->frame.local.w / 2.0) - dist0, 0, 3.14 * 2, basecol, basecol);
	    ku_draw_arc_grad(tg->back, (view->frame.local.w - 1.0) / 2.0, (view->frame.local.h - 1.0) / 2.0, dist1, dist3, 0, 3.14 * 2, outercol, outercol);
	    ku_draw_arc_grad(tg->back, (view->frame.local.w - 1.0) / 2.0, (view->frame.local.h - 1.0) / 2.0, (view->frame.local.w / 2.0) - dist0, (view->frame.local.w / 2.0) - 2.0, 0, 3.14 * 2, shadowcol, 0x00000000);

	    ku_draw_arc_grad(tg->fore, (view->frame.local.w - 1.0) / 2.0, (view->frame.local.h - 1.0) / 2.0, dist1, dist2, 0, 3.14 * 2, shadowcol, 0);
	    ku_draw_arc_grad(tg->fore, (view->frame.local.w - 1.0) / 2.0, (view->frame.local.h - 1.0) / 2.0, 0, dist8, 0, 3.14 * 2, centercol, centercol);
	    ku_view_set_texture_bmp(view, bmp);

	    REL(bmp); // REL 0
	}

	if (tg->angle < 0)
	    tg->angle += 6.28;

	ku_draw_insert(view->texture.bitmap, tg->back, 0, 0);

	if (tg->angle > 3.14 * 3 / 2)
	{
	    ku_draw_arc_grad(view->texture.bitmap, (view->frame.local.w - 1.0) / 2.0, (view->frame.local.h - 1.0) / 2.0, dist1, dist3, 3.14 * 3 / 2, tg->angle, 0x999999FF, 0x999999FF);
	}
	else
	{
	    ku_draw_arc_grad(view->texture.bitmap, (view->frame.local.w - 1.0) / 2.0, (view->frame.local.h - 1.0) / 2.0, dist1, dist3, 3.14 * 3 / 2, 6.28, 0x999999FF, 0x999999FF);
	    ku_draw_arc_grad(view->texture.bitmap, (view->frame.local.w - 1.0) / 2.0, (view->frame.local.h - 1.0) / 2.0, dist1, dist3, 0, tg->angle, 0x999999FF, 0x999999FF);
	}

	ku_draw_blend_argb(view->texture.bitmap, 0, 0, tg->fore);
	view->texture.changed = 1;
	view->texture.ready   = 1;
    }

    return 1;
}

void tg_knob_del(void* p)
{
    tg_knob_t* tg = p;
    if (tg->back) REL(tg->back);
    if (tg->fore) REL(tg->fore);
}

void tg_knob_desc(void* p, int level)
{
    printf("tg_knob");
}

void tg_knob_add(ku_view_t* view)
{
    assert(view->tex_gen == NULL);

    tg_knob_t* tg = CAL(sizeof(tg_knob_t), tg_knob_del, tg_knob_desc);
    tg->angle     = 3 * 3.14 / 2;

    view->tex_gen_data = tg;
    view->tex_gen      = tg_knob_gen;
}

void tg_knob_set_angle(ku_view_t* view, float angle)
{
    tg_knob_t* tg = view->tex_gen_data;

    tg->angle           = angle;
    view->texture.ready = 0; // force rerender
}

#endif
