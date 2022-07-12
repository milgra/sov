/* define posix standard for strdup */
#define _POSIX_C_SOURCE 200809L

#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <limits.h>
#include <poll.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include <unistd.h>

#include "config.c"
#include "fontconfig.c"
#include "kvlines.c"
#include "text.c"
#include "tree_drawer.c"
#include "tree_reader.c"
#include "wlr-layer-shell-unstable-v1-client-protocol.h"
#include "xdg-output-unstable-v1-client-protocol.h"
#include "zc_bitmap.c"
#include "zc_channel.c"
#include "zc_cstring.c"
#include "zc_cstrpath.c"
#include "zc_graphics.c"
#include "zc_log.c"
#include "zc_vector.c"

#define CFG_PATH_LOC "~/.config/sov/config"
#define GET_WORKSPACES_CMD "swaymsg -t get_workspaces"
#define GET_TREE_CMD "swaymsg -t get_tree"

struct sov_geom
{
    unsigned long anchor;
    unsigned long margin;
};

struct sov_output_config
{
    char*          name;
    struct wl_list link;
};

struct sov_surface
{
    struct zwlr_layer_surface_v1* wlr_layer_surface;
    struct wl_surface*            wl_surface;
};

struct sov_output
{
    char*                  name;
    struct wl_list         link;
    struct wl_output*      wl_output;
    struct sov*            app;
    struct sov_surface*    sov_surface;
    struct zxdg_output_v1* xdg_output;
    uint32_t               wl_name;
    int32_t                width;
    int32_t                height;
};

struct sov
{
    char*                          cfg_path;
    char*                          font_path;
    int                            timeout;
    int                            shmid;
    struct wl_compositor*          wl_compositor;
    struct wl_display*             wl_display;
    struct wl_list                 sov_outputs;
    struct wl_list                 output_configs;
    struct wl_registry*            wl_registry;
    struct wl_shm*                 wl_shm;
    struct sov_geom                sov_geom;
    struct zwlr_layer_shell_v1*    wlr_layer_shell;
    struct zxdg_output_manager_v1* xdg_output_manager;
    struct sov_surface*            fallback_sov_surface;
};

void sov_read_tree(vec_t* workspaces)
{
    char  buff[100];
    char* ws_json   = NULL; // REL 0
    char* tree_json = NULL; // REL 1

    FILE* pipe = popen(GET_WORKSPACES_CMD, "r"); // CLOSE 0
    ws_json    = cstr_new_cstring("{\"items\":");
    while (fgets(buff, sizeof(buff), pipe) != NULL) ws_json = cstr_append(ws_json, buff);
    ws_json = cstr_append(ws_json, "}");
    pclose(pipe); // CLOSE 0

    pipe      = popen(GET_TREE_CMD, "r"); // CLOSE 0
    tree_json = cstr_new_cstring("");
    while (fgets(buff, sizeof(buff), pipe) != NULL) tree_json = cstr_append(tree_json, buff);
    pclose(pipe); // CLOSE 0

    tree_reader_extract(ws_json, tree_json, workspaces);

    REL(ws_json);   // REL 0
    REL(tree_json); // REL 1
}

void noop()
{ /* intentionally left blank */
}

int sov_shm_create()
{
    int  shmid = -1;
    char shm_name[NAME_MAX];
    for (int i = 0; i < UCHAR_MAX; ++i)
    {
	if (snprintf(shm_name, NAME_MAX, "/wob-%d", i) >= NAME_MAX)
	{
	    break;
	}
	shmid = shm_open(shm_name, O_RDWR | O_CREAT | O_EXCL, 0600);
	if (shmid > 0 || errno != EEXIST)
	{
	    break;
	}
    }

    if (shmid < 0)
    {
	zc_log_error("shm_open() failed: %s", strerror(errno));
	return -1;
    }

    if (shm_unlink(shm_name) != 0)
    {
	zc_log_error("shm_unlink() failed: %s", strerror(errno));
	return -1;
    }

    return shmid;
}

void* sov_shm_alloc(const int shmid, const size_t size)
{
    if (ftruncate(shmid, size) != 0)
    {
	zc_log_error("ftruncate() failed: %s", strerror(errno));
	return NULL;
    }

    void* buffer = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, shmid, 0);
    if (buffer == MAP_FAILED)
    {
	zc_log_error("mmap() failed: %s", strerror(errno));
	return NULL;
    }

    return buffer;
}

