// This file is part of beameup, a presenter console for beamer PDF talks.
// Copyright (C) 2026 Johnathan K Burchill
// SPDX-License-Identifier: GPL-3.0-or-later

// A raylib program shown in a box on a beamer slide by beameup.
//
// A slide asks for one with \href{run:script.sh?app}{poster}: beameup runs
// the script with /bin/sh when the projector shows the slide, and stops it
// (SIGTERM to its process group) when the projector leaves. The program, run
// by the script, draws as usual into a hidden window; each frame it hands its
// pixels to beameup, which shows them in the box on both screens, and it
// takes the mouse and keys beameup passes on while the pointer is over the
// box. Outside beameup every call below does nothing.
//
// In one source file of the program, define the implementation and call:
//
//     #define BEAMEUP_EMBED_IMPLEMENTATION
//     #include "beameup_embed.h"
//
//     SetConfigFlags(beameup_embed_flags(FLAG_WINDOW_HIGHDPI));
//     InitWindow(w, h, title);
//     beameup_embed_init();          // after any window sizing of its own
//     while (...) {
//         beameup_embed_input();     // before reading the mouse and keys
//         ...
//         BeginDrawing();
//         ...
//         beameup_embed_frame();     // last, before EndDrawing
//         EndDrawing();
//     }
//
// It compiles as C99 or C++ and needs raylib's rlgl.h and POSIX shared
// memory (macOS, Linux). beameup keeps the master copy in embed/; a program
// carries a copy, which must match beameup's BEAMEUP_EMBED_VERSION.

#ifndef BEAMEUP_EMBED_H
#define BEAMEUP_EMBED_H

#include <stdint.h>

#define BEAMEUP_EMBED_MAGIC 0x62656d75u
#define BEAMEUP_EMBED_VERSION 1
// the name of the shared memory, in the program's environment
#define BEAMEUP_EMBED_ENV "BEAMEUP_EMBED"
#define BEAMEUP_EMBED_EVENTS 256

enum
{
    BEAMEUP_EMBED_KEY = 1,
    BEAMEUP_EMBED_CHAR,
    BEAMEUP_EMBED_BUTTON,
    BEAMEUP_EMBED_MOVE,
    BEAMEUP_EMBED_SCROLL
};

// one input event, in GLFW's terms, as raylib's own callbacks take it: a key
// or button with its action (0 up, 1 down, 2 repeat) and modifiers (1 shift,
// 2 control, 4 alt, 8 super), a typed codepoint, the pointer's position in
// the program's window, or a scroll
typedef struct
{
    int32_t type;
    int32_t code;
    int32_t action;
    int32_t mods;
    float x;
    float y;
} BeameupEmbedEvent;

// The shared memory: this header, then two frame buffers of max_w by max_h
// RGBA pixels, bottom row first, as glReadPixels leaves them. The program
// writes a frame into the buffer beameup is not reading, then counts it in
// frame; frame % 2 is the buffer last written. Events go round a ring the
// other way, written by beameup and taken by the program.
typedef struct BeameupEmbedShared
{
    uint32_t magic;
    uint32_t version;
    // beameup's: the frame size it asks for, which the buffers hold, and its
    // process, which the program outlives by no more than a second
    int32_t max_w;
    int32_t max_h;
    int32_t owner;
    // the program's: its window in its own mouse coordinates, and the last
    // frame's size and count
    int32_t screen_w;
    int32_t screen_h;
    int32_t frame_w;
    int32_t frame_h;
    uint32_t frame;
    // the event ring: events written by beameup, and taken by the program
    uint32_t ev_write;
    uint32_t ev_read;
    BeameupEmbedEvent events[BEAMEUP_EMBED_EVENTS];
} BeameupEmbedShared;

// where frame buffer i starts, past the header, on a cache line
#define BEAMEUP_EMBED_PIXELS ((sizeof(BeameupEmbedShared) + 63) / 64 * 64)
#define BEAMEUP_EMBED_SIZE(w, h) (BEAMEUP_EMBED_PIXELS + 2 * (size_t)(w) * (size_t)(h) * 4)

#ifdef BEAMEUP_EMBED_IMPLEMENTATION

