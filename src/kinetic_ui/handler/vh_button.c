#ifndef vh_button_h
#define vh_button_h

#include "ku_view.c"

typedef enum _vh_button_type_t
{
    VH_BUTTON_NORMAL,
    VH_BUTTON_TOGGLE
} vh_button_type_t;

typedef enum _vh_button_state_t
{
    VH_BUTTON_UP,
    VH_BUTTON_DOWN
} vh_button_state_t;

typedef struct _vh_button_t vh_button_t;

enum vh_button_event_id
{
    VH_BUTTON_EVENT
};

typedef struct _vh_button_event_t
{
    enum vh_button_event_id id;
    vh_button_t*            vh;
    ku_view_t*              view;
    ku_event_t              ev;
} vh_button_event_t;

struct _vh_button_t
{
    void (*on_event)(vh_button_event_t);
    vh_button_type_t  type;
    vh_button_state_t state;

    ku_view_t* offview;
    ku_view_t* onview;
    char       enabled;
};

void vh_button_add(ku_view_t* view, vh_button_type_t type, void (*on_event)(vh_button_event_t));
void vh_button_set_state(ku_view_t* view, vh_button_state_t state);
void vh_button_set_enabled(ku_view_t* view, int flag);

#endif

#if __INCLUDE_LEVEL__ == 0

#include "vh_anim.c"

void vh_button_on_anim(vh_anim_event_t event)
{
    ku_view_t*   btnview = (ku_view_t*) event.userdata;
    vh_button_t* vh      = btnview->evt_han_data;

    // if offview alpha is 0 and state is released

    if (vh->type == VH_BUTTON_NORMAL)
    {
	if (vh->offview->texture.alpha < 0.0001 && event.view == vh->offview)
	{
	    vh_anim_alpha(vh->offview, 0.0, 1.0, 10, AT_LINEAR);
	}
    }
}

int vh_button_evt(ku_view_t* view, ku_event_t ev)
{
    vh_button_t* vh = view->evt_han_data;

    if (vh->enabled)
    {
	if (ev.type == KU_EVENT_MOUSE_DOWN)
	{
	    if (vh->type == VH_BUTTON_NORMAL)
	    {
		vh->state = VH_BUTTON_DOWN;

		if (vh->offview)
		    vh_anim_alpha(vh->offview, 1.0, 0.0, 10, AT_LINEAR);
		if (vh->onview)
		    vh_anim_alpha(vh->onview, 0.0, 1.0, 10, AT_LINEAR);
	    }
	}
	else if (ev.type == KU_EVENT_MOUSE_UP)
	{
	    if (vh->type == VH_BUTTON_TOGGLE)
	    {
		if (vh->state == VH_BUTTON_UP)
		{
		    vh->state = VH_BUTTON_DOWN;
		    if (vh->offview)
			vh_anim_alpha(vh->offview, 1.0, 0.0, 10, AT_LINEAR);
		    if (vh->onview)
			vh_anim_alpha(vh->onview, 0.0, 1.0, 10, AT_LINEAR);
		}
		else
		{
		    vh->state = VH_BUTTON_UP;
		    if (vh->offview)
			vh_anim_alpha(vh->offview, 0.0, 1.0, 10, AT_LINEAR);
		    if (vh->onview)
			vh_anim_alpha(vh->onview, 1.0, 0.0, 10, AT_LINEAR);
		}

		vh_button_event_t event = {.id = VH_BUTTON_EVENT, .view = view, .vh = vh, .ev = ev};
		if (vh->on_event)
		    (*vh->on_event)(event);
	    }
	    else
	    {
		vh->state = VH_BUTTON_UP;

		vh_button_event_t event = {.id = VH_BUTTON_EVENT, .view = view, .vh = vh, .ev = ev};
		if (vh->on_event)
		    (*vh->on_event)(event);

		/* if (vh->offview) vh_anim_alpha(vh->offview, 1.0, 0.0, 10, AT_LINEAR); */
		/* if (vh->onview) vh_anim_alpha(vh->onview, 0.0, 1.0, 10, AT_LINEAR); */
	    }
	}
    }

    return 0;
}

void vh_button_set_state(ku_view_t* view, vh_button_state_t state)
{
    vh_button_t* vh = view->evt_han_data;

    if (vh->enabled)
    {
	vh->state = state;

	if (state)
	{
	    if (vh->offview)
		vh_anim_alpha(vh->offview, 1.0, 0.0, 10, AT_LINEAR);
	    if (vh->onview)
		vh_anim_alpha(vh->onview, 0.0, 1.0, 10, AT_LINEAR);
	}
	else
	{
	    if (vh->offview)
		vh_anim_alpha(vh->offview, 0.0, 1.0, 10, AT_LINEAR);
	    if (vh->onview)
		vh_anim_alpha(vh->onview, 1.0, 0.0, 10, AT_LINEAR);
	}
    }
}

void vh_button_del(void* p)
{
    /* vh_button_t* vh = p; */
}

void vh_button_desc(void* p, int level)
{
    printf("vh_button");
}

void vh_button_add(ku_view_t* view, vh_button_type_t type, void (*on_event)(vh_button_event_t))
{
    assert(view->evt_han == NULL && view->evt_han_data == NULL);

    vh_button_t* vh = CAL(sizeof(vh_button_t), vh_button_del, vh_button_desc);
    vh->on_event    = on_event;
    vh->type        = type;
    vh->state       = VH_BUTTON_UP;
    vh->enabled     = 1;

    if (view->views->length > 0)
    {
	vh->offview = view->views->data[0];
	vh_anim_add(vh->offview, vh_button_on_anim, view);
    }
    if (view->views->length > 1)
    {
	vh->type   = VH_BUTTON_TOGGLE;
	vh->onview = view->views->data[1];
	vh_anim_add(vh->onview, vh_button_on_anim, view);
    }

    if (vh->offview)
	ku_view_set_texture_alpha(vh->offview, 1.0, 0);
    if (vh->onview)
	ku_view_set_texture_alpha(vh->onview, 0.0, 0);

    view->evt_han      = vh_button_evt;
    view->evt_han_data = vh;
}

void vh_button_set_enabled(ku_view_t* view, int flag)
{
    vh_button_t* vh = view->evt_han_data;

    if (flag)
    {
	if (vh->enabled == 0 && vh->offview)
	    vh_anim_alpha(vh->offview, 0.3, 1.0, 10, AT_LINEAR);
    }
    else
    {
	if (vh->enabled == 1 && vh->offview)
	    vh_anim_alpha(vh->offview, 1.0, 0.3, 10, AT_LINEAR);
    }

    vh->enabled = flag;
}

#endif
