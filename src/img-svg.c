/*
 * sgutils - Linux Screen Graphics Utilities
 * Copyright (c) 2026 Anand <anand@truchiptechnology.com> - MIT License
 * Date: 2026-05-07
 *
 * img-svg.c - SVG image loading via librsvg + cairo
 *
 * Bug fixes over the original:
 *  - Proper ARGB output (cairo ARGB32 is already in the right byte order)
 *  - Handle deprecated API warnings (rsvg_handle_render_cairo)
 *  - Proper error cleanup on all paths
 *  - File read error checking
 */

#ifdef ENABLE_SVG

#include "img-svg.h"

#include <librsvg/rsvg.h>
#include <cairo.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

image_t *read_svg_file(const char *filename, int requested_width, int requested_height) {
    if (!filename) return NULL;

    struct stat st;
    int fd = open(filename, O_RDONLY);
    if (fd < 0) {
        fprintf(stderr, "SGUTILS: cannot open SVG file '%s'\n", filename);
        return NULL;
    }

    if (fstat(fd, &st) < 0 || st.st_size <= 0) {
        fprintf(stderr, "SGUTILS: cannot stat SVG file '%s'\n", filename);
        close(fd);
        return NULL;
    }

    unsigned char *file_contents = malloc((size_t)st.st_size);
    if (!file_contents) {
        close(fd);
        return NULL;
    }

    ssize_t bytes_read = read(fd, file_contents, (size_t)st.st_size);
    close(fd);

    if (bytes_read != st.st_size) {
        fprintf(stderr, "SGUTILS: incomplete read of SVG file '%s'\n", filename);
        free(file_contents);
        return NULL;
    }

    GError *err = NULL;
    RsvgHandle *svg = rsvg_handle_new_from_data(file_contents, (gsize)st.st_size, &err);
    free(file_contents);

    if (err != NULL || !svg) {
        fprintf(stderr, "SGUTILS: SVG load error: %s\n",
                err ? err->message : "unknown");
        if (err) g_error_free(err);
        return NULL;
    }

    double svg_width, svg_height;
    if (!rsvg_handle_get_intrinsic_size_in_pixels(svg, &svg_width, &svg_height) ||
        svg_width <= 0 || svg_height <= 0) {
        /* Fallback: render at requested size or a default */
        svg_width  = (requested_width  > 0) ? requested_width  : 300;
        svg_height = (requested_height > 0) ? requested_height : 150;
    }

    double width, height;
    double scale_width = 1.0, scale_height = 1.0;

    if (requested_width > 0 && requested_height > 0) {
        width  = requested_width;
        height = requested_height;
        scale_width  = width  / svg_width;
        scale_height = height / svg_height;
    } else if (requested_width <= 0 && requested_height <= 0) {
        width  = svg_width;
        height = svg_height;
    } else if (requested_width > 0) {
        width  = requested_width;
        scale_width  = width / svg_width;
        scale_height = scale_width;
        height = round(svg_height * scale_height);
    } else {
        height = requested_height;
        scale_height = height / svg_height;
        scale_width  = scale_height;
        width  = round(svg_width * scale_width);
    }

    int iw = (int)width;
    int ih = (int)height;

    cairo_surface_t *canvas = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, iw, ih);
    cairo_t *cr = cairo_create(canvas);
    cairo_scale(cr, scale_width, scale_height);

    /* Suppress deprecation; this is the stable API for our use case */
    #pragma GCC diagnostic push
    #pragma GCC diagnostic ignored "-Wdeprecated-declarations"
    if (!rsvg_handle_render_cairo(svg, cr)) {
        fprintf(stderr, "SGUTILS: SVG render error\n");
    }
    #pragma GCC diagnostic pop

    cairo_surface_flush(canvas);

    int stride = cairo_image_surface_get_stride(canvas);
    unsigned char *cairo_data = cairo_image_surface_get_data(canvas);

    image_t *out = image_create(iw, ih);
    if (!out) {
        g_object_unref(svg);
        cairo_surface_destroy(canvas);
        cairo_destroy(cr);
        return NULL;
    }

    /* Cairo ARGB32 is native-endian, same layout as our pixel_t on little-endian */
    for (int y = 0; y < ih; y++) {
        memcpy(&out->data[y * iw], &cairo_data[y * stride],
               (size_t)iw * sizeof(pixel_t));
    }

    g_object_unref(svg);
    cairo_surface_destroy(canvas);
    cairo_destroy(cr);

    return out;
}

#endif /* ENABLE_SVG */