#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#include "raylib.h"
#include "rlgl.h"
#ifdef __APPLE__
#define GL_SILENCE_DEPRECATION
#include <OpenGL/gl.h>
#include <objc/message.h>
#include <objc/runtime.h>
#else
#include <GL/gl.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif
// raylib's own GLFW, linked in with it; its callbacks are raylib's input
struct GLFWwindow;
typedef void (*BeameupKeyFn)(struct GLFWwindow *, int, int, int, int);
typedef void (*BeameupCharFn)(struct GLFWwindow *, unsigned int);
typedef void (*BeameupButtonFn)(struct GLFWwindow *, int, int, int);
typedef void (*BeameupPosFn)(struct GLFWwindow *, double, double);
typedef void (*BeameupScrollFn)(struct GLFWwindow *, double, double);
struct GLFWwindow *glfwGetCurrentContext(void);
BeameupKeyFn glfwSetKeyCallback(struct GLFWwindow *, BeameupKeyFn);
BeameupCharFn glfwSetCharCallback(struct GLFWwindow *, BeameupCharFn);
BeameupButtonFn glfwSetMouseButtonCallback(struct GLFWwindow *, BeameupButtonFn);
BeameupPosFn glfwSetCursorPosCallback(struct GLFWwindow *, BeameupPosFn);
BeameupScrollFn glfwSetScrollCallback(struct GLFWwindow *, BeameupScrollFn);
#ifdef __cplusplus
}
#endif

static struct
{
    BeameupEmbedShared *s;
    struct GLFWwindow *win;
    BeameupKeyFn key;
    BeameupCharFn chr;
    BeameupButtonFn button;
    BeameupPosFn pos;
    BeameupScrollFn scroll;
    int frames;
    // a window with more pixels than the frame: its picture copied off the
    // screen, and drawn down to the frame's size
    RenderTexture2D full;
    RenderTexture2D small;
} beameup_embed;

static inline int beameup_embedded(void)
{
    return getenv(BEAMEUP_EMBED_ENV) != NULL;
}

// Embedded, the window is hidden and never takes the focus from beameup. It
// is HiDPI whatever the program asks: on a Retina display without it, macOS
// draws a point to a pixel while GLFW reports two after a resize.
static inline unsigned int beameup_embed_flags(unsigned int flags)
{
    if (!beameup_embedded()) return flags;
    return flags | FLAG_WINDOW_HIGHDPI | FLAG_WINDOW_HIDDEN | FLAG_WINDOW_UNFOCUSED;
}

#ifdef __APPLE__
// no Dock icon, and no App Nap for a program without a visible window
static inline void beameup_embed_background(void)
{
    typedef id (*Get)(id, SEL);
    id app = ((Get)objc_msgSend)((id)objc_getClass("NSApplication"), sel_registerName("sharedApplication"));
    ((void (*)(id, SEL, long))objc_msgSend)(app, sel_registerName("setActivationPolicy:"), 1);
    id info = ((Get)objc_msgSend)((id)objc_getClass("NSProcessInfo"), sel_registerName("processInfo"));
    id reason = ((id (*)(id, SEL, const char *))objc_msgSend)((id)objc_getClass("NSString"), sel_registerName("stringWithUTF8String:"), "shown on a slide by beameup");
    // NSActivityUserInitiated | NSActivityLatencyCritical
    id activity = ((id (*)(id, SEL, unsigned long long, id))objc_msgSend)(info, sel_registerName("beginActivityWithOptions:reason:"), 0x00FFFFFFULL | 0xFF00000000ULL, reason);
    ((Get)objc_msgSend)(activity, sel_registerName("retain"));
}
#endif

