/*
 * sgutils - Linux Screen Graphics Utilities
 * Copyright (c) 2026 Anand <anand@truchiptechnology.com> - MIT License
 * Date: 2026-05-07
 *
 * img-bmp.c - BMP image loading (no external dependencies)
 *
 * Supports uncompressed 24-bit and 32-bit BMP files.
 * New feature: not present in the original project.
 */

#ifdef ENABLE_BMP

#include "img-bmp.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>

/* BMP file header (14 bytes) */
#pragma pack(push, 1)
typedef struct {
    uint16_t type;       /* "BM" = 0x4D42 */
    uint32_t file_size;
    uint16_t reserved1;
    uint16_t reserved2;
    uint32_t offset;     /* offset to pixel data */
} bmp_file_header_t;

/* DIB header (BITMAPINFOHEADER, 40 bytes) */
typedef struct {
    uint32_t header_size;
    int32_t  width;
    int32_t  height;     /* negative = top-down */
    uint16_t planes;
    uint16_t bpp;
    uint32_t compression;
    uint32_t image_size;
    int32_t  x_ppm;
    int32_t  y_ppm;
    uint32_t colors_used;
    uint32_t colors_important;
} bmp_info_header_t;
#pragma pack(pop)

image_t *read_bmp_file(const char *filename) {
    if (!filename) return NULL;

    FILE *fp = fopen(filename, "rb");
    if (!fp) {
        fprintf(stderr, "SGUTILS: cannot open BMP file '%s'\n", filename);
        return NULL;
    }

    bmp_file_header_t fhdr;
    if (fread(&fhdr, sizeof(fhdr), 1, fp) != 1 || fhdr.type != 0x4D42) {
        fprintf(stderr, "SGUTILS: '%s' is not a valid BMP file\n", filename);
        fclose(fp);
        return NULL;
    }

    bmp_info_header_t ihdr;
    if (fread(&ihdr, sizeof(ihdr), 1, fp) != 1) {
        fprintf(stderr, "SGUTILS: failed to read BMP info header\n");
        fclose(fp);
        return NULL;
    }

    if (ihdr.compression != 0) {
        fprintf(stderr, "SGUTILS: compressed BMP files are not supported\n");
        fclose(fp);
        return NULL;
    }

    if (ihdr.bpp != 24 && ihdr.bpp != 32) {
        fprintf(stderr, "SGUTILS: only 24-bit and 32-bit BMP supported (got %d)\n",
                ihdr.bpp);
        fclose(fp);
        return NULL;
    }

    int width  = (ihdr.width < 0)  ? -ihdr.width  : ihdr.width;
    int height = (ihdr.height < 0) ? -ihdr.height : ihdr.height;
    int top_down = (ihdr.height < 0);

    if (width <= 0 || height <= 0 || width > 32768 || height > 32768) {
        fprintf(stderr, "SGUTILS: invalid BMP dimensions %dx%d\n", width, height);
        fclose(fp);
        return NULL;
    }

    int bytes_per_pixel = ihdr.bpp / 8;
    int row_size = ((width * bytes_per_pixel + 3) / 4) * 4; /* padded to 4 bytes */

    unsigned char *row_buf = malloc((size_t)row_size);
    if (!row_buf) {
        fclose(fp);
        return NULL;
    }

    image_t *image = image_create(width, height);
    if (!image) {
        free(row_buf);
        fclose(fp);
        return NULL;
    }

    fseek(fp, (long)fhdr.offset, SEEK_SET);

    for (int y = 0; y < height; y++) {
        if (fread(row_buf, 1, (size_t)row_size, fp) != (size_t)row_size) {
            fprintf(stderr, "SGUTILS: incomplete BMP data at row %d\n", y);
            image_free(image);
            free(row_buf);
            fclose(fp);
            return NULL;
        }

        int dest_y = top_down ? y : (height - 1 - y);

        for (int x = 0; x < width; x++) {
            int idx = x * bytes_per_pixel;
            uint8_t b = row_buf[idx + 0];
            uint8_t g = row_buf[idx + 1];
            uint8_t r = row_buf[idx + 2];
            uint8_t a = (bytes_per_pixel == 4) ? row_buf[idx + 3] : 0xFF;
            image->data[dest_y * width + x] = PIXEL_ARGB(a, r, g, b);
        }
    }

    free(row_buf);
    fclose(fp);
    return image;
}

#endif /* ENABLE_BMP */
