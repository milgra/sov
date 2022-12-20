#ifndef ku_window_h
#define ku_window_h

#include "ku_bitmap.c"
#include "ku_event.c"
#include "ku_view.c"

typedef struct _ku_window_t ku_window_t;
struct _ku_window_t
{
    ku_view_t*   root;
    mt_vector_t* views;
    mt_vector_t* focusable;
    ku_view_t*   focused;

    int   width;
    int   height;
    float scale;

    mt_vector_t* ptrqueue; // views collected by mouse uo and down
    mt_vector_t* movqueue; // views collected by movement events
};

ku_window_t* ku_window_create(int width, int height, float scale);
void         ku_window_resize(ku_window_t* window, int width, int height, float scale);
void         ku_window_layout(ku_window_t* window);
void         ku_window_add(ku_window_t* window, ku_view_t* view);
void         ku_window_remove(ku_window_t* window, ku_view_t* view);
void         ku_window_activate(ku_window_t* win, ku_view_t* view, int flag);
void         ku_window_deactivate(ku_window_t* window, ku_view_t* view);
void         ku_window_set_focusable(ku_window_t* window, mt_vector_t* views);
void         ku_window_event(ku_window_t* window, ku_event_t event);
ku_rect_t    ku_window_update(ku_window_t* window, uint32_t time);

#endif

#if __INCLUDE_LEVEL__ == 0

#include "ku_gl.c"
#include "mt_math_2d.c"
#include "mt_time.c"
#include "mt_vector.c"
#include "mt_vector_2d.c"
#include <xkbcommon/xkbcommon.h>

void ku_window_del(void* p)
{
    ku_window_t* win = p;

    REL(win->root);
    REL(win->views);
    REL(win->ptrqueue);
    REL(win->movqueue);
    if (win->focusable) REL(win->focusable);
}

ku_window_t* ku_window_create(int width, int height, float scale)
{
    ku_window_t* win = CAL(sizeof(ku_window_t), ku_window_del, NULL);

    win->root     = ku_view_new("root", (ku_rect_t){0, 0, width, height}); // REL 0
    win->views    = VNEW();
    win->ptrqueue = VNEW();
    win->movqueue = VNEW();

    win->scale  = scale;
    win->width  = width;
    win->height = height;

    return win;
}

void ku_window_resize(ku_window_t* window, int width, int height, float scale)
{
    window->scale  = scale;
    window->width  = width;
    window->height = height;
    ku_view_set_frame(window->root, (ku_rect_t){0.0, 0.0, width, height});
}

void ku_window_layout(ku_window_t* window)
{
    ku_view_layout(window->root, window->scale);
    /* ku_view_describe(window->root, 0); */
}

void ku_window_add(ku_window_t* win, ku_view_t* view)
{
    ku_view_add_subview(win->root, view);

    // layout, window could be resized since
    ku_view_layout(win->root, win->scale);
}

void ku_window_remove(ku_window_t* win, ku_view_t* view)
{
    ku_view_remove_from_parent(view);
}

void ku_window_activate(ku_window_t* win, ku_view_t* view, int flag)
{
    if (flag)
	mt_vector_add_unique_data(win->ptrqueue, view);
    else
	mt_vector_rem(win->ptrqueue, view);
}

void ku_window_set_focusable(ku_window_t* window, mt_vector_t* views)
{
    if (window->focusable) REL(window->focusable);
    window->focusable = RET(views);
}

