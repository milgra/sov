/*
  Text texture generator
  Shows text in view
 */

#ifndef texgen_text_h
#define texgen_text_h

#include "ku_text.c"
#include "ku_view.c"

typedef struct _tg_text_t
{
    char*       text;
    float       scale;
    textstyle_t style;
} tg_text_t;

void  tg_text_add(ku_view_t* view);
void  tg_text_set(ku_view_t* view, char* text, textstyle_t style);
void  tg_text_set1(ku_view_t* view, char* text);
char* tg_text_get(ku_view_t* view);
void  tg_text_set_style(ku_view_t* view, textstyle_t style);

#endif

#if __INCLUDE_LEVEL__ == 0

#include "ku_bitmap.c"
#include "ku_draw.c"
#include "ku_gen_textstyle.c"
#include "mt_string.c"
#include "tg_css.c"

int tg_text_index = 0;

void tg_text_gen(ku_view_t* view)
{
    tg_text_t* gen = view->tex_gen_data;
    if (view->frame.local.w > 0 && view->frame.local.h > 0)
    {
	if (gen->scale != view->style.scale)
	{
	    float ratio;
	    if (gen->scale == 0.0) ratio = view->style.scale;
	    else ratio = view->style.scale / gen->scale;
	    gen->style.size *= ratio;
	    gen->scale = view->style.scale;
	}

	ku_bitmap_t* fontmap = ku_bitmap_new((int) view->frame.local.w, (int) view->frame.local.h); // REL 0
	textstyle_t  style   = gen->style;

	if ((style.textcolor & 0xFF) < 0xFF || (style.backcolor & 0xFF) < 0xFF)
	    view->texture.transparent = 1;
	else
	    view->texture.transparent = 0;

	if ((gen->text != NULL) && strlen(gen->text) > 0)
	{
	    ku_text_render(gen->text, style, fontmap);
	}
	else
	{
	    ku_draw_rect(fontmap, 0, 0, fontmap->w, fontmap->h, style.backcolor, 0);
	}

	ku_view_set_texture_bmp(view, fontmap);

	REL(fontmap); // REL 0
    }
}

void tg_text_del(void* p)
{
    tg_text_t* gen = p;
    if (gen->text) REL(gen->text);
}

void tg_text_desc(void* p, int level)
{
    printf("tg_text");
}

void tg_text_add(ku_view_t* view)
{
    assert(view->tex_gen == NULL);

    tg_text_t* gen = CAL(sizeof(tg_text_t), tg_text_del, tg_text_desc);

    gen->text  = NULL; // REL 1
    gen->scale = view->style.scale;
    gen->style = ku_gen_textstyle_parse(view);

    view->tex_gen_data = gen;
    view->tex_gen      = tg_text_gen;
}

void tg_text_set1(ku_view_t* view, char* text)
{
    tg_text_t* gen = view->tex_gen_data;

    if (gen->text) REL(gen->text);
    if (text) gen->text = mt_string_new_cstring(text);

    view->texture.ready = 0;
}

void tg_text_set(ku_view_t* view, char* text, textstyle_t style)
{
    tg_text_t* gen = view->tex_gen_data;

    if (gen->text) REL(gen->text);
    if (text) gen->text = mt_string_new_cstring(text);

    gen->style = style;
    gen->style.size *= gen->scale;

    view->texture.ready = 0;
}

void tg_text_set_style(ku_view_t* view, textstyle_t style)
{
    tg_text_t* gen = view->tex_gen_data;

    gen->style = style;
    gen->style.size *= gen->scale;
    view->texture.ready = 0;
}

char* tg_text_get(ku_view_t* view)
{
    tg_text_t* gen = view->tex_gen_data;
    return gen->text;
}

#endif
