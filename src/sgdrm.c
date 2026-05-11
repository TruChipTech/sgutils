/*
 * sgutils - Linux Screen Graphics Utilities
 * Copyright (c) 2026 Anand <anand@truchiptechnology.com> - MIT License
 * Date: 2026-05-07
 *
 * drm.c - DRM/KMS display access via dumb buffers
 *
 * Uses the Linux DRM (Direct Rendering Manager) / KMS subsystem
 * to create a dumb framebuffer for direct pixel rendering.
 * Works on modern systems where /dev/fb* may not be available.
 *
 * Requires: libdrm-dev
 */

#include "sgdrm.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/ioctl.h>

#include <linux/types.h>
#include <libdrm/drm.h>
#include <xf86drm.h>
#include <xf86drmMode.h>

#define DEFAULT_DRM_DEVICE "/dev/dri/card0"

struct sg_drm_context {
    int          fd;
    char        *device_path;

    /* Connector / CRTC state */
    uint32_t     connector_id;
    uint32_t     crtc_id;
    drmModeCrtc *saved_crtc;

    /* Dumb buffer */
    uint32_t     fb_id;
    uint32_t     handle;
    uint32_t     stride;
    uint32_t     size;
    int          width;
    int          height;
    pixel_t     *data;
};

/* ── helpers ─────────────────────────────────────────────── */

/* Find the first connected connector with a valid mode. */
static drmModeConnector *find_connector(int fd, drmModeRes *res) {
    for (int i = 0; i < res->count_connectors; i++) {
        drmModeConnector *conn = drmModeGetConnector(fd, res->connectors[i]);
        if (conn && conn->connection == DRM_MODE_CONNECTED && conn->count_modes > 0) {
            return conn;
        }
        drmModeFreeConnector(conn);
    }
    return NULL;
}

/* Find a CRTC that can drive the given connector. */
static uint32_t find_crtc(int fd, drmModeRes *res, drmModeConnector *conn) {
    drmModeEncoder *enc = NULL;

    /* Try the encoder already attached to the connector */
    if (conn->encoder_id) {
        enc = drmModeGetEncoder(fd, conn->encoder_id);
        if (enc) {
            uint32_t crtc_id = enc->crtc_id;
            drmModeFreeEncoder(enc);
            if (crtc_id) return crtc_id;
        }
    }

    /* Walk all encoders to find a compatible CRTC */
    for (int i = 0; i < conn->count_encoders; i++) {
        enc = drmModeGetEncoder(fd, conn->encoders[i]);
        if (!enc) continue;

        for (int j = 0; j < res->count_crtcs; j++) {
            if (enc->possible_crtcs & (1u << j)) {
                uint32_t crtc_id = res->crtcs[j];
                drmModeFreeEncoder(enc);
                return crtc_id;
            }
        }
        drmModeFreeEncoder(enc);
    }

    return 0;
}

/* ── public API ──────────────────────────────────────────── */

