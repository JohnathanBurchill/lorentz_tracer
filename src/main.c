#include "app.h"
#include "i18n.h"
#include "ui.h"
#include "raylib.h"
#include <string.h>
#include <stdio.h>
#include <math.h>

#ifdef __EMSCRIPTEN__
#include <emscripten/emscripten.h>
#elif defined(__APPLE__)
#include <mach-o/dyld.h>
#include <libgen.h>
#elif defined(_WIN32)
/* Include windows.h with NOGDI/NOUSER to avoid CloseWindow/ShowCursor
 * conflicts with raylib, then declare only what we need. */
#define WIN32_LEAN_AND_MEAN
#define NOGDI
#define NOUSER
#include <windows.h>
#undef near
#undef far
#elif defined(__linux__)
#include <unistd.h>
#include <libgen.h>
#endif

/* Resolve resource path: inside .app bundle on macOS, or next to executable */
static void resolve_resource_dir(char *buf, int buflen)
{
#ifdef __EMSCRIPTEN__
    snprintf(buf, buflen, "/resources");
#elif defined(__APPLE__)
    char exec_path[1024];
    uint32_t sz = sizeof(exec_path);
    if (_NSGetExecutablePath(exec_path, &sz) == 0) {
        char *dir = dirname(exec_path);
        /* If inside a .app bundle: .../Contents/MacOS/ → .../Contents/Resources/ */
        if (strstr(dir, ".app/Contents/MacOS")) {
            snprintf(buf, buflen, "%s/../Resources/resources", dir);
            return;
        }
        snprintf(buf, buflen, "%s/resources", dir);
        return;
    }
#elif defined(_WIN32)
    char exec_path[MAX_PATH];
    DWORD len = GetModuleFileNameA(NULL, exec_path, MAX_PATH);
    if (len > 0 && len < MAX_PATH) {
        /* Strip executable name to get directory */
        char *last_sep = strrchr(exec_path, '\\');
        if (last_sep) *last_sep = '\0';
        snprintf(buf, buflen, "%s\\resources", exec_path);
        return;
    }
#elif defined(__linux__)
    char exec_path[1024];
    ssize_t len = readlink("/proc/self/exe", exec_path, sizeof(exec_path) - 1);
    if (len > 0) {
        exec_path[len] = '\0';
        char *dir = dirname(exec_path);
        snprintf(buf, buflen, "%s/resources", dir);
        return;
    }
#endif
#ifndef __EMSCRIPTEN__
    snprintf(buf, buflen, "resources");
#endif
}

static AppState *g_app; /* for emscripten callback */

static void main_loop_step(void)
{
    g_app->win_w = GetScreenWidth();
    g_app->win_h = GetScreenHeight();
    app_update(g_app);
    app_render(g_app);
}

