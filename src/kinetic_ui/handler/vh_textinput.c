#ifndef vh_textinput_h
#define vh_textinput_h

#include "ku_event.c"
#include "ku_text.c"
#include "ku_view.c"
#include <xkbcommon/xkbcommon.h>

typedef struct _vh_textinput_t vh_textinput_t;

enum vh_textinput_event_id
{
    VH_TEXTINPUT_TEXT,
    VH_TEXTINPUT_RETURN,
    VH_TEXTINPUT_ACTIVATE,
    VH_TEXTINPUT_DEACTIVATE
};

typedef struct _vh_textinput_event_t
{
    enum vh_textinput_event_id id;
    vh_textinput_t*            vh;
    char*                      text;
    ku_view_t*                 view;
} vh_textinput_event_t;

typedef struct _glyphchain_t glyphchain_t;
struct _glyphchain_t
{
    mt_vector_t* views; // glpyh views
    char*        text;
    size_t       length;
    size_t       index; // glyph index
};

void glyphchain_init(glyphchain_t* chain);
void glyphchain_reset(glyphchain_t* chain);
void glyphchain_destroy(glyphchain_t* chain);
void glyphchain_add(glyphchain_t* chain, uint32_t cp, char* id, void* data);
void glyphchain_rem(glyphchain_t* chain, size_t index);

struct _vh_textinput_t
{
    ku_view_t* cursor_v; // cursor view
    ku_view_t* holder_v; // placeholder text view
    ku_rect_t  frame_s;  // starting frame for autosize minimal values

    size_t      glyph_index;
    textstyle_t style;
    char        active;

    size_t limit;
    int    mouse_out_deact;
    size_t new_glyph_index;

    glyphchain_t glyphchain;

    void (*on_event)(vh_textinput_event_t);
};

void  vh_textinput_add(ku_view_t* view, char* text, char* phtext, void (*on_event)(vh_textinput_event_t));
void  vh_textinput_set_limit(ku_view_t* view, int limit);
char* vh_textinput_get_text(ku_view_t* view);
void  vh_textinput_set_text(ku_view_t* view, char* text);
void  vh_textinput_set_deactivate_on_mouse_out(ku_view_t* view, int flag);
void  vh_textinput_activate(ku_view_t* view, char state);

#endif

#if __INCLUDE_LEVEL__ == 0

#include "ku_bitmap.c"
#include "ku_draw.c"
#include "ku_gen_textstyle.c"
#include "mt_log.c"
#include "mt_string.c"
#include "mt_vector.c"
#include "tg_css.c"
#include "tg_text.c"
#include "utf8.h"
#include "vh_anim.c"

void vh_textinput_on_anim(vh_anim_event_t event)
{
    ku_view_t* tiview = event.userdata;

    vh_textinput_t* data = tiview->evt_han_data;

    if (mt_vector_index_of_data(data->glyphchain.views, event.view) == SIZE_MAX)
	ku_view_remove_from_parent(event.view);
}

void glyphchain_init(glyphchain_t* chain)
{
    chain->views  = VNEW();
    chain->text   = STRNC("");
    chain->length = 0;
    chain->index  = 0;
}

void glyphchain_reset(glyphchain_t* chain)
{
    mt_vector_reset(chain->views);
    mt_string_reset(chain->text);
    chain->length = 0;
}

void glyphchain_destroy(glyphchain_t* chain)
{
    REL(chain->views);
    REL(chain->text);
}

void glyphchain_add(glyphchain_t* chain, uint32_t cp, char* id, void* data)
{
    chain->text = mt_string_append_cp(chain->text, cp);

    // create view for glyph

    char view_id[100];
    snprintf(view_id, 100, "%sglyph%zu", id, chain->index++);
    ku_view_t* glyph_view = ku_view_new(view_id, (ku_rect_t){0, 0, 0, 0}); // REL 0

    vh_anim_add(glyph_view, vh_textinput_on_anim, data);
    glyph_view->texture.resizable = 0;

    VADD(chain->views, glyph_view);

    REL(glyph_view);

    chain->length = utf8len(chain->text);
}

void glyphchain_rem(glyphchain_t* chain, size_t index)
{
    if (index < chain->length)
    {
	chain->text = mt_string_delete_utf_codepoints(chain->text, index, 1);

	chain->length = utf8len(chain->text);

	mt_vector_rem_index(chain->views, index);
    }
}

