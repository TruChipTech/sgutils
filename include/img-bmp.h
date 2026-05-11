/*
 * sgutils - Linux Screen Graphics Utilities
 * Copyright (c) 2026 Anand <anand@truchiptechnology.com> - MIT License
 * Date: 2026-05-07
 *
 * img-bmp.h - BMP image loading
 */

#ifndef SGUTILS_IMG_BMP_H
#define SGUTILS_IMG_BMP_H

#include "draw.h"

#ifdef ENABLE_BMP
image_t *read_bmp_file(const char *filename);
#endif

#endif /* SGUTILS_IMG_BMP_H */
