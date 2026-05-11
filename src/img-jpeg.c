/*
 * sgutils - Linux Screen Graphics Utilities
 * Copyright (c) 2026 Anand <anand@truchiptechnology.com> - MIT License
 * Date: 2026-05-07
 *
 * img-jpeg.c - JPEG image loading via libjpeg
 *
 * Bug fixes over the original:
 *  - Proper ARGB pixel format output
 *  - Better error handling with fprintf to stderr
 *  - Memory cleanup on all error paths
 */

#ifdef ENABLE_JPEG

#include "img-jpeg.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <setjmp.h>
#include <jpeglib.h>

struct my_error_mgr {
    struct jpeg_error_mgr pub;
    jmp_buf setjmp_buffer;
};

typedef struct my_error_mgr *my_error_ptr;

static void my_error_exit(j_common_ptr cinfo) {
    my_error_ptr myerr = (my_error_ptr)cinfo->err;
    (*cinfo->err->output_message)(cinfo);
    longjmp(myerr->setjmp_buffer, 1);
}

image_t *read_jpeg_file(const char *filename) {
    if (!filename) return NULL;

    struct jpeg_decompress_struct cinfo;
    struct my_error_mgr jerr;
    FILE *infile;
    JSAMPARRAY buffer;
    int row_stride;

    infile = fopen(filename, "rb");
    if (!infile) {
        fprintf(stderr, "SGUTILS: cannot open JPEG file '%s'\n", filename);
        return NULL;
    }

    cinfo.err = jpeg_std_error(&jerr.pub);
    jerr.pub.error_exit = my_error_exit;

    if (setjmp(jerr.setjmp_buffer)) {
        jpeg_destroy_decompress(&cinfo);
        fclose(infile);
        return NULL;
    }

    jpeg_create_decompress(&cinfo);
    jpeg_stdio_src(&cinfo, infile);
    (void)jpeg_read_header(&cinfo, TRUE);

    /* Request BGRX output for efficient 32-bit pixel handling */
    cinfo.out_color_space = JCS_EXT_BGRX;
    (void)jpeg_start_decompress(&cinfo);

    row_stride = (int)(cinfo.output_width * (unsigned)cinfo.output_components);
    buffer = (*cinfo.mem->alloc_sarray)((j_common_ptr)&cinfo,
                                         JPOOL_IMAGE,
                                         (JDIMENSION)row_stride, 1);

    image_t *image = image_create((int)cinfo.output_width,
                                   (int)cinfo.output_height);
    if (!image) {
        jpeg_destroy_decompress(&cinfo);
        fclose(infile);
        return NULL;
    }

    int row = 0;
    while (cinfo.output_scanline < cinfo.output_height) {
        (void)jpeg_read_scanlines(&cinfo, buffer, 1);

        /* Convert BGRX to ARGB */
        for (int x = 0; x < (int)cinfo.output_width; x++) {
            int idx = x * cinfo.output_components;
            uint8_t b = buffer[0][idx + 0];
            uint8_t g = buffer[0][idx + 1];
            uint8_t r = buffer[0][idx + 2];
            /* JPEG has no alpha; set to fully opaque */
            image->data[row * image->width + x] = PIXEL_ARGB(0xFF, r, g, b);
        }
        row++;
    }

    jpeg_finish_decompress(&cinfo);
    jpeg_destroy_decompress(&cinfo);
    fclose(infile);

    return image;
}

#endif /* ENABLE_JPEG */
