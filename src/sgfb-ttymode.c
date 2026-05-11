/*
 * sgutils - Linux Screen Graphics Utilities
 * Copyright (c) 2026 Anand <anand@truchiptechnology.com> - MIT License
 * Date: 2026-05-07
 *
 * sgfb-ttymode.c - Set TTY mode (graphics or text)
 *
 * Bug fixes over the original:
 *  - Fixed argv[1] access when argc < 2 (segfault)
 *  - Supports custom tty path with -t option
 *  - Proper getopt argument parsing
 *  - Clear error messages
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <getopt.h>
#include <sys/ioctl.h>
#include <linux/kd.h>

#ifndef VERSION
#define VERSION "2.0.0"
#endif

static void print_help(const char *prog) {
    printf("Usage: %s [OPTIONS] <graphics|text>\n\n"
           "Set the TTY mode for framebuffer graphics use.\n\n"
           "Modes:\n"
           "  graphics   Switch TTY to graphics mode (disables text cursor)\n"
           "  text       Switch TTY back to text mode\n\n"
           "Options:\n"
           "  -t, --tty PATH   TTY device path (default: /dev/tty1)\n"
           "  -h, --help       Show this help message\n"
           "  -v, --version    Show version\n\n"
           "Note: This command typically requires root privileges.\n",
           prog);
}

int main(int argc, char *argv[]) {
    const char *tty_path = "/dev/tty1";

    static struct option long_opts[] = {
        {"tty",     required_argument, 0, 't'},
        {"help",    no_argument,       0, 'h'},
        {"version", no_argument,       0, 'v'},
        {0, 0, 0, 0}
    };

    int opt;
    while ((opt = getopt_long(argc, argv, "t:hv", long_opts, NULL)) != -1) {
        switch (opt) {
            case 't': tty_path = optarg; break;
            case 'h': print_help(argv[0]); return 0;
            case 'v': printf("sgfb-ttymode " VERSION "\n"); return 0;
            default:  print_help(argv[0]); return 1;
        }
    }

    /* FIXED: check we have a mode argument (original crashed if argc < 2) */
    if (optind >= argc) {
        fprintf(stderr, "Error: mode argument required (graphics or text)\n");
        print_help(argv[0]);
        return 1;
    }

    const char *mode = argv[optind];

    if (strcmp(mode, "graphics") != 0 && strcmp(mode, "text") != 0) {
        fprintf(stderr, "Error: invalid mode '%s' (use 'graphics' or 'text')\n", mode);
        return 1;
    }

    printf("Using TTY: %s\n", tty_path);

    int ttyfd = open(tty_path, O_RDWR);
    if (ttyfd < 0) {
        fprintf(stderr, "Error: could not open '%s' (are you root?)\n", tty_path);
        return 1;
    }

    int kd_mode = (strcmp(mode, "graphics") == 0) ? KD_GRAPHICS : KD_TEXT;

    if (ioctl(ttyfd, KDSETMODE, kd_mode) < 0) {
        fprintf(stderr, "Error: ioctl KDSETMODE failed (are you root?)\n");
        close(ttyfd);
        return 1;
    }

    printf("TTY mode set to: %s\n", mode);
    close(ttyfd);
    return 0;
}
