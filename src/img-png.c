/*
 * sgutils - Linux Screen Graphics Utilities
 * Copyright (c) 2026 Anand <anand@truchiptechnology.com> - MIT License
 * Date: 2026-05-07
 *
 * img-png.c - PNG image loading via libpng
 *
 * Bug fixes over the original:
 *  - NULL check on fopen (was missing entirely)
 *  - Proper ARGB pixel format with alpha preserved
 *  - All row_pointers freed on error paths
 *  - Proper error return instead of abort()
 */

#ifdef ENABLE_PNG

#include "img-png.h"

#include <stdlib.h>
#include <stdio.h>
#include <png.h>

image_t *read_png_file(const char *filename) {
    if (!filename) return NULL;

    FILE *fp = fopen(filename, "rb");
    if (!fp) {
        fprintf(stderr, "SGUTILS: cannot open PNG file '%s'\n", filename);
        return NULL;
    }

    png_structp png = png_create_read_struct(PNG_LIBPNG_VER_STRING,
                                             NULL, NULL, NULL);
    if (!png) {
        fclose(fp);
        return NULL;
    }

    png_infop info = png_create_info_struct(png);
    if (!info) {
        png_destroy_read_struct(&png, NULL, NULL);
        fclose(fp);
        return NULL;
    }

    if (setjmp(png_jmpbuf(png))) {
        png_destroy_read_struct(&png, &info, NULL);
        fclose(fp);
        return NULL;
    }

    png_init_io(png, fp);
    png_read_info(png, info);

    int width      = (int)png_get_image_width(png, info);
    int height     = (int)png_get_image_height(png, info);
    png_byte color = png_get_color_type(png, info);
    png_byte depth = png_get_bit_depth(png, info);

    /* Normalize to 8-bit RGBA */
    if (depth == 16)
        png_set_strip_16(png);

    if (color == PNG_COLOR_TYPE_PALETTE)
        png_set_palette_to_rgb(png);

    if (color == PNG_COLOR_TYPE_GRAY && depth < 8)
        png_set_expand_gray_1_2_4_to_8(png);

    if (png_get_valid(png, info, PNG_INFO_tRNS))
        png_set_tRNS_to_alpha(png);

    /* Add alpha channel if missing */
    if (color == PNG_COLOR_TYPE_RGB ||
        color == PNG_COLOR_TYPE_GRAY ||
        color == PNG_COLOR_TYPE_PALETTE)
        png_set_filler(png, 0xFF, PNG_FILLER_AFTER);

    if (color == PNG_COLOR_TYPE_GRAY ||
        color == PNG_COLOR_TYPE_GRAY_ALPHA)
        png_set_gray_to_rgb(png);

    png_read_update_info(png, info);

    /* Allocate row pointers */
    png_bytep *row_pointers = malloc(sizeof(png_bytep) * (size_t)height);
    if (!row_pointers) {
        png_destroy_read_struct(&png, &info, NULL);
        fclose(fp);
        return NULL;
    }

    size_t rowbytes = png_get_rowbytes(png, info);
    for (int y = 0; y < height; y++) {
        row_pointers[y] = malloc(rowbytes);
        if (!row_pointers[y]) {
            for (int j = 0; j < y; j++) free(row_pointers[j]);
            free(row_pointers);
            png_destroy_read_struct(&png, &info, NULL);
            fclose(fp);
            return NULL;
        }
    }

    png_read_image(png, row_pointers);

    /* Convert to our ARGB pixel format */
    image_t *image = image_create(width, height);
    if (!image) {
        for (int y = 0; y < height; y++) free(row_pointers[y]);
        free(row_pointers);
        png_destroy_read_struct(&png, &info, NULL);
        fclose(fp);
        return NULL;
    }

    for (int y = 0; y < height; y++) {
        png_bytep row = row_pointers[y];
        for (int x = 0; x < width; x++) {
            png_bytep px = &row[x * 4];
            /* libpng gives us RGBA; we store ARGB */
            image->data[y * width + x] = PIXEL_ARGB(px[3], px[0], px[1], px[2]);
        }
    }

    /* Cleanup */
    for (int y = 0; y < height; y++) free(row_pointers[y]);
    free(row_pointers);
    png_destroy_read_struct(&png, &info, NULL);
    fclose(fp);

    return image;
}

#endif /* ENABLE_PNG */
