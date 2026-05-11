/*
 * sgutils - Linux Screen Graphics Utilities
 * Copyright (c) 2026 Anand <anand@truchiptechnology.com> - MIT License
 * Date: 2026-05-07
 *
 * draw.c - Core drawing primitives, image transforms, and alpha blending
 *
 * Bug fixes over the original:
 *  - Proper ARGB pixel format with alpha channel support
 *  - Fixed test_pattern() memcpy using context->data instead of context
 *  - Fixed clear_context_gray() to fill 32-bit pixels correctly
 *  - Fixed integer scaling in image_scale() to use float properly
 *  - All transforms preserve alpha channel
 *  - Bounds checks prevent out-of-bounds writes
 */

#include "draw.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>

/* ── Helper ──────────────────────────────────────────────────── */

static inline unsigned char absdiff(unsigned char a, unsigned char b) {
    return (a > b) ? (a - b) : (b - a);
}

static inline int clamp_int(int v, int lo, int hi) {
    if (v < lo) return lo;
    if (v > hi) return hi;
    return v;
}

/* ── Alpha blending ──────────────────────────────────────────── */

pixel_t alpha_blend(pixel_t bg, pixel_t fg) {
    uint8_t fa = PIXEL_A(fg);

    if (fa == 0xFF) return fg;
    if (fa == 0x00) return bg;

    uint8_t ba = PIXEL_A(bg);
    uint8_t fr = PIXEL_R(fg), fg_ = PIXEL_G(fg), fb = PIXEL_B(fg);
    uint8_t br = PIXEL_R(bg), bg_ = PIXEL_G(bg), bb = PIXEL_B(bg);

    /* Standard "over" compositing */
    uint16_t inv_fa = 255 - fa;
    uint8_t or = (uint8_t)((fr * fa + br * inv_fa) / 255);
    uint8_t og = (uint8_t)((fg_ * fa + bg_ * inv_fa) / 255);
    uint8_t ob = (uint8_t)((fb * fa + bb * inv_fa) / 255);
    uint8_t oa = (uint8_t)(fa + (ba * inv_fa) / 255);

    return PIXEL_ARGB(oa, or, og, ob);
}

/* ── Image lifecycle ─────────────────────────────────────────── */

image_t *image_create(int width, int height) {
    if (width <= 0 || height <= 0) return NULL;

    image_t *img = malloc(sizeof(image_t));
    if (!img) return NULL;

    img->data = calloc((size_t)width * (size_t)height, sizeof(pixel_t));
    if (!img->data) {
        free(img);
        return NULL;
    }
    img->width  = width;
    img->height = height;
    return img;
}

void image_free(image_t *image) {
    if (!image) return;
    free(image->data);
    image->data   = NULL;
    image->width  = 0;
    image->height = 0;
    free(image);
}

image_t *image_clone(const image_t *src) {
    if (!src || !src->data) return NULL;

    image_t *dst = image_create(src->width, src->height);
    if (!dst) return NULL;

    memcpy(dst->data, src->data,
           (size_t)src->width * (size_t)src->height * sizeof(pixel_t));
    return dst;
}

/* ── Pixel access ────────────────────────────────────────────── */

void set_pixel(image_t *dst, int x, int y, pixel_t color) {
    if (!dst || !dst->data) return;
    if (x < 0 || x >= dst->width || y < 0 || y >= dst->height) return;
    dst->data[y * dst->width + x] = color;
}

pixel_t get_pixel(const image_t *dst, int x, int y) {
    if (!dst || !dst->data) return 0;
    if (x < 0 || x >= dst->width || y < 0 || y >= dst->height) return 0;
    return dst->data[y * dst->width + x];
}

/* ── Transforms ──────────────────────────────────────────────── */

