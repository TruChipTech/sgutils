cmake_minimum_required(VERSION 3.14)

project(sgutils
    VERSION 2.0.0
    DESCRIPTION "Linux Screen Graphics Utilities"
    LANGUAGES C
)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# ── Feature options ──────────────────────────────────────────
option(ENABLE_PNG   "Enable PNG image support (requires libpng)"     ON)
option(ENABLE_JPEG  "Enable JPEG image support (requires libjpeg)"   ON)
option(ENABLE_SVG   "Enable SVG image support (requires librsvg)"    ON)
option(ENABLE_BMP   "Enable BMP image support (no dependencies)"     ON)
option(ENABLE_TEXT  "Enable FreeType text rendering"                  ON)
option(ENABLE_DRM   "Enable DRM/KMS display support (requires libdrm)" ON)
option(BUILD_SHARED_LIBS "Build shared library (in addition to static)" ON)
option(BUILD_TOOLS  "Build CLI tools"                                ON)

# ── Compiler flags ───────────────────────────────────────────
add_compile_options(-Wall -Wextra -Wpedantic)
add_compile_definitions(VERSION="${PROJECT_VERSION}")

# ── Find packages ────────────────────────────────────────────
find_package(PkgConfig REQUIRED)

if(ENABLE_PNG)
    pkg_check_modules(PNG REQUIRED libpng)
endif()

if(ENABLE_JPEG)
    find_library(JPEG_LIBRARY NAMES jpeg)
    if(NOT JPEG_LIBRARY)
        message(FATAL_ERROR "libjpeg not found, disable with -DENABLE_JPEG=OFF")
    endif()
endif()

if(ENABLE_SVG)
    pkg_check_modules(SVG REQUIRED librsvg-2.0 glib-2.0)
endif()

if(ENABLE_TEXT)
    pkg_check_modules(FREETYPE REQUIRED freetype2)
endif()

if(ENABLE_DRM)
    pkg_check_modules(DRM REQUIRED libdrm)
endif()

# ── Core library (shared between all tools) ──────────────────
set(CORE_SOURCES
    src/draw.c
    src/framebuffer.c
    src/compositing.c
)

set(CORE_COMPILE_DEFS "")
set(CORE_INCLUDE_DIRS include)
set(CORE_LINK_LIBS m)

if(ENABLE_PNG)
    list(APPEND CORE_SOURCES src/img-png.c)
    list(APPEND CORE_COMPILE_DEFS ENABLE_PNG)
    list(APPEND CORE_INCLUDE_DIRS ${PNG_INCLUDE_DIRS})
    list(APPEND CORE_LINK_LIBS ${PNG_LIBRARIES})
endif()

if(ENABLE_JPEG)
    list(APPEND CORE_SOURCES src/img-jpeg.c)
    list(APPEND CORE_COMPILE_DEFS ENABLE_JPEG)
    list(APPEND CORE_LINK_LIBS ${JPEG_LIBRARY})
endif()

if(ENABLE_SVG)
    list(APPEND CORE_SOURCES src/img-svg.c)
    list(APPEND CORE_COMPILE_DEFS ENABLE_SVG)
    list(APPEND CORE_INCLUDE_DIRS ${SVG_INCLUDE_DIRS})
    list(APPEND CORE_LINK_LIBS ${SVG_LIBRARIES})
endif()

if(ENABLE_BMP)
    list(APPEND CORE_SOURCES src/img-bmp.c)
    list(APPEND CORE_COMPILE_DEFS ENABLE_BMP)
endif()

if(ENABLE_TEXT)
    list(APPEND CORE_SOURCES src/text.c)
    list(APPEND CORE_COMPILE_DEFS ENABLE_TEXT)
    list(APPEND CORE_INCLUDE_DIRS ${FREETYPE_INCLUDE_DIRS})
    list(APPEND CORE_LINK_LIBS ${FREETYPE_LIBRARIES})
endif()

# Core static library
add_library(sgutils_core STATIC ${CORE_SOURCES})
target_include_directories(sgutils_core PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include/sgutils>
    ${CORE_INCLUDE_DIRS}
)
target_compile_definitions(sgutils_core PUBLIC ${CORE_COMPILE_DEFS})
target_link_libraries(sgutils_core PUBLIC ${CORE_LINK_LIBS})
set_target_properties(sgutils_core PROPERTIES POSITION_INDEPENDENT_CODE ON)

