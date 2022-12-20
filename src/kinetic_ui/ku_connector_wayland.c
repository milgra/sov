#ifndef ku_wayland_h
#define ku_wayland_h

#include "ku_bitmap.c"
#include "ku_draw.c"
#include "ku_event.c"
#include <EGL/egl.h>
#include <GLES2/gl2.h>
#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <limits.h>
#include <poll.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/socket.h>
#include <sys/timerfd.h>
#include <sys/types.h>
#include <sys/un.h>
#include <unistd.h>
#include <wayland-cursor.h>
#include <wayland-egl.h>
#include <wayland-server.h>
#include <xkbcommon/xkbcommon.h>

#include "mt_log.c"
#include "mt_memory.c"
#include "mt_time.c"
#include "pointer-gestures-unstable-v1-client-protocol.h"
#include "wlr-layer-shell-unstable-v1-client-protocol.h"
#include "xdg-output-unstable-v1-client-protocol.h"
#include "xdg-shell-client-protocol.h"

#define MAX_MONITOR_NAME_LEN 255

struct monitor_info
{
    int32_t physical_width;
    int32_t physical_height;
    int32_t logical_width;
    int32_t logical_height;
    int     scale;
    double  ratio;
    int     index;
    char    name[MAX_MONITOR_NAME_LEN + 1];

    enum wl_output_subpixel subpixel;
    struct zxdg_output_v1*  xdg_output;
    struct wl_output*       wl_output;
};

struct keyboard_info
{
    struct wl_keyboard* kbd;
    struct xkb_context* xkb_context;
    struct xkb_keymap*  xkb_keymap;
    struct xkb_state*   xkb_state;
    bool                control;
    bool                shift;

    /* key repeater config */

    int        rep_period;
    int        rep_delay;
    int        rep_timer_fd;
    ku_event_t rep_event;
};

struct pointer_info
{
    int      px;
    int      py;
    int      drag;
    int      down;
    float    scale;
    uint32_t lastdown;
};

/* TODO use added, removed and changed events */

enum wl_event_id
{
    WL_EVENT_OUTPUT_ADDED,
    WL_EVENT_OUTPUT_REMOVED,
    WL_EVENT_OUTPUT_CHANGED
};

typedef struct _wl_event_t wl_event_t;
struct _wl_event_t
{
    enum wl_event_id      id;
    struct monitor_info** monitors;
    int                   monitor_count;
};

enum wl_window_type
{
    WL_WINDOW_NATIVE,
    WL_WINDOW_EGL,
    WL_WINDOW_LAYER,
};

typedef struct _wl_window_t wl_window_t;
struct _wl_window_t
{
    int  scale;
    int  width;
    int  height;
    int  buffer_width;
    int  buffer_height;
    int  fullscreen;
    int  hidden;
    int  margin;
    char anchor[4];

    int shown;  /* surface_enter is called, scaling is set */
    int inited; /* egl is swapped or buffer is attached for the first time to trigger surface_enter event */

    enum wl_window_type type;

    struct monitor_info* monitor;      /* related monitor */
    struct wl_surface*   surface;      /* wl surface for window */
    struct wl_callback*  frame_cb;     /* frame done callback */
    struct xdg_surface*  xdg_surface;  /* window surface */
    struct xdg_toplevel* xdg_toplevel; /* window info */

    struct pointer_info                  pointer; /* pointer state */
    struct zwp_pointer_gesture_pinch_v1* pinch;
    struct wl_pointer*                   wl_pointer;

    /* backing buffer for native window */

    struct wl_buffer* buffer; /* wl buffer for surface */

    void* shm_data; /* active bufferdata */
    int   shm_size; /* active bufferdata size */

    /* backing bitmap for native window, also holds size data for egl drawing */
    /* TODO don't use this bitmap for egl drawing */

    ku_bitmap_t bitmap;

    /* needed for resize events */

    int new_scale;
    int new_width;
    int new_height;

    /* egl window related */

    struct wl_egl_window* eglwindow;
    struct wl_region*     region;

    EGLDisplay egldisplay;
    EGLSurface eglsurface;
    EGLContext eglcontext;

    struct zwlr_layer_shell_v1*   layer_shell;   /* active layer shell */
    struct zwlr_layer_surface_v1* layer_surface; /* active layer surface */
};

void ku_wayland_init(void (*init)(wl_event_t event), void (*update)(ku_event_t), void (*destroy)(), int time_event_delay);

wl_window_t* ku_wayland_create_window(char* title, int width, int height);
wl_window_t* ku_wayland_create_eglwindow(char* title, int width, int height);
wl_window_t* ku_wayland_create_generic_layer(struct monitor_info* monitor, int width, int height, int margin, char* anchor);
void         ku_wayland_show_window(wl_window_t* info);

void ku_wayland_draw_window(wl_window_t* info, int x, int y, int w, int h);
void ku_wayland_delete_window(wl_window_t* info);
void ku_wayland_request_frame(wl_window_t* info);

void ku_wayland_exit();
void ku_wayland_toggle_fullscreen(wl_window_t* window);
void ku_wayland_set_time_event_delay(int ms);

#endif

#if __INCLUDE_LEVEL__ == 0

void ku_wayland_resize_window_buffer(wl_window_t* info);

struct wlc_t
{
    void (*init)(wl_event_t event);
    void (*update)(ku_event_t);
    void (*destroy)();

    struct wl_display* display; /* global display object */

    struct wl_compositor* compositor; /* active compositor */
    struct wl_pointer*    ipointer;   /* active pointer interface */
    struct wl_seat*       seat;       /* active seat interface */
    struct wl_shm*        shm;        /* active shared memory interface */

    struct zxdg_output_manager_v1* xdg_output_manager; /* active output manager */
    struct xdg_wm_base*            xdg_wm_base;        /* active wm base interface */

    struct zwp_pointer_gestures_v1*      pointer_manager;
    struct zwp_pointer_gesture_pinch_v1* pinch_gesture;
    struct zwp_pointer_gesture_hold_v1*  hold_gesture;

    int pointer_manager_version;

    /* TODO move this into layer info objects */

    struct zwlr_layer_shell_v1*   layer_shell;   /* active layer shell */
    struct zwlr_layer_surface_v1* layer_surface; /* active layer surface */

    /* TODO store these in mt_vector_t */

    int                   monitor_count;
    int                   window_count;
    struct monitor_info** monitors;
    wl_window_t**         windows;

    struct keyboard_info keyboard; /* keyboard state */

    /* time event control */

    int             time_event_interval;
    int             time_event_timer_fd;
    struct timespec time_start;
    uint32_t        frame_index;

    /* framerate related */

    uint32_t last_frame;
    uint32_t frame_count;
    uint32_t frame_rate;

    /* last frame event to calculate delay */

    ku_event_t frame_event;

    int exit_flag;

    /* cursor */

    struct wl_cursor_theme* cursor_theme;
    struct wl_cursor*       default_cursor;
    struct wl_surface*      cursor_surface;

} wlc = {0};

/* init event with time data */

ku_event_t init_event()
{
    struct timespec stop;
    struct timespec start = wlc.time_start;
    struct timespec result;

    clock_gettime(CLOCK_REALTIME, &stop);

    if ((stop.tv_nsec - start.tv_nsec) < 0)
    {
	result.tv_sec  = stop.tv_sec - start.tv_sec - 1;
	result.tv_nsec = stop.tv_nsec - start.tv_nsec + 1000000000;
    }
    else
    {
	result.tv_sec  = stop.tv_sec - start.tv_sec;
	result.tv_nsec = stop.tv_nsec - start.tv_nsec;
    }

    ku_event_t event = {
	.time_unix = stop,
	.time      = result.tv_sec * 1000 + result.tv_nsec / 1000000};

    return event;
}

