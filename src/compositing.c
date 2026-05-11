/*
 * sgutils - Linux Screen Graphics Utilities
 * Copyright (c) 2026 Anand <anand@truchiptechnology.com> - MIT License
 * Date: 2026-05-07
 *
 * compositing.c - Offscreen buffer compositing system
 *
 * Implements the TODO item: "Add compositing which allows for multiple
 * draw commands to be rendered to an offscreen buffer, and then rendered"
 */

#include "compositing.h"

#include <stdlib.h>
#include <string.h>

#define INITIAL_CAPACITY 8

compositor_t *compositor_create(int width, int height) {
    if (width <= 0 || height <= 0) return NULL;

    compositor_t *comp = malloc(sizeof(compositor_t));
    if (!comp) return NULL;

    comp->layers   = calloc(INITIAL_CAPACITY, sizeof(layer_t *));
    comp->count    = 0;
    comp->capacity = INITIAL_CAPACITY;
    comp->width    = width;
    comp->height   = height;

    if (!comp->layers) {
        free(comp);
        return NULL;
    }

    return comp;
}

void compositor_free(compositor_t *comp) {
    if (!comp) return;

    for (int i = 0; i < comp->count; i++) {
        if (comp->layers[i]) {
            image_free(comp->layers[i]->image);
            free(comp->layers[i]);
        }
    }

    free(comp->layers);
    free(comp);
}

int compositor_add_layer(compositor_t *comp, image_t *image,
                          int x, int y, uint8_t opacity) {
    if (!comp || !image) return -1;

    /* Grow if needed */
    if (comp->count >= comp->capacity) {
        int new_cap = comp->capacity * 2;
        layer_t **new_layers = realloc(comp->layers,
                                        sizeof(layer_t *) * (size_t)new_cap);
        if (!new_layers) return -1;
        comp->layers   = new_layers;
        comp->capacity = new_cap;
    }

    layer_t *layer = malloc(sizeof(layer_t));
    if (!layer) return -1;

    layer->image   = image;
    layer->x       = x;
    layer->y       = y;
    layer->opacity = opacity;
    layer->visible = 1;

    int idx = comp->count;
    comp->layers[idx] = layer;
    comp->count++;

    return idx;
}

void compositor_render(compositor_t *comp, image_t *dst) {
    if (!comp || !dst || !dst->data) return;

    for (int i = 0; i < comp->count; i++) {
        layer_t *layer = comp->layers[i];
        if (!layer || !layer->visible || !layer->image) continue;

        image_t *src = layer->image;

        /* Calculate clipped region */
        int sx = 0, sy = 0;
        int sw = src->width, sh = src->height;
        int dx = layer->x, dy = layer->y;

        if (dx < 0) { sx = -dx; sw += dx; dx = 0; }
        if (dy < 0) { sy = -dy; sh += dy; dy = 0; }
        if (dx + sw > dst->width)  sw = dst->width  - dx;
        if (dy + sh > dst->height) sh = dst->height - dy;
        if (sw <= 0 || sh <= 0) continue;

        uint8_t layer_opacity = layer->opacity;

        for (int row = 0; row < sh; row++) {
            for (int col = 0; col < sw; col++) {
                pixel_t src_px = src->data[(sy + row) * src->width + (sx + col)];

                /* Modulate source alpha by layer opacity */
                if (layer_opacity < 255) {
                    uint8_t sa = PIXEL_A(src_px);
                    sa = (uint8_t)((uint16_t)sa * layer_opacity / 255);
                    src_px = PIXEL_ARGB(sa,
                                        PIXEL_R(src_px),
                                        PIXEL_G(src_px),
                                        PIXEL_B(src_px));
                }

                int di = (dy + row) * dst->width + (dx + col);
                dst->data[di] = alpha_blend(dst->data[di], src_px);
            }
        }
    }
}

image_t *compositor_render_to_image(compositor_t *comp) {
    if (!comp) return NULL;

    image_t *result = image_create(comp->width, comp->height);
    if (!result) return NULL;

    compositor_render(comp, result);
    return result;
}
