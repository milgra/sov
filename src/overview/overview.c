#define WOB_FILE "main.c"

#define WOB_DEFAULT_WIDTH 400
#define WOB_DEFAULT_HEIGHT 50
#define WOB_DEFAULT_BORDER_OFFSET 0
#define WOB_DEFAULT_BORDER_SIZE 0
#define WOB_DEFAULT_BAR_PADDING 0
#define WOB_DEFAULT_ANCHOR 0
#define WOB_DEFAULT_MARGIN 0
#define WOB_DEFAULT_MAXIMUM 100
#define WOB_DEFAULT_TIMEOUT 200

#define MIN_PERCENTAGE_BAR_WIDTH 1
#define MIN_PERCENTAGE_BAR_HEIGHT 1

#define STR(x) #x

// sizeof already includes NULL byte
#define INPUT_BUFFER_LENGTH (3 * sizeof(unsigned long) + sizeof(" #000000FF #FFFFFFFF #FFFFFFFF\n"))

#define _POSIX_C_SOURCE 200809L
#include <errno.h>
#include <getopt.h>
#include <limits.h>
#include <poll.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "buffer.h"
#include "color.h"
#include "log.h"
#include "wlr-layer-shell-unstable-v1-client-protocol.h"
#include "xdg-output-unstable-v1-client-protocol.h"

enum wob_overflow_mode
{
  OVERFLOW_MODE_NONE,
  OVERFLOW_MODE_WRAP,
  OVERFLOW_MODE_NOWRAP,
};

struct wob_geom
{
  unsigned long width;
  unsigned long height;
  unsigned long border_offset;
  unsigned long border_size;
  unsigned long bar_padding;
  unsigned long stride;
  unsigned long size;
  unsigned long anchor;
  unsigned long margin;
};

struct wob_output_config
{
  char*          name;
  struct wl_list link;
};

struct wob_surface
{
  struct zwlr_layer_surface_v1* wlr_layer_surface;
  struct wl_surface*            wl_surface;
};

struct wob_output
{
  char*                  name;
  struct wl_list         link;
  struct wl_output*      wl_output;
  struct wob*            app;
  struct wob_surface*    wob_surface;
  struct zxdg_output_v1* xdg_output;
  uint32_t               wl_name;
};

struct wob
{
  uint8_t*                       argb;
  int                            shmid;
  struct wl_buffer*              wl_buffer;
  struct wl_compositor*          wl_compositor;
  struct wl_display*             wl_display;
  struct wl_list                 wob_outputs;
  struct wl_list                 output_configs;
  struct wl_registry*            wl_registry;
  struct wl_shm*                 wl_shm;
  struct wob_geom*               wob_geom;
  struct zwlr_layer_shell_v1*    wlr_layer_shell;
  struct zxdg_output_manager_v1* xdg_output_manager;
  struct wob_surface*            fallback_wob_surface;
};

void noop()
{
  /* intentionally left blank */
}

void layer_surface_configure(void* data, struct zwlr_layer_surface_v1* surface, uint32_t serial, uint32_t w, uint32_t h)
{
  zwlr_layer_surface_v1_ack_configure(surface, serial);
}

void xdg_output_handle_name(void* data, struct zxdg_output_v1* xdg_output, const char* name)
{
  wob_log_info("Detected output %s", name);
  struct wob_output* output = (struct wob_output*)data;
  output->name              = strdup(name);
  if (output->name == NULL)
  {
    wob_log_error("strdup failed\n");
    exit(EXIT_FAILURE);
  }
}

#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>

#include "config.c"
#include "fontconfig.c"
#include "kvlines.c"
#include "text.c"
#include "tree_drawer.c"
#include "tree_reader.c"

#include "zc_bitmap.c"
#include "zc_channel.c"
#include "zc_cstring.c"
#include "zc_cstrpath.c"
#include "zc_vector.c"

#define CFG_PATH_LOC "~/.config/sway-overview/config"
#define CFG_PATH_GLO "/usr/share/sway-overview/config"
#define WIN_APPID "sway-overview"
#define WIN_TITLE "sway-overview"
#define GET_WORKSPACES_CMD "swaymsg -t get_workspaces"
#define GET_TREE_CMD "swaymsg -t get_tree"