int32_t round_to_int(double val)
{
    return (int32_t) (val + 0.5);
}

/* surface buffer related */

/* open posix shared memory object */

int ku_wayland_shm_create()
{
    int  shmid = -1;
    char shm_name[NAME_MAX];
    for (int i = 0; i < UCHAR_MAX; ++i)
    {
	if (snprintf(shm_name, NAME_MAX, "/wcp-%d", i) >= NAME_MAX)
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
	mt_log_error("shm_open() failed: %s", strerror(errno));
	return -1;
    }

    if (shm_unlink(shm_name) != 0)
    {
	mt_log_error("shm_unlink() failed: %s", strerror(errno));
	return -1;
    }

    return shmid;
}

/* allocate shared memory at previously created object */

void* ku_wayland_shm_alloc(const int shmid, const size_t size)
{
    if (ftruncate(shmid, size) != 0)
    {
	mt_log_debug("ftruncate() failed: %s", strerror(errno));
	return NULL;
    }

    void* buffer = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, shmid, 0);
    if (buffer == MAP_FAILED)
    {
	mt_log_debug("mmap() failed: %s", strerror(errno));
	return NULL;
    }

    return buffer;
}

static void ku_wayland_buffer_release(void* data, struct wl_buffer* wl_buffer)
{
    /* mt_log_debug("buffer release"); */
}

static const struct wl_buffer_listener buffer_listener = {
    .release = ku_wayland_buffer_release};

void ku_wayland_create_buffer(wl_window_t* info, int width, int height)
{
    if (info->buffer)
    {
	/* delete old buffer and bitmap */
	wl_buffer_destroy(info->buffer);
	munmap(info->shm_data, info->shm_size);
	info->bitmap.data = NULL;
    }

    int stride = width * 4;
    int size   = stride * height;

    int fd = ku_wayland_shm_create();
    if (fd < 0)
    {
	mt_log_error("Shm create failed");
	return;
    }

    info->shm_data = ku_wayland_shm_alloc(fd, size);

    if (info->shm_data == MAP_FAILED)
    {
	mt_log_error("mmap failed: %m");
	close(fd);
	return;
    }

    /* TODO don't use bitmap, use void* for data */

    info->bitmap.w    = width;
    info->bitmap.h    = height;
    info->bitmap.size = size;
    info->bitmap.data = info->shm_data;

    struct wl_shm_pool* pool   = wl_shm_create_pool(wlc.shm, fd, size);
    struct wl_buffer*   buffer = wl_shm_pool_create_buffer(pool, 0, width, height, stride, WL_SHM_FORMAT_ARGB8888);

    wl_shm_pool_destroy(pool);

    info->buffer   = buffer;
    info->shm_size = size;

    wl_buffer_add_listener(buffer, &buffer_listener, NULL);
}

/* frame listener */

static const struct wl_callback_listener wl_surface_frame_listener;

static void wl_surface_frame_done(void* data, struct wl_callback* cb, uint32_t time)
{
    wl_window_t* info = data;

    /* Count framerate */
    if (time - wlc.last_frame > 1000)
    {
	wlc.frame_rate  = wlc.frame_count;
	wlc.frame_count = 1;
	wlc.last_frame  = time;
    }
    else
	wlc.frame_count++;

    /* Destroy this callback */
    wl_callback_destroy(cb);
    info->frame_cb = NULL;

    /* Dispatch frame event to trigger rendering */
    ku_event_t event = init_event();

    event.type       = KU_EVENT_FRAME;
    event.time_frame = (float) (event.time - wlc.frame_event.time) / 1000.0;
    event.frame      = wlc.frame_index++;
    event.window     = (void*) info;

    /* Store event to be able to calculate frame delta */
    wlc.frame_event = event;

    /* TODO refresh rate can differ so animations should be time based */
    (*wlc.update)(event);
}

static const struct wl_callback_listener wl_surface_frame_listener = {
    .done = wl_surface_frame_done,
};

/* layer surface listener */

static void ku_wayland_layer_surface_configure(void* data, struct zwlr_layer_surface_v1* surface, uint32_t serial, uint32_t width, uint32_t height)
{
    mt_log_debug("layer surface configure serial %u width %i height %i", serial, width, height);

    zwlr_layer_surface_v1_ack_configure(surface, serial);

    wl_window_t* info = data;

    if (info->inited == 0)
    {
	info->inited = 1;

	ku_wayland_create_buffer(info, info->buffer_width, info->buffer_height);

	ku_event_t event = init_event();
	event.type       = KU_EVENT_WINDOW_SHOWN;
	event.w          = info->buffer_width;
	event.h          = info->buffer_height;
	event.window     = (void*) info;

	(*wlc.update)(event);
    }
    else
    {
	if (info->width != width || info->height != height)
	{
	    info->width         = width;
	    info->height        = height;
	    info->buffer_width  = info->width * info->scale;
	    info->buffer_height = info->height * info->scale;

	    ku_wayland_create_buffer(info, info->buffer_width, info->buffer_height);

	    ku_event_t event = init_event();
	    event.type       = KU_EVENT_RESIZE;
	    event.w          = info->buffer_width;
	    event.h          = info->buffer_height;
	    event.window     = (void*) info;

	    (*wlc.update)(event);
	}
    }
}

static void ku_wayland_layer_surface_closed(void* _data, struct zwlr_layer_surface_v1* surface)
{
    mt_log_debug("layer surface configure");
}

struct zwlr_layer_surface_v1_listener layer_surface_listener = {
    .configure = ku_wayland_layer_surface_configure,
    .closed    = ku_wayland_layer_surface_closed,
};

/* xdg toplevel events */

void xdg_toplevel_configure(void* data, struct xdg_toplevel* xdg_toplevel, int32_t width, int32_t height, struct wl_array* states)
{
    mt_log_debug("xdg toplevel configure w %i h %i", width, height);

    wl_window_t* info = data;

    if (width > 0 && height > 0)
    {
	info->new_width  = width;
	info->new_height = height;
    }
}

void xdg_toplevel_close(void* data, struct xdg_toplevel* xdg_toplevel)
{
    /* mt_log_debug("xdg toplevel close"); */
    wlc.exit_flag = 1;
}

void xdg_toplevel_configure_bounds(void* data, struct xdg_toplevel* xdg_toplevel, int32_t width, int32_t height)
{
    mt_log_debug("xdg toplevel configure bounds w %i h %i", width, height);
}

void xdg_toplevel_wm_capabilities(void* data, struct xdg_toplevel* xdg_toplevel, struct wl_array* capabilities)
{
    /* mt_log_debug("xdg toplevel wm capabilities"); */
}

static const struct xdg_toplevel_listener xdg_toplevel_listener = {
    .configure        = xdg_toplevel_configure,
    .close            = xdg_toplevel_close,
    .configure_bounds = xdg_toplevel_configure_bounds,
    /* .wm_capabilities  = xdg_toplevel_wm_capabilities */
};

/* xdg surface events */

