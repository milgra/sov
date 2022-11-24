#ifndef vh_tbl_head_h
#define vh_tbl_head_h

#include "ku_view.c"
#include "mt_vector.c"

typedef struct _vh_tbl_head_t
{
    ku_view_t* (*head_create)(ku_view_t* view, void* userdata);
    void (*head_move)(ku_view_t* hview, int index, int pos, void* userdata);
    void (*head_resize)(ku_view_t* hview, int index, int size, void* userdata);
    void (*head_reorder)(ku_view_t* hview, int ind1, int ind2, void* userdata);
    void*      userdata;
    ku_view_t* head;
    int        active;
    int        resize;
    int        touchx;
} vh_tbl_head_t;

void vh_tbl_head_attach(
    ku_view_t* view,
    ku_view_t* (*head_create)(ku_view_t* hview, void* userdata),
    void (*head_move)(ku_view_t* hview, int index, int pos, void* userdata),
    void (*head_resize)(ku_view_t* hview, int index, int size, void* userdata),
    void (*head_reorder)(ku_view_t* hview, int ind1, int ind2, void* userdata),
    void* userdata);

void vh_tbl_head_move(ku_view_t* hview, float dx);
void vh_tbl_head_jump(ku_view_t* hview, float x);

#endif

#if __INCLUDE_LEVEL__ == 0

#define EDGE_DRAG_SIZE 10

void vh_tbl_head_align(ku_view_t* view)
{
    vh_tbl_head_t* vh  = view->handler_data;
    int            pos = 0;
    for (int index = 0; index < vh->head->views->length; index++)
    {
	ku_view_t* sv   = vh->head->views->data[index];
	ku_rect_t  svfl = sv->frame.local;
	svfl.x          = pos;
	pos += svfl.w + 2;
	ku_view_set_frame(sv, svfl);
    }
}

void vh_tbl_head_evt(ku_view_t* view, ku_event_t ev)
{
    vh_tbl_head_t* vh = view->handler_data;

    if (vh->head)
    {
	if (ev.type == KU_EVENT_MDOWN)
	{
	    // look for
	    for (int index = 0; index < vh->head->views->length; index++)
	    {
		ku_view_t* sv  = vh->head->views->data[index];
		ku_rect_t  svf = sv->frame.global;
		// inside
		if (ev.x > svf.x + EDGE_DRAG_SIZE &&
		    ev.x < svf.x + svf.w - EDGE_DRAG_SIZE)
		{
		    vh->active = index;
		    vh->touchx = ev.x - svf.x;
		    break;
		}
		// block start
		if ((ev.x < svf.x + EDGE_DRAG_SIZE) &&
		    ev.x >= svf.x)
		{
		    vh->resize = 1;
		    vh->active = index - 1;
		    break;
		}
		// block end
		if ((ev.x > svf.x + svf.w - EDGE_DRAG_SIZE) &&
		    ev.x <= svf.x + svf.w)
		{
		    vh->resize = 1;
		    vh->active = index;
		    break;
		}
	    }
	}
	else if (ev.type == KU_EVENT_MUP || ev.type == KU_EVENT_MUP_OUT)
	{
	    if (vh->active > -1)
	    {
		if (vh->resize == 0)
		{
		    // look for
		    for (int index = 0; index < vh->head->views->length; index++)
		    {
			ku_view_t* sv  = vh->head->views->data[index];
			ku_rect_t  svf = sv->frame.global;
			// inside
			if (ev.x > svf.x && ev.x < svf.x + svf.w)
			{
			    if (index != vh->active)
			    {
				// drop on different cell
				ku_view_t* cell1 = RET(vh->head->views->data[vh->active]);
				ku_view_t* cell2 = RET(vh->head->views->data[index]);

				ku_view_remove_subview(vh->head, cell1);
				ku_view_insert_subview(vh->head, cell1, index);
				ku_view_remove_subview(vh->head, cell2);
				ku_view_insert_subview(vh->head, cell2, vh->active);

				REL(cell1);
				REL(cell2);

				vh_tbl_head_align(view);

				if (vh->head_reorder) (*vh->head_reorder)(view, vh->active, index, vh->userdata);
				break;
			    }
			    else
			    {
				// self click
				if (vh->head_reorder) (*vh->head_reorder)(view, -1, index, vh->userdata);
			    }
			}
		    }

		    if (vh->head_move) (*vh->head_move)(view, -1, 0, vh->userdata);

		    vh_tbl_head_align(view);
		}
		else
		{
		    if (vh->head_move) (*vh->head_resize)(view, -1, 0, vh->userdata);
		}
	    }

	    vh->active = -1;
	    vh->resize = 0;
	}
	else if (ev.type == KU_EVENT_MMOVE)
	{
	    if (vh->active > -1)
	    {
		if (vh->active < vh->head->views->length)
		{
		    ku_view_t* sv   = vh->head->views->data[vh->active];
		    ku_rect_t  svfg = sv->frame.global;
		    ku_rect_t  svfl = sv->frame.local;

		    if (vh->resize)
		    {
			svfl.w = ev.x - svfg.x;
			ku_view_set_frame(sv, svfl);
			vh_tbl_head_align(view);
			if (vh->head_resize) (*vh->head_resize)(view, vh->active, svfl.w, vh->userdata);
		    }
		    else
		    {
			svfl.x = ev.x - vh->head->frame.global.x - vh->touchx;
			ku_view_set_frame(sv, svfl);
			if (vh->head_move) (*vh->head_move)(view, vh->active, svfl.x, vh->userdata);
		    }
		}
	    }
	}
    }
}

void vh_tbl_head_move(
    ku_view_t* view,
    float      dx)
{
    vh_tbl_head_t* vh = view->handler_data;

    ku_rect_t frame = vh->head->frame.local;

    frame.x += dx;

    ku_view_set_frame(vh->head, frame);
}

void vh_tbl_head_jump(
    ku_view_t* view,
    float      x)
{
    vh_tbl_head_t* vh = view->handler_data;

    ku_rect_t frame = vh->head->frame.local;

    frame.x = x;

    ku_view_set_frame(vh->head, frame);
}

void vh_tbl_head_del(void* p)
{
    vh_tbl_head_t* th = p;

    ku_view_remove_from_parent(th->head);
    REL(th->head);
    th->head = NULL;
}

void vh_tbl_head_desc(void* p, int level)
{
    printf("vh_tbl_head");
}

void vh_tbl_head_attach(
    ku_view_t* view,
    ku_view_t* (*head_create)(ku_view_t* hview, void* userdata),
    void (*head_move)(ku_view_t* hview, int index, int pos, void* userdata),
    void (*head_resize)(ku_view_t* hview, int index, int size, void* userdata),
    void (*head_reorder)(ku_view_t* hview, int ind1, int ind2, void* userdata),
    void* userdata)
{
    assert(view->handler == NULL && view->handler_data == NULL);

    vh_tbl_head_t* vh = CAL(sizeof(vh_tbl_head_t), vh_tbl_head_del, vh_tbl_head_desc);
    vh->userdata      = userdata;
    vh->head_create   = head_create;
    vh->head_move     = head_move;
    vh->head_resize   = head_resize;
    vh->head_reorder  = head_reorder;
    vh->head          = (*head_create)(view, userdata); // REL 0
    vh->active        = -1;

    view->handler_data = vh;
    view->handler      = vh_tbl_head_evt;
    view->needs_touch  = 1;

    ku_view_add_subview(view, vh->head);
}

#endif