void ku_window_event(ku_window_t* win, ku_event_t ev)
{
    if (ev.type == KU_EVENT_FRAME)
    {
	ku_view_evt(win->root, ev);
    }
    else if (ev.type == KU_EVENT_RESIZE)
    {
	ku_rect_t rf = win->root->frame.local;
	if (rf.w != (float) ev.w ||
	    rf.h != (float) ev.h)
	{
	    ku_view_set_frame(win->root, (ku_rect_t){0.0, 0.0, (float) ev.w, (float) ev.h});
	    ku_view_layout(win->root, win->scale);
	    /* ku_view_describe(win->root, 0); */
	    ku_view_evt(win->root, ev);

	    win->width  = ev.w;
	    win->height = ev.h;
	}
	ku_view_evt(win->root, ev);
    }
    else if (ev.type == KU_EVENT_MOUSE_MOVE)
    {
	ku_event_t outev = ev;
	outev.type       = KU_EVENT_MOUSE_MOVE_OUT;
	for (int i = win->movqueue->length - 1; i > -1; i--)
	{
	    ku_view_t* v = win->movqueue->data[i];
	    if (v->needs_touch)
	    {
		if (ev.x > v->frame.global.x &&
		    ev.x < v->frame.global.x + v->frame.global.w &&
		    ev.y > v->frame.global.y &&
		    ev.y < v->frame.global.y + v->frame.global.h)
		{
		}
		else
		{
		    if (v->handler) (*v->handler)(v, outev);
		    if (v->blocks_touch) break;
		}
	    }
	}

	mt_vector_reset(win->movqueue);
	ku_view_coll_touched(win->root, ev, win->movqueue);

	for (int i = win->movqueue->length - 1; i > -1; i--)
	{
	    ku_view_t* v = win->movqueue->data[i];
	    if (v->needs_touch && v->parent)
	    {
		if (v->handler) (*v->handler)(v, ev);
		if (v->blocks_touch) break;
	    }
	}
    }
    else if (ev.type == KU_EVENT_MOUSE_DOWN || ev.type == KU_EVENT_MOUSE_UP)
    {
	ku_event_t outev = ev;
	if (ev.type == KU_EVENT_MOUSE_DOWN) outev.type = KU_EVENT_MOUSE_DOWN_OUT;
	if (ev.type == KU_EVENT_MOUSE_UP) outev.type = KU_EVENT_MOUSE_UP_OUT;

	if (ev.type == KU_EVENT_MOUSE_DOWN)
	{
	    for (int i = win->ptrqueue->length - 1; i > -1; i--)
	    {
		ku_view_t* v = win->ptrqueue->data[i];
		if (v->needs_touch)
		{
		    if (v->handler) (*v->handler)(v, outev);
		    if (v->blocks_touch) break;
		}
	    }

	    mt_vector_reset(win->ptrqueue);
	}
	ku_view_coll_touched(win->root, ev, win->ptrqueue);

	for (int i = win->ptrqueue->length - 1; i > -1; i--)
	{
	    ku_view_t* v = win->ptrqueue->data[i];
	    if (v->needs_touch && v->parent)
	    {
		if (v->handler) (*v->handler)(v, ev);
		if (v->blocks_touch) break;
	    }
	}
    }
    else if (ev.type == KU_EVENT_SCROLL)
    {
	mt_vector_reset(win->ptrqueue);
	ku_view_coll_touched(win->root, ev, win->ptrqueue);

	for (int i = win->ptrqueue->length - 1; i > -1; i--)
	{
	    ku_view_t* v = win->ptrqueue->data[i];
	    if (v->needs_touch && v->parent)
	    {
		if (v->handler) (*v->handler)(v, ev);
		if (v->blocks_scroll) break;
	    }
	}
    }
    else if (ev.type == KU_EVENT_PINCH)
    {
	mt_vector_reset(win->ptrqueue);
	ku_view_coll_touched(win->root, ev, win->ptrqueue);

	for (int i = win->ptrqueue->length - 1; i > -1; i--)
	{
	    ku_view_t* v = win->ptrqueue->data[i];
	    if (v->needs_touch && v->parent)
	    {
		if (v->handler) (*v->handler)(v, ev);
		if (v->blocks_scroll) break;
	    }
	}
    }
    else if (ev.type == KU_EVENT_KEY_DOWN || ev.type == KU_EVENT_KEY_UP)
    {
	if (ev.type == KU_EVENT_KEY_DOWN && ev.keycode == XKB_KEY_Tab && win->focusable->length > 0)
	{
	    if (win->focused == NULL)
	    {
		ku_view_t* v = win->focusable->data[0];
		win->focused = v;
		if (v->handler) (*v->handler)(v, (ku_event_t){.type = KU_EVENT_FOCUS});
		mt_vector_add_unique_data(win->ptrqueue, v);
	    }
	    else
	    {
		/* unfocus first */
		int index = 0;
		for (index = 0; index < win->focusable->length; index++)
		{
		    ku_view_t* v = win->focusable->data[index];

		    if (win->focused == v)
		    {
			win->focused = NULL;
			if (v->handler) (*v->handler)(v, (ku_event_t){.type = KU_EVENT_UNFOCUS});
			mt_vector_rem(win->ptrqueue, v);
			break;
		    }
		}

		/* focus next in queue */
		index++;
		if (index == win->focusable->length) index = 0;

		ku_view_t* v = win->focusable->data[index];
		win->focused = v;
		if (v->handler) (*v->handler)(v, (ku_event_t){.type = KU_EVENT_FOCUS});
		mt_vector_add_unique_data(win->ptrqueue, v);
	    }
	}

	for (int i = win->ptrqueue->length - 1; i > -1; i--)
	{
	    ku_view_t* v = win->ptrqueue->data[i];

	    if (v->needs_key && v->parent)
	    {
		if (v->handler) (*v->handler)(v, ev);
		if (v->blocks_key) break;
	    }
	}
    }
    else if (ev.type == KU_EVENT_TEXT)
    {
	for (int i = win->ptrqueue->length - 1; i > -1; i--)
	{
	    ku_view_t* v = win->ptrqueue->data[i];
	    if (v->needs_text)
	    {
		if (v->handler) (*v->handler)(v, ev);
		break;
	    }
	}
    }
}

void ku_window_rearrange(ku_window_t* win, ku_view_t* view, mt_vector_t* views)
{
    VADD(views, view);
    if (view->style.unmask == 1) view->style.unmask = 0; // reset unmasking
    mt_vector_t* vec = view->views;
    for (int i = 0; i < vec->length; i++) ku_window_rearrange(win, vec->data[i], views);
    if (view->style.masked)
    {
	ku_view_t* last    = views->data[views->length - 1];
	last->style.unmask = 1;
    }
}

ku_rect_t ku_window_update(ku_window_t* win, uint32_t time)
{
    ku_rect_t result = {0};

    if (win->root->rearrange == 1)
    {
	mt_vector_reset(win->views);
	ku_window_rearrange(win, win->root, win->views);

	result = win->root->frame.global;

	win->root->rearrange = 0;
    }

    for (int i = 0; i < win->views->length; i++)
    {
	ku_view_t* view = win->views->data[i];

	if (view->texture.ready == 0) ku_view_gen_texture(view);

	if (view->texture.changed)
	{
	    result = ku_rect_add(result, view->frame.global);
	    /* view->texture.changed = 0; */
	}
	else if (view->frame.dim_changed)
	{
	    result                  = ku_rect_add(result, view->frame.global);
	    view->frame.dim_changed = 0;
	}
	else if (view->frame.pos_changed)
	{
	    result                  = ku_rect_add(result, view->frame.global);
	    view->frame.pos_changed = 0;
	}
	else if (view->frame.reg_changed)
	{
	    result                  = ku_rect_add(result, view->frame.global);
	    view->frame.reg_changed = 0;
	}
	else if (view->texture.alpha_changed)
	{
	    result                      = ku_rect_add(result, view->frame.global);
	    view->texture.alpha_changed = 0;
	}
    }

    return result;
}

#endif
