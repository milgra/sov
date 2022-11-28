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
#define CFG_PATH_LOC "~/.config/sov/config"

struct sov
{
    mt_vector_t* workspaces; /* workspaces for current frame draw event */

    int timeout;
    int request; /* show requested */

    mt_vector_t* wlwindows; /* wayland layers for each output */

    struct monitor_info** monitors; /* available monitors */
    int                   monitor_count;

    char* cfg_path;
    char* img_path;
    char* css_path;
    char* html_path;

    int   columns;
    int   ratio;
    char* anchor;
    int   margin;
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

void init(wl_event_t event)
{
    sov.monitors      = event.monitors;
    sov.monitor_count = event.monitor_count;

    gen_init(sov.html_path, sov.css_path, sov.img_path);
}

void create_layers()
{
    ku_wayland_set_time_event_delay(0);

    if (sov.workspaces == NULL) sov.workspaces = VNEW(); // REL 0
    mt_vector_reset(sov.workspaces);
    mt_vector_reset(sov.wlwindows);

    sov_read_tree(sov.workspaces);

    // show layer over all workspaces

    for (int m = 0; m < sov.monitor_count; m++)
    {
	struct monitor_info* monitor = sov.monitors[m];

	mt_vector_t* workspaces = VNEW(); // REL 1

	for (int index = 0; index < sov.workspaces->length; index++)
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

	wl_window_t* wlwindow = ku_wayland_create_generic_layer(monitor, width, height, sov.margin, sov.anchor, 1);
	VADDR(sov.wlwindows, wlwindow);

	REL(workspaces);
    }
}

/* window update */

void update(ku_event_t ev)
{
    if (ev.type == KU_EVENT_FRAME)
    {
	wl_window_t* info = ev.window;

	mt_vector_t* workspaces = VNEW();

	for (int index = 0; index < sov.workspaces->length; index++)
	{
	    sway_workspace_t* ws = sov.workspaces->data[index];
	    if (strcmp(ws->output, info->monitor->name) == 0) VADD(workspaces, ws);
	}

	int cols = sov.columns;
	int rows = (int) ceilf((float) workspaces->length / cols);

	mt_log_debug(
	    "Drawing layer %s : workspaces %i cols %i rows %i ratio %i",
	    info->monitor->name,
	    workspaces->length,
	    cols,
	    rows,
	    sov.ratio);

	gen_render(
	    info->monitor->logical_width / sov.ratio,
	    info->monitor->logical_height / sov.ratio,
	    (float) info->monitor->scale,
	    cols,
	    rows,
	    workspaces,
	    &info->bitmap);

	ku_wayland_draw_window(info, 0, 0, info->width, info->height);

	REL(workspaces);
    }

    if (ev.type == KU_EVENT_TIME)
    {
	create_layers();
    }

    if (ev.type == KU_EVENT_STDIN)
    {
	if (ev.text[0] == '0')
	{
	    ku_wayland_set_time_event_delay(0);
	    sov.request = 0;

	    if (sov.wlwindows->length > 0)
	    {
		for (int w = 0; w < sov.wlwindows->length; w++)
		{
		    wl_window_t* window = sov.wlwindows->data[w];
		    ku_wayland_delete_window(window);
		}
		mt_vector_reset(sov.wlwindows);
	    }
	}
	else if (ev.text[0] == '1' && sov.wlwindows->length == 0 && sov.request == 0)
	{
	    sov.request = 1;
	    if (sov.timeout == 0)
		create_layers();
	    else
		ku_wayland_set_time_event_delay(sov.timeout);
	}
	else if (ev.text[0] == '2')
	{
	    ku_wayland_exit();
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
	   "- Wayland Control Panel ( github.com/milgra/wcp )\n"
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
	"  -c, --columns=[columns]               Number of thumbnail columns\n"
	"  -a, --anchor=[lrtp]                   Anchor window to window edge in directions, use rt for right top\n"
	"  -m, --margin=[size]                   Margin\n"
	"  -r, --ratio=[ratio]                   Thumbnail to screen ratio, positive integer\n"
	"  -t, --timeout=[millisecs]             Milliseconds to wait for showing up overlays, positive integer\n"
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
	{"timeout", optional_argument, NULL, 'r'}};

    while ((c = getopt_long(argc, argv, "vho:r:s:a:m:t:c:", long_options, &option_index)) != -1)
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
	    case 't': sov.timeout = atoi(optarg); break;
	    case 'h': printf("%s", usage); return EXIT_SUCCESS;
	    case 'v': mt_log_inc_verbosity(); break;
	    case 'a': anc_par = mt_string_new_cstring(optarg); break;
	    case 'm': mrg_par = mt_string_new_cstring(optarg); break;
	    case 'c': sov.columns = atoi(optarg); break;
	    case 'r': sov.ratio = atoi(optarg); break;
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

    char* cfg_path_loc = cfg_par ? mt_path_new_normalize(cfg_par, cwd) : mt_path_new_normalize(CFG_PATH_LOC, getenv("HOME")); // REL 1
    char* cfg_path_glo = mt_path_new_append(PKG_DATADIR, "config");                                                           // REL 2

    if (cfg_par) REL(cfg_par);

    DIR* dir = opendir(cfg_path_loc);
    if (dir)
    {
	sov.cfg_path = cfg_path_loc;
	closedir(dir);
    }
    else
	sov.cfg_path = cfg_path_glo;

    sov.css_path  = mt_path_new_append(sov.cfg_path, "html/main.css");  // REL 6
    sov.html_path = mt_path_new_append(sov.cfg_path, "html/main.html"); // REL 7
    sov.img_path  = mt_path_new_append(sov.cfg_path, "img");            // REL 6

    printf("style path    : %s\n", sov.cfg_path);
    printf("css path      : %s\n", sov.css_path);
    printf("html path     : %s\n", sov.html_path);
    printf("image path    : %s\n", sov.img_path);
    printf("ratio         : %i\n", sov.ratio);
    printf("anchor        : %s\n", sov.anchor);
    printf("margin        : %i\n", sov.margin);
    printf("timeout       : %i\n", sov.timeout);
    printf("columns       : %i\n", sov.columns);

    sov.wlwindows = VNEW();

    /* init text rendering */

    ku_text_init(); // DESTROY 1

    ku_wayland_init(init, update, destroy, 0);

    /* cleanup */

    REL(cfg_path_glo); // REL 2
    REL(cfg_path_loc); // REL 1
    REL(sov.css_path);
    REL(sov.html_path);
    REL(sov.img_path);

    ku_text_destroy();

#ifdef MT_MEMORY_DEBUG
    mt_memory_stats();
#endif
}
