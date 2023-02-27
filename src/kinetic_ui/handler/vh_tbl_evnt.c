#ifndef vh_tbl_evnt_h
#define vh_tbl_evnt_h

#include "ku_view.c"
#include "mt_log.c"
#include "vh_tbl_body.c"
#include "vh_tbl_head.c"
#include "vh_tbl_scrl.c"
#include <limits.h>

enum vh_tbl_evnt_event_id
{
    VH_TBL_EVENT_CONTEXT,
    VH_TBL_EVENT_SELECT,
    VH_TBL_EVENT_OPEN,
    VH_TBL_EVENT_DRAG,
    VH_TBL_EVENT_DROP,
    VH_TBL_EVENT_KEY_DOWN,
    VH_TBL_EVENT_KEY_UP,
};

typedef struct _vh_tbl_evnt_t       vh_tbl_evnt_t;
typedef struct _vh_tbl_evnt_event_t vh_tbl_evnt_event_t;

struct _vh_tbl_evnt_event_t
{
    enum vh_tbl_evnt_event_id id;
    ku_view_t*                view;
    ku_view_t*                rowview;
    int                       index;
    ku_event_t                ev;
    void*                     userdata;
};

struct _vh_tbl_evnt_t
{
    ku_view_t* tbody_view;
    ku_view_t* tscrl_view;
    ku_view_t* thead_view;
    void*      userdata;
    ku_view_t* selected_item;
    int        selected_index;
    int        inertia_x;
    int        inertia_y;
    float      sx;
    float      sy;
    void (*on_event)(vh_tbl_evnt_event_t event);
};

void vh_tbl_evnt_attach(
    ku_view_t* view,
    ku_view_t* tbody_view,
    ku_view_t* tscrl_view,
    ku_view_t* thead_view,
    void (*on_event)(vh_tbl_evnt_event_t event),
    void* userdata);

#endif

#if __INCLUDE_LEVEL__ == 0

#define SCROLLBAR 20.0

float linear_slowdown(float speed, float pos, float max)
{
    if (speed > 0.0)
    {
	pos = pos > max ? max : pos;
	speed *= 1.0 - pos / max;
    }

    return speed;
}

void vh_tbl_evnt_move(ku_view_t* view)
{
    vh_tbl_evnt_t* vh  = view->evt_han_data;
    vh_tbl_body_t* bvh = vh->tbody_view->evt_han_data;

    if (bvh->items && bvh->items->length > 0)
    {
	/* if (vh->sy > 0.0001 || vh->sy < -0.0001 || vh->sx > 0.0001 || vh->sx < -0.0001) */
	/* { */
	ku_view_t* head = mt_vector_head(bvh->items);
	ku_view_t* tail = mt_vector_tail(bvh->items);

	float top = head->frame.local.y;
	float bot = tail->frame.local.y + tail->frame.local.h;
	float hth = bot - top;
	float wth = head->frame.local.w;
	float lft = head->frame.local.x;
	float rgt = head->frame.local.x + head->frame.local.w;

	float dx = vh->sx;
	float dy = vh->sy;

	float top_gap = 0.001;
	float bot_gap = view->frame.local.h - SCROLLBAR - 0.001;

	if (top > top_gap)
	{
	    vh->sy = linear_slowdown(vh->sy, top, 200.0);
	    dy     = vh->sy;
	    dy += -top / 8.0;
	}
	else if (bot < bot_gap)
	{
	    if (dy < 0.0)
	    {
		if (hth > bot_gap) // scroll back to bottom item
		{
		    vh->sy = -1 * linear_slowdown(-vh->sy, view->frame.local.h - bot, 200.0);
		    dy     = vh->sy;
		    dy += (bot_gap - bot) / 8.0;
		}
		else // scroll back to top item
		{
		    vh->sy = -1 * linear_slowdown(-vh->sy, -top, 200.0);
		    dy     = vh->sy;
		    dy += -top / 8.0;
		}
	    }
	}

	float lft_gap = 0.001;
	float rgt_gap = view->frame.local.w - SCROLLBAR - 0.001;

	if (lft > lft_gap)
	{
	    vh->sx = linear_slowdown(vh->sx, lft, 200.0);
	    dx     = vh->sx;
	    dx += -lft / 8.0;
	}
	else if (rgt < rgt_gap)
	{
	    if (dx < 0)
	    {
		if (wth > rgt_gap) // scroll back to right edge
		{
		    vh->sx = -1 * linear_slowdown(-vh->sx, view->frame.local.w - rgt, 200.0);
		    dx     = vh->sx;
		    dx += (rgt_gap - rgt) / 8.0;
		}
		else // scroll back to left edge
		{
		    vh->sx = -1 * linear_slowdown(-vh->sx, -lft, 200.0);
		    dx     = vh->sx;
		    dx += -lft / 8.0;
		}
	    }
	}

	vh_tbl_body_move(vh->tbody_view, dx, dy);

	if (vh->thead_view)
	    vh_tbl_head_move(vh->thead_view, dx);
	if (vh->tscrl_view)
	    vh_tbl_scrl_update(vh->tscrl_view);
	//}

	if (vh->tscrl_view)
	{
	    vh_tbl_scrl_t* svh = vh->tscrl_view->evt_han_data;
	    if (svh->state > 0)
		vh_tbl_scrl_update(vh->tscrl_view);
	}
    }
}

