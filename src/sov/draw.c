#include "gen.c"
#include "ku_renderer_soft.c"
#include "ku_text.c"
#include "mt_bitmap_ext.c"
#include "mt_log.c"
#include "mt_path.c"
#include "mt_string.c"
#include "mt_string_ext.c"
#include "mt_time.c"
#include "tree.c"
#include <dirent.h>
#include <errno.h>
#include <getopt.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main(int argc, char** argv)
{
    mt_log_use_colors(isatty(STDERR_FILENO));
    mt_log_level_info();
    mt_time(NULL);

    /* parse options */

    char* cfg_par  = NULL;
    char* tree_par = NULL;
    char* ws_par   = NULL;
    char* res_par  = NULL;

    int c, option_index = 0;

    static struct option long_options[] = {
	{"tree", required_argument, NULL, 't'},
	{"workspace", required_argument, NULL, 'w'},
	{"style", required_argument, NULL, 's'},
	{"result", required_argument, NULL, 'r'},
    };

    while ((c = getopt_long(argc, argv, "s:t:w:r:", long_options, &option_index)) != -1)
    {
	switch (c)
	{
	    case 's': cfg_par = mt_string_new_cstring(optarg); break;
	    case 't': tree_par = mt_string_new_cstring(optarg); break;
	    case 'w': ws_par = mt_string_new_cstring(optarg); break;
	    case 'r': res_par = mt_string_new_cstring(optarg); break;
	}
    }

    /* init config */

    char cwd[PATH_MAX];
    if (getcwd(cwd, sizeof(cwd)) == NULL) printf("Cannot get working directory\n");

    char* cfg_path  = mt_path_new_normalize(cfg_par, cwd);
    char* css_path  = mt_path_new_append(cfg_path, "html/main.css");  // REL 6
    char* html_path = mt_path_new_append(cfg_path, "html/main.html"); // REL 7
    char* img_path  = mt_path_new_append(cfg_path, "img");            // REL 6

    printf("style path    : %s\n", cfg_path);
    printf("css path      : %s\n", css_path);
    printf("html path     : %s\n", html_path);
    printf("image path    : %s\n", img_path);
    printf("tree path     : %s\n", tree_par);
    printf("ws path       : %s\n", ws_par);
    printf("res path      : %s\n", res_par);

    /* init text rendering */

    ku_text_init(); // DESTROY 1

    /* read sway tree */

    char* tree_json = mt_string_new_file(tree_par);
    char* ws_part   = mt_string_new_file(ws_par);
    char* ws_json   = mt_string_new_cstring("{\"items\":");
    ws_json         = mt_string_append(ws_json, ws_part);
    ws_json         = mt_string_append(ws_json, "}");

    mt_vector_t* workspaces = VNEW();

    tree_reader_extract(ws_json, tree_json, workspaces);

    printf("WORKSPACES LENGTH %i\n", workspaces->length);

    gen_init(html_path, css_path, img_path);

    int cols = 5;
    int rows = (int) ceilf((float) workspaces->length / cols);

    ku_bitmap_t* bitmap = ku_bitmap_new(1000, 500);

    gen_render(
	100,
	50,
	cols,
	rows,
	workspaces,
	bitmap);

    char* tgt_path = mt_path_new_append(res_par, "render0.bmp");

    bm_write(bitmap, tgt_path);

    /* cleanup */

    if (cfg_par) REL(cfg_par);
    REL(cfg_path);
    REL(css_path);
    REL(html_path);
    REL(img_path);

    ku_text_destroy();

#ifdef MT_MEMORY_DEBUG
    mt_memory_stats();
#endif
}
