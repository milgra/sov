#ifndef vh_tbl_scrl_h
#define vh_tbl_scrl_h

#include "ku_view.c"
#include "vh_tbl_body.c"
#include "vh_tbl_head.c"

typedef struct _vh_tbl_scrl_t
{
    ku_view_t* tbody_view;
    ku_view_t* thead_view;
    ku_view_t* vert_v;
    ku_view_t* hori_v;
    int        state; // 0 scroll 1 open 2 close
    int        steps;
    uint32_t   item_cnt;
    void*      userdata;
    int        enabled;
} vh_tbl_scrl_t;

void vh_tbl_scrl_attach(
    ku_view_t* view,
    ku_view_t* tbody_view,
    ku_view_t* thead_view,
    void*      userdata);

void vh_tbl_scrl_update(ku_view_t* view);
void vh_tbl_scrl_show(ku_view_t* view);
void vh_tbl_scrl_hide(ku_view_t* view);
void vh_tbl_scrl_set_item_count(ku_view_t* view, uint32_t count);
void vh_tbl_scrl_scroll_v(ku_view_t* view, int y);
void vh_tbl_scrl_scroll_h(ku_view_t* view, int x);
void vh_tbl_scrl_enable(ku_view_t* view, int flag);

#endif

#if __INCLUDE_LEVEL__ == 0

void vh_tbl_scrl_del(void* p)
{
    vh_tbl_scrl_t* vh = p;
    REL(vh->vert_v);
    REL(vh->hori_v);
}

void vh_tbl_scrl_desc(void* p, int level)
{
    printf("vh_tbl_scrl");
}

void vh_tbl_scrl_attach(
    ku_view_t* view,
    ku_view_t* tbody_view,
    ku_view_t* thead_view,
    void*      userdata)
{
    vh_tbl_scrl_t* vh = CAL(sizeof(vh_tbl_scrl_t), vh_tbl_scrl_del, vh_tbl_scrl_desc);
    vh->userdata      = userdata;
    vh->tbody_view    = tbody_view;
    vh->thead_view    = thead_view;
    vh->enabled       = 1;

    assert(view->views->length > 1);

    vh->vert_v = RET(view->views->data[0]);
    vh->hori_v = RET(view->views->data[1]);

    ku_view_set_texture_alpha(vh->hori_v, 0.0, 0);
    ku_view_set_texture_alpha(vh->vert_v, 0.0, 0);

    view->handler_data = vh;
}

void vh_tbl_scrl_set_item_count(ku_view_t* view, uint32_t count)
{
    vh_tbl_scrl_t* vh = view->handler_data;

    vh->item_cnt = count;
}