glyph_t* vh_textinput_glyphs_from_string(char* text, size_t* count)
{
    const void*  part   = text;
    size_t       length = utf8len(text);
    glyph_t*     glyphs = malloc(sizeof(glyph_t) * length); // REL 0
    utf8_int32_t cp;

    for (size_t i = 0; i < length; i++)
    {
	part         = utf8codepoint(part, &cp);
	glyphs[i].cp = cp;
    }

    *count = length;

    return glyphs;
}

void vh_textinput_upd(ku_view_t* view)
{
    vh_textinput_t* data  = view->evt_han_data;
    ku_rect_t       frame = view->frame.local;

    if (data->glyphchain.length > 0)
    {
	size_t   count  = 0;
	glyph_t* glyphs = vh_textinput_glyphs_from_string(data->glyphchain.text, &count);

	if (data->glyphchain.length != count)
	    printf("GLYPHCHAIN LENGTH MISMATCH %lu %lu\n", count, data->glyphchain.length);

	int nw;
	int nh;

	ku_text_layout(glyphs, count, data->style, frame.w, frame.h, &nw, &nh);

	// resize frame if needed
	if (data->style.autosize == AS_AUTO)
	{
	    frame = view->frame.local;
	    if (nw > frame.w)
	    {
		frame.w = nw;
		ku_view_set_frame(view, frame);
	    }
	    if (nw <= frame.w)
	    {
		if (nw <= data->frame_s.w)
		    nw = data->frame_s.w;
		frame.w = nw;
		ku_view_set_frame(view, frame);
	    }
	    if (nh > frame.h)
	    {
		frame.h = nh;
		ku_view_set_frame(view, frame);
	    }
	    if (nh <= frame.h)
	    {
		if (nh <= data->frame_s.h)
		    nh = data->frame_s.h;
		frame.h = nh;
		ku_view_set_frame(view, frame);
	    }
	}

	for (size_t i = 0; i < count; i++)
	{
	    glyph_t g = glyphs[i];

	    if (i < data->glyphchain.length)
	    {
		ku_view_t* gv = data->glyphchain.views->data[i];

		/* glyph has dimension */
		if (g.w > 0 && g.h > 0)
		{
		    ku_rect_t f  = gv->frame.local;
		    ku_rect_t nf = (ku_rect_t){g.x, g.y, g.w, g.h};

		    /* resize glyph view frame to glyph dimensions */
		    ku_view_set_frame(gv, nf);

		    /* if glyph view is not opened yet */
		    if (f.w == 0 || f.h == 0)
		    {
			/* generate glyph texture */
			ku_bitmap_t* texture = ku_bitmap_new(g.w, g.h); // REL 0
			ku_text_render_glyph(g, data->style, texture);
			ku_view_set_texture_bmp(gv, texture);
			REL(texture);

			/* add to textinput container */
			ku_view_add_subview(view, gv);

			/* start opening animation */
			ku_rect_t sf = nf;
			sf.x         = 0.0;
			sf.y         = 0.0;
			nf.x         = 0.0;
			nf.y         = 0.0;
			sf.w         = 0.0;

			if (i == data->new_glyph_index)
			{
			    /* glyph is at the end of the text */
			    vh_anim_region(gv, sf, nf, 0, 10, AT_LINEAR);
			    data->new_glyph_index = 0;
			}
			else
			{
			    /* glyph is not at the end */
			    vh_anim_region(gv, sf, nf, 0, 10, AT_LINEAR);
			}
			/* set starting region */
			ku_view_set_region(gv, sf);
		    }
		    else
		    {
			/* if glyph is already opened, set frame if not okay */
			ku_rect_t rf = nf;
			rf.x         = 0;
			rf.y         = 0;
			ku_view_set_region(gv, rf);

			if (!ku_rect_equals(rf, gv->frame.region))
			{
			    vh_anim_finish(gv);
			    vh_anim_frame(gv, gv->frame.local, nf, 0, 10, AT_EASE);
			}
		    }
		}
	    }
	    else
		printf("glyph and string count mismatch\n");
	}

	// update cursor position

	glyph_t last = {0};
	if (count > 0)
	    last = glyphs[count - 1];

	ku_rect_t crsr_f = {0};
	crsr_f.x         = last.x + last.w + 1;
	crsr_f.y         = last.base_y - last.asc - last.desc / 2.0;
	crsr_f.w         = 2;
	crsr_f.h         = last.asc;

	vh_anim_finish(data->cursor_v);
	vh_anim_frame(data->cursor_v, data->cursor_v->frame.local, crsr_f, 0, 10, AT_EASE);

	// ku_view_set_frame(data->cursor_v, crsr_f);

	free(glyphs); // REL 0
    }
    else
    {
	// move cursor to starting position based on textstyle

	// get line height
	glyph_t glyph;
	glyph.cp = ' ';
	int nw;
	int nh;
	ku_text_layout(&glyph, 1, data->style, frame.w, frame.h, &nw, &nh);

	ku_rect_t crsr_f = {0};
	crsr_f.w         = 2;
	crsr_f.h         = glyph.asc;

	if (data->style.align == TA_LEFT)
	{
	    crsr_f.x = data->style.margin_left;
	    crsr_f.y = data->style.margin || data->style.margin_top;
	    if (data->style.valign == VA_CENTER)
		crsr_f.y = frame.h / 2 - crsr_f.h / 2;
	    if (data->style.valign == VA_BOTTOM)
		crsr_f.y = frame.h - data->style.margin_bottom - crsr_f.h;
	}
	if (data->style.align == TA_RIGHT)
	{
	    crsr_f.x = frame.w - data->style.margin_right - crsr_f.w;
	    crsr_f.y = data->style.margin_top;
	    if (data->style.valign == VA_CENTER)
		crsr_f.y = frame.h / 2 - crsr_f.h / 2;
	    if (data->style.valign == VA_BOTTOM)
		crsr_f.y = frame.h - data->style.margin_bottom - crsr_f.h;
	}
	if (data->style.align == TA_CENTER)
	{
	    crsr_f.x = frame.w / 2 - crsr_f.w / 2;
	    crsr_f.y = data->style.margin_top;
	    if (data->style.valign == VA_CENTER)
		crsr_f.y = frame.h / 2 - crsr_f.h / 2;
	    if (data->style.valign == VA_BOTTOM)
		crsr_f.y = frame.h - data->style.margin_bottom - crsr_f.h;
	}

	vh_anim_finish(data->cursor_v);
	vh_anim_frame(data->cursor_v, data->cursor_v->frame.local, crsr_f, 0, 10, AT_EASE);
    }

    // textinput_render_glyphs(glyphs, text->length, style, bitmap);
    // vh_anim_add(glyphview);
    // vh_anim_set(glyphview, sf, ef, 10, AT_LINEAR);

    // show text as texture
    // char* cstr = str_new_cstring(text);
    // tg_textet(view, cstr, data->style);
    // REL(cstr);
}

