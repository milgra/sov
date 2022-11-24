#ifndef vh_table_h
#define vh_table_h

#include "ku_text.c"
#include "ku_view.c"
#include "mt_vector.c"
#include <stdint.h>

typedef struct _vh_table_t vh_table_t;

enum vh_table_event_id
{
    VH_TABLE_EVENT_SELECT,
    VH_TABLE_EVENT_OPEN,
    VH_TABLE_EVENT_CONTEXT,
    VH_TABLE_EVENT_DRAG,
    VH_TABLE_EVENT_DROP,
    VH_TABLE_EVENT_KEY_DOWN,
    VH_TABLE_EVENT_KEY_UP,
    VH_TABLE_EVENT_FIELDS_UPDATE,
    VH_TABLE_EVENT_FIELD_SELECT
};

typedef struct _vh_table_event_t
{
    enum vh_table_event_id id;
    vh_table_t*            table;
    char*                  field;
    mt_vector_t*           fields;
    mt_vector_t*           selected_items;
    int32_t                selected_index;
    ku_view_t*             rowview;
    ku_event_t             ev;
    ku_view_t*             view;
} vh_table_event_t;

struct _vh_table_t
{
    char*      id;  // unique id for item generation
    uint32_t   cnt; // item count for item generation
    ku_view_t* view;

    mt_vector_t* items;  // data items
    mt_vector_t* cache;  // item cache
    mt_vector_t* fields; // field name field size interleaved vector

    mt_vector_t* selected_items; // selected items
    int32_t      selected_index; // index of last selected
    ku_view_t*   body_v;
    ku_view_t*   evnt_v;
    ku_view_t*   scrl_v;
    ku_view_t*   head_v;
    ku_view_t*   layr_v;

    textstyle_t rowastyle; // alternating row a style
    textstyle_t rowbstyle; // alternating row b style
    textstyle_t rowsstyle; // selected row textstyle
    textstyle_t headstyle; // header textstyle

    void (*on_event)(vh_table_event_t event);
};

void vh_table_attach(
    ku_view_t*   view,
    mt_vector_t* fields,
    void (*on_event)(vh_table_event_t event));

void vh_table_select(
    ku_view_t* view,
    int32_t    index,
    int        add);

void vh_table_set_data(
    ku_view_t*   view,
    mt_vector_t* data);

mt_vector_t* vh_table_get_fields(
    ku_view_t* view);

#endif

#if __INCLUDE_LEVEL__ == 0

#include "config.c"
#include "ku_gen_textstyle.c"
#include "mt_log.c"
#include "mt_memory.c"
#include "mt_number.c"
#include "mt_string.c"
#include "tg_css.c"
#include "tg_text.c"
#include "vh_tbl_body.c"
#include "vh_tbl_evnt.c"
#include "vh_tbl_head.c"
#include "vh_tbl_scrl.c"
#include <xkbcommon/xkbcommon.h>

/* header order/size change, update cells */

void vh_table_head_update_cells(vh_table_t* vh, int fixed_index, int fixed_pos)
{
    for (int ri = 0; ri < vh->body_v->views->length; ri++)
    {
	ku_view_t* rowview = vh->body_v->views->data[ri];
	float      wth     = 0;

	for (int ci = 0; ci < rowview->views->length; ci++)
	{
	    ku_view_t*   cellview = rowview->views->data[ci];
	    ku_rect_t    frame    = cellview->frame.local;
	    mt_number_t* sizep    = vh->fields->data[ci * 2 + 1];
	    frame.x               = ci == fixed_index ? (float) fixed_pos : wth;
	    frame.w               = (float) sizep->intv;
	    ku_view_set_frame(cellview, frame);
	    wth += frame.w + 2;
	}

	ku_rect_t frame = rowview->frame.local;
	frame.w         = wth;
	ku_view_set_frame(rowview, frame);
    }
}

/* header field moved, update cells */

void vh_table_head_move(ku_view_t* hview, int index, int pos, void* userdata)
{
    vh_table_t* vh = (vh_table_t*) userdata;

    vh_table_head_update_cells(vh, index, pos);
}