int sov_shm_dealloc(void* buffer, const size_t size)
{
    return munmap(buffer, size);
}

void layer_surface_configure(
    void*                         data,
    struct zwlr_layer_surface_v1* surface,
    uint32_t                      serial,
    uint32_t                      w,
    uint32_t                      h)
{
    zwlr_layer_surface_v1_ack_configure(surface, serial);
}

struct sov_surface* sov_surface_create(
    struct sov*       app,
    struct wl_output* wl_output,
    unsigned long     width,
    unsigned long     height,
    unsigned long     margin,
    unsigned long     anchor)
{
    const static struct zwlr_layer_surface_v1_listener zwlr_layer_surface_listener = {
	.configure = layer_surface_configure,
	.closed    = noop,
    };

    struct sov_surface* sov_surface = calloc(1, sizeof(struct sov_surface));
    if (sov_surface == NULL)
    {
	zc_log_error("calloc failed");
	exit(EXIT_FAILURE);
    }

    sov_surface->wl_surface = wl_compositor_create_surface(app->wl_compositor);
    if (sov_surface->wl_surface == NULL)
    {
	zc_log_error("wl_compositor_create_surface failed");
	exit(EXIT_FAILURE);
    }

    sov_surface->wlr_layer_surface = zwlr_layer_shell_v1_get_layer_surface(app->wlr_layer_shell, sov_surface->wl_surface, wl_output, ZWLR_LAYER_SHELL_V1_LAYER_OVERLAY, "sov");
    if (sov_surface->wlr_layer_surface == NULL)
    {
	zc_log_error("wlr_layer_shell_v1_get_layer_surface failed");
	exit(EXIT_FAILURE);
    }

    zwlr_layer_surface_v1_set_size(sov_surface->wlr_layer_surface, width, height);
    zwlr_layer_surface_v1_set_anchor(sov_surface->wlr_layer_surface, anchor);
    zwlr_layer_surface_v1_set_margin(sov_surface->wlr_layer_surface, margin, margin, margin, margin);
    zwlr_layer_surface_v1_add_listener(sov_surface->wlr_layer_surface, &zwlr_layer_surface_listener, app);
    wl_surface_commit(sov_surface->wl_surface);

    return sov_surface;
}

void sov_surface_destroy(
    struct sov_surface* sov_surface)
{
    if (sov_surface == NULL) { return; }

    zwlr_layer_surface_v1_destroy(sov_surface->wlr_layer_surface);
    wl_surface_destroy(sov_surface->wl_surface);

    sov_surface->wl_surface        = NULL;
    sov_surface->wlr_layer_surface = NULL;
}

void sov_output_destroy(
    struct sov_output* output)
{
    sov_surface_destroy(output->sov_surface);
    zxdg_output_v1_destroy(output->xdg_output);
    wl_output_destroy(output->wl_output);

    free(output->name);
    free(output->sov_surface);

    output->sov_surface = NULL;
    output->wl_output   = NULL;
    output->xdg_output  = NULL;
    output->name        = NULL;

    free(output);
}

void xdg_output_handle_name(
    void*                  data,
    struct zxdg_output_v1* xdg_output,
    const char*            name)
{
    zc_log_info("Detected output %s", name);

    struct sov_output* output = (struct sov_output*) data;
    output->name              = strdup(name);
    if (output->name == NULL)
    {
	zc_log_error("strdup failed\n");
	exit(EXIT_FAILURE);
    }
}

void xdg_output_handle_size(
    void*                  data,
    struct zxdg_output_v1* xdg_output,
    int32_t                width,
    int32_t                height)
{

    struct sov_output* output = (struct sov_output*) data;

    zc_log_info("Detected size for %s %i %i", output->name, width, height);

    output->width  = width;
    output->height = height;
}

void xdg_output_handle_done(
    void*                  data,
    struct zxdg_output_v1* xdg_output)
{
    struct sov_output* output = (struct sov_output*) data;

    wl_list_insert(&output->app->sov_outputs, &output->link);
    zc_log_info("Adding output %s", output->name);
}

