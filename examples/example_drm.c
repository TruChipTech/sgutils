/*
 * example_drm.c - Example: draw on DRM display using libsgutils
 *
 * Build:
 *   gcc example_drm.c -o example_drm $(pkg-config --cflags --libs sgutils)
 */

#include <sgutils/sgutils.h>
#include <stdio.h>

int main(void) {
    /* Query DRM display info */
    drm_mode_info_t info[4];
    int count = drm_get_mode_info(NULL, info, 4);
    if (count <= 0) {
        fprintf(stderr, "No DRM displays found\n");
        return 1;
    }

    printf("Found %d display(s):\n", count);
    for (int i = 0; i < count; i++) {
        printf("  %s: %dx%d@%dHz (%s)\n",
               info[i].connector_name,
               info[i].width, info[i].height,
               info[i].refresh, info[i].driver_name);
    }

    /* Open DRM display */
    sg_drm_context_t *ctx = drm_context_create(NULL);  /* /dev/dri/card0 */
    if (!ctx) {
        fprintf(stderr, "Failed to open DRM display\n");
        return 1;
    }

    /* Get an image view */
    image_t fb = drm_context_as_image(ctx);

    /* Clear to black */
    clear_image(&fb);

    /* Draw a gradient */
    int w = drm_context_width(ctx);
    int h = drm_context_height(ctx);
    for (int y = 0; y < h; y++) {
        uint8_t r = (uint8_t)(y * 255 / h);
        for (int x = 0; x < w; x++) {
            uint8_t b = (uint8_t)(x * 255 / w);
            set_pixel(&fb, x, y, PIXEL_RGB(r, 0, b));
        }
    }

    /* Draw some shapes on top */
    draw_rect(&fb, w / 4, h / 4, w / 2, h / 2, PIXEL_ARGB(128, 255, 255, 255));
    draw_circle(&fb, w / 2, h / 2, h / 4, PIXEL_RGB(255, 255, 0), 1);

    /* Present the buffer */
    drm_context_present(ctx);

    printf("Drew on %dx%d DRM display\n", w, h);

    drm_context_release(ctx);
    return 0;
}
