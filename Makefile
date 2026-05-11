# sgutils - Linux Screen Graphics Utilities
# Enhanced version with bug fixes, alpha blending, compositing, and FreeType text rendering

PREFIX ?= /usr/local
BINDIR ?= $(PREFIX)/bin
MANDIR ?= $(PREFIX)/share/man/man1
LIBDIR ?= $(PREFIX)/lib
INCLUDEDIR ?= $(PREFIX)/include/sgutils
PKGCONFIGDIR ?= $(LIBDIR)/pkgconfig

CC ?= gcc
PKG_CONFIG ?= pkg-config

# Feature flags (set to 0 to disable)
ENABLE_PNG   ?= 1
ENABLE_JPEG  ?= 1
ENABLE_SVG   ?= 1
ENABLE_BMP   ?= 1
ENABLE_TEXT  ?= 1
ENABLE_DRM   ?= 1

VERSION := 2.0.0

CFLAGS  := -g -std=gnu11 -Wall -Wextra -Wpedantic -fPIC -DVERSION=\"$(VERSION)\"
LDFLAGS :=
LIBS    := -lm

# Source directories
SRCDIR  := src
INCDIR  := include
OBJDIR  := build

CFLAGS += -I$(INCDIR)

# Core sources (always included)
CORE_SRC := $(SRCDIR)/draw.c $(SRCDIR)/framebuffer.c $(SRCDIR)/compositing.c

# Optional feature sources and flags
ifeq ($(ENABLE_PNG),1)
  CFLAGS   += -DENABLE_PNG
  CORE_SRC += $(SRCDIR)/img-png.c
  LIBS     += -lpng
endif

ifeq ($(ENABLE_JPEG),1)
  CFLAGS   += -DENABLE_JPEG
  CORE_SRC += $(SRCDIR)/img-jpeg.c
  LIBS     += -ljpeg
endif

ifeq ($(ENABLE_SVG),1)
  ifeq ($(shell $(PKG_CONFIG) --exists librsvg-2.0 glib-2.0 2>/dev/null && echo yes),yes)
    SVG_CFLAGS := $(shell $(PKG_CONFIG) --cflags librsvg-2.0 glib-2.0)
    SVG_LIBS   := $(shell $(PKG_CONFIG) --libs librsvg-2.0 glib-2.0)
    CFLAGS     += -DENABLE_SVG $(SVG_CFLAGS)
    CORE_SRC   += $(SRCDIR)/img-svg.c
    LIBS       += $(SVG_LIBS)
  else
    $(warning librsvg-2.0 not found — disabling SVG support. Install librsvg2-dev to enable.)
    ENABLE_SVG := 0
  endif
endif

ifeq ($(ENABLE_BMP),1)
  CFLAGS   += -DENABLE_BMP
  CORE_SRC += $(SRCDIR)/img-bmp.c
endif

ifeq ($(ENABLE_TEXT),1)
  ifeq ($(shell $(PKG_CONFIG) --exists freetype2 2>/dev/null && echo yes),yes)
    FT_CFLAGS := $(shell $(PKG_CONFIG) --cflags freetype2)
    FT_LIBS   := $(shell $(PKG_CONFIG) --libs freetype2)
    CFLAGS    += -DENABLE_TEXT $(FT_CFLAGS)
    CORE_SRC  += $(SRCDIR)/text.c
    LIBS      += $(FT_LIBS)
  else
    $(warning freetype2 not found — disabling text support. Install libfreetype-dev to enable.)
    ENABLE_TEXT := 0
  endif
endif

# DRM/KMS support
ifeq ($(ENABLE_DRM),1)
  ifeq ($(shell $(PKG_CONFIG) --exists libdrm 2>/dev/null && echo yes),yes)
    DRM_CFLAGS := $(shell $(PKG_CONFIG) --cflags libdrm)
    DRM_LIBS   := $(shell $(PKG_CONFIG) --libs libdrm)
    CFLAGS     += -DENABLE_DRM $(DRM_CFLAGS)
    DRM_SRC    := $(SRCDIR)/sgdrm.c
    DRM_OBJ    := $(OBJDIR)/sgdrm.o
    LIBS       += $(DRM_LIBS)
  else
    $(warning libdrm not found — disabling DRM support. Install libdrm-dev to enable.)
    ENABLE_DRM := 0
  endif
endif

CORE_OBJ := $(patsubst $(SRCDIR)/%.c,$(OBJDIR)/%.o,$(CORE_SRC))

# All library objects (core + optionally DRM)
LIB_OBJ := $(CORE_OBJ)
ifeq ($(ENABLE_DRM),1)
  LIB_OBJ += $(DRM_OBJ)