static inline void beameup_embed_init(void)
{
    const char *name = getenv(BEAMEUP_EMBED_ENV);
    if (!name) return;
    int fd = shm_open(name, O_RDWR, 0);
    struct stat st;
    void *mem = MAP_FAILED;
    if (fd >= 0 && fstat(fd, &st) == 0 && (size_t)st.st_size >= sizeof(BeameupEmbedShared)) {
        mem = mmap(NULL, (size_t)st.st_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    }
    if (fd >= 0) close(fd);
    BeameupEmbedShared *s = (BeameupEmbedShared *)mem;
    if (mem == MAP_FAILED || s->magic != BEAMEUP_EMBED_MAGIC || s->version != BEAMEUP_EMBED_VERSION
        || (size_t)st.st_size < BEAMEUP_EMBED_SIZE(s->max_w, s->max_h)) {
        // a hidden window nobody shows would run unseen
        fprintf(stderr, "beameup_embed: cannot use the shared memory %s\n", name);
        exit(EXIT_FAILURE);
    }
    beameup_embed.s = s;
    // take raylib's callbacks by setting none and putting them back
    struct GLFWwindow *w = glfwGetCurrentContext();
    beameup_embed.win = w;
    beameup_embed.key = glfwSetKeyCallback(w, NULL);
    glfwSetKeyCallback(w, beameup_embed.key);
    beameup_embed.chr = glfwSetCharCallback(w, NULL);
    glfwSetCharCallback(w, beameup_embed.chr);
    beameup_embed.button = glfwSetMouseButtonCallback(w, NULL);
    glfwSetMouseButtonCallback(w, beameup_embed.button);
    beameup_embed.pos = glfwSetCursorPosCallback(w, NULL);
    glfwSetCursorPosCallback(w, beameup_embed.pos);
    beameup_embed.scroll = glfwSetScrollCallback(w, NULL);
    glfwSetScrollCallback(w, beameup_embed.scroll);
    // beameup decides when the program ends; Esc is just a key
    SetExitKey(KEY_NULL);
    // the program lays out a screen of the frame's size, in points
    SetWindowSize(s->max_w, s->max_h);
#ifdef __APPLE__
    beameup_embed_background();
#endif
}

static inline void beameup_embed_input(void)
{
    BeameupEmbedShared *s = beameup_embed.s;
    if (!s) return;
    // about once a second: beameup gone, nobody sees the program
    if (++beameup_embed.frames % 60 == 0 && kill(s->owner, 0) != 0) exit(EXIT_SUCCESS);
    uint32_t end = __atomic_load_n(&s->ev_write, __ATOMIC_ACQUIRE);
    struct GLFWwindow *w = beameup_embed.win;
    for (uint32_t i = s->ev_read; i != end; i++) {
        const BeameupEmbedEvent *e = &s->events[i % BEAMEUP_EMBED_EVENTS];
        if (e->type == BEAMEUP_EMBED_KEY && beameup_embed.key) beameup_embed.key(w, e->code, 0, e->action, e->mods);
        else if (e->type == BEAMEUP_EMBED_CHAR && beameup_embed.chr) beameup_embed.chr(w, (unsigned int)e->code);
        else if (e->type == BEAMEUP_EMBED_BUTTON && beameup_embed.button) beameup_embed.button(w, e->code, e->action, e->mods);
        else if (e->type == BEAMEUP_EMBED_MOVE && beameup_embed.pos) beameup_embed.pos(w, e->x, e->y);
        else if (e->type == BEAMEUP_EMBED_SCROLL && beameup_embed.scroll) beameup_embed.scroll(w, e->x, e->y);
    }
    __atomic_store_n(&s->ev_read, end, __ATOMIC_RELEASE);
}

static inline void beameup_embed_frame(void)
{
    BeameupEmbedShared *s = beameup_embed.s;
    if (!s) return;
    int rw = GetRenderWidth();
    int rh = GetRenderHeight();
    int w = rw;
    int h = rh;
    uint32_t next = s->frame + 1;
    unsigned char *px = (unsigned char *)s + BEAMEUP_EMBED_PIXELS + (size_t)(next % 2) * (size_t)s->max_w * (size_t)s->max_h * 4;
    // what is batched is drawn first
    rlDrawRenderBatchActive();
    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    if (rw <= s->max_w && rh <= s->max_h) {
        glReadPixels(0, 0, w, h, GL_RGBA, GL_UNSIGNED_BYTE, px);
    } else {
        // more pixels than the frame holds (a Retina display): the picture
        // is copied off the screen and drawn down, smoothed, to its points
        RenderTexture2D *full = &beameup_embed.full;
        RenderTexture2D *small = &beameup_embed.small;
        w = GetScreenWidth() < s->max_w ? GetScreenWidth() : s->max_w;
        h = GetScreenHeight() < s->max_h ? GetScreenHeight() : s->max_h;
        if (full->texture.width != rw || full->texture.height != rh) {
            if (full->id) UnloadRenderTexture(*full);
            *full = LoadRenderTexture(rw, rh);
            SetTextureFilter(full->texture, TEXTURE_FILTER_BILINEAR);
        }
        if (small->texture.width != w || small->texture.height != h) {
            if (small->id) UnloadRenderTexture(*small);
            *small = LoadRenderTexture(w, h);
        }
        rlBindFramebuffer(RL_READ_FRAMEBUFFER, 0);
        rlBindFramebuffer(RL_DRAW_FRAMEBUFFER, full->id);
        rlBlitFramebuffer(0, 0, rw, rh, 0, 0, rw, rh, 0x00004000);
        rlBindFramebuffer(RL_DRAW_FRAMEBUFFER, 0);
        Rectangle from = { 0, 0, (float)rw, (float)-rh };
        Rectangle to = { 0, 0, (float)w, (float)h };
        Vector2 origin = { 0, 0 };
        BeginTextureMode(*small);
        DrawTexturePro(full->texture, from, to, origin, 0, WHITE);
        EndTextureMode();
        rlBindFramebuffer(RL_READ_FRAMEBUFFER, small->id);
        glReadPixels(0, 0, w, h, GL_RGBA, GL_UNSIGNED_BYTE, px);
        rlBindFramebuffer(RL_READ_FRAMEBUFFER, 0);
    }
    s->screen_w = GetScreenWidth();
    s->screen_h = GetScreenHeight();
    s->frame_w = w;
    s->frame_h = h;
    __atomic_store_n(&s->frame, next, __ATOMIC_RELEASE);
}

#endif // BEAMEUP_EMBED_IMPLEMENTATION

#endif // BEAMEUP_EMBED_H