void handle_global(
    void*               data,
    struct wl_registry* registry,
    uint32_t            name,
    const char*         interface,
    uint32_t            version)
{
    const static struct zxdg_output_v1_listener xdg_output_listener = {
	.logical_position = noop,
	.logical_size     = xdg_output_handle_size,
	.name             = xdg_output_handle_name,
	.description      = noop,
	.done             = xdg_output_handle_done,
    };

    struct sov* app = (struct sov*) data;

    if (strcmp(interface, wl_shm_interface.name) == 0)
    {
	app->wl_shm = wl_registry_bind(registry, name, &wl_shm_interface, 1);
    }
    else if (strcmp(interface, wl_compositor_interface.name) == 0)
    {
	app->wl_compositor = wl_registry_bind(registry, name, &wl_compositor_interface, 1);
    }
    else if (strcmp(interface, "wl_output") == 0)
    {
	struct sov_output* output = calloc(1, sizeof(struct sov_output));
	output->wl_output         = wl_registry_bind(registry, name, &wl_output_interface, 1);
	output->app               = app;
	output->wl_name           = name;

	output->xdg_output = zxdg_output_manager_v1_get_xdg_output(app->xdg_output_manager, output->wl_output);
	zxdg_output_v1_add_listener(output->xdg_output, &xdg_output_listener, output);

	if (wl_display_roundtrip(app->wl_display) == -1)
	{
	    zc_log_error("wl_display_roundtrip failed");
	    exit(EXIT_FAILURE);
	}
	else
	    zc_log_debug("wl_display_roundtrip success");
    }
    else if (strcmp(interface, zwlr_layer_shell_v1_interface.name) == 0)
    {
	app->wlr_layer_shell = wl_registry_bind(registry, name, &zwlr_layer_shell_v1_interface, 1);
    }
    else if (strcmp(interface, zxdg_output_manager_v1_interface.name) == 0)
    {
	app->xdg_output_manager = wl_registry_bind(registry, name, &zxdg_output_manager_v1_interface, 2);
    }
}

void handle_global_remove(
    void*               data,
    struct wl_registry* registry,
    uint32_t            name)
{
    struct sov*        app = (struct sov*) data;
    struct sov_output *output, *tmp;
    wl_list_for_each_safe(output, tmp, &(app->sov_outputs), link)
    {
	if (output->wl_name == name)
	{
	    wl_list_remove(&output->link);
	    sov_output_destroy(output);
	    break;
	}
    }
}

int sov_connect_display(struct sov* app)
{
    const static struct wl_registry_listener wl_registry_listener = {
	.global        = handle_global,
	.global_remove = handle_global_remove,
    };

    app->wl_display = wl_display_connect(NULL);
    if (app->wl_display == NULL)
    {
	zc_log_error("wl_display_connect failed");
	return EXIT_FAILURE;
    }

    app->wl_registry = wl_display_get_registry(app->wl_display);
    if (app->wl_registry == NULL)
    {
	zc_log_error("wl_display_get_registry failed");
	return EXIT_FAILURE;
    }

    wl_registry_add_listener(app->wl_registry, &wl_registry_listener, app);

    wl_list_init(&app->sov_outputs);
    if (wl_display_roundtrip(app->wl_display) == -1)
    {
	zc_log_error("wl_display_roundtrip failed");
	return EXIT_FAILURE;
    }

    return 0;
}

bool sov_parse_input(const char* input_buffer, unsigned long* state)
{
    char *input_ptr, *newline_position;

    newline_position = strchr(input_buffer, '\n');
    if (newline_position == NULL) { return false; }

    if (newline_position == input_buffer) { return false; }

    *state = strtoul(input_buffer, &input_ptr, 10);
    if (input_ptr == newline_position) { return true; }
    else
	return false;
}

