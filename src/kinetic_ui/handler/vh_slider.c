#ifndef vh_slider_h
#define vh_slider_h

#include "ku_view.c"

typedef struct _vh_slider_t vh_slider_t;

typedef struct _vh_slider_event_t
{
    vh_slider_t* vh;
    ku_view_t*   view;
    float        ratio;
} vh_slider_event_t;

struct _vh_slider_t
{
    float ratio;
    char  enabled;
    void (*on_event)(vh_slider_event_t);
};

void  vh_slider_add(ku_view_t* view, void (*on_event)(vh_slider_event_t));
void  vh_slider_set(ku_view_t* view, float ratio);
float vh_slider_get_ratio(ku_view_t* view);
void  vh_slider_set_enabled(ku_view_t* view, int flag);

#endif

#if __INCLUDE_LEVEL__ == 0

#include "vh_anim.c"
#include <stdio.h>

void vh_slider_evt(ku_view_t* view, ku_event_t ev)
{
    vh_slider_t* vh = view->handler_data;

    if (vh->enabled)
    {
	if (ev.type == KU_EVENT_MDOWN || (ev.type == KU_EVENT_MMOVE && ev.drag))
	{
	    float dx  = ev.x - view->frame.global.x;
	    vh->ratio = dx / view->frame.global.w;

	    ku_view_t* bar   = view->views->data[0];
	    ku_rect_t  frame = bar->frame.local;
	    frame.w          = dx;
	    ku_view_set_frame(bar, frame);

	    vh_slider_event_t event = {.view = view, .ratio = vh->ratio, .vh = vh};
	    if (vh->on_event) (*vh->on_event)(event);
	}
	else if (ev.type == KU_EVENT_SCROLL)
	{
	    float ratio = vh->ratio - ev.dx / 50.0;

	    if (ratio < 0) ratio = 0;
	    if (ratio > 1) ratio = 1;

	    vh->ratio        = ratio;
	    ku_view_t* bar   = view->views->data[0];
	    ku_rect_t  frame = bar->frame.local;
	    frame.w          = view->frame.global.w * vh->ratio;
	    ku_view_set_frame(bar, frame);

	    vh_slider_event_t event = {.view = view, .ratio = vh->ratio, .vh = vh};
	    if (vh->on_event) (*vh->on_event)(event);
	}
    }
}

void vh_slider_set(ku_view_t* view, float ratio)
{
    vh_slider_t* vh  = view->handler_data;
    vh->ratio        = ratio;
    ku_view_t* bar   = view->views->data[0];
    ku_rect_t  frame = bar->frame.local;
    frame.w          = view->frame.global.w * vh->ratio;
    ku_view_set_frame(bar, frame);
}

float vh_slider_get_ratio(ku_view_t* view)
{
    vh_slider_t* vh = view->handler_data;
    return vh->ratio;
}

void vh_slider_desc(void* p, int level)
{
    printf("vh_slider");
}

void vh_slider_del(void* p)
{
}

void vh_slider_add(ku_view_t* view, void (*on_event)(vh_slider_event_t))
{
    assert(view->handler == NULL && view->handler_data == NULL);

    vh_slider_t* vh = CAL(sizeof(vh_slider_t), vh_slider_del, vh_slider_desc);
    vh->on_event    = on_event;
    vh->enabled     = 1;

    view->handler_data = vh;
    view->handler      = vh_slider_evt;

    ku_view_t* bar = view->views->data[0];
    vh_anim_add(bar, NULL, NULL);
}

void vh_slider_set_enabled(ku_view_t* view, int flag)
{
    vh_slider_t* vh = view->handler_data;

    if (flag)
    {
	if (vh->enabled == 0)
	{
	    ku_view_t* bar = view->views->data[0];
	    vh_anim_alpha(bar, 0.3, 1.0, 10, AT_LINEAR);
	}
    }
    else
    {
	if (vh->enabled == 1)
	{
	    ku_view_t* bar = view->views->data[0];
	    vh_anim_alpha(bar, 1.0, 0.3, 10, AT_LINEAR);
	}
    }

    vh->enabled = flag;
}

#endif