void image_invert(image_t *image) {
    if (!image || !image->data) return;

    int count = image->width * image->height;
    for (int i = 0; i < count; i++) {
        pixel_t px = image->data[i];
        uint8_t a = PIXEL_A(px);
        uint8_t r = 255 - PIXEL_R(px);
        uint8_t g = 255 - PIXEL_G(px);
        uint8_t b = 255 - PIXEL_B(px);
        image->data[i] = PIXEL_ARGB(a, r, g, b);
    }
}

void image_grayscale(image_t *image) {
    image_hueify(image, MASK_WHITE, 0);
}

void image_hueify(image_t *image, uint32_t mask, int min_level) {
    if (!image || !image->data) return;

    int count = image->width * image->height;
    for (int i = 0; i < count; i++) {
        pixel_t px = image->data[i];
        uint8_t a = PIXEL_A(px);
        uint8_t r = PIXEL_R(px);
        uint8_t g = PIXEL_G(px);
        uint8_t b = PIXEL_B(px);

        int is_color = (absdiff(r, g) > min_level ||
                        absdiff(g, b) > min_level ||
                        absdiff(r, b) > min_level);

        if (!is_color) continue;

        /* Approximate intensity via bitwise OR of components */
        uint8_t intensity = r | g | b;

        uint8_t newr = (uint8_t)((mask & ((uint32_t)intensity << 16)) >> 16);
        uint8_t newg = (uint8_t)((mask & ((uint32_t)intensity <<  8)) >>  8);
        uint8_t newb = (uint8_t)((mask & ((uint32_t)intensity      ))      );

        image->data[i] = PIXEL_ARGB(a, newr, newg, newb);
    }
}

void image_brightness(image_t *image, int delta) {
    if (!image || !image->data) return;

    int count = image->width * image->height;
    for (int i = 0; i < count; i++) {
        pixel_t px = image->data[i];
        uint8_t a = PIXEL_A(px);
        int r = clamp_int(PIXEL_R(px) + delta, 0, 255);
        int g = clamp_int(PIXEL_G(px) + delta, 0, 255);
        int b = clamp_int(PIXEL_B(px) + delta, 0, 255);
        image->data[i] = PIXEL_ARGB(a, r, g, b);
    }
}

void image_rotate(image_t *image, int degrees) {
    if (!image || !image->data) return;

    /* Normalize to 0, 90, 180, 270 */
    degrees = ((degrees % 360) + 360) % 360;
    if (degrees == 0) return;

    int sw = image->width;
    int sh = image->height;
    int dw, dh;

    if (degrees == 180) {
        dw = sw;
        dh = sh;
    } else { /* 90 or 270 */
        dw = sh;
        dh = sw;
    }

    pixel_t *new_data = malloc(sizeof(pixel_t) * (size_t)dw * (size_t)dh);
    if (!new_data) return;

    for (int sy = 0; sy < sh; sy++) {
        for (int sx = 0; sx < sw; sx++) {
            int dx, dy;
            if (degrees == 90) {
                dx = sh - 1 - sy;
                dy = sx;
            } else if (degrees == 180) {
                dx = sw - 1 - sx;
                dy = sh - 1 - sy;
            } else { /* 270 */
                dx = sy;
                dy = sw - 1 - sx;
            }
            new_data[dy * dw + dx] = image->data[sy * sw + sx];
        }
    }

    free(image->data);
    image->data   = new_data;
    image->width  = dw;
    image->height = dh;
}

/*
 * Scale with crop-to-fill (maintains aspect ratio).
 * Fixed: uses proper float-based coordinate mapping.
 */
