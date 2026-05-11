/*
 * sgutils - Linux Screen Graphics Utilities
 * Copyright (c) 2026 Anand <anand@truchiptechnology.com> - MIT License
 * Date: 2026-05-07
 *
 * framebuffer.c - Linux framebuffer device access
 *
 * Bug fixes over the original:
 *  - Configurable device path (not hardcoded /dev/fb0)
 *  - Proper error messages with errno
 *  - fb_context_get_dimensions() properly closes fd
 *  - fb_size stored in context for proper munmap
 */

#include "framebuffer.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <linux/fb.h>

#define DEFAULT_FB_DEVICE "/dev/fb0"

fb_context_t *fb_context_create(const char *device) {
    if (!device) device = DEFAULT_FB_DEVICE;

    struct fb_fix_screeninfo fixinfo;
    struct fb_var_screeninfo varinfo;

    int fd = open(device, O_RDWR);
    if (fd < 0) {
        fprintf(stderr, "SGUTILS: unable to open %s: %s\n", device, strerror(errno));
        return NULL;
    }

    if (ioctl(fd, FBIOGET_FSCREENINFO, &fixinfo) < 0) {
        fprintf(stderr, "SGUTILS: FBIOGET_FSCREENINFO failed on %s: %s\n",
                device, strerror(errno));
        close(fd);
        return NULL;
    }

    if (ioctl(fd, FBIOGET_VSCREENINFO, &varinfo) < 0) {
        fprintf(stderr, "SGUTILS: FBIOGET_VSCREENINFO failed on %s: %s\n",
                device, strerror(errno));
        close(fd);
        return NULL;
    }

    int fb_size = (int)(fixinfo.line_length * varinfo.yres);

    void *mapped = mmap(NULL, (size_t)fb_size, PROT_READ | PROT_WRITE,
                        MAP_SHARED, fd, 0);
    if (mapped == MAP_FAILED) {
        fprintf(stderr, "SGUTILS: mmap failed on %s: %s\n", device, strerror(errno));
        close(fd);
        return NULL;
    }

    fb_context_t *ctx = malloc(sizeof(fb_context_t));
    if (!ctx) {
        munmap(mapped, (size_t)fb_size);
        close(fd);
        return NULL;
    }

    ctx->data         = (pixel_t *)mapped;
    ctx->width        = (int)(fixinfo.line_length / 4);
    ctx->height       = (int)varinfo.yres;
    ctx->fb_file_desc = fd;
    ctx->fb_size      = fb_size;
    ctx->fb_name      = strdup(device);

    return ctx;
}

fb_context_t *fb_context_get_dimensions(const char *device) {
    if (!device) device = DEFAULT_FB_DEVICE;

    struct fb_fix_screeninfo fixinfo;
    struct fb_var_screeninfo varinfo;

    int fd = open(device, O_RDONLY);
    if (fd < 0) {
        fprintf(stderr, "SGUTILS: unable to open %s: %s\n", device, strerror(errno));
        return NULL;
    }

    if (ioctl(fd, FBIOGET_FSCREENINFO, &fixinfo) < 0) {
        fprintf(stderr, "SGUTILS: FBIOGET_FSCREENINFO failed: %s\n", strerror(errno));
        close(fd);
        return NULL;
    }

    if (ioctl(fd, FBIOGET_VSCREENINFO, &varinfo) < 0) {
        fprintf(stderr, "SGUTILS: FBIOGET_VSCREENINFO failed: %s\n", strerror(errno));
        close(fd);
        return NULL;
    }

    close(fd);

    fb_context_t *ctx = malloc(sizeof(fb_context_t));
    if (!ctx) return NULL;

    ctx->data         = NULL;
    ctx->width        = (int)(fixinfo.line_length / 4);
    ctx->height       = (int)varinfo.yres;
    ctx->fb_file_desc = -1;
    ctx->fb_size      = 0;
    ctx->fb_name      = strdup(device);

    return ctx;
}

void fb_context_release(fb_context_t *ctx) {
    if (!ctx) return;

    if (ctx->data && ctx->fb_size > 0) {
        munmap(ctx->data, (size_t)ctx->fb_size);
    }

    if (ctx->fb_file_desc >= 0) {
        close(ctx->fb_file_desc);
    }

    free(ctx->fb_name);
    free(ctx);
}

image_t fb_context_as_image(fb_context_t *ctx) {
    image_t img = {0};
    if (ctx) {
        img.data   = ctx->data;
        img.width  = ctx->width;
        img.height = ctx->height;
    }
    return img;
}
