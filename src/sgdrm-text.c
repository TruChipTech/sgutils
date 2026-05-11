/*
 * sgutils - Linux Screen Graphics Utilities
 * Copyright (c) 2026 Anand <anand@truchiptechnology.com> - MIT License
 * Date: 2026-05-07
 *
 * sgdrm-text.c - Render text to a DRM/KMS display using FreeType fonts
 */

#ifdef ENABLE_TEXT

#include "sgdrm.h"
#include "draw.h"
#include "text.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>

#ifndef VERSION
#define VERSION "2.0.0"
#endif

static void print_help(const char *prog) {
    printf("Usage: %s -t TEXT -F FONT [OPTIONS]\n\n"
           "Render text onto a DRM/KMS display using a TrueType font.\n\n"
           "Required:\n"
           "  -t, --text STRING        Text to render\n"
           "  -F, --font PATH          Path to a .ttf or .otf font file\n\n"
           "Options:\n"
           "  -d, --device PATH        DRM device (default: /dev/dri/card0)\n"
           "  -s, --size N             Font size in pixels (default: 32)\n"
           "  -x, --xpos N             X position (default: 0)\n"
           "  -y, --ypos N             Y position (default: font size)\n"
           "  -c, --color RRGGBB       Text color in hex (default: FFFFFF)\n"
           "  -b, --bg-color RRGGBB    Background fill color (optional)\n"
           "  -h, --help               Show this help message\n"
           "  -v, --version            Show version\n",
           prog);
}

int main(int argc, char *argv[]) {
    const char *device    = NULL;
    const char *text      = NULL;
    const char *font_path = NULL;
    int font_size   = 32;
    int pos_x       = 0;
    int pos_y       = -1;
    unsigned int fg_hex = 0xFFFFFF;
    int use_bg      = 0;
    unsigned int bg_hex = 0;

    static struct option long_opts[] = {
        {"text",     required_argument, 0, 't'},
        {"font",     required_argument, 0, 'F'},
        {"device",   required_argument, 0, 'd'},
        {"size",     required_argument, 0, 's'},
        {"xpos",     required_argument, 0, 'x'},
        {"ypos",     required_argument, 0, 'y'},
        {"color",    required_argument, 0, 'c'},
        {"bg-color", required_argument, 0, 'b'},
        {"help",     no_argument,       0, 'h'},
        {"version",  no_argument,       0, 'v'},
        {0, 0, 0, 0}
    };

    int opt;
    while ((opt = getopt_long(argc, argv, "t:F:d:s:x:y:c:b:hv",
                               long_opts, NULL)) != -1) {
        switch (opt) {
            case 't': text = optarg; break;
            case 'F': font_path = optarg; break;
            case 'd': device = optarg; break;
            case 's': font_size = atoi(optarg); break;
            case 'x': pos_x = atoi(optarg); break;
            case 'y': pos_y = atoi(optarg); break;
            case 'c': fg_hex = (unsigned int)strtoul(optarg, NULL, 16); break;
            case 'b':
                use_bg = 1;
                bg_hex = (unsigned int)strtoul(optarg, NULL, 16);
                break;
            case 'h': print_help(argv[0]); return 0;
            case 'v': printf("sgdrm-text " VERSION "\n"); return 0;
            default:  print_help(argv[0]); return 1;
        }
    }

    if (!text || !font_path) {
        fprintf(stderr, "Error: --text and --font are required\n\n");
        print_help(argv[0]);
        return 1;
    }

    if (pos_y < 0) pos_y = font_size;

    if (text_init() != 0) return 1;

    fb_font_t *font = font_load(font_path, font_size);
    if (!font) {
        text_shutdown();
        return 1;
    }

    sg_drm_context_t *ctx = drm_context_create(device);
    if (!ctx) {
        font_free(font);
        text_shutdown();
        return 1;
    }

    image_t fb = drm_context_as_image(ctx);

    if (use_bg) {
        fill_image(&fb, PIXEL_RGB((bg_hex >> 16) & 0xFF,
                                  (bg_hex >> 8) & 0xFF,
                                  bg_hex & 0xFF));
    }

    pixel_t fg = PIXEL_RGB((fg_hex >> 16) & 0xFF,
                           (fg_hex >> 8) & 0xFF,
                           fg_hex & 0xFF);

    draw_text(&fb, pos_x, pos_y, text, font, fg);

    drm_context_present(ctx);

    font_free(font);
    drm_context_release(ctx);
    text_shutdown();

    return 0;
}

#else /* !ENABLE_TEXT */

#include <stdio.h>
int main(void) {
    fprintf(stderr, "sgdrm-text: FreeType support not compiled. "
                    "Rebuild with ENABLE_TEXT=1\n");
    return 1;
}

#endif