image_t *image_scale(const image_t *src, int w, int h) {
    if (!src || !src->data || w <= 0 || h <= 0) return NULL;

    float src_aspect = (float)src->width / (float)src->height;
    float dst_aspect = (float)w / (float)h;

    int crop_w = src->width;
    int crop_h = src->height;
    int crop_x = 0;
    int crop_y = 0;

    if (src_aspect > dst_aspect) {
        /* Source is wider: crop sides */
        crop_w = (int)(src->height * dst_aspect);
        crop_x = (src->width - crop_w) / 2;
    } else if (src_aspect < dst_aspect) {
        /* Source is taller: crop top/bottom */
        crop_h = (int)(src->width / dst_aspect);
        crop_y = (src->height - crop_h) / 2;
    }

    image_t *dst = image_create(w, h);
    if (!dst) return NULL;

    for (int y = 0; y < h; y++) {
        for (int x = 0; x < w; x++) {
            int sx = (int)((float)crop_w / (float)w * (float)x) + crop_x;
            int sy = (int)((float)crop_h / (float)h * (float)y) + crop_y;

            /* Clamp to source bounds */
            if (sx >= src->width)  sx = src->width  - 1;
            if (sy >= src->height) sy = src->height - 1;
            if (sx < 0) sx = 0;
            if (sy < 0) sy = 0;

            dst->data[y * w + x] = src->data[sy * src->width + sx];
        }
    }

    return dst;
}

/* ── Drawing primitives ──────────────────────────────────────── */

void draw_rect(image_t *dst, int x, int y, int w, int h, pixel_t color) {
    if (!dst || !dst->data) return;

    /* Clip to bounds */
    int x0 = (x < 0) ? 0 : x;
    int y0 = (y < 0) ? 0 : y;
    int x1 = (x + w > dst->width)  ? dst->width  : x + w;
    int y1 = (y + h > dst->height) ? dst->height : y + h;

    if (x0 >= x1 || y0 >= y1) return;

    /* Fill first row */
    for (int rx = x0; rx < x1; rx++) {
        dst->data[y0 * dst->width + rx] = color;
    }

    /* Copy first row to remaining rows */
    int row_bytes = (x1 - x0) * (int)sizeof(pixel_t);
    for (int ry = y0 + 1; ry < y1; ry++) {
        memcpy(&dst->data[ry * dst->width + x0],
               &dst->data[y0 * dst->width + x0],
               (size_t)row_bytes);
    }
}

void draw_rect_outline(image_t *dst, int x, int y, int w, int h,
                        pixel_t color, int thickness) {
    if (!dst || !dst->data || thickness <= 0) return;

    draw_rect(dst, x, y, w, thickness, color);               /* top */
    draw_rect(dst, x, y + h - thickness, w, thickness, color); /* bottom */
    draw_rect(dst, x, y, thickness, h, color);               /* left */
    draw_rect(dst, x + w - thickness, y, thickness, h, color); /* right */
}

void draw_line(image_t *dst, int x0, int y0, int x1, int y1, pixel_t color) {
    if (!dst || !dst->data) return;

    /* Bresenham's line algorithm */
    int dx = abs(x1 - x0);
    int dy = -abs(y1 - y0);
    int sx = (x0 < x1) ? 1 : -1;
    int sy = (y0 < y1) ? 1 : -1;
    int err = dx + dy;

    for (;;) {
        set_pixel(dst, x0, y0, color);
        if (x0 == x1 && y0 == y1) break;
        int e2 = 2 * err;
        if (e2 >= dy) { err += dy; x0 += sx; }
        if (e2 <= dx) { err += dx; y0 += sy; }
    }
}

void draw_circle(image_t *dst, int cx, int cy, int radius, pixel_t color, int filled) {
    if (!dst || !dst->data || radius < 0) return;

    /* Midpoint circle algorithm */
    int x = radius;
    int y = 0;
    int err = 1 - radius;

    while (x >= y) {
        if (filled) {
            /* Draw horizontal spans */
            for (int i = cx - x; i <= cx + x; i++) {
                set_pixel(dst, i, cy + y, color);
                set_pixel(dst, i, cy - y, color);
            }
            for (int i = cx - y; i <= cx + y; i++) {
                set_pixel(dst, i, cy + x, color);
                set_pixel(dst, i, cy - x, color);
            }
        } else {
            set_pixel(dst, cx + x, cy + y, color);
            set_pixel(dst, cx - x, cy + y, color);
            set_pixel(dst, cx + x, cy - y, color);
            set_pixel(dst, cx - x, cy - y, color);
            set_pixel(dst, cx + y, cy + x, color);
            set_pixel(dst, cx - y, cy + x, color);
            set_pixel(dst, cx + y, cy - x, color);
            set_pixel(dst, cx - y, cy - x, color);
        }

        y++;
        if (err < 0) {
            err += 2 * y + 1;
        } else {
            x--;
            err += 2 * (y - x) + 1;
        }
    }
}