char* font_path;

void read_tree(vec_t* workspaces)
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

void wob_flush(struct wob* app)
{

  if (wl_list_empty(&(app->wob_outputs)))
  {
    wl_surface_attach(app->fallback_wob_surface->wl_surface, app->wl_buffer, 0, 0);
    wl_surface_damage(app->fallback_wob_surface->wl_surface, 0, 0, app->wob_geom->width, app->wob_geom->height);
    wl_surface_commit(app->fallback_wob_surface->wl_surface);
  }
  else
  {
    struct wob_output *output, *tmp;
    wl_list_for_each_safe(output, tmp, &(app->wob_outputs), link)
    {
      wl_surface_attach(output->wob_surface->wl_surface, app->wl_buffer, 0, 0);
      wl_surface_damage(output->wob_surface->wl_surface, 0, 0, app->wob_geom->width, app->wob_geom->height);
      wl_surface_commit(output->wob_surface->wl_surface);
    }
  }

  wl_buffer_destroy(app->wl_buffer);
  app->wl_buffer = NULL;

  if (wl_display_dispatch(app->wl_display) == -1)
  {
    wob_log_error("wl_display_dispatch failed");
    exit(EXIT_FAILURE);
  }
}

struct wob_surface*
wob_surface_create(struct wob* app, struct wl_output* wl_output)
{
  vec_t* workspaces = VNEW(); // REL 6
  bm_t*  bitmap     = NULL;

  read_tree(workspaces);

  sway_workspace_t* ws  = workspaces->data[0];
  sway_workspace_t* wsl = workspaces->data[workspaces->length - 1];

  if (ws->width > 0 && ws->height > 0)
  {
    int gap   = config_get_int("gap");
    int cols  = config_get_int("columns");
    int rows  = (int)ceilf((float)wsl->number / cols);
    int ratio = config_get_int("ratio");

    int lay_wth = cols * (ws->width / ratio) + (cols + 1) * gap;
    int lay_hth = rows * (ws->height / ratio) + (rows + 1) * gap;

    bitmap = bm_new(lay_wth, lay_hth); // REL 5
    // TODO set this from config
    gfx_rect(bitmap, 0, 0, bitmap->w, bitmap->h, 0x00000077, 0);

    textstyle_t main_style = {
        .font       = font_path,
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
        .font        = font_path,
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
        .font      = font_path,
        .margin    = config_get_int("text_margin_size"),
        .align     = TA_RIGHT,
        .valign    = VA_TOP,
        .size      = config_get_int("text_workspace_size"),
        .textcolor = cstr_color_from_cstring(config_get("text_workspace_color")),
        .backcolor = 0x00002200,
    };

    tree_drawer_draw(bitmap,
                     workspaces,
                     gap,
                     cols,
                     ratio,
                     main_style,
                     sub_style,
                     wsnum_style,
                     cstr_color_from_cstring(config_get("background_color")),
                     cstr_color_from_cstring(config_get("background_color_focused")),
                     cstr_color_from_cstring(config_get("border_color")),
                     config_get_int("text_workspace_xshift"),
                     config_get_int("text_workspace_yshift"));

    app->wob_geom->width  = bitmap->w;
    app->wob_geom->height = bitmap->h;
    app->wob_geom->size   = bitmap->w * bitmap->h * 4;
    app->wob_geom->stride = bitmap->w * 4;

    if (app->wl_buffer == NULL)
    {
      struct wl_shm_pool* pool = wl_shm_create_pool(app->wl_shm, app->shmid, app->wob_geom->size);
      if (pool == NULL)
      {
        wob_log_error("wl_shm_create_pool failed");
        exit(EXIT_FAILURE);
      }

      app->wl_buffer = wl_shm_pool_create_buffer(pool, 0, app->wob_geom->width, app->wob_geom->height, app->wob_geom->stride, WL_SHM_FORMAT_ARGB8888);
      wl_shm_pool_destroy(pool);
      if (app->wl_buffer == NULL)
      {
        wob_log_error("wl_shm_pool_create_buffer failed");
        exit(EXIT_FAILURE);
      }
    }

    if (app->argb == NULL)
    {
      app->argb = wob_shm_alloc(app->shmid, app->wob_geom->size);
    }

    if (app->argb == NULL)
    {
      return EXIT_FAILURE;
    }

    // memcpy((uint8_t*)app->argb, bitmap->data, bitmap->size);
    // we have to do this until RGBA8888 is working for buffer format

    for (int i = 0; i < bitmap->size; i += 4)
    {
      app->argb[i]     = bitmap->data[i + 2];
      app->argb[i + 1] = bitmap->data[i + 1];
      app->argb[i + 2] = bitmap->data[i];
      app->argb[i + 3] = bitmap->data[i + 3];
    }
  }

  const static struct zwlr_layer_surface_v1_listener zwlr_layer_surface_listener = {
      .configure = layer_surface_configure,
      .closed    = noop,
  };

  struct wob_surface* wob_surface = calloc(1, sizeof(struct wob_surface));
  if (wob_surface == NULL)
  {
    wob_log_error("calloc failed");
    exit(EXIT_FAILURE);
  }

  wob_surface->wl_surface = wl_compositor_create_surface(app->wl_compositor);
  if (wob_surface->wl_surface == NULL)
  {
    wob_log_error("wl_compositor_create_surface failed");
    exit(EXIT_FAILURE);
  }

  wob_surface->wlr_layer_surface = zwlr_layer_shell_v1_get_layer_surface(app->wlr_layer_shell, wob_surface->wl_surface, wl_output, ZWLR_LAYER_SHELL_V1_LAYER_OVERLAY, "wob");
  if (wob_surface->wlr_layer_surface == NULL)
  {
    wob_log_error("wlr_layer_shell_v1_get_layer_surface failed");
    exit(EXIT_FAILURE);
  }

  zwlr_layer_surface_v1_set_size(wob_surface->wlr_layer_surface, app->wob_geom->width, app->wob_geom->height);
  zwlr_layer_surface_v1_set_anchor(wob_surface->wlr_layer_surface, app->wob_geom->anchor);
  zwlr_layer_surface_v1_set_margin(wob_surface->wlr_layer_surface, app->wob_geom->margin, app->wob_geom->margin, app->wob_geom->margin, app->wob_geom->margin);
  zwlr_layer_surface_v1_add_listener(wob_surface->wlr_layer_surface, &zwlr_layer_surface_listener, app);
  wl_surface_commit(wob_surface->wl_surface);

  return wob_surface;
}