static void xdg_surface_configure(void* data, struct xdg_surface* xdg_surface, uint32_t serial)
{
    xdg_surface_ack_configure(xdg_surface, serial);

    wl_window_t* info = data;

    mt_log_debug("xdg surface configure %i %i %i", info->type, info->width, info->height);

    if (info->inited == 0)
    {
	info->inited = 1;

	if (info->type == WL_WINDOW_EGL) eglSwapBuffers(wlc.windows[0]->egldisplay, wlc.windows[0]->eglsurface);
	if (info->type == WL_WINDOW_NATIVE)
	{
	    ku_wayland_create_buffer(info, info->width, info->height);

	    wl_surface_attach(info->surface, info->buffer, 0, 0);
	    wl_surface_damage(info->surface, 0, 0, info->width, info->height);
	    wl_surface_commit(info->surface);
	}
    }
    else if (info->width != info->new_width || info->height != info->new_height)
    {
	info->width         = info->new_width;
	info->height        = info->new_height;
	info->buffer_width  = info->width * info->scale;
	info->buffer_height = info->height * info->scale;

	if (info->type == WL_WINDOW_EGL)
	{
	    info->bitmap.w = info->buffer_width;
	    info->bitmap.h = info->buffer_height;
	    wl_egl_window_resize(info->eglwindow, info->buffer_width, info->buffer_height, 0, 0);
	}
	if (info->type == WL_WINDOW_NATIVE) ku_wayland_create_buffer(info, info->buffer_width, info->buffer_height);

	/* we shouldn't send events before surface enter ( and ku_event_window_shown ) event */
	if (info->shown)
	{
	    ku_event_t event = init_event();
	    event.type       = KU_EVENT_RESIZE;
	    event.w          = info->buffer_width;
	    event.h          = info->buffer_height;
	    event.window     = (void*) info;

	    (*wlc.update)(event);
	}
    }
}

static const struct xdg_surface_listener xdg_surface_listener = {
    .configure = xdg_surface_configure,
};

/* wl surface events */

static void wl_surface_enter(void* userData, struct wl_surface* surface, struct wl_output* output)
{
    mt_log_debug("wl surface enter");

    wl_window_t* info = userData;

    info->shown = 1;

    for (int index = 0; index < wlc.monitor_count; index++)
    {
	struct monitor_info* monitor = wlc.monitors[index];

	if (monitor->wl_output == output)
	{
	    mt_log_debug("output name %s monitor scale %i info scale %i", monitor->name, monitor->scale, info->scale);

	    info->monitor       = monitor;
	    info->scale         = monitor->scale;
	    info->buffer_width  = info->width * info->scale;
	    info->buffer_height = info->height * info->scale;

	    wl_surface_set_buffer_scale(info->surface, info->scale);

	    if (info->type == WL_WINDOW_EGL)
	    {
		wl_egl_window_resize(info->eglwindow, info->buffer_width, info->buffer_height, 0, 0);
		info->bitmap.w = info->buffer_width;
		info->bitmap.h = info->buffer_height;
	    }
	    if (info->type == WL_WINDOW_NATIVE && info->buffer_width != info->bitmap.w) ku_wayland_create_buffer(info, info->buffer_width, info->buffer_height);

	    ku_event_t event = init_event();
	    event.type       = KU_EVENT_WINDOW_SHOWN;
	    event.w          = info->buffer_width;
	    event.h          = info->buffer_height;
	    event.window     = (void*) info;

	    (*wlc.update)(event);
	}
    }
}

static void wl_surface_leave(void* userData, struct wl_surface* surface, struct wl_output* output)
{
    mt_log_debug("wl surface leave");
}

static const struct wl_surface_listener wl_surface_listener = {
    .enter = wl_surface_enter,
    .leave = wl_surface_leave,
};

wl_window_t* ku_wayland_create_generic_window(char* title, int width, int height, int type)
{
    wl_window_t* info = CAL(sizeof(wl_window_t), NULL, NULL);

    info->type   = type;
    info->scale  = 1;
    info->width  = width;
    info->height = height;
    info->hidden = 1;

    info->surface      = wl_compositor_create_surface(wlc.compositor);
    info->xdg_surface  = xdg_wm_base_get_xdg_surface(wlc.xdg_wm_base, info->surface);
    info->xdg_toplevel = xdg_surface_get_toplevel(info->xdg_surface);

    wl_surface_add_listener(info->surface, &wl_surface_listener, info);
    xdg_surface_add_listener(info->xdg_surface, &xdg_surface_listener, info);
    xdg_toplevel_add_listener(info->xdg_toplevel, &xdg_toplevel_listener, info);

    xdg_toplevel_set_title(info->xdg_toplevel, title);
    xdg_toplevel_set_app_id(info->xdg_toplevel, title);

    return info;
}

/* creates xdg surface and toplevel */

wl_window_t* ku_wayland_create_window(char* title, int width, int height)
{
    wl_window_t* info = ku_wayland_create_generic_window(title, width, height, WL_WINDOW_NATIVE);

    wlc.windows[wlc.window_count] = info;
    wlc.window_count += 1;

    wl_surface_commit(info->surface);
    wl_display_roundtrip(wlc.display);

    return info;
}

/* creates egl window
   window won't be shown, you have to show it with ku_wayland_show_window
   when shown, a KU_EVENT_WINDOW_SHOWN will be dispatched

   after this function an xdg_surface_configure event will be called
*/

wl_window_t* ku_wayland_create_eglwindow(char* title, int width, int height)
{
    wl_window_t* info = ku_wayland_create_generic_window(title, width, height, WL_WINDOW_EGL);

    wlc.windows[wlc.window_count] = info;
    wlc.window_count += 1;

    /* set opaque region */

    info->region = wl_compositor_create_region(wlc.compositor);
    wl_region_add(info->region, 0, 0, width, height);
    wl_surface_set_opaque_region(info->surface, info->region);

    /* setup egl */
    /* TODO move this into ku_gl */

    struct wl_egl_window* egl_window = wl_egl_window_create(info->surface, width, height);

    info->eglwindow = egl_window;

    if (egl_window == EGL_NO_SURFACE) mt_log_error("Cannot create EGL surface");

    EGLint     numConfigs;
    EGLint     majorVersion;
    EGLint     minorVersion;
    EGLContext context;
    EGLSurface surface;
    EGLConfig  config;
    EGLint     fbAttribs[] = {
	    EGL_SURFACE_TYPE,
	    EGL_WINDOW_BIT,
	    EGL_RENDERABLE_TYPE,
	    EGL_OPENGL_ES2_BIT,
	    EGL_RED_SIZE,
	    8,
	    EGL_GREEN_SIZE,
	    8,
	    EGL_BLUE_SIZE,
	    8,
	    EGL_ALPHA_SIZE,
	    8,
	    EGL_NONE};
    EGLint contextAttribs[] = {
	EGL_CONTEXT_CLIENT_VERSION,
	2,
	EGL_NONE,
	EGL_NONE};

    EGLDisplay display = eglGetDisplay(wlc.display);
    if (display == EGL_NO_DISPLAY)
    {
	printf("No EGL Display...\n");
    }

    info->egldisplay = display;

    /* Initialize EGL */
    if (!eglInitialize(display, &majorVersion, &minorVersion))
    {
	printf("No Initialisation...\n");
    }

    /* Get configs */
    if ((eglGetConfigs(display, NULL, 0, &numConfigs) != EGL_TRUE) || (numConfigs == 0))
    {
	printf("No configuration...\n");
    }

    /* Choose config */
    if ((eglChooseConfig(display, fbAttribs, &config, 1, &numConfigs) != EGL_TRUE) || (numConfigs != 1))
    {
	printf("No configuration...\n");
    }

    /* EGLint sfAttribs[] = { */
    /* 	EGL_RENDER_BUFFER, */
    /* 	EGL_SINGLE_BUFFER, */
    /* 	EGL_NONE}; */

    /* Create a surface */
    surface = eglCreateWindowSurface(display, config, (EGLNativeWindowType) egl_window, NULL);
    if (surface == EGL_NO_SURFACE)
    {
	printf("No surface...\n");
    }

    /* eglSurfaceAttrib(display, surface, EGL_SWAP_BEHAVIOR, EGL_BUFFER_PRESERVED); */

    info->eglsurface = surface;

    /* Create a GL context */
    context = eglCreateContext(display, config, EGL_NO_CONTEXT, contextAttribs);
    if (context == EGL_NO_CONTEXT)
    {
	printf("No context...\n");
    }

    info->eglcontext = context;

    /* Make the context current */
    if (!eglMakeCurrent(display, surface, surface, context))
    {
	printf("Could not make the current window current !\n");
    }

    wl_surface_commit(info->surface);
    wl_display_roundtrip(wlc.display);

    return info;
}

