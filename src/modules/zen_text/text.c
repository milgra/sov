#ifndef text_h
#define text_h

#include "text.c"
#include "zc_bitmap.c"
#include "zc_string.c"
#include <stdint.h>

typedef enum _textalign_t
{
    TA_LEFT,
    TA_CENTER,
    TA_RIGHT,
    TA_JUSTIFY,
} textalign_t;

typedef enum _vertalign_t
{
    VA_CENTER,
    VA_TOP,
    VA_BOTTOM,
} vertalign_t;

typedef enum _autosize_t
{
    AS_FIX,
    AS_AUTO,
} autosize_t;

typedef struct _textstyle_t
{
    char*       font;
    textalign_t align;
    vertalign_t valign;
    autosize_t  autosize;
    char        multiline;
    int         line_height;

    float size;
    int   margin;
    int   margin_top;
    int   margin_right;
    int   margin_bottom;
    int   margin_left;

    uint32_t textcolor;
    uint32_t backcolor;
} textstyle_t;

typedef struct _glyph_t
{
    int      x;
    int      y;
    int      w;
    int      h;
    float    x_scale;
    float    y_scale;
    float    x_shift;
    float    y_shift;
    float    asc;
    float    desc;
    float    base_y;
    uint32_t cp;
} glyph_t;

void text_init();
void text_destroy();

void text_break_glyphs(glyph_t* glyphs, int count, textstyle_t style, int wth, int hth, int* nwth, int* nhth);

void text_align_glyphs(glyph_t* glyphs, int count, textstyle_t style, int w, int h);

void text_render_glyph(glyph_t g, textstyle_t style, bm_t* bitmap);

void text_render_glyphs(glyph_t* glyphs, int count, textstyle_t style, bm_t* bitmap);

void text_layout(glyph_t* glyphs, int count, textstyle_t style, int wth, int hth, int* nwth, int* nhth);

void text_render(str_t* text, textstyle_t style, bm_t* bitmap);

void text_measure(str_t* text, textstyle_t style, int w, int h, int* nw, int* nh);

#endif

#if __INCLUDE_LEVEL__ == 0

#include "text.c"
#include "zc_graphics.c"
#include "zc_map.c"
#include "zc_wrapper.c"
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_GLYPH_H
#include FT_BITMAP_H

struct _txt_ft_t
{
    map_t*         libs;
    map_t*         fonts;
    unsigned char* gbytes; // byte array for glyph baking
    size_t         gcount; // byte array size for glyph baking
} txt_ft;

void text_init()
{
    txt_ft.libs   = MNEW();
    txt_ft.fonts  = MNEW();                        // GREL 0
    txt_ft.gbytes = malloc(sizeof(unsigned char)); // GREL 1
    txt_ft.gcount = 1;
}

void text_destroy()
{
    vec_t* paths = VNEW(); // REL 0
    map_keys(txt_ft.fonts, paths);

    for (int i = 0; i < paths->length; i++)
    {
	char* path = paths->data[i];

	wrapper_t* face = MGET(txt_ft.fonts, path);
	wrapper_t* lib  = MGET(txt_ft.libs, path);

	FT_Done_Face(face->data);
	FT_Done_FreeType(lib->data);
    }

    REL(paths); // REL 0

    REL(txt_ft.libs);
    REL(txt_ft.fonts); // GREL 0

    free(txt_ft.gbytes); // GREL 1
}

void text_font_load(char* path)
{
    assert(txt_ft.fonts != NULL);

    FT_Library library;
    FT_Face    face;

    int error = FT_Init_FreeType(&library);
    if (error == 0)
    {
	error = FT_New_Face(library, path, 0, &face);
	if (error == 0)
	{

	    wrapper_t* libwrp  = wrapper_new(library);
	    wrapper_t* facewrp = wrapper_new(face);

	    MPUTR(txt_ft.libs, path, libwrp);
	    MPUTR(txt_ft.fonts, path, facewrp);

	    if (error != 0) printf("FT Set Char Size error\n");

	    // printf("Font loaded %s txt_ft.fonts in file %li\n", path, face->num_faces);
	}
	else
	{
	    if (error == FT_Err_Unknown_File_Format) { printf("FT Unknown font file format\n"); }
	    else if (error)
	    {
		printf("FT New Face error\n");
	    }
	}
    }
    else
	printf("FT_Init error\n");
}

// breaks text into lines in given rect
// first step for further alignment

