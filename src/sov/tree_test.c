#include "bm_rgba_util.c"
#include "config.c"
#include "cstr_util.c"
#include "fontconfig.c"
#include "kvlines.c"
#include "tree_drawer.c"
#include "tree_reader.c"
#include "zc_bm_rgba.c"
#include "zc_cstring.c"
#include "zc_draw.c"
#include "zc_log.c"
#include "zc_path.c"
#include "zc_text.c"
#include <getopt.h>
#include <limits.h>
#ifdef __linux__
    #include <linux/limits.h>
#endif
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

int main(int argc, char* argv[])
{
    zc_log_use_colors(isatty(STDERR_FILENO));
    zc_log_level_debug();

    char* cfg_path       = NULL;
    char* ws_json_path   = NULL;
    char* tree_json_path = NULL;
    char* output_path    = NULL;

    char cwd[PATH_MAX];
    if (getcwd(cwd, sizeof(cwd)) == NULL) printf("Cannot get working directory\n");

    const struct option long_options[] = {
	{"config", required_argument, 0, 'c'},
	{"workspaces", required_argument, 0, 'w'},
	{"tree", required_argument, 0, 't'},
	{"output", required_argument, 0, 'o'},
	{0, 0, 0, 0},
    };

    int option       = 0;
    int option_index = 0;

    while ((option = getopt_long(argc, argv, "c:w:t:o:", long_options, &option_index)) != -1)
    {
	if (option == 'c') cfg_path = path_new_normalize(optarg, cwd);       // REL 0
	if (option == 'w') ws_json_path = path_new_normalize(optarg, cwd);   // REL 0
	if (option == 't') tree_json_path = path_new_normalize(optarg, cwd); // REL 0
	if (option == 'o') output_path = path_new_normalize(optarg, cwd);    // REL 0
	if (option == '?') printf("-c --config= [path] \t use config file for session\n");
    }

    printf("*****************\n");
    printf("cfg_path %s\n", cfg_path);
    printf("ws_json_path %s\n", ws_json_path);
    printf("tree_json_path %s\n", tree_json_path);
    printf("output_path %s\n", output_path);

    /* init config */

    config_init();
    config_read(cfg_path);
    config_describe();

    /* init text rendeing */

    text_init(); // DESTROY 1

    char* font_face = config_get("font_face");
    char* font_path = fontconfig_new_path(font_face ? font_face : ""); // REL 4

    char*  ws_json     = cstr_new_file(ws_json_path);
    char*  tree_json   = cstr_new_file(tree_json_path);
    vec_t* workspaces  = VNEW(); // REL 6
    vec_t* outputs     = VNEW();
    char*  curr_output = NULL;

    tree_reader_extract(ws_json, tree_json, workspaces);

    // extract outputs

    for (int index = 0; index < workspaces->length; index++)
    {
	sway_workspace_t* ws = workspaces->data[index];

	if (!curr_output || strcmp(ws->output, curr_output) != 0)
	{
	    curr_output = ws->output;
	    VADD(outputs, ws->output);
	}
    }

    for (int oi = 0; oi < outputs->length; oi++)
    {
	char*  curr_output = outputs->data[oi];
	vec_t* curr_ws     = VNEW(); // REL 1

	for (int index = 0; index < workspaces->length; index++)
	{
	    sway_workspace_t* ws = workspaces->data[index];
	    if (strcmp(ws->output, curr_output) == 0) VADD(curr_ws, ws);
	}

	textstyle_t main_style = {
	    .margin     = config_get_int("text_margin_size"),
	    .margin_top = config_get_int("text_margin_top_size"),
	    .align      = TA_LEFT,
	    .valign     = VA_TOP,
	    .size       = config_get_int("text_title_size"),
	    .textcolor  = cstr_color_from_cstring(config_get("text_title_color")),
	    .backcolor  = 0,
	    .multiline  = 0,
	};

	strcpy(main_style.font, font_path);

	textstyle_t sub_style = {
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

	strcpy(sub_style.font, font_path);

	textstyle_t wsnum_style = {
	    .margin    = config_get_int("text_margin_size"),
	    .align     = TA_RIGHT,
	    .valign    = VA_TOP,
	    .size      = config_get_int("text_workspace_size"),
	    .textcolor = cstr_color_from_cstring(config_get("text_workspace_color")),
	    .backcolor = 0x00002200,
	};

	strcpy(wsnum_style.font, font_path);

	int gap   = config_get_int("gap");
	int cols  = config_get_int("columns");
	int ratio = config_get_int("ratio");

	bm_rgba_t* bitmap = tree_drawer_bm_create(
	    curr_ws,
	    gap,
	    cols,
	    ratio,
	    main_style,
	    sub_style,
	    wsnum_style,
	    cstr_color_from_cstring(config_get("window_color")),
	    cstr_color_from_cstring(config_get("background_color")),
	    cstr_color_from_cstring(config_get("background_color_focused")),
	    cstr_color_from_cstring(config_get("border_color")),
	    cstr_color_from_cstring(config_get("empty_color")),
	    cstr_color_from_cstring(config_get("empty_frame_color")),
	    config_get_int("window_border_radius"),
	    config_get_int("window_border_size"),
	    config_get_int("window_border_color"),
	    config_get_int("workspace_border_radius"),
	    config_get_int("workspace_border_size"),
	    config_get_int("text_workspace_xshift"),
	    config_get_int("text_workspace_yshift"));

	char* filename = cstr_new_format(strlen(output_path) + 10, "%s_o%i.bmp", output_path, oi); // REL 9
	bm_rgba_write(bitmap, filename);
	REL(filename);
	REL(bitmap);
    }

    REL(workspaces); // REL 6
}
