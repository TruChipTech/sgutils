/*
 * sgutils - Linux Screen Graphics Utilities
 * Copyright (c) 2026 Anand <anand@truchiptechnology.com> - MIT License
 * Date: 2026-05-07
 *
 * draw.h - Core drawing primitives and image/context types
 */

#ifndef SGUTILS_DRAW_H
#define SGUTILS_DRAW_H

#include <stdint.h>

/* ── Pixel format ──────────────────────────────────────────────
 * Pixels are stored as 32-bit ARGB:
 *   bits 31-24: alpha (0x00 = transparent, 0xFF = opaque)
 *   bits 23-16: red
 *   bits 15-8 : green
 *   bits  7-0 : blue
 */
typedef uint32_t pixel_t;

/* Helper macros for pixel manipulation */
#define PIXEL_ARGB(a, r, g, b) \
    (((pixel_t)(a) << 24) | ((pixel_t)(r) << 16) | ((pixel_t)(g) << 8) | (pixel_t)(b))
#define PIXEL_RGB(r, g, b)     PIXEL_ARGB(0xFF, r, g, b)
#define PIXEL_A(px)            (((px) >> 24) & 0xFF)
#define PIXEL_R(px)            (((px) >> 16) & 0xFF)
#define PIXEL_G(px)            (((px) >>  8) & 0xFF)
#define PIXEL_B(px)            (((px)      ) & 0xFF)

/* ── Image ─────────────────────────────────────────────────── */
typedef struct {
    pixel_t *data;
    int      width;
    int      height;
} image_t;

image_t *image_create(int width, int height);
void     image_free(image_t *image);
image_t *image_clone(const image_t *src);

/* ── Transforms ────────────────────────────────────────────── */
void     image_invert(image_t *image);
void     image_grayscale(image_t *image);
void     image_hueify(image_t *image, uint32_t mask, int min_level);
void     image_brightness(image_t *image, int delta);
void     image_rotate(image_t *image, int degrees); /* 90, 180, 270 */
image_t *image_scale(const image_t *src, int w, int h);

/* Commonly used hue mask values */
#define MASK_RED     0xFF0000
#define MASK_GREEN   0x00FF00
#define MASK_BLUE    0x0000FF
#define MASK_YELLOW  0xFFFF00
#define MASK_CYAN    0x00FFFF
#define MASK_MAGENTA 0xFF00FF
#define MASK_WHITE   0xFFFFFF
#define MASK_NONE    0x000000

#define HUE_THRESHOLD_DEFAULT 20

/* ── Drawing primitives ────────────────────────────────────── */
void set_pixel(image_t *dst, int x, int y, pixel_t color);
pixel_t get_pixel(const image_t *dst, int x, int y);
void draw_rect(image_t *dst, int x, int y, int w, int h, pixel_t color);
void draw_rect_outline(image_t *dst, int x, int y, int w, int h, pixel_t color, int thickness);
void draw_line(image_t *dst, int x0, int y0, int x1, int y1, pixel_t color);
void draw_circle(image_t *dst, int cx, int cy, int radius, pixel_t color, int filled);
void draw_image(image_t *dst, int x, int y, const image_t *src);
void draw_image_alpha(image_t *dst, int x, int y, const image_t *src);
void clear_image(image_t *dst);
void fill_image(image_t *dst, pixel_t color);
void test_pattern(image_t *dst);

/* ── Alpha blending ────────────────────────────────────────── */
pixel_t alpha_blend(pixel_t bg, pixel_t fg);

#endif /* SGUTILS_DRAW_H */
