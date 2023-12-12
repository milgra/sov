#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>

#include "gen.c"
#include "ku_connector_wayland.c"
#include "ku_text.c"
#include "mt_log.c"
#include "mt_path.c"
#include "mt_string.c"
#include "mt_vector.c"
#include "tree.c"

#define GET_WORKSPACES_CMD "swaymsg -t get_workspaces"
#define GET_TREE_CMD "swaymsg -t get_tree"
#define CFG_PATH_LOC "~/.config/sov"

struct sov
{
    struct monitor_info** monitors;
    int                   monitor_count;

    mt_vector_t* workspaces;
    mt_vector_t* wlwindows;

    int use_ws_name;
    int is_shown;
    int is_show_requested;

    uint32_t show_delay;
    uint32_t holdkey;

    char* cfg_path;
    char* img_path;
    char* css_path;
    char* html_path;

    int ratio;
    int margin;
    int columns;

    char* anchor;

} sov = {0};

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

/* calculates overview dimensions for each monitor and creates layers */

void create_layers()
{
    sov.is_shown = 1;

    mt_vector_reset(sov.workspaces);
    mt_vector_reset(sov.wlwindows);

    sov_read_tree(sov.workspaces);

    for (int m = 0; m < sov.monitor_count; m++)
    {
	struct monitor_info* monitor = sov.monitors[m];

	mt_vector_t* workspaces = VNEW(); // REL 1

	for (size_t index = 0; index < sov.workspaces->length; index++)
	{
	    sway_workspace_t* ws = sov.workspaces->data[index];
	    if (strcmp(ws->output, monitor->name) == 0) VADD(workspaces, ws);
	}

	/* calculate full width */

	int cols = sov.columns;
	int rows = (int) ceilf((float) workspaces->length / cols);

	int width  = 0;
	int height = 0;

	gen_calc_size(
	    monitor->logical_width / sov.ratio,
	    monitor->logical_height / sov.ratio,
	    1.0,
	    cols,
	    rows,
	    &width,
	    &height);

	mt_log_debug("Creating layer for %s : workspaces %i cols %i rows %i ratio %i width %i height %i", monitor->name, workspaces->length, cols, rows, sov.ratio, width, height);

	wl_window_t* wlwindow = ku_wayland_create_generic_layer(monitor, width, height, sov.margin, sov.anchor);
	ku_wayland_show_window(wlwindow);
	/* ku_wayland_request_frame(wlwindow); */
	VADDR(sov.wlwindows, wlwindow);

	REL(workspaces);
    }
}

/* deletes all layers */

void delete_layers()
{
    ku_wayland_set_time_event_delay(0);
    sov.is_show_requested = 0;
    sov.is_shown          = 0;

    if (sov.wlwindows->length > 0)
    {
	for (size_t w = 0; w < sov.wlwindows->length; w++)
	{
	    wl_window_t* window = sov.wlwindows->data[w];
	    ku_wayland_delete_window(window);
	}
	mt_vector_reset(sov.wlwindows);
    }
}

/* draws overview for given monitor */

void draw_window(wl_window_t* info)
{
    mt_vector_t* workspaces = VNEW();

    for (size_t index = 0; index < sov.workspaces->length; index++)
    {
	sway_workspace_t* ws = sov.workspaces->data[index];
	if (strcmp(ws->output, info->monitor->name) == 0) VADD(workspaces, ws);
    }

    int cols        = sov.columns;
    int rows        = (int) ceilf((float) workspaces->length / cols);
    int use_ws_name = sov.use_ws_name;

    mt_log_debug(
	"Drawing layer %s : workspaces %i cols %i rows %i ratio %i",
	info->monitor->name,
	workspaces->length,
	cols,
	rows,
	sov.ratio);

    ku_bitmap_reset(&info->bitmap);

    gen_render(
	info->monitor->logical_width / sov.ratio,
	info->monitor->logical_height / sov.ratio,
	(float) info->monitor->scale,
	cols,
	rows,
	use_ws_name,
	workspaces,
	&info->bitmap);

    ku_wayland_draw_window(info, 0, 0, info->width, info->height);

    REL(workspaces);
}

void init(wl_event_t event)
{
    sov.monitors      = event.monitors;
    sov.monitor_count = event.monitor_count;

    gen_init(sov.html_path, sov.css_path, sov.img_path);
}