# ── DRM library (optional) ──────────────────────────────────
if(ENABLE_DRM)
    add_library(sgutils_drm STATIC src/sgdrm.c)
    target_include_directories(sgutils_drm PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include/sgutils>
        ${DRM_INCLUDE_DIRS}
    )
    target_compile_definitions(sgutils_drm PUBLIC ${CORE_COMPILE_DEFS})
    target_link_libraries(sgutils_drm PUBLIC sgutils_core ${DRM_LIBRARIES})
    set_target_properties(sgutils_drm PROPERTIES POSITION_INDEPENDENT_CODE ON)
endif()

# ── Installable libraries ───────────────────────────────────
# Combine all objects into a single libsgutils

set(ALL_LIB_SOURCES ${CORE_SOURCES})
if(ENABLE_DRM)
    list(APPEND ALL_LIB_SOURCES src/sgdrm.c)
endif()

# Static library
add_library(sgutils_static STATIC ${ALL_LIB_SOURCES})
set_target_properties(sgutils_static PROPERTIES OUTPUT_NAME sgutils)
target_include_directories(sgutils_static PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include/sgutils>
    ${CORE_INCLUDE_DIRS}
)
target_compile_definitions(sgutils_static PUBLIC ${CORE_COMPILE_DEFS})
target_link_libraries(sgutils_static PUBLIC ${CORE_LINK_LIBS})
if(ENABLE_DRM)
    target_include_directories(sgutils_static PUBLIC ${DRM_INCLUDE_DIRS})
    target_link_libraries(sgutils_static PUBLIC ${DRM_LIBRARIES})
endif()

# Shared library
if(BUILD_SHARED_LIBS)
    add_library(sgutils_shared SHARED ${ALL_LIB_SOURCES})
    set_target_properties(sgutils_shared PROPERTIES
        OUTPUT_NAME sgutils
        VERSION ${PROJECT_VERSION}
        SOVERSION ${PROJECT_VERSION_MAJOR}
    )
    target_include_directories(sgutils_shared PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include/sgutils>
        ${CORE_INCLUDE_DIRS}
    )
    target_compile_definitions(sgutils_shared PUBLIC ${CORE_COMPILE_DEFS})
    target_link_libraries(sgutils_shared PUBLIC ${CORE_LINK_LIBS})
    if(ENABLE_DRM)
        target_include_directories(sgutils_shared PUBLIC ${DRM_INCLUDE_DIRS})
        target_link_libraries(sgutils_shared PUBLIC ${DRM_LIBRARIES})
    endif()
endif()

# ── Helper: add a framebuffer tool ───────────────────────────
if(BUILD_TOOLS)

function(add_fb_tool name source)
    add_executable(${name} ${source})
    target_link_libraries(${name} PRIVATE sgutils_core)
endfunction()

# ── Helper: add a DRM tool ───────────────────────────────────
function(add_drm_tool name source)
    if(ENABLE_DRM)
        add_executable(${name} ${source})
        target_link_libraries(${name} PRIVATE sgutils_drm)
    endif()
endfunction()

# ── Framebuffer tools ────────────────────────────────────────
add_fb_tool(sgfb-describe  src/sgfb-describe.c)
add_fb_tool(sgfb-clear     src/sgfb-clear.c)
add_fb_tool(sgfb-ttymode   src/sgfb-ttymode.c)
add_fb_tool(sgfb-composite src/sgfb-composite.c)

if(ENABLE_TEXT)
    add_fb_tool(sgfb-text src/sgfb-text.c)
endif()

# Image draw tools (all built from sgfb-imgdraw.c)
if(ENABLE_PNG)
    add_fb_tool(sgfb-pngdraw src/sgfb-imgdraw.c)
endif()
if(ENABLE_JPEG)
    add_fb_tool(sgfb-jpgdraw src/sgfb-imgdraw.c)
endif()
if(ENABLE_SVG)
    add_fb_tool(sgfb-svgdraw src/sgfb-imgdraw.c)
endif()
if(ENABLE_BMP)
    add_fb_tool(sgfb-bmpdraw src/sgfb-imgdraw.c)
endif()

# ── DRM tools ────────────────────────────────────────────────
add_drm_tool(sgdrm-describe  src/sgdrm-describe.c)
add_drm_tool(sgdrm-clear     src/sgdrm-clear.c)
add_drm_tool(sgdrm-composite src/sgdrm-composite.c)