/* header field resized, update cells */

void vh_table_head_resize(ku_view_t* hview, int index, int size, void* userdata)
{
    vh_table_t* vh = (vh_table_t*) userdata;

    if (index > -1)
    {
	mt_number_t* sizep = vh->fields->data[index * 2 + 1];
	sizep->intv        = size;

	vh_table_head_update_cells(vh, -1, 0);
    }
    else
    {
	vh_table_event_t event = {.table = vh, .id = VH_TABLE_EVENT_FIELDS_UPDATE, .fields = vh->fields, .view = vh->view};
	(*vh->on_event)(event);
    }
}

/* header field reordered, update cells */

void vh_table_head_reorder(ku_view_t* hview, int ind1, int ind2, void* userdata)
{
    vh_table_t* vh = (vh_table_t*) userdata;

    if (ind1 == -1)
    {
	/* self click, dispatch event */
	char*            field = vh->fields->data[ind2 * 2];
	vh_table_event_t event = {.id = VH_TABLE_EVENT_FIELD_SELECT, .field = field, .view = vh->view};
	(*vh->on_event)(event);
    }
    else
    {
	char*        field1 = vh->fields->data[ind1 * 2];
	mt_number_t* size1  = vh->fields->data[ind1 * 2 + 1];
	char*        field2 = vh->fields->data[ind2 * 2];
	mt_number_t* size2  = vh->fields->data[ind2 * 2 + 1];

	vh->fields->data[ind1 * 2]     = field2;
	vh->fields->data[ind1 * 2 + 1] = size2;

	vh->fields->data[ind2 * 2]     = field1;
	vh->fields->data[ind2 * 2 + 1] = size1;

	for (int ri = 0; ri < vh->body_v->views->length; ri++)
	{
	    ku_view_t* rowview = vh->body_v->views->data[ri];

	    ku_view_t* cell1 = RET(rowview->views->data[ind1]);
	    ku_view_t* cell2 = RET(rowview->views->data[ind2]);

	    ku_view_remove_subview(rowview, cell1);
	    ku_view_insert_subview(rowview, cell1, ind2);
	    ku_view_remove_subview(rowview, cell2);
	    ku_view_insert_subview(rowview, cell2, ind1);

	    REL(cell1);
	    REL(cell2);
	}

	vh_table_head_update_cells(vh, -1, 0);

	vh_table_event_t event = {.table = vh, .id = VH_TABLE_EVENT_FIELDS_UPDATE, .fields = vh->fields, .view = vh->view};
	(*vh->on_event)(event);
    }
}

/* ceeate header */

ku_view_t* vh_table_head_create(
    ku_view_t* head_v,
    void*      userdata)
{
    vh_table_t* vh = (vh_table_t*) userdata;

    char*      headid   = mt_string_new_format(100, "%s_header", vh->id); // REL 0
    ku_view_t* headview = ku_view_new(headid, (ku_rect_t){0, 0, 100, vh->headstyle.line_height});

    REL(headid);

    int wth = 0;

    /* create header fields/cells */

    for (int i = 0; i < vh->fields->length; i += 2)
    {
	char*        field    = vh->fields->data[i];
	mt_number_t* size     = vh->fields->data[i + 1];
	char*        cellid   = mt_string_new_format(100, "%s_cell_%s", headview->id, field);                    // REL 2
	ku_view_t*   cellview = ku_view_new(cellid, (ku_rect_t){wth, 0, size->intv, vh->headstyle.line_height}); // REL 3

	wth += size->intv + 2;

	tg_text_add(cellview);
	tg_text_set(cellview, field, vh->headstyle);

	ku_view_add_subview(headview, cellview);

	REL(cellid);
	REL(cellview);
    }

    ku_view_set_frame(headview, (ku_rect_t){0, 0, wth, vh->headstyle.line_height});

    return headview;
}

/* create item */