void draw_image(image_t *dst, int x, int y, const image_t *src) {
    if (!dst || !dst->data || !src || !src->data) return;

    /* Clip source region to destination bounds */
    int sx = 0, sy = 0;
    int sw = src->width, sh = src->height;

    if (x < 0) { sx = -x; sw += x; x = 0; }
    if (y < 0) { sy = -y; sh += y; y = 0; }
    if (x + sw > dst->width)  sw = dst->width  - x;
    if (y + sh > dst->height) sh = dst->height - y;
    if (sw <= 0 || sh <= 0) return;

    for (int row = 0; row < sh; row++) {
        memcpy(&dst->data[(y + row) * dst->width + x],
               &src->data[(sy + row) * src->width + sx],
               (size_t)sw * sizeof(pixel_t));
    }
}

void draw_image_alpha(image_t *dst, int x, int y, const image_t *src) {
    if (!dst || !dst->data || !src || !src->data) return;

    int sx = 0, sy = 0;
    int sw = src->width, sh = src->height;

    if (x < 0) { sx = -x; sw += x; x = 0; }
    if (y < 0) { sy = -y; sh += y; y = 0; }
    if (x + sw > dst->width)  sw = dst->width  - x;
    if (y + sh > dst->height) sh = dst->height - y;
    if (sw <= 0 || sh <= 0) return;

    for (int row = 0; row < sh; row++) {
        for (int col = 0; col < sw; col++) {
            int di = (y + row) * dst->width + (x + col);
            int si = (sy + row) * src->width + (sx + col);
            dst->data[di] = alpha_blend(dst->data[di], src->data[si]);
        }
    }
}

void clear_image(image_t *dst) {
    if (!dst || !dst->data) return;
    memset(dst->data, 0, (size_t)dst->width * (size_t)dst->height * sizeof(pixel_t));
}

void fill_image(image_t *dst, pixel_t color) {
    if (!dst || !dst->data) return;

    /* Set first pixel, then memcpy-fill entire buffer */
    int count = dst->width * dst->height;
    if (count <= 0) return;

    dst->data[0] = color;
    /* Doubling memcpy fill for efficiency */
    int filled = 1;
    while (filled < count) {
        int chunk = (filled < (count - filled)) ? filled : (count - filled);
        memcpy(&dst->data[filled], dst->data, (size_t)chunk * sizeof(pixel_t));
        filled += chunk;
    }
}

void test_pattern(image_t *dst) {
    if (!dst || !dst->data) return;

    static const pixel_t pattern[8] = {
        0xFFFFFFFF, /* white */
        0xFFFFFF00, /* yellow */
        0xFF00FFFF, /* cyan */
        0xFF00FF00, /* green */
        0xFFFF00FF, /* magenta */
        0xFFFF0000, /* red */
        0xFF0000FF, /* blue */
        0xFF000000  /* black */
    };

    int col_width = dst->width / 8;
    if (col_width == 0) col_width = 1;

    /* Fill first row */
    for (int x = 0; x < dst->width; x++) {
        int idx = x / col_width;
        if (idx > 7) idx = 7;
        dst->data[x] = pattern[idx];
    }

    /* Copy first row to all other rows (FIXED: use dst->data, not dst) */
    for (int y = 1; y < dst->height; y++) {
        memcpy(&dst->data[y * dst->width],
               dst->data,
               (size_t)dst->width * sizeof(pixel_t));
    }
}
