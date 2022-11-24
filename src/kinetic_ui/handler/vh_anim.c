#ifndef vh_anim_h
#define vh_anim_h

#include "ku_view.c"
#include "mt_math_2d.c"
#include "mt_vector_2d.c"

typedef enum _animtype_t
{
    AT_LINEAR,
    AT_EASE,
    AT_EASE_IN,
    AT_EASE_OUT
} animtype_t;

enum vh_anim_event_id
{
    VH_ANIM_END
};

typedef struct _vh_anim_event_t
{
    enum vh_anim_event_id id;
    ku_view_t*            view;
    void*                 userdata;
} vh_anim_event_t;

typedef struct _vh_anim_t
{
    animtype_t type;

    ku_rect_t sf; // starting frame
    ku_rect_t ef; // ending frame

    ku_rect_t sr; // starting region
    ku_rect_t er; // ending region

    float sa; // starting alpha
    float ea; // ending alpha

    char anim_alpha;
    char anim_frame;
    char anim_region;

    int astep;
    int asteps;
    int rstep;
    int rsteps;
    int rstart;
    int fstep;
    int fsteps;
    int fstart;

    void* userdata;
    void (*on_event)(vh_anim_event_t event);
} vh_anim_t;

void vh_anim_frame(ku_view_t* view, ku_rect_t sf, ku_rect_t ef, int start, int steps, animtype_t type);

void vh_anim_alpha(ku_view_t* view, float sa, float ea, int steps, animtype_t type);

void vh_anim_region(ku_view_t* view, ku_rect_t sr, ku_rect_t er, int start, int steps, animtype_t type);

void vh_anim_finish(ku_view_t* view);

void vh_anim_add(ku_view_t* view, void (*on_event)(vh_anim_event_t), void* userdata);

#endif

#if __INCLUDE_LEVEL__ == 0

void vh_anim_evt(ku_view_t* view, ku_event_t ev)
{
    vh_anim_t* vh = view->handler_data;
    if (ev.type == KU_EVENT_FRAME)
    {
	if (vh->anim_frame)
	{
	    if (vh->fstep < vh->fstart)
	    {
		view->frame.pos_changed = 1;
		vh->fstep += 1;
	    }
	    else if (vh->fstep < vh->fstart + vh->fsteps)
	    {
		int delta = vh->fstep - vh->fstart + 1;

		ku_rect_t sf = vh->sf;
		ku_rect_t cf = sf;
		ku_rect_t ef = vh->ef;

		if (vh->type == AT_LINEAR)
		{
		    // just increase current with delta
		    cf.x = sf.x + ((ef.x - sf.x) / vh->fsteps) * delta;
		    cf.y = sf.y + ((ef.y - sf.y) / vh->fsteps) * delta;
		    cf.w = sf.w + ((ef.w - sf.w) / vh->fsteps) * delta;
		    cf.h = sf.h + ((ef.h - sf.h) / vh->fsteps) * delta;
		}
		else if (vh->type == AT_EASE)
		{
		    // speed function based on cosine ( half circle )
		    float angle = 3.14 + (3.14 / vh->fsteps) * delta;
		    float delta = (cos(angle) + 1.0) / 2.0;

		    cf.x = sf.x + (ef.x - sf.x) * delta;
		    cf.y = sf.y + (ef.y - sf.y) * delta;
		    cf.w = sf.w + (ef.w - sf.w) * delta;
		    cf.h = sf.h + (ef.h - sf.h) * delta;
		}

		if (delta == vh->fsteps - 1) cf = ef;

		ku_view_set_frame(view, cf);

		vh->fstep += 1;

		if (delta == vh->fsteps)
		{
		    vh->anim_frame        = 0;
		    vh_anim_event_t event = {.id = VH_ANIM_END, .view = view, .userdata = vh->userdata};
		    if (vh->on_event) (*vh->on_event)(event);
		}
	    }
	}

	if (vh->anim_region)
	{
	    if (vh->rstep < vh->rstart)
	    {
		view->frame.pos_changed = 1;
		vh->rstep += 1;
	    }
	    else if (vh->rstep < vh->rstart + vh->rsteps)
	    {
		int delta = vh->rstep - vh->rstart + 1;

		ku_rect_t sr = vh->sr;
		ku_rect_t cr = sr;
		ku_rect_t er = vh->er;

		if (vh->type == AT_LINEAR)
		{
		    // just increase current with delta
		    cr.x = sr.x + ((er.x - sr.x) / vh->rsteps) * delta;
		    cr.y = sr.y + ((er.y - sr.y) / vh->rsteps) * delta;
		    cr.w = sr.w + ((er.w - sr.w) / vh->rsteps) * delta;
		    cr.h = sr.h + ((er.h - sr.h) / vh->rsteps) * delta;
		}
		else if (vh->type == AT_EASE)
		{
		    // speed function based on cosine ( half circle )
		    float angle = 3.14 + (3.14 / vh->rsteps) * delta;
		    float delta = (cos(angle) + 1.0) / 2.0;

		    cr.x = sr.x + (er.x - sr.x) * delta;
		    cr.y = sr.y + (er.y - sr.y) * delta;
		    cr.w = sr.w + (er.w - sr.w) * delta;
		    cr.h = sr.h + (er.h - sr.h) * delta;
		}

		if (delta == vh->rsteps - 1) cr = er;

		ku_view_set_region(view, cr);

		vh->rstep += 1;

		if (delta == vh->rsteps)
		{
		    ku_view_set_region(view, (ku_rect_t){-1, -1, -1. - 1});
		    vh_anim_event_t event = {.id = VH_ANIM_END, .view = view, .userdata = vh->userdata};
		    if (vh->on_event) (*vh->on_event)(event);
		}
	    }
	}

	if (vh->anim_alpha)
	{
	    if (vh->astep < vh->asteps)
	    {
		float sa = vh->sa;
		float ea = vh->ea;
		float ca = sa;

		if (vh->type == AT_LINEAR)
		{
		    // just increase current with delta
		    ca = sa + ((ea - sa) / vh->asteps) * vh->astep;
		}
		else if (vh->type == AT_EASE)
		{
		    // speed function based on cosine ( half circle )
		    float angle = 3.14 + (3.14 / vh->asteps) * vh->astep;
		    float delta = (cos(angle) + 1.0) / 2.0;
		    ca          = sa + (ea - sa) * delta;
		}

		if (vh->astep == vh->asteps - 1) ca = ea;

		ku_view_set_texture_alpha(view, ca, 1);

		vh->astep += 1;

		if (vh->astep == vh->asteps)
		{
		    vh->anim_alpha        = 0;
		    vh_anim_event_t event = {.id = VH_ANIM_END, .view = view, .userdata = vh->userdata};
		    if (vh->on_event) (*vh->on_event)(event);
		}
	    }
	}
    }
}