void text_break_glyphs(glyph_t* glyphs, int count, textstyle_t style, int wth, int hth, int* nwth, int* nhth)
{

    wrapper_t* facewrp = MGET(txt_ft.fonts, style.font);
    if (facewrp == NULL)
    {
	text_font_load(style.font);
	facewrp = MGET(txt_ft.fonts, style.font);
	if (!facewrp) return;
    }

    FT_Face font = facewrp->data;

    int   spc_a, spc_p;    // actual space, prev space
    int   asc, desc, lgap; // glyph ascent, descent, linegap
    int   lsb, advx;       // left side bearing, glyph advance
    int   fonth, lineh;    // base height, line height
    float xpos, ypos;      // font scale, position

    int error = FT_Set_Char_Size(
	font,            /* handle to face object           */
	style.size * 64, /* char_width in 1/64th of points  */
	0,               /* char_height in 1/64th of points */
	72,              /* horizontal device resolution    */
	72);             /* vertical device resolution      */

    asc  = font->size->metrics.ascender >> 6;
    desc = font->size->metrics.descender >> 6;
    lgap = font->size->metrics.height >> 6;

    fonth = asc - desc;
    lineh = fonth + lgap;

    if (style.line_height != 0) lineh = style.line_height;

    // printf("asc %i desc %i lgap %i fonth %i lineh %i\n", asc, desc, lgap, fonth, lineh);

    spc_a = 0;
    spc_p = 0;
    xpos  = 0;
    ypos  = asc;

    for (int index = 0; index < count; index++)
    {
	glyph_t glyph = glyphs[index];

	int cp  = glyphs[index].cp;
	int ncp = index < count - 1 ? glyphs[index + 1].cp : 0;

	int   x0, y0, x1, y1;
	float x_shift = xpos - (float) floor(xpos); // subpixel shift along x
	float y_shift = ypos - (float) floor(ypos); // subpixel shift along y

	FT_UInt glyph_index  = FT_Get_Char_Index(font, cp);
	FT_UInt nglyph_index = FT_Get_Char_Index(font, ncp);

	error = FT_Load_Glyph(font, glyph_index, FT_LOAD_DEFAULT);
	if (error) printf("FT LOAD CHAR ERROR\n");

	/* printf("GLYPH: %c loaded, width %li height %li horiBearingX %li horiBearingY %li horiAdvance %li vertBearingX %li vertBearingY %li vertAdvance %li\n", */
	/*        glyph.cp, */
	/*        font->glyph->metrics.width >> 6, */
	/*        font->glyph->metrics.height >> 6, */
	/*        font->glyph->metrics.horiBearingX >> 6, */
	/*        font->glyph->metrics.horiBearingY >> 6, */
	/*        font->glyph->metrics.horiAdvance >> 6, */
	/*        font->glyph->metrics.vertBearingX >> 6, */
	/*        font->glyph->metrics.vertBearingY >> 6, */
	/*        font->glyph->metrics.vertAdvance >> 6); */

	/* printf("advance x %li y %li linearHoriAdvance %li linearVertAdvance %li\n", */
	/*        font->glyph->advance.x >> 6, */
	/*        font->glyph->advance.y >> 6, */
	/*        font->glyph->linearHoriAdvance >> 6, */
	/*        font->glyph->linearVertAdvance >> 6); */

	FT_Glyph glph;
	FT_Get_Glyph(font->glyph, &glph);

	FT_BBox bbox;
	FT_Glyph_Get_CBox(glph, FT_GLYPH_BBOX_SUBPIXELS, &bbox); // FT_GLYPH_BBOX_GRIDFIT
	FT_Done_Glyph(glph);

	advx = font->glyph->advance.x >> 6;
	lsb  = font->glyph->metrics.horiBearingX >> 6;

	// increase xpos with left side bearing if first character in line
	if (xpos == 0 && lsb < 0)
	{
	    xpos    = (float) -lsb;
	    x_shift = xpos - (float) floor(xpos); // subpixel shift
	}

	x0 = bbox.xMin >> 6;
	y0 = bbox.yMin >> 6;
	x1 = bbox.xMax >> 6;
	y1 = bbox.yMax >> 6;

	// printf("%c x0 %i y0 %i x1 %i y1 %i\n", glyph.cp, x0, y0, x1, y1);

	int w = x1 - x0;
	int h = y1 - y0;

	/* int size = w * h; */
	/* printf("w %i h %i size %i\n", w, h, size); */
	// printf("%c bitmap left %i bitmap top %i\n", glyph.cp, font->glyph->bitmap_left, font->glyph->bitmap_top);

	glyph.x       = xpos + font->glyph->bitmap_left;
	glyph.y       = ypos - font->glyph->bitmap_top;
	glyph.w       = w;
	glyph.h       = h;
	glyph.x_shift = x_shift;
	glyph.y_shift = y_shift;
	glyph.base_y  = ypos;
	glyph.asc     = (float) asc;
	glyph.desc    = (float) desc;
	glyph.cp      = cp;

	// printf("glyph : %c x : %i y : %i\n", glyph.cp, glyph.x, glyph.y);

	// advance x axis
	xpos += advx;

	// in case of space/invisible, set width based on pos
	if (w == 0) glyph.w = xpos - glyph.x;

	FT_Vector kerning;
	error = FT_Get_Kerning(
	    font,               /* handle to face object */
	    glyph_index,        /* left glyph index      */
	    nglyph_index,       /* right glyph index     */
	    FT_KERNING_DEFAULT, /* kerning mode          */
	    &kerning);          /* target vector         */

	// printf("kerning x %li y %li\n", kerning.x, kerning.y);

	// advance with kerning
	if (ncp > 0) xpos += kerning.x >> 6;

	// line break
	if (cp == '\n' || cp == '\r') glyph.w = 0; // they should be invisible altough they get an empty unicode font face
	if (cp == ' ') spc_a = index;

	// store glyph
	glyphs[index] = glyph;

	if (style.multiline)
	{
	    if (cp == '\n' || cp == '\r' || xpos > wth)
	    {
		if (xpos > wth)
		{
		    // check if we already fell back to this index, break word if yes
		    if (spc_a == spc_p)
			index -= 1;
		    else
			index = spc_a;
		    spc_p = spc_a;
		}
		xpos = 0.0;
		ypos += (float) lineh;
	    }
	}
    }

    *nwth = xpos;
    *nhth = ypos;
}