void wob_surface_destroy(struct wob_surface* wob_surface)
{
  if (wob_surface == NULL)
  {
    return;
  }

  zwlr_layer_surface_v1_destroy(wob_surface->wlr_layer_surface);
  wl_surface_destroy(wob_surface->wl_surface);

  wob_surface->wl_surface        = NULL;
  wob_surface->wlr_layer_surface = NULL;
}

void wob_output_destroy(struct wob_output* output)
{
  wob_surface_destroy(output->wob_surface);
  zxdg_output_v1_destroy(output->xdg_output);
  wl_output_destroy(output->wl_output);

  free(output->name);
  free(output->wob_surface);

  output->wob_surface = NULL;
  output->wl_output   = NULL;
  output->xdg_output  = NULL;
  output->name        = NULL;
}

void xdg_output_handle_done(void* data, struct zxdg_output_v1* xdg_output)
{
  struct wob_output* output = (struct wob_output*)data;
  struct wob*        app    = output->app;

  struct wob_output_config *output_config, *tmp;
  wl_list_for_each_safe(output_config, tmp, &app->output_configs, link)
  {
    if (strcmp(output->name, output_config->name) == 0 || strcmp("*", output_config->name) == 0)
    {
      wl_list_insert(&output->app->wob_outputs, &output->link);
      wob_log_info("Bar will be displayed on output %s", output->name);
      return;
    }
  }

  wob_log_info("Bar will NOT be displayed on output %s", output->name);

  wob_output_destroy(output);
  free(output);
}

