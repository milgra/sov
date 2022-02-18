#define _POSIX_C_SOURCE 200112L
#include "xdg-shell-client-protocol.h"
#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <limits.h>
#include <stdbool.h>
#include <string.h>
#include <sys/mman.h>
#include <time.h>
#include <unistd.h>
#include <wayland-client.h>

#include "config.c"
#include "fontconfig.c"
#include "kvlines.c"
#include "text.c"
#include "tree_drawer.c"
#include "tree_reader.c"

#include "zc_bitmap.c"
#include "zc_cstring.c"
#include "zc_cstrpath.c"
#include "zc_vector.c"

#define CFG_PATH_LOC "~/.config/i3-overview/config"
#define CFG_PATH_GLO "/usr/share/i3-overview/config"
#define WIN_APPID "sway-overview"
#define WIN_TITLE "sway-overview"
#define GET_WORKSPACES_CMD "swaymsg -t get_workspaces"
#define GET_TREE_CMD "swaymsg -t get_tree"

int alive = 1;

bm_t* bitmap = NULL; // REL 5

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

/* Shared memory support code */
static void
randname(char* buf)
{
  struct timespec ts;
  clock_gettime(CLOCK_REALTIME, &ts);
  long r = ts.tv_nsec;
  for (int i = 0; i < 6; ++i)
  {
    buf[i] = 'A' + (r & 15) + (r & 16) * 2;
    r >>= 5;
  }
}

static int
create_shm_file(void)
{
  int retries = 100;
  do {
    char name[] = "/wl_shm-XXXXXX";
    randname(name + sizeof(name) - 7);
    --retries;
    int fd = shm_open(name, O_RDWR | O_CREAT | O_EXCL, 0600);
    if (fd >= 0)
    {
      shm_unlink(name);
      return fd;
    }
  } while (retries > 0 && errno == EEXIST);
  return -1;
}

static int
allocate_shm_file(size_t size)
{
  int fd = create_shm_file();
  if (fd < 0)
    return -1;
  int ret;
  do {
    ret = ftruncate(fd, size);
  } while (ret < 0 && errno == EINTR);
  if (ret < 0)
  {
    close(fd);
    return -1;
  }
  return fd;
}

/* Wayland code */
struct client_state
{
  /* Globals */
  struct wl_display*    wl_display;
  struct wl_registry*   wl_registry;
  struct wl_shm*        wl_shm;
  struct wl_compositor* wl_compositor;
  struct xdg_wm_base*   xdg_wm_base;
  /* Objects */
  struct wl_surface*   wl_surface;
  struct xdg_surface*  xdg_surface;
  struct xdg_toplevel* xdg_toplevel;
};

static void
wl_buffer_release(void* data, struct wl_buffer* wl_buffer)
{
  /* Sent by the compositor when it's no longer using this buffer */
  wl_buffer_destroy(wl_buffer);
}

static const struct wl_buffer_listener wl_buffer_listener = {
    .release = wl_buffer_release,
};

static struct wl_buffer* draw_frame(struct client_state* state)
{
  const int width = bitmap->w, height = bitmap->h;
  int       stride = width * 4;
  int       size   = stride * height;

  int fd = allocate_shm_file(size);
  if (fd == -1)
  {
    return NULL;
  }

  uint32_t* data = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
  if (data == MAP_FAILED)
  {
    close(fd);
    return NULL;
  }

  struct wl_shm_pool* pool   = wl_shm_create_pool(state->wl_shm, fd, size);
  struct wl_buffer*   buffer = wl_shm_pool_create_buffer(pool, 0, width, height, stride, WL_SHM_FORMAT_XRGB8888);
  wl_shm_pool_destroy(pool);
  close(fd);

  /* Draw checkerboxed background */
  /*  for (int y = 0; y < height; ++y) */
  /*   { */
  /*     for (int x = 0; x < width; ++x) */
  /*     { */
  /*       if ((x + y / 8 * 8) % 16 < 8) */
  /*         data[y * width + x] = 0xFF666666; */
  /*       else */
  /*         data[y * width + x] = 0xFFEEEEEE; */
  /*     } */
  /*   } */

  memcpy(data, (uint8_t*)bitmap->data, bitmap->size);

  munmap(data, size);
  wl_buffer_add_listener(buffer, &wl_buffer_listener, NULL);
  return buffer;
}

static void
xdg_surface_configure(void*               data,
                      struct xdg_surface* xdg_surface,
                      uint32_t            serial)
{
  struct client_state* state = data;
  xdg_surface_ack_configure(xdg_surface, serial);

  struct wl_buffer* buffer = draw_frame(state);
  wl_surface_attach(state->wl_surface, buffer, 0, 0);
  wl_surface_commit(state->wl_surface);
}

static const struct xdg_surface_listener xdg_surface_listener = {
    .configure = xdg_surface_configure,
};

static void
xdg_wm_base_ping(void* data, struct xdg_wm_base* xdg_wm_base, uint32_t serial)
{
  xdg_wm_base_pong(xdg_wm_base, serial);
}

static const struct xdg_wm_base_listener xdg_wm_base_listener = {
    .ping = xdg_wm_base_ping,
};

