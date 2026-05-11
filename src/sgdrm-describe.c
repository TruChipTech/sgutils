/*
 * sgutils - Linux Screen Graphics Utilities
 * Copyright (c) 2026 Anand <anand@truchiptechnology.com> - MIT License
 * Date: 2026-05-07
 *
 * sgdrm-describe.c - Describe DRM/KMS display outputs
 *
 * Lists connected displays, their resolutions, refresh rates,
 * driver name, and connector type.
 */

#include "sgdrm.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>

#ifndef VERSION
#define VERSION "2.0.0"
#endif

static void print_help(const char *prog) {
    printf("Usage: %s [OPTIONS]\n\n"
           "Describe DRM/KMS display outputs.\n\n"
           "Options:\n"
           "  -d, --device PATH   DRM device (default: /dev/dri/card0)\n"
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
            case 'v': printf("sgdrm-describe " VERSION "\n"); return 0;
            default:  print_help(argv[0]); return 1;
        }
    }

    drm_mode_info_t info[16];
    int count = drm_get_mode_info(device, info, 16);
    if (count < 0) {
        fprintf(stderr, "Error: could not query DRM device\n");
        return 1;
    }

    if (count == 0) {
        fprintf(stderr, "No connected displays found\n");
        return 1;
    }

    if (json_output) {
        printf("[");
        for (int i = 0; i < count; i++) {
            if (i > 0) printf(",");
            printf("{\"connector\":\"%s\",\"width\":%d,\"height\":%d,"
                   "\"refresh\":%d,\"driver\":\"%s\"}",
                   info[i].connector_name, info[i].width, info[i].height,
                   info[i].refresh, info[i].driver_name);
        }
        printf("]\n");
    } else {
        for (int i = 0; i < count; i++) {
            printf("%s %dx%d@%dHz (driver: %s)\n",
                   info[i].connector_name,
                   info[i].width, info[i].height,
                   info[i].refresh, info[i].driver_name);
        }
    }

    return 0;
}