/* deletes xdg surface and toplevel */

void ku_wayland_delete_window(wl_window_t* info)
{
    if (info->hidden == 0)
    {
	if (info->frame_cb)
	{
	    wl_callback_destroy(info->frame_cb);
	    info->frame_cb = NULL;
	}

	if (info->type == WL_WINDOW_NATIVE || info->type == WL_WINDOW_EGL)
	{
	    if (info->type == WL_WINDOW_NATIVE)
	    {
	    }
	    else if (info->type == WL_WINDOW_EGL)
	    {
		eglMakeCurrent(info->egldisplay, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);

		eglDestroyContext(info->egldisplay, info->eglcontext);
		eglDestroySurface(info->egldisplay, info->eglsurface);

		wl_region_destroy(info->region);
		wl_egl_window_destroy(info->eglwindow);

		/* TODO ask wayland devs how to free up egl display properly because now it leaks */
		eglTerminate(info->egldisplay);
		eglReleaseThread();
	    }

	    xdg_surface_destroy(info->xdg_surface);
	    xdg_toplevel_destroy(info->xdg_toplevel);
	}

	if (info->type == WL_WINDOW_LAYER)
	{
	    zwlr_layer_surface_v1_destroy(info->layer_surface);
	}

	wl_surface_destroy(info->surface);
	wl_display_roundtrip(wlc.display);

	info->hidden = 1;
    }

    // TODO replace window array with vector
    wlc.window_count = 0;
}

void ku_wayland_show_window(wl_window_t* info)
{
    if (info->hidden == 1)
    {
	info->hidden = 0;

	if (info->type == WL_WINDOW_LAYER)
	{
	    info->surface = wl_compositor_create_surface(wlc.compositor);
	    wl_surface_set_buffer_scale(info->surface, info->scale);

	    info->layer_surface = zwlr_layer_shell_v1_get_layer_surface(
		wlc.layer_shell,
		info->surface,
		info->monitor->wl_output,
		ZWLR_LAYER_SHELL_V1_LAYER_OVERLAY,
		"wcp");

	    zwlr_layer_surface_v1_set_size(
		info->layer_surface,
		info->width,
		info->height);

	    uint  af     = 0;
	    char* anchor = info->anchor;
	    while (*anchor != '\0')
	    {
		if (*anchor == 'l') af |= ZWLR_LAYER_SURFACE_V1_ANCHOR_LEFT;
		if (*anchor == 'r') af |= ZWLR_LAYER_SURFACE_V1_ANCHOR_RIGHT;
		if (*anchor == 't') af |= ZWLR_LAYER_SURFACE_V1_ANCHOR_TOP;
		if (*anchor == 'b') af |= ZWLR_LAYER_SURFACE_V1_ANCHOR_BOTTOM;
		anchor++;
	    }

	    zwlr_layer_surface_v1_set_anchor(
		info->layer_surface,
		af);

	    zwlr_layer_surface_v1_set_margin(
		info->layer_surface,
		info->margin,
		info->margin,
		info->margin,
		info->margin);

	    zwlr_layer_surface_v1_add_listener(info->layer_surface, &layer_surface_listener, info);

	    /* zwlr_layer_surface_v1_set_keyboard_interactivity(info->layer_surface, 1); */

	    wl_surface_commit(info->surface);
	    /* wl_display_roundtrip(wlc.display); */

	    /* info->frame_cb = wl_surface_frame(info->surface); */
	    /* wl_callback_add_listener(info->frame_cb, &wl_surface_frame_listener, info); */
	}
	else
	{
	    wl_surface_commit(info->surface);
	    wl_display_roundtrip(wlc.display);
	}
    }
}

wl_window_t* ku_wayland_create_generic_layer(struct monitor_info* monitor, int width, int height, int margin, char* anchor)
{
    wl_window_t* info = CAL(sizeof(wl_window_t), NULL, NULL);

    wlc.windows[wlc.window_count] = info;
    wlc.window_count += 1;

    info->scale         = monitor->scale;
    info->width         = width;
    info->height        = height;
    info->buffer_width  = width * info->scale;
    info->buffer_height = height * info->scale;
    info->margin        = margin;
    info->monitor       = monitor;
    info->hidden        = 1;
    memcpy(info->anchor, anchor, 4);

    info->type = WL_WINDOW_LAYER;

    return info;
}

/* resizes buffer on surface configure */

void ku_wayland_resize_window_buffer(wl_window_t* info)
{
    if (info->new_width != info->width || info->new_height != info->height || info->new_scale != info->scale)
    {
	int32_t nwidth  = round_to_int(info->new_width * info->new_scale);
	int32_t nheight = round_to_int(info->new_height * info->new_scale);

	info->scale  = info->new_scale;
	info->width  = nwidth;
	info->height = nheight;

	ku_wayland_create_buffer(info, nwidth, nheight);

	wl_surface_attach(info->surface, info->buffer, 0, 0);
	wl_surface_commit(info->surface);
    }
}

/* update surface with bitmap data */

void ku_wayland_draw_window(wl_window_t* info, int x, int y, int w, int h)
{
    if (info->hidden == 0)
    {
	/* Reattach backing buffer or swap context */
	if (info->type == WL_WINDOW_NATIVE || info->type == WL_WINDOW_LAYER)
	{
	    wl_surface_attach(info->surface, info->buffer, 0, 0);
	    wl_surface_damage(info->surface, x, y, w, h);
	    wl_surface_commit(info->surface);
	}
	else if (info->type == WL_WINDOW_EGL)
	{
	    eglSwapBuffers(wlc.windows[0]->egldisplay, wlc.windows[0]->eglsurface);
	}
    }
}

void ku_wayland_request_frame(wl_window_t* info)
{
    if (info->hidden == 0)
    {
	/* Request another frame */
	if (info->frame_cb == NULL)
	{
	    info->frame_cb = wl_surface_frame(info->surface);
	    wl_callback_add_listener(info->frame_cb, &wl_surface_frame_listener, info);
	}
    }
}

/* gesture listener */

static void gesture_hold_begin(void* data, struct zwp_pointer_gesture_hold_v1* hold, uint32_t serial, uint32_t time, struct wl_surface* surface, uint32_t fingers)
{
    mt_log_debug("hold start");
}

static void gesture_hold_end(void* data, struct zwp_pointer_gesture_hold_v1* hold, uint32_t serial, uint32_t time, int32_t cancelled)
{
    mt_log_debug("hold end");
}

static const struct zwp_pointer_gesture_hold_v1_listener gesture_hold_listener = {
    gesture_hold_begin,
    gesture_hold_end};

static void gesture_pinch_begin(void* data, struct zwp_pointer_gesture_pinch_v1* pinch, uint32_t serial, uint32_t time, struct wl_surface* surface, uint32_t fingers)
{
    /* GdkWaylandSeat* seat = data; */

    /* emit_gesture_pinch_event(seat, GDK_TOUCHPAD_GESTURE_PHASE_BEGIN, time, fingers, 0, 0, 1, 0); */
    /* seat->gesture_n_fingers = fingers; */

    for (int index = 0; index < wlc.window_count; index++)
    {
	wl_window_t* window = wlc.windows[index];

	if (window->surface == surface && window->monitor)
	{
	    window->pinch         = pinch;
	    window->pointer.scale = 1.0;
	}
    }
}

