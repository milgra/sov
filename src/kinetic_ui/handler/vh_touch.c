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

int vh_touch_evt(ku_view_t* view, ku_event_t ev)
{
    if (ev.type == KU_EVENT_MOUSE_DOWN)
    {
	vh_touch_t*      vh    = view->evt_han_data;
	vh_touch_event_t event = {.id = VH_TOUCH_EVENT, .vh = vh, .view = view};
	if (vh->on_event)
	    (*vh->on_event)(event);
    }

    return 0;
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
    assert(view->evt_han == NULL && view->evt_han_data == NULL);

    vh_touch_t* vh = CAL(sizeof(vh_touch_t), vh_touch_del, vh_touch_desc);
    vh->on_event   = on_event;

    view->evt_han      = vh_touch_evt;
    view->evt_han_data = vh;
}

#endif
