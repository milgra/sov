#ifndef _SOV_COLOR_H
#define _SOV_COLOR_H

#include <stdint.h>

struct sov_color
{
  float a;
  float r;
  float g;
  float b;
};

uint32_t sov_color_to_argb(struct sov_color color);

uint32_t sov_color_to_rgba(struct sov_color color);

struct sov_color sov_color_premultiply_alpha(struct sov_color color);

#endif