static void gesture_pinch_update(void* data, struct zwp_pointer_gesture_pinch_v1* pinch, uint32_t time, wl_fixed_t dx, wl_fixed_t dy, wl_fixed_t scale, wl_fixed_t rotation)
{
    /* mt_log_debug("pinch dx %f dy %f scale %f rotation %f", wl_fixed_to_double(dx), wl_fixed_to_double(dy), wl_fixed_to_double(scale), wl_fixed_to_double(rotation)); */

    for (int index = 0; index < wlc.window_count; index++)
    {
	wl_window_t* window = wlc.windows[index];

	if (window->pinch == pinch)
	{
	    float delta           = wl_fixed_to_double(scale) - window->pointer.scale;
	    window->pointer.scale = wl_fixed_to_double(scale);

	    ku_event_t event = init_event();
	    event.type       = KU_EVENT_PINCH;
	    event.x          = window->pointer.px;
	    event.y          = window->pointer.py;
	    event.ratio      = delta;
	    event.ctrl_down  = wlc.keyboard.control;
	    event.shift_down = wlc.keyboard.shift;

	    (*wlc.update)(event);
	}
    }
}

static void gesture_pinch_end(void* data, struct zwp_pointer_gesture_pinch_v1* pinch, uint32_t serial, uint32_t time, int32_t cancelled)
{
    /* mt_log_debug("pinch end"); */
}

static const struct zwp_pointer_gesture_pinch_v1_listener gesture_pinch_listener = {
    gesture_pinch_begin,
    gesture_pinch_update,
    gesture_pinch_end};

/* pointer listener */

void ku_wayland_pointer_handle_enter(void* data, struct wl_pointer* wl_pointer, uint serial, struct wl_surface* surface, wl_fixed_t surface_x, wl_fixed_t surface_y)
{
    /* mt_log_debug("pointer handle enter %zu", (size_t) wl_pointer); */

    struct wl_buffer*       buffer;
    struct wl_cursor*       cursor = wlc.default_cursor;
    struct wl_cursor_image* image;

    if (wlc.windows[0]->fullscreen)
	wl_pointer_set_cursor(wl_pointer, serial, NULL, 0, 0);
    else if (cursor)
    {
	image  = wlc.default_cursor->images[0];
	buffer = wl_cursor_image_get_buffer(image);
	wl_pointer_set_cursor(wl_pointer, serial, wlc.cursor_surface, image->hotspot_x, image->hotspot_y);
	wl_surface_attach(wlc.cursor_surface, buffer, 0, 0);
	wl_surface_damage(wlc.cursor_surface, 0, 0, image->width, image->height);
	wl_surface_commit(wlc.cursor_surface);
    }
    for (int index = 0; index < wlc.window_count; index++)
    {
	wl_window_t* window = wlc.windows[index];

	if (window->surface == surface && window->monitor)
	{
	    window->wl_pointer = wl_pointer;

	    ku_event_t event = init_event();
	    event.type       = KU_EVENT_MOUSE_MOVE;
	    event.drag       = window->pointer.drag;
	    event.x          = (int) wl_fixed_to_double(surface_x) * window->monitor->scale;
	    event.y          = (int) wl_fixed_to_double(surface_y) * window->monitor->scale;
	    event.ctrl_down  = wlc.keyboard.control;
	    event.shift_down = wlc.keyboard.shift;
	    event.window     = (void*) window;

	    window->pointer.px = event.x;
	    window->pointer.py = event.y;

	    (*wlc.update)(event);
	}
    }
}

void ku_wayland_pointer_handle_leave(void* data, struct wl_pointer* wl_pointer, uint serial, struct wl_surface* surface)
{
    /* mt_log_debug("pointer handle leave %zu", (size_t) wl_pointer); */

    for (int index = 0; index < wlc.window_count; index++)
    {
	wl_window_t* window = wlc.windows[index];

	if (window->surface == surface)
	{
	    ku_event_t event = init_event();
	    event.type       = KU_EVENT_MOUSE_MOVE;
	    event.drag       = window->pointer.drag;
	    event.x          = -100;
	    event.y          = -100;
	    event.ctrl_down  = wlc.keyboard.control;
	    event.shift_down = wlc.keyboard.shift;
	    event.window     = (void*) window;

	    window->pointer.px = event.x;
	    window->pointer.py = event.y;

	    (*wlc.update)(event);
	}
    }
}

void ku_wayland_pointer_handle_motion(void* data, struct wl_pointer* wl_pointer, uint time, wl_fixed_t surface_x, wl_fixed_t surface_y)
{
    /* mt_log_debug("pointer handle motion %zu %f %f", (size_t) wl_pointer, wl_fixed_to_double(surface_x), wl_fixed_to_double(surface_y)); */

    for (int index = 0; index < wlc.window_count; index++)
    {
	wl_window_t* window = wlc.windows[index];

	if (window->wl_pointer == wl_pointer && window->monitor)
	{
	    window->pointer.drag = window->pointer.down;

	    ku_event_t event = init_event();
	    event.type       = KU_EVENT_MOUSE_MOVE;
	    event.drag       = window->pointer.drag;
	    event.x          = (int) wl_fixed_to_double(surface_x) * window->monitor->scale;
	    event.y          = (int) wl_fixed_to_double(surface_y) * window->monitor->scale;
	    event.ctrl_down  = wlc.keyboard.control;
	    event.shift_down = wlc.keyboard.shift;

	    window->pointer.px = event.x;
	    window->pointer.py = event.y;

	    (*wlc.update)(event);
	}
    }
}

void ku_wayland_pointer_handle_button(void* data, struct wl_pointer* wl_pointer, uint serial, uint time, uint button, uint state)
{
    /* mt_log_debug("pointer handle button %u state %u time %u", button, state, time); */

    for (int index = 0; index < wlc.window_count; index++)
    {
	wl_window_t* window = wlc.windows[index];

	if (window->wl_pointer == wl_pointer)
	{
	    window->pointer.drag = window->pointer.down;

	    ku_event_t event = init_event();
	    event.x          = window->pointer.px;
	    event.y          = window->pointer.py;
	    event.button     = button == 272 ? 1 : 3;
	    event.ctrl_down  = wlc.keyboard.control;
	    event.shift_down = wlc.keyboard.shift;

	    if (state)
	    {
		uint delay               = time - window->pointer.lastdown;
		window->pointer.lastdown = time;

		event.dclick         = delay < 300;
		event.type           = KU_EVENT_MOUSE_DOWN;
		window->pointer.down = 1;
	    }
	    else
	    {
		event.type           = KU_EVENT_MOUSE_UP;
		event.drag           = window->pointer.drag;
		window->pointer.drag = 0;
		window->pointer.down = 0;
	    }

	    (*wlc.update)(event);
	}
    }
}

void ku_wayland_pointer_handle_axis(void* data, struct wl_pointer* wl_pointer, uint time, uint axis, wl_fixed_t value)
{
    /* mt_log_debug("pointer handle axis %u %i", axis, value); */

    for (int index = 0; index < wlc.window_count; index++)
    {
	wl_window_t* window = wlc.windows[index];

	if (window->wl_pointer == wl_pointer)
	{
	    ku_event_t event = init_event();
	    event.type       = KU_EVENT_SCROLL;
	    event.x          = window->pointer.px;
	    event.y          = window->pointer.py;
	    event.dx         = axis == 1 ? (float) value / 200.0 : 0;
	    event.dy         = axis == 0 ? (float) -value / 200.0 : 0;
	    event.ctrl_down  = wlc.keyboard.control;
	    event.shift_down = wlc.keyboard.shift;

	    (*wlc.update)(event);
	}
    }
}

void ku_wayland_pointer_handle_frame(void* data, struct wl_pointer* wl_pointer)
{
    /* mt_log_debug("pointer handle frame"); */
}

