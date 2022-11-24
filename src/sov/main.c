#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>

#include "config.c"
#include "ku_connector_wayland.c"
#include "ku_gen_css.c"
#include "ku_gen_html.c"
#include "ku_gen_type.c"
#include "ku_renderer_soft.c"
#include "ku_window.c"
#include "kvlines.c"
#include "mt_log.c"
#include "mt_path.c"
#include "mt_string.c"
#include "mt_vector.c"
#include "tg_css.c"
#include "tg_text.c"
#include "tree_reader.c"

#define CFG_PATH_LOC "~/.config/sov/config"
#define GET_WORKSPACES_CMD "swaymsg -t get_workspaces"
#define GET_TREE_CMD "swaymsg -t get_tree"

struct sov
{
    mt_vector_t* workspaces; /* workspaces for current frame draw event */

    char* cfg_path;
    int   timeout;

    ku_window_t* kuwindow; /* kinetic ui window for building up workspaces for each display */
    wl_window_t* wlwindow;

    struct monitor_info** monitors; /* available monitors */
    int                   monitor_count;

    /* master views */

    ku_view_t* workspace;
    ku_view_t* number;
    ku_view_t* window;
    ku_view_t* window_active;
    ku_view_t* title;
    ku_view_t* content;
    ku_view_t* row;
    ku_view_t* view_base;
    ku_view_t* base;
} sov;

/* asks for sway workspaces and tree */

void sov_read_tree(mt_vector_t* workspaces)
{
    char  buff[100];
    char* ws_json   = NULL;
    char* tree_json = NULL;

    FILE* pipe = popen(GET_WORKSPACES_CMD, "r");
    ws_json    = mt_string_new_cstring("{\"items\":");
    while (fgets(buff, sizeof(buff), pipe) != NULL) ws_json = mt_string_append(ws_json, buff);
    ws_json = mt_string_append(ws_json, "}");
    pclose(pipe);

    pipe      = popen(GET_TREE_CMD, "r");
    tree_json = mt_string_new_cstring("");
    while (fgets(buff, sizeof(buff), pipe) != NULL) tree_json = mt_string_append(tree_json, buff);
    pclose(pipe);

    tree_reader_extract(ws_json, tree_json, sov.workspaces);

    REL(ws_json);
    REL(tree_json);
}

void init(wl_event_t event)
{
    sov.monitors      = event.monitors;
    sov.monitor_count = event.monitor_count;

    sov.kuwindow = ku_window_create(500, 500);

    mt_vector_t* view_list = VNEW();

    ku_gen_html_parse(config_get("html_path"), view_list);
    ku_gen_css_apply(view_list, config_get("css_path"), config_get("img_path"), 1.0);
    ku_gen_type_apply(view_list, NULL, NULL);

    sov.view_base = mt_vector_head(view_list);

    sov.workspace     = RET(GETV(sov.view_base, "workspace"));
    sov.window        = RET(GETV(sov.view_base, "window"));
    sov.window_active = RET(GETV(sov.view_base, "window_active"));
    sov.number        = RET(GETV(sov.view_base, "number"));
    sov.title         = RET(GETV(sov.view_base, "title"));
    sov.base          = RET(GETV(sov.view_base, "base"));
    sov.content       = RET(GETV(sov.view_base, "content"));
    sov.row           = RET(GETV(sov.view_base, "row"));

    ku_view_remove_from_parent(sov.content);
    ku_view_remove_from_parent(sov.title);
    ku_view_remove_from_parent(sov.window);
    ku_view_remove_from_parent(sov.window_active);
    ku_view_remove_from_parent(sov.number);
    ku_view_remove_from_parent(sov.workspace);
    ku_view_remove_from_parent(sov.row);

    /* initial layout of views */

    ku_window_add(sov.kuwindow, sov.view_base);
}

/* window update */

