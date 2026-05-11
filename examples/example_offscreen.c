/*
 * example_offscreen.c - Example: offscreen rendering + compositing
 *
 * This example shows how to create offscreen images, draw on them,
 * composite layers, and output to the framebuffer — all using
 * the libsgutils API without any CLI tools.
 *
 * Build:
 *   gcc example_offscreen.c -o example_offscreen $(pkg-config --cflags --libs sgutils)
 */

#include <sgutils/sgutils.h>
#include <stdio.h>
#include <stdlib.h>

int main(void) {
    int width  = 800;
    int height = 600;

    /* Create an offscreen background */
    image_t *background = image_create(width, height);
    if (!background) return 1;
    fill_image(background, PIXEL_RGB(30, 30, 60));

    /* Draw a grid pattern on the background */
    for (int x = 0; x < width; x += 40) {
        draw_line(background, x, 0, x, height - 1, PIXEL_RGB(50, 50, 80));
    }
    for (int y = 0; y < height; y += 40) {
        draw_line(background, 0, y, width - 1, y, PIXEL_RGB(50, 50, 80));
    }

    /* Create a foreground overlay with shapes */
    image_t *overlay = image_create(width, height);
    if (!overlay) { image_free(background); return 1; }
    clear_image(overlay);  /* transparent black */

    /* Draw semi-transparent circles */
    draw_circle(overlay, 200, 200, 100, PIXEL_ARGB(180, 255, 0, 0), 1);
    draw_circle(overlay, 350, 200, 100, PIXEL_ARGB(180, 0, 255, 0), 1);
    draw_circle(overlay, 275, 330, 100, PIXEL_ARGB(180, 0, 0, 255), 1);

    /* Use the compositor to blend layers */
    compositor_t *comp = compositor_create(width, height);
    if (!comp) {
        image_free(overlay);
        image_free(background);
        return 1;
    }

    compositor_add_layer(comp, background, 0, 0, 255);
    compositor_add_layer(comp, overlay, 0, 0, 200);

    /* Render to a new image */
    image_t *result = compositor_render_to_image(comp);

    if (result) {
        /* Apply a grayscale transform to a clone */
        image_t *gray = image_clone(result);
        if (gray) {
            image_grayscale(gray);
            printf("Created %dx%d grayscale version\n", gray->width, gray->height);
            image_free(gray);
        }

        /* Optionally write to framebuffer */
        fb_context_t *ctx = fb_context_create(NULL);
        if (ctx) {
            image_t fb = fb_context_as_image(ctx);

            /* Scale result to fit screen */
            image_t *scaled = image_scale(result, fb.width, fb.height);
            if (scaled) {
                draw_image(&fb, 0, 0, scaled);
                image_free(scaled);
            } else {
                draw_image(&fb, 0, 0, result);
            }

            fb_context_release(ctx);
            printf("Rendered to framebuffer\n");
        } else {
            printf("No framebuffer available (OK for testing)\n");
        }

        image_free(result);
    }

    /* compositor_free frees the layers (background, overlay) too */
    compositor_free(comp);

    return 0;
}