int vh_tbl_evnt_evt(ku_view_t* view, ku_event_t ev)
{
    vh_tbl_evnt_t* vh = view->evt_han_data;

    if (ev.type == KU_EVENT_FRAME)
    {
	if (vh->inertia_x || vh->inertia_y)
	{
	    vh->sx *= 0.99;
	    vh->sy *= 0.99;
	    vh_tbl_evnt_move(view);
	}
    }
    else if (ev.type == KU_EVENT_SCROLL)
    {
	vh->sx = -ev.dx;
	vh->sy = ev.dy;
	vh_tbl_evnt_move(view);
	// cause dirty rect which causes frame events to flow for later animation
	vh->tbody_view->frame.dim_changed = 1;
    }
    else if (ev.type == KU_EVENT_SCROLL_X_END)
    {
	if (fabs(vh->sx) > 2.0)
	{
	    vh->sx *= 1.5;
	    vh->inertia_x = 1;
	    // cause dirty rect which causes frame events to flow for later animation
	    vh->tbody_view->frame.dim_changed = 1;
	}
    }
    else if (ev.type == KU_EVENT_SCROLL_Y_END)
    {
	if (fabs(vh->sy) > 2.0)
	{
	    vh->sy *= 1.5;
	    vh->inertia_y = 1;
	    // cause dirty rect which causes frame events to flow for later animation
	    vh->tbody_view->frame.dim_changed = 1;
	}
    }
    else if (ev.type == KU_EVENT_RESIZE)
    {
	vh_tbl_body_move(vh->tbody_view, 0, 0);
    }
    else if (ev.type == KU_EVENT_MOUSE_MOVE)
    {
	if (vh->selected_item && ev.drag)
	{
	    vh_tbl_evnt_event_t event = {.id = VH_TBL_EVENT_DRAG, .view = view, .rowview = vh->selected_item, .index = 0, .ev = ev, .userdata = vh->userdata};
	    if (vh->on_event)
		(*vh->on_event)(event);

	    vh->selected_item = NULL;
	}
    }
    else if (ev.type == KU_EVENT_MOUSE_DOWN)
    {
	if (ev.x < view->frame.global.x + view->frame.global.w - SCROLLBAR &&
	    ev.y < view->frame.global.y + view->frame.global.h - SCROLLBAR)
	{
	    vh_tbl_body_t* bvh = vh->tbody_view->evt_han_data;

	    ku_view_t* context_item  = NULL;
	    int        context_index = -1;

	    for (size_t index = 0; index < bvh->items->length; index++)
	    {
		ku_view_t* item = bvh->items->data[index];
		if (ev.x > item->frame.global.x &&
		    ev.x < item->frame.global.x + item->frame.global.w &&
		    ev.y > item->frame.global.y &&
		    ev.y < item->frame.global.y + item->frame.global.h)
		{
		    vh->selected_item  = item;
		    vh->selected_index = bvh->head_index + index;

		    if (!ev.dclick)
		    {
			if (ev.button == 1)
			{
			    vh_tbl_evnt_event_t event = {
				.id       = VH_TBL_EVENT_SELECT,
				.view     = view,
				.rowview  = vh->selected_item,
				.index    = vh->selected_index,
				.ev       = ev,
				.userdata = vh->userdata};
			    if (vh->on_event)
				(*vh->on_event)(event);
			}
			if (ev.button == 3)
			{
			    context_item  = vh->selected_item;
			    context_index = vh->selected_index;
			}
		    }
		    else
		    {
			vh_tbl_evnt_event_t event =
			    {
				.id       = VH_TBL_EVENT_OPEN,
				.view     = view,
				.rowview  = vh->selected_item,
				.index    = vh->selected_index,
				.ev       = ev,
				.userdata = vh->userdata};
			if (vh->on_event)
			    (*vh->on_event)(event);
		    }

		    break;
		}
	    }
	    if (ev.button == 3)
	    {
		vh_tbl_evnt_event_t event = {
		    .id       = VH_TBL_EVENT_CONTEXT,
		    .view     = view,
		    .rowview  = context_item,
		    .index    = context_index,
		    .ev       = ev,
		    .userdata = vh->userdata};
		if (vh->on_event)
		    (*vh->on_event)(event);
	    }
	}
    }
    else if (ev.type == KU_EVENT_MOUSE_UP)
    {
	if (ev.drag)
	{
	    if (!vh->selected_item)
	    {
		vh_tbl_body_t* bvh = vh->tbody_view->evt_han_data;

		size_t index = 0;
		// ku_view_t* item  = NULL;
		for (index = 0; index < bvh->items->length; index++)
		{
		    ku_view_t* item = bvh->items->data[index];
		    if (ev.x > item->frame.global.x &&
			ev.x < item->frame.global.x + item->frame.global.w &&
			ev.y > item->frame.global.y &&
			ev.y < item->frame.global.y + item->frame.global.h)
		    {
			break;
		    }
		}

		vh_tbl_evnt_event_t event = {.id = VH_TBL_EVENT_DROP, .view = view, .rowview = vh->selected_item, .index = bvh->head_index + index, .ev = ev, .userdata = vh->userdata};
		if (vh->on_event)
		    (*vh->on_event)(event);
	    }
	}
    }
    else if (ev.type == KU_EVENT_KEY_DOWN)
    {
	vh_tbl_evnt_event_t event = {.id = VH_TBL_EVENT_KEY_DOWN, .view = view, .rowview = vh->selected_item, .index = 0, .ev = ev, .userdata = vh->userdata};
	if (vh->on_event)
	    (*vh->on_event)(event);
    }
    else if (ev.type == KU_EVENT_KEY_UP)
    {
	vh_tbl_evnt_event_t event = {.id = VH_TBL_EVENT_KEY_UP, .view = view, .rowview = vh->selected_item, .index = 0, .ev = ev, .userdata = vh->userdata};
	if (vh->on_event)
	    (*vh->on_event)(event);
    }
    else if (ev.type == KU_EVENT_FOCUS)
    {
	if (vh->tscrl_view)
	    vh_tbl_scrl_show(vh->tscrl_view);
    }
    else if (ev.type == KU_EVENT_UNFOCUS)
    {
	if (vh->tscrl_view)
	    vh_tbl_scrl_hide(vh->tscrl_view);
    }
    else if (ev.type == KU_EVENT_HOLD_START)
    {
	vh->inertia_x = 0;
	vh->inertia_y = 0;
    }

    /* TODO refactor ku_window's key and text handling */
    if (ev.type == KU_EVENT_KEY_DOWN || ev.type == KU_EVENT_KEY_UP)
	return 0;
    else
	return (vh->tscrl_view != NULL);
}

void vh_tbl_evnt_del(void* p)
{
}

void vh_tbl_evnt_desc(void* p, int level)
{
    printf("vh_tbl_evnt");
}

void vh_tbl_evnt_attach(
    ku_view_t* view,
    ku_view_t* tbody_view,
    ku_view_t* tscrl_view,
    ku_view_t* thead_view,
    void (*on_event)(vh_tbl_evnt_event_t event),
    void* userdata)
{
    assert(view->evt_han == NULL && view->evt_han_data == NULL);

    vh_tbl_evnt_t* vh = CAL(sizeof(vh_tbl_evnt_t), vh_tbl_evnt_del, vh_tbl_evnt_desc);
    vh->on_event      = on_event;
    vh->userdata      = userdata;
    vh->tbody_view    = tbody_view;
    vh->tscrl_view    = tscrl_view;
    vh->thead_view    = thead_view;

    view->evt_han_data = vh;
    view->evt_han      = vh_tbl_evnt_evt;
}

#endif
