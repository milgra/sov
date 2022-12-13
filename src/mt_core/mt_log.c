#ifndef _mt_log_h
#define _mt_log_h

#include <stdbool.h>

typedef enum
{
    MT_LOG_DEBUG = 0,
    MT_LOG_INFO  = 1,
    MT_LOG_WARN  = 2,
    MT_LOG_ERROR = 3,
} mt_log_importance;

void mt_log(mt_log_importance importance, const char* file, int line, const char* fmt, ...);
void mt_log_set_level(mt_log_importance importance);
int  mt_log_get_level();
void mt_log_inc_verbosity(void);
void mt_log_use_colors(bool use_colors);
void mt_log_set_file_column(int column);

#define mt_log_debug(...) mt_log(MT_LOG_DEBUG, __FILE__, __LINE__, __VA_ARGS__)
#define mt_log_info(...) mt_log(MT_LOG_INFO, __FILE__, __LINE__, __VA_ARGS__)
#define mt_log_warn(...) mt_log(MT_LOG_WARN, __FILE__, __LINE__, __VA_ARGS__)
#define mt_log_error(...) mt_log(MT_LOG_ERROR, __FILE__, __LINE__, __VA_ARGS__)

#define mt_log_level_debug() mt_log_set_level(MT_LOG_DEBUG);
#define mt_log_level_info() mt_log_set_level(MT_LOG_INFO);
#define mt_log_level_warn() mt_log_set_level(MT_LOG_WARN);
#define mt_log_level_error() mt_log_set_level(MT_LOG_ERROR);

#endif

#if __INCLUDE_LEVEL__ == 0

#define COLOR_RESET "\x1B[0m"
#define COLOR_WHITE "\x1B[1;37m"
#define COLOR_BLACK "\x1B[0;30m"
#define COLOR_BLUE "\x1B[0;34m"
#define COLOR_LIGHT_BLUE "\x1B[1;34m"
#define COLOR_GREEN "\x1B[0;32m"
#define COLOR_LIGHT_GREEN "\x1B[1;32m"
#define COLOR_CYAN "\x1B[0;36m"
#define COLOR_LIGHT_CYAN "\x1B[1;36m"
#define COLOR_RED "\x1B[0;31m"
#define COLOR_LIGHT_RED "\x1B[1;31m"
#define COLOR_PURPLE "\x1B[0;35m"
#define COLOR_LIGHT_PURPLE "\x1B[1;35m"
#define COLOR_BROWN "\x1B[0;33m"
#define COLOR_YELLOW "\x1B[1;33m"
#define COLOR_GRAY "\x1B[0;30m"
#define COLOR_LIGHT_GRAY "\x1B[0;37m"

#define __USE_POSIX199309 1
/* #define _POSIX_C_SOURCE 199506L */

#include <errno.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

static mt_log_importance min_importance_to_log = MT_LOG_WARN;

static bool use_colors = false;
static int  file_col   = 100;

static const char* verbosity_names[] = {
    "DEBUG",
    "INFO",
    "WARN",
    "ERROR",
};

static const char* verbosity_colors[] = {
    COLOR_LIGHT_CYAN,
    COLOR_GREEN,
    COLOR_YELLOW,
    COLOR_LIGHT_RED,
};

void mt_log(const mt_log_importance importance, const char* file, const int line, const char* fmt, ...)
{
    if (importance < min_importance_to_log)
    {
	return;
    }

    struct timespec ts;
    if (clock_gettime(CLOCK_REALTIME, &ts) != 0)
    {
	fprintf(stdout, "clock_gettime() failed: %s\n", strerror(errno));
	ts.tv_sec  = 0;
	ts.tv_nsec = 0;
    }

    struct tm* my_tm = localtime(&ts.tv_sec);

    if (use_colors)
    {
	fprintf(
	    stdout,
	    "%s%-5s%s  ",
	    verbosity_colors[importance],
	    verbosity_names[importance],
	    COLOR_RESET);
    }
    else
    {
	fprintf(stdout, "%-5s  ", verbosity_names[importance]);
    }

    va_list args;
    va_start(args, fmt);
    vfprintf(stdout, fmt, args);
    va_end(args);

    if (use_colors)
    {
	fprintf(
	    stdout,
	    "\033[%iG  %.2i:%.2i:%.2i:%.6li %15s : %d%s ",
	    file_col,
	    my_tm->tm_hour,
	    my_tm->tm_min,
	    my_tm->tm_sec,
	    ts.tv_nsec / 1000,
	    file,
	    line,
	    COLOR_RESET);
    }
    else
    {
	fprintf(
	    stdout,
	    "\033[%iG  %.2i:%.2i:%.2i:%.6li ( %s:%d: )",
	    file_col,
	    my_tm->tm_hour,
	    my_tm->tm_min,
	    my_tm->tm_sec,
	    ts.tv_nsec / 1000,
	    file,
	    line);
    }

    fprintf(stdout, "\n");
}

void mt_log_set_level(const mt_log_importance importance)
{
    min_importance_to_log = importance;
}

void mt_log_use_colors(const bool colors)
{
    use_colors = colors;
}

void mt_log_set_file_column(int column)
{
    file_col = column;
}

void mt_log_inc_verbosity(void)
{
    if (min_importance_to_log != MT_LOG_DEBUG)
    {
	min_importance_to_log -= 1;
	mt_log_debug("Set log level to %s", verbosity_names[min_importance_to_log]);
    }
}

int mt_log_get_level()
{
    return min_importance_to_log;
}

#endif
