#ifndef _SOV_LOG_H
#define _SOV_LOG_H

#include <stdbool.h>

typedef enum
{
  SOV_LOG_DEBUG = 0,
  SOV_LOG_INFO  = 1,
  SOV_LOG_WARN  = 2,
  SOV_LOG_ERROR = 3,
} sov_log_importance;

void sov_log(sov_log_importance importance, const char* file, int line, const char* fmt, ...);

void sov_log_set_level(sov_log_importance importance);

int sov_log_get_level();

void sov_log_inc_verbosity(void);

void sov_log_use_colors(bool use_colors);

#define sov_log_debug(...) sov_log(SOV_LOG_DEBUG, SOV_FILE, __LINE__, __VA_ARGS__)
#define sov_log_info(...) sov_log(SOV_LOG_INFO, SOV_FILE, __LINE__, __VA_ARGS__)
#define sov_log_warn(...) sov_log(SOV_LOG_WARN, SOV_FILE, __LINE__, __VA_ARGS__)
#define sov_log_error(...) sov_log(SOV_LOG_ERROR, SOV_FILE, __LINE__, __VA_ARGS__)

#define sov_log_level_debug() sov_log_set_level(SOV_LOG_DEBUG);
#define sov_log_level_info() sov_log_set_level(SOV_LOG_INFO);
#define sov_log_level_warn() sov_log_set_level(SOV_LOG_WARN);
#define sov_log_level_error() sov_log_set_level(SOV_LOG_ERROR);

#endif
