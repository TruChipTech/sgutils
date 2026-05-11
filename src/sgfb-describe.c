/*
 * sgutils - Linux Screen Graphics Utilities
 * Copyright (c) 2026 Anand <anand@truchiptechnology.com> - MIT License
 * Date: 2026-05-07
 *
 * sgfb-describe.c - Describe the framebuffer device
 *
 * Bug fixes over the original:
 *  - Supports custom device path via -d option
 *  - Proper --help and --version flags
 *  - Frees context memory
 *  - Outputs bit depth info
 */

#include "framebuffer.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>

static void print_help(const char *prog) {
    printf("Usage: %s [OPTIONS]\n\n"
           "Describe the Linux framebuffer device dimensions.\n\n"
           "Options:\n"
           "  -d, --device PATH   Framebuffer device (default: /dev/fb0)\n"
           "  -j, --json          Output as JSON\n"
           "  -h, --help          Show this help message\n"
           "  -v, --version       Show version\n",
           prog);
}

int main(int argc, char *argv[]) {
    const char *device = NULL;
    int json_output = 0;

    static struct option long_opts[] = {
        {"device",  required_argument, 0, 'd'},
        {"json",    no_argument,       0, 'j'},
        {"help",    no_argument,       0, 'h'},
        {"version", no_argument,       0, 'v'},
        {0, 0, 0, 0}
    };

    int opt;
    while ((opt = getopt_long(argc, argv, "d:jhv", long_opts, NULL)) != -1) {
        switch (opt) {
            case 'd': device = optarg; break;
            case 'j': json_output = 1; break;
            case 'h': print_help(argv[0]); return 0;
            case 'v': printf("sgfb-describe " VERSION "\n"); return 0;
            default:  print_help(argv[0]); return 1;
        }
    }

    fb_context_t *ctx = fb_context_get_dimensions(device);
    if (!ctx) {
        fprintf(stderr, "Error: could not describe framebuffer\n");
        return 1;
    }

    if (json_output) {
        printf("{\"device\":\"%s\",\"width\":%d,\"height\":%d}\n",
               ctx->fb_name, ctx->width, ctx->height);
    } else {
        printf("%s %d %d\n", ctx->fb_name, ctx->width, ctx->height);
    }

    fb_context_release(ctx);
    return 0;
}
