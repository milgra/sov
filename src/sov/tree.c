#ifndef tree_reader_h
#define tree_reader_h

#include "mt_vector.c"

typedef struct _sway_window_t
{
    int   x;
    int   y;
    int   width;
    int   height;
    char* title;
    char* appid;
} sway_window_t;

typedef struct _sway_workspace_t
{
    int          x;
    int          y;
    int          width;
    int          height;
    char*        name;
    int          number;
    int          focused;
    char*        output;
    mt_vector_t* windows;
} sway_workspace_t;

void tree_reader_extract(char* ws_json, char* tree_json, mt_vector_t* workspaces);

#endif

#if __INCLUDE_LEVEL__ == 0

#include "json.c"
#include "mt_log.c"
#include "mt_path.c"
#include "mt_string.c"

void sway_workspace_del(void* p)
{
    sway_workspace_t* ws = p;
    REL(ws->windows);
    if (ws->output) REL(ws->output);
}

void sway_workspace_desc(void* p, int level)
{
    sway_workspace_t* ws = p;
    printf("workspace name %s num %i w %i h %i focus %i output %s\n", ws->name, ws->number, ws->width, ws->height, ws->focused, ws->output);
    mt_memory_describe(ws->windows, level);
}

sway_workspace_t* sway_workspace_new()
{
    sway_workspace_t* ws = CAL(sizeof(sway_workspace_t), sway_workspace_del, sway_workspace_desc);
    ws->windows          = VNEW();
    return ws;
}

void sway_window_del(void* p)
{
    sway_window_t* wi = p;
    REL(wi->appid);
    REL(wi->title);
}

void sway_window_desc(void* p, int level)
{
    sway_window_t* wi = p;
    printf("window x %i y %i w %i h %i title %s", wi->x, wi->y, wi->width, wi->height, wi->title);
}

sway_window_t* sway_window_new()
{
    sway_window_t* wi = CAL(sizeof(sway_window_t), sway_window_del, sway_window_desc);

    return wi;
}

char* sway_get_value(mt_vector_t* vec, int pos, char* path)
{
    for (int index = pos; index < vec->length; index++)
    {
	if (strcmp(vec->data[index], path) == 0) return vec->data[index + 1];
    }
    for (int index = pos; index > -1; index--)
    {
	if (strcmp(vec->data[index], path) == 0) return vec->data[index + 1];
    }
    return NULL;
}