int sov_show(struct sov* app)
{
    // get tree first

    vec_t* workspaces = VNEW(); // REL 0

    sov_read_tree(workspaces);

    // go through outputs

    struct sov_output *output, *tmp;
    wl_list_for_each_safe(output, tmp, &(app->sov_outputs), link)
    {
	// get workspaces for output, get biggest number and biggest ws dimension

	vec_t* curr_ws = VNEW(); // REL 1

	int wth = 0;
	int hth = 0;

	for (int index = 0; index < workspaces->length; index++)
	{
	    sway_workspace_t* ws = workspaces->data[index];
	    if (strcmp(ws->output, output->name) == 0)
	    {
		VADD(curr_ws, ws);
		if (ws->width > wth || ws->height > hth)
		{
		    wth = ws->width;
		    hth = ws->height;
		}
	    }
	}

	zc_log_debug("showing %i workspaces on output %s wth %i hth %i", curr_ws->length, output->name, wth, hth);

	textstyle_t main_style = {
	    .font       = app->font_path,
	    .margin     = config_get_int("text_margin_size"),
	    .margin_top = config_get_int("text_margin_top_size"),
	    .align      = TA_LEFT,
	    .valign     = VA_TOP,
	    .size       = config_get_int("text_title_size"),
	    .textcolor  = cstr_color_from_cstring(config_get("text_title_color")),
	    .backcolor  = 0,
	    .multiline  = 0,
	};

	textstyle_t sub_style = {
	    .font        = app->font_path,
	    .margin      = config_get_int("text_margin_size"),
	    .margin_top  = config_get_int("text_margin_top_size") + config_get_int("text_title_size"),
	    .align       = TA_LEFT,
	    .valign      = VA_TOP,
	    .size        = config_get_int("text_description_size"),
	    .textcolor   = cstr_color_from_cstring(config_get("text_description_color")),
	    .backcolor   = 0,
	    .line_height = config_get_int("text_description_size"),
	    .multiline   = 1,
	};

	textstyle_t wsnum_style = {
	    .font      = app->font_path,
	    .margin    = config_get_int("text_margin_size"),
	    .align     = TA_RIGHT,
	    .valign    = VA_TOP,
	    .size      = config_get_int("text_workspace_size"),
	    .textcolor = cstr_color_from_cstring(config_get("text_workspace_color")),
	    .backcolor = 0x00002200,
	};

	int gap   = config_get_int("gap");
	int cols  = config_get_int("columns");
	int ratio = config_get_int("ratio");

	bm_t* bitmap = tree_drawer_bm_create(
	    curr_ws,
	    gap,
	    cols,
	    ratio,
	    main_style,
	    sub_style,
	    wsnum_style,
	    cstr_color_from_cstring(config_get("window_color")),
	    cstr_color_from_cstring(config_get("background_color")),
	    cstr_color_from_cstring(config_get("background_color_focused")),
	    cstr_color_from_cstring(config_get("border_color")),
	    cstr_color_from_cstring(config_get("empty_color")),
	    cstr_color_from_cstring(config_get("empty_frame_color")),
	    config_get_int("text_workspace_xshift"),
	    config_get_int("text_workspace_yshift"));

	REL(curr_ws); // REL 1

	if (bitmap)
	{
	    struct wl_buffer* wl_buffer = NULL;

	    uint32_t size   = bitmap->w * bitmap->h * 4;
	    uint32_t stride = bitmap->w * 4;

	    struct wl_shm_pool* pool = wl_shm_create_pool(app->wl_shm, app->shmid, size);
	    if (pool == NULL)
	    {
		zc_log_error("wl_shm_create_pool failed");
		return EXIT_FAILURE;
	    }

	    wl_buffer = wl_shm_pool_create_buffer(pool, 0, bitmap->w, bitmap->h, stride, WL_SHM_FORMAT_ARGB8888);
	    wl_shm_pool_destroy(pool);

	    if (wl_buffer == NULL)
	    {
		zc_log_error("wl_shm_pool_create_buffer failed");
		return EXIT_FAILURE;
	    }

	    uint8_t* argb = sov_shm_alloc(app->shmid, size);
	    if (argb == NULL) { return EXIT_FAILURE; }

	    // memcpy((uint8_t*)app->argb, bitmap->data, bitmap->size);
	    // we have to do this until RGBA8888 is working for buffer format

	    for (int i = 0; i < bitmap->size; i += 4)
	    {
		argb[i]     = bitmap->data[i + 2];
		argb[i + 1] = bitmap->data[i + 1];
		argb[i + 2] = bitmap->data[i];
		argb[i + 3] = bitmap->data[i + 3];
	    }

	    // create surface
	    output->sov_surface = sov_surface_create(app, output->wl_output, bitmap->w, bitmap->h, app->sov_geom.margin, app->sov_geom.anchor);

	    if (wl_display_roundtrip(app->wl_display) == -1)
	    {
		zc_log_error("wl_display_roundtrip failed");
		return EXIT_FAILURE;
	    }

	    zc_log_info("Showing on output %s", output->name);

	    // flush
	    wl_surface_attach(output->sov_surface->wl_surface, wl_buffer, 0, 0);
	    wl_surface_damage(output->sov_surface->wl_surface, 0, 0, bitmap->w, bitmap->h);
	    wl_surface_commit(output->sov_surface->wl_surface);

	    /* cleanup */

	    wl_buffer_destroy(wl_buffer);

	    if (wl_display_dispatch(app->wl_display) == -1)
	    {
		zc_log_error("wl_display_dispatch failed");
		return EXIT_FAILURE;
	    }

	    REL(bitmap); // REL 1

	    sov_shm_dealloc(argb, size);
	}
    }

    REL(workspaces); // REL 0

    return 0;
}

