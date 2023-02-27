/* content view event */

#ifndef vh_cv_evnt_h
#define vh_cv_evnt_h

#include "ku_view.c"
#include "vh_cv_body.c"
#include "vh_cv_scrl.c"

enum vh_cv_evnt_event_id
{
    VH_CV_EVENT_RESIZE,
    VH_CV_EVENT_CLICK,
    VH_CV_EVENT_KEY_DOWN,
    VH_CV_EVENT_KEY_UP
};

typedef struct _vh_cv_evnt_event_t
{
    enum vh_cv_evnt_event_id id;
    ku_view_t*               view;
    ku_event_t               ev;
} vh_cv_evnt_event_t;

typedef struct _vh_cv_evnt_t
{
    ku_view_t* tbody_view;
    ku_view_t* tscrl_view;
    void*      userdata;
    int        scroll_drag;
    int        scroll_visible;
    float      sx;
    float      sy;
    float      mx;
    float      my;
    float      zoom;
    void (*on_event)(vh_cv_evnt_event_t event);
} vh_cv_evnt_t;

void vh_cv_evnt_attach(
    ku_view_t* view,
    ku_view_t* tbody_view,
    ku_view_t* tscrl_view,
    void*      userdata,
    void (*on_event)(vh_cv_evnt_event_t));

void vh_cv_evnt_zoom(ku_view_t* view, float delta);

#endif

#if __INCLUDE_LEVEL__ == 0

#define SCROLLBAR 20.0

