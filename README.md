# sgutils — Linux Screen Graphics Utilities

A collection of command-line tools for rendering graphics directly to the
Linux display — via either the legacy **framebuffer** (`/dev/fb*`) or the
modern **DRM/KMS** (`/dev/dri/card*`) subsystem. Display images, render text,
composite layers, and control the TTY — without a window manager.

**sgutils** is ideal for Raspberry Pi dashboards, kiosk displays, embedded
signage, and lightweight graphics on headless Linux systems.

## Features

| Feature | Description |
|---------|-------------|
| **Multi-format images** | PNG, JPEG, SVG, and BMP support |
| **Text rendering** | TrueType/OpenType fonts via FreeType with kerning |
| **Compositing** | Layer-based offscreen compositing with per-layer opacity |
| **Alpha blending** | Full ARGB pixel pipeline with "over" compositing |
| **Image transforms** | Invert, grayscale, hue shift, brightness, rotation (90/180/270) |
| **Dual backend** | Framebuffer (`/dev/fbN`) and DRM/KMS (`/dev/dri/cardN`) support |
| **Configurable device** | Specify any framebuffer or DRM device |
| **Clean CLI** | `getopt_long`-based argument parsing with `--help` and `--version` |

## Dependencies

Install the development libraries for the features you need:

```bash
# Core (always required)
sudo apt-get install build-essential

# Image format support
sudo apt-get install libpng-dev libjpeg-dev

# SVG support (librsvg + cairo)
sudo apt-get install librsvg2-dev libglib2.0-dev

# Text rendering
sudo apt-get install libfreetype-dev

# DRM/KMS support
sudo apt-get install libdrm-dev

# All dependencies at once
sudo apt-get install build-essential libpng-dev libjpeg-dev \
    librsvg2-dev libglib2.0-dev libfreetype-dev libdrm-dev
```

> **Note:** BMP support has no external dependencies.

## Building

### With Make

```bash
make
```

### Build Options (Make)

Disable features you don't need to reduce dependencies:

```bash
# Build without SVG support
make ENABLE_SVG=0

# Build without DRM (framebuffer only)
make ENABLE_DRM=0

# Build with only PNG and BMP support
make ENABLE_SVG=0 ENABLE_JPEG=0 ENABLE_TEXT=0

# Build everything (default)
make ENABLE_PNG=1 ENABLE_JPEG=1 ENABLE_SVG=1 ENABLE_BMP=1 ENABLE_TEXT=1 ENABLE_DRM=1
```

### With CMake

```bash
cmake -B build
cmake --build build
```

### Build Options (CMake)

```bash
# Build without DRM
cmake -B build -DENABLE_DRM=OFF

# Minimal build
cmake -B build -DENABLE_PNG=OFF -DENABLE_JPEG=OFF -DENABLE_SVG=OFF -DENABLE_TEXT=OFF

# Release build
cmake -B build -DCMAKE_BUILD_TYPE=Release
```

### Cleaning

```bash
# Make
make clean

# CMake — clean build artifacts (keeps build config)
cmake --build build --target clean

# CMake — full clean (remove entire build directory)
rm -rf build
```

## Installation

```bash
sudo make install              # Install to /usr/local/bin
sudo make install PREFIX=/usr  # Install to /usr/bin
sudo make uninstall            # Remove installed binaries
```

## Setup

You may need to add your user to the `video` group to access the framebuffer
without `sudo`:

```bash
sudo usermod -aG video $(whoami)
# Log out and back in for the change to take effect
```

## Tools & Usage

### `sgfb-describe` — Show framebuffer information

Outputs the device path and dimensions.

```bash
$ sgfb-describe
/dev/fb0 1920 1080

# JSON output
$ sgfb-describe --json
{"device":"/dev/fb0","width":1920,"height":1080}

# Custom device
$ sgfb-describe -d /dev/fb1
```

**Options:**

| Flag | Description |
|------|-------------|
| `-d, --device PATH` | Framebuffer device (default: `/dev/fb0`) |
| `-j, --json` | Output as JSON |
| `-h, --help` | Show help |
| `-v, --version` | Show version |

---

### `sgfb-clear` — Clear the framebuffer

Fill the screen with a solid color, grayscale value, or test pattern.

```bash
# Clear to black
$ sgfb-clear

# Clear to white
$ sgfb-clear -g 255

# Clear to red
$ sgfb-clear -c FF0000

# Display a color bar test pattern
$ sgfb-clear --test-pattern
```