ku_view_t* vh_table_item_create(
    ku_view_t* table_v,
    int        index,
    void*      userdata)
{
    vh_table_t* vh = (vh_table_t*) userdata;

    ku_view_t* rowview = NULL;

    if (vh->items)
    {
	if (index > -1 && index < vh->items->length)
	{
	    mt_map_t* data = vh->items->data[index];

	    if (vh->cache->length > 0)
	    {
		/* get cached item */
		rowview = RET(vh->cache->data[0]);
		mt_vector_rem_at_index(vh->cache, 0);
	    }
	    else
	    {
		/* create new item */
		char* rowid = mt_string_new_format(100, "%s_rowitem_%i", vh->id, vh->cnt++);
		rowview     = ku_view_new(rowid, (ku_rect_t){0, 0, table_v->frame.local.w, vh->rowastyle.line_height});
		REL(rowid);

		/* create cells */
		int wth = 0;

		for (int i = 0; i < vh->fields->length; i += 2)
		{
		    char*        field    = vh->fields->data[i];
		    mt_number_t* size     = vh->fields->data[i + 1];
		    char*        cellid   = mt_string_new_format(100, "%s_cell_%s", rowview->id, field);                     // REL 2
		    ku_view_t*   cellview = ku_view_new(cellid, (ku_rect_t){wth, 0, size->intv, vh->rowastyle.line_height}); // REL 3

		    wth += size->intv + 2;

		    tg_text_add(cellview);

		    ku_view_add_subview(rowview, cellview);

		    REL(cellid);
		    REL(cellview);
		}
	    }

	    /* select style */
	    textstyle_t style = index % 2 == 0 ? vh->rowastyle : vh->rowbstyle;

	    if (vh->selected_items->length > 0)
	    {
		uint32_t pos = mt_vector_index_of_data(vh->selected_items, data);
		if (pos < UINT32_MAX) style = vh->rowsstyle;
	    }

	    /* update cells */
	    int wth = 0;

	    for (int i = 0; i < vh->fields->length; i += 2)
	    {
		char*        field    = vh->fields->data[i];
		mt_number_t* size     = vh->fields->data[i + 1];
		char*        value    = MGET(data, field);
		ku_view_t*   cellview = rowview->views->data[i / 2];
		ku_rect_t    frame    = cellview->frame.local;

		frame.x = wth;
		frame.w = size->intv;
		ku_view_set_frame(cellview, frame);

		wth += size->intv + 2;

		if (value) tg_text_set(cellview, value, style);
		else tg_text_set(cellview, "", style); // reset old value
	    }

	    ku_view_set_frame(rowview, (ku_rect_t){0, 0, wth, style.line_height});
	}
    }

    return rowview;
}

/* add item to cache */

void vh_table_item_recycle(
    ku_view_t* table_v,
    ku_view_t* item_v,
    void*      userdata)
{
    vh_table_t* vh = (vh_table_t*) userdata;
    VADD(vh->cache, item_v);
}

/* table event */

