#include "config.c"
#include "fontconfig.c"
#include "kvlines.c"
#include "text.c"
#include "tree_drawer.c"
#include "tree_reader.c"
#include "zc_bitmap.c"
#include "zc_cstring.c"
#include "zc_cstrpath.c"
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/extensions/XInput.h>
#include <X11/extensions/XInput2.h>
#include <getopt.h>
#include <limits.h>
#include <math.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

#define CFG_PATH_LOC "~/.config/i3-overview/config"
#define CFG_PATH_GLO "/usr/share/i3-overview/config"
#define WIN_CLASS "i3-overview"
#define WIN_TITLE "i3-overview"
#define GET_WORKSPACES_CMD "sway-msg -t get_workspaces"
#define GET_TREE_CMD "sway-msg -t get_tree"

int alive = 1;

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

void sighandler(int signal)
{
  alive = 0;
}

int errorhandler(Display* display, XErrorEvent* event)
{
  return 0;
}

int main(int argc, char* argv[])
{
  printf("i3-overview v%i.%i by Milan Toth\n", VERSION, BUILD);

  /* close gracefully for SIGINT */

  if (signal(SIGINT, &sighandler) == SIG_ERR) printf("Could not set signal handler\n");

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

  /* init X11 */

  XSetErrorHandler(errorhandler);

  Display* display = XOpenDisplay(NULL);

  int opcode;
  int event;
  int error;

  if (!XQueryExtension(display, "XInputExtension", &opcode, &event, &error)) printf("X Input extension not available.\n");

  /* create overlay window */

  Window view_win = XCreateSimpleWindow(display, DefaultRootWindow(display), 0, 0, 200, 100, 0, 0xFFFFFF, 0); // DESTROY 2

  XClassHint hint;

  hint.res_name  = WIN_CLASS;
  hint.res_class = WIN_CLASS;

  XSetClassHint(display, view_win, &hint);
  XStoreName(display, view_win, WIN_TITLE);

  XSelectInput(display, view_win, StructureNotifyMask);

  /* start listening for global key events */

  Window root_win = DefaultRootWindow(display);

  XIEventMask mask;
  mask.deviceid = XIAllDevices;
  mask.mask_len = XIMaskLen(XI_LASTEVENT);
  mask.mask     = calloc(mask.mask_len, sizeof(char)); // FREE 0

  XISetMask(mask.mask, XI_KeyPress);
  XISetMask(mask.mask, XI_KeyRelease);

  XISelectEvents(display, root_win, &mask, 1);
  XSync(display, False);

  free(mask.mask); // FREE 0

  int last_code     = 0;
  int meta_code     = config_get_int("meta_code");
  int sec_code      = config_get_int("secondary_code");
  int meta_pressed  = 0;
  int sec_pressed   = 0;
  int window_mapped = 0;

  if (sec_code == INT_MAX) sec_pressed = 1;

  bm_t* bitmap = bm_new(1, 1); // REL 5

  while (alive)
  {
    XEvent               ev;
    XGenericEventCookie* cookie = (XGenericEventCookie*)&ev.xcookie;

    XNextEvent(display, (XEvent*)&ev); // FREE 1

    if (XGetEventData(display, cookie) && cookie->type == GenericEvent && cookie->extension == opcode)
    {
      XIDeviceEvent* event = cookie->data;

      if (event->evtype == XI_KeyPress)
      {
        if (event->detail != last_code) // avoid key repeat
        {
          last_code = event->detail;

          if (event->detail == meta_code) meta_pressed = 1;
          if (event->detail == sec_code) sec_pressed = 1;

          if (meta_pressed && sec_pressed)
          {
            vec_t* workspaces = VNEW(); // REL 6

            read_tree(workspaces);

            i3_workspace_t* ws  = workspaces->data[0];
            i3_workspace_t* wsl = workspaces->data[workspaces->length - 1];

            if (ws->width > 0 && ws->height > 0)
            {
              int gap   = config_get_int("gap");
              int cols  = config_get_int("columns");
              int rows  = (int)ceilf((float)wsl->number / cols);
              int ratio = config_get_int("ratio");

              int lay_wth = cols * (ws->width / ratio) + (cols + 1) * gap;
              int lay_hth = rows * (ws->height / ratio) + (rows + 1) * gap;

              XWindowAttributes win_attr;
              XGetWindowAttributes(display, view_win, &win_attr);

              int win_wth = win_attr.width;
              int win_hth = win_attr.height;

              /* resize if needed */

              if (win_wth != lay_wth || win_hth != lay_hth)
              {
                XResizeWindow(display, view_win, lay_wth, lay_hth);
                XMoveWindow(display, view_win, ws->width / 2 - lay_wth / 2, ws->height / 2 - lay_hth / 2);
              }

              /* map if necessary */

              if (!window_mapped)
              {
                window_mapped = 1;
                XMapWindow(display, view_win);
                XMoveWindow(display, view_win, ws->width / 2 - lay_wth / 2, ws->height / 2 - lay_hth / 2);
              }

              /* flush move, resize and map commands */

              XSync(display, False);

              /* create overlay bitmap */

              if (bitmap->w != lay_wth || bitmap->h != lay_hth)
              {
                REL(bitmap);
                bitmap = bm_new(lay_wth, lay_hth); // REL 5
              }

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

              XImage* image = XGetImage(display, view_win, 0, 0, lay_wth, lay_hth, AllPlanes, ZPixmap); // DESTROY 3

              if (image)
              {

                uint8_t* data = bitmap->data;

                for (int y = 0; y < lay_hth; y++)
                {
                  for (int x = 0; x < lay_wth; x++)
                  {
                    uint8_t  r     = data[0];
                    uint8_t  g     = data[1];
                    uint8_t  b     = data[2];
                    uint32_t pixel = (r << 16) | (g << 8) | b;

                    XPutPixel(image, x, y, pixel);

                    data += 4;
                  }
                }

                GC gc = XCreateGC(display, view_win, 0, NULL); // FREE 0
                XPutImage(display, view_win, gc, image, 0, 0, 0, 0, lay_wth, lay_hth);

                /* cleanup */

                XFreeGC(display, gc); // FREE 0
                XDestroyImage(image); // DESTROY 3
              }
            }

            REL(workspaces); // REL 6
          }
        }
      }
      else if (event->evtype == XI_KeyRelease)
      {
        if (event->detail == meta_code)
        {
          meta_pressed = 0;
          last_code    = 0;
        }
        if (event->detail == sec_code)
        {
          sec_pressed = 0;
          last_code   = 0;
        }

        if (!(meta_pressed && sec_pressed))
        {
          if (window_mapped)
          {
            XUnmapWindow(display, view_win);
            XSync(display, False);

            window_mapped = 0;
          }
        }
      }
    }

    XFreeEventData(display, cookie); // FREE 1
  }

  XDestroyWindow(display, view_win); // DESTROY 2
  XSync(display, False);
  XCloseDisplay(display);

  /* cleanup */

  config_destroy(); // DESTROY 0
  text_destroy();   // DESTROY 1

  REL(font_path); // REL 4
  REL(bitmap);    // REL 5

  if (cfg_path) REL(cfg_path); // REL 0

#ifdef DEBUG
  mem_stats();
#endif
}