**Options:**

| Flag | Description |
|------|-------------|
| `-d, --device PATH` | Framebuffer device (default: `/dev/fb0`) |
| `-g, --gray VALUE` | Grayscale 0–255 (0=black, 255=white) |
| `-c, --color RRGGBB` | Hex color (e.g., `FF0000` for red) |
| `-t, --test-pattern` | Display SMPTE-style color bars |
| `-h, --help` | Show help |

---

### `sgfb-ttymode` — Set TTY graphics/text mode

Switch the TTY between graphics mode (hides text cursor) and text mode.

```bash
# Switch to graphics mode (run over SSH!)
$ sudo sgfb-ttymode graphics

# Switch back to text mode
$ sudo sgfb-ttymode text

# Use a different TTY
$ sudo sgfb-ttymode -t /dev/tty2 graphics
```

**Options:**

| Flag | Description |
|------|-------------|
| `-t, --tty PATH` | TTY device (default: `/dev/tty1`) |
| `-h, --help` | Show help |

> **Note:** Requires root privileges. Switching to graphics mode over a direct
> console will hide the text cursor — use SSH.

---

### `sgfb-pngdraw` / `sgfb-jpgdraw` / `sgfb-svgdraw` / `sgfb-bmpdraw` — Draw images

All image drawing tools share the same interface. The binary name determines
the default format, or use `-F` to override.

```bash
# Draw a PNG, scaled to fill the screen
$ sgfb-pngdraw -f photo.png

# Draw a JPEG at a specific position and size
$ sgfb-jpgdraw -f photo.jpg -x 100 -y 50 -w 800 -H 600

# Draw with color filters
$ sgfb-pngdraw -f photo.png --invert
$ sgfb-pngdraw -f photo.png --grayscale
$ sgfb-pngdraw -f photo.png --hue r          # Red hue shift
$ sgfb-pngdraw -f photo.png --hue cym        # Cyan+Yellow+Magenta

# Adjust brightness and rotate
$ sgfb-jpgdraw -f photo.jpg --brightness 30 --rotate 90

# Draw with alpha blending (for PNG with transparency)
$ sgfb-pngdraw -f overlay.png -x 100 -y 100 --alpha

# Draw a BMP image
$ sgfb-bmpdraw -f image.bmp

# Draw SVG at specific dimensions
$ sgfb-svgdraw -f icon.svg -w 200 -H 200
```

**Options:**

| Flag | Description |
|------|-------------|
| `-f, --file PATH` | **(Required)** Image file path |
| `-d, --device PATH` | Framebuffer device (default: `/dev/fb0`) |
| `-x, --xpos N` | X position (default: 0) |
| `-y, --ypos N` | Y position (default: 0) |
| `-w, --width N` | Display width (default: framebuffer width) |
| `-H, --height N` | Display height (default: framebuffer height) |
| `-F, --format FMT` | Force format: `png`, `jpeg`, `svg`, `bmp` |
| `-i, --invert` | Invert colors |
| `-G, --grayscale` | Convert to grayscale |
| `-u, --hue MASK` | Hue shift (chars: `r`,`g`,`b`,`c`,`y`,`m`,`w`) |
| `-T, --hue-threshold N` | Hue threshold (default: 20) |
| `-b, --brightness N` | Brightness adjustment (-255 to 255) |
| `-r, --rotate DEG` | Rotate: 90, 180, or 270 degrees |
| `-a, --alpha` | Enable alpha blending |
| `-h, --help` | Show help |

---

### `sgfb-text` — Render text with TrueType fonts

Render text directly to the framebuffer using any TrueType or OpenType font.

```bash
# Basic text rendering
$ sgfb-text -t "Hello, World!" -F /usr/share/fonts/truetype/dejavu/DejaVuSans.ttf

# Custom size, position, and color
$ sgfb-text -t "Dashboard" -F /path/to/font.ttf -s 64 -x 100 -y 80 -c 00FF00

# White text on blue background
$ sgfb-text -t "Status: OK" -F /path/to/font.ttf -c FFFFFF -b 0000AA
```

**Options:**

| Flag | Description |
|------|-------------|
| `-t, --text STRING` | **(Required)** Text to render |
| `-F, --font PATH` | **(Required)** Path to `.ttf` or `.otf` font |
| `-d, --device PATH` | Framebuffer device (default: `/dev/fb0`) |
| `-s, --size N` | Font size in pixels (default: 32) |
| `-x, --xpos N` | X position (default: 0) |
| `-y, --ypos N` | Y baseline position (default: font size) |
| `-c, --color RRGGBB` | Text color (default: `FFFFFF`) |
| `-b, --bg-color RRGGBB` | Fill background before text |
| `-h, --help` | Show help |