void vh_table_evnt_event(vh_tbl_evnt_event_t event)
{
    vh_table_t* vh = (vh_table_t*) event.userdata;

    if (event.id == VH_TBL_EVENT_SELECT)
    {
	vh->selected_index = event.index;

	mt_map_t* data = vh->items->data[event.index];

	uint32_t pos = mt_vector_index_of_data(vh->selected_items, data);

	if (pos == UINT32_MAX)
	{
	    /* reset selected if control is not down */

	    if (!event.ev.ctrl_down)
	    {
		mt_vector_reset(vh->selected_items);
		vh_tbl_body_t* bvh = vh->body_v->handler_data;

		for (int index = 0; index < bvh->items->length; index++)
		{
		    ku_view_t* item = bvh->items->data[index];

		    textstyle_t style = index % 2 == 0 ? vh->rowastyle : vh->rowbstyle;

		    for (int i = 0; i < item->views->length; i++)
		    {
			ku_view_t* cellview = item->views->data[i];
			tg_text_set_style(cellview, style);
		    }
		}
	    }

	    VADD(vh->selected_items, data);

	    for (int i = 0; i < event.rowview->views->length; i++)
	    {
		ku_view_t* cellview = event.rowview->views->data[i];
		tg_text_set_style(cellview, vh->rowsstyle);
	    }
	}
	else
	{
	    VREM(vh->selected_items, data);

	    textstyle_t style = event.index % 2 == 0 ? vh->rowastyle : vh->rowbstyle;

	    for (int i = 0; i < event.rowview->views->length; i++)
	    {
		ku_view_t* cellview = event.rowview->views->data[i];
		tg_text_set_style(cellview, style);
	    }
	}

	vh_table_event_t tevent = {
	    .table          = vh,
	    .id             = VH_TABLE_EVENT_SELECT,
	    .selected_items = vh->selected_items,
	    .selected_index = event.index,
	    .rowview        = event.rowview,
	    .ev             = event.ev,
	    .view           = vh->view};

	(*vh->on_event)(tevent);
    }
    else if (event.id == VH_TBL_EVENT_CONTEXT)
    {
	vh->selected_index = event.index;

	mt_map_t* data = vh->items->data[event.index];

	uint32_t pos = mt_vector_index_of_data(vh->selected_items, data);

	if (pos == UINT32_MAX)
	{
	    /* reset selected if control is not down */
	    if (!event.ev.ctrl_down)
	    {
		mt_vector_reset(vh->selected_items);
		vh_tbl_body_t* bvh = vh->body_v->handler_data;

		for (int index = 0; index < bvh->items->length; index++)
		{
		    ku_view_t* item = bvh->items->data[index];
		    if (item->style.background_color == 0x006600FF)
		    {
			item->style.background_color = 0x000000FF;
			ku_view_invalidate_texture(item);
		    }
		}
	    }

	    VADD(vh->selected_items, data);

	    if (event.rowview)
	    {
		event.rowview->style.background_color = 0x006600FF;
		ku_view_invalidate_texture(event.rowview);
	    }
	}

	vh_table_event_t tevent = {
	    .table          = vh,
	    .id             = VH_TABLE_EVENT_CONTEXT,
	    .selected_items = vh->selected_items,
	    .selected_index = event.index,
	    .rowview        = event.rowview,
	    .view           = vh->view,
	    .ev             = event.ev};
	(*vh->on_event)(tevent);
    }
    else if (event.id == VH_TBL_EVENT_OPEN)
    {
	vh_table_event_t tevent = {
	    .table          = vh,
	    .id             = VH_TABLE_EVENT_OPEN,
	    .selected_items = vh->selected_items,
	    .selected_index = event.index,
	    .view           = vh->view,
	    .rowview        = event.rowview};
	(*vh->on_event)(tevent);
    }
    else if (event.id == VH_TBL_EVENT_DRAG)
    {
	vh_table_event_t tevent = {
	    .table          = vh,
	    .id             = VH_TABLE_EVENT_DRAG,
	    .selected_items = vh->selected_items,
	    .selected_index = event.index,
	    .view           = vh->view,
	    .rowview        = event.rowview};
	(*vh->on_event)(tevent);
    }
    else if (event.id == VH_TBL_EVENT_DROP)
    {
	vh_table_event_t tevent = {
	    .table          = vh,
	    .id             = VH_TABLE_EVENT_DROP,
	    .selected_items = vh->selected_items,
	    .selected_index = event.index,
	    .view           = vh->view,
	    .rowview        = event.rowview};
	(*vh->on_event)(tevent);
    }
    else if (event.id == VH_TBL_EVENT_KEY_DOWN)
    {
	vh_table_event_t tevent = {
	    .table          = vh,
	    .id             = VH_TABLE_EVENT_KEY_DOWN,
	    .selected_items = vh->selected_items,
	    .selected_index = vh->selected_index,
	    .view           = vh->view,
	    .rowview        = event.rowview,
	    .ev             = event.ev};
	(*vh->on_event)(tevent);

	if (event.ev.keycode == XKB_KEY_Down || event.ev.keycode == XKB_KEY_Up)
	{
	    if (event.ev.keycode == XKB_KEY_Down) vh_table_select(vh->view, vh->selected_index + 1, event.ev.shift_down);
	    if (event.ev.keycode == XKB_KEY_Up) vh_table_select(vh->view, vh->selected_index - 1, event.ev.shift_down);

	    tevent = (vh_table_event_t){
		.table          = vh,
		.id             = VH_TABLE_EVENT_SELECT,
		.ev             = event.ev,
		.selected_items = vh->selected_items,
		.selected_index = vh->selected_index,
		.view           = vh->view,
		.rowview        = vh_tbl_body_item_for_index(vh->body_v, vh->selected_index)};

	    (*vh->on_event)(tevent);
	}

	if (event.ev.keycode == XKB_KEY_Return)
	{
	    tevent = (vh_table_event_t){
		.table          = vh,
		.id             = VH_TABLE_EVENT_OPEN,
		.ev             = event.ev,
		.selected_items = vh->selected_items,
		.selected_index = vh->selected_index,
		.view           = vh->view,
		.rowview        = vh_tbl_body_item_for_index(vh->body_v, vh->selected_index),
	    };
	    (*vh->on_event)(tevent);
	}
    }
    else if (event.id == VH_TBL_EVENT_KEY_UP)
    {
	vh_table_event_t tevent = (vh_table_event_t){
	    .table          = vh,
	    .id             = VH_TABLE_EVENT_KEY_UP,
	    .selected_items = vh->selected_items,
	    .selected_index = vh->selected_index,
	    .view           = vh->view,
	    .rowview        = event.rowview,
	    .ev             = event.ev};
	(*vh->on_event)(tevent);
    }
}