void tree_reader_extract(char* ws_json, char* tree_json, mt_vector_t* workspaces)
{
    mt_vector_t* json = VNEW(); // REL 0
    json_parse(ws_json, json);

    // find workspaces

    for (int index = 0; index < json->length; index += 2)
    {
	char* key      = json->data[index];
	char* type_prt = strstr(key, "/type");

	if (type_prt != NULL && strcmp(json->data[index + 1], "workspace") == 0)
	{
	    sway_workspace_t* ws = sway_workspace_new(); // REL 1

	    char* path    = mt_path_new_remove_last_component(key); // REL 2
	    int   pathlen = strlen(path);

	    char* wx  = mt_string_new_format(pathlen + 20, "%srect/x", path);      // REL 3
	    char* wy  = mt_string_new_format(pathlen + 20, "%srect/y", path);      // REL 4
	    char* wk  = mt_string_new_format(pathlen + 20, "%srect/width", path);  // REL 5
	    char* hk  = mt_string_new_format(pathlen + 20, "%srect/height", path); // REL 6
	    char* ok  = mt_string_new_format(pathlen + 20, "%soutput", path);      // REL 7
	    char* nk  = mt_string_new_format(pathlen + 20, "%snum", path);         // REL 8
	    char* fk  = mt_string_new_format(pathlen + 20, "%sfocused", path);     // REL 9
	    char* nmk = mt_string_new_format(pathlen + 20, "%sname", path);        // REL 7

	    char* x  = sway_get_value(json, index, wx);
	    char* y  = sway_get_value(json, index, wy);
	    char* w  = sway_get_value(json, index, wk);
	    char* h  = sway_get_value(json, index, hk);
	    char* o  = sway_get_value(json, index, ok);
	    char* n  = sway_get_value(json, index, nk);
	    char* f  = sway_get_value(json, index, fk);
	    char* nm = sway_get_value(json, index, nmk);

	    if (x) ws->x = atoi(x);
	    if (y) ws->y = atoi(y);
	    if (w) ws->width = atoi(w);
	    if (h) ws->height = atoi(h);
	    if (f) ws->focused = strcmp(f, "true") == 0;
	    if (n) ws->number = atoi(n);
	    if (o) ws->output = mt_string_new_cstring(o);
	    if (nm) ws->name = mt_string_new_cstring(nm);

	    REL(path); // REL 2
	    REL(wx);   // REL 3
	    REL(wy);   // REl 4
	    REL(wk);   // REL 5
	    REL(hk);   // REL 6
	    REL(ok);   // REL 7
	    REL(nk);   // REL 8
	    REL(fk);   // REL 9
	    REL(nmk);  // REL 10

	    VADDR(workspaces, ws); // REL 1

	    mt_log_debug("Found workspace, name: %s num : %i output : %s", ws->name, ws->number, ws->output);
	}
    }

    REL(json); // REL 0
    json = VNEW();

    json_parse(tree_json, json); // REL 0

    // find windows

    int curr_wspc_n = 0;

    for (int index = 0; index < json->length; index += 2)
    {
	char* key      = json->data[index];
	char* type_prt = strstr(key, "type");

	if (type_prt != NULL)
	{
	    if (strcmp(json->data[index + 1], "workspace") == 0)
	    {
		char* path    = mt_path_new_remove_last_component(key); // REL 1
		int   pathlen = strlen(path);
		char* pathkey = mt_string_new_format(pathlen + 20, "%snum", path); // REL 2

		char* n = sway_get_value(json, index, pathkey);

		if (n) { curr_wspc_n = atoi(n); }

		REL(path);    // REL 1
		REL(pathkey); // REL 2
	    }
	}

	char* wind_prt = strstr(key, "app_id");

	if (wind_prt != NULL && curr_wspc_n > -1)
	{
	    sway_window_t* wi = sway_window_new(); // REL 3c

	    char* path    = mt_path_new_remove_last_component(key); // REL 4
	    int   pathlen = strlen(path);

	    char* tk = mt_string_new_format(pathlen + 20, "%sname", path);        // REL 6
	    char* ck = mt_string_new_format(pathlen + 20, "%sapp_id", path);      // REL 7
	    char* xk = mt_string_new_format(pathlen + 20, "%srect/x", path);      // REL 8
	    char* yk = mt_string_new_format(pathlen + 20, "%srect/y", path);      // REL 9
	    char* wk = mt_string_new_format(pathlen + 20, "%srect/width", path);  // REL 10
	    char* hk = mt_string_new_format(pathlen + 20, "%srect/height", path); // REL 11

	    char* c = sway_get_value(json, index, ck);
	    // char* i = json->data[index + 1];
	    char* t = sway_get_value(json, index, tk);

	    char* rx = sway_get_value(json, index, xk);
	    char* ry = sway_get_value(json, index, yk);
	    char* rw = sway_get_value(json, index, wk);
	    char* rh = sway_get_value(json, index, hk);

	    if (rx) wi->x = atoi(rx);
	    if (ry) wi->y = atoi(ry);
	    if (rw) wi->width = atoi(rw);
	    if (rh) wi->height = atoi(rh);
	    if (c) wi->appid = mt_string_new_cstring(c);
	    if (t) wi->title = mt_string_new_cstring(t);

	    mt_log_debug("Found window, appid %s title %s %i %i %i %i", wi->appid, wi->title, wi->x, wi->y, wi->width, wi->height);

	    for (int wsi = 0; wsi < workspaces->length; wsi++)
	    {
		sway_workspace_t* ws = workspaces->data[wsi];

		if (ws->number == curr_wspc_n)
		{
		    wi->x -= ws->x;
		    wi->y -= ws->y;
		    VADD(ws->windows, wi);
		}
	    }

	    REL(wi);   // REL 3
	    REL(path); // REL 4
	    REL(tk);   // REL 6
	    REL(ck);   // REL 7
	    REL(xk);   // REL 8
	    REL(yk);   // REL 9
	    REL(wk);   // REL 10
	    REL(hk);   // REL 11
	}
    }

    REL(json); // REL 0
}

#endif
