/*
 * sgutils - Linux Screen Graphics Utilities
 * Copyright (c) 2026 Anand <anand@truchiptechnology.com> - MIT License
 * Date: 2026-05-07
 *
 * img-png.h - PNG image loading
 */

#ifndef SGUTILS_IMG_PNG_H
#define SGUTILS_IMG_PNG_H

#include "draw.h"

#ifdef ENABLE_PNG
image_t *read_png_file(const char *filename);
#endif

#endif /* SGUTILS_IMG_PNG_H */