endif

# Library names
SOVERSION  := 2
LIB_STATIC := libsgutils.a
LIB_SHARED := libsgutils.so.$(VERSION)
LIB_SONAME := libsgutils.so.$(SOVERSION)
LIB_LINK   := libsgutils.so

# Binary targets
BINARIES := sgfb-describe sgfb-clear sgfb-ttymode

ifeq ($(ENABLE_PNG),1)
  BINARIES += sgfb-pngdraw
endif
ifeq ($(ENABLE_JPEG),1)
  BINARIES += sgfb-jpgdraw
endif
ifeq ($(ENABLE_SVG),1)
  BINARIES += sgfb-svgdraw
endif
ifeq ($(ENABLE_BMP),1)
  BINARIES += sgfb-bmpdraw
endif
ifeq ($(ENABLE_TEXT),1)
  BINARIES += sgfb-text
endif

# Compositing tool always included
BINARIES += sgfb-composite

# DRM tools
ifeq ($(ENABLE_DRM),1)
  BINARIES += sgdrm-describe sgdrm-clear sgdrm-composite
  ifeq ($(ENABLE_PNG),1)
    BINARIES += sgdrm-pngdraw
  endif
  ifeq ($(ENABLE_JPEG),1)
    BINARIES += sgdrm-jpgdraw
  endif
  ifeq ($(ENABLE_SVG),1)
    BINARIES += sgdrm-svgdraw
  endif
  ifeq ($(ENABLE_BMP),1)
    BINARIES += sgdrm-bmpdraw
  endif
  ifeq ($(ENABLE_TEXT),1)
    BINARIES += sgdrm-text
  endif
endif

.PHONY: all clean install uninstall lib

all: $(BINARIES) lib

lib: $(LIB_STATIC) $(LIB_SHARED)

$(OBJDIR)/%.o: $(SRCDIR)/%.c | $(OBJDIR)
	$(CC) -c $(CFLAGS) $< -o $@

$(OBJDIR):
	mkdir -p $(OBJDIR)

# ── Libraries ────────────────────────────────────────────────
$(LIB_STATIC): $(LIB_OBJ)
	$(AR) rcs $@ $^

$(LIB_SHARED): $(LIB_OBJ)
	$(CC) -shared -Wl,-soname,$(LIB_SONAME) -o $@ $^ $(LIBS)
	ln -sf $(LIB_SHARED) $(LIB_SONAME)
	ln -sf $(LIB_SHARED) $(LIB_LINK)

# ── Tool binaries ────────────────────────────────────────────
sgfb-describe: $(OBJDIR)/sgfb-describe.o $(CORE_OBJ)
	$(CC) $(CFLAGS) $(LDFLAGS) $^ -o $@ $(LIBS)

sgfb-clear: $(OBJDIR)/sgfb-clear.o $(CORE_OBJ)
	$(CC) $(CFLAGS) $(LDFLAGS) $^ -o $@ $(LIBS)

sgfb-ttymode: $(OBJDIR)/sgfb-ttymode.o
	$(CC) $(CFLAGS) $(LDFLAGS) $^ -o $@ $(LIBS)

sgfb-pngdraw: $(OBJDIR)/sgfb-imgdraw.o $(CORE_OBJ)
	$(CC) $(CFLAGS) $(LDFLAGS) $^ -o $@ $(LIBS) -DFORMAT_PNG

sgfb-jpgdraw: $(OBJDIR)/sgfb-imgdraw.o $(CORE_OBJ)
	$(CC) $(CFLAGS) $(LDFLAGS) $^ -o $@ $(LIBS) -DFORMAT_JPEG

sgfb-svgdraw: $(OBJDIR)/sgfb-imgdraw.o $(CORE_OBJ)
	$(CC) $(CFLAGS) $(LDFLAGS) $^ -o $@ $(LIBS) -DFORMAT_SVG

sgfb-bmpdraw: $(OBJDIR)/sgfb-imgdraw.o $(CORE_OBJ)
	$(CC) $(CFLAGS) $(LDFLAGS) $^ -o $@ $(LIBS) -DFORMAT_BMP

sgfb-text: $(OBJDIR)/sgfb-text.o $(CORE_OBJ)
	$(CC) $(CFLAGS) $(LDFLAGS) $^ -o $@ $(LIBS)

sgfb-composite: $(OBJDIR)/sgfb-composite.o $(CORE_OBJ)
	$(CC) $(CFLAGS) $(LDFLAGS) $^ -o $@ $(LIBS)

