#ifndef vh_touch_h
#define vh_touch_h

#include "ku_view.c"

typedef struct _vh_touch_t vh_touch_t;

enum vh_touch_event_id
{
    VH_TOUCH_EVENT
};

typedef struct _vh_touch_event_t
{
    enum vh_touch_event_id id;
    vh_touch_t*            vh;
    ku_view_t*             view;
} vh_touch_event_t;

struct _vh_touch_t
{
    void (*on_event)(vh_touch_event_t);
};

void vh_touch_add(ku_view_t* view, void (*on_event)(vh_touch_event_t));

#endif

#if __INCLUDE_LEVEL__ == 0

#include "ku_event.c"

void vh_touch_evt(ku_view_t* view, ku_event_t ev)
{
    if (ev.type == KU_EVENT_MDOWN)
    {
	vh_touch_t*      vh    = view->handler_data;
	vh_touch_event_t event = {.id = VH_TOUCH_EVENT, .vh = vh, .view = view};
	if (vh->on_event) (*vh->on_event)(event);
    }
}

void vh_touch_del(void* p)
{
    /* vh_touch_t* vh = p; */
}

void vh_touch_desc(void* p, int level)
{
    printf("vh_touch");
}

void vh_touch_add(ku_view_t* view, void (*on_event)(vh_touch_event_t))
{
    assert(view->handler == NULL && view->handler_data == NULL);

    vh_touch_t* vh = CAL(sizeof(vh_touch_t), vh_touch_del, vh_touch_desc);
    vh->on_event   = on_event;

    view->handler      = vh_touch_evt;
    view->handler_data = vh;

    view->needs_touch = 1;
}

#endif
