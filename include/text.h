/*
 * sgutils - Linux Screen Graphics Utilities
 * Copyright (c) 2026 Anand <anand@truchiptechnology.com> - MIT License
 * Date: 2026-05-07
 *
 * text.h - FreeType-based text rendering
 */

#ifndef SGUTILS_TEXT_H
#define SGUTILS_TEXT_H

#include "draw.h"

#ifdef ENABLE_TEXT

typedef struct fb_font fb_font_t;

/*
 * Initialize the text subsystem. Call once before using any text functions.
 * Returns 0 on success, -1 on failure.
 */
int text_init(void);

/*
 * Shut down the text subsystem. Call once when done.
 */
void text_shutdown(void);

/*
 * Load a TrueType/OpenType font from a file at the given pixel size.
 * Returns NULL on failure.
 */
fb_font_t *font_load(const char *path, int pixel_size);

/*
 * Free a loaded font.
 */
void font_free(fb_font_t *font);

/*
 * Draw a UTF-8 string onto the destination image.
 * @param dst     Target image to draw onto.
 * @param x, y    Top-left baseline origin.
 * @param text    Null-terminated UTF-8 string.
 * @param font    Loaded font.
 * @param color   Foreground color (alpha channel is used for blending).
 */
void draw_text(image_t *dst, int x, int y, const char *text,
               fb_font_t *font, pixel_t color);

/*
 * Measure the bounding box of a string without drawing.
 * @param font    Loaded font.
 * @param text    Null-terminated UTF-8 string.
 * @param out_w   Output: width in pixels.
 * @param out_h   Output: height in pixels.
 */
void measure_text(fb_font_t *font, const char *text, int *out_w, int *out_h);

#endif /* ENABLE_TEXT */
#endif /* SGUTILS_TEXT_H */
