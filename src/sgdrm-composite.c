/*
 * sgutils - Linux Screen Graphics Utilities
 * Copyright (c) 2026 Anand <anand@truchiptechnology.com> - MIT License
 * Date: 2026-05-07
 *
 * sgdrm-composite.c - Composite multiple images on DRM display
 *
 * Each layer is specified as: -l FILE:X:Y[:OPACITY]
 * Layers are rendered bottom-to-top with alpha blending.
 */

#include "sgdrm.h"
#include "draw.h"
#include "compositing.h"

#ifdef ENABLE_PNG
#include "img-png.h"
#endif
#ifdef ENABLE_JPEG
#include "img-jpeg.h"
#endif
#ifdef ENABLE_SVG
#include "img-svg.h"
#endif
#ifdef ENABLE_BMP
#include "img-bmp.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>

#ifndef VERSION
#define VERSION "2.0.0"
#endif

static const char *detect_ext(const char *filename) {
    const char *dot = strrchr(filename, '.');
    if (!dot) return NULL;
    if (strcasecmp(dot, ".png") == 0)  return "png";
    if (strcasecmp(dot, ".jpg") == 0)  return "jpeg";
    if (strcasecmp(dot, ".jpeg") == 0) return "jpeg";
    if (strcasecmp(dot, ".svg") == 0)  return "svg";
    if (strcasecmp(dot, ".bmp") == 0)  return "bmp";
    return NULL;
}

static image_t *load_image(const char *path, int w, int h) {
    const char *fmt = detect_ext(path);
    if (!fmt) {
        fprintf(stderr, "Error: unknown image format for '%s'\n", path);
        return NULL;
    }

#ifdef ENABLE_PNG
    if (strcmp(fmt, "png") == 0) return read_png_file(path);
#endif
#ifdef ENABLE_JPEG
    if (strcmp(fmt, "jpeg") == 0) return read_jpeg_file(path);
#endif
#ifdef ENABLE_SVG
    if (strcmp(fmt, "svg") == 0) return read_svg_file(path, w, h);
#endif
#ifdef ENABLE_BMP
    if (strcmp(fmt, "bmp") == 0) return read_bmp_file(path);
#endif

    (void)w; (void)h;
    fprintf(stderr, "Error: format '%s' not compiled in\n", fmt);
    return NULL;
}

static int parse_layer(const char *spec, char *path, size_t path_size,
                        int *x, int *y, int *opacity) {
    *x = 0;
    *y = 0;
    *opacity = 255;

    char *buf = strdup(spec);
    if (!buf) return -1;

    char *tokens[5];
    int ntokens = 0;
    char *tok = strtok(buf, ":");
    while (tok && ntokens < 5) {
        tokens[ntokens++] = tok;
        tok = strtok(NULL, ":");
    }

    if (ntokens < 3) {
        fprintf(stderr, "Error: layer format is FILE:X:Y[:OPACITY]\n");
        free(buf);
        return -1;
    }

    strncpy(path, tokens[0], path_size - 1);
    path[path_size - 1] = '\0';

    *x = atoi(tokens[1]);
    *y = atoi(tokens[2]);

    if (ntokens >= 4) {
        *opacity = atoi(tokens[3]);
        if (*opacity < 0) *opacity = 0;
        if (*opacity > 255) *opacity = 255;
    }

    free(buf);
    return 0;
}

static void print_help(const char *prog) {
    printf("Usage: %s -l FILE:X:Y[:OPACITY] [-l ...] [OPTIONS]\n\n"
           "Composite multiple images onto a DRM/KMS display.\n\n"
           "Layers are rendered bottom-to-top with alpha blending.\n\n"
           "Options:\n"
           "  -l, --layer SPEC         Layer specification FILE:X:Y[:OPACITY]\n"
           "                           OPACITY is 0-255 (default: 255)\n"
           "  -d, --device PATH        DRM device (default: /dev/dri/card0)\n"
           "  -c, --clear RRGGBB       Clear color before compositing\n"
           "  -h, --help               Show this help message\n"
           "  -v, --version            Show version\n",
           prog);
}

#define MAX_LAYERS 64

int main(int argc, char *argv[]) {
    const char *device = NULL;
    int use_clear = 0;
    unsigned int clear_hex = 0;

    const char *layer_specs[MAX_LAYERS];
    int layer_count = 0;

    static struct option long_opts[] = {
        {"layer",   required_argument, 0, 'l'},
        {"device",  required_argument, 0, 'd'},
        {"clear",   required_argument, 0, 'c'},
        {"help",    no_argument,       0, 'h'},
        {"version", no_argument,       0, 'v'},
        {0, 0, 0, 0}
    };

    int opt;
    while ((opt = getopt_long(argc, argv, "l:d:c:hv", long_opts, NULL)) != -1) {
        switch (opt) {
            case 'l':
                if (layer_count >= MAX_LAYERS) {
                    fprintf(stderr, "Error: too many layers (max %d)\n", MAX_LAYERS);
                    return 1;
                }
                layer_specs[layer_count++] = optarg;
                break;
            case 'd': device = optarg; break;
            case 'c':
                use_clear = 1;
                clear_hex = (unsigned int)strtoul(optarg, NULL, 16);
                break;
            case 'h': print_help(argv[0]); return 0;
            case 'v': printf("sgdrm-composite " VERSION "\n"); return 0;
            default:  print_help(argv[0]); return 1;
        }
    }

    if (layer_count == 0) {
        fprintf(stderr, "Error: at least one -l/--layer is required\n\n");
        print_help(argv[0]);
        return 1;
    }

    sg_drm_context_t *ctx = drm_context_create(device);
    if (!ctx) return 1;

    int scr_w = drm_context_width(ctx);
    int scr_h = drm_context_height(ctx);

    compositor_t *comp = compositor_create(scr_w, scr_h);
    if (!comp) {
        drm_context_release(ctx);
        return 1;
    }

    for (int i = 0; i < layer_count; i++) {
        char path[1024];
        int lx, ly, opacity;

        if (parse_layer(layer_specs[i], path, sizeof(path),
                         &lx, &ly, &opacity) != 0) {
            compositor_free(comp);
            drm_context_release(ctx);
            return 1;
        }

        image_t *img = load_image(path, scr_w, scr_h);
        if (!img) {
            fprintf(stderr, "Warning: skipping layer '%s'\n", path);
            continue;
        }

        compositor_add_layer(comp, img, lx, ly, (uint8_t)opacity);
    }

    image_t fb = drm_context_as_image(ctx);

    if (use_clear) {
        fill_image(&fb, PIXEL_RGB((clear_hex >> 16) & 0xFF,
                                  (clear_hex >> 8) & 0xFF,
                                  clear_hex & 0xFF));
    } else {
        clear_image(&fb);
    }

    compositor_render(comp, &fb);
    drm_context_present(ctx);

    compositor_free(comp);
    drm_context_release(ctx);
    return 0;
}
