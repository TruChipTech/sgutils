/*
 * sgutils - Linux Screen Graphics Utilities
 * Copyright (c) 2026 Anand <anand@truchiptechnology.com> - MIT License
 * Date: 2026-05-07
 *
 * sgfb-imgdraw.c - Unified image drawing tool for all formats
 *
 * This single source file builds into sgfb-pngdraw, sgfb-jpgdraw,
 * sgfb-svgdraw, and sgfb-bmpdraw depending on which format headers
 * are available. The binary name determines the default behavior,
 * but the -F flag can override the format.
 *
 * Bug fixes over the original:
 *  - height defaults to context->height (was incorrectly context->width)
 *  - scaled images are freed (memory leak fix)
 *  - NULL file path shows proper error and usage
 *  - Proper getopt_long argument parsing
 *  - Added --brightness and --rotate options
 */

#include "framebuffer.h"
#include "draw.h"

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
#include <libgen.h>

static const char *detect_format_from_name(const char *progname) {
    if (!progname) return "auto";
    if (strstr(progname, "pngdraw"))  return "png";
    if (strstr(progname, "jpgdraw"))  return "jpeg";
    if (strstr(progname, "jpegdraw")) return "jpeg";
    if (strstr(progname, "svgdraw"))  return "svg";
    if (strstr(progname, "bmpdraw"))  return "bmp";
    return "auto";
}

static const char *detect_format_from_ext(const char *filename) {
    if (!filename) return NULL;
    const char *dot = strrchr(filename, '.');
    if (!dot) return NULL;

    if (strcasecmp(dot, ".png") == 0)  return "png";
    if (strcasecmp(dot, ".jpg") == 0)  return "jpeg";
    if (strcasecmp(dot, ".jpeg") == 0) return "jpeg";
    if (strcasecmp(dot, ".svg") == 0)  return "svg";
    if (strcasecmp(dot, ".bmp") == 0)  return "bmp";
    return NULL;
}

static uint32_t parse_hue_mask(const char *str) {
    uint32_t mask = MASK_NONE;
    for (size_t i = 0; str[i]; i++) {
        switch (str[i]) {
            case 'r': mask |= MASK_RED;     break;
            case 'g': mask |= MASK_GREEN;   break;
            case 'b': mask |= MASK_BLUE;    break;
            case 'c': mask |= MASK_CYAN;    break;
            case 'y': mask |= MASK_YELLOW;  break;
            case 'm': mask |= MASK_MAGENTA; break;
            case 'w': mask |= MASK_WHITE;   break;
            default:
                fprintf(stderr, "Warning: unknown hue character '%c'\n", str[i]);
                break;
        }
    }
    return mask;
}

static void print_help(const char *prog) {
    printf("Usage: %s -f FILE [OPTIONS]\n\n"
           "Draw an image to the Linux framebuffer.\n\n"
           "Required:\n"
           "  -f, --file PATH          Path to the image file\n\n"
           "Options:\n"
           "  -d, --device PATH        Framebuffer device (default: /dev/fb0)\n"
           "  -x, --xpos N             X position (default: 0)\n"
           "  -y, --ypos N             Y position (default: 0)\n"
           "  -w, --width N            Display width (default: framebuffer width)\n"
           "  -H, --height N           Display height (default: framebuffer height)\n"
           "  -F, --format FMT         Force image format: png, jpeg, svg, bmp\n"
           "  -i, --invert             Invert colors\n"
           "  -G, --grayscale          Convert to grayscale\n"
           "  -u, --hue MASK           Hue shift (mask chars: r,g,b,c,y,m,w)\n"
           "  -T, --hue-threshold N    Hue threshold (default: 20)\n"
           "  -b, --brightness N       Adjust brightness (-255 to 255)\n"
           "  -r, --rotate DEG         Rotate image (90, 180, 270)\n"
           "  -a, --alpha              Use alpha blending when drawing\n"
           "  -h, --help               Show this help message\n"
           "  -v, --version            Show version\n",
           prog);
}