# DRM tool binaries
sgdrm-describe: $(OBJDIR)/sgdrm-describe.o $(CORE_OBJ) $(DRM_OBJ)
	$(CC) $(CFLAGS) $(LDFLAGS) $^ -o $@ $(LIBS)

sgdrm-clear: $(OBJDIR)/sgdrm-clear.o $(CORE_OBJ) $(DRM_OBJ)
	$(CC) $(CFLAGS) $(LDFLAGS) $^ -o $@ $(LIBS)

sgdrm-pngdraw: $(OBJDIR)/sgdrm-imgdraw.o $(CORE_OBJ) $(DRM_OBJ)
	$(CC) $(CFLAGS) $(LDFLAGS) $^ -o $@ $(LIBS) -DFORMAT_PNG

sgdrm-jpgdraw: $(OBJDIR)/sgdrm-imgdraw.o $(CORE_OBJ) $(DRM_OBJ)
	$(CC) $(CFLAGS) $(LDFLAGS) $^ -o $@ $(LIBS) -DFORMAT_JPEG

sgdrm-svgdraw: $(OBJDIR)/sgdrm-imgdraw.o $(CORE_OBJ) $(DRM_OBJ)
	$(CC) $(CFLAGS) $(LDFLAGS) $^ -o $@ $(LIBS) -DFORMAT_SVG

sgdrm-bmpdraw: $(OBJDIR)/sgdrm-imgdraw.o $(CORE_OBJ) $(DRM_OBJ)
	$(CC) $(CFLAGS) $(LDFLAGS) $^ -o $@ $(LIBS) -DFORMAT_BMP

sgdrm-text: $(OBJDIR)/sgdrm-text.o $(CORE_OBJ) $(DRM_OBJ)
	$(CC) $(CFLAGS) $(LDFLAGS) $^ -o $@ $(LIBS)

sgdrm-composite: $(OBJDIR)/sgdrm-composite.o $(CORE_OBJ) $(DRM_OBJ)
	$(CC) $(CFLAGS) $(LDFLAGS) $^ -o $@ $(LIBS)

clean:
	rm -rf $(OBJDIR) $(BINARIES)
	rm -f $(LIB_STATIC) $(LIB_SHARED) $(LIB_SONAME) $(LIB_LINK)

install: all
	install -d $(DESTDIR)$(BINDIR)
	install -m 755 $(BINARIES) $(DESTDIR)$(BINDIR)/
	install -d $(DESTDIR)$(MANDIR)
	@for f in man/*.1; do \
		if [ -f "$$f" ]; then install -m 644 "$$f" $(DESTDIR)$(MANDIR)/; fi; \
	done
	install -d $(DESTDIR)$(LIBDIR)
	install -m 644 $(LIB_STATIC) $(DESTDIR)$(LIBDIR)/
	install -m 755 $(LIB_SHARED) $(DESTDIR)$(LIBDIR)/
	ln -sf $(LIB_SHARED) $(DESTDIR)$(LIBDIR)/$(LIB_SONAME)
	ln -sf $(LIB_SHARED) $(DESTDIR)$(LIBDIR)/$(LIB_LINK)
	install -d $(DESTDIR)$(INCLUDEDIR)
	install -m 644 $(INCDIR)/*.h $(DESTDIR)$(INCLUDEDIR)/
	install -d $(DESTDIR)$(PKGCONFIGDIR)
	sed -e 's|@PREFIX@|$(PREFIX)|g' \
	    -e 's|@LIBDIR@|$(LIBDIR)|g' \
	    -e 's|@INCLUDEDIR@|$(PREFIX)/include|g' \
	    -e 's|@VERSION@|$(VERSION)|g' \
	    -e 's|@LIBS_PRIVATE@|$(LIBS)|g' \
	    sgutils.pc.in > $(DESTDIR)$(PKGCONFIGDIR)/sgutils.pc
	ldconfig 2>/dev/null || true

uninstall:
	@for b in $(BINARIES); do rm -f $(DESTDIR)$(BINDIR)/$$b; done
	@for f in man/*.1; do \
		if [ -f "$$f" ]; then rm -f $(DESTDIR)$(MANDIR)/$$(basename $$f); fi; \
	done
	rm -f $(DESTDIR)$(LIBDIR)/$(LIB_STATIC)
	rm -f $(DESTDIR)$(LIBDIR)/$(LIB_SHARED)
	rm -f $(DESTDIR)$(LIBDIR)/$(LIB_SONAME)
	rm -f $(DESTDIR)$(LIBDIR)/$(LIB_LINK)
	rm -rf $(DESTDIR)$(INCLUDEDIR)
	rm -f $(DESTDIR)$(PKGCONFIGDIR)/sgutils.pc
	ldconfig 2>/dev/null || true
