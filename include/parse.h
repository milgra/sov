#ifndef _SOV_PARSE_H
#define _SOV_PARSE_H

#include <stdbool.h>

#include "color.h"

bool sov_parse_color(const char* restrict str, char** restrict str_end, struct sov_color* color);

bool sov_parse_input(const char* input_buffer, unsigned long* percentage, struct sov_color* background, struct sov_color* border, struct sov_color* bar);

#endif