static void
registry_global(void* data, struct wl_registry* wl_registry, uint32_t name, const char* interface, uint32_t version)
{
  struct client_state* state = data;
  if (strcmp(interface, wl_shm_interface.name) == 0)
  {
    state->wl_shm = wl_registry_bind(
        wl_registry, name, &wl_shm_interface, 1);
  }
  else if (strcmp(interface, wl_compositor_interface.name) == 0)
  {
    state->wl_compositor = wl_registry_bind(
        wl_registry, name, &wl_compositor_interface, 4);
  }
  else if (strcmp(interface, xdg_wm_base_interface.name) == 0)
  {
    state->xdg_wm_base = wl_registry_bind(
        wl_registry, name, &xdg_wm_base_interface, 1);
    xdg_wm_base_add_listener(state->xdg_wm_base,
                             &xdg_wm_base_listener, state);
  }
}

static void
registry_global_remove(void*               data,
                       struct wl_registry* wl_registry,
                       uint32_t            name)
{
  /* This space deliberately left blank */
}

static const struct wl_registry_listener wl_registry_listener = {
    .global        = registry_global,
    .global_remove = registry_global_remove,
};

int main(int argc, char* argv[])
{
  printf("sway-overview v%i.%i by Milan Toth\n", VERSION, BUILD);

  /* parse parameters */

  char* cfg_path = NULL;
  char  verbose  = 0;

  const struct option long_options[] = {
      {"config", required_argument, 0, 'c'},
      {"verbose", no_argument, 0, 'v'},
      {0, 0, 0, 0},
  };

  int option       = 0;
  int option_index = 0;

  while ((option = getopt_long(argc, argv, "c:v", long_options, &option_index)) != -1)
  {
    if (option == 'c') cfg_path = cstr_new_cstring(optarg); // REL 0
    if (option == 'v') verbose = 1;
    if (option == '?') printf("-c --config= [path] \t use config file for session\n-v --verbose \t\t verbose standard output\n");
  }

  /* init config */

  config_init(); // DESTROY 0

  char cwd[PATH_MAX];
  if (getcwd(cwd, sizeof(cwd)) == NULL) printf("Cannot get working directory\n");

  char* cfg_path_loc = cfg_path ? cstr_new_path_normalize(cfg_path, cwd) : cstr_new_path_normalize(CFG_PATH_LOC, getenv("HOME")); // REL 2
  char* cfg_path_glo = cstr_new_cstring(CFG_PATH_GLO);                                                                            // REL 3

  if (config_read(cfg_path_loc) < 0)
  {
    if (config_read(cfg_path_glo) < 0)
      printf("No local or global config file found\n");
    else if (verbose)
      printf("Using config file : %s\n", cfg_path_glo);
  }
  else if (verbose)
    printf("Using config file : %s\n", cfg_path_loc);

  REL(cfg_path_glo); // REL 3
  REL(cfg_path_loc); // REL 2

  if (verbose) config_describe();

  /* init text rendeing */

  text_init(); // DESTROY 1

  char* font_face = config_get("font_face");
  char* font_path = fontconfig_new_path(font_face ? font_face : ""); // REL 4

  /* DRAWING */

  vec_t* workspaces = VNEW(); // REL 6

  read_tree(workspaces);

  i3_workspace_t* ws  = workspaces->data[0];
  i3_workspace_t* wsl = workspaces->data[workspaces->length - 1];

  mem_describe(ws, 0);

  if (ws->width > 0 && ws->height > 0)
  {
    int gap   = config_get_int("gap");
    int cols  = config_get_int("columns");
    int rows  = (int)ceilf((float)wsl->number / cols);
    int ratio = config_get_int("ratio");

    int lay_wth = cols * (ws->width / ratio) + (cols + 1) * gap;
    int lay_hth = rows * (ws->height / ratio) + (rows + 1) * gap;

    /* resize if needed */

    /* map if necessary */

    /* flush move, resize and map commands */

    /* create overlay bitmap */

    bitmap = bm_new(lay_wth, lay_hth); // REL 5

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
  }

  struct client_state state = {0};
  state.wl_display          = wl_display_connect(NULL);
  state.wl_registry         = wl_display_get_registry(state.wl_display);
  wl_registry_add_listener(state.wl_registry, &wl_registry_listener, &state);
  wl_display_roundtrip(state.wl_display);

  state.wl_surface  = wl_compositor_create_surface(state.wl_compositor);
  state.xdg_surface = xdg_wm_base_get_xdg_surface(state.xdg_wm_base, state.wl_surface);
  xdg_surface_add_listener(state.xdg_surface, &xdg_surface_listener, &state);
  state.xdg_toplevel = xdg_surface_get_toplevel(state.xdg_surface);
  xdg_toplevel_set_title(state.xdg_toplevel, WIN_TITLE);
  xdg_toplevel_set_app_id(state.xdg_toplevel, WIN_APPID);
  wl_surface_commit(state.wl_surface);

  while (wl_display_dispatch(state.wl_display))
  {
    /* This space deliberately left blank */
  }

  // cleanup

  REL(workspaces);
  REL(bitmap);

  /* cleanup */

  config_destroy(); // DESTROY 0
  text_destroy();   // DESTROY 1

  REL(font_path); // REL 4
  REL(bitmap);    // REL 5

  if (cfg_path) REL(cfg_path); // REL 0

#ifdef DEBUG
  mem_stats();
#endif

  return 0;
}
