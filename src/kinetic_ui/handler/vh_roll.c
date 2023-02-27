#ifndef vh_roll_h
#define vh_roll_h

#include "ku_event.c"
#include "ku_view.c"

enum vh_roll_event_id
{
    VH_ROLL_IN,
    VH_ROLL_OUT
};

typedef struct _vh_roll_t vh_roll_t;

typedef struct _vh_roll_event_t
{
    enum vh_roll_event_id id;
    vh_roll_t*            vh;
    ku_view_t*            view;
} vh_roll_event_t;

struct _vh_roll_t
{
    char active;
    void (*on_event)(vh_roll_event_t);
};

void vh_roll_add(ku_view_t* view, void (*on_event)(vh_roll_event_t));

#endif

#if __INCLUDE_LEVEL__ == 0

int vh_roll_evt(ku_view_t* view, ku_event_t ev)
{
    if (ev.type == KU_EVENT_MOUSE_MOVE)
    {
	vh_roll_t* vh    = view->evt_han_data;
	ku_rect_t  frame = view->frame.global;

	if (!vh->active)
	{
	    if (ev.x >= frame.x &&
		ev.x <= frame.x + frame.w &&
		ev.y >= frame.y &&
		ev.y <= frame.y + frame.h)
	    {
		vh->active = 1;

		vh_roll_event_t event = {.id = VH_ROLL_IN, .view = view, .vh = vh};
		if (vh->on_event)
		    (*vh->on_event)(event);
	    }
	}
    }
    else if (ev.type == KU_EVENT_MOUSE_MOVE_OUT)
    {
	vh_roll_t* vh    = view->evt_han_data;
	ku_rect_t  frame = view->frame.global;

	if (vh->active)
	{
	    if (ev.x < frame.x ||
		ev.x > frame.x + frame.w ||
		ev.y < frame.y ||
		ev.y > frame.y + frame.h)
	    {
		vh->active            = 0;
		vh_roll_event_t event = {.id = VH_ROLL_OUT, .view = view, .vh = vh};
		if (vh->on_event)
		    (*vh->on_event)(event);
	    }
	}
    }

    return 1;
}

void vh_roll_del(void* p)
{
}

void vh_roll_desc(void* p, int level)
{
    printf("vh_roll");
}

void vh_roll_add(ku_view_t* view, void (*on_event)(vh_roll_event_t))
{
    assert(view->evt_han == NULL && view->evt_han_data == NULL);

    vh_roll_t* vh = CAL(sizeof(vh_roll_t), vh_roll_del, vh_roll_desc);
    vh->on_event  = on_event;

    view->evt_han_data = vh;
    view->evt_han      = vh_roll_evt;
}

#endif