void vh_textinput_activate(ku_view_t* view, char state)
{
    assert(view && view->evt_han_data != NULL);

    vh_textinput_t* data = view->evt_han_data;

    if (state)
    {
	if (!data->active)
	{
	    data->active = 1;

	    if (strlen(data->glyphchain.text) == 0)
	    {
		vh_anim_alpha(data->holder_v, 1.0, 0.0, 10, AT_LINEAR);
	    }
	    vh_anim_alpha(data->cursor_v, 0.0, 1.0, 10, AT_LINEAR);
	}
    }
    else
    {
	if (data->active)
	{
	    data->active = 0;

	    if (strlen(data->glyphchain.text) == 0)
	    {
		vh_anim_alpha(data->holder_v, 0.0, 1.0, 10, AT_LINEAR);
	    }
	    vh_anim_alpha(data->cursor_v, 1.0, 0.0, 10, AT_LINEAR);
	}
    }

    vh_textinput_upd(view);
}

int vh_textinput_evt(ku_view_t* view, ku_event_t ev)
{
    vh_textinput_t* data = view->evt_han_data;

    if (ev.type == KU_EVENT_MOUSE_DOWN)
    {
	// ku_rect_t frame = view->frame.global;

	vh_textinput_activate(view, 1);

	vh_textinput_event_t event = {.id = VH_TEXTINPUT_ACTIVATE, .vh = data, .text = data->glyphchain.text, .view = view};
	if (data->on_event)
	    (*data->on_event)(event);
    }
    else if (ev.type == KU_EVENT_MOUSE_DOWN_OUT)
    {
	if (data->mouse_out_deact)
	{
	    ku_rect_t frame = view->frame.global;

	    if (ev.x < frame.x ||
		ev.x > frame.x + frame.w ||
		ev.y < frame.y ||
		ev.y > frame.y + frame.h)
	    {
		vh_textinput_activate(view, 0);
		vh_textinput_event_t event = {.id = VH_TEXTINPUT_DEACTIVATE, .vh = data, .text = data->glyphchain.text, .view = view};
		if (data->on_event)
		    (*data->on_event)(event);
	    }
	}
    }
    else if (ev.type == KU_EVENT_TEXT)
    {
	if (data->limit == 0 || (data->limit > 0 && data->glyphchain.length < data->limit))
	{
	    data->new_glyph_index = data->glyphchain.length;

	    utf8_int32_t codepoint;
	    utf8codepoint(ev.text, &codepoint);
	    glyphchain_add(&data->glyphchain, codepoint, view->id, view);

	    // append or break-insert new glyph(s)

	    vh_textinput_upd(view);

	    vh_textinput_event_t event = {.id = VH_TEXTINPUT_TEXT, .vh = data, .text = data->glyphchain.text, .view = view};
	    if (data->on_event)
		(*data->on_event)(event);
	}
    }
    else if (ev.type == KU_EVENT_KEY_DOWN)
    {
	if (ev.keycode == XKB_KEY_BackSpace && utf8len(data->glyphchain.text) > 0)
	{
	    ku_view_t* glyph_view = mt_vector_tail(data->glyphchain.views);

	    ku_rect_t sf = glyph_view->frame.local;
	    ku_rect_t ef = sf;
	    sf.x         = 0.0;
	    sf.y         = 0.0;
	    ef.x         = 0.0;
	    ef.y         = 0.0;
	    ef.w         = 0.0;

	    vh_anim_region(glyph_view, sf, ef, 0, 10, AT_EASE);

	    glyphchain_rem(&data->glyphchain, data->glyphchain.length - 1);

	    vh_textinput_upd(view);

	    vh_textinput_event_t event = {.id = VH_TEXTINPUT_TEXT, .vh = data, .text = data->glyphchain.text, .view = view};
	    if (data->on_event)
		(*data->on_event)(event);
	}
	if (ev.keycode == XKB_KEY_Return)
	{
	    vh_textinput_event_t event = {.id = VH_TEXTINPUT_RETURN, .vh = data, .text = data->glyphchain.text, .view = view};
	    if (data->on_event)
		(*data->on_event)(event);
	}
	if (ev.keycode == XKB_KEY_Escape)
	{
	    vh_textinput_activate(view, 0);

	    vh_textinput_event_t event = {.id = VH_TEXTINPUT_DEACTIVATE, .vh = data, .text = data->glyphchain.text, .view = view};
	    if (data->on_event)
		(*data->on_event)(event);
	}
    }
    else if (ev.type == KU_EVENT_FOCUS)
    {
	vh_textinput_activate(view, 1);
    }
    else if (ev.type == KU_EVENT_UNFOCUS)
    {
	vh_textinput_activate(view, 0);
    }
    else if (ev.type == KU_EVENT_RESIZE)
    {
	/* vh_textinput_upd(view); */
    }

    return 0;
}

