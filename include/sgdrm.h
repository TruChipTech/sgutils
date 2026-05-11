/*
 * sgutils - Linux Screen Graphics Utilities
 * Copyright (c) 2026 Anand <anand@truchiptechnology.com> - MIT License
 * Date: 2026-05-07
 *
 * drm.h - DRM/KMS display access
 *
 * Provides a display context similar to framebuffer.h but using
 * the Linux DRM (Direct Rendering Manager) / KMS subsystem.
 * This allows sgutils to work on modern systems where /dev/fb*
 * may not be available.
 */

#ifndef SGUTILS_DRM_H
#define SGUTILS_DRM_H

#include "draw.h"

#include <stdint.h>

/* Forward declarations for DRM internals */
typedef struct sg_drm_context sg_drm_context_t;

/*
 * Information about the current DRM connector and mode.
 */
typedef struct {
    char     connector_name[64];
    uint32_t connector_id;
    uint32_t crtc_id;
    int      width;
    int      height;
    int      refresh;
    int      bpp;
    char     driver_name[64];
} drm_mode_info_t;

/*
 * Open a DRM device and set up a dumb buffer for rendering.
 *
 * @param device  Path such as "/dev/dri/card0" (NULL defaults to /dev/dri/card0).
 * @return        Context pointer, or NULL on failure.
 *
 * The context maps a dumb framebuffer that can be drawn to directly.
 * Call drm_context_release() when done.
 */
sg_drm_context_t *drm_context_create(const char *device);

/*
 * Query DRM device information without mapping a framebuffer.
 * Fills in the mode_info structure.
 *
 * @param device     Path such as "/dev/dri/card0" (NULL for default).
 * @param info       Output mode info structure.
 * @param info_count Maximum number of entries in info array.
 * @return           Number of connected outputs found, or -1 on error.
 */
int drm_get_mode_info(const char *device, drm_mode_info_t *info, int info_count);

/*
 * Get dimensions of the active DRM context.
 */
int drm_context_width(const sg_drm_context_t *ctx);
int drm_context_height(const sg_drm_context_t *ctx);

/*
 * Create an image_t view wrapping the DRM dumb buffer.
 * The returned image does NOT own the data (do not call image_free on it).
 */
image_t drm_context_as_image(sg_drm_context_t *ctx);

/*
 * Page-flip / present the buffer to the screen.
 * For double-buffered setups this swaps front/back.
 * For single-buffer dumb buffers this is a no-op
 * (the buffer is already visible).
 */
void drm_context_present(sg_drm_context_t *ctx);

/*
 * Release the DRM context — unmap buffer, restore original mode, close device.
 */
void drm_context_release(sg_drm_context_t *ctx);

/*
 * Get the device path from a context.
 */
const char *drm_context_device(const sg_drm_context_t *ctx);

#endif /* SGUTILS_DRM_H */
