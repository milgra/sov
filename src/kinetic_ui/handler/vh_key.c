#ifndef vh_key_h
#define vh_key_h

#include "ku_view.c"

typedef struct _vh_key_t vh_key_t;

typedef struct _vh_key_event_t
{
    ku_event_t ev;
    vh_key_t*  vh;
    ku_view_t* view;
} vh_key_event_t;

struct _vh_key_t
{
    void (*on_event)(vh_key_event_t event);
};

void vh_key_add(ku_view_t* view, void (*on_event)(vh_key_event_t));

#endif

#if __INCLUDE_LEVEL__ == 0

void vh_key_evt(ku_view_t* view, ku_event_t ev)
{
    if (ev.type == KU_EVENT_KDOWN)
    {
	vh_key_t*      vh    = view->handler_data;
	vh_key_event_t event = {.ev = ev, .vh = vh, .view = view};
	if (vh->on_event) (*vh->on_event)(event);
    }
}

void vh_key_del(void* p)
{
}

void vh_key_desc(void* p, int level)
{
    printf("vh_key");
}

void vh_key_add(ku_view_t* view, void (*on_event)(vh_key_event_t))
{
    assert(view->handler == NULL && view->handler_data == NULL);

    vh_key_t* vh = CAL(sizeof(vh_key_t), vh_key_del, vh_key_desc);
    vh->on_event = on_event;

    view->needs_key    = 1;
    view->handler_data = vh;
    view->handler      = vh_key_evt;
}

#endif
