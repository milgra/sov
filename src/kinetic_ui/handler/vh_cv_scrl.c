#ifndef vh_cv_scrl_h
#define vh_cv_scrl_h

#include "ku_view.c"
#include "vh_cv_body.c"

typedef struct _vh_cv_scrl_t
{
    ku_view_t* tbody_view;
    ku_view_t* vert_v;
    ku_view_t* hori_v;
    int        state; // 0 scroll 1 open 2 close
    int        steps;
    size_t     item_cnt;
    void*      userdata;
} vh_cv_scrl_t;

void vh_cv_scrl_attach(
    ku_view_t* view,
    ku_view_t* tbody_view,
    void*      userdata);

void vh_cv_scrl_update(ku_view_t* view);
void vh_cv_scrl_show(ku_view_t* view);
void vh_cv_scrl_hide(ku_view_t* view);
void vh_cv_scrl_set_item_count(ku_view_t* view, size_t count);
void vh_cv_scrl_scroll_v(ku_view_t* view, int y);
void vh_cv_scrl_scroll_h(ku_view_t* view, int x);

#endif

#if __INCLUDE_LEVEL__ == 0

void vh_cv_scrl_del(void* p)
{
    vh_cv_scrl_t* vh = p;
    REL(vh->vert_v);
    REL(vh->hori_v);
}

void vh_cv_scrl_desc(void* p, int level)
{
    printf("vh_cv_scrl");
}

void vh_cv_scrl_attach(
    ku_view_t* view,
    ku_view_t* tbody_view,
    void*      userdata)
{
    vh_cv_scrl_t* vh = CAL(sizeof(vh_cv_scrl_t), vh_cv_scrl_del, vh_cv_scrl_desc);
    vh->userdata     = userdata;
    vh->tbody_view   = tbody_view;

    assert(view->views->length > 1);

    vh->vert_v = RET(view->views->data[0]);
    vh->hori_v = RET(view->views->data[1]);

    ku_view_set_texture_alpha(vh->hori_v, 0.0, 0);
    ku_view_set_texture_alpha(vh->vert_v, 0.0, 0);

    view->evt_han_data = vh;
}

void vh_cv_scrl_set_item_count(ku_view_t* view, size_t count)
{
    vh_cv_scrl_t* vh = view->evt_han_data;

    vh->item_cnt = count;
}

void vh_cv_scrl_update(ku_view_t* view)
{
    vh_cv_scrl_t* vh  = view->evt_han_data;
    vh_cv_body_t* bvh = vh->tbody_view->evt_han_data;

    ku_rect_t vf = view->frame.local;
    ku_rect_t cf = bvh->content->frame.local;

    float pratio = -cf.y / cf.h;
    float sratio = cf.h / vf.h;

    if (sratio > 1.0)
    {
	float hth = (vf.h - vh->hori_v->frame.local.h) * (1 / sratio);
	float pos = (vf.h - vh->hori_v->frame.local.h) * pratio;

	if (vh->state == 2)
	{
	    pos += hth / 2.0;
	    hth = 1.0;
	}

	ku_rect_t frame = vh->vert_v->frame.local;
	frame.h += (hth - frame.h) / 2.0;
	frame.y += (pos - frame.y) / 2.0;

	ku_view_set_frame(vh->vert_v, frame);
    }

    pratio = -cf.x / cf.w;
    sratio = cf.w / vf.w;

    if (sratio > 1.0)
    {
	float wth = (vf.w - vh->vert_v->frame.local.w) * (1 / sratio);
	float pos = (vf.w - vh->vert_v->frame.local.w) * pratio;

	if (vh->state == 2)
	{
	    pos += wth / 2.0;
	    wth = 1.0;
	}

	ku_rect_t frame = vh->hori_v->frame.local;
	frame.w += (wth - frame.w) / 2.0;
	frame.x += (pos - frame.x) / 2.0;

	ku_view_set_frame(vh->hori_v, frame);
    }

    if (vh->state > 0)
    {
	vh->steps += 1;
	if (vh->steps == 5)
	{
	    if (vh->state == 2)
	    {
		ku_view_set_texture_alpha(vh->hori_v, 0.0, 0);
		ku_view_set_texture_alpha(vh->vert_v, 0.0, 0);
	    }
	    vh->state = 0;
	}
    }
}