if(ENABLE_TEXT AND ENABLE_DRM)
    add_drm_tool(sgdrm-text src/sgdrm-text.c)
endif()

if(ENABLE_DRM)
    if(ENABLE_PNG)
        add_drm_tool(sgdrm-pngdraw src/sgdrm-imgdraw.c)
    endif()
    if(ENABLE_JPEG)
        add_drm_tool(sgdrm-jpgdraw src/sgdrm-imgdraw.c)
    endif()
    if(ENABLE_SVG)
        add_drm_tool(sgdrm-svgdraw src/sgdrm-imgdraw.c)
    endif()
    if(ENABLE_BMP)
        add_drm_tool(sgdrm-bmpdraw src/sgdrm-imgdraw.c)
    endif()
endif()

endif() # BUILD_TOOLS

# ── Install ──────────────────────────────────────────────────
include(GNUInstallDirs)

# Install libraries
set(INSTALL_LIBS sgutils_static)
if(BUILD_SHARED_LIBS)
    list(APPEND INSTALL_LIBS sgutils_shared)
endif()

install(TARGETS ${INSTALL_LIBS}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

# Install headers
install(DIRECTORY include/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/sgutils
    FILES_MATCHING PATTERN "*.h"
)

# Install pkg-config file
set(PREFIX "${CMAKE_INSTALL_PREFIX}")
set(LIBDIR "${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_LIBDIR}")
set(INCLUDEDIR "${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_INCLUDEDIR}")
string(REPLACE ";" " " LIBS_PRIVATE "${CORE_LINK_LIBS}")
if(ENABLE_DRM)
    string(REPLACE ";" " " _drm_libs "${DRM_LIBRARIES}")
    set(LIBS_PRIVATE "${LIBS_PRIVATE} ${_drm_libs}")
endif()
configure_file(sgutils.pc.in ${CMAKE_BINARY_DIR}/sgutils.pc @ONLY)
install(FILES ${CMAKE_BINARY_DIR}/sgutils.pc
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig
)

# Install tools (if built)
if(BUILD_TOOLS)
    set(ALL_TOOLS
        sgfb-describe sgfb-clear sgfb-ttymode sgfb-composite
    )

    if(ENABLE_TEXT)
        list(APPEND ALL_TOOLS sgfb-text)
    endif()
    if(ENABLE_PNG)
        list(APPEND ALL_TOOLS sgfb-pngdraw)
    endif()
    if(ENABLE_JPEG)
        list(APPEND ALL_TOOLS sgfb-jpgdraw)
    endif()
    if(ENABLE_SVG)
        list(APPEND ALL_TOOLS sgfb-svgdraw)
    endif()
    if(ENABLE_BMP)
        list(APPEND ALL_TOOLS sgfb-bmpdraw)
    endif()

    if(ENABLE_DRM)
        list(APPEND ALL_TOOLS sgdrm-describe sgdrm-clear sgdrm-composite)
        if(ENABLE_TEXT)
            list(APPEND ALL_TOOLS sgdrm-text)
        endif()
        if(ENABLE_PNG)
            list(APPEND ALL_TOOLS sgdrm-pngdraw)
        endif()
        if(ENABLE_JPEG)
            list(APPEND ALL_TOOLS sgdrm-jpgdraw)
        endif()
        if(ENABLE_SVG)
            list(APPEND ALL_TOOLS sgdrm-svgdraw)
        endif()
        if(ENABLE_BMP)
            list(APPEND ALL_TOOLS sgdrm-bmpdraw)
        endif()
    endif()

    install(TARGETS ${ALL_TOOLS}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    )
endif()

# Install man pages
install(DIRECTORY man/
    DESTINATION ${CMAKE_INSTALL_MANDIR}/man1
    FILES_MATCHING PATTERN "*.1"
)

# ── Summary ──────────────────────────────────────────────────
message(STATUS "")
message(STATUS "sgutils ${PROJECT_VERSION} build configuration:")
message(STATUS "  PNG support ......... ${ENABLE_PNG}")
message(STATUS "  JPEG support ........ ${ENABLE_JPEG}")
message(STATUS "  SVG support ......... ${ENABLE_SVG}")
message(STATUS "  BMP support ......... ${ENABLE_BMP}")
message(STATUS "  FreeType text ....... ${ENABLE_TEXT}")
message(STATUS "  DRM/KMS support ..... ${ENABLE_DRM}")
message(STATUS "")
