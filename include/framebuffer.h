/*
 * sgutils - Linux Screen Graphics Utilities
 * Copyright (c) 2026 Anand <anand@truchiptechnology.com> - MIT License
 * Date: 2026-05-07
 *
 * framebuffer.h - Framebuffer device access
 */

#ifndef SGUTILS_FRAMEBUFFER_H
#define SGUTILS_FRAMEBUFFER_H

#include "draw.h"

/* Framebuffer context wraps the mmap'd device */
typedef struct {
    pixel_t *data;
    int      width;
    int      height;
    char    *fb_name;
    int      fb_file_desc;
    int      fb_size;
} fb_context_t;

/*
 * Open the framebuffer device and mmap it for read/write access.
 * @param device  Path such as "/dev/fb0" (NULL defaults to /dev/fb0).
 * @return        Context pointer, or NULL on failure.
 */
fb_context_t *fb_context_create(const char *device);

/*
 * Query framebuffer dimensions without mapping the buffer.
 * Useful for sgfb-describe. The data pointer will be NULL.
 */
fb_context_t *fb_context_get_dimensions(const char *device);

/*
 * Unmap and close the framebuffer device. Frees the context.
 */
void fb_context_release(fb_context_t *ctx);

/*
 * Create an image_t view wrapping the framebuffer data.
 * The returned image does NOT own the data (do not call image_free on it).
 */
image_t fb_context_as_image(fb_context_t *ctx);

#endif /* SGUTILS_FRAMEBUFFER_H */
