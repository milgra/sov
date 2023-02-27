#ifndef ku_floatbuffer_h
#define ku_floatbuffer_h

#include "mt_memory.c"
#include <GL/glew.h>
#include <string.h>

typedef struct ku_floatbuffer_t ku_floatbuffer_t;
struct ku_floatbuffer_t
{
    GLfloat* data;
    size_t   pos;
    size_t   cap;
    char     changed;
};

ku_floatbuffer_t* ku_floatbuffer_new(void);
void              ku_floatbuffer_del(void* fb);
void              ku_floatbuffer_reset(ku_floatbuffer_t* fb);
void              ku_floatbuffer_add(ku_floatbuffer_t* fb, GLfloat* data, size_t count);

#endif

#if __INCLUDE_LEVEL__ == 0

void ku_floatbuffer_desc(void* p, int level)
{
    printf("fb");
}

void ku_floatbuffer_desc_data(void* p, int level)
{
    printf("fb data");
}

ku_floatbuffer_t* ku_floatbuffer_new()
{
    ku_floatbuffer_t* fb = CAL(sizeof(ku_floatbuffer_t), ku_floatbuffer_del, ku_floatbuffer_desc);
    fb->data             = CAL(sizeof(GLfloat) * 10, NULL, ku_floatbuffer_desc_data);
    fb->pos              = 0;
    fb->cap              = 10;

    return fb;
}

void ku_floatbuffer_del(void* pointer)
{
    ku_floatbuffer_t* fb = pointer;
    REL(fb->data);
}

void ku_floatbuffer_reset(ku_floatbuffer_t* fb)
{
    fb->pos = 0;
}

void ku_floatbuffer_expand(ku_floatbuffer_t* fb)
{
    assert(fb->cap < SIZE_MAX / 2);
    fb->cap *= 2;
    fb->data = mt_memory_realloc(fb->data, sizeof(void*) * fb->cap);
}

void ku_floatbuffer_add(ku_floatbuffer_t* fb, GLfloat* data, size_t count)
{
    while (fb->pos + count >= fb->cap) ku_floatbuffer_expand(fb);
    memcpy(fb->data + fb->pos, data, sizeof(GLfloat) * count);
    fb->pos += count;
    fb->changed = 1;
}

#endif