int main(int argc, char *argv[]) {
    const char *device  = NULL;
    const char *file    = NULL;
    const char *format  = NULL;
    int pos_x    = 0;
    int pos_y    = 0;
    int width    = -1; /* -1 means "use framebuffer width" */
    int height   = -1; /* FIXED: was initialized to context->width in original */
    int do_invert     = 0;
    int do_grayscale  = 0;
    int do_hue        = 0;
    int brightness    = 0;
    int rotation      = 0;
    int do_alpha      = 0;
    uint32_t hue_mask = MASK_NONE;
    int hue_threshold = HUE_THRESHOLD_DEFAULT;

    static struct option long_opts[] = {
        {"file",          required_argument, 0, 'f'},
        {"device",        required_argument, 0, 'd'},
        {"xpos",          required_argument, 0, 'x'},
        {"ypos",          required_argument, 0, 'y'},
        {"width",         required_argument, 0, 'w'},
        {"height",        required_argument, 0, 'H'},
        {"format",        required_argument, 0, 'F'},
        {"invert",        no_argument,       0, 'i'},
        {"grayscale",     no_argument,       0, 'G'},
        {"hue",           required_argument, 0, 'u'},
        {"hue-threshold", required_argument, 0, 'T'},
        {"brightness",    required_argument, 0, 'b'},
        {"rotate",        required_argument, 0, 'r'},
        {"alpha",         no_argument,       0, 'a'},
        {"help",          no_argument,       0, 'h'},
        {"version",       no_argument,       0, 'v'},
        {0, 0, 0, 0}
    };

    int opt;
    while ((opt = getopt_long(argc, argv, "f:d:x:y:w:H:F:iGu:T:b:r:ahv",
                               long_opts, NULL)) != -1) {
        switch (opt) {
            case 'f': file = optarg; break;
            case 'd': device = optarg; break;
            case 'x': pos_x = atoi(optarg); break;
            case 'y': pos_y = atoi(optarg); break;
            case 'w': width = atoi(optarg); break;
            case 'H': height = atoi(optarg); break;
            case 'F': format = optarg; break;
            case 'i': do_invert = 1; break;
            case 'G': do_grayscale = 1; break;
            case 'u':
                do_hue = 1;
                hue_mask = parse_hue_mask(optarg);
                break;
            case 'T': hue_threshold = atoi(optarg); break;
            case 'b': brightness = atoi(optarg); break;
            case 'r': rotation = atoi(optarg); break;
            case 'a': do_alpha = 1; break;
            case 'h': print_help(argv[0]); return 0;
            case 'v': printf("sgfb-imgdraw " VERSION "\n"); return 0;
            default:  print_help(argv[0]); return 1;
        }
    }

    if (!file) {
        fprintf(stderr, "Error: -f/--file is required\n\n");
        print_help(argv[0]);
        return 1;
    }

    /* Determine format */
    const char *fmt = format;
    if (!fmt) {
        fmt = detect_format_from_name(argv[0]);
        if (strcmp(fmt, "auto") == 0) {
            fmt = detect_format_from_ext(file);
            if (!fmt) {
                fprintf(stderr, "Error: cannot determine image format. Use -F.\n");
                return 1;
            }
        }
    }

    /* Open framebuffer */
    fb_context_t *ctx = fb_context_create(device);
    if (!ctx) return 1;

    if (width  < 0) width  = ctx->width;
    if (height < 0) height = ctx->height; /* FIXED: was ctx->width */

    /* Load image based on format */
    image_t *image = NULL;

#ifdef ENABLE_PNG
    if (strcmp(fmt, "png") == 0) {
        image = read_png_file(file);
    }
#endif
#ifdef ENABLE_JPEG
    if (strcmp(fmt, "jpeg") == 0) {
        image = read_jpeg_file(file);
    }
#endif
#ifdef ENABLE_SVG
    if (strcmp(fmt, "svg") == 0) {
        image = read_svg_file(file, width, height);
    }
#endif
#ifdef ENABLE_BMP
    if (strcmp(fmt, "bmp") == 0) {
        image = read_bmp_file(file);
    }
#endif

    if (!image) {
        fprintf(stderr, "Error: failed to load '%s' as %s\n", file, fmt);
        fb_context_release(ctx);
        return 1;
    }

    /* Apply transforms */
    if (do_invert)    image_invert(image);
    if (do_grayscale) image_grayscale(image);
    if (do_hue)       image_hueify(image, hue_mask, hue_threshold);
    if (brightness)   image_brightness(image, brightness);
    if (rotation)     image_rotate(image, rotation);

    /* Scale (SVG is pre-scaled) */
    image_t *scaled = NULL;
    if (strcmp(fmt, "svg") != 0) {
        scaled = image_scale(image, width, height);
        if (!scaled) {
            fprintf(stderr, "Error: failed to scale image\n");
            image_free(image);
            fb_context_release(ctx);
            return 1;
        }
    }

    image_t *to_draw = scaled ? scaled : image;

    /* Draw to framebuffer */
    image_t fb = fb_context_as_image(ctx);
    if (do_alpha) {
        draw_image_alpha(&fb, pos_x, pos_y, to_draw);
    } else {
        draw_image(&fb, pos_x, pos_y, to_draw);
    }

    /* FIXED: free scaled image (was leaked in original) */
    if (scaled) image_free(scaled);
    image_free(image);
    fb_context_release(ctx);

    return 0;
}
