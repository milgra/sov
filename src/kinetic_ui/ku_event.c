#ifndef ku_event_h
#define ku_event_h

#include <stdint.h>
#include <stdio.h>
#include <time.h>

enum evtype
{
    KU_EVENT_EMPTY,
    KU_EVENT_FRAME,
    KU_EVENT_TIME,
    KU_EVENT_RESIZE,
    KU_EVENT_MMOVE,
    KU_EVENT_MDOWN,
    KU_EVENT_MUP,
    KU_EVENT_MMOVE_OUT,
    KU_EVENT_MDOWN_OUT,
    KU_EVENT_MUP_OUT,
    KU_EVENT_SCROLL,
    KU_EVENT_KDOWN,
    KU_EVENT_KUP,
    KU_EVENT_TEXT,
    KU_EVENT_WINDOW_SHOW,
    KU_EVENT_PINCH,
    KU_EVENT_STDIN,
    KU_EVENT_FOCUS,
    KU_EVENT_UNFOCUS,
};

typedef struct _ku_event_t
{
    int type;

    /* poiniter properties */

    int x; // mouse coord x
    int y; // mouse coord y

    /* resize */

    int w; // resize width
    int h; // resize height

    /* scroll */

    float dx; // scroll x
    float dy; // scroll y

    /* pinch */

    float ratio; // pinch ratio

    /* mouse */

    int drag;   // mouse drag
    int dclick; // double click
    int button; // mouse button id

    /* time */

    uint32_t        time;       // milliseconds since start
    struct timespec time_unix;  // unix timestamp
    float           time_frame; // elapsed time since last frame
    uint32_t        frame;      // actual frame count

    /* keyboard */

    uint32_t keycode;
    int      repeat; // key event is coming from repeat

    int ctrl_down;  // modifiers
    int shift_down; // modifiers

    char text[8];

    /* window */

    void* window;

} ku_event_t;

void       ku_event_write(FILE* file, ku_event_t ev);
ku_event_t ku_event_read(FILE* file);

#endif

#if __INCLUDE_LEVEL__ == 0

/*                  frame type  x  y  w  h dx dy ratio drag dclick button time time_frame keycode repeat ctrl_down shift_down text */
char* ku_event_format = "%i %i %i %i %i %i %f %f %f %i %i %i %u %f %u %i %i %i %s\n";

void ku_event_write(FILE* file, ku_event_t ev)
{
    fprintf(
	file,
	ku_event_format,
	ev.frame,
	ev.type,
	ev.x,
	ev.y,
	ev.w,
	ev.h,
	ev.dx,
	ev.dy,
	ev.ratio,
	ev.drag,
	ev.dclick,
	ev.button,
	ev.time,
	ev.time_frame,
	ev.keycode,
	ev.repeat,
	ev.ctrl_down,
	ev.shift_down,
	ev.text[0] == '\0' ? "T" : ev.text);
}

ku_event_t ku_event_read(FILE* file)
{
    ku_event_t ev = {0};
    fscanf(
	file,
	ku_event_format,
	&ev.frame,
	&ev.type,
	&ev.x,
	&ev.y,
	&ev.w,
	&ev.h,
	&ev.dx,
	&ev.dy,
	&ev.ratio,
	&ev.drag,
	&ev.dclick,
	&ev.button,
	&ev.time,
	&ev.time_frame,
	&ev.keycode,
	&ev.repeat,
	&ev.ctrl_down,
	&ev.shift_down,
	&ev.text);

    return ev;
}

#endif