void update(ku_event_t ev)
{
    /* time event from delaying timer, create and show layers
       and that triggers window_shown event */
    if (ev.type == KU_EVENT_TIME)
    {
	ku_wayland_set_time_event_delay(0);
	create_layers();
    }

    /* draw overview for given window */
    if (ev.type == KU_EVENT_WINDOW_SHOWN) draw_window(ev.window);

    /* if we have a holdkey and it is released delete layers */
    if (ev.type == KU_EVENT_KEY_UP && ev.keycode == sov.holdkey) delete_layers();

    if (ev.type == KU_EVENT_STDIN)
    {
	if (ev.text[0] == '0')
	{
	    if (sov.holdkey)
	    {
		/* when a combo is released and we have a hold key delete delaying timer */
		ku_wayland_set_time_event_delay(0);
		sov.is_show_requested = 0;
	    }
	    else
	    {
		/* simply delete layers by default when combo is released */
		delete_layers();
	    }
	}
	else if (ev.text[0] == '1')
	{
	    if (sov.wlwindows->length == 0 && sov.is_show_requested == 0)
	    {
		sov.is_show_requested = 1;
		if (sov.show_delay == 0)
		{
		    /* if no timeout create layers instantly */
		    create_layers();
		}
		else
		{
		    /* start displaying timer */
		    ku_wayland_set_time_event_delay(sov.show_delay);
		}
	    }
	    else if (sov.is_shown == 1)
	    {
		/* if layers are already displayed just redraw contents */
		mt_vector_reset(sov.workspaces);
		sov_read_tree(sov.workspaces);

		for (size_t w = 0; w < sov.wlwindows->length; w++)
		{
		    wl_window_t* info = sov.wlwindows->data[w];
		    draw_window(info);
		}
	    }
	}
	else if (ev.text[0] == '2')
	{
	    /* toggle layers */
	    if (sov.is_shown == 0)
	    {
		create_layers();
	    }
	    else
	    {
		delete_layers();
	    }
	}
	else if (ev.text[0] == '3')
	{
	    ku_wayland_exit();
	}
    }

    if (ev.type == KU_EVENT_MOUSE_DOWN)
    {
	/* switch to workspace when clicked */

	wl_window_t* info = ev.window;

	mt_vector_t* workspaces = VNEW(); // REL 1

	for (size_t index = 0; index < sov.workspaces->length; index++)
	{
	    sway_workspace_t* ws = sov.workspaces->data[index];
	    if (strcmp(ws->output, info->monitor->name) == 0) VADD(workspaces, ws);
	}

	int cols = sov.columns;
	int rows = (int) ceilf((float) workspaces->length / cols);

	int index = gen_calc_index(
	    info->monitor->logical_width / sov.ratio,
	    info->monitor->logical_height / sov.ratio,
	    1.0,
	    cols,
	    rows,
	    ev.x,
	    ev.y);

	char* command = STRNF(100, "swaymsg workspace %i", index + 1);
	system(command);
	REL(command);

	mt_vector_reset(sov.workspaces);
	sov_read_tree(sov.workspaces);

	for (size_t w = 0; w < sov.wlwindows->length; w++)
	{
	    wl_window_t* info = sov.wlwindows->data[w];
	    draw_window(info);
	}
    }
}

void destroy()
{
    gen_destroy();
}

