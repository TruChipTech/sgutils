/*
 * example_fb.c - Example: draw on framebuffer using libsgutils
 *
 * Build:
 *   gcc example_fb.c -o example_fb $(pkg-config --cflags --libs sgutils)
 *
 * Or without pkg-config (after make install):
 *   gcc example_fb.c -o example_fb -I/usr/local/include/sgutils -lsgutils -lm
 */

#include <sgutils/sgutils.h>
#include <stdio.h>

int main(void) {
    /* Open the framebuffer */
    fb_context_t *ctx = fb_context_create(NULL);  /* /dev/fb0 */
    if (!ctx) {
        fprintf(stderr, "Failed to open framebuffer\n");
        return 1;
    }

    /* Get an image view of the framebuffer */
    image_t fb = fb_context_as_image(ctx);

    /* Clear to dark blue */
    fill_image(&fb, PIXEL_RGB(0, 0, 64));

    /* Draw a red filled rectangle */
    draw_rect(&fb, 50, 50, 200, 150, PIXEL_RGB(255, 0, 0));

    /* Draw a green outlined rectangle */
    draw_rect_outline(&fb, 300, 50, 200, 150, PIXEL_RGB(0, 255, 0), 3);

    /* Draw a yellow line */
    draw_line(&fb, 0, 0, fb.width - 1, fb.height - 1, PIXEL_RGB(255, 255, 0));

    /* Draw a white filled circle */
    draw_circle(&fb, 400, 300, 80, PIXEL_RGB(255, 255, 255), 1);

    /* Draw a cyan circle outline */
    draw_circle(&fb, 600, 300, 60, PIXEL_RGB(0, 255, 255), 0);

#ifdef ENABLE_PNG
    /* Load and draw a PNG image with alpha blending */
    image_t *img = read_png_file("logo.png");
    if (img) {
        draw_image_alpha(&fb, 100, 400, img);
        image_free(img);
    }
#endif

#ifdef ENABLE_TEXT
    /* Render text */
    if (text_init() == 0) {
        fb_font_t *font = font_load("/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf", 48);
        if (font) {
            draw_text(&fb, 50, 250, "Hello from libsgutils!",
                      font, PIXEL_RGB(255, 255, 255));
            font_free(font);
        }
        text_shutdown();
    }
#endif

    printf("Drew on %dx%d framebuffer\n", fb.width, fb.height);

    fb_context_release(ctx);
    return 0;
}
