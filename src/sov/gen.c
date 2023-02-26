#ifndef gen_h
#define gen_h

#include "ku_bitmap.c"
#include "ku_window.c"
#include "mt_vector.c"

void gen_init(char* html_path, char* css_path, char* img_path);
void gen_render(int twidth, int theight, float scale, int cols, int rows, int use_name, mt_vector_t* workspaces, ku_bitmap_t* bitmap);
void gen_destroy();
void gen_calc_size(int twidth, int theight, float scale, int cols, int rows, int* width, int* height);

#endif

#if __INCLUDE_LEVEL__ == 0

#include "ku_draw.c"
#include "ku_gen_css.c"
#include "ku_gen_html.c"
#include "ku_gen_type.c"
#include "ku_renderer_soft.c"
#include "ku_view.c"
#include "mt_log.c"
#include "mt_string.c"
#include "tg_css.c"
#include "tg_text.c"
#include "tree.c"

ku_window_t* kuwindow; /* kinetic ui window for building up workspaces for each display */

/* master views */

ku_view_t*   workspace;
ku_view_t*   number;
ku_view_t*   window;
ku_view_t*   window_active;
ku_view_t*   title;
ku_view_t*   content;
ku_view_t*   row;
ku_view_t*   view_base;
ku_view_t*   base;
mt_vector_t* view_list;

void gen_init(char* html_path, char* css_path, char* img_path)
{
    view_list = VNEW();

    ku_gen_html_parse(html_path, view_list);
    ku_gen_css_apply(view_list, css_path, img_path);
    ku_gen_type_apply(view_list, NULL, NULL);

    kuwindow  = ku_window_create(500, 500, 1.0);
    view_base = mt_vector_head(view_list);

    workspace     = RET(GETV(view_base, "workspace"));
    window        = RET(GETV(view_base, "window"));
    window_active = RET(GETV(view_base, "window_active"));
    number        = RET(GETV(view_base, "number"));
    title         = RET(GETV(view_base, "title"));
    base          = RET(GETV(view_base, "base"));
    content       = RET(GETV(view_base, "content"));
    row           = RET(GETV(view_base, "row"));

    ku_view_remove_from_parent(content);
    ku_view_remove_from_parent(title);
    ku_view_remove_from_parent(window);
    ku_view_remove_from_parent(window_active);
    ku_view_remove_from_parent(number);
    ku_view_remove_from_parent(workspace);
    ku_view_remove_from_parent(row);

    ku_window_add(kuwindow, view_base);
}

void gen_destroy()
{
    /* reset window */

    for (int index = base->views->length - 1; index > -1; index--)
    {
	ku_view_t* view = base->views->data[index];
	ku_view_remove_from_parent(view);
    }

    REL(workspace);
    REL(window);
    REL(window_active);
    REL(number);
    REL(title);
    REL(base);
    REL(content);
    REL(row);

    REL(view_list);
}

void gen_calc_size(int twidth, int theight, float scale, int cols, int rows, int* width, int* height)
{
    *width  = cols * twidth + 2 * (cols + 1) * workspace->style.margin * scale;
    *height = rows * theight + 2 * (rows + 1) * workspace->style.margin * scale;
}