void text_align_glyphs(glyph_t* glyphs, int count, textstyle_t style, int w, int h)
{
    if (count > 0)
    {
	// calculate vertical shift
	glyph_t head   = glyphs[0];
	glyph_t tail   = glyphs[count - 1];
	float   height = (tail.base_y - tail.desc) - (head.base_y - head.asc);
	float   vs     = 0.0; // vertical shift

	if (style.valign == VA_CENTER) vs = (h - height) / 2.0;
	if (style.valign == VA_BOTTOM) vs = h - height - head.asc;

	// printf("align h %i height %f vs %f\n", h, height, vs);

	// vs = roundf(vs) + 1.0; // TODO investigate this a little, maybe no magic numbers are needed

	for (int i = 0; i < count; i++)
	{
	    glyph_t g = glyphs[i];
	    float   x = g.x;

	    // get last glyph in row for row width

	    float ex = x; // end x

	    int ri;     // row index
	    int sc = 0; // space count

	    for (ri = i; ri < count; ri++)
	    {
		glyph_t rg = glyphs[ri];          // row glyph
		if (rg.base_y != g.base_y) break; // last glyph in row
		ex = rg.x + rg.w;
		if (rg.cp == ' ') sc += 1; // count spaces
	    }

	    float rw = ex - x; // row width

	    // calculate horizontal shift

	    float hs = 0; // horizontal space before first glyph

	    if (style.align == TA_RIGHT) hs = (float) w - rw;
	    if (style.align == TA_CENTER) hs = ((float) w - rw) / 2.0; // space
	    if (style.align == TA_JUSTIFY) hs = ((float) w - rw) / sc; // space

	    // shift glyphs in both direction

	    for (int ni = i; ni < ri; ni++)
	    {
		glyphs[ni].x += (int) hs;
		glyphs[ni].y += (int) vs;
		glyphs[ni].base_y += (int) vs;
	    }

	    // jump to next row

	    i = ri - 1;
	}
    }
}

void text_shift_glyphs(glyph_t* glyphs, int count, textstyle_t style)
{
    int x = style.margin_left;
    int y = style.margin_top;

    for (int i = 0; i < count; i++)
    {
	glyphs[i].x += x;
	glyphs[i].y += y;
	glyphs[i].base_y += y;
    }
}

void text_render_glyph(glyph_t g, textstyle_t style, bm_t* bitmap)
{
    if (g.w > 0 && g.h > 0)
    {
	if ((style.backcolor & 0xFF) > 0) gfx_rect(bitmap, 0, 0, bitmap->w, bitmap->h, style.backcolor, 0);

	wrapper_t* facewrp = MGET(txt_ft.fonts, style.font);
	if (facewrp == NULL)
	{
	    text_font_load(style.font);
	    facewrp = MGET(txt_ft.fonts, style.font);
	    if (!facewrp) return;
	}
	FT_Face font = facewrp->data;

	int size = g.w * g.h;

	// increase glyph baking bitmap size if needed
	if (size > txt_ft.gcount)
	{
	    txt_ft.gcount = size;
	    txt_ft.gbytes = realloc(txt_ft.gbytes, txt_ft.gcount);
	}

	int error = FT_Load_Char(font, g.cp, FT_LOAD_RENDER);
	if (error) { printf("FT Load Char error\n"); }

	error = FT_Render_Glyph(font->glyph, FT_RENDER_MODE_NORMAL);
	if (error) { printf("FT_Render_Glyph error\n"); }

	FT_Bitmap fontmap = font->glyph->bitmap;

	// insert to bitmap
	gfx_blend_8(bitmap, 0, 0, style.textcolor, fontmap.buffer, g.w, g.h);
    }
}