---

### `sgfb-composite` — Layer-based compositing

Compose multiple images with positioning and per-layer opacity, rendered via
an offscreen buffer with alpha blending.

```bash
# Overlay a logo on top of a background image
$ sgfb-composite \
    -l background.jpg:0:0 \
    -l logo.png:50:50:200

# Three-layer composition with a clear color
$ sgfb-composite \
    -c 333333 \
    -l photo.jpg:0:0:255 \
    -l overlay.png:100:100:180 \
    -l badge.png:800:20:255
```

**Layer specification format:** `FILE:X:Y[:OPACITY]`

- `FILE` — Image file path
- `X, Y` — Position on screen
- `OPACITY` — 0 (transparent) to 255 (opaque), default 255

**Options:**

| Flag | Description |
|------|-------------|
| `-l, --layer SPEC` | Layer specification (repeatable) |
| `-d, --device PATH` | Framebuffer device (default: `/dev/fb0`) |
| `-c, --clear RRGGBB` | Background fill color |
| `-h, --help` | Show help |

---

## DRM/KMS Tools

The `sgdrm-*` tools mirror the framebuffer tools but use the DRM/KMS subsystem
(`/dev/dri/cardN`). They work on modern systems where `/dev/fb*` may not be
available. All `sgdrm-*` tools accept `-d /dev/dri/cardN` to select a GPU device.

### `sgdrm-describe` — Show DRM display information

```bash
$ sgdrm-describe
HDMI-A-1 1920x1080@60Hz (driver: i915)

$ sgdrm-describe --json
[{"connector":"HDMI-A-1","width":1920,"height":1080,"refresh":60,"driver":"i915"}]
```

### `sgdrm-clear` — Clear DRM display

```bash
$ sgdrm-clear                  # Clear to black
$ sgdrm-clear -c FF0000        # Clear to red
$ sgdrm-clear --test-pattern   # Color bars
```

### `sgdrm-pngdraw` / `sgdrm-jpgdraw` / `sgdrm-svgdraw` / `sgdrm-bmpdraw` — Draw images

Same interface as the `sgfb-*` image tools:

```bash
$ sgdrm-pngdraw -f photo.png
$ sgdrm-jpgdraw -f photo.jpg -x 100 -y 50 -w 800 -H 600
$ sgdrm-pngdraw -f overlay.png --alpha --grayscale
```

### `sgdrm-text` — Render text on DRM display

```bash
$ sgdrm-text -t "Hello" -F /usr/share/fonts/truetype/dejavu/DejaVuSans.ttf
$ sgdrm-text -t "Status" -F /path/to/font.ttf -s 64 -c 00FF00
```

### `sgdrm-composite` — Layer compositing on DRM display

```bash
$ sgdrm-composite \
    -l background.jpg:0:0 \
    -l logo.png:50:50:200
```

> **Choosing between sgfb-\* and sgdrm-\*:** Use `sgfb-*` if your system has a
> framebuffer device (`/dev/fb0`). Use `sgdrm-*` on modern systems with DRM/KMS.
> Functionality is identical — only the display backend differs.

---

## Architecture

```
include/
  draw.h            Core types (pixel_t, image_t) and drawing API
  framebuffer.h     Framebuffer device abstraction
  drm.h             DRM/KMS device abstraction
  compositing.h     Layer-based compositing
  text.h            FreeType text rendering
  img-png.h         PNG loader
  img-jpeg.h        JPEG loader
  img-svg.h         SVG loader
  img-bmp.h         BMP loader

src/
  draw.c            Pixel ops, transforms, drawing primitives, alpha blending
  framebuffer.c     /dev/fbN mmap and ioctl
  sgdrm.c           /dev/dri/cardN DRM dumb buffer access
  compositing.c     Offscreen compositor with layer opacity
  text.c            FreeType glyph rendering with kerning
  img-png.c         libpng integration
  img-jpeg.c        libjpeg integration
  img-svg.c         librsvg + cairo integration
  img-bmp.c         Pure C BMP parser (no dependencies)
  sgfb-describe.c   CLI: describe framebuffer
  sgfb-clear.c      CLI: clear/fill framebuffer
  sgfb-ttymode.c    CLI: switch TTY mode
  sgfb-imgdraw.c    CLI: draw images (framebuffer)
  sgfb-text.c       CLI: render text (framebuffer)
  sgfb-composite.c  CLI: compositing (framebuffer)
  sgdrm-describe.c  CLI: describe DRM outputs
  sgdrm-clear.c     CLI: clear DRM display
  sgdrm-imgdraw.c   CLI: draw images (DRM)
  sgdrm-text.c      CLI: render text (DRM)
  sgdrm-composite.c CLI: compositing (DRM)

examples/
  example_fb.c        Draw shapes on framebuffer
  example_drm.c       Draw gradient on DRM display
  example_offscreen.c Offscreen compositing demo
```