void ku_wayland_pointer_handle_axis_source(void* data, struct wl_pointer* wl_pointer, uint axis_source)
{
    /* mt_log_debug("pointer handle axis source"); */
}

void ku_wayland_pointer_handle_axis_stop(void* data, struct wl_pointer* wl_pointer, uint time, uint axis)
{
    /* mt_log_debug("pointer handle axis stop"); */
}

void ku_wayland_pointer_handle_axis_discrete(void* data, struct wl_pointer* wl_pointer, uint axis, int discrete)
{
    /* mt_log_debug("pointer handle axis discrete"); */
}

struct wl_pointer_listener pointer_listener =
    {
	.enter         = ku_wayland_pointer_handle_enter,
	.leave         = ku_wayland_pointer_handle_leave,
	.motion        = ku_wayland_pointer_handle_motion,
	.button        = ku_wayland_pointer_handle_button,
	.axis          = ku_wayland_pointer_handle_axis,
	.frame         = ku_wayland_pointer_handle_frame,
	.axis_source   = ku_wayland_pointer_handle_axis_source,
	.axis_stop     = ku_wayland_pointer_handle_axis_stop,
	.axis_discrete = ku_wayland_pointer_handle_axis_discrete,
};

/* lkeyboard listener */

static void keyboard_keymap(void* data, struct wl_keyboard* wl_keyboard, uint32_t format, int32_t fd, uint32_t size)
{
    /* mt_log_debug("keyboard keymap"); */

    wlc.keyboard.xkb_context = xkb_context_new(XKB_CONTEXT_NO_FLAGS);

    if (format != WL_KEYBOARD_KEYMAP_FORMAT_XKB_V1)
    {
	close(fd);
	exit(1);
    }
    char* map_shm = mmap(NULL, size, PROT_READ, MAP_SHARED, fd, 0);
    if (map_shm == MAP_FAILED)
    {
	close(fd);
	exit(1);
    }

    wlc.keyboard.xkb_keymap = xkb_keymap_new_from_string(wlc.keyboard.xkb_context, map_shm, XKB_KEYMAP_FORMAT_TEXT_V1, 0);
    munmap(map_shm, size);
    close(fd);

    wlc.keyboard.xkb_state = xkb_state_new(wlc.keyboard.xkb_keymap);
}

static void keyboard_enter(void* data, struct wl_keyboard* wl_keyboard, uint32_t serial, struct wl_surface* surface, struct wl_array* keys)
{
    /* mt_log_debug("keyboard enter"); */
}

static void keyboard_leave(void* data, struct wl_keyboard* wl_keyboard, uint32_t serial, struct wl_surface* surface)
{
    /* mt_log_debug("keyboard leave"); */
}

static void keyboard_key(void* data, struct wl_keyboard* wl_keyboard, uint32_t serial, uint32_t time, uint32_t key, uint32_t _key_state)
{
    enum wl_keyboard_key_state key_state = _key_state;

    xkb_keysym_t sym = xkb_state_key_get_one_sym(wlc.keyboard.xkb_state, key + 8);

    /* switch (xkb_keysym_to_lower(sym)) */

    if (!(sym == XKB_KEY_BackSpace ||
	  sym == XKB_KEY_Escape ||
	  sym == XKB_KEY_Return ||
	  sym == XKB_KEY_Print ||
	  sym == XKB_KEY_Tab))
    {
	/* send text event */

	char buf[8];
	if (xkb_keysym_to_utf8(sym, buf, 8))
	{
	    if (key_state == WL_KEYBOARD_KEY_STATE_PRESSED)
	    {
		ku_event_t event = init_event();
		event.keycode    = sym;
		event.type       = KU_EVENT_TEXT;
		event.ctrl_down  = wlc.keyboard.control;
		event.shift_down = wlc.keyboard.shift;

		memcpy(event.text, buf, 8);
		(*wlc.update)(event);
	    }
	}
    }

    if (key_state == WL_KEYBOARD_KEY_STATE_PRESSED && wlc.keyboard.rep_period > 0)
    {
	/* start repeater */

	struct itimerspec spec = {0};
	spec.it_value.tv_sec   = wlc.keyboard.rep_delay / 1000;
	spec.it_value.tv_nsec  = (wlc.keyboard.rep_delay % 1000) * 1000000l;
	timerfd_settime(wlc.keyboard.rep_timer_fd, 0, &spec, NULL);
    }
    else if (key_state == WL_KEYBOARD_KEY_STATE_RELEASED)
    {
	/* stop repeater */

	struct itimerspec spec = {0};
	timerfd_settime(wlc.keyboard.rep_timer_fd, 0, &spec, NULL);

	ku_event_t event = wlc.keyboard.rep_event;
	event.type       = KU_EVENT_KEY_UP;
	event.repeat     = 1;
	(*wlc.update)(event);
    }

    /* send key down/up event */

    ku_event_t event = init_event();
    event.keycode    = sym;
    event.type       = key_state == WL_KEYBOARD_KEY_STATE_PRESSED ? KU_EVENT_KEY_DOWN : KU_EVENT_KEY_UP;
    event.ctrl_down  = wlc.keyboard.control;
    event.shift_down = wlc.keyboard.shift;
    (*wlc.update)(event);

    wlc.keyboard.rep_event = event;
}

static void keyboard_repeat()
{
    /* resend last key event */
    ku_event_t event = wlc.keyboard.rep_event;

    event.repeat = 1;
    (*wlc.update)(event);

    /* reset timer */
    struct itimerspec spec = {0};
    spec.it_value.tv_sec   = wlc.keyboard.rep_period / 1000;
    spec.it_value.tv_nsec  = (wlc.keyboard.rep_period % 1000) * 1000000l;
    timerfd_settime(wlc.keyboard.rep_timer_fd, 0, &spec, NULL);
}

static void keyboard_repeat_info(void* data, struct wl_keyboard* wl_keyboard, int32_t rate, int32_t delay)
{
    /* store systems key repeat time */
    wlc.keyboard.rep_delay = delay;
    if (rate > 0)
    {
	wlc.keyboard.rep_period = 1000 / rate;
    }
    else
    {
	wlc.keyboard.rep_period = -1;
    }
}

static void keyboard_modifiers(void* data, struct wl_keyboard* keyboard, uint32_t serial, uint32_t mods_depressed, uint32_t mods_latched, uint32_t mods_locked, uint32_t group)
{
    /* mt_log_debug("keyboard modifiers"); */
    xkb_state_update_mask(wlc.keyboard.xkb_state, mods_depressed, mods_latched, mods_locked, 0, 0, group);
    wlc.keyboard.control = xkb_state_mod_name_is_active(wlc.keyboard.xkb_state, XKB_MOD_NAME_CTRL, XKB_STATE_MODS_DEPRESSED | XKB_STATE_MODS_LATCHED);
    wlc.keyboard.shift   = xkb_state_mod_name_is_active(wlc.keyboard.xkb_state, XKB_MOD_NAME_SHIFT, XKB_STATE_MODS_DEPRESSED | XKB_STATE_MODS_LATCHED);
}

static const struct wl_keyboard_listener keyboard_listener = {
    .keymap      = keyboard_keymap,
    .enter       = keyboard_enter,
    .leave       = keyboard_leave,
    .key         = keyboard_key,
    .modifiers   = keyboard_modifiers,
    .repeat_info = keyboard_repeat_info,
};

/* wm base listener */

static void xdg_wm_base_ping(void* data, struct xdg_wm_base* xdg_wm_base, uint32_t serial)
{
    xdg_wm_base_pong(xdg_wm_base, serial);
}

static const struct xdg_wm_base_listener xdg_wm_base_listener = {
    .ping = xdg_wm_base_ping,
};

/* xdg output events */

