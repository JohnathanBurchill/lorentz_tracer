cmake_minimum_required(VERSION 3.20)
project(lorentz_tracer C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Default to Release build for optimization (-O2 enables auto-vectorization)
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

# ---- raylib via FetchContent ----
include(FetchContent)

FetchContent_Declare(
    raylib
    GIT_REPOSITORY https://github.com/raysan5/raylib.git
    GIT_TAG        5.5
    GIT_SHALLOW    TRUE
)
set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(BUILD_GAMES OFF CACHE BOOL "" FORCE)
if(EMSCRIPTEN)
    set(PLATFORM Web CACHE STRING "" FORCE)
endif()
FetchContent_MakeAvailable(raylib)

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
    src/nax_axis.c
    src/nax.c
    src/boris.c
    src/gc.c
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

# ---- Platform-specific ----
if(EMSCRIPTEN)
    # WASM build: raylib detects PLATFORM_WEB automatically
    set(EM_FLAGS "-s USE_GLFW=3 -s ASYNCIFY -s TOTAL_MEMORY=335544320")
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