## Using libsgutils in Your C Program

sgutils also ships as a library (`libsgutils.a` / `libsgutils.so`) so you can
call all drawing, image loading, compositing, and display functions directly
from your own C code.

### Install the library

```bash
# With Make
sudo make install

# With CMake
cmake -B build
cmake --build build
sudo cmake --install build
```

### Include and link

```c
#include <sgutils/sgutils.h>
```

```bash
# Using pkg-config (recommended)
gcc myapp.c -o myapp $(pkg-config --cflags --libs sgutils)

# Or manually
gcc myapp.c -o myapp -I/usr/local/include/sgutils -lsgutils -lm
```

### Library-only build (no CLI tools)

```bash
# CMake
cmake -B build -DBUILD_TOOLS=OFF
cmake --build build
sudo cmake --install build
```

### Quick example — draw on framebuffer

```c
#include <sgutils/sgutils.h>

int main(void) {
    fb_context_t *ctx = fb_context_create(NULL);  /* /dev/fb0 */
    if (!ctx) return 1;

    image_t fb = fb_context_as_image(ctx);

    fill_image(&fb, PIXEL_RGB(0, 0, 64));                   /* dark blue bg */
    draw_rect(&fb, 50, 50, 200, 150, PIXEL_RGB(255, 0, 0)); /* red box     */
    draw_circle(&fb, 400, 300, 80, PIXEL_RGB(255, 255, 255), 1); /* white circle */
    draw_line(&fb, 0, 0, fb.width, fb.height, PIXEL_RGB(255, 255, 0));

    fb_context_release(ctx);
    return 0;
}
```

### Quick example — DRM display

```c
#include <sgutils/sgutils.h>

int main(void) {
    sg_drm_context_t *ctx = drm_context_create(NULL);  /* /dev/dri/card0 */
    if (!ctx) return 1;

    image_t fb = drm_context_as_image(ctx);
    fill_image(&fb, PIXEL_RGB(0, 128, 0));  /* green */
    draw_circle(&fb, 400, 300, 100, PIXEL_RGB(255, 255, 0), 1);

    drm_context_present(ctx);
    drm_context_release(ctx);
    return 0;
}
```

### Quick example — offscreen compositing

```c
#include <sgutils/sgutils.h>

int main(void) {
    image_t *bg = image_create(800, 600);
    fill_image(bg, PIXEL_RGB(30, 30, 60));

    image_t *overlay = image_create(800, 600);
    clear_image(overlay);
    draw_circle(overlay, 400, 300, 100, PIXEL_ARGB(180, 255, 0, 0), 1);

    compositor_t *comp = compositor_create(800, 600);
    compositor_add_layer(comp, bg, 0, 0, 255);
    compositor_add_layer(comp, overlay, 0, 0, 200);

    image_t *result = compositor_render_to_image(comp);
    /* result is an 800x600 composited image you can draw or save */

    image_free(result);
    compositor_free(comp);  /* frees bg and overlay too */
    return 0;
}
```

### API summary

| Module | Key functions |
|--------|---------------|
| **draw.h** | `image_create()`, `image_free()`, `image_clone()`, `set_pixel()`, `get_pixel()`, `draw_rect()`, `draw_line()`, `draw_circle()`, `draw_image()`, `draw_image_alpha()`, `fill_image()`, `clear_image()`, `test_pattern()`, `alpha_blend()` |
| **draw.h** (transforms) | `image_invert()`, `image_grayscale()`, `image_hueify()`, `image_brightness()`, `image_rotate()`, `image_scale()` |
| **framebuffer.h** | `fb_context_create()`, `fb_context_get_dimensions()`, `fb_context_release()`, `fb_context_as_image()` |
| **drm.h** | `drm_context_create()`, `drm_get_mode_info()`, `drm_context_as_image()`, `drm_context_present()`, `drm_context_release()` |
| **compositing.h** | `compositor_create()`, `compositor_add_layer()`, `compositor_render()`, `compositor_render_to_image()`, `compositor_free()` |
| **text.h** | `text_init()`, `text_shutdown()`, `font_load()`, `font_free()`, `draw_text()`, `measure_text()` |
| **img-png.h** | `read_png_file()` |
| **img-jpeg.h** | `read_jpeg_file()` |
| **img-svg.h** | `read_svg_file()` |
| **img-bmp.h** | `read_bmp_file()` |