static void ku_wayland_xdg_output_handle_logical_position(void* data, struct zxdg_output_v1* xdg_output, int32_t x, int32_t y)
{
    /* struct monitor_info* monitor = data; */
    /* mt_log_debug("xdg output handle logical position, %i %i for monitor %i", x, y, monitor->index); */
}

static void ku_wayland_xdg_output_handle_logical_size(void* data, struct zxdg_output_v1* xdg_output, int32_t width, int32_t height)
{
    struct monitor_info* monitor = data;
    mt_log_debug("xdg output handle logical size, %i %i for monitor %i", width, height, monitor->index);

    monitor->logical_width  = width;
    monitor->logical_height = height;
}

static void ku_wayland_xdg_output_handle_done(void* data, struct zxdg_output_v1* xdg_output)
{
    /* struct monitor_info* monitor = data; */
    /* mt_log_debug("xdg output handle done, for monitor %i", monitor->index); */
}

static void ku_wayland_xdg_output_handle_name(void* data, struct zxdg_output_v1* xdg_output, const char* name)
{
    struct monitor_info* monitor = data;
    strncpy(monitor->name, name, MAX_MONITOR_NAME_LEN);

    /* mt_log_debug("xdg output handle name, %s for monitor %i", name, monitor->index); */
}

static void ku_wayland_xdg_output_handle_description(void* data, struct zxdg_output_v1* xdg_output, const char* description)
{
    /* struct monitor_info* monitor = data; */
    /* mt_log_debug("xdg output handle description for monitor %i", description, monitor->index); */
}

struct zxdg_output_v1_listener xdg_output_listener = {
    .logical_position = ku_wayland_xdg_output_handle_logical_position,
    .logical_size     = ku_wayland_xdg_output_handle_logical_size,
    .done             = ku_wayland_xdg_output_handle_done,
    .name             = ku_wayland_xdg_output_handle_name,
    .description      = ku_wayland_xdg_output_handle_description,
};

/* output events */

static void ku_wayland_wl_output_handle_geometry(
    void*             data,
    struct wl_output* wl_output,
    int32_t           x,
    int32_t           y,
    int32_t           width_mm,
    int32_t           height_mm,
    int32_t           subpixel,
    const char*       make,
    const char*       model,
    int32_t           transform)
{
    struct monitor_info* monitor = data;

    /* mt_log_debug( */
    /* 	"wl output handle geometry x %i y %i width_mm %i height_mm %i subpixel %i make %s model %s transform %i for monitor %i", */
    /* 	x, */
    /* 	y, */
    /* 	width_mm, */
    /* 	height_mm, */
    /* 	subpixel, */
    /* 	make, */
    /* 	model, */
    /* 	transform, */
    /* 	monitor->index); */

    monitor->subpixel = subpixel;
}

static void ku_wayland_wl_output_handle_mode(
    void*             data,
    struct wl_output* wl_output,
    uint32_t          flags,
    int32_t           width,
    int32_t           height,
    int32_t           refresh)
{
    struct monitor_info* monitor = data;

    /* mt_log_debug( */
    /* 	"wl output handle mode flags %u width %i height %i for monitor %i", */
    /* 	flags, */
    /* 	width, */
    /* 	height, */
    /* 	monitor->index); */

    monitor->physical_width  = width;
    monitor->physical_height = height;
}

static void ku_wayland_wl_output_handle_done(void* data, struct wl_output* wl_output)
{
    /* struct monitor_info* monitor = data; */

    /* mt_log_debug("wl output handle done for monitor %i", monitor->index); */
}

static void ku_wayland_wl_output_handle_scale(void* data, struct wl_output* wl_output, int32_t factor)
{
    struct monitor_info* monitor = data;

    /* mt_log_debug("wl output handle scale %i for monitor %i", factor, monitor->index); */

    monitor->scale = factor;
}

struct wl_output_listener wl_output_listener = {
    .geometry = ku_wayland_wl_output_handle_geometry,
    .mode     = ku_wayland_wl_output_handle_mode,
    .done     = ku_wayland_wl_output_handle_done,
    .scale    = ku_wayland_wl_output_handle_scale,
};

/* seat events */

static void ku_wayland_seat_handle_capabilities(void* data, struct wl_seat* wl_seat, enum wl_seat_capability caps)
{
    /* mt_log_debug("seat handle capabilities %i", caps); */

    if (caps & WL_SEAT_CAPABILITY_KEYBOARD)
    {
	wlc.keyboard.kbd = wl_seat_get_keyboard(wl_seat);
	wl_keyboard_add_listener(wlc.keyboard.kbd, &keyboard_listener, NULL);
    }

    if (caps & WL_SEAT_CAPABILITY_POINTER)
    {
	wlc.ipointer = wl_seat_get_pointer(wl_seat);
	wl_pointer_add_listener(wlc.ipointer, &pointer_listener, NULL);

	if (wlc.pointer_manager)
	{
	    wlc.pinch_gesture = zwp_pointer_gestures_v1_get_pinch_gesture(wlc.pointer_manager, wlc.ipointer);
	    zwp_pointer_gesture_pinch_v1_add_listener(wlc.pinch_gesture, &gesture_pinch_listener, wl_seat);

	    if (wlc.pointer_manager_version >= ZWP_POINTER_GESTURES_V1_GET_HOLD_GESTURE_SINCE_VERSION)
	    {
		wlc.hold_gesture = zwp_pointer_gestures_v1_get_hold_gesture(wlc.pointer_manager, wlc.ipointer);
		zwp_pointer_gesture_hold_v1_add_listener(wlc.hold_gesture, &gesture_hold_listener, wl_seat);
	    }
	}
    }
}

static void ku_wayland_seat_handle_name(void* data, struct wl_seat* wl_seat, const char* name)
{
    /* mt_log_debug("seat handle name %s", name); */
}

const struct wl_seat_listener seat_listener = {
    .capabilities = ku_wayland_seat_handle_capabilities,
    .name         = ku_wayland_seat_handle_name,
};

/* global events */
/* TODO check if we can use newer versions of the interfaces */