void vh_table_del(
    void* p)
{
    vh_table_t* vh = p;

    REL(vh->id);             // REL S0
    REL(vh->cache);          // REL S1
    REL(vh->fields);         // REL S2
    REL(vh->selected_items); // REL S3

    if (vh->items) REL(vh->items);

    if (vh->body_v) REL(vh->body_v);
    if (vh->evnt_v) REL(vh->evnt_v);
    if (vh->scrl_v) REL(vh->scrl_v);
    if (vh->layr_v) REL(vh->layr_v);
    if (vh->head_v) REL(vh->head_v);
}

void vh_table_desc(
    void* p,
    int   level)
{
    printf("vh_table");
}

void vh_table_attach(
    ku_view_t*   view,
    mt_vector_t* fields,
    void (*on_event)(vh_table_event_t event))
{
    assert(view != NULL);

    /* Create inner structure first from scrollbars and items*/

    /* <div id="songtable" class="fullscale colflex"> */
    /*   <div id="songtablevertscroll" class="vertscroll"/> */
    /*   <div id="songtablehoriscroll" class="horiscroll"/> */
    /*   <div id="songtable_row_a" class="rowa rowtext" type="label" text="row a"/> */
    /*   <div id="songtable_row_b" class="rowb rowtext" type="label" text="row b"/> */
    /*   <div id="songtable_row_selected" class="rowselected rowtext" type="label" text="row selected"/> */
    /*   <div id="songtableheadrow" class="head rowtext"/> */
    /* </div> */

    /* This will be the reasulting structure */

    /* <div id="songtable" class="fullscale colflex"> */
    /*    <div id="songtablehead" class="tablehead"> */
    /* 	<div id="songtableheadrow" class="head rowtext"/> */
    /*   </div> */
    /*   <div id="songtablelayers" class="tableback fullscale"> */
    /* 	<div id="songtablebody" class="fullscale"> */
    /* 	  <div id="songtable_row_a" class="rowa rowtext" type="label" text="row a"/> */
    /* 	  <div id="songtable_row_b" class="rowb rowtext" type="label" text="row b"/> */
    /* 	  <div id="songtable_row_selected" class="rowselected rowtext" type="label" text="row selected"/> */
    /* 	</div> */
    /* 	<div id="songtablescroll" class="fullscale"> */
    /* 	  <div id="songtablevertscroll" class="vertscroll"/> */
    /* 	  <div id="songtablehoriscroll" class="horiscroll"/> */
    /* 	</div> */
    /* 	<div id="songtableevt" class="fullscale"/> */
    /*   </div> */
    /* </div> */

    if (view->views->length >= 5)
    {
	/* grab styled views prepared for us */
	ku_view_t* vscroll = view->views->data[0];
	ku_view_t* hscroll = view->views->data[1];
	ku_view_t* rowa    = view->views->data[2];
	ku_view_t* rowb    = view->views->data[3];
	ku_view_t* rows    = view->views->data[4];
	ku_view_t* headrow = NULL;
	if (view->views->length == 6) headrow = view->views->data[5];

	/* extract needed styles */
	textstyle_t rowastyle = ku_gen_textstyle_parse(rowa);
	textstyle_t rowbstyle = ku_gen_textstyle_parse(rowb);
	textstyle_t rowsstyle = ku_gen_textstyle_parse(rows);
	textstyle_t headstyle = headrow == NULL ? (textstyle_t){0} : ku_gen_textstyle_parse(headrow);

	/* generate ids */
	char* headid = STRNF(100, "%s_head", view->id);
	char* layrid = STRNF(100, "%s_layers", view->id);
	char* bodyid = STRNF(100, "%s_body", view->id);
	char* scrlid = STRNF(100, "%s_scroll", view->id);
	char* evntid = STRNF(100, "%s_event", view->id);

	/* generate viewas */
	ku_rect_t rect    = view->frame.local;
	rect.x            = 0;
	rect.y            = 0;
	ku_view_t* layr_v = ku_view_new(layrid, rect);
	ku_view_t* body_v = ku_view_new(bodyid, rect);
	ku_view_t* scrl_v = ku_view_new(scrlid, rect);
	ku_view_t* evnt_v = ku_view_new(evntid, rect);
	ku_view_t* head_v = NULL;
	if (view->views->length == 6) head_v = ku_view_new(headid, headrow->frame.local);

	REL(headid);
	REL(layrid);
	REL(bodyid);
	REL(scrlid);
	REL(evntid);

	/* set fullscale */
	layr_v->style.w_per = 1.0;
	layr_v->style.h_per = 1.0;
	body_v->style.w_per = 1.0;
	body_v->style.h_per = 1.0;
	scrl_v->style.w_per = 1.0;
	scrl_v->style.h_per = 1.0;
	evnt_v->style.w_per = 1.0;
	evnt_v->style.h_per = 1.0;
	if (head_v)
	{
	    head_v->style.w_per  = 1.0;
	    head_v->style.height = headrow->frame.local.h;
	}

	/* set mask */
	body_v->style.masked = 1;
	if (head_v) head_v->style.masked = 1;

	/* attach header view as first view */
	if (head_v) ku_view_add_subview(view, head_v);

	/* attach layers view as first or second view */
	ku_view_add_subview(view, layr_v);
	ku_view_add_subview(layr_v, body_v);
	ku_view_add_subview(layr_v, scrl_v);
	ku_view_add_subview(layr_v, evnt_v);

	/* move scrollbars to scroll container */
	RET(vscroll);
	RET(hscroll);
	ku_view_remove_from_parent(vscroll);
	ku_view_remove_from_parent(hscroll);
	ku_view_add_subview(scrl_v, vscroll);
	ku_view_add_subview(scrl_v, hscroll);
	REL(vscroll);
	REL(hscroll);

	/* remove style views */
	ku_view_remove_from_parent(rowa);
	ku_view_remove_from_parent(rowb);
	ku_view_remove_from_parent(rows);
	if (headrow) ku_view_remove_from_parent(headrow);

	vh_table_t* vh     = CAL(sizeof(vh_table_t), vh_table_del, vh_table_desc);
	vh->id             = mt_string_new_cstring(view->id);
	vh->cache          = VNEW();
	vh->fields         = RET(fields);
	vh->selected_items = VNEW();
	vh->on_event       = on_event;
	vh->view           = view;

	view->handler_data = vh;

	vh->layr_v = layr_v;
	vh->body_v = body_v;
	vh->evnt_v = evnt_v;
	vh->scrl_v = scrl_v;
	vh->head_v = head_v;

	if (rowastyle.line_height == 0) rowastyle.line_height = 20;
	if (rowbstyle.line_height == 0) rowbstyle.line_height = 20;
	if (rowsstyle.line_height == 0) rowsstyle.line_height = 20;
	if (headstyle.line_height == 0) headstyle.line_height = 20;

	vh->rowastyle = rowastyle;
	vh->rowbstyle = rowbstyle;
	vh->rowsstyle = rowsstyle;
	vh->headstyle = headstyle;

	vh_tbl_body_attach(
	    body_v,
	    vh_table_item_create,
	    vh_table_item_recycle,
	    vh);

	vh_tbl_scrl_attach(
	    scrl_v,
	    body_v,
	    head_v,
	    vh);

	vh_tbl_evnt_attach(
	    evnt_v,
	    body_v,
	    scrl_v,
	    head_v,
	    vh_table_evnt_event,
	    vh);

	if (head_v)
	{
	    vh_tbl_head_attach(
		head_v,
		vh_table_head_create,
		vh_table_head_move,
		vh_table_head_resize,
		vh_table_head_reorder,
		vh);
	}
    }
    else mt_log_error("Not enough subviews for creating a table ( scrollv,scrollh,rowa,rowb,rowselected needed and rowhead is optional");
}

