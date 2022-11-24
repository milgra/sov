#ifndef ku_gen_textstyle_h
#define ku_gen_textstyle_h

#include "ku_text.c"
#include "ku_view.c"

textstyle_t ku_gen_textstyle_parse(ku_view_t* view);

#endif

#if __INCLUDE_LEVEL__ == 0

#include "ku_fontconfig.c"
#include <limits.h>

textstyle_t ku_gen_textstyle_parse(ku_view_t* view)
{
    textstyle_t style = {0};

    char* font = ku_fontconfig_path(view->style.font_family);
    if (font) strcpy(style.font, font);

    style.size = view->style.font_size > 0 ? view->style.font_size : 15;

    style.align  = view->style.text_align;
    style.valign = view->style.vertical_align;
    /* style.autosize = */
    style.multiline   = view->style.word_wrap == 1;
    style.line_height = view->style.line_height;

    style.margin = view->style.margin;
    if (view->style.margin_left < INT_MAX) style.margin_left = view->style.margin_left;
    if (view->style.margin_right < INT_MAX) style.margin_right = view->style.margin_right;
    if (view->style.margin_top < INT_MAX) style.margin_top = view->style.margin_top;
    if (view->style.margin_bottom < INT_MAX) style.margin_bottom = view->style.margin_bottom;

    style.textcolor = view->style.color;
    style.backcolor = view->style.background_color;

    return style;
}

#endif
