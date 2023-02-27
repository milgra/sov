#ifndef ku_recorder_h
#define ku_recorder_h

#include "ku_event.c"

void ku_recorder_init(void (*update)(ku_event_t));
void ku_recorder_destroy();
void ku_recorder_update(ku_event_t ev);
void ku_recorder_record(char* path);
void ku_recorder_replay(char* path);

#endif

#if __INCLUDE_LEVEL__ == 0

#include "mt_vector.c"
#include <math.h>

enum ku_recorder_mode_t
{
    KU_REC_MODE_NORMAL,
    KU_REC_MODE_RECORD,
    KU_REC_MODE_REPLAY,
};

struct ku_recorder_t
{
    size_t       index;
    FILE*        file;
    mt_vector_t* eventqueue;
    void (*update)(ku_event_t);
    enum ku_recorder_mode_t mode;
} kurec;

void ku_recorder_init(void (*update)(ku_event_t))
{
    kurec.mode       = KU_REC_MODE_NORMAL;
    kurec.update     = update;
    kurec.eventqueue = VNEW();
}

void ku_recorder_record(char* path)
{
    kurec.mode = KU_REC_MODE_RECORD;
    FILE* file = fopen(path, "w");
    if (!file) printf("evrec recorder : cannot open file %s\n", path);
    kurec.file = file;
}

void ku_recorder_replay(char* path)
{
    kurec.mode = KU_REC_MODE_REPLAY;
    FILE* file = fopen(path, "r");
    if (!file)
	printf("evrec player : cannot open file %s\n", path);

    kurec.file = file;

    while (1)
    {
	ku_event_t ev = ku_event_read(file);
	VADDR(kurec.eventqueue, HEAP(ev));
	if (feof(file))
	    break;
    }

    printf("%li events read\n", kurec.eventqueue->length);
}

void ku_recorder_destroy()
{
    if (kurec.file) fclose(kurec.file);
    if (kurec.eventqueue) REL(kurec.eventqueue);
}

void ku_recorder_update_record(ku_event_t ev)
{
    /* normalize floats for deterministic movements during record/replay */
    ev.dx         = floor(ev.dx * 10000) / 10000;
    ev.dy         = floor(ev.dy * 10000) / 10000;
    ev.ratio      = floor(ev.ratio * 10000) / 10000;
    ev.time_frame = floor(ev.time_frame * 10000) / 10000;

    if (ev.type == KU_EVENT_FRAME || ev.type == KU_EVENT_TIME || ev.type == KU_EVENT_WINDOW_SHOWN)
    {
	/* record and send waiting events */
	for (size_t index = 0; index < kurec.eventqueue->length; index++)
	{
	    ku_event_t* event = (ku_event_t*) kurec.eventqueue->data[index];
	    event->frame      = ev.frame;

	    ku_event_write(kurec.file, *event);

	    (*kurec.update)(*event);
	}

	mt_vector_reset(kurec.eventqueue);

	/* send frame event */
	(*kurec.update)(ev);
    }
    else
    {
	/* queue event */
	void* event = HEAP(ev);
	VADDR(kurec.eventqueue, event);
    }
}

void ku_recorder_update_replay(ku_event_t ev)
{
    if (ev.type == KU_EVENT_FRAME || ev.type == KU_EVENT_WINDOW_SHOWN)
    {
	while (kurec.index < kurec.eventqueue->length)
	{
	    ku_event_t* event = kurec.eventqueue->data[kurec.index];

	    if (event->frame < ev.frame)
	    {
		kurec.index++;
		(*kurec.update)(*event);
	    }
	    else break;
	}

	(*kurec.update)(ev);
    }
}

void ku_recorder_update(ku_event_t ev)
{
    if (kurec.mode == KU_REC_MODE_NORMAL) (*kurec.update)(ev);
    else if (kurec.mode == KU_REC_MODE_RECORD) ku_recorder_update_record(ev);
    else if (kurec.mode == KU_REC_MODE_REPLAY) ku_recorder_update_replay(ev);
}

#endif