int main(int argc, char **argv)
{
    int skip_splash = 0;
    for (int i = 1; i < argc; i++)
        if (strcmp(argv[i], "--no-splash") == 0) skip_splash = 1;

    static AppState app;
    g_app = &app;
    i18n_init();
    app_init(&app);
    app_load_state(&app);
    ui_sync_from_app(&app);

    SetTraceLogLevel(LOG_NONE);
    SetConfigFlags(FLAG_MSAA_4X_HINT | FLAG_WINDOW_HIGHDPI);
    InitWindow(app.win_w, app.win_h, "Lorentz Tracer");
    SetExitKey(0);  /* Esc closes help overlay, not the window */
    SetTargetFPS(60);
    ToggleBorderlessWindowed();  /* start fullscreen */
#ifdef __APPLE__
    extern void macos_fix_window_switching(void *nswindow);
    macos_fix_window_switching(GetWindowHandle());
#endif

    app.dpi_scale = (float)GetRenderWidth() / (float)GetScreenWidth();

    /* Resolve resource directory */
    char res_dir[1024];
    resolve_resource_dir(res_dir, sizeof(res_dir));
    strncpy(app.res_dir, res_dir, sizeof(app.res_dir) - 1);

    char font_path[1100];

    /* Load TTF fonts with ASCII + Latin-1 + Latin Extended + Cyrillic + Greek */
    int font_size = (int)(16 * app.dpi_scale);
    int codepoints[1200];
    int ncp = 0;
    for (int i = 32; i < 256; i++) codepoints[ncp++] = i;     /* ASCII + Latin-1 */
    for (int i = 0x100; i <= 0x17F; i++) codepoints[ncp++] = i; /* Latin Extended-A (ĉĝŝŭ etc.) */
    for (int i = 0x180; i <= 0x24F; i++) codepoints[ncp++] = i; /* Latin Extended-B */
    for (int i = 0x400; i <= 0x4FF; i++) codepoints[ncp++] = i; /* Cyrillic */
    codepoints[ncp++] = 0x03B1;  /* α */
    codepoints[ncp++] = 0x03B2;  /* β */
    codepoints[ncp++] = 0x03B3;  /* γ */
    codepoints[ncp++] = 0x03B5;  /* ε */
    codepoints[ncp++] = 0x03BA;  /* κ */
    codepoints[ncp++] = 0x03BB;  /* λ */
    codepoints[ncp++] = 0x03BC;  /* μ */
    codepoints[ncp++] = 0x03C0;  /* π */
    codepoints[ncp++] = 0x03C6;  /* φ */
    codepoints[ncp++] = 0x03C9;  /* ω */
    codepoints[ncp++] = 0x2202;  /* ∂ */
    codepoints[ncp++] = 0x2207;  /* ∇ */
    codepoints[ncp++] = 0x00D7;  /* × */
    codepoints[ncp++] = 0x2016;  /* ‖ (parallel) */
    codepoints[ncp++] = 0x22A5;  /* ⊥ */
    codepoints[ncp++] = 0x2081;  /* ₁ */
    codepoints[ncp++] = 0x2082;  /* ₂ */
    codepoints[ncp++] = 0x2080;  /* ₀ */
    codepoints[ncp++] = 0x00B2;  /* ² */
    codepoints[ncp++] = 0x00B3;  /* ³ */
    codepoints[ncp++] = 0x2212;  /* − (minus) */
    codepoints[ncp++] = 0x00B7;  /* · (middle dot) */
    codepoints[ncp++] = 0x00A0;  /* non-breaking space */
    codepoints[ncp++] = 0x00EA;  /* ê (for ê_z unit vector) */
    codepoints[ncp++] = 0x00E2;  /* â (for â hat) */
    codepoints[ncp++] = 0x221A;  /* √ */
    codepoints[ncp++] = 0x2098;  /* ₘ */
    codepoints[ncp++] = 0x2099;  /* ₙ */
    codepoints[ncp++] = 0x2083;  /* ₃ */
    codepoints[ncp++] = 0x03B5;  /* ε (already above but harmless) */
    codepoints[ncp++] = 0x2084;  /* ₄ */

    snprintf(font_path, sizeof(font_path), "%s/fonts/Inter-Regular.ttf", res_dir);
    app.font_ui = LoadFontEx(font_path, font_size, codepoints, ncp);
    snprintf(font_path, sizeof(font_path), "%s/fonts/SourceCodePro-Regular.ttf", res_dir);
    app.font_mono = LoadFontEx(font_path, font_size, codepoints, ncp);
    SetTextureFilter(app.font_ui.texture, TEXTURE_FILTER_BILINEAR);
    SetTextureFilter(app.font_mono.texture, TEXTURE_FILTER_BILINEAR);

    ui_init(&app);

    /* If saved language needs CJK fonts, reload now */
    if (i18n_lang_info(i18n_get_lang())->needs_cjk)
        app_reload_fonts(&app);

    if (skip_splash) {
        app_reset_particle(&app);
        goto main_loop;
    }

    /* Load splash font at large size for studio branding */
    snprintf(font_path, sizeof(font_path), "%s/fonts/Inter-Regular.ttf", res_dir);
    Font splash_font = LoadFontEx(font_path, (int)(80 * app.dpi_scale), codepoints, ncp);
    SetTextureFilter(splash_font.texture, TEXTURE_FILTER_BILINEAR);

    /* ---- Splash screen ----
     * Phase 1: "STUDIO 509" fade in + tagline (0 – 3.0s)
     * Phase 2: crossfade to "Lorentz Tracer" with trajectory (2.6 – 5.0s)
     * Phase 3: hold, tap to continue (5.0s+)
     * Phase 4: breakup — elements shatter with E×B drift, revealing program */
    {
        float elapsed = 0.0f;
        int done = 0;

        /* Precompute spiral trajectory: helix with curvature drift */
        #define TRAJ_N 800
        float traj_x[TRAJ_N], traj_y[TRAJ_N], traj_depth[TRAJ_N];

        /* Breakup transition: fragments are actual characters / circles that
         * gradually separate with E×B drift + gyration, not an instant explosion. */
        #define MAX_FRAGS 300
        typedef struct {
            float x0, y0;       /* original position */
            float dx, dy;       /* accumulated displacement */
            float vx, vy;       /* random initial velocity (small) */
            Color color;
            float gyro_r, gyro_freq, gyro_phase;
            int type;           /* 0=char(splash), 1=char(ui font), 2=circle */
            char ch[4];         /* UTF-8 char for type 0,1 */
            float font_size;    /* font size for chars, radius for circles */
        } SplashFrag;
        SplashFrag frags[MAX_FRAGS];
        int n_frags = 0;
        int breaking = 0;
        float break_time = 0.0f;

        /* Deterministic RNG for fragment generation */
        #define FRAG_RNG(s) ((s) = (s) * 1103515245u + 12345u)
        #define FRAG_RNDF(s, lo, hi) \
            ((lo) + ((float)(FRAG_RNG(s) % 10000u) / 10000.0f) * ((hi) - (lo)))

        /* Smoothstep helper */
        #define SS(x) ((x)<=0?0:(x)>=1?1:(x)*(x)*(3-2*(x)))

        while (!done && !WindowShouldClose()) {
            float dt_frame = GetFrameTime();
            elapsed += dt_frame;
            int w = GetScreenWidth();
            int h = GetScreenHeight();
            float S = (w < h ? w : h) / 600.0f;
            if (S < 0.5f) S = 0.5f;

            /* Compute trajectory points (in screen coords, centered) */
            {
                float cx = w * 0.5f, cy = h * 0.55f;
                float span = w * 0.7f;
                float radius = 15.0f * S;
                float drift = -30.0f * S;
                float freq = 28.0f;
                for (int i = 0; i < TRAJ_N; i++) {
                    float t = (float)i / (TRAJ_N - 1);
                    float x_base = -span * 0.5f + span * t;
                    float y_drift = drift * t * t;
                    float phase = freq * 2.0f * 3.14159f * t;
                    float persp = 0.7f + 0.3f * t;
                    traj_x[i] = cx + x_base + radius * persp * cosf(phase) * 0.55f;
                    traj_y[i] = cy + y_drift + radius * persp * sinf(phase);
                    traj_depth[i] = cosf(phase);
                }
            }

            if (breaking) {
                /* ---- Phase 4: Breakup transition ---- */
                break_time += dt_frame;

                /* Run program behind the overlay */
                app.win_w = w;
                app.win_h = h;
                app_update(&app);
                if (app.ui_visible || app.show_help || app.playback.show_picker)
                    ui_update(&app);

                BeginDrawing();
                app_render_inner(&app);

                /* Fading dark overlay */
                float bg_fade = 1.0f - SS(break_time / 1.6f);
                DrawRectangle(0, 0, w, h,
                              (Color){8, 8, 14, (unsigned char)(255 * bg_fade)});

                /* E×B drift ramps up gradually (not instant) */
                float drift_vx = 250.0f * S;
                float drift_vy = -90.0f * S;
                float drift_f = SS(break_time / 0.7f);  /* 0→1 over 0.7s */
                float gyro_f = SS(break_time / 0.5f);   /* gyration builds up */
                float frag_alpha = 1.0f - SS((break_time - 0.6f) / 1.4f);

                for (int i = 0; i < n_frags; i++) {
                    SplashFrag *f = &frags[i];

                    /* Accumulate displacement: small random + ramping E×B drift */
                    f->dx += (f->vx + drift_vx * drift_f) * dt_frame;
                    f->dy += (f->vy + drift_vy * drift_f) * dt_frame;

                    /* Gyration offset (spiraling, grows from zero) */
                    float gx = f->gyro_r * gyro_f * cosf(f->gyro_freq * break_time + f->gyro_phase);
                    float gy = f->gyro_r * gyro_f * sinf(f->gyro_freq * break_time + f->gyro_phase);

                    float rx = f->x0 + f->dx + gx;
                    float ry = f->y0 + f->dy + gy;

                    unsigned char a = (unsigned char)(frag_alpha * f->color.a);
                    if (a < 2) continue;
                    Color c = {f->color.r, f->color.g, f->color.b, a};

                    if (f->type == 0) {
                        DrawTextEx(splash_font, f->ch, (Vector2){rx, ry},
                                   f->font_size, 0, c);
                    } else if (f->type == 1) {
                        DrawTextEx(app.font_ui, f->ch, (Vector2){rx, ry},
                                   f->font_size, 0, c);
                    } else {
                        DrawCircle((int)rx, (int)ry, f->font_size, c);
                    }
                }

                EndDrawing();
                if (break_time > 2.2f) done = 1;

            } else {
                /* ---- Phases 1-3: Normal splash ---- */

                /* Dismiss after phase 2 complete → start breakup */
                if (elapsed > 6.0f) {
                    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) ||
                        IsKeyPressed(KEY_SPACE) || IsKeyPressed(KEY_ENTER) ||
                        GetKeyPressed() != 0 || GetTouchPointCount() > 0) {
                        breaking = 1;
                        app_reset_particle(&app);

                        /* Generate character-level fragments from visible elements.
                         * Each text character becomes its own fragment so the
                         * dissolve looks like the actual title breaking apart. */
                        unsigned int seed = 42u;
                        float scale_factor = splash_font.baseSize > 0
                            ? 1.0f / (float)splash_font.baseSize : 1.0f;

                        /* Helper: create one text-character fragment */
                        #define ADD_CHAR_FRAG(font_type, ch_char, px, py, fsz, col) \
                            if (n_frags < MAX_FRAGS) { \
                                SplashFrag *_f = &frags[n_frags++]; \
                                _f->x0 = (px); _f->y0 = (py); \
                                _f->dx = _f->dy = 0; \
                                _f->ch[0] = (ch_char); _f->ch[1] = 0; \
                                _f->type = (font_type); _f->font_size = (fsz); \
                                _f->color = (col); \
                                float _a = FRAG_RNDF(seed, 0, 6.283f); \
                                float _s = FRAG_RNDF(seed, 15, 60) * S; \
                                _f->vx = _s * cosf(_a); _f->vy = _s * sinf(_a); \
                                _f->gyro_r = FRAG_RNDF(seed, 8, 35) * S; \
                                _f->gyro_freq = FRAG_RNDF(seed, 3, 9); \
                                _f->gyro_phase = FRAG_RNDF(seed, 0, 6.283f); \
                            }

                        /* "Lorentz" — per-character fragments */
                        {
                            float lt1_sz = 68.0f * S, lt1_sp = 10.0f * S;
                            float lt2_sz = 34.0f * S;
                            Vector2 sz1 = MeasureTextEx(splash_font, "Lorentz", lt1_sz, lt1_sp);
                            float blk_h = sz1.y + 6.0f * S + lt2_sz;
                            float by = h * 0.30f - blk_h * 0.5f;
                            float cx = (w - sz1.x) * 0.5f;
                            const char *str = "Lorentz";
                            for (int j = 0; str[j]; j++) {
                                ADD_CHAR_FRAG(0, str[j], cx, by, lt1_sz,
                                              ((Color){255, 250, 245, 255}));
                                int gi = GetGlyphIndex(splash_font, str[j]);
                                float adv = splash_font.glyphs[gi].advanceX;
                                if (adv <= 0) adv = (float)splash_font.recs[gi].width;
                                cx += adv * scale_factor * lt1_sz + lt1_sp;
                            }
                        }

                        /* "TRACER" — per-character fragments */
                        {
                            float lt1_sz = 68.0f * S, lt1_sp = 10.0f * S;
                            float lt2_sz = 34.0f * S, lt2_sp = 18.0f * S;
                            Vector2 sz1 = MeasureTextEx(splash_font, "Lorentz", lt1_sz, lt1_sp);
                            Vector2 sz2 = MeasureTextEx(splash_font, "TRACER", lt2_sz, lt2_sp);
                            float blk_h = sz1.y + 6.0f * S + sz2.y;
                            float by = h * 0.30f - blk_h * 0.5f;
                            float ty = by + sz1.y + 4.0f * S;
                            float cx = (w - sz2.x) * 0.5f;
                            const char *str = "TRACER";
                            for (int j = 0; str[j]; j++) {
                                ADD_CHAR_FRAG(0, str[j], cx, ty, lt2_sz,
                                              ((Color){100, 170, 240, 200}));
                                int gi = GetGlyphIndex(splash_font, str[j]);
                                float adv = splash_font.glyphs[gi].advanceX;
                                if (adv <= 0) adv = (float)splash_font.recs[gi].width;
                                cx += adv * scale_factor * lt2_sz + lt2_sp;
                            }
                        }

                        /* Trajectory helix — circle fragments (every 4th point) */
                        for (int i = 0; i < TRAJ_N && n_frags < MAX_FRAGS; i += 4) {
                            SplashFrag *f = &frags[n_frags++];
                            f->x0 = traj_x[i]; f->y0 = traj_y[i];
                            f->dx = f->dy = 0;
                            float db = 0.45f + 0.55f * (traj_depth[i] * 0.5f + 0.5f);
                            f->type = 2; f->ch[0] = 0;
                            f->font_size = (1.2f + 1.5f * db) * S;
                            f->color = (Color){(unsigned char)(80*db),
                                               (unsigned char)(180*db),
                                               255, (unsigned char)(200*db)};
                            float ang = FRAG_RNDF(seed, 0, 6.283f);
                            float spd = FRAG_RNDF(seed, 10, 60) * S;
                            f->vx = spd * cosf(ang); f->vy = spd * sinf(ang);
                            f->gyro_r = FRAG_RNDF(seed, 3, 15) * S;
                            f->gyro_freq = FRAG_RNDF(seed, 4, 12);
                            f->gyro_phase = FRAG_RNDF(seed, 0, 6.283f);
                        }

                        /* Head dot */
                        if (n_frags < MAX_FRAGS) {
                            SplashFrag *f = &frags[n_frags++];
                            f->x0 = traj_x[TRAJ_N-1]; f->y0 = traj_y[TRAJ_N-1];
                            f->dx = f->dy = 0; f->type = 2; f->ch[0] = 0;
                            f->font_size = 4.0f * S;
                            f->color = (Color){200, 240, 255, 200};
                            f->vx = FRAG_RNDF(seed, -30, 30) * S;
                            f->vy = FRAG_RNDF(seed, -30, 30) * S;
                            f->gyro_r = 15.0f * S;
                            f->gyro_freq = 6.0f;
                            f->gyro_phase = FRAG_RNDF(seed, 0, 6.283f);
                        }

                        /* Copyright — per-character fragments */
                        {
                            const char *str = "© 2026 Johnathan K. Burchill";
                            float c_sz = 11.0f * S;
                            Vector2 csz = MeasureTextEx(app.font_ui, str, c_sz, 1);
                            float cx = (w - csz.x) * 0.5f;
                            float cy_pos = h * 0.92f;
                            float ui_sf = app.font_ui.baseSize > 0
                                ? 1.0f / (float)app.font_ui.baseSize : 1.0f;
                            /* Just use the raw UTF-8 bytes; skip © (2-byte) */
                            const unsigned char *p = (const unsigned char *)str;
                            while (*p && n_frags < MAX_FRAGS) {
                                int cp; int bytes = 1;
                                if (*p < 0x80) { cp = *p; }
                                else if (*p < 0xE0) { cp = *p & 0x1F; bytes = 2;
                                    if (p[1]) cp = (cp<<6)|(p[1]&0x3F); }
                                else { cp = *p; } /* skip higher */
                                if (cp > 32) {
                                    SplashFrag *f = &frags[n_frags++];
                                    f->x0 = cx; f->y0 = cy_pos;
                                    f->dx = f->dy = 0;
                                    /* Store the UTF-8 bytes for this char */
                                    for (int b = 0; b < bytes && b < 3; b++)
                                        f->ch[b] = (char)p[b];
                                    f->ch[bytes < 4 ? bytes : 3] = 0;
                                    f->type = 1; f->font_size = c_sz;
                                    f->color = (Color){80, 90, 110, 120};
                                    float ang = FRAG_RNDF(seed, 0, 6.283f);
                                    float spd = FRAG_RNDF(seed, 10, 50) * S;
                                    f->vx = spd * cosf(ang); f->vy = spd * sinf(ang);
                                    f->gyro_r = FRAG_RNDF(seed, 2, 10) * S;
                                    f->gyro_freq = FRAG_RNDF(seed, 4, 10);
                                    f->gyro_phase = FRAG_RNDF(seed, 0, 6.283f);
                                }
                                int gi = GetGlyphIndex(app.font_ui, cp);
                                float adv = app.font_ui.glyphs[gi].advanceX;
                                if (adv <= 0) adv = (float)app.font_ui.recs[gi].width;
                                cx += adv * ui_sf * c_sz + 1.0f;
                                p += bytes;
                            }
                        }

                        #undef ADD_CHAR_FRAG

                        continue; /* next frame enters breakup path */
                    }
                }

                BeginDrawing();
                ClearBackground((Color){8, 8, 14, 255});

                /* ---- Phase 1: STUDIO 509 (0 – 1.8s) ---- */
                {
                    float fade = SS(elapsed / 0.8f);
                    float out = 1.0f - SS((elapsed - 2.4f) / 0.6f);
                    float vis = fade * out;
                    if (vis > 0.001f) {
                        const char *title = "STUDIO 509";
                        float title_sz = 62.0f * S;
                        float title_spacing = 16.0f * S;
                        Vector2 tsz = MeasureTextEx(splash_font, title, title_sz, title_spacing);
                        float tx = (w - tsz.x) * 0.5f;
                        float ty = h * 0.42f - tsz.y * 0.5f;

                        for (int g = 4; g >= 1; g--) {
                            unsigned char ga = (unsigned char)(18 * vis);
                            Color gc = {90, 140, 220, ga};
                            float off = g * 2.5f * S;
                            DrawTextEx(splash_font, title, (Vector2){tx-off, ty}, title_sz, title_spacing, gc);
                            DrawTextEx(splash_font, title, (Vector2){tx+off, ty}, title_sz, title_spacing, gc);
                            DrawTextEx(splash_font, title, (Vector2){tx, ty-off}, title_sz, title_spacing, gc);
                            DrawTextEx(splash_font, title, (Vector2){tx, ty+off}, title_sz, title_spacing, gc);
                        }
                        DrawTextEx(splash_font, title, (Vector2){tx, ty}, title_sz, title_spacing,
                                   (Color){220, 230, 255, (unsigned char)(255 * vis)});

                        float rule_w = 260.0f * S;
                        float ry = h * 0.42f + 38.0f * S;
                        DrawRectangle((int)((w - rule_w) * 0.5f), (int)ry,
                                      (int)rule_w, 1, (Color){100, 130, 200, (unsigned char)(80 * vis)});

                        const char *tag = "explanations matter";
                        float tag_sz = 18.0f * S;
                        float tag_sp = 3.5f * S;
                        float tag_grow = SS(elapsed / 2.4f);
                        unsigned char tr = (unsigned char)(140 + 80 * tag_grow);
                        unsigned char tg = (unsigned char)(155 + 70 * tag_grow);
                        unsigned char tb = (unsigned char)(190 + 50 * tag_grow);
                        unsigned char ta = (unsigned char)((140 + 80 * tag_grow) * vis);
                        Vector2 tgsz = MeasureTextEx(app.font_ui, tag, tag_sz, tag_sp);
                        DrawTextEx(app.font_ui, tag,
                                   (Vector2){(w - tgsz.x) * 0.5f, h * 0.42f + 52.0f * S},
                                   tag_sz, tag_sp, (Color){tr, tg, tb, ta});
                    }
                }

                /* ---- Phase 2: "Lorentz Tracer" + trajectory (2.6s+) ---- */
                {
                    float vis = SS((elapsed - 2.6f) / 1.5f);
                    if (vis > 0.001f) {
                        float lt1_sz = 68.0f * S;
                        float lt1_sp = 10.0f * S;
                        float lt2_sz = 34.0f * S;
                        float lt2_sp = 18.0f * S;
                        Vector2 sz1 = MeasureTextEx(splash_font, "Lorentz", lt1_sz, lt1_sp);
                        Vector2 sz2 = MeasureTextEx(splash_font, "TRACER", lt2_sz, lt2_sp);
                        float block_h = sz1.y + 6.0f * S + sz2.y;
                        float base_y = h * 0.30f - block_h * 0.5f;

                        float scale_raw = (elapsed - 2.6f) / 3.4f;
                        if (scale_raw < 0) scale_raw = 0;
                        if (scale_raw > 1) scale_raw = 1;
                        float sc = 0.85f + 0.15f * (1.0f - (1.0f - scale_raw) * (1.0f - scale_raw));

                        /* "Lorentz" */
                        {
                            float sz = lt1_sz * sc;
                            float sp = lt1_sp * sc;
                            Vector2 msz = MeasureTextEx(splash_font, "Lorentz", sz, sp);
                            float tx = (w - msz.x) * 0.5f;
                            float ty = base_y;
                            for (int g = 4; g >= 1; g--) {
                                unsigned char ga = (unsigned char)(14 * vis);
                                float off = g * 2.5f * S;
                                DrawTextEx(splash_font, "Lorentz", (Vector2){tx, ty - off}, sz, sp, (Color){255, 200, 120, ga});
                                DrawTextEx(splash_font, "Lorentz", (Vector2){tx, ty + off}, sz, sp, (Color){255, 200, 120, ga});
                                DrawTextEx(splash_font, "Lorentz", (Vector2){tx - off, ty}, sz, sp, (Color){255, 200, 120, ga});
                                DrawTextEx(splash_font, "Lorentz", (Vector2){tx + off, ty}, sz, sp, (Color){255, 200, 120, ga});
                            }
                            DrawTextEx(splash_font, "Lorentz", (Vector2){tx, ty}, sz, sp,
                                       (Color){255, 250, 245, (unsigned char)(255 * vis)});
                        }

                        /* "TRACER" */
                        {
                            float sz = lt2_sz * sc;
                            float sp = lt2_sp * sc;
                            Vector2 msz = MeasureTextEx(splash_font, "TRACER", sz, sp);
                            float tx = (w - msz.x) * 0.5f;
                            float ty = base_y + sz1.y * sc + 4.0f * S;
                            DrawTextEx(splash_font, "TRACER", (Vector2){tx, ty}, sz, sp,
                                       (Color){100, 170, 240, (unsigned char)(200 * vis)});
                        }

                        /* Animated trajectory */
                        float traj_progress = SS((elapsed - 3.5f) / 2.5f);
                        int n_vis = (int)(traj_progress * TRAJ_N);
                        if (n_vis > TRAJ_N) n_vis = TRAJ_N;

                        for (int i = 1; i < n_vis; i++) {
                            float age = 1.0f - (float)i / (float)n_vis;
                            float depth = 0.5f * (traj_depth[i] + traj_depth[i-1]);
                            float depth_bright = 0.45f + 0.55f * (depth * 0.5f + 0.5f);
                            float base_a = 200 * vis * (0.15f + 0.85f * (1.0f - age)) * depth_bright;
                            unsigned char ta = (unsigned char)(base_a > 255 ? 255 : base_a);
                            Color tc = {(unsigned char)(80 * depth_bright),
                                        (unsigned char)(180 * depth_bright), 255, ta};
                            float lw = (1.2f + 1.2f * (depth * 0.5f + 0.5f)) * S;
                            DrawLineEx((Vector2){traj_x[i-1], traj_y[i-1]},
                                       (Vector2){traj_x[i], traj_y[i]}, lw, tc);
                        }

                        if (n_vis > 2) {
                            float hx = traj_x[n_vis - 1];
                            float hy = traj_y[n_vis - 1];
                            DrawCircle((int)hx, (int)hy, 6.0f * S,
                                       (Color){120, 220, 255, (unsigned char)(60 * vis)});
                            DrawCircle((int)hx, (int)hy, 3.0f * S,
                                       (Color){200, 240, 255, (unsigned char)(180 * vis)});
                        }

                        {
                            const char *copy = "\xc2\xa9 2026 Johnathan K. Burchill";
                            float c_sz = 11.0f * S;
                            Vector2 csz = MeasureTextEx(app.font_ui, copy, c_sz, 1);
                            DrawTextEx(app.font_ui, copy,
                                       (Vector2){(w - csz.x) * 0.5f, h * 0.92f},
                                       c_sz, 1, (Color){80, 90, 110, (unsigned char)(120 * vis)});
                        }
                    }
                }

                /* "tap to continue" hint */
                if (elapsed > 6.0f) {
                    float blink = 0.5f + 0.5f * sinf(elapsed * 2.5f);
                    unsigned char ha = (unsigned char)(60 * blink);
                    const char *hint = TR(STR_TAP_CONTINUE);
                    float hint_sz = 13.0f * S;
                    Vector2 hsz = MeasureTextEx(app.font_ui, hint, hint_sz, 2);
                    DrawTextEx(app.font_ui, hint,
                               (Vector2){(w - hsz.x) * 0.5f, h * 0.82f},
                               hint_sz, 2, (Color){100, 110, 140, ha});
                }

                EndDrawing();
            }
        }
        #undef SS
        #undef TRAJ_N
        #undef MAX_FRAGS
        #undef FRAG_RNG
        #undef FRAG_RNDF
    }
    UnloadFont(splash_font);

    /* First run: language picker, then tutorial prompt */
    if (!app.tutorial_seen) {
        app.tutorial_seen = 1;
        app.show_lang_picker = 1;
        app.tutorial.active = 1;
        app.tutorial.step = -2; /* splash prompt (waits behind lang picker) */
    }

main_loop:
#ifdef __EMSCRIPTEN__
    emscripten_set_main_loop(main_loop_step, 0, 1);
#else
    while (!WindowShouldClose()) {
        main_loop_step();
    }
#endif

    app_shutdown(&app);
    CloseWindow();
    return 0;
}
