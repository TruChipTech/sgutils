/*
 * sgutils - Linux Screen Graphics Utilities
 * Copyright (c) 2026 Anand <anand@truchiptechnology.com> - MIT License
 * Date: 2026-05-07
 *
 * img-svg.h - SVG image loading (via librsvg + cairo)
 */

#ifndef SGUTILS_IMG_SVG_H
#define SGUTILS_IMG_SVG_H

#include "draw.h"

#ifdef ENABLE_SVG
image_t *read_svg_file(const char *filename, int r_width, int r_height);
#endif

#endif /* SGUTILS_IMG_SVG_H */