int sov_hide(struct sov* app)
{
    struct sov_output *output, *tmp;
    wl_list_for_each_safe(output, tmp, &app->sov_outputs, link)
    {
	zc_log_info("Hiding on output %s", output->name);
	sov_surface_destroy(output->sov_surface);
	free(output->sov_surface);
	output->sov_surface = NULL;
    }

    if (wl_display_roundtrip(app->wl_display) == -1)
    {
	zc_log_error("wl_display_roundtrip failed");
	return EXIT_FAILURE;
    }

    return 0;
}

void sov_destroy(struct sov* app)
{
    struct sov_output *output, *output_tmp;
    wl_list_for_each_safe(output, output_tmp, &app->sov_outputs, link)
    {
	sov_output_destroy(output);
    }

    struct sov_output_config *config, *config_tmp;
    wl_list_for_each_safe(config, config_tmp, &app->output_configs, link)
    {
	free(config->name);
	free(config);
    }

    zwlr_layer_shell_v1_destroy(app->wlr_layer_shell);
    wl_registry_destroy(app->wl_registry);
    wl_compositor_destroy(app->wl_compositor);
    wl_shm_destroy(app->wl_shm);
    zxdg_output_manager_v1_destroy(app->xdg_output_manager);

    wl_display_disconnect(app->wl_display);
}

