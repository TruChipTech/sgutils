/*
 * sgutils - Linux Screen Graphics Utilities
 * Copyright (c) 2026 Anand <anand@truchiptechnology.com> - MIT License
 * Date: 2026-05-07
 *
 * text.c - FreeType-based text rendering
 *
 * Implements the TODO item: "Add text rendering function with freetype"
 * Supports TrueType/OpenType fonts, proper kerning, and alpha-blended
 * glyph rendering onto an image_t target.
 */

#ifdef ENABLE_TEXT

#include "text.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ft2build.h>
#include FT_FREETYPE_H

static FT_Library ft_library = NULL;

struct fb_font {
    FT_Face  face;
    int      pixel_size;
};

int text_init(void) {
    if (ft_library) return 0; /* already initialized */
    if (FT_Init_FreeType(&ft_library)) {
        fprintf(stderr, "SGUTILS: failed to initialize FreeType\n");
        return -1;
    }
    return 0;
}

void text_shutdown(void) {
    if (ft_library) {
        FT_Done_FreeType(ft_library);
        ft_library = NULL;
    }
}

fb_font_t *font_load(const char *path, int pixel_size) {
    if (!path || pixel_size <= 0) return NULL;

    if (!ft_library && text_init() != 0) return NULL;

    fb_font_t *font = malloc(sizeof(fb_font_t));
    if (!font) return NULL;

    if (FT_New_Face(ft_library, path, 0, &font->face)) {
        fprintf(stderr, "SGUTILS: failed to load font '%s'\n", path);
        free(font);
        return NULL;
    }

    FT_Set_Pixel_Sizes(font->face, 0, (FT_UInt)pixel_size);
    font->pixel_size = pixel_size;
    return font;
}

void font_free(fb_font_t *font) {
    if (!font) return;
    if (font->face) FT_Done_Face(font->face);
    free(font);
}

void draw_text(image_t *dst, int x, int y, const char *text,
               fb_font_t *font, pixel_t color) {
    if (!dst || !dst->data || !text || !font || !font->face) return;

    FT_Face face = font->face;
    uint8_t cr = PIXEL_R(color);
    uint8_t cg = PIXEL_G(color);
    uint8_t cb = PIXEL_B(color);

    int pen_x = x;
    int pen_y = y;

    FT_Bool use_kerning = FT_HAS_KERNING(face);
    FT_UInt prev_glyph = 0;

    for (const char *p = text; *p; p++) {
        FT_UInt glyph_index = FT_Get_Char_Index(face, (FT_ULong)(unsigned char)*p);

        /* Apply kerning if available */
        if (use_kerning && prev_glyph && glyph_index) {
            FT_Vector delta;
            FT_Get_Kerning(face, prev_glyph, glyph_index,
                           FT_KERNING_DEFAULT, &delta);
            pen_x += (int)(delta.x >> 6);
        }

        if (FT_Load_Glyph(face, glyph_index, FT_LOAD_RENDER)) continue;

        FT_GlyphSlot slot = face->glyph;
        FT_Bitmap *bmp = &slot->bitmap;

        int gx = pen_x + slot->bitmap_left;
        int gy = pen_y - slot->bitmap_top;

        /* Render the glyph bitmap with alpha blending */
        for (int row = 0; row < (int)bmp->rows; row++) {
            for (int col = 0; col < (int)bmp->width; col++) {
                int dx = gx + col;
                int dy = gy + row;

                if (dx < 0 || dx >= dst->width ||
                    dy < 0 || dy >= dst->height)
                    continue;

                uint8_t alpha = bmp->buffer[row * bmp->pitch + col];
                if (alpha == 0) continue;

                pixel_t fg = PIXEL_ARGB(alpha, cr, cg, cb);
                int idx = dy * dst->width + dx;
                dst->data[idx] = alpha_blend(dst->data[idx], fg);
            }
        }

        pen_x += (int)(slot->advance.x >> 6);
        prev_glyph = glyph_index;
    }
}

void measure_text(fb_font_t *font, const char *text, int *out_w, int *out_h) {
    if (!font || !font->face || !text) {
        if (out_w) *out_w = 0;
        if (out_h) *out_h = 0;
        return;
    }

    FT_Face face = font->face;
    int w = 0;
    int max_ascent  = 0;
    int max_descent = 0;

    FT_Bool use_kerning = FT_HAS_KERNING(face);
    FT_UInt prev_glyph = 0;

    for (const char *p = text; *p; p++) {
        FT_UInt gi = FT_Get_Char_Index(face, (FT_ULong)(unsigned char)*p);

        if (use_kerning && prev_glyph && gi) {
            FT_Vector delta;
            FT_Get_Kerning(face, prev_glyph, gi, FT_KERNING_DEFAULT, &delta);
            w += (int)(delta.x >> 6);
        }

        if (FT_Load_Glyph(face, gi, FT_LOAD_DEFAULT)) continue;

        FT_GlyphSlot slot = face->glyph;
        w += (int)(slot->advance.x >> 6);

        int ascent  = (int)(slot->metrics.horiBearingY >> 6);
        int descent = (int)((slot->metrics.height >> 6) - ascent);

        if (ascent > max_ascent)   max_ascent  = ascent;
        if (descent > max_descent) max_descent = descent;

        prev_glyph = gi;
    }

    if (out_w) *out_w = w;
    if (out_h) *out_h = max_ascent + max_descent;
}

#endif /* ENABLE_TEXT */
