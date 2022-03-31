#ifndef tree_drawer_h
#define tree_drawer_h

#include "text.c"
#include "zc_bitmap.c"
#include "zc_vector.c"

void tree_drawer_draw(
    bm_t*       bm,
    vec_t*      workspaces,
    int         gap,
    int         cols,
    int         scale,
    textstyle_t main_style,
    textstyle_t sub_style,
    textstyle_t wsnum_style,
    uint32_t    window_color,
    uint32_t    background_color,
    uint32_t    focused_color,
    uint32_t    border_color,
    uint32_t    empty_color,
    uint32_t    empty_border,
    int         wsnum_dx,
    int         wsnum_dy);

#endif

#if __INCLUDE_LEVEL__ == 0

#include "config.c"
#include "tree_reader.c"
#include "zc_graphics.c"
#include <math.h>

void tree_drawer_draw(
    bm_t*       bm,
    vec_t*      workspaces,
    int         gap,
    int         cols,
    int         scale,
    textstyle_t main_style,
    textstyle_t sub_style,
    textstyle_t wsnum_style,
    uint32_t    window_color,
    uint32_t    background_color,
    uint32_t    focused_color,
    uint32_t    border_color,
    uint32_t    empty_color,
    uint32_t    empty_border,
    int         wsnum_dx,
    int         wsnum_dy)
{
    sway_workspace_t* ws0 = workspaces->data[0];
    sway_workspace_t* wsl = workspaces->data[workspaces->length - 1];

    int max = ceilf((float) wsl->number / cols) * cols;

    /* find biggest workspace */

    int wsw = 0;
    int wsh = 0;

    for (int wsi = 0; wsi < workspaces->length; wsi++)
    {
	sway_workspace_t* ws = workspaces->data[wsi];

	if (ws->width > wsw)
	{
	    wsw = ws->width;
	    wsh = ws->height;
	}
    }

    wsw /= scale;
    wsh /= scale;

    /* draw workspace backgrounds including empty */

    for (int wsi = 0; wsi < max; wsi++)
    {
	int cx = gap + wsi % cols * (wsw + gap);
	int cy = gap + wsi / cols * (wsh + gap);

	// gfx_rounded_rect(bm, cx - 1, cy - 1, wsw + 3, wsh + 3, 8, 0.0, empty_color, window_color);

	gfx_rounded_rect(bm, cx, cy, wsw, wsh, 8, 0.0, empty_border, window_color);
	gfx_rounded_rect(bm, cx + 1, cy + 1, wsw - 2, wsh - 2, 8, 0.0, empty_color, empty_border);
    }

    for (int wsi = 0; wsi < workspaces->length; wsi++)
    {
	sway_workspace_t* ws = workspaces->data[wsi];

	int num = ws->number;

	int cx = gap + (num - 1) % cols * (wsw + gap);
	int cy = gap + (num - 1) / cols * (wsh + gap);

	/* draw focused workspace background */

	if (ws->focused) gfx_rounded_rect(bm, cx + 1, cy + 1, wsw - 2, wsh - 2, 8, 0.0, focused_color, empty_border);

	/* draw windows */

	for (int wii = 0; wii < ws->windows->length; wii++)
	{
	    sway_window_t* wi = ws->windows->data[wii];

	    if (strstr(wi->appid, "overview") == NULL)
	    {
		int wiw = roundf((float) wi->width / scale);
		int wih = roundf((float) wi->height / scale);
		int wix = roundf(((float) wi->x) / scale);
		int wiy = roundf(((float) wi->y) / scale);

		int wcx = cx + wix;
		int wcy = cy + wiy;

		main_style.backcolor = background_color;
		if (ws->focused) main_style.backcolor = focused_color;

		if (wiw > 5 && wih > 5)
		{
		    /* draw appid */

		    bm_t* tbm = bm_new(wiw - 4, wih - 4); // REL 0

		    str_t* str = str_new(); // REL 1

		    str_add_bytearray(str, wi->appid);

		    text_render(str, main_style, tbm);

		    str_reset(str);

		    /* draw title */

		    str_add_bytearray(str, wi->title);

		    text_render(str, sub_style, tbm);

		    /* draw frame */

		    gfx_rounded_rect(bm, wcx, wcy, wiw, wih, 7, 0.0, border_color, 0);
		    gfx_rounded_rect(bm, wcx + 1, wcy + 1, wiw - 2, wih - 2, 7, 0.0, main_style.backcolor, empty_border);

		    /* insert text bitmap */

		    gfx_blend_rgba1(bm, tbm->data, tbm->w, tbm->h, wcx + 2, wcy + 2);

		    REL(str); // REL 1
		    REL(tbm); // REL 0
		}
	    }
	}
    }

    /* draw all workspace numbers */

    for (int wsi = 0; wsi < max; wsi++)
    {
	int cx = gap + wsi % cols * (wsw + gap);
	int cy = gap + wsi / cols * (wsh + gap);

	if (wsw > 0 && wsh > 0)
	{
	    bm_t*  tbm     = bm_new(wsw, wsh); // REL 0
	    str_t* str     = str_new();        // REL 1
	    char   nums[4] = {0};

	    snprintf(nums, 4, "%i", wsi + 1);
	    str_add_bytearray(str, nums);

	    text_render(str, wsnum_style, tbm);
	    gfx_blend_bitmap(bm, tbm, cx + wsnum_dx, cy + wsnum_dy);

	    REL(str); // REL 1
	    REL(tbm); // REL 0
	}
    }
}

#endif