static void ku_wayland_handle_global(
    void*               data,
    struct wl_registry* registry,
    uint32_t            name,
    const char*         interface,
    uint32_t            version)
{
    /* mt_log_debug("handle global : %s, version %u", interface, version); */

    if (strcmp(interface, wl_compositor_interface.name) == 0)
    {
	/* TODO LEAKS!!! */
	wlc.compositor = wl_registry_bind(registry, name, &wl_compositor_interface, 4);
    }
    else if (strcmp(interface, wl_seat_interface.name) == 0)
    {
	wlc.seat = wl_registry_bind(registry, name, &wl_seat_interface, 4);
	wl_seat_add_listener(wlc.seat, &seat_listener, NULL);
    }
    else if (strcmp(interface, wl_shm_interface.name) == 0)
    {
	wlc.shm = wl_registry_bind(registry, name, &wl_shm_interface, 1);

	wlc.cursor_theme   = wl_cursor_theme_load(NULL, 16, wlc.shm);
	wlc.default_cursor = wl_cursor_theme_get_cursor(wlc.cursor_theme, "left_ptr");
    }
    else if (strcmp(interface, wl_output_interface.name) == 0)
    {
	if (wlc.monitor_count >= 16) return;

	struct monitor_info* monitor = malloc(sizeof(struct monitor_info));
	memset(monitor->name, 0, MAX_MONITOR_NAME_LEN);
	monitor->wl_output = wl_registry_bind(registry, name, &wl_output_interface, 2);
	monitor->index     = wlc.monitor_count;

	/* get wl_output events */
	wl_output_add_listener(monitor->wl_output, &wl_output_listener, monitor);

	/* set up output if it comes after xdg_output_manager_init */
	if (wlc.xdg_output_manager != NULL)
	{
	    monitor->xdg_output = zxdg_output_manager_v1_get_xdg_output(wlc.xdg_output_manager, monitor->wl_output);
	    zxdg_output_v1_add_listener(monitor->xdg_output, &xdg_output_listener, monitor);
	}

	wlc.monitors[wlc.monitor_count++] = monitor;
    }
    else if (strcmp(interface, zwlr_layer_shell_v1_interface.name) == 0)
    {
	wlc.layer_shell = wl_registry_bind(registry, name, &zwlr_layer_shell_v1_interface, 1);
    }
    else if (strcmp(interface, zxdg_output_manager_v1_interface.name) == 0)
    {
	wlc.xdg_output_manager = wl_registry_bind(registry, name, &zxdg_output_manager_v1_interface, 2);

	/* set up outputs if event comes after interface setup */
	for (int index = 0; index < wlc.monitor_count; index++)
	{
	    wlc.monitors[index]->xdg_output = zxdg_output_manager_v1_get_xdg_output(wlc.xdg_output_manager, wlc.monitors[index]->wl_output);
	    zxdg_output_v1_add_listener(wlc.monitors[index]->xdg_output, &xdg_output_listener, wlc.monitors[index]);
	}
    }
    else if (strcmp(interface, xdg_wm_base_interface.name) == 0)
    {
	wlc.xdg_wm_base = wl_registry_bind(registry, name, &xdg_wm_base_interface, 1);
	xdg_wm_base_add_listener(wlc.xdg_wm_base, &xdg_wm_base_listener, NULL);
    }
    else if (strcmp(interface, zwp_pointer_gestures_v1_interface.name) == 0)
    {
	if (version >= 3)
	{
	    wlc.pointer_manager_version = version;
	    wlc.pointer_manager         = wl_registry_bind(registry, name, &zwp_pointer_gestures_v1_interface, 3);
	}
    }
}

static void ku_wayland_handle_global_remove(void* data, struct wl_registry* registry, uint32_t name)
{
    mt_log_debug("handle global remove");
}

static const struct wl_registry_listener registry_listener =
    {.global        = ku_wayland_handle_global,
     .global_remove = ku_wayland_handle_global_remove};

/* init wayland */

void ku_wayland_init(
    void (*init)(wl_event_t event),
    void (*update)(ku_event_t event),
    void (*destroy)(),
    int time_event_interval)
{
    wlc.monitors = CAL(sizeof(struct monitor_info) * 16, NULL, NULL);
    wlc.windows  = CAL(sizeof(wl_window_t) * 16, NULL, NULL);

    wlc.init    = init;
    wlc.update  = update;
    wlc.destroy = destroy;

    wlc.display = wl_display_connect(NULL);

    clock_gettime(CLOCK_REALTIME, &wlc.time_start);

    if (wlc.display)
    {
	wlc.time_event_timer_fd   = timerfd_create(CLOCK_MONOTONIC, 0);
	wlc.keyboard.rep_timer_fd = timerfd_create(CLOCK_MONOTONIC, 0);

	if (wlc.keyboard.rep_timer_fd < 0) mt_log_error("cannot create key repeat timer");

	struct wl_registry* registry = wl_display_get_registry(wlc.display);
	wl_registry_add_listener(registry, &registry_listener, NULL);

	/* first roundtrip triggers global events */
	wl_display_roundtrip(wlc.display);

	/* second roundtrip triggers events attached in global events */
	wl_display_roundtrip(wlc.display);

	if (wlc.compositor)
	{
	    wlc.cursor_surface = wl_compositor_create_surface(wlc.compositor);

	    /* dispatch init event */
	    struct _wl_event_t event = {
		.id            = WL_EVENT_OUTPUT_ADDED,
		.monitors      = wlc.monitors,
		.monitor_count = wlc.monitor_count};

	    (*wlc.init)(event);

	    /* file descriptors */
	    struct pollfd fds[] = {
		{.fd = wl_display_get_fd(wlc.display), .events = POLLIN},
		{.fd = wlc.keyboard.rep_timer_fd, .events = POLLIN},
		{.fd = wlc.time_event_timer_fd, .events = POLLIN},
		{.fd = STDIN_FILENO, .events = POLLIN}};

	    const int nfds = sizeof(fds) / sizeof(*fds);

	    /* wl_display_dispatch(wlc.display); */

	    ku_wayland_set_time_event_delay(time_event_interval);

	    while (!wlc.exit_flag)
	    {
		if (wl_display_flush(wlc.display) < 0)
		{
		    if (errno == EAGAIN) continue;
		    mt_log_error("wayland display flush error");
		    break;
		}

		if (poll(fds, nfds, -1) < 0)
		{
		    if (errno == EAGAIN) continue;
		    mt_log_error("poll error");
		    break;
		}

		if (fds[0].revents & POLLIN) /* wayland events */
		{
		    if (wl_display_dispatch(wlc.display) < 0)
		    {
			mt_log_error("wayland display dispatch error");
			break;
		    }
		}

		if (fds[1].revents & POLLIN) /* key repeat timer events */
		{
		    keyboard_repeat();
		}

		if (fds[2].revents & POLLIN) /* time event timer events */
		{
		    struct itimerspec spec = {0};
		    spec.it_value.tv_sec   = wlc.time_event_interval / 1000;
		    spec.it_value.tv_nsec  = (wlc.time_event_interval % 1000) * 1000000l;
		    timerfd_settime(wlc.time_event_timer_fd, 0, &spec, NULL);

		    ku_event_t event = init_event();
		    event.type       = KU_EVENT_TIME;

		    (*wlc.update)(event);
		}

		if (fds[3].revents & POLLIN) /* stdin events */
		{
		    char buffer[3] = {0};
		    while (fgets(buffer, 3, stdin))
		    {
			ku_event_t event = init_event();
			event.type       = KU_EVENT_STDIN;
			event.text[0]    = buffer[0];
			event.text[1]    = '\0';

			(*wlc.update)(event);
			break;
		    }
		}
	    }

	    (*wlc.destroy)();

	    wl_surface_destroy(wlc.cursor_surface);
	    if (wlc.cursor_theme) wl_cursor_theme_destroy(wlc.cursor_theme);

	    wl_compositor_destroy(wlc.compositor);
	}
	else
	    mt_log_error("compositor not found");

	for (int m = 0; m < wlc.monitor_count; m++) free(wlc.monitors[m]);
	for (int w = 0; w < wlc.window_count; w++) REL(wlc.windows[w]);

	wl_display_disconnect(wlc.display);
    }
    else
	mt_log_debug("cannot open display");

    REL(wlc.monitors);
    REL(wlc.windows);
}

/* request exit */

void ku_wayland_exit()
{
    wlc.exit_flag = 1;
    wl_display_flush(wlc.display);
}

/* request fullscreen */

void ku_wayland_toggle_fullscreen(wl_window_t* window)
{
    if (wlc.windows[0]->fullscreen == 0)
    {
	xdg_toplevel_set_fullscreen(window->xdg_toplevel, window->monitor->wl_output);
	window->fullscreen = 1;
    }
    else
    {
	xdg_toplevel_unset_fullscreen(window->xdg_toplevel);
	window->fullscreen = 0;
    }
    wl_display_flush(wlc.display);
}

/* set timer delay */

void ku_wayland_set_time_event_delay(int ms)
{
    wlc.time_event_interval = ms;
    struct itimerspec spec  = {0};
    if (ms > 0)
    {
	spec.it_value.tv_sec  = ms / 1000;
	spec.it_value.tv_nsec = (ms % 1000) * 1000000l;
    }
    timerfd_settime(wlc.time_event_timer_fd, 0, &spec, NULL);
}

#endif