int main(int argc, char** argv)
{
    printf("swayoverview v" SOV_VERSION " by Milan Toth ( www.milgra.com )\n");
    printf("listens on standard input for '0' - hide panel '1' - show panel '2' - quit\n");

    zc_log_use_colors(isatty(STDERR_FILENO));
    zc_log_level_info();

    const char* usage =
	"Usage: sov [options]\n"
	"\n"
	"  -c  --config= [path]                Use config file for session.\n"
	"  -h, --help                          Show help message and quit.\n"
	"  -v                                  Increase verbosity of messages, defaults to errors and warnings only.\n"
	"\n";

    struct sov app = {0};

    /* parse options */

    int c, option_index = 0;

    wl_list_init(&(app.output_configs));

    static struct option long_options[] = {
	{"help", no_argument, NULL, 'h'},
	{"config", required_argument, NULL, 'c'},
	{"verbose", no_argument, NULL, 'v'}};

    while ((c = getopt_long(argc, argv, "c:vho:", long_options, &option_index)) != -1)
    {
	switch (c)
	{
	    case 'c':
		app.cfg_path = cstr_new_cstring(optarg);
		if (errno == ERANGE || app.cfg_path == NULL)
		{
		    zc_log_error("Invalid config path value", ULONG_MAX);
		    return EXIT_FAILURE;
		}
		break;
	    case 'h': printf("%s", usage); return EXIT_SUCCESS;
	    case 'v': zc_log_inc_verbosity(); break;
	    default: fprintf(stderr, "%s", usage); return EXIT_FAILURE;
	}
    }

    /* init config */

    config_init(); // DESTROY 0

    char cwd[PATH_MAX];
    if (getcwd(cwd, sizeof(cwd)) == NULL) printf("Cannot get working directory\n");

    char* cfg_path_loc = app.cfg_path ? cstr_new_path_normalize(app.cfg_path, cwd) : cstr_new_path_normalize(CFG_PATH_LOC, getenv("HOME")); // REL 1
    char* cfg_path_glo = cstr_new_path_append(PKG_DATADIR, "config");                                                                       // REL 2

    if (config_read(cfg_path_loc) < 0)
    {
	if (config_read(cfg_path_glo) < 0)
	    zc_log_warn("No configuration found ( local : %s , global : %s )\n", cfg_path_loc, cfg_path_glo);
	else
	    zc_log_info("Using config file : %s\n", cfg_path_glo);
    }
    else
	zc_log_info("Using config file : %s\n", cfg_path_loc);

    REL(cfg_path_glo); // REL 2
    REL(cfg_path_loc); // REL 1

    /* get anchor and margin values from config */

    app.sov_geom.margin = config_get_int("margin");
    char* anchor        = config_get("anchor");
    if (anchor)
    {
	if (strcmp(anchor, "left") == 0) app.sov_geom.anchor |= ZWLR_LAYER_SURFACE_V1_ANCHOR_LEFT;
	if (strcmp(anchor, "right") == 0) app.sov_geom.anchor |= ZWLR_LAYER_SURFACE_V1_ANCHOR_RIGHT;
	if (strcmp(anchor, "top") == 0) app.sov_geom.anchor |= ZWLR_LAYER_SURFACE_V1_ANCHOR_TOP;
	if (strcmp(anchor, "bottom") == 0) app.sov_geom.anchor |= ZWLR_LAYER_SURFACE_V1_ANCHOR_BOTTOM;
    }

    if (zc_log_get_level() == 2) config_describe();

    /* get timout from config */

    app.timeout = config_get_int("timeout");

    /* init text rendeing */

    text_init(); // DESTROY 1

    char* font_face = config_get("font_face");
    app.font_path   = fontconfig_new_path(font_face ? font_face : ""); // REL 3

    if (!app.font_path)
    {
	zc_log_error("No font files found.");
	return EXIT_FAILURE;
    }

    /* init wayland */

    int shmid = sov_shm_create();
    if (shmid < 0) return EXIT_FAILURE;

    app.shmid = shmid;

    if (sov_connect_display(&app) < 0) return EXIT_FAILURE;

    if (app.wl_shm == NULL ||
	app.wl_compositor == NULL ||
	app.wlr_layer_shell == NULL)
    {
	zc_log_error("Wayland compositor doesn't support all required protocols");
	return EXIT_FAILURE;
    }

    /* create display and stdin file descriptors */

    struct pollfd fds[2] = {
	{
	    .fd     = wl_display_get_fd(app.wl_display),
	    .events = POLLIN,
	},
	{
	    .fd     = STDIN_FILENO,
	    .events = POLLIN,
	},
    };

    bool showup = false;
    bool hidden = true;
    bool alive  = true;

    while (alive)
    {
	unsigned long state           = 0;
	char          input_buffer[3] = {0};
	char*         fgets_rv;

	switch (poll(fds, 2, !showup ? -1 : app.timeout))
	{
	    case -1:
	    {
		zc_log_error("poll() failed: %s", strerror(errno));

		alive = false;
		break;
	    }
	    case 0:
	    {
		if (hidden && showup)
		{
		    if (sov_show(&app) < 0) alive = false;
		    showup = false;
		    hidden = false;
		}

		break;
	    }
	    default:
	    {
		if (fds[0].revents)
		{
		    if (!(fds[0].revents & POLLIN))
		    {
			zc_log_error("WL_DISPLAY_FD unexpectedly closed, revents = %hd", fds[0].revents);
			alive = false;
			break;
		    }

		    if (wl_display_dispatch(app.wl_display) == -1)
		    {
			alive = false;
			break;
		    }
		}

		if (fds[1].revents)
		{
		    if (!(fds[1].revents & POLLIN))
		    {
			zc_log_error("STDIN unexpectedly closed, revents = %hd", fds[1].revents);
			if (!hidden) sov_hide(&app);

			alive = false;
			break;
		    }

		    fgets_rv = fgets(input_buffer, 3, stdin);

		    if (feof(stdin))
		    {
			zc_log_info("Received EOF");
			if (!hidden) sov_hide(&app);

			alive = false;
			break;
		    }

		    if (fgets_rv == NULL)
		    {
			zc_log_error("fgets() failed: %s", strerror(errno));
			if (!hidden) sov_hide(&app);

			alive = false;
			break;
		    }

		    if (!sov_parse_input(input_buffer, &state))
		    {
			zc_log_error("Received invalid input");

			if (!hidden) sov_hide(&app);

			alive = false;
			break;
		    }

		    if (state == 2)
		    {
			if (!hidden) sov_hide(&app);
			alive = false;
			break;
		    }

		    if (state == 1)
		    {
			if (showup == 0) showup = 1;
		    }

		    if (state == 0)
		    {
			if (!hidden)
			{
			    hidden = true;
			    sov_hide(&app);
			}
			showup = 0;
		    }
		}
	    }
	}
    }

    /* cleanup */

    config_destroy();
    text_destroy();
    REL(app.font_path);
    if (app.cfg_path) REL(app.cfg_path);
    sov_destroy(&app);

#ifdef DEBUG
    mem_stats();
#endif
}
