/*
 * sgutils - Linux Screen Graphics Utilities
 * Copyright (c) 2026 Anand <anand@truchiptechnology.com> - MIT License
 * Date: 2026-05-07
 *
 * compositing.h - Offscreen buffer compositing
 */

#ifndef SGUTILS_COMPOSITING_H
#define SGUTILS_COMPOSITING_H

#include "draw.h"

/*
 * A compositing layer: an offscreen image with position and opacity.
 */
typedef struct {
    image_t *image;
    int      x;
    int      y;
    uint8_t  opacity;  /* 0 = transparent, 255 = fully opaque */
    int      visible;
} layer_t;

/*
 * A compositor manages an ordered list of layers and renders them
 * onto a destination image.
 */
typedef struct {
    layer_t **layers;
    int       count;
    int       capacity;
    int       width;
    int       height;
} compositor_t;

compositor_t *compositor_create(int width, int height);
void          compositor_free(compositor_t *comp);

/*
 * Add a layer to the compositor. Returns the layer index, or -1 on failure.
 * The compositor takes ownership of the image.
 */
int compositor_add_layer(compositor_t *comp, image_t *image, int x, int y, uint8_t opacity);

/*
 * Render all visible layers onto the destination image, bottom to top.
 * Uses alpha blending.
 */
void compositor_render(compositor_t *comp, image_t *dst);

/*
 * Render all visible layers to a new image and return it.
 * Caller must free the returned image.
 */
image_t *compositor_render_to_image(compositor_t *comp);

#endif /* SGUTILS_COMPOSITING_H */