int vh_cv_evnt_evt(ku_view_t* view, ku_event_t ev)
{
    vh_cv_evnt_t* vh = view->evt_han_data;

    if (ev.type == KU_EVENT_FRAME)
    {
	vh_cv_body_t* bvh = vh->tbody_view->evt_han_data;
	ku_rect_t     cf  = bvh->content->frame.local;

	vh->sx *= 0.8;
	vh->sy *= 0.8;

	float dx = vh->sx;
	float dy = vh->sy;

	float top = cf.y;
	float bot = cf.y + cf.h;
	float wth = cf.w;
	float hth = cf.h;
	float lft = cf.x;
	float rgt = cf.x + cf.w;

	if (hth >= view->frame.local.h)
	{
	    if (top > 0.001)
		dy -= top / 5.0; // scroll back top item
	    if (bot < view->frame.local.h - 0.001)
	    {
		dy += (view->frame.local.h - bot) / 5.0; // scroll back bottom item
	    }
	}
	else
	{
	    vh->my = view->frame.global.y + view->frame.global.h / 2.0;
	    dy     = ((view->frame.local.h - hth) / 2.0 - top) / 2.0;
	}

	if (wth >= view->frame.local.w)
	{
	    if (lft > 0.01)
		dx -= lft / 5.0;
	    if (rgt < view->frame.local.w - 0.01)
	    {
		dx += (view->frame.local.w - rgt) / 5.0;
	    }
	}
	else
	{
	    vh->mx = view->frame.global.x + view->frame.global.w / 2.0;
	    dx     = ((view->frame.local.w - wth) / 2.0 - lft) / 2.0;
	}

	vh_cv_body_move(vh->tbody_view, dx, dy);

	if (vh->zoom > 1.0001 || vh->zoom < 0.9999)
	{
	    vh->zoom += (1.0 - vh->zoom) / 5.0;

	    float zoom = bvh->zoom * vh->zoom;

	    vh_cv_body_zoom(vh->tbody_view, zoom, vh->mx, vh->my);
	    vh_cv_evnt_event_t event = {.id = VH_CV_EVENT_RESIZE};
	    if (vh->on_event)
		(*vh->on_event)(event);
	}

	if (vh->tscrl_view && vh->scroll_visible)
	    vh_cv_scrl_update(vh->tscrl_view);

	vh_cv_scrl_t* svh = vh->tscrl_view->evt_han_data;

	if (svh->state > 0)
	    vh_cv_scrl_update(vh->tscrl_view);
    }
    else if (ev.type == KU_EVENT_SCROLL)
    {
	if (!ev.ctrl_down)
	{
	    vh->sx -= ev.dx;
	    vh->sy += ev.dy;
	}
	else
	{
	    vh->zoom -= (ev.dy - 1.0);
	}
	// cause dirty rect which causes frame events to flow for later animation
	vh->tbody_view->frame.dim_changed = 1;
    }
    else if (ev.type == KU_EVENT_PINCH)
    {
	vh->zoom += ev.ratio;
	// cause dirty rect which causes frame events to flow for later animation
	vh->tbody_view->frame.dim_changed = 1;
    }
    else if (ev.type == KU_EVENT_RESIZE)
    {
    }
    else if (ev.type == KU_EVENT_MOUSE_MOVE)
    {
	// show scroll
	if (!vh->scroll_visible)
	{
	    vh->scroll_visible = 1;
	    vh_cv_scrl_show(vh->tscrl_view);
	}
	if (vh->scroll_drag)
	{
	    if (ev.x > view->frame.global.x + view->frame.global.w - SCROLLBAR)
	    {
		vh_cv_scrl_scroll_v(vh->tscrl_view, ev.y - view->frame.global.y);
	    }
	    if (ev.y > view->frame.global.y + view->frame.global.h - SCROLLBAR)
	    {
		vh_cv_scrl_scroll_h(vh->tscrl_view, ev.x - view->frame.global.x);
	    }
	}

	vh->mx = ev.x;
	vh->my = ev.y;
    }
    else if (ev.type == KU_EVENT_MOUSE_MOVE_OUT)
    {
	// hide scroll
	if (vh->scroll_visible)
	{
	    vh->scroll_visible = 0;
	    vh_cv_scrl_hide(vh->tscrl_view);
	}
    }
    else if (ev.type == KU_EVENT_MOUSE_DOWN)
    {
	if (ev.x > view->frame.global.x + view->frame.global.w - SCROLLBAR)
	{
	    vh->scroll_drag = 1;
	    vh_cv_scrl_scroll_v(vh->tscrl_view, ev.y - view->frame.global.y);
	}
	if (ev.y > view->frame.global.y + view->frame.global.h - SCROLLBAR)
	{
	    vh->scroll_drag = 1;
	    vh_cv_scrl_scroll_h(vh->tscrl_view, ev.x - view->frame.global.x);
	}
	vh_cv_evnt_event_t event = {.id = VH_CV_EVENT_CLICK};
	if (vh->on_event)
	    (*vh->on_event)(event);
    }
    else if (ev.type == KU_EVENT_MOUSE_UP)
    {
	vh->scroll_drag = 0;
    }
    else if (ev.type == KU_EVENT_FOCUS)
    {
	if (vh->tscrl_view)
	    vh_cv_scrl_show(vh->tscrl_view);
    }
    else if (ev.type == KU_EVENT_UNFOCUS)
    {
	if (vh->tscrl_view)
	    vh_cv_scrl_hide(vh->tscrl_view);
    }
    else if (ev.type == KU_EVENT_KEY_DOWN)
    {
	vh_cv_evnt_event_t event = {.id = VH_CV_EVENT_KEY_DOWN, .view = view, .ev = ev};
	if (vh->on_event)
	    (*vh->on_event)(event);
    }
    else if (ev.type == KU_EVENT_KEY_UP)
    {
	vh_cv_evnt_event_t event = {.id = VH_CV_EVENT_KEY_UP, .view = view, .ev = ev};
	if (vh->on_event)
	    (*vh->on_event)(event);
    }

    return 0;
}

void vh_cv_evnt_del(void* p)
{
}

void vh_cv_evnt_desc(void* p, int level)
{
    printf("vh_cv_evnt");
}

void vh_cv_evnt_attach(
    ku_view_t* view,
    ku_view_t* tbody_view,
    ku_view_t* tscrl_view,
    void*      userdata,
    void (*on_event)(vh_cv_evnt_event_t))
{
    assert(view->evt_han == NULL && view->evt_han_data == NULL);

    vh_cv_evnt_t* vh = CAL(sizeof(vh_cv_evnt_t), vh_cv_evnt_del, vh_cv_evnt_desc);
    vh->userdata     = userdata;
    vh->tbody_view   = tbody_view;
    vh->tscrl_view   = tscrl_view;
    vh->on_event     = on_event;
    vh->zoom         = 1.0;

    view->evt_han_data = vh;
    view->evt_han      = vh_cv_evnt_evt;
}

void vh_cv_evnt_zoom(ku_view_t* view, float delta)
{
    vh_cv_evnt_t* vh = view->evt_han_data;

    vh->zoom += delta;
    vh->tbody_view->frame.dim_changed = 1;
}

#endif
