#ifndef tree_drawer_h
#define tree_drawer_h

#include "text.c"
#include "zc_bitmap.c"
#include "zc_vector.c"

bm_t* tree_drawer_bm_create(
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
    int         background_corner_radius,
    int         tree_corner_radius,
    int         wsnum_dx,
    int         wsnum_dy);

#endif

#if __INCLUDE_LEVEL__ == 0

#include "config.c"
#include "tree_reader.c"
#include "zc_graphics.c"
#include "zc_log.c"
#include <math.h>

bm_t* tree_drawer_bm_create(
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
    int         background_corner_radius,
    int         tree_corner_radius,
    int         wsnum_dx,
    int         wsnum_dy)
{
    int wth  = 0;
    int hth  = 0;
    int rows = (int) ceilf((float) workspaces->length / cols);

    /* get biggest workspace */

    for (int index = 0; index < workspaces->length; index++)
    {
	sway_workspace_t* ws = workspaces->data[index];
	if (ws->width > wth || ws->height > hth)
	{
	    wth = ws->width;
	    hth = ws->height;
	}
    }

    /* calculate full width */

    int lay_wth = cols * (wth / scale) + (cols + 1) * gap;
    int lay_hth = rows * (hth / scale) + (rows + 1) * gap;

    /* draw rounded background */

    bm_t* bm = bm_new(lay_wth, lay_hth); // REL 0

    gfx_rounded_rect(
	bm,
	0,
	0,
	bm->w,
	bm->h,
	background_corner_radius,
	1.0,
	window_color,
	0);

    /* calculate individual ws schema dimensions */

    int wsw = wth / scale;
    int wsh = hth / scale;

    zc_log_debug("Scaled workspace : %ix%i", wsw, wsh);

    /* draw workspace backgrounds including empty */

    for (int wsi = 0; wsi < rows * cols; wsi++)
    {
	int cx = gap + wsi % cols * (wsw + gap);
	int cy = gap + wsi / cols * (wsh + gap);

	zc_log_debug("Drawing background at : %ix%i", cx, cy);

	// gfx_rounded_rect(bm, cx - 1, cy - 1, wsw + 3, wsh + 3, 8, 0.0, empty_color, window_color);

	gfx_rounded_rect(bm, cx, cy, wsw, wsh, tree_corner_radius, 0.0, empty_border, window_color);
	gfx_rounded_rect(bm, cx + 1, cy + 1, wsw - 2, wsh - 2, tree_corner_radius, 0.0, empty_color, empty_border);
    }

    for (int wsi = 0; wsi < workspaces->length; wsi++)
    {
	sway_workspace_t* ws = workspaces->data[wsi];

	zc_log_debug("Drawing workspace %i", wsi);

	int cx = gap + wsi % cols * (wsw + gap);
	int cy = gap + wsi / cols * (wsh + gap);

	/* draw focused workspace background */

	if (ws->focused) gfx_rounded_rect(bm, cx + 1, cy + 1, wsw - 2, wsh - 2, tree_corner_radius, 0.0, focused_color, empty_border);

	/* draw windows */

	for (int wii = 0; wii < ws->windows->length; wii++)
	{
	    sway_window_t* wi = ws->windows->data[wii];

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

		if (wi->appid && strcmp(wi->appid, "null") != 0)
		    str_add_bytearray(str, wi->appid);
		else if (wi->title && strcmp(wi->title, "null") != 0)
		    str_add_bytearray(str, wi->title);
		else
		    str_add_bytearray(str, "unknown");

		text_render(str, main_style, tbm);

		str_reset(str);

		/* draw title */

		if (wi->title && strcmp(wi->title, "null") != 0)
		    str_add_bytearray(str, wi->title);
		else
		    str_add_bytearray(str, "unkown");

		text_render(str, sub_style, tbm);

		/* draw frame */

		// prevent trying to draw a negative radius
		int frame_radius = tree_corner_radius > 1 ? tree_corner_radius - 1 : 0;

		gfx_rounded_rect(bm, wcx, wcy, wiw, wih, frame_radius, 0.0, border_color, 0);
		gfx_rounded_rect(bm, wcx + 1, wcy + 1, wiw - 2, wih - 2, frame_radius, 0.0, main_style.backcolor, empty_border);

		/* insert text bitmap */

		gfx_blend_rgba1(bm, tbm->data, tbm->w, tbm->h, wcx + 2, wcy + 2);

		char* title = str_new_cstring(str);
		zc_log_debug("Drawing window : %i %i %i %i %s", wcx, wcy, wiw, wih, title);
		REL(title);

		REL(str); // REL 1
		REL(tbm); // REL 0
	    }
	}
    }

    /* draw all workspace numbers */

    for (uint8_t wsi = 0; wsi < workspaces->length; wsi++)
    {
	sway_workspace_t* ws  = workspaces->data[wsi];
	int               cx  = gap + wsi % cols * (wsw + gap);
	int               cy  = gap + wsi / cols * (wsh + gap);
	int               num = ws->number;

	if (wsw > 0 && wsh > 0)
	{
	    bm_t*  tbm      = bm_new(wsw, wsh); // REL 0
	    str_t* str      = str_new();        // REL 1
	    char   nums[10] = {0};

	    snprintf(nums, 4, "%d", num);
	    str_add_bytearray(str, nums);

	    text_render(str, wsnum_style, tbm);
	    gfx_blend_bitmap(bm, tbm, cx + wsnum_dx, cy + wsnum_dy);

	    zc_log_debug("Drawing number : %s %i %i", str, cx + wsnum_dx, cy + wsnum_dy);

	    REL(str); // REL 1
	    REL(tbm); // REL 0
	}
    }

    return bm;
}

#endif