sg_drm_context_t *drm_context_create(const char *device) {
    if (!device) device = DEFAULT_DRM_DEVICE;

    int fd = open(device, O_RDWR | O_CLOEXEC);
    if (fd < 0) {
        fprintf(stderr, "sgutils: unable to open DRM device %s: %s\n",
                device, strerror(errno));
        return NULL;
    }

    /* Check the device supports dumb buffers */
    uint64_t has_dumb = 0;
    if (drmGetCap(fd, DRM_CAP_DUMB_BUFFER, &has_dumb) < 0 || !has_dumb) {
        fprintf(stderr, "sgutils: DRM device %s does not support dumb buffers\n", device);
        close(fd);
        return NULL;
    }

    drmModeRes *res = drmModeGetResources(fd);
    if (!res) {
        fprintf(stderr, "sgutils: cannot get DRM resources on %s: %s\n",
                device, strerror(errno));
        close(fd);
        return NULL;
    }

    drmModeConnector *conn = find_connector(fd, res);
    if (!conn) {
        fprintf(stderr, "sgutils: no connected display found on %s\n", device);
        drmModeFreeResources(res);
        close(fd);
        return NULL;
    }

    uint32_t crtc_id = find_crtc(fd, res, conn);
    if (!crtc_id) {
        fprintf(stderr, "sgutils: no suitable CRTC found for connector %u\n",
                conn->connector_id);
        drmModeFreeConnector(conn);
        drmModeFreeResources(res);
        close(fd);
        return NULL;
    }

    /* Use the preferred mode (first mode) */
    drmModeModeInfo mode = conn->modes[0];

    /* Create a dumb buffer */
    struct drm_mode_create_dumb creq = {0};
    creq.width  = mode.hdisplay;
    creq.height = mode.vdisplay;
    creq.bpp    = 32;

    if (drmIoctl(fd, DRM_IOCTL_MODE_CREATE_DUMB, &creq) < 0) {
        fprintf(stderr, "sgutils: cannot create dumb buffer: %s\n", strerror(errno));
        drmModeFreeConnector(conn);
        drmModeFreeResources(res);
        close(fd);
        return NULL;
    }

    /* Create a framebuffer object for the dumb buffer */
    uint32_t fb_id = 0;
    if (drmModeAddFB(fd, mode.hdisplay, mode.vdisplay, 24, 32,
                     creq.pitch, creq.handle, &fb_id) != 0) {
        fprintf(stderr, "sgutils: cannot create DRM framebuffer: %s\n", strerror(errno));
        struct drm_mode_destroy_dumb dreq = { .handle = creq.handle };
        drmIoctl(fd, DRM_IOCTL_MODE_DESTROY_DUMB, &dreq);
        drmModeFreeConnector(conn);
        drmModeFreeResources(res);
        close(fd);
        return NULL;
    }

    /* Map the dumb buffer */
    struct drm_mode_map_dumb mreq = {0};
    mreq.handle = creq.handle;
    if (drmIoctl(fd, DRM_IOCTL_MODE_MAP_DUMB, &mreq) < 0) {
        fprintf(stderr, "sgutils: cannot map dumb buffer: %s\n", strerror(errno));
        drmModeRmFB(fd, fb_id);
        struct drm_mode_destroy_dumb dreq = { .handle = creq.handle };
        drmIoctl(fd, DRM_IOCTL_MODE_DESTROY_DUMB, &dreq);
        drmModeFreeConnector(conn);
        drmModeFreeResources(res);
        close(fd);
        return NULL;
    }

    void *mapped = mmap(NULL, (size_t)creq.size, PROT_READ | PROT_WRITE,
                        MAP_SHARED, fd, (off_t)mreq.offset);
    if (mapped == MAP_FAILED) {
        fprintf(stderr, "sgutils: mmap failed: %s\n", strerror(errno));
        drmModeRmFB(fd, fb_id);
        struct drm_mode_destroy_dumb dreq = { .handle = creq.handle };
        drmIoctl(fd, DRM_IOCTL_MODE_DESTROY_DUMB, &dreq);
        drmModeFreeConnector(conn);
        drmModeFreeResources(res);
        close(fd);
        return NULL;
    }

    memset(mapped, 0, creq.size);

    /* Save the current CRTC so we can restore it later */
    drmModeCrtc *saved = drmModeGetCrtc(fd, crtc_id);

    /* Set the CRTC to display our buffer */
    if (drmModeSetCrtc(fd, crtc_id, fb_id, 0, 0,
                       &conn->connector_id, 1, &mode) != 0) {
        fprintf(stderr, "sgutils: cannot set CRTC: %s\n", strerror(errno));
        if (saved) drmModeFreeCrtc(saved);
        munmap(mapped, (size_t)creq.size);
        drmModeRmFB(fd, fb_id);
        struct drm_mode_destroy_dumb dreq = { .handle = creq.handle };
        drmIoctl(fd, DRM_IOCTL_MODE_DESTROY_DUMB, &dreq);
        drmModeFreeConnector(conn);
        drmModeFreeResources(res);
        close(fd);
        return NULL;
    }

    /* Build context */
    sg_drm_context_t *ctx = malloc(sizeof(sg_drm_context_t));
    if (!ctx) {
        if (saved) {
            drmModeSetCrtc(fd, saved->crtc_id, saved->buffer_id,
                           saved->x, saved->y, &conn->connector_id, 1,
                           &saved->mode);
            drmModeFreeCrtc(saved);
        }
        munmap(mapped, (size_t)creq.size);
        drmModeRmFB(fd, fb_id);
        struct drm_mode_destroy_dumb dreq2 = { .handle = creq.handle };
        drmIoctl(fd, DRM_IOCTL_MODE_DESTROY_DUMB, &dreq2);
        drmModeFreeConnector(conn);
        drmModeFreeResources(res);
        close(fd);
        return NULL;
    }

    ctx->fd           = fd;
    ctx->device_path  = strdup(device);
    ctx->connector_id = conn->connector_id;
    ctx->crtc_id      = crtc_id;
    ctx->saved_crtc   = saved;
    ctx->fb_id        = fb_id;
    ctx->handle       = creq.handle;
    ctx->stride       = creq.pitch;
    ctx->size         = creq.size;
    ctx->width        = (int)mode.hdisplay;
    ctx->height       = (int)mode.vdisplay;
    ctx->data         = (pixel_t *)mapped;

    drmModeFreeConnector(conn);
    drmModeFreeResources(res);

    return ctx;
}