void vh_textinput_del(void* p)
{
    vh_textinput_t* vh = p;

    glyphchain_destroy(&vh->glyphchain);

    REL(vh->cursor_v);
    REL(vh->holder_v);
}

void vh_textinput_desc(void* p, int level)
{
    printf("vh_textinput");
}

void vh_textinput_add(ku_view_t* view, char* text, char* phtext, void (*on_event)(vh_textinput_event_t))
{
    assert(view->evt_han == NULL && view->evt_han_data == NULL);

    float scale = view->style.scale;

    char* id_c = mt_string_new_format(100, "%s%s", view->id, "crsr");   // REL 0
    char* id_h = mt_string_new_format(100, "%s%s", view->id, "holder"); // REL 1

    vh_textinput_t* data = CAL(sizeof(vh_textinput_t), vh_textinput_del, vh_textinput_desc);

    glyphchain_init(&data->glyphchain);

    data->style          = ku_gen_textstyle_parse(view);
    data->style.autosize = AS_AUTO;
    /* TODO this is not too dynamic, won't work when moving to different scaled output */
    data->style.size *= scale;

    data->on_event = on_event;

    data->frame_s = view->frame.local;

    data->mouse_out_deact = 1;

    view->evt_han      = vh_textinput_evt;
    view->evt_han_data = data;

    data->style.backcolor = 0;

    // cursor

    data->cursor_v                         = ku_view_new(id_c, (ku_rect_t){50, 12, 2, 0}); // REL 4
    data->cursor_v->style.background_color = data->style.textcolor;
    data->cursor_v->style.scale            = scale;

    tg_css_add(data->cursor_v);
    vh_anim_add(data->cursor_v, NULL, NULL);

    ku_view_set_texture_alpha(data->cursor_v, 0.0, 1);
    ku_view_add_subview(view, data->cursor_v);

    // placeholder

    textstyle_t phts = data->style;
    phts.align       = TA_CENTER;
    phts.textcolor   = 0x888888FF;

    data->holder_v              = ku_view_new(id_h, (ku_rect_t){0, 0, view->frame.local.w, view->frame.local.h}); // REL 0
    data->holder_v->style.w_per = 1.0;
    data->holder_v->style.h_per = 1.0;
    data->holder_v->style.scale = scale;

    tg_text_add(data->holder_v);
    tg_text_set(data->holder_v, phtext, phts);

    vh_anim_add(data->holder_v, NULL, NULL);

    ku_view_add_subview(view, data->holder_v);

    // view setup

    // tg_text_add(view);

    // add placeholder view

    // text_s setup

    if (text)
    {
	char*        part = text;
	utf8_int32_t cp;
	for (size_t i = 0; i < utf8len(text); i++)
	{
	    part = utf8codepoint(part, &cp);
	    glyphchain_add(&data->glyphchain, cp, view->id, view);
	}
    }

    // update text

    vh_textinput_upd(view);

    // cleanup

    REL(id_c);
    REL(id_h);
}

