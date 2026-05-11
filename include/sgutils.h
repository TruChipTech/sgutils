/*
 * sgutils - Linux Screen Graphics Utilities
 * Copyright (c) 2026 Anand <anand@truchiptechnology.com> - MIT License
 * Date: 2026-05-07
 *
 * sgutils.h - Unified public API header
 *
 * Include this single header to access the complete sgutils library.
 *
 * Example:
 *   #include <sgutils/sgutils.h>
 *
 * Link with:
 *   gcc myapp.c -lsgutils -lm
 *   or: gcc myapp.c $(pkg-config --cflags --libs sgutils)
 */

#ifndef SGUTILS_H
#define SGUTILS_H

/* ── Version ─────────────────────────────────────────────── */
#define SGUTILS_VERSION_MAJOR 2
#define SGUTILS_VERSION_MINOR 0
#define SGUTILS_VERSION_PATCH 0
#define SGUTILS_VERSION_STRING "2.0.0"

/* ── Core: pixel types, images, drawing, transforms ──────── */
#include "draw.h"

/* ── Display backends ────────────────────────────────────── */
#include "framebuffer.h"

#ifdef ENABLE_DRM
#include "sgdrm.h"
#endif

/* ── Compositing ─────────────────────────────────────────── */
#include "compositing.h"

/* ── Text rendering ──────────────────────────────────────── */
#ifdef ENABLE_TEXT
#include "text.h"
#endif

/* ── Image loaders ───────────────────────────────────────── */
#ifdef ENABLE_PNG
#include "img-png.h"
#endif

#ifdef ENABLE_JPEG
#include "img-jpeg.h"
#endif

#ifdef ENABLE_SVG
#include "img-svg.h"
#endif

#ifdef ENABLE_BMP
#include "img-bmp.h"
#endif

#endif /* SGUTILS_H */