int drm_get_mode_info(const char *device, drm_mode_info_t *info, int info_count) {
    if (!device) device = DEFAULT_DRM_DEVICE;

    int fd = open(device, O_RDONLY | O_CLOEXEC);
    if (fd < 0) {
        fprintf(stderr, "sgutils: unable to open DRM device %s: %s\n",
                device, strerror(errno));
        return -1;
    }

    drmModeRes *res = drmModeGetResources(fd);
    if (!res) {
        fprintf(stderr, "sgutils: cannot get DRM resources on %s: %s\n",
                device, strerror(errno));
        close(fd);
        return -1;
    }

    /* Get driver name */
    char driver_name[64] = {0};
    drmVersion *ver = drmGetVersion(fd);
    if (ver) {
        snprintf(driver_name, sizeof(driver_name), "%s", ver->name);
        drmFreeVersion(ver);
    }

    int found = 0;
    for (int i = 0; i < res->count_connectors && found < info_count; i++) {
        drmModeConnector *conn = drmModeGetConnector(fd, res->connectors[i]);
        if (!conn) continue;

        if (conn->connection == DRM_MODE_CONNECTED && conn->count_modes > 0) {
            drm_mode_info_t *mi = &info[found];
            memset(mi, 0, sizeof(*mi));

            /* Connector name: type + type_id */
            static const char *const conn_types[] = {
                "Unknown", "VGA", "DVII", "DVID", "DVIA", "Composite",
                "SVIDEO", "LVDS", "Component", "9PinDIN", "DisplayPort",
                "HDMI-A", "HDMI-B", "TV", "eDP", "VIRTUAL", "DSI", "DPI",
                "Writeback", "SPI", "USB"
            };
            int type_idx = conn->connector_type;
            if (type_idx < 0 || type_idx > 20) type_idx = 0;
            snprintf(mi->connector_name, sizeof(mi->connector_name),
                     "%s-%u", conn_types[type_idx], conn->connector_type_id);

            mi->connector_id = conn->connector_id;

            /* Find CRTC for this connector */
            mi->crtc_id = find_crtc(fd, res, conn);

            /* Preferred mode */
            mi->width   = (int)conn->modes[0].hdisplay;
            mi->height  = (int)conn->modes[0].vdisplay;
            mi->refresh = (int)conn->modes[0].vrefresh;
            mi->bpp     = 32;

            strncpy(mi->driver_name, driver_name, sizeof(mi->driver_name) - 1);

            found++;
        }

        drmModeFreeConnector(conn);
    }

    drmModeFreeResources(res);
    close(fd);
    return found;
}

int drm_context_width(const sg_drm_context_t *ctx) {
    return ctx ? ctx->width : 0;
}

int drm_context_height(const sg_drm_context_t *ctx) {
    return ctx ? ctx->height : 0;
}

image_t drm_context_as_image(sg_drm_context_t *ctx) {
    image_t img = {0};
    if (ctx) {
        img.data   = ctx->data;
        img.width  = ctx->width;
        img.height = ctx->height;
    }
    return img;
}

void drm_context_present(sg_drm_context_t *ctx) {
    /* With a single dumb buffer that's already set on the CRTC,
     * the pixels are visible immediately. Nothing to flip. */
    (void)ctx;
}

void drm_context_release(sg_drm_context_t *ctx) {
    if (!ctx) return;

    /* Restore the saved CRTC */
    if (ctx->saved_crtc) {
        drmModeSetCrtc(ctx->fd, ctx->saved_crtc->crtc_id,
                       ctx->saved_crtc->buffer_id,
                       ctx->saved_crtc->x, ctx->saved_crtc->y,
                       &ctx->connector_id, 1,
                       &ctx->saved_crtc->mode);
        drmModeFreeCrtc(ctx->saved_crtc);
    }

    /* Unmap buffer */
    if (ctx->data && ctx->size > 0) {
        munmap(ctx->data, (size_t)ctx->size);
    }

    /* Remove framebuffer and destroy dumb buffer */
    if (ctx->fb_id) {
        drmModeRmFB(ctx->fd, ctx->fb_id);
    }
    if (ctx->handle) {
        struct drm_mode_destroy_dumb dreq = { .handle = ctx->handle };
        drmIoctl(ctx->fd, DRM_IOCTL_MODE_DESTROY_DUMB, &dreq);
    }

    if (ctx->fd >= 0) {
        close(ctx->fd);
    }

    free(ctx->device_path);
    free(ctx);
}

const char *drm_context_device(const sg_drm_context_t *ctx) {
    return ctx ? ctx->device_path : NULL;
}