void vh_textinput_set_text(ku_view_t* view, char* text)
{
    vh_textinput_t* data = view->evt_han_data;

    // remove glyphs

    for (size_t i = data->glyphchain.length; i-- > 0;)
    {
	ku_view_t* gv = data->glyphchain.views->data[i];

	ku_view_remove_from_parent(gv);

	/* ku_rect_t sf = gv->frame.local; */
	/* ku_rect_t ef = sf; */
	/* sf.x    = 0.0; */
	/* sf.y    = 0.0; */
	/* ef.x    = 0.0; */
	/* ef.y    = 0.0; */
	/* ef.w    = 0.0; */

	/* vh_anim_region(gv, sf, ef, 0,10 + i, AT_EASE); */
	/* vh_anim_set_event(gv, view, vh_textinput_on_glyph_close); */
    }

    glyphchain_reset(&data->glyphchain);

    // text_s setup
    // TODO create function from this to reuse

    if (text)
    {
	char*        part = text;
	utf8_int32_t cp;
	for (size_t i = 0; i < utf8len(text); i++)
	{
	    part = utf8codepoint(part, &cp);
	    glyphchain_add(&data->glyphchain, cp, view->id, view);
	}

	ku_view_set_texture_alpha(data->holder_v, 0, 1);
    }

    vh_textinput_upd(view);

    vh_textinput_event_t event = {.id = VH_TEXTINPUT_TEXT, .vh = data, .text = data->glyphchain.text, .view = view};
    if (data->on_event)
	(*data->on_event)(event);
}

char* vh_textinput_get_text(ku_view_t* view)
{
    vh_textinput_t* data = view->evt_han_data;
    return data->glyphchain.text;
}

void vh_textinput_set_deactivate_on_mouse_out(ku_view_t* view, int flag)
{
    vh_textinput_t* data  = view->evt_han_data;
    data->mouse_out_deact = flag;
}

void vh_textinput_set_limit(ku_view_t* view, int limit)
{
    vh_textinput_t* data = view->evt_han_data;
    data->limit          = limit;
}

#endif