void update(ku_event_t ev)
{
    if (ev.type == KU_EVENT_FRAME)
    {
	wl_window_t* info = ev.window;

	if (info == sov.wlwindow)
	{
	    mt_vector_t* curr_ws = VNEW();

	    for (int index = 0; index < sov.workspaces->length; index++)
	    {
		sway_workspace_t* ws = sov.workspaces->data[index];
		if (strcmp(ws->output, info->monitor->name) == 0) VADD(curr_ws, ws);
	    }

	    /* calculate full width */

	    int cols   = config_get_int("columns");
	    int rows   = (int) ceilf((float) curr_ws->length / cols);
	    int ratio  = config_get_int("ratio");
	    int width  = cols * (info->monitor->logical_width / ratio) + (cols + 1);
	    int height = rows * (info->monitor->logical_height / ratio) + (rows + 1);

	    mt_log_debug(
		"Drawing layer %s : workspaces %i cols %i rows %i ratio %i width %i height %i",
		info->monitor->name,
		curr_ws->length,
		cols,
		rows,
		ratio,
		width,
		height);

	    /* resize window */

	    ku_view_set_frame(sov.view_base, (ku_rect_t){0.0, 0.0, width, height});

	    /* add rows */

	    int wsi = 0;

	    for (int rowi = 0; rowi < rows; rowi++)
	    {
		char name[100] = {0};
		snprintf(name, 100, "row%i", rowi);
		ku_view_t* rowview = ku_view_new(name, (ku_rect_t){0, 0, 100, 100});
		rowview->style     = sov.row->style;

		mt_log_debug("adding row %s", name);

		ku_view_add_subview(sov.base, rowview);

		/* add workspaces */

		for (int coli = 0; coli < cols; coli++)
		{
		    char name[100] = {0};
		    snprintf(name, 100, "workspace%i", wsi);
		    ku_view_t* wsview = ku_view_new(name, (ku_rect_t){0, 0, 100, 100});
		    wsview->style     = sov.workspace->style;
		    tg_css_add(wsview);

		    ku_view_add_subview(rowview, wsview);

		    /* add number */

		    if (wsi < curr_ws->length)
		    {
			sway_workspace_t* ws = curr_ws->data[wsi];

			char numname[100] = {0};
			char numnumb[10]  = {0};
			snprintf(numname, 100, "number%i%i", wsi, 0);
			snprintf(numnumb, 10, "%i", ws->number);
			ku_view_t* numview = ku_view_new(numname, (ku_rect_t){0, 0, sov.number->frame.local.w, sov.number->frame.local.h});
			numview->style     = sov.number->style;
			tg_text_add(numview);
			tg_text_set1(numview, numnumb);

			ku_view_add_subview(wsview, numview);
		    }

		    wsi += 1;
		}
	    }

	    /* finalize workspace sizes by layouting */

	    ku_view_layout(sov.view_base);

	    /* add windows */

	    wsi = 0;

	    for (int rowi = 0; rowi < rows; rowi++)
	    {
		ku_view_t* rowview = sov.base->views->data[rowi];

		for (int coli = 0; coli < cols; coli++)
		{
		    ku_view_t* wsview = rowview->views->data[coli];

		    if (wsi < curr_ws->length)
		    {
			sway_workspace_t* ws = curr_ws->data[wsi];

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
				winview->style     = sov.window->style;
				if (ws->focused) winview->style = sov.window_active->style;
				winview->style.left   = wix;
				winview->style.top    = wiy;
				winview->style.width  = wiw;
				winview->style.height = wih;
				tg_css_add(winview);

				ku_view_t* titleview = ku_view_new(titlename, (ku_rect_t){0, 0, wiw, wih});
				titleview->style     = sov.title->style;
				tg_text_add(titleview);

				ku_view_t* contentview = ku_view_new(contentname, (ku_rect_t){0, 0, wiw, wih});
				contentview->style     = sov.content->style;
				tg_text_add(contentview);

				ku_view_insert_subview(wsview, winview, 0);
				ku_view_add_subview(winview, titleview);
				ku_view_add_subview(winview, contentview);

				tg_text_set1(titleview, titlestr);
				tg_text_set1(contentview, contentstr);

				REL(titlestr);
				REL(contentstr);

				mt_log_debug("Drawing window for %s %i : %i %i %i %i %s", info->monitor->name, wsi, wix, wiy, wiw, wih, titlestr);
			    }
			}
		    }

		    /* increase workspace index */

		    wsi++;
		}
	    }

	    ku_view_layout(sov.view_base);
	    ku_window_update(sov.kuwindow, 0);
	    ku_renderer_software_render(sov.kuwindow->views, &sov.wlwindow->bitmap, sov.view_base->frame.local);
	    ku_wayland_draw_window(sov.wlwindow, 0, 0, sov.wlwindow->width, sov.wlwindow->height);
	}
    }

    if (ev.type == KU_EVENT_STDIN)
    {
	if (ev.text[0] == '1')
	{
	    if (sov.workspaces == NULL) sov.workspaces = VNEW(); // REL 0
	    mt_vector_reset(sov.workspaces);

	    sov_read_tree(sov.workspaces);

	    // show layer over all workspaces

	    for (int m = 0; m < sov.monitor_count; m++)
	    {
		struct monitor_info* monitor = sov.monitors[m];

		if (sov.wlwindow == NULL)
		{
		    mt_vector_t* curr_ws = VNEW(); // REL 1

		    for (int index = 0; index < sov.workspaces->length; index++)
		    {
			sway_workspace_t* ws = sov.workspaces->data[index];
			if (strcmp(ws->output, monitor->name) == 0) VADD(curr_ws, ws);
		    }

		    /* calculate full width */

		    int cols   = config_get_int("columns");
		    int rows   = (int) ceilf((float) curr_ws->length / cols);
		    int ratio  = config_get_int("ratio");
		    int width  = cols * (monitor->logical_width / ratio) + (cols + 1);
		    int height = rows * (monitor->logical_height / ratio) + (rows + 1);

		    mt_log_debug("Creating layer for %s : workspaces %i cols %i rows %i ratio %i width %i height %i", monitor->name, curr_ws->length, cols, rows, ratio, width, height);

		    sov.wlwindow = ku_wayland_create_generic_layer(monitor, width, height, 0, "", 1);

		    REL(curr_ws);
		}
	    }
	}
    }
    if (ev.text[0] == '3') ku_wayland_exit();
}