void vh_tbl_scrl_update(ku_view_t* view)
{
    vh_tbl_scrl_t* vh  = view->handler_data;
    vh_tbl_body_t* bvh = vh->tbody_view->handler_data;

    if (bvh->items->length > 0 && vh->item_cnt > 0)
    {
	ku_view_t* head = mt_vector_head(bvh->items);
	ku_view_t* tail = mt_vector_tail(bvh->items);

	int vert_pos = bvh->top_index;
	int vert_vis = bvh->bot_index - bvh->top_index + 1;
	int vert_max = vh->item_cnt;

	if (vert_max > 1.0)
	{
	    float pratio = (float) vert_pos / (float) vert_max;
	    float sratio = (float) vert_vis / (float) vert_max;

	    float height = tail->frame.local.y + tail->frame.local.h - head->frame.local.y;
	    if (height < view->frame.local.h)
	    {
		pratio = 0.0;
		sratio = 1.0;
	    }

	    float pos = (view->frame.local.h - vh->hori_v->frame.local.h) * pratio;
	    float hth = (view->frame.local.h - vh->hori_v->frame.local.h) * sratio;

	    if (hth < 30.0) hth = 30.0;

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

	float hori_pos = -head->frame.local.x;
	float hori_vis = view->frame.local.w;
	float hori_max = head->frame.local.w;

	if (hori_max > 1.0)
	{
	    float pratio = (float) hori_pos / (float) hori_max;
	    float sratio = (float) hori_vis / (float) hori_max;

	    float width = head->frame.local.w;
	    if (width < view->frame.local.w)
	    {
		pratio = 0.0;
		sratio = 1.0;
	    }

	    float pos = view->frame.local.w * pratio;
	    float wth = view->frame.local.w * sratio;

	    if (wth < 30.0) wth = 30.0;

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
}

void vh_tbl_scrl_show(ku_view_t* view)
{
    vh_tbl_scrl_t* vh  = view->handler_data;
    vh_tbl_body_t* bvh = vh->tbody_view->handler_data;

    if (vh->enabled)
    {

	if (bvh->items->length > 0 && vh->item_cnt > 0)
	{
	    vh->state = 1;
	    vh->steps = 0;
	    ku_view_set_texture_alpha(vh->hori_v, 1.0, 0);
	    ku_view_set_texture_alpha(vh->vert_v, 1.0, 0);
	}
    }
}

void vh_tbl_scrl_hide(ku_view_t* view)
{
    vh_tbl_scrl_t* vh = view->handler_data;

    if (vh->enabled)
    {
	vh->state = 2;
	vh->steps = 0;
	/* make rects dirty for possible frame requests */
	vh->hori_v->frame.dim_changed = 1;
	vh->vert_v->frame.dim_changed = 1;
    }
}

void vh_tbl_scrl_scroll_v(ku_view_t* view, int y)
{
    vh_tbl_scrl_t* vh  = view->handler_data;
    vh_tbl_body_t* bvh = vh->tbody_view->handler_data;

    if (bvh->items->length > 0 && vh->item_cnt > 0)
    {
	// int vert_pos = bvh->top_index;
	int vert_vis = bvh->bot_index - bvh->top_index;
	int vert_max = vh->item_cnt;

	if (vert_max > 1)
	{
	    float sratio = (float) vert_vis / (float) vert_max;
	    float height = (view->frame.local.h - view->frame.local.h * sratio);
	    float pratio = (float) y / height;

	    if (pratio < 0.0) pratio = 0.0;
	    if (pratio > 1.0) pratio = 1.0;
	    int topindex = pratio * (vert_max - vert_vis);

	    vh_tbl_body_vjump(vh->tbody_view, topindex, 1);

	    ku_rect_t frame = vh->vert_v->frame.local;
	    frame.h         = view->frame.local.h * sratio;
	    frame.y         = y;

	    ku_view_set_frame(vh->vert_v, frame);
	}
    }
}

void vh_tbl_scrl_scroll_h(ku_view_t* view, int x)
{
    vh_tbl_scrl_t* vh  = view->handler_data;
    vh_tbl_body_t* bvh = vh->tbody_view->handler_data;

    if (bvh->items->length > 0 && vh->item_cnt > 0)
    {
	ku_view_t* head = bvh->items->data[0];

	// float hori_pos = -head->frame.local.x;
	float hori_vis = view->frame.local.w;
	float hori_max = head->frame.local.w;

	if (hori_max > 1.0)
	{
	    float sratio = (float) hori_vis / (float) hori_max;
	    float width  = (view->frame.local.w - view->frame.local.w * sratio);
	    float pratio = (float) x / width;

	    if (pratio < 0.0) pratio = 0.0;
	    if (pratio > 1.0) pratio = 1.0;
	    float dx = pratio * (hori_max - hori_vis);

	    vh_tbl_body_hjump(vh->tbody_view, -dx);

	    if (vh->thead_view) vh_tbl_head_jump(vh->thead_view, -dx);

	    ku_rect_t frame = vh->hori_v->frame.local;
	    frame.w         = view->frame.local.w * sratio;
	    frame.x         = x;

	    ku_view_set_frame(vh->hori_v, frame);
	}
    }
}

void vh_tbl_scrl_enable(ku_view_t* view, int flag)
{
    vh_tbl_scrl_t* vh = view->handler_data;
    vh->enabled       = flag;
}

#endif