void vh_anim_frame(ku_view_t* view, ku_rect_t sf, ku_rect_t ef, int start, int steps, animtype_t type)
{
    vh_anim_t* vh = view->handler_data;
    if (vh->fstep == vh->fsteps)
    {
	vh->sf         = sf;
	vh->ef         = ef;
	vh->fstart     = start;
	vh->fstep      = 0;
	vh->type       = type;
	vh->fsteps     = steps;
	vh->anim_frame = 1;
    }
}

void vh_anim_region(ku_view_t* view, ku_rect_t sr, ku_rect_t er, int start, int steps, animtype_t type)
{
    vh_anim_t* vh = view->handler_data;
    if (vh->rstep == vh->rsteps)
    {
	vh->sr          = sr;
	vh->er          = er;
	vh->rstep       = 0;
	vh->rstart      = start;
	vh->type        = type;
	vh->rsteps      = steps;
	vh->anim_region = 1;

	ku_view_set_region(view, sr);
    }
}

void vh_anim_alpha(ku_view_t* view, float sa, float ea, int steps, animtype_t type)
{
    vh_anim_t* vh = view->handler_data;

    if (vh->astep == vh->asteps)
    {
	vh->sa         = sa;
	vh->ea         = ea;
	vh->astep      = 0;
	vh->type       = type;
	vh->asteps     = steps;
	vh->anim_alpha = 1;
    }

    // force frame animation with dirty rect
    view->frame.pos_changed = 1;
}

void vh_anim_finish(ku_view_t* view)
{
    vh_anim_t* vh = view->handler_data;
    vh->astep     = vh->asteps;
    vh->fstep     = vh->fsteps;
    vh->rstep     = vh->rsteps;

    if (vh->anim_frame) ku_view_set_frame(view, vh->ef);
    if (vh->anim_region) ku_view_set_region(view, vh->er);
    if (vh->anim_alpha) ku_view_set_texture_alpha(view, vh->ea, 1);

    vh->anim_frame  = 0;
    vh->anim_region = 0;
    vh->anim_alpha  = 0;
}

void vh_anim_desc(void* p, int level)
{
    printf("vh_anim");
}

void vh_anim_add(ku_view_t* view, void (*on_event)(vh_anim_event_t), void* userdata)
{
    assert(view->handler == NULL && view->handler_data == NULL);

    vh_anim_t* vh = CAL(sizeof(vh_anim_t), NULL, vh_anim_desc);
    vh->on_event  = on_event;
    vh->userdata  = userdata;

    view->handler      = vh_anim_evt;
    view->handler_data = vh;
}

#endif
