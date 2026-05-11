/*
 * sgutils - Linux Screen Graphics Utilities
 * Copyright (c) 2026 Anand <anand@truchiptechnology.com> - MIT License
 * Date: 2026-05-07
 *
 * img-jpeg.h - JPEG image loading
 */

#ifndef SGUTILS_IMG_JPEG_H
#define SGUTILS_IMG_JPEG_H

#include "draw.h"

#ifdef ENABLE_JPEG
image_t *read_jpeg_file(const char *filename);
#endif

#endif /* SGUTILS_IMG_JPEG_H */