void handle_global(void* data, struct wl_registry* registry, uint32_t name, const char* interface, uint32_t version)
{
  const static struct zxdg_output_v1_listener xdg_output_listener = {
      .logical_position = noop,
      .logical_size     = noop,
      .name             = xdg_output_handle_name,
      .description      = noop,
      .done             = xdg_output_handle_done,
  };

  struct wob* app = (struct wob*)data;

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
    if (!wl_list_empty(&(app->output_configs)))
    {
      struct wob_output* output = calloc(1, sizeof(struct wob_output));
      output->wl_output         = wl_registry_bind(registry, name, &wl_output_interface, 1);
      output->app               = app;
      output->wl_name           = name;

      output->xdg_output = zxdg_output_manager_v1_get_xdg_output(app->xdg_output_manager, output->wl_output);
      zxdg_output_v1_add_listener(output->xdg_output, &xdg_output_listener, output);

      if (wl_display_roundtrip(app->wl_display) == -1)
      {
        wob_log_error("wl_display_roundtrip failed");
        exit(EXIT_FAILURE);
      }
    }
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

void handle_global_remove(void* data, struct wl_registry* registry, uint32_t name)
{
  struct wob*        app = (struct wob*)data;
  struct wob_output *output, *tmp;
  wl_list_for_each_safe(output, tmp, &(app->wob_outputs), link)
  {
    if (output->wl_name == name)
    {
      wob_output_destroy(output);
      break;
    }
  }
}

void wob_hide(struct wob* app)
{
  if (wl_list_empty(&(app->wob_outputs)))
  {
    wob_log_info("Hiding bar on focused output");
    wob_surface_destroy(app->fallback_wob_surface);
    free(app->fallback_wob_surface);
    app->fallback_wob_surface = NULL;
  }
  else
  {
    struct wob_output *output, *tmp;
    wl_list_for_each_safe(output, tmp, &app->wob_outputs, link)
    {
      wob_log_info("Hiding bar on output %s", output->name);
      wob_surface_destroy(output->wob_surface);
      free(output->wob_surface);
      output->wob_surface = NULL;
    }
  }

  if (wl_display_roundtrip(app->wl_display) == -1)
  {
    wob_log_error("wl_display_roundtrip failed");
    exit(EXIT_FAILURE);
  }
}

void wob_show(struct wob* app)
{
  if (wl_list_empty(&(app->wob_outputs)))
  {
    wob_log_info("No output matching configuration found, fallbacking to focused output");
    app->fallback_wob_surface = wob_surface_create(app, NULL);
  }
  else
  {
    struct wob_output *output, *tmp;
    wl_list_for_each_safe(output, tmp, &app->wob_outputs, link)
    {
      wob_log_info("Showing bar on output %s", output->name);
      output->wob_surface = wob_surface_create(app, output->wl_output);
    }
  }

  if (wl_display_roundtrip(app->wl_display) == -1)
  {
    wob_log_error("wl_display_roundtrip failed");
    exit(EXIT_FAILURE);
  }
}

void wob_destroy(struct wob* app)
{
  struct wob_output *output, *output_tmp;
  wl_list_for_each_safe(output, output_tmp, &app->wob_outputs, link)
  {
    wob_output_destroy(output);
    free(output);
  }

  struct wob_output_config *config, *config_tmp;
  wl_list_for_each_safe(config, config_tmp, &app->output_configs, link)
  {
    free(config->name);
    free(config);
  }

  zwlr_layer_shell_v1_destroy(app->wlr_layer_shell);
  wl_registry_destroy(app->wl_registry);
  wl_buffer_destroy(app->wl_buffer);
  wl_compositor_destroy(app->wl_compositor);
  wl_shm_destroy(app->wl_shm);
  zxdg_output_manager_v1_destroy(app->xdg_output_manager);

  wl_display_disconnect(app->wl_display);
}

void wob_connect(struct wob* app)
{
  const static struct wl_registry_listener wl_registry_listener = {
      .global        = handle_global,
      .global_remove = noop,
  };

  app->wl_display = wl_display_connect(NULL);
  if (app->wl_display == NULL)
  {
    wob_log_error("wl_display_connect failed");
    exit(EXIT_FAILURE);
  }

  app->wl_registry = wl_display_get_registry(app->wl_display);
  if (app->wl_registry == NULL)
  {
    wob_log_error("wl_display_get_registry failed");
    exit(EXIT_FAILURE);
  }

  wl_registry_add_listener(app->wl_registry, &wl_registry_listener, app);

  wl_list_init(&app->wob_outputs);
  if (wl_display_roundtrip(app->wl_display) == -1)
  {
    wob_log_error("wl_display_roundtrip failed");
    exit(EXIT_FAILURE);
  }
}

bool wob_parse_input(const char* input_buffer, unsigned long* state)
{
  char *input_ptr, *newline_position, *str_end;

  newline_position = strchr(input_buffer, '\n');
  if (newline_position == NULL)
  {
    return false;
  }

  if (newline_position == input_buffer)
  {
    return false;
  }

  *state = strtoul(input_buffer, &input_ptr, 10);
  if (input_ptr == newline_position)
  {
    return true;
  }
  else
    return false;
}

int main(int argc, char** argv)
{
  wob_log_use_colors(isatty(STDERR_FILENO));
  wob_log_level_info();

  const char* usage =
      "Usage: wob [options]\n"
      "\n"
      "  -h, --help                          Show help message and quit.\n"
      "  --version                           Show the version number and quit.\n"
      "  -v                                  Increase verbosity of messages, defaults to errors and warnings only\n"
      "  -t, --timeout <ms>                  Hide wob after <ms> milliseconds, defaults to " STR(WOB_DEFAULT_TIMEOUT) ".\n"
                                                                                                                      "  -m, --max <%>                       Define the maximum percentage, defaults to " STR(WOB_DEFAULT_MAXIMUM) ". \n"
                                                                                                                                                                                                                                   "  -W, --width <px>                    Define bar width in pixels, defaults to " STR(WOB_DEFAULT_WIDTH) ". \n"
                                                                                                                                                                                                                                                                                                                                           "  -H, --height <px>                   Define bar height in pixels, defaults to " STR(WOB_DEFAULT_HEIGHT) ". \n"
                                                                                                                                                                                                                                                                                                                                                                                                                                                     "  -o, --offset <px>                   Define border offset in pixels, defaults to " STR(WOB_DEFAULT_BORDER_OFFSET) ". \n"
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                         "  -b, --border <px>                   Define border size in pixels, defaults to " STR(WOB_DEFAULT_BORDER_SIZE) ". \n"
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                         "  -p, --padding <px>                  Define bar padding in pixels, defaults to " STR(WOB_DEFAULT_BAR_PADDING) ". \n"
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                         "  -a, --anchor <s>                    Define anchor point; one of 'top', 'left', 'right', 'bottom', 'center' (default). \n"
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                         "                                      May be specified multiple times. \n"
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                         "  -M, --margin <px>                   Define anchor margin in pixels, defaults to " STR(WOB_DEFAULT_MARGIN) ". \n"
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                      "  -O, --output <name>                 Define output to show bar on or '*' for all. If ommited, focused output is chosen.\n"
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                      "                                      May be specified multiple times.\n"
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                      "  --border-color <#rgba>              Define border color\n"
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                      "  --background-color <#rgba>          Define background color\n"
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                      "  --bar-color <#rgba>                 Define bar color\n"
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                      "  --overflow-mode <mode>              Change the overflow behavior. Valid options are `none`, `wrap` (default), and `nowrap`.\n"
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                      "  --overflow-bar-color <#rgba>        Define bar color when overflowed\n"
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                      "  --overflow-border-color <#rgba>     Define the border color when overflowed\n"
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                      "  --overflow-background-color <#rgba> Define the background color when overflowed\n"
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                      "\n";

  struct wob app = {0};
  wl_list_init(&(app.output_configs));

  char*                  cfg_path      = NULL;
  unsigned long          maximum       = WOB_DEFAULT_MAXIMUM;
  unsigned long          timeout_msec  = WOB_DEFAULT_TIMEOUT;
  enum wob_overflow_mode overflow_mode = OVERFLOW_MODE_WRAP;
  struct wob_geom        geom          = {
                      .width         = WOB_DEFAULT_WIDTH,
                      .height        = WOB_DEFAULT_HEIGHT,
                      .border_offset = WOB_DEFAULT_BORDER_OFFSET,
                      .border_size   = WOB_DEFAULT_BORDER_SIZE,
                      .bar_padding   = WOB_DEFAULT_BAR_PADDING,
                      .anchor        = WOB_DEFAULT_ANCHOR,
                      .margin        = WOB_DEFAULT_MARGIN,
  };

  struct wob_output_config* output_config;
  int                       option_index = 0;
  int                       c;
  char*                     strtoul_end;
  static struct option      long_options[] = {
      {"help", no_argument, NULL, 'h'},
      {"version", no_argument, NULL, 4},
      {"config", required_argument, NULL, 'c'},
      {"timeout", required_argument, NULL, 't'},
      {"max", required_argument, NULL, 'm'},
      {"width", required_argument, NULL, 'W'},
      {"height", required_argument, NULL, 'H'},
      {"offset", required_argument, NULL, 'o'},
      {"border", required_argument, NULL, 'b'},
      {"padding", required_argument, NULL, 'p'},
      {"anchor", required_argument, NULL, 'a'},
      {"margin", required_argument, NULL, 'M'},
      {"output", required_argument, NULL, 'O'},
      {"border-color", required_argument, NULL, 1},
      {"background-color", required_argument, NULL, 2},
      {"bar-color", required_argument, NULL, 3},
      {"verbose", no_argument, NULL, 'v'},
      {"overflow-mode", required_argument, NULL, 6},
      {"overflow-bar-color", required_argument, NULL, 5},
      {"overflow-background-color", required_argument, NULL, 7},
      {"overflow-border-color", required_argument, NULL, 8}};

  while ((c = getopt_long(argc, argv, "c:t:m:W:H:o:b:p:a:M:O:vh:f", long_options, &option_index)) != -1)
  {
    switch (c)
    {
    /* case 1: */
    /*   if (!wob_parse_color(optarg, &strtoul_end, &(colors.border))) */
    /*   { */
    /*     wob_log_error("Border color must be a value between #00000000 and #FFFFFFFF."); */
    /*     return EXIT_FAILURE; */
    /*   } */
    /*   break; */
    case 'c':
      cfg_path = cstr_new_cstring(optarg);
      if (errno == ERANGE || cfg_path == NULL)
      {
        wob_log_error("Invalid config path value", ULONG_MAX);
        return EXIT_FAILURE;
      }
      break;
    case 't':
      timeout_msec = strtoul(optarg, &strtoul_end, 10);
      if (*strtoul_end != '\0' || errno == ERANGE || timeout_msec == 0)
      {
        wob_log_error("Timeout must be a value between 1 and %lu.", ULONG_MAX);
        return EXIT_FAILURE;
      }
      break;
    case 'm':
      maximum = strtoul(optarg, &strtoul_end, 10);
      if (*strtoul_end != '\0' || errno == ERANGE || maximum == 0)
      {
        wob_log_error("Maximum must be a value between 1 and %lu.", ULONG_MAX);
        return EXIT_FAILURE;
      }
      break;
    case 'W':
      geom.width = strtoul(optarg, &strtoul_end, 10);
      if (*strtoul_end != '\0' || errno == ERANGE)
      {
        wob_log_error("Width must be a positive value.");
        return EXIT_FAILURE;
      }
      break;
    case 'H':
      geom.height = strtoul(optarg, &strtoul_end, 10);
      if (*strtoul_end != '\0' || errno == ERANGE)
      {
        wob_log_error("Height must be a positive value.");
        return EXIT_FAILURE;
      }
      break;
    case 'o':
      geom.border_offset = strtoul(optarg, &strtoul_end, 10);
      if (*strtoul_end != '\0' || errno == ERANGE)
      {
        wob_log_error("Border offset must be a positive value.");
        return EXIT_FAILURE;
      }
      break;
    case 'b':
      geom.border_size = strtoul(optarg, &strtoul_end, 10);
      if (*strtoul_end != '\0' || errno == ERANGE)
      {
        wob_log_error("Border size must be a positive value.");
        return EXIT_FAILURE;
      }
      break;
    case 'p':
      geom.bar_padding = strtoul(optarg, &strtoul_end, 10);
      if (*strtoul_end != '\0' || errno == ERANGE)
      {
        wob_log_error("Bar padding must be a positive value.");
        return EXIT_FAILURE;
      }
      break;
    case 'a':
      if (strcmp(optarg, "left") == 0)
      {
        geom.anchor |= ZWLR_LAYER_SURFACE_V1_ANCHOR_LEFT;
      }
      else if (strcmp(optarg, "right") == 0)
      {
        geom.anchor |= ZWLR_LAYER_SURFACE_V1_ANCHOR_RIGHT;
      }
      else if (strcmp(optarg, "top") == 0)
      {
        geom.anchor |= ZWLR_LAYER_SURFACE_V1_ANCHOR_TOP;
      }
      else if (strcmp(optarg, "bottom") == 0)
      {
        geom.anchor |= ZWLR_LAYER_SURFACE_V1_ANCHOR_BOTTOM;
      }
      else if (strcmp(optarg, "center") != 0)
      {
        wob_log_error("Anchor must be one of 'top', 'bottom', 'left', 'right', 'center'.");
        return EXIT_FAILURE;
      }
      break;
    case 'M':
      geom.margin = strtoul(optarg, &strtoul_end, 10);
      if (*strtoul_end != '\0' || errno == ERANGE)
      {
        wob_log_error("Anchor margin must be a positive value.");
        return EXIT_FAILURE;
      }
      break;
    case 'O':
      output_config = calloc(1, sizeof(struct wob_output_config));
      if (output_config == NULL)
      {
        wob_log_error("calloc failed");
        return EXIT_FAILURE;
      }

      output_config->name = strdup(optarg);
      if (output_config->name == NULL)
      {
        free(output_config);
        wob_log_error("strdup failed");
        return EXIT_FAILURE;
      }

      wl_list_insert(&(app.output_configs), &(output_config->link));
      break;
    case 4:
      printf("wob version: " WOB_VERSION "\n");
      return EXIT_SUCCESS;
    case 'h':
      printf("%s", usage);
      return EXIT_SUCCESS;
    case 'v':
      wob_log_inc_verbosity();
      break;
    case 6:
      if (strcmp(optarg, "none") == 0)
      {
        overflow_mode = OVERFLOW_MODE_NONE;
      }
      else if (strcmp(optarg, "wrap") == 0)
      {
        overflow_mode = OVERFLOW_MODE_WRAP; // this is the default
      }
      else if (strcmp(optarg, "nowrap") == 0)
      {
        overflow_mode = OVERFLOW_MODE_NOWRAP;
      }
      else
      {
        wob_log_error("Invalid argument for overflow-mode. Valid options are none, wrap, and nowrap.");
        return EXIT_FAILURE;
      }
      break;
    default:
      fprintf(stderr, "%s", usage);
      return EXIT_FAILURE;
    }
  }

  if (geom.width < MIN_PERCENTAGE_BAR_WIDTH + 2 * (geom.border_offset + geom.border_size + geom.bar_padding))
  {
    wob_log_error("Invalid geometry: width is too small for given parameters");
    return EXIT_FAILURE;
  }

  if (geom.height < MIN_PERCENTAGE_BAR_HEIGHT + 2 * (geom.border_offset + geom.border_size + geom.bar_padding))
  {
    wob_log_error("Invalid geometry: height is too small for given parameters");
    return EXIT_FAILURE;
  }

  ////

  /* init config */

  config_init(); // DESTROY 0

  char cwd[PATH_MAX];
  if (getcwd(cwd, sizeof(cwd)) == NULL) printf("Cannot get working directory\n");

  char* cfg_path_loc = cfg_path ? cstr_new_path_normalize(cfg_path, cwd) : cstr_new_path_normalize(CFG_PATH_LOC, getenv("HOME")); // REL 2
  char* cfg_path_glo = cstr_new_cstring(CFG_PATH_GLO);                                                                            // REL 3

  if (config_read(cfg_path_loc) < 0)
  {
    if (config_read(cfg_path_glo) < 0)
      wob_log_warn("No local or global config file found\n");
    else
      wob_log_info("Using config file : %s\n", cfg_path_glo);
  }
  else
    wob_log_info("Using config file : %s\n", cfg_path_loc);

  REL(cfg_path_glo); // REL 3
  REL(cfg_path_loc); // REL 2

  if (wob_log_get_level() == 2) config_describe();

  /* init text rendeing */

  text_init(); // DESTROY 1

  char* font_face = config_get("font_face");
  font_path       = fontconfig_new_path(font_face ? font_face : ""); // REL 4

  geom.stride  = geom.width * 4;
  geom.size    = geom.stride * geom.height;
  app.wob_geom = &geom;

  int shmid = wob_shm_create();
  if (shmid < 0)
  {
    return EXIT_FAILURE;
  }
  app.shmid = shmid;

  wob_connect(&app);
  if (app.wl_shm == NULL || app.wl_compositor == NULL || app.wlr_layer_shell == NULL)
  {
    wob_log_error("Wayland compositor doesn't support all required protocols");
    return EXIT_FAILURE;
  }

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

  bool realshow = false;
  bool showup   = false;
  bool hidden   = true;
  for (;;)
  {
    unsigned long state                             = 0;
    char          input_buffer[INPUT_BUFFER_LENGTH] = {0};
    char*         fgets_rv;

    switch (poll(fds, 2, !showup ? -1 : timeout_msec))
    {
    case -1:
    {
      wob_log_info("POLL -1\n");
      wob_log_error("poll() failed: %s", strerror(errno));

      return EXIT_FAILURE;
    }
    case 0:
    {
      wob_log_info("POLL 0\n");

      // showing window on timeout
      if (hidden && showup)
      {
        wob_show(&app);
        hidden = false;
        wob_flush(&app);
      }

      break;
    }
    default:
    {
      wob_log_info("POLL DEFAULT\n");

      if (fds[0].revents)
      {
        if (!(fds[0].revents & POLLIN))
        {
          wob_log_error("WL_DISPLAY_FD unexpectedly closed, revents = %hd", fds[0].revents);
          return EXIT_FAILURE;
        }

        if (wl_display_dispatch(app.wl_display) == -1)
        {
          return EXIT_FAILURE;
        }
      }

      if (fds[1].revents)
      {
        if (!(fds[1].revents & POLLIN))
        {
          wob_log_error("STDIN unexpectedly closed, revents = %hd", fds[1].revents);
          if (!hidden) wob_hide(&app);
          wob_destroy(&app);

          return EXIT_FAILURE;
        }

        fgets_rv = fgets(input_buffer, INPUT_BUFFER_LENGTH, stdin);

        if (feof(stdin))
        {
          wob_log_info("Received EOF");
          if (!hidden) wob_hide(&app);
          wob_destroy(&app);

          return EXIT_SUCCESS;
        }

        if (fgets_rv == NULL)
        {
          wob_log_error("fgets() failed: %s", strerror(errno));
          if (!hidden) wob_hide(&app);
          wob_destroy(&app);

          return EXIT_FAILURE;
        }

        if (!wob_parse_input(input_buffer, &state))
        {
          wob_log_error("Received invalid input");

          if (!hidden) wob_hide(&app);

          wob_destroy(&app);

          return EXIT_FAILURE;
        }

        if (state == 1 && showup == 1)
        {
          hidden = false;
        }

        if (state == 1 && showup == 0)
        {
          showup = 1;
        }

        if (state == 0)
        {

          if (!hidden)
          {
            hidden = true;
            wob_hide(&app);
          }
          showup = 0;
        }
      }
    }
    }
  }
}