## Pixel Format

All pixels are stored as 32-bit ARGB:

```
Bits 31-24: Alpha (0x00 = transparent, 0xFF = opaque)
Bits 23-16: Red
Bits 15-8:  Green
Bits  7-0:  Blue
```

Use the provided macros:

```c
#include "draw.h"

pixel_t red   = PIXEL_RGB(255, 0, 0);
pixel_t semi  = PIXEL_ARGB(128, 0, 255, 0);  // 50% green

uint8_t r = PIXEL_R(red);   // 255
uint8_t a = PIXEL_A(semi);  // 128
```

## Bug Fixes Over the Original

This project addresses all known issues from the original
[richinfante/fbutils](https://github.com/richinfante/fbutils):

| Bug | Fix |
|-----|-----|
| `height` initialized to `context->width` in all image draw tools | Fixed: defaults to `context->height` |
| `test_pattern()` used `&context[offset]` instead of `&context->data[offset]` | Fixed: uses `dst->data` properly |
| `clear_context_gray()` used `memset` (sets bytes, not 32-bit pixels) | Fixed: fills with proper ARGB gray pixels |
| `sgfb-ttymode` accessed `argv[1]` without bounds check | Fixed: validates argc before access |
| Scaled images never freed (memory leak) | Fixed: all temporary images freed |
| `sgfb-describe` leaked context struct | Fixed: context released on all paths |
| `img-png.c` missing NULL check on `fopen` | Fixed: proper NULL check and error message |
| Missing `librsvg2-dev` in build dependencies (Issue #10) | Fixed: documented in Dependencies section |
| No `--help` or `--version` on any tool | Fixed: all tools support `--help` and `--version` |
| Hardcoded `/dev/fb0` device path | Fixed: configurable via `-d` flag |

## Enhancements Over the Original

| Enhancement | Description |
|-------------|-------------|
| **FreeType text rendering** | Full TrueType/OpenType support with kerning (was TODO) |
| **Compositing engine** | Multi-layer offscreen compositing with opacity (was TODO) |
| **Alpha blending** | Proper ARGB pipeline with "over" compositing (was TODO) |
| **BMP format** | New image format with zero dependencies |
| **Brightness adjustment** | `-b` / `--brightness` option |
| **Image rotation** | `-r` / `--rotate` (90, 180, 270 degrees) |
| **Hex color clear** | `sgfb-clear -c RRGGBB` |
| **JSON output** | `sgfb-describe --json` |
| **Test pattern** | `sgfb-clear --test-pattern` |
| **Drawing primitives** | Lines (Bresenham), circles (midpoint), outlined rectangles |
| **Proper build system** | Feature flags, install/uninstall targets, `pkg-config` |
| **CMake support** | Full CMake build system alongside Make |
| **DRM/KMS backend** | Modern display output via `/dev/dri/cardN` dumb buffers |
| **Configurable device** | All tools accept `-d /dev/fbN` or `-d /dev/dri/cardN` |

## Scripting Examples

### Dashboard with background + text overlay

```bash
#!/bin/bash
sgfb-clear -c 1A1A2E
sgfb-pngdraw -f dashboard_bg.png --alpha
sgfb-text -t "CPU: $(top -bn1 | grep Cpu | awk '{print $2}')%" \
    -F /usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf \
    -s 48 -x 50 -y 80 -c 00FF88
sgfb-text -t "$(date '+%H:%M:%S')" \
    -F /usr/share/fonts/truetype/dejavu/DejaVuSans.ttf \
    -s 96 -x 50 -y 200 -c FFFFFF
```

### Slideshow

```bash
#!/bin/bash
for img in /path/to/images/*.jpg; do
    sgfb-jpgdraw -f "$img"
    sleep 5
done
```

### Composite status display

```bash
sgfb-composite \
    -c 000000 \
    -l background.png:0:0:255 \
    -l status_icon.png:20:20:255 \
    -l watermark.png:700:500:80
```

## License

MIT