/* data items have to be maps containing the same keys */

void vh_table_set_data(
    ku_view_t*   view,
    mt_vector_t* data)
{
    vh_table_t* vh = view->handler_data;

    if (vh->items) REL(vh->items);
    vh->items = RET(data);

    vh->selected_index = 0;

    mt_vector_reset(vh->selected_items);
    if (vh->selected_index < vh->items->length)
    {
	mt_map_t* sel = vh->items->data[vh->selected_index];
	VADD(vh->selected_items, sel);
    }

    vh_tbl_body_reset(vh->body_v);
    vh_tbl_body_move(vh->body_v, 0, 0);

    if (vh->scrl_v) vh_tbl_scrl_set_item_count(vh->scrl_v, data->length);
}

/* select index */

void vh_table_select(
    ku_view_t* view,
    int32_t    index,
    int        add)
{
    vh_table_t*    vh  = view->handler_data;
    vh_tbl_body_t* bvh = vh->body_v->handler_data;

    vh->selected_index = index;
    if (vh->selected_index < 0) vh->selected_index = 0;
    if (vh->selected_index > vh->items->length - 1) vh->selected_index = vh->items->length - 1;

    if (bvh->bot_index < vh->selected_index)
    {
	vh_tbl_body_vjump(vh->body_v, vh->selected_index + 1, 0);

	if (bvh->tail_index == bvh->bot_index)
	{
	    /* check if bottom item is out of bounds */
	    ku_view_t* lastitem = mt_vector_tail(bvh->items);
	    ku_rect_t  iframe   = lastitem->frame.local;
	    ku_rect_t  vframe   = vh->body_v->frame.local;

	    if (iframe.y + iframe.h > vframe.h)
	    {
		vh_tbl_body_move(vh->body_v, 0, iframe.y + iframe.h - vframe.h - 20.0);
	    }
	}
    }

    if (vh->selected_index <= bvh->top_index)
    {
	vh_tbl_body_vjump(vh->body_v, vh->selected_index - 1, 1);
    }

    if (add == 0) mt_vector_reset(vh->selected_items);
    mt_map_t* sel = vh->items->data[vh->selected_index];
    VADD(vh->selected_items, sel);

    /* color item */

    for (int i = 0; i < bvh->items->length; i++)
    {
	int        realindex = bvh->head_index + i;
	mt_map_t*  data      = vh->items->data[realindex];
	ku_view_t* item      = bvh->items->data[i];

	textstyle_t style = realindex % 2 == 0 ? vh->rowastyle : vh->rowbstyle;
	if (mt_vector_index_of_data(vh->selected_items, data) < UINT32_MAX) style = vh->rowsstyle;

	for (int i = 0; i < item->views->length; i++)
	{
	    ku_view_t* cellview = item->views->data[i];
	    tg_text_set_style(cellview, style);
	}
    }

    if (vh->scrl_v) vh_tbl_scrl_update(vh->scrl_v);
}

mt_vector_t* vh_table_get_fields(ku_view_t* view)
{
    vh_table_t* vh = view->handler_data;
    return vh->fields;
}

#endif