void text_render_glyphs(glyph_t* glyphs, int count, textstyle_t style, bm_t* bitmap)
{
    // if ((style.backcolor & 0xFF) > 0) gfx_rect(bitmap, 0, 0, bitmap->w, bitmap->h, style.backcolor, 0);

    wrapper_t* facewrp = MGET(txt_ft.fonts, style.font);
    wrapper_t* libwrp  = MGET(txt_ft.libs, style.font);
    if (facewrp == NULL)
    {
	text_font_load(style.font);
	facewrp = MGET(txt_ft.fonts, style.font);
	libwrp  = MGET(txt_ft.libs, style.font);
	if (!facewrp) return;
    }
    FT_Face    font    = facewrp->data;
    FT_Library library = libwrp->data;

    // draw glyphs
    for (int i = 0; i < count; i++)
    {
	glyph_t g = glyphs[i];

	// don't write bitmap in case of empty glyphs ( space )
	if (g.w > 0 && g.h > 0)
	{
	    int size = g.w * g.h;

	    // increase glyph baking bitmap size if needed
	    if (size > txt_ft.gcount)
	    {
		txt_ft.gcount = size;
		txt_ft.gbytes = realloc(txt_ft.gbytes, txt_ft.gcount);
	    }

	    int error = FT_Load_Char(font, g.cp, FT_LOAD_RENDER);
	    if (error) { printf("FT Load Char error\n"); }

	    error = FT_Render_Glyph(font->glyph, FT_RENDER_MODE_NORMAL);
	    if (error) { printf("FT_Render_Glyph error\n"); }

	    FT_Bitmap fontmap = font->glyph->bitmap;

	    // printf("blending fontmap width %i height %i mode %i pitch %i\n", fontmap.width, fontmap.rows, fontmap.pixel_mode, fontmap.pitch);

	    if (fontmap.pixel_mode == ft_pixel_mode_mono)
	    {
		// todo avoid conversion somehow
		FT_Bitmap convmap;
		FT_Bitmap_New(&convmap);

		FT_Bitmap_Convert(library, &fontmap, &convmap, 1);

		// insert to bitmap
		gfx_blend_8_1(bitmap, g.x, g.y, style.textcolor, convmap.buffer, convmap.width, convmap.rows);

		FT_Bitmap_Done(library, &convmap);
	    }
	    else
	    {
		// insert to bitmap
		gfx_blend_8(bitmap, g.x, g.y, style.textcolor, fontmap.buffer, fontmap.width, fontmap.rows);
	    }
	}
    }
}

void text_describe_glyphs(glyph_t* glyphs, int count)
{
    for (int i = 0; i < count; i++)
    {
	glyph_t g = glyphs[i];
	printf("%i cp %i xy %i %i wh %i %i\n", i, g.cp, g.x, g.y, g.w, g.h);
    }
}

void text_layout(glyph_t* glyphs, int count, textstyle_t style, int wth, int hth, int* nwth, int* nhth)
{
    if (style.margin_left == 0 && style.margin > 0) style.margin_left = style.margin;
    if (style.margin_right == 0 && style.margin > 0) style.margin_right = style.margin;
    if (style.margin_top == 0 && style.margin > 0) style.margin_top = style.margin;
    if (style.margin_bottom == 0 && style.margin > 0) style.margin_bottom = style.margin;

    int w = wth - style.margin_right - style.margin_left;
    int h = hth - style.margin_top - style.margin_bottom;

    text_break_glyphs(glyphs, count, style, w, h, nwth, nhth);
    text_align_glyphs(glyphs, count, style, w, h);
    text_shift_glyphs(glyphs, count, style);
}

void text_render(str_t* text, textstyle_t style, bm_t* bitmap)
{
    glyph_t* glyphs = malloc(sizeof(glyph_t) * text->length); // REL 0
    for (int i = 0; i < text->length; i++) glyphs[i].cp = text->codepoints[i];

    int nw;
    int nh;

    text_layout(glyphs, text->length, style, bitmap->w, bitmap->h, &nw, &nh);
    text_render_glyphs(glyphs, text->length, style, bitmap);

    free(glyphs); // REL 1
}

void text_measure(str_t* text, textstyle_t style, int w, int h, int* nw, int* nh)
{
    glyph_t* glyphs = malloc(sizeof(glyph_t) * text->length); // REL 0
    for (int i = 0; i < text->length; i++) glyphs[i].cp = text->codepoints[i];

    text_break_glyphs(glyphs, text->length, style, w, h, nw, nh);

    free(glyphs); // REL 1
}

#endif