void vh_cv_scrl_show(ku_view_t* view)
{
    vh_cv_scrl_t* vh = view->evt_han_data;

    vh->state = 1;
    vh->steps = 0;
    ku_view_set_texture_alpha(vh->hori_v, 1.0, 0);
    ku_view_set_texture_alpha(vh->vert_v, 1.0, 0);
}

void vh_cv_scrl_hide(ku_view_t* view)
{
    vh_cv_scrl_t* vh = view->evt_han_data;
    vh->state        = 2;
    vh->steps        = 0;
}

void vh_cv_scrl_scroll_v(ku_view_t* view, int y)
{
    /* vh_cv_scrl_t* vh  = view->evt_han_data; */
    /* vh_cv_body_t* bvh = vh->tbody_view->evt_han_data; */

    /* if (bvh->items->length > 0 && vh->item_cnt > 0) */
    /* { */
    /* 	// int vert_pos = bvh->top_index; */
    /* 	int vert_vis = bvh->bot_index - bvh->top_index; */
    /* 	int vert_max = vh->item_cnt; */

    /* 	if (vert_max > 1) */
    /* 	{ */
    /* 	    float sratio = (float) vert_vis / (float) vert_max; */
    /* 	    float height = (view->frame.local.h - view->frame.local.h * sratio); */
    /* 	    float pratio = (float) y / height; */

    /* 	    if (pratio < 0.0) pratio = 0.0; */
    /* 	    if (pratio > 1.0) pratio = 1.0; */
    /* 	    int topindex = pratio * (vert_max - vert_vis); */

    /* 	    vh_cv_body_vjump(vh->tbody_view, topindex); */

    /* 	    ku_rect_t frame = vh->vert_v->frame.local; */
    /* 	    frame.h    = view->frame.local.h * sratio; */
    /* 	    frame.y    = y; */

    /* 	    ku_view_set_frame(vh->vert_v, frame); */
    /* 	} */
    /* } */
}

void vh_cv_scrl_scroll_h(ku_view_t* view, int x)
{
    /* vh_cv_scrl_t* vh  = view->evt_han_data; */
    /* vh_cv_body_t* bvh = vh->tbody_view->evt_han_data; */

    /* if (bvh->items->length > 0 && vh->item_cnt > 0) */
    /* { */
    /* 	ku_view_t* head = bvh->items->data[0]; */

    /* 	// float hori_pos = -head->frame.local.x; */
    /* 	float hori_vis = view->frame.local.w; */
    /* 	float hori_max = head->frame.local.w; */

    /* 	if (hori_max > 1.0) */
    /* 	{ */
    /* 	    float sratio = (float) hori_vis / (float) hori_max; */
    /* 	    float width  = (view->frame.local.w - view->frame.local.w * sratio); */
    /* 	    float pratio = (float) x / width; */

    /* 	    if (pratio < 0.0) pratio = 0.0; */
    /* 	    if (pratio > 1.0) pratio = 1.0; */
    /* 	    float dx = pratio * (hori_max - hori_vis); */

    /* 	    vh_cv_body_hjump(vh->tbody_view, -dx); */
    /* 	    vh_cv_head_jump(vh->thead_view, -dx); */

    /* 	    ku_rect_t frame = vh->hori_v->frame.local; */
    /* 	    frame.w    = view->frame.local.w * sratio; */
    /* 	    frame.x    = x; */

    /* 	    ku_view_set_frame(vh->hori_v, frame); */
    /* 	} */
    /* } */
}

#endif
