#include "ku_gen_css.c"
#include "ku_gen_html.c"
#include "ku_gen_type.c"
#include "ku_text.c"
#include "ku_view.c"
#include "ku_window.c"
#include "mt_bitmap_ext.c"
#include "mt_log.c"
#include "mt_path.c"
#include "mt_string.c"
#include "mt_string_ext.c"
#include "mt_time.c"
#include "tg_css.c"
#include "tg_text.c"
#include "tree.c"
#include <dirent.h>
#include <errno.h>
#include <getopt.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main(int argc, char** argv)
{
    mt_log_use_colors(isatty(STDERR_FILENO));
    mt_log_level_info();
    mt_time(NULL);

    /* parse options */

    char* cfg_par  = NULL;
    char* mrg_par  = NULL;
    char* anc_par  = NULL;
    char* tree_par = NULL;
    char* ws_par   = NULL;
    char* res_par  = NULL;

    int c, option_index = 0;

    static struct option long_options[] = {
	{"tree", required_argument, NULL, 't'},
	{"workspace", required_argument, NULL, 'w'},
	{"style", required_argument, NULL, 's'},
	{"result", required_argument, NULL, 'r'},
    };

    while ((c = getopt_long(argc, argv, "s:t:w:r:", long_options, &option_index)) != -1)
    {
	switch (c)
	{
	    case 's': cfg_par = mt_string_new_cstring(optarg); break;
	    case 't': tree_par = mt_string_new_cstring(optarg); break;
	    case 'w': ws_par = mt_string_new_cstring(optarg); break;
	    case 'r': res_par = mt_string_new_cstring(optarg); break;
	}
    }

    /* init config */

    char cwd[PATH_MAX];
    if (getcwd(cwd, sizeof(cwd)) == NULL) printf("Cannot get working directory\n");

    char* cfg_path  = mt_path_new_normalize(cfg_par, cwd);
    char* css_path  = mt_path_new_append(cfg_path, "html/main.css");  // REL 6
    char* html_path = mt_path_new_append(cfg_path, "html/main.html"); // REL 7
    char* img_path  = mt_path_new_append(cfg_path, "img");            // REL 6

    printf("style path    : %s\n", cfg_path);
    printf("css path      : %s\n", css_path);
    printf("html path     : %s\n", html_path);
    printf("image path    : %s\n", img_path);
    printf("tree path     : %s\n", tree_par);
    printf("ws path       : %s\n", ws_par);
    printf("res path      : %s\n", res_par);

    /* init text rendering */

    ku_text_init(); // DESTROY 1

    /* read sway tree */

    char* tree_json = mt_string_new_file(tree_par);
    char* ws_part   = mt_string_new_file(ws_par);
    char* ws_json   = mt_string_new_cstring("{\"items\":");
    ws_json         = mt_string_append(ws_json, ws_part);
    ws_json         = mt_string_append(ws_json, "}");

    mt_vector_t* workspaces = VNEW();

    tree_reader_extract(ws_json, tree_json, workspaces);

    printf("WORKSPACES LENGTH %i\n", workspaces->length);

    /* extract outputs */

    /* create window */

    ku_window_t* kuwindow = ku_window_create(1000, 500);

    mt_vector_t* view_list = VNEW();

    ku_gen_html_parse(html_path, view_list);
    ku_gen_css_apply(view_list, css_path, img_path, 1.0);
    ku_gen_type_apply(view_list, NULL, NULL);

    ku_view_t* view_base = mt_vector_head(view_list);

    ku_view_t* workspace     = RET(GETV(view_base, "workspace"));
    ku_view_t* window        = RET(GETV(view_base, "window"));
    ku_view_t* window_active = RET(GETV(view_base, "window_active"));
    ku_view_t* number        = RET(GETV(view_base, "number"));
    ku_view_t* title         = RET(GETV(view_base, "title"));
    ku_view_t* base          = RET(GETV(view_base, "base"));
    ku_view_t* content       = RET(GETV(view_base, "content"));
    ku_view_t* row           = RET(GETV(view_base, "row"));

    ku_view_remove_from_parent(content);
    ku_view_remove_from_parent(title);
    ku_view_remove_from_parent(window);
    ku_view_remove_from_parent(window_active);
    ku_view_remove_from_parent(number);
    ku_view_remove_from_parent(workspace);
    ku_view_remove_from_parent(row);

    /* initial layout of views */

    ku_window_add(kuwindow, view_base);

    REL(view_list);

    /* draw */

    /* calculate full width */

    int ratio   = 8;
    int mwidth  = 1900;
    int mheight = 900;

    int cols   = 5;
    int rows   = (int) ceilf((float) workspaces->length / cols);
    int width  = cols * (mwidth / ratio + workspace->style.margin);
    int height = rows * (mheight / ratio + workspace->style.margin);

    mt_log_debug(
	"Drawing layer : workspaces %i cols %i rows %i ratio %i width %i height %i",
	workspaces->length,
	cols,
	rows,
	ratio,
	width,
	height);

    /* resize window */

    ku_window_resize(kuwindow, width, height);
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

    ku_view_layout(view_base);

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

		    int wiw = roundf(((float) wi->width / (float) ws->width) * wsview->frame.local.w);
		    int wih = roundf(((float) wi->height / (float) ws->height) * wsview->frame.local.h);
		    int wix = roundf((((float) wi->x) / (float) ws->width) * wsview->frame.local.w);
		    int wiy = roundf((((float) wi->y) / (float) ws->height) * wsview->frame.local.h);

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

			mt_log_debug("Drawing window for %s %i : %i %i %i %i %s", wsi, wix, wiy, wiw, wih, titlestr);
		    }
		}
	    }

	    /* increase workspace index */

	    wsi++;
	}
    }

    ku_view_layout(view_base);

    ku_window_update(kuwindow, 0);

    ku_bitmap_t* bitmap = ku_bitmap_new(kuwindow->width, kuwindow->height);

    ku_renderer_software_render(kuwindow->views, bitmap, view_base->frame.local);

    char* tgt_path = mt_path_new_append(res_par, "render0.bmp");

    bm_write(bitmap, tgt_path);

    /* cleanup */

    if (cfg_par) REL(cfg_par);
    REL(cfg_path);
    REL(css_path);
    REL(html_path);
    REL(img_path);

    ku_text_destroy();

#ifdef MT_MEMORY_DEBUG
    mt_memory_stats();
#endif
}
