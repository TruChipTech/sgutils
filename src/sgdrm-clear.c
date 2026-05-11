/*
 * sgutils - Linux Screen Graphics Utilities
 * Copyright (c) 2026 Anand <anand@truchiptechnology.com> - MIT License
 * Date: 2026-05-07
 *
 * sgdrm-clear.c - Clear DRM display to a solid color
 */

#include "sgdrm.h"
#include "draw.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>

#ifndef VERSION
#define VERSION "2.0.0"
#endif

static void print_help(const char *prog) {
    printf("Usage: %s [OPTIONS]\n\n"
           "Clear the DRM display to a solid color.\n\n"
           "Options:\n"
           "  -d, --device PATH        DRM device (default: /dev/dri/card0)\n"
           "  -g, --gray VALUE         Grayscale value 0-255 (0=black, 255=white)\n"
           "  -c, --color RRGGBB       Hex color (e.g., FF0000 for red)\n"
           "  -t, --test-pattern       Display a color test pattern\n"
           "  -h, --help               Show this help message\n"
           "  -v, --version            Show version\n",
           prog);
}

int main(int argc, char *argv[]) {
    const char *device = NULL;
    int use_gray   = 0;
    int gray_value = 0;
    int use_color  = 0;
    unsigned int hex_color = 0;
    int test = 0;

    static struct option long_opts[] = {
        {"device",       required_argument, 0, 'd'},
        {"gray",         required_argument, 0, 'g'},
        {"color",        required_argument, 0, 'c'},
        {"test-pattern", no_argument,       0, 't'},
        {"help",         no_argument,       0, 'h'},
        {"version",      no_argument,       0, 'v'},
        {0, 0, 0, 0}
    };

    int opt;
    while ((opt = getopt_long(argc, argv, "d:g:c:thv", long_opts, NULL)) != -1) {
        switch (opt) {
            case 'd': device = optarg; break;
            case 'g':
                use_gray = 1;
                gray_value = atoi(optarg) & 0xFF;
                break;
            case 'c':
                use_color = 1;
                hex_color = (unsigned int)strtoul(optarg, NULL, 16);
                break;
            case 't': test = 1; break;
            case 'h': print_help(argv[0]); return 0;
            case 'v': printf("sgdrm-clear " VERSION "\n"); return 0;
            default:  print_help(argv[0]); return 1;
        }
    }

    sg_drm_context_t *ctx = drm_context_create(device);
    if (!ctx) return 1;

    image_t fb = drm_context_as_image(ctx);

    if (test) {
        test_pattern(&fb);
    } else if (use_color) {
        fill_image(&fb, PIXEL_RGB((hex_color >> 16) & 0xFF,
                                  (hex_color >> 8) & 0xFF,
                                  hex_color & 0xFF));
    } else if (use_gray) {
        fill_image(&fb, PIXEL_RGB(gray_value, gray_value, gray_value));
    } else {
        clear_image(&fb);
    }

    drm_context_present(ctx);
    drm_context_release(ctx);
    return 0;
}