int main(int argc, char** argv)
{
    mt_log_use_colors(isatty(STDERR_FILENO));
    mt_log_level_info();
    mt_time(NULL);

    printf("Sway Overview v" SOV_VERSION
	   " by Milan Toth ( www.milgra.com )\n"
	   "If you like this app try :\n"
	   "- Wayland Control Panel ( github.com/milgra/wcp)\n"
	   "- Visual Music Player (github.com/milgra/vmp)\n"
	   "- Multimedia File Manager (github.com/milgra/mmfm)\n"
	   "- SwayOS (swayos.github.io)\n"
	   "Games :\n"
	   "- Brawl (github.com/milgra/brawl)\n"
	   "- Cortex ( github.com/milgra/cortex )\n"
	   "- Termite (github.com/milgra/termite)\n\n");

    const char* usage =
	"Usage: sov [options]\n"
	"\n"
	"  -h, --help                            Show help message and quit.\n"
	"  -v                                    Increase verbosity of messages, defaults to errors and warnings only.\n"
	"  -s,                                   Location of html folder for styling.\n"
	"  -n,                                   Use workspace name instead of workspace number.\n"
	"  -c, --columns=[columns]               Number of thumbnail columns\n"
	"  -a, --anchor=[lrtb]                   Anchor window to window edge in directions, use rt for right top\n"
	"  -m, --margin=[size]                   Margin\n"
	"  -r, --ratio=[ratio]                   Thumbnail to screen ratio, positive integer\n"
	"  -t, --timeout=[millisecs]             Milliseconds to wait for showing up overlays, positive integer\n"
	"  -k, --holdkey=[keycode]               Keycode of hold key, sov won't disappear while pressed, get value with wev\n"
	"\n";

    sov.ratio   = 8;
    sov.columns = 5;

    /* parse options */

    char* cfg_par = NULL;
    char* mrg_par = NULL;
    char* anc_par = NULL;

    int c, option_index = 0;

    static struct option long_options[] = {
	{"help", no_argument, NULL, 'h'},
	{"style", required_argument, NULL, 's'},
	{"verbose", no_argument, NULL, 'v'},
	{"columns", optional_argument, NULL, 'c'},
	{"anchor", optional_argument, NULL, 'a'},
	{"margin", optional_argument, NULL, 'm'},
	{"ratio", optional_argument, NULL, 's'},
	{"timeout", optional_argument, NULL, 'r'},
	{"holdkey", optional_argument, NULL, 'k'}};

    while ((c = getopt_long(argc, argv, "vhno:r:s:a:m:t:c:k:", long_options, &option_index)) != -1)
    {
	switch (c)
	{
	    case 's':
		cfg_par = mt_string_new_cstring(optarg);
		if (errno == ERANGE || cfg_par == NULL)
		{
		    mt_log_error("Invalid config path value", ULONG_MAX);
		    return EXIT_FAILURE;
		}
		break;
	    case 't': sov.show_delay = atoi(optarg); break;
	    case 'h': printf("%s", usage); return EXIT_SUCCESS;
	    case 'v': mt_log_inc_verbosity(); break;
	    case 'a': anc_par = mt_string_new_cstring(optarg); break;
	    case 'm': mrg_par = mt_string_new_cstring(optarg); break;
	    case 'c': sov.columns = atoi(optarg); break;
	    case 'k': sov.holdkey = atoi(optarg); break;
	    case 'r': sov.ratio = atoi(optarg); break;
	    case 'n': sov.use_ws_name = 1; break;
	    default: fprintf(stderr, "%s", usage); return EXIT_FAILURE;
	}
    }

    if (anc_par)
	sov.anchor = anc_par;
    else
	sov.anchor = "";

    if (mrg_par != NULL)
    {
	sov.margin = atoi(mrg_par);
    }

    /* init config */

    char cwd[PATH_MAX];
    if (getcwd(cwd, sizeof(cwd)) == NULL) printf("Cannot get working directory\n");

    char* cfg_path_loc = cfg_par ? mt_path_new_normalize(cfg_par) : mt_path_new_normalize(CFG_PATH_LOC); // REL
    char* cfg_path_glo = STRNC(PKG_DATADIR);                                                             // REL

    if (cfg_par) REL(cfg_par);

    DIR* dir = opendir(cfg_path_loc);
    if (dir)
    {
	sov.cfg_path = cfg_path_loc;
	closedir(dir);
    }
    else
	sov.cfg_path = cfg_path_glo;

    sov.css_path  = mt_path_new_append(sov.cfg_path, "html/main.css");  // REL
    sov.html_path = mt_path_new_append(sov.cfg_path, "html/main.html"); // REL
    sov.img_path  = mt_path_new_append(sov.cfg_path, "img");            // REL

    printf("style path    : %s\n", sov.cfg_path);
    printf("css path      : %s\n", sov.css_path);
    printf("html path     : %s\n", sov.html_path);
    printf("image path    : %s\n", sov.img_path);
    printf("ratio         : %i\n", sov.ratio);
    printf("anchor        : %s\n", sov.anchor);
    printf("margin        : %i\n", sov.margin);
    printf("timeout       : %i\n", sov.show_delay);
    printf("columns       : %i\n", sov.columns);
    printf("holdkey       : %i\n", sov.holdkey);
    printf("use_name      : %s\n", sov.use_ws_name ? "true" : "false");

    sov.wlwindows  = VNEW();
    sov.workspaces = VNEW();

    /* init text rendering */

    ku_text_init(); // DESTROY

    ku_wayland_init(init, update, destroy, 0);

    /* cleanup */

    REL(cfg_path_glo);
    REL(cfg_path_loc);
    REL(sov.css_path);
    REL(sov.html_path);
    REL(sov.img_path);

    REL(sov.wlwindows);
    REL(sov.workspaces);

    ku_text_destroy();

#ifdef MT_MEMORY_DEBUG
    mt_memory_stats();
#endif
}