void destroy()
{
}

int main(int argc, char** argv)
{
    printf("swayoverview v" SOV_VERSION " by Milan Toth ( www.milgra.com )\n");
    printf("listens on standard input for '0' - hide panel '1' - show panel '2' - quit\n");

    mt_log_use_colors(isatty(STDERR_FILENO));
    mt_log_level_info();

    const char* usage =
	"Usage: sov [options]\n"
	"\n"
	"  -c  --config= [path]                Use config file for session.\n"
	"  -h, --help                          Show help message and quit.\n"
	"  -v                                  Increase verbosity of messages, defaults to errors and warnings only.\n"
	"  -r, --resources=[resources folder]  Resources dir for current session\n"
	"\n";

    /* parse options */

    char* res_par = NULL;

    int c, option_index = 0;

    static struct option long_options[] = {
	{"help", no_argument, NULL, 'h'},
	{"config", required_argument, NULL, 'c'},
	{"verbose", no_argument, NULL, 'v'},
	{"resources", optional_argument, 0, 'r'}};

    while ((c = getopt_long(argc, argv, "c:vho:r:", long_options, &option_index)) != -1)
    {
	switch (c)
	{
	    case 'c':
		sov.cfg_path = mt_string_new_cstring(optarg);
		if (errno == ERANGE || sov.cfg_path == NULL)
		{
		    mt_log_error("Invalid config path value", ULONG_MAX);
		    return EXIT_FAILURE;
		}
		break;
	    case 'h': printf("%s", usage); return EXIT_SUCCESS;
	    case 'v': mt_log_inc_verbosity(); break;
	    case 'r': res_par = mt_string_new_cstring(optarg); break;
	    default: fprintf(stderr, "%s", usage); return EXIT_FAILURE;
	}
    }

    printf("RESPAR %s\n", res_par);

    /* init config */

    config_init(); // DESTROY 0

    char cwd[PATH_MAX];
    if (getcwd(cwd, sizeof(cwd)) == NULL) printf("Cannot get working directory\n");

    char* cfg_path_loc = sov.cfg_path ? mt_path_new_normalize(sov.cfg_path, cwd) : mt_path_new_normalize(CFG_PATH_LOC, getenv("HOME")); // REL 1
    char* cfg_path_glo = mt_path_new_append(PKG_DATADIR, "config");                                                                     // REL 2

    if (config_read(cfg_path_loc) < 0)
    {
	if (config_read(cfg_path_glo) < 0)
	    mt_log_warn("No configuration found ( local : %s , global : %s )\n", cfg_path_loc, cfg_path_glo);
	else
	    mt_log_info("Using config file : %s\n", cfg_path_glo);
    }
    else
	mt_log_info("Using config file : %s\n", cfg_path_loc);

    REL(cfg_path_glo); // REL 2
    REL(cfg_path_loc); // REL 1

    char* res_path = NULL;
    char* wrk_path = mt_path_new_normalize(cwd, NULL); // REL 3

    char* res_path_loc = res_par ? mt_path_new_normalize(res_par, wrk_path) : mt_path_new_normalize("~/.config/wcp", getenv("HOME")); // REL 4
    char* res_path_glo = mt_string_new_cstring(PKG_DATADIR);                                                                          // REL 5

    DIR* dir = opendir(res_path_loc);
    if (dir)
    {
	res_path = res_path_loc;
	closedir(dir);
    }
    else
	res_path = res_path_glo;

    char* css_path  = mt_path_new_append(res_path, "html/main.css");  // REL 6
    char* html_path = mt_path_new_append(res_path, "html/main.html"); // REL 7
    char* img_path  = mt_path_new_append(res_path, "img");            // REL 6

    config_set("res_path", res_path);
    config_set("css_path", css_path);
    config_set("html_path", html_path);
    config_set("img_path", img_path);

    printf("resource path : %s\n", res_path);
    printf("css path      : %s\n", css_path);
    printf("html path     : %s\n", html_path);
    printf("image path    : %s\n", img_path);

    /* get timout from config */

    sov.timeout = config_get_int("timeout");

    /* init text rendering */

    ku_text_init(); // DESTROY 1

    ku_wayland_init(init, update, destroy, 0);

    /* cleanup */

    config_destroy();
    ku_text_destroy();
    if (sov.cfg_path) REL(sov.cfg_path);

#ifdef MT_MEMORY_DEBUG
    mt_memory_stats();
#endif
}
