cmake_minimum_required(VERSION 3.20)
project(lorentz_tracer C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Default to Release build for optimization (-O2 enables auto-vectorization)
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

# ---- raylib 6: system install if present, else FetchContent ----
if(NOT EMSCRIPTEN AND NOT CMAKE_CROSSCOMPILING)
    find_package(raylib 6.0 CONFIG QUIET)
endif()

if(raylib_FOUND)
    message(STATUS "Using system raylib ${raylib_VERSION}: ${raylib_DIR}")
else()
    # Release tarball (~50 MB) rather than a git clone, which pulls ~150 MB
    # and prints nothing while it does.
    message(STATUS "Downloading raylib 6.2 (~50 MB)...")
    include(FetchContent)
    FetchContent_Declare(
        raylib
        URL      https://github.com/raysan5/raylib/archive/refs/tags/6.2.tar.gz
        URL_HASH SHA256=df7a13129b1bec9e73a6962efff170beba310ee6eab8f94cd000b7316b4db5ee
        DOWNLOAD_EXTRACT_TIMESTAMP TRUE
    )
    set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
    set(BUILD_GAMES OFF CACHE BOOL "" FORCE)
    if(EMSCRIPTEN)
        set(PLATFORM Web CACHE STRING "" FORCE)
    endif()
    FetchContent_MakeAvailable(raylib)
endif()

# ---- raygui (vendored single header) ----
# raygui.h should be in third_party/

# ---- Optional: libav for video recording (native builds only) ----
if(NOT EMSCRIPTEN AND NOT CMAKE_CROSSCOMPILING)
    find_package(PkgConfig QUIET)
    if(PkgConfig_FOUND)
        pkg_check_modules(LIBAV
            libavcodec libavformat libavutil libswscale)
    endif()
endif()

# ---- Sources ----
set(SOURCES
    src/main.c
    src/app.c
    src/field.c
    src/field_circular.c
    src/field_grad_b.c
    src/field_higher_grad.c
    src/field_sinusoidal.c
    src/field_nonphysical.c
    src/field_dipole.c
    src/field_bottle.c
    src/field_tokamak.c
    src/field_stellarator.c
    src/field_torus.c
    src/field_harris.c
    src/field_turbulence.c
    src/field_t89.c
    src/nax_axis.c
    src/nax.c
    src/boris.c
    src/gc.c
    src/drivers.c
    src/trail.c
    src/diagnostics.c
    src/ui.c
    src/help.c
    src/playback.c
    src/tutorial.c
    src/i18n.c
    src/explorer.c
    src/render3d.c
    src/render2d.c
    src/recorder.c
    src/pdf_export.c
)
if(APPLE)
    list(APPEND SOURCES src/platform_macos.c)
endif()

add_executable(lorentz_tracer ${SOURCES})

target_include_directories(lorentz_tracer PRIVATE
    src
    third_party
)

target_link_libraries(lorentz_tracer PRIVATE raylib)
if(NOT EMSCRIPTEN AND (NOT WIN32 OR NOT MSVC))
    target_link_libraries(lorentz_tracer PRIVATE m)
endif()

# MSVC does not declare M_PI and friends from <math.h> without this.
if(MSVC)
    target_compile_definitions(lorentz_tracer PRIVATE _USE_MATH_DEFINES)
endif()

# ---- libav integration ----
if(LIBAV_FOUND)
    target_compile_definitions(lorentz_tracer PRIVATE HAS_LIBAV)
    target_include_directories(lorentz_tracer PRIVATE ${LIBAV_INCLUDE_DIRS})
    target_link_libraries(lorentz_tracer PRIVATE ${LIBAV_LIBRARIES})
    target_link_directories(lorentz_tracer PRIVATE ${LIBAV_LIBRARY_DIRS})
    message(STATUS "libav found: video recording enabled")
else()
    message(STATUS "libav not found: video recording disabled (stub)")
endif()

# ---- beameup: run on a beamer slide ----
# third_party/beameup_embed.h is GPL-3.0-or-later, so it is opt-in; it needs
# POSIX shared memory (macOS, Linux).
option(BEAMEUP_EMBED "Run in a box on a beamer slide in beameup (GPL-3.0 code)" OFF)
if(BEAMEUP_EMBED AND NOT EMSCRIPTEN AND NOT WIN32)
    target_compile_definitions(lorentz_tracer PRIVATE BEAMEUP_EMBED)
    if(NOT APPLE)
        find_package(OpenGL REQUIRED)
        target_link_libraries(lorentz_tracer PRIVATE OpenGL::GL)
    endif()
    message(STATUS "beameup embedding enabled")
endif()

# ---- Platform-specific ----
if(EMSCRIPTEN)
    # WASM build: raylib detects PLATFORM_WEB automatically
    set(EM_FLAGS "-s USE_GLFW=3 -s ASYNCIFY -s TOTAL_MEMORY=738197504")
    set(EM_FLAGS "${EM_FLAGS} -s ALLOW_MEMORY_GROWTH=1")
    set(EM_FLAGS "${EM_FLAGS} --preload-file ${CMAKE_SOURCE_DIR}/resources@/resources")
    set(EM_FLAGS "${EM_FLAGS} --shell-file ${CMAKE_SOURCE_DIR}/resources/shell.html")
    set_target_properties(lorentz_tracer PROPERTIES
        SUFFIX ".html"
    )
    target_link_options(lorentz_tracer PRIVATE "SHELL:${EM_FLAGS}")
    message(STATUS "Emscripten WASM build")
elseif(APPLE)
    target_link_libraries(lorentz_tracer PRIVATE
        "-framework IOKit"
        "-framework Cocoa"
        "-framework OpenGL"
        "-framework CoreVideo"
    )

    # ---- macOS .app bundle ----
    set_target_properties(lorentz_tracer PROPERTIES
        MACOSX_BUNDLE TRUE
        MACOSX_BUNDLE_INFO_PLIST ${CMAKE_SOURCE_DIR}/resources/Info.plist.in
    )

    # Icon into Resources/
    set(ICON_FILE ${CMAKE_SOURCE_DIR}/resources/lorentz_tracer.icns)
    set_source_files_properties(${ICON_FILE} PROPERTIES
        MACOSX_PACKAGE_LOCATION "Resources")
    target_sources(lorentz_tracer PRIVATE ${ICON_FILE})

    # Copy resources (fonts, scenarios) into the bundle
    add_custom_command(TARGET lorentz_tracer POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_SOURCE_DIR}/resources/fonts
            $<TARGET_BUNDLE_CONTENT_DIR:lorentz_tracer>/Resources/resources/fonts
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_SOURCE_DIR}/resources/scenarios
            $<TARGET_BUNDLE_CONTENT_DIR:lorentz_tracer>/Resources/resources/scenarios
    )
else()
    # ---- Copy resources for non-bundle builds (Linux, Windows) ----
    add_custom_command(TARGET lorentz_tracer POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_SOURCE_DIR}/resources
            $<TARGET_FILE_DIR:lorentz_tracer>/resources
    )
endif()

# ---- Windows-specific ----
if(WIN32)
    set_target_properties(lorentz_tracer PROPERTIES WIN32_EXECUTABLE TRUE)
endif()

# ---- Compiler warnings ----
if(MSVC)
    target_compile_options(lorentz_tracer PRIVATE /W4 /wd4100)
else()
    target_compile_options(lorentz_tracer PRIVATE -Wall -Wextra -Wno-unused-parameter)
endif()

# ---- SIMD: enable native instruction set for auto-vectorization ----
# -march=native lets the compiler use SSE2/AVX (x86) or NEON (ARM) for the
# batch Boris loops. Only for native builds; cross-compiles and WASM excluded.
if(NOT EMSCRIPTEN AND NOT CMAKE_CROSSCOMPILING)
    if(NOT MSVC)
        target_compile_options(lorentz_tracer PRIVATE -march=native)
    endif()
endif()
