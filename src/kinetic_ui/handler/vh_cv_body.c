/* content view body */

#ifndef vh_cv_body_h
#define vh_cv_body_h

#include "ku_view.c"

typedef struct _vh_cv_body_t
{
    void* userdata;

    ku_view_t* content;
    float      cw; // content width
    float      ch; // content height
    float      px;
    float      py;
    float      zoom;
} vh_cv_body_t;

void vh_cv_body_attach(
    ku_view_t* view,
    void*      userdata);

void vh_cv_body_set_content_size(
    ku_view_t* view,
    int        cw,
    int        ch);

void vh_cv_body_move(
    ku_view_t* view,
    float      dx,
    float      dy);

void vh_cv_body_zoom(
    ku_view_t* view,
    float      s,
    int        x,
    int        y);

void vh_cv_body_reset(
    ku_view_t* view);

void vh_cv_body_hjump(
    ku_view_t* view,
    float      dx);

void vh_cv_body_vjump(
    ku_view_t* view,
    int        topindex);

#endif

#if __INCLUDE_LEVEL__ == 0

#include "tg_scaledimg.c"

void vh_cv_body_del(void* p)
{
}

void vh_cv_body_desc(void* p, int level)
{
    printf("vh_cv_body");
}

void vh_cv_body_attach(
    ku_view_t* view,
    void*      userdata)
{
    assert(view->handler == NULL && view->handler_data == NULL);

    vh_cv_body_t* vh = CAL(sizeof(vh_cv_body_t), vh_cv_body_del, vh_cv_body_desc);
    vh->userdata     = userdata;
    vh->content      = view->views->data[0];
    vh->zoom         = 1.0;

    vh->cw = 1;
    vh->ch = 1;

    view->handler_data = vh;
}

void vh_cv_body_set_content_size(
    ku_view_t* view,
    int        cw,
    int        ch)
{
    vh_cv_body_t* vh = view->handler_data;

    vh->cw = cw;
    vh->ch = ch;

    ku_rect_t lf = view->frame.local; // local frame

    float cr = (float) ch / (float) cw; // content aspect ratio

    /* fit width first */

    float nw = lf.w;
    float nh = nw * cr;

    vh->zoom = (float) nw / cw;

    if (nh > lf.h)
    {
	cr = lf.h / nh;

	nh = lf.h;
	nw *= cr;
	vh->zoom = (float) nh / ch;
    }

    ku_rect_t frame = vh->content->frame.local;
    frame.x         = (lf.w - nw) / 2.0;
    frame.y         = (lf.h - nh) / 2.0;
    frame.w         = nw;
    frame.h         = nh;

    ku_view_set_frame(vh->content, frame);

    tg_scaledimg_set_content_size(vh->content, cw, ch);
    tg_scaledimg_gen(vh->content);
}

void vh_cv_body_move(
    ku_view_t* view,
    float      dx,
    float      dy)
{
    vh_cv_body_t* vh = view->handler_data;

    ku_rect_t frame = vh->content->frame.local;
    frame.x += dx;
    frame.y += dy;
    ku_view_set_frame(vh->content, frame);
}

void vh_cv_body_zoom(
    ku_view_t* view,
    float      z,
    int        x,
    int        y)
{
    vh_cv_body_t* vh = view->handler_data;

    ku_rect_t gf = vh->content->frame.global;
    ku_rect_t lf = vh->content->frame.local;

    /* partial width and height from mouse position */

    float pw = (float) x - gf.x;
    float ph = (float) y - gf.y;

    /* ratios */

    float rw = pw / gf.w;
    float rh = ph / gf.h;

    vh->zoom = z;
    if (vh->zoom < 0.001) vh->zoom = 0.001;

    float nw = vh->cw * vh->zoom;
    float nh = vh->ch * vh->zoom;

    lf.x = (float) x - rw * nw - view->frame.global.x;
    lf.y = (float) y - rh * nh - view->frame.global.y;
    lf.w = nw;
    lf.h = nh;

    ku_view_set_frame(vh->content, lf);
    tg_scaledimg_gen(vh->content);
}

void vh_cv_body_reset(
    ku_view_t* view)
{
}

void vh_cv_body_hjump(
    ku_view_t* view,
    float      x)
{
}

void vh_cv_body_vjump(
    ku_view_t* view,
    int        topindex)
{
}

#endif