void gen_render(
    int          twidth,
    int          theight,
    float        scale,
    int          cols,
    int          rows,
    int          use_name,
    mt_vector_t* workspaces,
    ku_bitmap_t* bitmap)
{
    int width  = (cols * twidth + 2 * (cols + 1) * workspace->style.margin) * scale;
    int height = (rows * theight + 2 * (rows + 1) * workspace->style.margin) * scale;

    /* reset window */

    for (int index = base->views->length - 1; index > -1; index--)
    {
	ku_view_t* view = base->views->data[index];
	ku_view_remove_from_parent(view);
    }

    /* resize window */

    ku_window_resize(kuwindow, width, height, scale);
    ku_view_set_frame(view_base, (ku_rect_t){0.0, 0.0, width, height});

    /* add rows */

    int wsi = 0;

    for (int rowi = 0; rowi < rows; rowi++)
    {
	char name[100] = {0};
	snprintf(name, 100, "row%i", rowi);
	ku_view_t* rowview = ku_view_new(name, (ku_rect_t){0, 0, 100, 100});
	rowview->style     = row->style;

	mt_log_debug("adding row %s", name);

	ku_view_add_subview(base, rowview);
	REL(rowview);

	/* add workspaces */

	for (int coli = 0; coli < cols; coli++)
	{
	    char name[100] = {0};
	    snprintf(name, 100, "workspace%i", wsi);
	    ku_view_t* wsview = ku_view_new(name, (ku_rect_t){0, 0, 100, 100});
	    wsview->style     = workspace->style;
	    tg_css_add(wsview);

	    ku_view_add_subview(rowview, wsview);
	    REL(wsview);

	    /* add number */

	    if (wsi < workspaces->length)
	    {
		sway_workspace_t* ws = workspaces->data[wsi];

		char numname[100] = {0};
		char numnumb[10]  = {0};

		snprintf(numname, 100, "number%i%i", wsi, 0);
		if (use_name)
		    snprintf(numnumb, 10, "%s", ws->name);
		else
		    snprintf(numnumb, 10, "%i", ws->number);

		ku_view_t* numview = ku_view_new(numname, (ku_rect_t){0, 0, number->frame.local.w, number->frame.local.h});
		numview->style     = number->style;
		tg_text_add(numview);
		tg_text_set1(numview, numnumb);

		ku_view_add_subview(wsview, numview);
		REL(numview);
	    }

	    wsi += 1;
	}
    }

    /* finalize workspace sizes by layouting */

    ku_view_layout(view_base, scale);

    /* add windows */

    wsi = 0;

    for (int rowi = 0; rowi < rows; rowi++)
    {
	ku_view_t* rowview = base->views->data[rowi];

	for (int coli = 0; coli < cols; coli++)
	{
	    ku_view_t* wsview = rowview->views->data[coli];

	    if (wsi < workspaces->length)
	    {
		sway_workspace_t* ws = workspaces->data[wsi];

		for (int wii = 0; wii < ws->windows->length; wii++)
		{
		    sway_window_t* wi = ws->windows->data[wii];

		    int wiw = roundf(((float) wi->width / (float) ws->width / scale) * wsview->frame.local.w);
		    int wih = roundf(((float) wi->height / (float) ws->height / scale) * wsview->frame.local.h);
		    int wix = roundf((((float) wi->x) / (float) ws->width / scale) * wsview->frame.local.w);
		    int wiy = roundf((((float) wi->y) / (float) ws->height / scale) * wsview->frame.local.h);

		    if (wiw > 5 && wih > 5)
		    {
			char* titlestr = STRNC("");

			if (wi->appid && strcmp(wi->appid, "null") != 0)
			    titlestr = mt_string_append(titlestr, wi->appid);
			else if (wi->title && strcmp(wi->title, "null") != 0)
			    titlestr = mt_string_append(titlestr, wi->title);
			else
			    titlestr = mt_string_append(titlestr, "unknown");

			char* contentstr = STRNC("");

			if (wi->title && strcmp(wi->title, "null") != 0)
			    contentstr = mt_string_append(contentstr, wi->title);
			else
			    contentstr = mt_string_append(contentstr, "unkown");

			/* frame */

			char winname[100]     = {0};
			char titlename[100]   = {0};
			char contentname[100] = {0};
			snprintf(winname, 100, "window%i", wii);
			snprintf(titlename, 100, "title%i", wii);
			snprintf(contentname, 100, "content%i", wii);

			ku_view_t* winview = ku_view_new(winname, (ku_rect_t){wix, wiy, wiw, wih});
			winview->style     = window->style;
			if (ws->focused) winview->style = window_active->style;
			winview->style.left   = wix;
			winview->style.top    = wiy;
			winview->style.width  = wiw;
			winview->style.height = wih;
			tg_css_add(winview);

			ku_view_t* titleview = ku_view_new(titlename, (ku_rect_t){0, 0, wiw, wih});
			titleview->style     = title->style;
			tg_text_add(titleview);

			ku_view_t* contentview = ku_view_new(contentname, (ku_rect_t){0, 0, wiw, wih});
			contentview->style     = content->style;
			tg_text_add(contentview);

			ku_view_insert_subview(wsview, winview, wsview->views->length - 1);
			ku_view_add_subview(winview, titleview);
			ku_view_add_subview(winview, contentview);

			REL(winview);
			REL(titleview);
			REL(contentview);

			tg_text_set1(titleview, titlestr);
			tg_text_set1(contentview, contentstr);

			REL(titlestr);
			REL(contentstr);
		    }
		}
	    }

	    /* increase workspace index */

	    wsi++;
	}
    }

    ku_view_layout(view_base, scale);
    /* ku_view_describe(view_base, 0); */
    ku_window_update(kuwindow, 0);
    ku_renderer_software_render(kuwindow->views, bitmap, view_base->frame.local);
}

#endif
