/* ui.c — Clay-based UI for Lorentz Tracer
 * Replaces raygui with clay.h for layout, scrolling, and text wrapping.
 * Widgets (slider, toggle, button, dropdown) implemented manually. */

#define CLAY_IMPLEMENTATION
#include "clay.h"

#include "ui.h"
#include "help.h"
#include "playback.h"
#include "tutorial.h"
#include "explorer.h"
#include "i18n.h"
#include "app.h"
#include "raylib.h"
#include "raymath.h"
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <stdarg.h>
#include <stdlib.h>

/* ---- Font IDs ---- */
#define FONT_UI   0
#define FONT_MONO 1

/* ---- Globals ---- */
static Font g_fonts[2];
static Clay_RenderCommandArray g_cmds;
static uint32_t g_active_slider;    /* hash id of slider being dragged */
static int g_dark_mode;             /* cached for widget helpers */
static float g_ui_zoom = 1.0f;     /* font scale for UI panel */
#define UZ(sz) ((uint16_t)((sz) * g_ui_zoom))
static int g_field_dd_open;
static int g_species_dd_open;
static int g_stel_dd_open;
static int g_lang_dd_open;
static char *g_rbuf;                /* render text buffer */
static int g_rbuf_len;

/* Dynamic string pool (reset each frame) */
#define SPOOL_N  128
#define SPOOL_SZ 128
static char g_spool[SPOOL_N][SPOOL_SZ];
static int g_sp;

/* Section triangle custom data */
typedef struct { int open; Clay_Color color; } TriData;
static TriData g_tri[5];

/* ---- Helpers ---- */
#define RC(c) ((Clay_Color){(float)(c).r, (float)(c).g, (float)(c).b, (float)(c).a})
#define CtoR(cc) ((Color){(unsigned char)roundf((cc).r), (unsigned char)roundf((cc).g), \
                          (unsigned char)roundf((cc).b), (unsigned char)roundf((cc).a)})

static Clay_String Sf(const char *fmt, ...) {
    char *buf = g_spool[g_sp++ % SPOOL_N];
    va_list ap;
    va_start(ap, fmt);
    int n = vsnprintf(buf, SPOOL_SZ, fmt, ap);
    va_end(ap);
    if (n < 0) n = 0;
    if (n >= SPOOL_SZ) n = SPOOL_SZ - 1;
    return (Clay_String){ .length = n, .chars = buf };
}

static Clay_String S(const char *s) {
    return (Clay_String){ .length = (int32_t)strlen(s), .chars = s };
}

/* ---- Text measurement (UTF-8 aware) ---- */
static Clay_Dimensions measure_text(Clay_StringSlice text, Clay_TextElementConfig *cfg,
                                    void *userData)
{
    Font *fonts = (Font *)userData;
    Font f = fonts[cfg->fontId];
    if (!f.glyphs) f = GetFontDefault();
    float sc = (float)cfg->fontSize / (float)f.baseSize;
    float maxW = 0, lineW = 0;
    int nch = 0;
    for (int i = 0; i < text.length; ) {
        if (text.chars[i] == '\n') {
            if (lineW > maxW) maxW = lineW;
            lineW = 0; nch = 0; i++; continue;
        }
        int csz = 0;
        int cp = GetCodepoint(&text.chars[i], &csz);
        int gi = GetGlyphIndex(f, cp);
        lineW += (f.glyphs[gi].advanceX != 0)
            ? (float)f.glyphs[gi].advanceX
            : (f.recs[gi].width + f.glyphs[gi].offsetX);
        nch++; i += csz;
    }
    if (lineW > maxW) maxW = lineW;
    return (Clay_Dimensions){
        .width = maxW * sc + nch * cfg->letterSpacing,
        .height = (float)cfg->fontSize
    };
}

/* ---- Clay renderer ---- */
static void clay_render_cmds(Clay_RenderCommandArray cmds)
{
    for (int j = 0; j < cmds.length; j++) {
        Clay_RenderCommand *c = Clay_RenderCommandArray_Get(&cmds, j);
        Clay_BoundingBox bb = c->boundingBox;
        switch (c->commandType) {
        case CLAY_RENDER_COMMAND_TYPE_RECTANGLE: {
            Clay_RectangleRenderData *r = &c->renderData.rectangle;
            Color col = CtoR(r->backgroundColor);
            if (r->cornerRadius.topLeft > 0) {
                float rad = (r->cornerRadius.topLeft * 2) /
                    fminf(bb.width > 0 ? bb.width : 1, bb.height > 0 ? bb.height : 1);
                DrawRectangleRounded((Rectangle){bb.x, bb.y, bb.width, bb.height},
                                     rad, 8, col);
            } else {
                DrawRectangle((int)roundf(bb.x), (int)roundf(bb.y),
                              (int)roundf(bb.width), (int)roundf(bb.height), col);
            }
            break;
        }
        case CLAY_RENDER_COMMAND_TYPE_TEXT: {
            Clay_TextRenderData *t = &c->renderData.text;
            int slen = t->stringContents.length + 1;
            if (slen > g_rbuf_len) {
                free(g_rbuf); g_rbuf = malloc(slen); g_rbuf_len = slen;
            }
            memcpy(g_rbuf, t->stringContents.chars, t->stringContents.length);
            g_rbuf[t->stringContents.length] = '\0';
            DrawTextEx(g_fonts[t->fontId], g_rbuf,
                       (Vector2){bb.x, bb.y}, (float)t->fontSize,
                       (float)t->letterSpacing, CtoR(t->textColor));
            break;
        }
        case CLAY_RENDER_COMMAND_TYPE_BORDER: {
            Clay_BorderRenderData *b = &c->renderData.border;
            Color col = CtoR(b->color);
            if (b->width.left > 0)
                DrawRectangle(roundf(bb.x), roundf(bb.y + b->cornerRadius.topLeft),
                    b->width.left, roundf(bb.height - b->cornerRadius.topLeft - b->cornerRadius.bottomLeft), col);
            if (b->width.right > 0)
                DrawRectangle(roundf(bb.x + bb.width - b->width.right),
                    roundf(bb.y + b->cornerRadius.topRight), b->width.right,
                    roundf(bb.height - b->cornerRadius.topRight - b->cornerRadius.bottomRight), col);
            if (b->width.top > 0)
                DrawRectangle(roundf(bb.x + b->cornerRadius.topLeft), roundf(bb.y),
                    roundf(bb.width - b->cornerRadius.topLeft - b->cornerRadius.topRight),
                    b->width.top, col);
            if (b->width.bottom > 0)
                DrawRectangle(roundf(bb.x + b->cornerRadius.bottomLeft),
                    roundf(bb.y + bb.height - b->width.bottom),
                    roundf(bb.width - b->cornerRadius.bottomLeft - b->cornerRadius.bottomRight),
                    b->width.bottom, col);
            break;
        }
        case CLAY_RENDER_COMMAND_TYPE_SCISSOR_START:
            BeginScissorMode((int)roundf(bb.x), (int)roundf(bb.y),
                             (int)roundf(bb.width), (int)roundf(bb.height));
            break;
        case CLAY_RENDER_COMMAND_TYPE_SCISSOR_END:
            EndScissorMode();
            break;
        case CLAY_RENDER_COMMAND_TYPE_CUSTOM: {
            TriData *td = (TriData *)c->renderData.custom.customData;
            if (!td) break;
            Color col = CtoR(td->color);
            float cx = bb.x, cy = bb.y;
            if (td->open) {
                DrawTriangle((Vector2){cx + 4, cy + 7}, (Vector2){cx + 8, cy + 1},
                             (Vector2){cx, cy + 1}, col);
            } else {
                DrawTriangle((Vector2){cx + 7, cy + 4}, (Vector2){cx + 1, cy},
                             (Vector2){cx + 1, cy + 8}, col);
            }
            break;
        }
        default: break;
        }
    }
}

/* ---- Error handler ---- */
static bool g_reinit_clay;
static void clay_error(Clay_ErrorData err) {
    TraceLog(LOG_WARNING, "Clay: %s", err.errorText.chars);
    if (err.errorType == CLAY_ERROR_TYPE_ELEMENTS_CAPACITY_EXCEEDED) {
        g_reinit_clay = true;
        Clay_SetMaxElementCount(Clay_GetMaxElementCount() * 2);
    } else if (err.errorType == CLAY_ERROR_TYPE_TEXT_MEASUREMENT_CAPACITY_EXCEEDED) {
        g_reinit_clay = true;
        Clay_SetMaxMeasureTextCacheWordCount(Clay_GetMaxMeasureTextCacheWordCount() * 2);
    }
}

/* ---- Public: init ---- */
static void *g_clay_mem;

void ui_init(struct AppState *app)
{
    g_fonts[FONT_UI] = app->font_ui;
    g_fonts[FONT_MONO] = app->font_mono;
    Clay_SetMaxElementCount(1024);
    uint64_t sz = Clay_MinMemorySize();
    g_clay_mem = malloc(sz);
    Clay_Arena arena = Clay_CreateArenaWithCapacityAndMemory(sz, g_clay_mem);
    Clay_Initialize(arena, (Clay_Dimensions){(float)app->win_w, (float)app->win_h},
                    (Clay_ErrorHandler){clay_error, 0});
    Clay_SetMeasureTextFunction(measure_text, g_fonts);
}

void ui_sync_from_app(const struct AppState *app) { (void)app; }

/* ======== Widget helpers ======== */

/* Slider with embedded label (left) and value (right), Blender-style.
 * Returns 1 if value changed. label and val_text may be NULL. */
static int do_slider(Clay_ElementId tid, const char *label, Clay_String val_text,
                     float *val, float vmin, float vmax,
                     Clay_Color track_c, Clay_Color fill_c, Clay_Color text_c)
{
    float pct = (vmax > vmin) ? (*val - vmin) / (vmax - vmin) : 0;
    if (pct < 0) pct = 0; if (pct > 1) pct = 1;
    Clay_Color slider_text_c = {240, 240, 240, 255};
    if (!g_dark_mode) slider_text_c = (Clay_Color){30, 30, 30, 255};
    Clay_Color label_c = slider_text_c; label_c.a = 180;
    CLAY(tid, {
        .layout = { .sizing = { CLAY_SIZING_GROW(0), CLAY_SIZING_FIXED(18) } },
        .backgroundColor = track_c,
        .cornerRadius = CLAY_CORNER_RADIUS(3)
    }) {
        /* Fill bar: normal child, takes percentage of width */
        if (pct > 0.002f) {
            CLAY_AUTO_ID({
                .layout = { .sizing = { CLAY_SIZING_PERCENT(pct), CLAY_SIZING_GROW(0) } },
                .backgroundColor = fill_c,
                .cornerRadius = CLAY_CORNER_RADIUS(3)
            }) {}
        }
        /* Text overlay: floating on top of fill, passthrough for pointer */
        CLAY_AUTO_ID({
            .layout = { .sizing = { CLAY_SIZING_GROW(0), CLAY_SIZING_GROW(0) },
                         .padding = {4, 4, 0, 0},
                         .childAlignment = { .y = CLAY_ALIGN_Y_CENTER } },
            .floating = { .attachTo = CLAY_ATTACH_TO_PARENT,
                           .pointerCaptureMode = CLAY_POINTER_CAPTURE_MODE_PASSTHROUGH,
                           .attachPoints = { .element = CLAY_ATTACH_POINT_LEFT_CENTER,
                                              .parent = CLAY_ATTACH_POINT_LEFT_CENTER } }
        }) {
            if (label) {
                CLAY_TEXT(S(label), CLAY_TEXT_CONFIG({ .fontId = FONT_UI, .fontSize = UZ(11),
                          .letterSpacing = 1, .textColor = label_c }));
            }
            CLAY_AUTO_ID({ .layout = { .sizing = { .width = CLAY_SIZING_GROW(0) } } }) {}
            if (val_text.chars) {
                CLAY_TEXT(val_text, CLAY_TEXT_CONFIG({ .fontId = FONT_MONO, .fontSize = UZ(11),
                          .letterSpacing = 1, .textColor = slider_text_c }));
            }
        }
    }
    if (Clay_PointerOver(tid) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
        g_active_slider = tid.id;
    int changed = 0;
    if (g_active_slider == tid.id) {
        Clay_ElementData d = Clay_GetElementData(tid);
        if (d.found && d.boundingBox.width > 0) {
            float t = (GetMousePosition().x - d.boundingBox.x) / d.boundingBox.width;
            if (t < 0) t = 0; if (t > 1) t = 1;
            float nv = vmin + t * (vmax - vmin);
            if (nv != *val) { *val = nv; changed = 1; }
        }
    }
    return changed;
}

/* Toggle: colored rectangle + label. Returns 1 if clicked. */
static int do_toggle(Clay_ElementId eid, const char *label, int active,
                     Clay_Color on_c, Clay_Color off_c, Clay_Color hover_c,
                     Clay_TextElementConfig *tcfg)
{
    CLAY(eid, {
        .layout = { .sizing = { CLAY_SIZING_GROW(0), CLAY_SIZING_FIXED(18) },
                     .padding = {4, 4, 2, 2},
                     .childAlignment = { .x = CLAY_ALIGN_X_CENTER, .y = CLAY_ALIGN_Y_CENTER } },
        .backgroundColor = active ? on_c : (Clay_Hovered() ? hover_c : off_c),
        .border = { .width = CLAY_BORDER_OUTSIDE(1), .color = {80,80,90,100} },
        .cornerRadius = CLAY_CORNER_RADIUS(2)
    }) {
        CLAY_TEXT(S(label), CLAY_TEXT_CONFIG(*tcfg));
    }
    return Clay_PointerOver(eid) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
}

/* Button: returns 1 if clicked */
static int do_button(Clay_ElementId eid, const char *label,
                     Clay_Color bg, Clay_Color hover_c, Clay_TextElementConfig *tcfg)
{
    CLAY(eid, {
        .layout = { .sizing = { CLAY_SIZING_GROW(0), CLAY_SIZING_FIXED(22) },
                     .padding = {4, 4, 2, 2},
                     .childAlignment = { .x = CLAY_ALIGN_X_CENTER, .y = CLAY_ALIGN_Y_CENTER } },
        .backgroundColor = Clay_Hovered() ? hover_c : bg,
        .border = { .width = CLAY_BORDER_OUTSIDE(1), .color = {80,80,90,120} },
        .cornerRadius = CLAY_CORNER_RADIUS(3)
    }) {
        CLAY_TEXT(S(label), CLAY_TEXT_CONFIG(*tcfg));
    }
    return Clay_PointerOver(eid) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
}

/* Section header with triangle + click to toggle */
static void do_section_header(AppState *app, int idx, const char *title,
                              Clay_Color dim_c, Clay_Color hover_c)
{
    Clay_ElementId hid = CLAY_IDI("SecHdr", idx);
    Clay_Color tc = Clay_PointerOver(hid) ? hover_c : dim_c;
    g_tri[idx] = (TriData){ app->ui_section_open[idx], tc };
    CLAY(hid, {
        .layout = { .sizing = { CLAY_SIZING_GROW(0), CLAY_SIZING_FIT(0) },
                     .childAlignment = { .y = CLAY_ALIGN_Y_CENTER }, .childGap = 4,
                     .padding = {0, 0, 2, 2} },
        .border = { .width = { .top = 1 }, .color = dim_c }
    }) {
        CLAY(CLAY_IDI("SecTri", idx), {
            .layout = { .sizing = { CLAY_SIZING_FIXED(10), CLAY_SIZING_FIXED(10) } },
            .custom = { .customData = &g_tri[idx] }
        }) {}
        CLAY_TEXT(S(title), CLAY_TEXT_CONFIG({ .fontId = FONT_UI, .fontSize = UZ(13),
                  .letterSpacing = 1, .textColor = tc }));
    }
    if (Clay_PointerOver(hid) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
        app->ui_section_open[idx] = !app->ui_section_open[idx];
}

/* Label + value on one line */
static void label_value(const char *label, Clay_String val,
                        Clay_Color dim_c, Clay_Color text_c)
{
    CLAY_AUTO_ID({
        .layout = { .sizing = { CLAY_SIZING_GROW(0), CLAY_SIZING_FIT(0) } }
    }) {
        CLAY_TEXT(S(label), CLAY_TEXT_CONFIG({ .fontId = FONT_UI, .fontSize = UZ(13),
                  .letterSpacing = 1, .textColor = dim_c }));
        CLAY_TEXT(val, CLAY_TEXT_CONFIG({ .fontId = FONT_MONO, .fontSize = UZ(13),
                  .letterSpacing = 1, .textColor = text_c }));
    }
}

/* ======== Section declarations ======== */

static void section_explore(AppState *app, Clay_Color text_c, Clay_Color dim_c,
                            Clay_Color track_c, Clay_Color fill_c,
                            Clay_TextElementConfig *wcfg)
{
    (void)track_c; (void)fill_c; (void)wcfg;
    do_section_header(app, 0, TR(STR_EXPLORE), dim_c, text_c);
    if (!app->ui_section_open[0]) return;

    int dark = app->dark_mode;
    Clay_Color btn_c = dark
        ? (Clay_Color){50, 50, 65, 255} : (Clay_Color){210, 210, 220, 255};
    Clay_Color btn_hover = dark
        ? (Clay_Color){70, 70, 90, 255} : (Clay_Color){190, 190, 205, 255};

    /* Button row */
    CLAY_AUTO_ID({
        .layout = { .sizing = { CLAY_SIZING_GROW(0), CLAY_SIZING_FIT(0) },
                     .childGap = 6 }
    }) {
        /* Load Scenario button */
        Clay_ElementId load_id = CLAY_ID("ExplLoadBtn");
        CLAY(load_id, {
            .layout = { .sizing = { CLAY_SIZING_FIT(.min = 0), CLAY_SIZING_FIXED(UZ(24)) },
                         .padding = {(uint16_t)UZ(8), (uint16_t)UZ(8), 0, 0},
                         .childAlignment = { .x = CLAY_ALIGN_X_CENTER, .y = CLAY_ALIGN_Y_CENTER } },
            .backgroundColor = Clay_Hovered() ? btn_hover : btn_c,
            .border = { .width = CLAY_BORDER_OUTSIDE(1), .color = {80,80,90,120} },
            .cornerRadius = CLAY_CORNER_RADIUS(3)
        }) {
            CLAY_TEXT(S(TR(STR_EXPLORE_LOAD)), CLAY_TEXT_CONFIG({
                .fontId = FONT_UI, .fontSize = UZ(12), .letterSpacing = 1, .textColor = text_c }));
        }
        if (Clay_PointerOver(load_id) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            explorer_scan_files(app);
            app->explorer.show_picker = 1;
        }

        /* Export Template button */
        Clay_ElementId export_id = CLAY_ID("ExplExportBtn");
        CLAY(export_id, {
            .layout = { .sizing = { CLAY_SIZING_FIT(.min = 0), CLAY_SIZING_FIXED(UZ(24)) },
                         .padding = {(uint16_t)UZ(8), (uint16_t)UZ(8), 0, 0},
                         .childAlignment = { .x = CLAY_ALIGN_X_CENTER, .y = CLAY_ALIGN_Y_CENTER } },
            .backgroundColor = Clay_Hovered() ? btn_hover : btn_c,
            .border = { .width = CLAY_BORDER_OUTSIDE(1), .color = {80,80,90,120} },
            .cornerRadius = CLAY_CORNER_RADIUS(3)
        }) {
            CLAY_TEXT(S(TR(STR_EXPLORE_EXPORT)), CLAY_TEXT_CONFIG({
                .fontId = FONT_UI, .fontSize = UZ(12), .letterSpacing = 1, .textColor = text_c }));
        }
        if (Clay_PointerOver(export_id) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
            explorer_export_template(app);
    }

    /* Show active scenario info */
    if (app->explorer.active && app->explorer.title[0]) {
        Clay_Color accent = dark
            ? (Clay_Color){100, 160, 255, 255} : (Clay_Color){40, 80, 180, 255};
        CLAY_TEXT(S(app->explorer.title), CLAY_TEXT_CONFIG({
            .fontId = FONT_UI, .fontSize = UZ(13), .letterSpacing = 1, .textColor = accent }));

        /* Stop button */
        Clay_ElementId stop_id = CLAY_ID("ExplStopBtn");
        CLAY(stop_id, {
            .layout = { .sizing = { CLAY_SIZING_FIT(.min = 0), CLAY_SIZING_FIXED(UZ(22)) },
                         .padding = {(uint16_t)UZ(8), (uint16_t)UZ(8), 0, 0},
                         .childAlignment = { .x = CLAY_ALIGN_X_CENTER, .y = CLAY_ALIGN_Y_CENTER } },
            .backgroundColor = Clay_Hovered() ? btn_hover : btn_c,
            .border = { .width = CLAY_BORDER_OUTSIDE(1), .color = {80,80,90,120} },
            .cornerRadius = CLAY_CORNER_RADIUS(3)
        }) {
            CLAY_TEXT(S(TR(STR_TUT_STOP)), CLAY_TEXT_CONFIG({
                .fontId = FONT_UI, .fontSize = UZ(12), .letterSpacing = 1, .textColor = text_c }));
        }
        if (Clay_PointerOver(stop_id) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
            explorer_stop(app);
    }
}

static void section_model(AppState *app, Clay_Color text_c, Clay_Color dim_c,
                          Clay_Color track_c, Clay_Color fill_c,
                          Clay_TextElementConfig *wcfg)
{
    do_section_header(app, 1, TR(STR_MODEL_PARTICLE), dim_c, text_c);
    if (!app->ui_section_open[1]) return;

    /* Field model dropdown trigger */
    CLAY_TEXT(S(TR(STR_FIELD_MODEL)), CLAY_TEXT_CONFIG({ .fontId = FONT_UI, .fontSize = UZ(13),
              .letterSpacing = 1, .textColor = dim_c }));
    {
        Clay_ElementId fid = CLAY_ID("FieldDDBtn");
        Clay_Color bg = Clay_Hovered() ? (Clay_Color){60,60,70,255} : (Clay_Color){30,30,36,255};
        if (!app->dark_mode) bg = Clay_Hovered() ? (Clay_Color){210,210,218,255} : (Clay_Color){232,232,236,255};
        CLAY(fid, {
            .layout = { .sizing = { CLAY_SIZING_GROW(0), CLAY_SIZING_FIXED(24) },
                         .padding = {6, 6, 3, 3}, .childAlignment = { .y = CLAY_ALIGN_Y_CENTER } },
            .backgroundColor = bg,
            .border = { .width = CLAY_BORDER_OUTSIDE(1), .color = {80,80,90,160} },
            .cornerRadius = CLAY_CORNER_RADIUS(3)
        }) {
            CLAY_TEXT(S(app->models[app->current_model].name),
                      CLAY_TEXT_CONFIG({ .fontId = FONT_UI, .fontSize = UZ(13), .letterSpacing = 1, .textColor = text_c }));
        }
        if (Clay_PointerOver(fid) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            g_field_dd_open = !g_field_dd_open;
            g_species_dd_open = 0; g_stel_dd_open = 0;
        }
    }

    /* Field parameters */
    if (!g_field_dd_open && !g_species_dd_open) {
        FieldModel *fm = &app->models[app->current_model];
        for (int i = 0; i < fm->n_params; i++) {
            /* Stellarator config dropdown */
            if (app->current_model == 8 && i == 0) {
                CLAY_TEXT(S(TR(STR_CONFIG)), CLAY_TEXT_CONFIG({ .fontId = FONT_UI, .fontSize = UZ(13),
                          .letterSpacing = 1, .textColor = dim_c }));
                Clay_ElementId sid = CLAY_ID("StelDDBtn");
                const char *cfgs[] = {TR(STR_STEL_CFG_R2), TR(STR_STEL_CFG_LP22)};
                int cfg_idx = (int)fm->params[0];
                if (cfg_idx < 0) cfg_idx = 0; if (cfg_idx > 1) cfg_idx = 1;
                Clay_Color sbg = Clay_Hovered() ? (Clay_Color){60,60,70,255} : (Clay_Color){30,30,36,255};
                if (!app->dark_mode) sbg = Clay_Hovered() ? (Clay_Color){210,210,218,255} : (Clay_Color){232,232,236,255};
                CLAY(sid, {
                    .layout = { .sizing = { CLAY_SIZING_GROW(0), CLAY_SIZING_FIXED(20) },
                                 .padding = {6,6,2,2}, .childAlignment = { .y = CLAY_ALIGN_Y_CENTER } },
                    .backgroundColor = sbg,
                    .border = { .width = CLAY_BORDER_OUTSIDE(1), .color = {80,80,90,160} },
                    .cornerRadius = CLAY_CORNER_RADIUS(3)
                }) {
                    CLAY_TEXT(S(cfgs[cfg_idx]), CLAY_TEXT_CONFIG({ .fontId = FONT_UI, .fontSize = UZ(13),
                              .letterSpacing = 1, .textColor = text_c }));
                }
                if (Clay_PointerOver(sid) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                    g_stel_dd_open = !g_stel_dd_open;
                    g_field_dd_open = 0; g_species_dd_open = 0;
                }
                continue;
            }
            float val = (float)fm->params[i];
            do_slider(CLAY_IDI("PSlider", i), fm->param_names[i], Sf("%.3g", fm->params[i]),
                      &val, (float)fm->param_min[i], (float)fm->param_max[i], track_c, fill_c, text_c);
            fm->params[i] = (double)val;
        }

        /* Particle */
        CLAY_TEXT(S(TR(STR_PARTICLE)), CLAY_TEXT_CONFIG({ .fontId = FONT_UI, .fontSize = UZ(13),
                  .letterSpacing = 1, .textColor = dim_c }));
        /* Species dropdown trigger */
        {
            Clay_ElementId spid = CLAY_ID("SpeciesDDBtn");
            const char *species_names[] = {TR(STR_SPECIES_ELECTRON), TR(STR_SPECIES_POSITRON),
                TR(STR_SPECIES_PROTON), TR(STR_SPECIES_ALPHA), TR(STR_SPECIES_OPLUS), TR(STR_SPECIES_CUSTOM)};
            Clay_Color bg = Clay_Hovered() ? (Clay_Color){60,60,70,255} : (Clay_Color){30,30,36,255};
            if (!app->dark_mode) bg = Clay_Hovered() ? (Clay_Color){210,210,218,255} : (Clay_Color){232,232,236,255};
            CLAY(spid, {
                .layout = { .sizing = { CLAY_SIZING_GROW(0), CLAY_SIZING_FIXED(24) },
                             .padding = {6,6,3,3}, .childAlignment = { .y = CLAY_ALIGN_Y_CENTER } },
                .backgroundColor = bg,
                .border = { .width = CLAY_BORDER_OUTSIDE(1), .color = {80,80,90,160} },
                .cornerRadius = CLAY_CORNER_RADIUS(3)
            }) {
                CLAY_TEXT(S(species_names[app->species]),
                          CLAY_TEXT_CONFIG({ .fontId = FONT_UI, .fontSize = UZ(13), .letterSpacing = 1, .textColor = text_c }));
            }
            if (Clay_PointerOver(spid) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                g_species_dd_open = !g_species_dd_open;
                g_field_dd_open = 0; g_stel_dd_open = 0;
            }
        }

        /* Custom particle */
        if (app->species == SPECIES_CUSTOM) {
            float qv = (float)app->custom_q;
            if (do_slider(CLAY_ID("QSlider"), TR(STR_CHARGE), Sf("%.2f C", app->custom_q),
                          &qv, -10, 10, track_c, fill_c, text_c))
                app->custom_q = (double)qv;

            float mv = (float)app->custom_m;
            if (do_slider(CLAY_ID("MSlider"), TR(STR_MASS), Sf("%.2f kg", app->custom_m),
                          &mv, 0.01f, 10, track_c, fill_c, text_c))
                app->custom_m = (double)mv;

            float sv = (float)app->custom_speed;
            if (do_slider(CLAY_ID("SpdSlider"), TR(STR_SPEED), Sf("%.2f m/s", app->custom_speed),
                          &sv, 0.01f, 5, track_c, fill_c, text_c))
                app->custom_speed = (double)sv;
        } else {
            /* Energy */
            float e_max_t[] = {100, 1000, 10000};
            int er = app->energy_range; if (er < 0) er = 0; if (er > 2) er = 2;
            Clay_String e_val = (app->E_keV >= 1000.0)
                ? Sf("%.2f MeV", app->E_keV / 1000.0)
                : Sf("%.2f keV", app->E_keV);
            float ev = (float)app->E_keV;
            if (do_slider(CLAY_ID("ESlider"), TR(STR_ENERGY), e_val,
                          &ev, 0.01f, e_max_t[er], track_c, fill_c, text_c))
                app->E_keV = (double)ev;

            /* Energy range toggle group */
            CLAY(CLAY_ID("ERGroup"), {
                .layout = { .childGap = 2, .sizing = { CLAY_SIZING_GROW(0), CLAY_SIZING_FIT(0) } }
            }) {
                const char *elabels[] = {"x1", "x10", "x100"};
                for (int i = 0; i < 3; i++) {
                    Clay_ElementId bid = CLAY_IDI("ERBtn", i);
                    if (do_toggle(bid, elabels[i], app->energy_range == i,
                                  fill_c, track_c, (Clay_Color){60,60,80,255}, wcfg))
                        app->energy_range = i;
                }
            }
        }

        /* Pitch */
        float pv = (float)app->pitch_deg;
        if (do_slider(CLAY_ID("PitchSlider"), TR(STR_PITCH), Sf("%.1f\xc2\xb0", app->pitch_deg),
                      &pv, 0, 180, track_c, fill_c, text_c))
            app->pitch_deg = (double)pv;
    }
}

static void section_playback(AppState *app, Clay_Color text_c, Clay_Color dim_c,
                             Clay_Color track_c, Clay_Color fill_c,
                             Clay_TextElementConfig *wcfg)
{
    do_section_header(app, 2, TR(STR_PLAYBACK_RECORD), dim_c, text_c);
    if (!app->ui_section_open[2]) return;

    /* Speed range */
    CLAY(CLAY_ID("SRGroup"), {
        .layout = { .childGap = 2, .sizing = { CLAY_SIZING_GROW(0), CLAY_SIZING_FIT(0) } }
    }) {
        const char *slabels[] = {"x0.1", "x1", "x10", "x100"};
        float max_spd[] = {0.1f, 1.0f, 10.0f, 100.0f};
        for (int i = 0; i < 4; i++) {
            Clay_ElementId bid = CLAY_IDI("SRBtn", i);
            if (do_toggle(bid, slabels[i], app->speed_range == i,
                          fill_c, track_c, (Clay_Color){60,60,80,255}, wcfg)) {
                app->speed_range = i;
                if (app->playback_speed > max_spd[i]) app->playback_speed = max_spd[i];
            }
        }
    }

    /* Speed slider */
    float max_spd[] = {0.1f, 1.0f, 10.0f, 100.0f};
    float spd = (float)app->playback_speed;
    do_slider(CLAY_ID("SpeedSlider"), TR(STR_SPEED), Sf("%.2fx", app->playback_speed),
              &spd, 0, max_spd[app->speed_range], track_c, fill_c, text_c);
    app->playback_speed = (double)spd;

    /* Pause + Reset buttons */
    CLAY_AUTO_ID({
        .layout = { .childGap = 4, .sizing = { CLAY_SIZING_GROW(0), CLAY_SIZING_FIT(0) } }
    }) {
        if (do_button(CLAY_ID("PauseBtn"), app->paused ? TR(STR_RESUME) : TR(STR_PAUSE),
                      track_c, fill_c, wcfg))
            app->paused = !app->paused;
        if (do_button(CLAY_ID("ResetBtn"), TR(STR_RESET), track_c, fill_c, wcfg))
            app->needs_reset = 1;
    }

    /* Trigger rec toggle */
    if (do_toggle(CLAY_ID("TrigRecTgl"),
                  (app->recorder.active || app->event_log) ? TR(STR_TRIGGER_REC_ACTIVE) : TR(STR_TRIGGER_REC),
                  app->trigger_record, fill_c, track_c, (Clay_Color){60,60,80,255}, wcfg))
        app->trigger_record = !app->trigger_record;
}

/* ---- Color swatch + picker ---- */

/* ---- HSV helpers for hue bar ---- */

static void rgb_to_hsv(Color c, float *h, float *s, float *v)
{
    float r = c.r / 255.0f, g = c.g / 255.0f, b = c.b / 255.0f;
    float mx = fmaxf(r, fmaxf(g, b)), mn = fminf(r, fminf(g, b));
    float d = mx - mn;
    *v = mx;
    *s = (mx > 0) ? d / mx : 0;
    if (d < 1e-6f) { *h = 0; return; }
    if (mx == r) *h = 60.0f * fmodf((g - b) / d + 6.0f, 6.0f);
    else if (mx == g) *h = 60.0f * ((b - r) / d + 2.0f);
    else *h = 60.0f * ((r - g) / d + 4.0f);
}

static Color hsv_to_rgb(float h, float s, float v, unsigned char a)
{
    float c = v * s, x = c * (1.0f - fabsf(fmodf(h / 60.0f, 2.0f) - 1.0f));
    float m = v - c;
    float r, g, b;
    if      (h < 60)  { r=c; g=x; b=0; }
    else if (h < 120) { r=x; g=c; b=0; }
    else if (h < 180) { r=0; g=c; b=x; }
    else if (h < 240) { r=0; g=x; b=c; }
    else if (h < 300) { r=x; g=0; b=c; }
    else               { r=c; g=0; b=x; }
    return (Color){(unsigned char)((r+m)*255), (unsigned char)((g+m)*255),
                   (unsigned char)((b+m)*255), a};
}

/* Active color picker state */
static Color *g_cpick_target;      /* pointer to the Color being edited */
static uint32_t g_cpick_id;        /* element id that owns the picker */
static int g_cpick_tab;            /* 0=RGB, 1=HSV, 2=Gray */
static Color g_cpick_edit;         /* working copy — only applied on OK */
static Color g_cpick_orig;         /* original value for cancel */
static float g_cpick_click_y;      /* mouse Y when swatch was clicked */
static float g_cpick_hsv[3];      /* cached H, S, V to avoid hue drift */
static int g_cpick_hsv_init;      /* 1 = cached HSV is valid */

/* Inline color swatch. Clicking opens/closes the picker tooltip.
 * Returns 1 if the picker is open for this swatch. */
static int do_swatch(Clay_ElementId eid, Color *col)
{
    Clay_Color cc = {col->r, col->g, col->b, 255};
    CLAY(eid, {
        .layout = { .sizing = { CLAY_SIZING_FIXED(16), CLAY_SIZING_FIXED(16) } },
        .backgroundColor = cc,
        .border = { .width = CLAY_BORDER_OUTSIDE(1), .color = {180,180,180,200} },
        .cornerRadius = CLAY_CORNER_RADIUS(2)
    }) {}
    if (Clay_PointerOver(eid) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        if (g_cpick_id == eid.id) { g_cpick_target = NULL; g_cpick_id = 0; }
        else { g_cpick_target = col; g_cpick_id = eid.id;
               g_cpick_orig = *col; g_cpick_edit = *col;
               g_cpick_click_y = (float)GetMouseY();
               g_cpick_hsv_init = 0; }
    }
    return g_cpick_id == eid.id;
}

/* Floating color picker tooltip. Call once per frame, after all swatches. */
static void declare_color_picker(Clay_Color panel_bg, Clay_Color text_c)
{
    if (!g_cpick_target || !g_cpick_id) return;

    Clay_Color bg = {panel_bg.r * 0.9f, panel_bg.g * 0.9f, panel_bg.b * 0.9f, 250};
    Clay_Color track_c = g_dark_mode ? (Clay_Color){30,30,36,255} : (Clay_Color){210,210,218,255};
    Clay_Color fill_c = g_dark_mode ? (Clay_Color){80,80,106,255} : (Clay_Color){128,128,160,255};
    Clay_Color hov = {60,60,80,255};
    Clay_TextElementConfig tcfg = { .fontId = FONT_UI, .fontSize = UZ(11),
        .letterSpacing = 1, .textColor = text_c };

    /* Clamp position so the picker doesn't go off the bottom of the screen.
     * Picker height is ~180px. Place at click Y, but push up if needed. */
    float cp_h = 180.0f;
    float cp_y = g_cpick_click_y;
    float screen_h = (float)GetScreenHeight();
    if (cp_y + cp_h > screen_h - 10) cp_y = screen_h - cp_h - 10;
    if (cp_y < 10) cp_y = 10;

    CLAY(CLAY_ID("ColorPicker"), {
        .floating = { .attachTo = CLAY_ATTACH_TO_ELEMENT_WITH_ID,
                      .parentId = g_cpick_id,
                      .zIndex = 20,
                      .attachPoints = { CLAY_ATTACH_POINT_LEFT_TOP, CLAY_ATTACH_POINT_RIGHT_TOP },
                      .offset = { .x = 0, .y = cp_y - g_cpick_click_y } },
        .layout = { .layoutDirection = CLAY_TOP_TO_BOTTOM,
                     .sizing = { CLAY_SIZING_FIXED(180), CLAY_SIZING_FIT(0) },
                     .padding = {6,6,6,6}, .childGap = 4 },
        .backgroundColor = bg,
        .border = { .width = CLAY_BORDER_OUTSIDE(1), .color = {100,100,110,200} },
        .cornerRadius = CLAY_CORNER_RADIUS(4)
    }) {
        /* Preview: new (edit) left, original right */
        CLAY(CLAY_ID("CPPrevRow"), {
            .layout = { .childGap = 2, .sizing = { CLAY_SIZING_GROW(0), CLAY_SIZING_FIXED(14) } }
        }) {
            Clay_Color enew = {g_cpick_edit.r, g_cpick_edit.g, g_cpick_edit.b, 255};
            Clay_Color eorig = {g_cpick_orig.r, g_cpick_orig.g, g_cpick_orig.b, 255};
            CLAY(CLAY_ID("CPPNew"), {
                .layout = { .sizing = { CLAY_SIZING_GROW(0), CLAY_SIZING_GROW(0) } },
                .backgroundColor = enew, .cornerRadius = CLAY_CORNER_RADIUS(2)
            }) {}
            CLAY(CLAY_ID("CPPOrig"), {
                .layout = { .sizing = { CLAY_SIZING_GROW(0), CLAY_SIZING_GROW(0) } },
                .backgroundColor = eorig, .cornerRadius = CLAY_CORNER_RADIUS(2)
            }) {}
        }

        /* Tab row: RGB / HSV / Gray */
        CLAY(CLAY_ID("CPTabs"), {
            .layout = { .childGap = 2, .sizing = { CLAY_SIZING_GROW(0), CLAY_SIZING_FIT(0) } }
        }) {
            if (do_toggle(CLAY_ID("CPTabRGB"), "RGB", g_cpick_tab == 0, fill_c, track_c, hov, &tcfg))
                { g_cpick_tab = 0; g_cpick_hsv_init = 0; }
            if (do_toggle(CLAY_ID("CPTabHSV"), "HSV", g_cpick_tab == 1, fill_c, track_c, hov, &tcfg))
                { g_cpick_tab = 1; g_cpick_hsv_init = 0; }
            if (do_toggle(CLAY_ID("CPTabGray"), "Gray", g_cpick_tab == 2, fill_c, track_c, hov, &tcfg))
                { g_cpick_tab = 2; g_cpick_hsv_init = 0; }
        }

        float af = g_cpick_edit.a / 255.0f;

        if (g_cpick_tab == 0) {
            /* RGB */
            float rf = g_cpick_edit.r / 255.0f;
            float gf = g_cpick_edit.g / 255.0f;
            float bf = g_cpick_edit.b / 255.0f;
            Clay_Color r_fill = {g_cpick_edit.r, 0, 0, 255};
            Clay_Color g_fill = {0, g_cpick_edit.g, 0, 255};
            Clay_Color b_fill = {0, 0, g_cpick_edit.b, 255};
            do_slider(CLAY_ID("CPR"), "R", Sf("%d", g_cpick_edit.r), &rf, 0, 1, track_c, r_fill, text_c);
            do_slider(CLAY_ID("CPG"), "G", Sf("%d", g_cpick_edit.g), &gf, 0, 1, track_c, g_fill, text_c);
            do_slider(CLAY_ID("CPB"), "B", Sf("%d", g_cpick_edit.b), &bf, 0, 1, track_c, b_fill, text_c);
            do_slider(CLAY_ID("CPA"), "A", Sf("%d", g_cpick_edit.a), &af, 0, 1, track_c, fill_c, text_c);
            g_cpick_edit = (Color){
                (unsigned char)(rf * 255), (unsigned char)(gf * 255),
                (unsigned char)(bf * 255), (unsigned char)(af * 255)
            };
        } else if (g_cpick_tab == 1) {
            /* HSV — use cached values to prevent hue drift at low saturation */
            if (!g_cpick_hsv_init) {
                rgb_to_hsv(g_cpick_edit, &g_cpick_hsv[0], &g_cpick_hsv[1], &g_cpick_hsv[2]);
                g_cpick_hsv_init = 1;
            }
            float h = g_cpick_hsv[0], s = g_cpick_hsv[1], v = g_cpick_hsv[2];
            Color hc = hsv_to_rgb(h, 1.0f, 1.0f, 255);
            Color sc = hsv_to_rgb(h, s, 1.0f, 255);
            Color vc = hsv_to_rgb(h, s, v, 255);
            Clay_Color h_fill = {hc.r, hc.g, hc.b, 255};
            Clay_Color s_fill = {sc.r, sc.g, sc.b, 255};
            Clay_Color v_fill = {vc.r, vc.g, vc.b, 255};
            do_slider(CLAY_ID("CPH"), "H", Sf("%.0f\xc2\xb0", h), &h, 0, 359, track_c, h_fill, text_c);
            do_slider(CLAY_ID("CPS"), "S", Sf("%.0f%%", s*100), &s, 0, 1, track_c, s_fill, text_c);
            do_slider(CLAY_ID("CPV"), "V", Sf("%.0f%%", v*100), &v, 0, 1, track_c, v_fill, text_c);
            do_slider(CLAY_ID("CPA"), "A", Sf("%d", g_cpick_edit.a), &af, 0, 1, track_c, fill_c, text_c);
            g_cpick_hsv[0] = h; g_cpick_hsv[1] = s; g_cpick_hsv[2] = v;
            g_cpick_edit = hsv_to_rgb(h, s, v, (unsigned char)(af * 255));
        } else {
            /* Gray: single brightness slider, sets R=G=B */
            float gray = (g_cpick_edit.r * 0.299f + g_cpick_edit.g * 0.587f
                        + g_cpick_edit.b * 0.114f) / 255.0f;
            unsigned char gv = (unsigned char)(gray * 255);
            Clay_Color gy_fill = {gv, gv, gv, 255};
            do_slider(CLAY_ID("CPGray"), "V", Sf("%d", gv), &gray, 0, 1, track_c, gy_fill, text_c);
            do_slider(CLAY_ID("CPA"), "A", Sf("%d", g_cpick_edit.a), &af, 0, 1, track_c, fill_c, text_c);
            unsigned char g8 = (unsigned char)(gray * 255);
            g_cpick_edit = (Color){g8, g8, g8, (unsigned char)(af * 255)};
        }

        /* OK / Cancel buttons */
        CLAY(CLAY_ID("CPBtns"), {
            .layout = { .childGap = 4, .sizing = { CLAY_SIZING_GROW(0), CLAY_SIZING_FIT(0) } }
        }) {
            if (do_button(CLAY_ID("CPOK"), TR(STR_OK), fill_c, hov, &tcfg)) {
                *g_cpick_target = g_cpick_edit;
                g_cpick_target = NULL; g_cpick_id = 0;
            }
            if (do_button(CLAY_ID("CPCancel"), TR(STR_CANCEL), track_c, hov, &tcfg)) {
                g_cpick_target = NULL; g_cpick_id = 0;
            }
        }
    }
}

static void section_display(AppState *app, Clay_Color text_c, Clay_Color dim_c,
                            Clay_Color track_c, Clay_Color fill_c,
                            Clay_TextElementConfig *wcfg)
{
    do_section_header(app, 3, TR(STR_DISPLAY), dim_c, text_c);
    if (!app->ui_section_open[3]) return;

    Clay_Color hov = {60,60,80,255};

    /* Field lines & trail toggles */
    CLAY_AUTO_ID({ .layout = { .childGap = 4, .sizing = { CLAY_SIZING_GROW(0), CLAY_SIZING_FIT(0) } } }) {
        if (do_toggle(CLAY_ID("TglTrail"), TR(STR_TRAIL), app->show_trail, fill_c, track_c, hov, wcfg))
            app->show_trail = !app->show_trail;
        if (do_toggle(CLAY_ID("TglFL"), TR(STR_FIELD_LINES), app->show_field_lines, fill_c, track_c, hov, wcfg))
            app->show_field_lines = !app->show_field_lines;
        if (do_toggle(CLAY_ID("TglGCFL"), TR(STR_GC_FL), app->show_gc_field_line, fill_c, track_c, hov, wcfg))
            app->show_gc_field_line = !app->show_gc_field_line;
        if (do_toggle(CLAY_ID("TglPFL"), TR(STR_FL_POS), app->show_pos_field_line, fill_c, track_c, hov, wcfg))
            app->show_pos_field_line = !app->show_pos_field_line;
    }
    if (app->show_trail) {
        do_slider(CLAY_ID("TrailFadeSlider"), TR(STR_TRAIL_FADE), Sf("%.0f%%", app->trail_fade * 100),
                  &app->trail_fade, 0.0f, 1.0f, track_c, fill_c, text_c);
    }
    if (app->show_gc_field_line) {
        float fl = (float)app->gc_fl_length;
        do_slider(CLAY_ID("GCLenSlider"), TR(STR_GC_LEN), Sf("%.1f", app->gc_fl_length),
                  &fl, 0.5f, 100, track_c, fill_c, text_c);
        app->gc_fl_length = (double)fl;
    }

    /* Vector toggles */
    CLAY_AUTO_ID({ .layout = { .childGap = 4, .sizing = { CLAY_SIZING_GROW(0), CLAY_SIZING_FIT(0) } } }) {
        if (do_toggle(CLAY_ID("TglVel"), "v", app->show_velocity_vec, fill_c, track_c, hov, wcfg))
            app->show_velocity_vec = !app->show_velocity_vec;
        if (do_toggle(CLAY_ID("TglBvec"), "B", app->show_B_vec, fill_c, track_c, hov, wcfg))
            app->show_B_vec = !app->show_B_vec;
        if (do_toggle(CLAY_ID("TglFvec"), "F", app->show_F_vec, fill_c, track_c, hov, wcfg))
            app->show_F_vec = !app->show_F_vec;
        if (do_toggle(CLAY_ID("TglVScaled"), TR(STR_SCALED), app->vec_scaled, fill_c, track_c, hov, wcfg))
            app->vec_scaled = !app->vec_scaled;
    }
    if (app->vec_scaled) {
        if (app->show_velocity_vec)
            do_slider(CLAY_ID("VScaleSlider"), TR(STR_V_SCALE), Sf("10^%.0f", app->vec_scale_v),
                      &app->vec_scale_v, -10, 2, track_c, fill_c, text_c);
        if (app->show_B_vec)
            do_slider(CLAY_ID("BScaleSlider"), TR(STR_B_SCALE), Sf("10^%.0f", app->vec_scale_B),
                      &app->vec_scale_B, -4, 4, track_c, fill_c, text_c);
        if (app->show_F_vec)
            do_slider(CLAY_ID("FScaleSlider"), TR(STR_F_SCALE), Sf("10^%.0f", app->vec_scale_F),
                      &app->vec_scale_F, -4, 20, track_c, fill_c, text_c);
    }

    CLAY_AUTO_ID({ .layout = { .childGap = 4, .sizing = { CLAY_SIZING_GROW(0), CLAY_SIZING_FIT(0) } } }) {
        if (do_toggle(CLAY_ID("TglGij"), TR(STR_GIJ), app->show_Gij, fill_c, track_c, hov, wcfg))
            app->show_Gij = !app->show_Gij;
        if (do_toggle(CLAY_ID("TglScale"), TR(STR_SCALE_BAR), app->show_scale_bar, fill_c, track_c, hov, wcfg))
            app->show_scale_bar = !app->show_scale_bar;
    }

    /* Axes slider */
    do_slider(CLAY_ID("AxesSlider"), TR(STR_AXES), Sf("%.1f", app->axis_scale),
              &app->axis_scale, 0, 10, track_c, fill_c, text_c);

    /* Telemetry plots: show toggle, position, range */
    CLAY(CLAY_ID("PlotCtrl"), {
        .layout = { .childGap = 2, .sizing = { CLAY_SIZING_GROW(0), CLAY_SIZING_FIT(0) } }
    }) {
        if (do_toggle(CLAY_ID("TglPlots"), TR(STR_PLOTS), app->show_plots, fill_c, track_c, hov, wcfg))
            app->show_plots = !app->show_plots;
        if (app->show_plots) {
            const char *pos_labels[] = {TR(STR_BOTTOM), TR(STR_TOP)};
            for (int i = 0; i < 2; i++) {
                if (do_toggle(CLAY_IDI("PlotPos", i), pos_labels[i], app->plots_edge == i,
                              fill_c, track_c, hov, wcfg))
                    app->plots_edge = i;
            }
        }
    }
    if (app->show_plots) {
        CLAY(CLAY_ID("PRGroup"), {
            .layout = { .childGap = 2, .sizing = { CLAY_SIZING_GROW(0), CLAY_SIZING_FIT(0) } }
        }) {
            const char *plabels[] = {"x1", "x10", "x100"};
            for (int i = 0; i < 3; i++) {
                if (do_toggle(CLAY_IDI("PRBtn", i), plabels[i], app->plot_range == i,
                              fill_c, track_c, hov, wcfg))
                    app->plot_range = i;
            }
        }
    }

    CLAY_AUTO_ID({ .layout = { .childGap = 4, .sizing = { CLAY_SIZING_GROW(0), CLAY_SIZING_FIT(0) } } }) {
        if (do_toggle(CLAY_ID("TglAS"), TR(STR_AUTOSCALE), app->pitch_autoscale, fill_c, track_c, hov, wcfg))
            app->pitch_autoscale = !app->pitch_autoscale;
        if (do_toggle(CLAY_ID("TglRad"), TR(STR_RADIATION), app->radiation_loss, fill_c, track_c, hov, wcfg))
            app->radiation_loss = !app->radiation_loss;
    }
    if (app->radiation_loss) {
        do_slider(CLAY_ID("RadSlider"), TR(STR_RAD_X), Sf("x10^%.0f", app->radiation_mult),
                  &app->radiation_mult, 0, 12, track_c, fill_c, text_c);
    }

    if (do_toggle(CLAY_ID("TglRel"), TR(STR_RELATIVISTIC), app->relativistic, fill_c, track_c, hov, wcfg))
        app->relativistic = !app->relativistic;

    if (do_toggle(CLAY_ID("TglFollow"), TR(STR_FOLLOW), app->follow_particle, fill_c, track_c, hov, wcfg))
        app->follow_particle = !app->follow_particle;

    /* Camera view cycling */
    CLAY_AUTO_ID({ .layout = { .childGap = 4, .sizing = { CLAY_SIZING_GROW(0), CLAY_SIZING_FIT(0) } } }) {
        const char *vlabels[] = {TR(STR_CAM_FREE), TR(STR_CAM_PB), TR(STR_CAM_MB)};
        if (do_button(CLAY_ID("CamViewBtn"), vlabels[app->cam_field_aligned],
                      track_c, fill_c, wcfg)) {
            app->cam_field_aligned = (app->cam_field_aligned + 1) % 3;
            if (app->cam_field_aligned) app->follow_particle = 1;
            app->frame_e1_init = 0;
        }
    }
}

static void section_readouts(AppState *app, Clay_Color text_c, Clay_Color dim_c)
{
    do_section_header(app, 4, TR(STR_READOUTS), dim_c, text_c);
    if (!app->ui_section_open[4]) return;

    Clay_TextElementConfig rcfg = { .fontId = FONT_MONO, .fontSize = UZ(13), .letterSpacing = 1, .textColor = text_c };

    int si = app->selected_particle;
    const Particle *sp = &app->particles[si];
    double E_now = boris_energy_keV(sp, app->relativistic);
    double E0 = app->E0_keVs[si];
    double dE = (E0 > 0) ? fabs(E_now - E0) / E0 : 0;
    FieldModel *cfm = &app->models[app->current_model];

    if (app->n_particles > 1)
        CLAY_TEXT(Sf(TR(STR_FMT_PARTICLE_N), si + 1, app->n_particles), CLAY_TEXT_CONFIG(rcfg));
    CLAY_TEXT(Sf(TR(STR_FMT_TIME), app->sim_time), CLAY_TEXT_CONFIG(rcfg));
    CLAY_TEXT(Sf(TR(STR_FMT_ENERGY_ERR), dE), CLAY_TEXT_CONFIG(rcfg));
    CLAY_TEXT(Sf(TR(STR_FMT_PITCH), boris_pitch_angle(sp, cfm)), CLAY_TEXT_CONFIG(rcfg));
    CLAY_TEXT(Sf(TR(STR_FMT_MU), boris_mu(sp, cfm)), CLAY_TEXT_CONFIG(rcfg));
    CLAY_TEXT(Sf(TR(STR_FMT_BMAG), field_Bmag(cfm, sp->pos)), CLAY_TEXT_CONFIG(rcfg));

    if (app->radiation_loss) {
        double E_J = E_now * KEV_TO_J;
        double P = app->rad_power;
        double mult = pow(10.0, (double)app->radiation_mult);
        double P_eff = P * mult;
        if (E_J >= 1e6 * KEV_TO_J)
            CLAY_TEXT(Sf(TR(STR_FMT_ENERGY_MEV), E_J / (1e3 * KEV_TO_J)), CLAY_TEXT_CONFIG(rcfg));
        else
            CLAY_TEXT(Sf(TR(STR_FMT_ENERGY_KEV), E_J / KEV_TO_J), CLAY_TEXT_CONFIG(rcfg));
        CLAY_TEXT(Sf(TR(STR_FMT_PRAD), P), CLAY_TEXT_CONFIG(rcfg));
        if (mult > 1.5)
            CLAY_TEXT(Sf(TR(STR_FMT_DEDT_MULT), -P_eff / KEV_TO_J, mult), CLAY_TEXT_CONFIG(rcfg));
        else
            CLAY_TEXT(Sf(TR(STR_FMT_DEDT), -P_eff / KEV_TO_J), CLAY_TEXT_CONFIG(rcfg));
    }

    /* Position readouts */
    Vec3 pp = sp->pos;
    double rr = sqrt(pp.x*pp.x + pp.y*pp.y + pp.z*pp.z);
    double rho = sqrt(pp.x*pp.x + pp.y*pp.y);
    CLAY_AUTO_ID({ .layout = { .sizing = { CLAY_SIZING_GROW(0), CLAY_SIZING_FIT(0) } } }) {
        CLAY_TEXT(Sf("x %8.3f", pp.x), CLAY_TEXT_CONFIG(rcfg));
        CLAY_TEXT(Sf("  y %8.3f", pp.y), CLAY_TEXT_CONFIG(rcfg));
        CLAY_TEXT(Sf("  z %8.3f", pp.z), CLAY_TEXT_CONFIG(rcfg));
    }
    CLAY_AUTO_ID({ .layout = { .sizing = { CLAY_SIZING_GROW(0), CLAY_SIZING_FIT(0) } } }) {
        CLAY_TEXT(Sf("r %8.3f", rr), CLAY_TEXT_CONFIG(rcfg));
        CLAY_TEXT(Sf("  rho %8.3f", rho), CLAY_TEXT_CONFIG(rcfg));
    }

    /* Particle legend (when multiple particles) */
    if (app->n_particles > 1) {
        static const char *sp_names[] = {"e-", "e+", "p+", "a2+", "O+", "?"};

        CLAY_AUTO_ID({ .layout = { .sizing = { CLAY_SIZING_GROW(0), CLAY_SIZING_FIXED(4) } } }) {}
        CLAY_AUTO_ID({
            .layout = { .sizing = { CLAY_SIZING_GROW(0), CLAY_SIZING_FIXED(1) } },
            .backgroundColor = dim_c
        }) {}
        CLAY_AUTO_ID({ .layout = { .sizing = { CLAY_SIZING_GROW(0), CLAY_SIZING_FIXED(2) } } }) {}

        for (int i = 0; i < app->n_particles; i++) {
            int spi = app->particles[i].species;
            if (spi < 0 || spi >= NUM_SPECIES) spi = NUM_SPECIES - 1;
            Color sc = app->species_colors[spi];
            Clay_Color pc = {sc.r, sc.g, sc.b, 255};
            if (i == app->selected_particle) {
                /* brighten selected */
                pc.r += (255 - pc.r) * 0.3f;
                pc.g += (255 - pc.g) * 0.3f;
                pc.b += (255 - pc.b) * 0.3f;
            }
            double Ei = boris_energy_keV(&app->particles[i], app->relativistic);
            Clay_TextElementConfig lcfg = { .fontId = FONT_MONO, .fontSize = UZ(12),
                .letterSpacing = 1, .textColor = pc };
            const char *sel = (i == app->selected_particle) ? ">" : " ";
            if (Ei >= 1000.0)
                CLAY_TEXT(Sf("%s%d %s %.1f MeV", sel, i+1, sp_names[spi], Ei/1000.0),
                          CLAY_TEXT_CONFIG(lcfg));
            else
                CLAY_TEXT(Sf("%s%d %s %.1f keV", sel, i+1, sp_names[spi], Ei),
                          CLAY_TEXT_CONFIG(lcfg));
        }
    }
}

/* ======== Dropdown floating elements ======== */

static void declare_dropdowns(AppState *app, Clay_Color text_c, Clay_Color dim_c,
                              Clay_Color panel_bg)
{
    Clay_Color item_hover = app->dark_mode ? (Clay_Color){50,50,65,255} : (Clay_Color){200,200,215,255};
    Clay_TextElementConfig tcfg = { .fontId = FONT_UI, .fontSize = UZ(13), .letterSpacing = 1, .textColor = text_c };

    /* Field model dropdown list */
    if (g_field_dd_open) {
        CLAY(CLAY_ID("FieldDDList"), {
            .floating = { .attachTo = CLAY_ATTACH_TO_ELEMENT_WITH_ID,
                          .parentId = CLAY_ID("FieldDDBtn").id,
                          .zIndex = 10,
                          .attachPoints = { CLAY_ATTACH_POINT_LEFT_TOP, CLAY_ATTACH_POINT_LEFT_BOTTOM } },
            .layout = { .layoutDirection = CLAY_TOP_TO_BOTTOM,
                         .sizing = { CLAY_SIZING_FIXED(app->ui_panel_width - 16), CLAY_SIZING_FIT(0) } },
            .backgroundColor = panel_bg,
            .border = { .width = CLAY_BORDER_OUTSIDE(1), .color = {80,80,90,180} }
        }) {
            for (int i = 0; i < FIELD_NUM_MODELS; i++) {
                Clay_ElementId iid = CLAY_IDI("FItem", i);
                CLAY(iid, {
                    .layout = { .sizing = { CLAY_SIZING_GROW(0), CLAY_SIZING_FIXED(22) },
                                 .padding = {6,6,2,2}, .childAlignment = { .y = CLAY_ALIGN_Y_CENTER } },
                    .backgroundColor = (Clay_Hovered() || i == app->current_model) ? item_hover : panel_bg
                }) {
                    CLAY_TEXT(S(app->models[i].name), CLAY_TEXT_CONFIG(tcfg));
                }
                if (Clay_PointerOver(iid) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                    if (i != app->current_model) { app->current_model = i; app->needs_reset = 2; }
                    g_field_dd_open = 0;
                }
            }
        }
    }

    /* Species dropdown list */
    if (g_species_dd_open) {
        const char *names[] = {TR(STR_SPECIES_ELECTRON), TR(STR_SPECIES_POSITRON),
            TR(STR_SPECIES_PROTON), TR(STR_SPECIES_ALPHA), TR(STR_SPECIES_OPLUS), TR(STR_SPECIES_CUSTOM)};
        CLAY(CLAY_ID("SpeciesDDList"), {
            .floating = { .attachTo = CLAY_ATTACH_TO_ELEMENT_WITH_ID,
                          .parentId = CLAY_ID("SpeciesDDBtn").id, .zIndex = 10,
                          .attachPoints = { CLAY_ATTACH_POINT_LEFT_TOP, CLAY_ATTACH_POINT_LEFT_BOTTOM } },
            .layout = { .layoutDirection = CLAY_TOP_TO_BOTTOM,
                         .sizing = { CLAY_SIZING_FIXED(app->ui_panel_width - 16), CLAY_SIZING_FIT(0) } },
            .backgroundColor = panel_bg,
            .border = { .width = CLAY_BORDER_OUTSIDE(1), .color = {80,80,90,180} }
        }) {
            for (int i = 0; i < 6; i++) {
                Clay_ElementId iid = CLAY_IDI("SItem", i);
                CLAY(iid, {
                    .layout = { .sizing = { CLAY_SIZING_GROW(0), CLAY_SIZING_FIXED(22) },
                                 .padding = {6,6,2,2}, .childAlignment = { .y = CLAY_ALIGN_Y_CENTER } },
                    .backgroundColor = (Clay_Hovered() || i == app->species) ? item_hover : panel_bg
                }) {
                    CLAY_TEXT(S(names[i]), CLAY_TEXT_CONFIG(tcfg));
                }
                if (Clay_PointerOver(iid) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                    if (i != app->species) app->species = i;
                    g_species_dd_open = 0;
                }
            }
        }
    }

    /* Stellarator config dropdown list */
    if (g_stel_dd_open) {
        const char *cfgs[] = {TR(STR_STEL_CFG_R2), TR(STR_STEL_CFG_LP22)};
        CLAY(CLAY_ID("StelDDList"), {
            .floating = { .attachTo = CLAY_ATTACH_TO_ELEMENT_WITH_ID,
                          .parentId = CLAY_ID("StelDDBtn").id, .zIndex = 10,
                          .attachPoints = { CLAY_ATTACH_POINT_LEFT_TOP, CLAY_ATTACH_POINT_LEFT_BOTTOM } },
            .layout = { .layoutDirection = CLAY_TOP_TO_BOTTOM,
                         .sizing = { CLAY_SIZING_FIXED(app->ui_panel_width - 16), CLAY_SIZING_FIT(0) } },
            .backgroundColor = panel_bg,
            .border = { .width = CLAY_BORDER_OUTSIDE(1), .color = {80,80,90,180} }
        }) {
            FieldModel *fm = &app->models[app->current_model];
            int cur = (int)fm->params[0];
            for (int i = 0; i < 2; i++) {
                Clay_ElementId iid = CLAY_IDI("SCItem", i);
                CLAY(iid, {
                    .layout = { .sizing = { CLAY_SIZING_GROW(0), CLAY_SIZING_FIXED(22) },
                                 .padding = {6,6,2,2}, .childAlignment = { .y = CLAY_ALIGN_Y_CENTER } },
                    .backgroundColor = (Clay_Hovered() || i == cur) ? item_hover : panel_bg
                }) {
                    CLAY_TEXT(S(cfgs[i]), CLAY_TEXT_CONFIG(tcfg));
                }
                if (Clay_PointerOver(iid) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                    if (i != cur) { fm->params[0] = (double)i; app->needs_reset = 2; }
                    g_stel_dd_open = 0;
                }
            }
        }
    }

    /* Language dropdown list */
    if (g_lang_dd_open) {
        float max_h = app->win_h * 0.6f;
        CLAY(CLAY_ID("LangDDList"), {
            .floating = { .attachTo = CLAY_ATTACH_TO_ELEMENT_WITH_ID,
                          .parentId = CLAY_ID("LangDDBtn").id,
                          .zIndex = 10,
                          .attachPoints = { CLAY_ATTACH_POINT_LEFT_TOP, CLAY_ATTACH_POINT_LEFT_BOTTOM } },
            .layout = { .layoutDirection = CLAY_TOP_TO_BOTTOM,
                         .sizing = { CLAY_SIZING_FIXED(app->ui_panel_width - 16),
                                     CLAY_SIZING_FIT(.max = max_h) } },
            .backgroundColor = panel_bg,
            .border = { .width = CLAY_BORDER_OUTSIDE(1), .color = {80,80,90,180} },
            .clip = { .vertical = true, .childOffset = Clay_GetScrollOffset() }
        }) {
            LangId cur_lang = i18n_get_lang();
            for (int i = 0; i < LANG__COUNT; i++) {
                const LangInfo *li = i18n_lang_info((LangId)i);
                Clay_ElementId iid = CLAY_IDI("LItem", i);
                CLAY(iid, {
                    .layout = { .sizing = { CLAY_SIZING_GROW(0), CLAY_SIZING_FIXED(22) },
                                 .padding = {6,6,2,2}, .childAlignment = { .y = CLAY_ALIGN_Y_CENTER } },
                    .backgroundColor = (Clay_Hovered() || i == (int)cur_lang) ? item_hover : panel_bg
                }) {
                    CLAY_TEXT(S(li->name_en), CLAY_TEXT_CONFIG(tcfg));
                }
                if (Clay_PointerOver(iid) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                    if (i != (int)cur_lang) {
                        i18n_set_lang((LangId)i);
                        app->language = i;
                        field_retranslate(app->models);
                        app->needs_font_reload = 1;
                    }
                    g_lang_dd_open = 0;
                }
            }
        }
    }
}

/* ======== Settings section (replaces main sections when active) ======== */

static void section_settings(AppState *app, Clay_Color text_c, Clay_Color dim_c,
                             Clay_Color track_c, Clay_Color fill_c,
                             Clay_TextElementConfig *wcfg)
{
    Clay_Color hov = {60,60,80,255};

    /* Section title */
    CLAY_TEXT(S(TR(STR_SETTINGS)), CLAY_TEXT_CONFIG({
        .fontId = FONT_UI, .fontSize = UZ(15), .letterSpacing = 1, .textColor = text_c }));

    /* --- Language --- */
    CLAY_TEXT(S(TR(STR_LANGUAGE)), CLAY_TEXT_CONFIG({ .fontId = FONT_UI, .fontSize = UZ(13),
              .letterSpacing = 1, .textColor = dim_c }));
    {
        const LangInfo *cur = i18n_lang_info(i18n_get_lang());
        Clay_ElementId lid = CLAY_ID("LangDDBtn");
        Clay_Color lbg = Clay_Hovered() ? (Clay_Color){60,60,70,255} : (Clay_Color){30,30,36,255};
        if (!app->dark_mode) lbg = Clay_Hovered() ? (Clay_Color){210,210,218,255} : (Clay_Color){232,232,236,255};
        CLAY(lid, {
            .layout = { .sizing = { CLAY_SIZING_GROW(0), CLAY_SIZING_FIXED(24) },
                         .padding = {6,6,3,3}, .childAlignment = { .y = CLAY_ALIGN_Y_CENTER } },
            .backgroundColor = lbg,
            .border = { .width = CLAY_BORDER_OUTSIDE(1), .color = {80,80,90,160} },
            .cornerRadius = CLAY_CORNER_RADIUS(3)
        }) {
            CLAY_TEXT(S(cur->name_en), CLAY_TEXT_CONFIG({ .fontId = FONT_UI, .fontSize = UZ(13),
                      .letterSpacing = 1, .textColor = text_c }));
        }
        if (Clay_PointerOver(lid) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            g_lang_dd_open = !g_lang_dd_open;
            g_field_dd_open = 0; g_species_dd_open = 0; g_stel_dd_open = 0;
        }
    }

    /* Dark / Light mode */
    CLAY_AUTO_ID({ .layout = { .childGap = 4, .sizing = { CLAY_SIZING_GROW(0), CLAY_SIZING_FIT(0) } } }) {
        int prev_dark = app->dark_mode;
        if (do_toggle(CLAY_ID("STglDark"), TR(STR_DARK_MODE), app->dark_mode, fill_c, track_c, hov, wcfg))
            app->dark_mode = !app->dark_mode;
        if (app->dark_mode != prev_dark) {
            app_switch_colors(app, prev_dark);
            app_apply_theme(app);
        }
    }

    /* UI position: Left / Right */
    CLAY_AUTO_ID({ .layout = { .childGap = 4, .sizing = { CLAY_SIZING_GROW(0), CLAY_SIZING_FIT(0) } } }) {
        CLAY_TEXT(S(TR(STR_PANEL)), CLAY_TEXT_CONFIG({ .fontId = FONT_UI, .fontSize = UZ(13),
                  .letterSpacing = 1, .textColor = dim_c }));
        const char *elabels[] = {TR(STR_LEFT), TR(STR_RIGHT)};
        for (int i = 0; i < 2; i++) {
            if (do_toggle(CLAY_IDI("EdgeBtn", i), elabels[i], app->ui_edge == i,
                          fill_c, track_c, hov, wcfg))
                app->ui_edge = i;
        }
    }

    /* Stereo 3D */
    CLAY_AUTO_ID({ .layout = { .childGap = 4, .sizing = { CLAY_SIZING_GROW(0), CLAY_SIZING_FIT(0) } } }) {
        if (do_toggle(CLAY_ID("STglStereo"), TR(STR_STEREO_3D), app->stereo_3d, fill_c, track_c, hov, wcfg))
            app->stereo_3d = !app->stereo_3d;
    }
    if (app->stereo_3d) {
        do_slider(CLAY_ID("SSepSlider"), TR(STR_SEPARATION), Sf("%.3f", app->stereo_separation),
                  &app->stereo_separation, 0.005f, 0.15f, track_c, fill_c, text_c);
        do_slider(CLAY_ID("SGapSlider"), TR(STR_GAP), Sf("%.2f", app->stereo_gap),
                  &app->stereo_gap, -0.5f, 0.5f, track_c, fill_c, text_c);
    }

    /* --- Arrowhead & vector scales --- */
    do_slider(CLAY_ID("SArrowHead"), TR(STR_ARROWHEAD), Sf("%.3f", app->arrow_head_size),
              &app->arrow_head_size, 0.002f, 0.1f, track_c, fill_c, text_c);

    /* --- Line widths --- */
    CLAY_TEXT(S(TR(STR_LINE_WIDTHS)), CLAY_TEXT_CONFIG({ .fontId = FONT_UI, .fontSize = UZ(13),
              .letterSpacing = 1, .textColor = dim_c }));
    {
        const char *wl[] = {"1","2","3","4","5"};
        float label_w = 50;

#define LW_ROW_BEGIN(lbl)                                                     \
    CLAY_AUTO_ID({                                                            \
        .layout = { .childGap = 2,                                            \
                    .sizing = { CLAY_SIZING_GROW(0), CLAY_SIZING_FIT(0) } }   \
    }) {                                                                      \
        CLAY_AUTO_ID({                                                        \
            .layout = { .sizing = { CLAY_SIZING_FIXED(label_w), CLAY_SIZING_FIT(0) }, \
                        .childAlignment = { .x = CLAY_ALIGN_X_RIGHT } }       \
        }) {                                                                  \
            CLAY_TEXT(S(lbl), CLAY_TEXT_CONFIG({ .fontId = FONT_UI,            \
                .fontSize = UZ(11), .letterSpacing = 1, .textColor = dim_c })); \
        }

#define LW_BTN(id, idx, cur)                                                  \
        if (do_toggle(CLAY_IDI(id, idx), wl[idx], (cur) == (idx)+1,           \
                      fill_c, track_c, hov, wcfg))

        LW_ROW_BEGIN(TR(STR_LW_TRAIL))
            for (int i = 0; i < 5; i++)
                LW_BTN("LWT", i, (int)app->trail_thickness)
                    app->trail_thickness = (float)(i + 1);
        }
        LW_ROW_BEGIN(TR(STR_LW_B_LINES))
            for (int i = 0; i < 5; i++)
                LW_BTN("LWFL", i, app->lw_field_lines)
                    app->lw_field_lines = i + 1;
        }
        LW_ROW_BEGIN(TR(STR_LW_FL_GC))
            for (int i = 0; i < 5; i++)
                LW_BTN("LWGC", i, app->lw_gc_fl)
                    app->lw_gc_fl = i + 1;
        }
        LW_ROW_BEGIN(TR(STR_LW_FL_POS))
            for (int i = 0; i < 5; i++)
                LW_BTN("LWFP", i, app->lw_pos_fl)
                    app->lw_pos_fl = i + 1;
        }
        LW_ROW_BEGIN(TR(STR_LW_ARROWS))
            for (int i = 0; i < 5; i++)
                LW_BTN("LWA", i, app->lw_arrows)
                    app->lw_arrows = i + 1;
        }
        LW_ROW_BEGIN(TR(STR_LW_KAPPA))
            for (int i = 0; i < 5; i++)
                LW_BTN("LWTR", i, app->lw_triad)
                    app->lw_triad = i + 1;
        }
        LW_ROW_BEGIN(TR(STR_LW_AXES))
            for (int i = 0; i < 5; i++)
                LW_BTN("LWAX", i, app->lw_axes)
                    app->lw_axes = i + 1;
        }
#undef LW_ROW_BEGIN
#undef LW_BTN
    }

    /* --- Colors --- */
    CLAY_TEXT(S(TR(STR_COLORS)), CLAY_TEXT_CONFIG({ .fontId = FONT_UI, .fontSize = UZ(13),
              .letterSpacing = 1, .textColor = dim_c }));
    {
#define COLOR_ROW(lbl, sid, col)                                              \
        CLAY_AUTO_ID({ .layout = { .childGap = 6,                             \
            .sizing = { CLAY_SIZING_GROW(0), CLAY_SIZING_FIT(0) },            \
            .childAlignment = { .y = CLAY_ALIGN_Y_CENTER } } }) {             \
            do_swatch(CLAY_ID(sid), col);                                      \
            CLAY_TEXT(S(lbl), CLAY_TEXT_CONFIG({ .fontId = FONT_UI,            \
                .fontSize = UZ(11), .letterSpacing = 1, .textColor = dim_c })); \
        }

        COLOR_ROW(TR(STR_COLOR_V_ARROW),     "SCVel",  &app->color_vel)
        COLOR_ROW(TR(STR_COLOR_B_ARROW),    "SCB",    &app->color_B)
        COLOR_ROW(TR(STR_COLOR_F_ARROW),    "SCF",    &app->color_F)
        COLOR_ROW(TR(STR_COLOR_FIELD_LINES),"SCFL",   &app->color_field_lines)
        COLOR_ROW(TR(STR_COLOR_FL_GC),      "SCGCFL", &app->color_gc_fl)
        COLOR_ROW(TR(STR_COLOR_FL_POS),     "SCPFL",  &app->color_pos_fl)
        COLOR_ROW(TR(STR_COLOR_KAPPA),      "SCKap",  &app->color_kappa)
        COLOR_ROW(TR(STR_COLOR_BINORMAL),   "SCBin",  &app->color_binormal)
        COLOR_ROW(TR(STR_COLOR_AXES),       "SCAx",   &app->color_axes)

        CLAY_AUTO_ID({ .layout = { .sizing = { CLAY_SIZING_GROW(0), CLAY_SIZING_FIXED(4) } } }) {}
        CLAY_TEXT(S(TR(STR_PARTICLES)), CLAY_TEXT_CONFIG({ .fontId = FONT_UI, .fontSize = UZ(13),
                  .letterSpacing = 1, .textColor = dim_c }));
        {
            const char *sp_names[] = {TR(STR_SPECIES_ELECTRON), TR(STR_SPECIES_POSITRON),
                TR(STR_SPECIES_PROTON), TR(STR_SPECIES_ALPHA), TR(STR_SPECIES_OPLUS), TR(STR_SPECIES_CUSTOM)};
            for (int i = 0; i < NUM_SPECIES; i++) {
                CLAY_AUTO_ID({ .layout = { .childGap = 6,
                    .sizing = { CLAY_SIZING_GROW(0), CLAY_SIZING_FIT(0) },
                    .childAlignment = { .y = CLAY_ALIGN_Y_CENTER } } }) {
                    do_swatch(CLAY_IDI("SCSp", i), &app->species_colors[i]);
                    CLAY_TEXT(S(sp_names[i]), CLAY_TEXT_CONFIG({ .fontId = FONT_UI,
                        .fontSize = UZ(11), .letterSpacing = 1, .textColor = dim_c }));
                }
            }
        }
#undef COLOR_ROW
    }

}

/* ======== Main layout ======== */

static void declare_layout(AppState *app)
{
    g_dark_mode = app->dark_mode;
    g_ui_zoom = app->ui_zoom;
    Clay_Color bg = RC(app->theme.panel_bg);
    Clay_Color text_c = RC(app->theme.text);
    Clay_Color dim_c = RC(app->theme.text_dim);
    Clay_Color track_c = app->dark_mode
        ? (Clay_Color){30, 30, 36, 255}
        : (Clay_Color){210, 210, 218, 255};
    Clay_Color fill_c = app->dark_mode
        ? (Clay_Color){80, 80, 106, 255}
        : (Clay_Color){128, 128, 160, 255};

    Clay_TextElementConfig wcfg = { .fontId = FONT_UI, .fontSize = UZ(12),
        .letterSpacing = 1, .textColor = text_c };
    float pw = app->ui_panel_width;

    int panel_left = (app->ui_edge == 0);
    Clay_BorderElementConfig panel_border = panel_left
        ? (Clay_BorderElementConfig){ .width = { .right = 1 }, .color = dim_c }
        : (Clay_BorderElementConfig){ .width = { .left = 1 }, .color = dim_c };

    CLAY(CLAY_ID("Root"), {
        .layout = { .sizing = { CLAY_SIZING_GROW(0), CLAY_SIZING_GROW(0) } }
    }) {
        if (!panel_left) {
            CLAY(CLAY_ID("SceneSpacer"), {
                .layout = { .sizing = { CLAY_SIZING_GROW(0), CLAY_SIZING_GROW(0) } }
            }) {}
        }

        /* Panel */
        CLAY(CLAY_ID("Panel"), {
            .layout = {
                .layoutDirection = CLAY_TOP_TO_BOTTOM,
                .sizing = { CLAY_SIZING_FIXED(pw), CLAY_SIZING_GROW(0) },
                .padding = {8, 8, 8, 8},
                .childGap = 6
            },
            .backgroundColor = bg,
            .border = panel_border
        }) {
            /* Title row with settings + help buttons */
            CLAY_AUTO_ID({
                .layout = { .sizing = { CLAY_SIZING_GROW(0), CLAY_SIZING_FIT(0) },
                             .childAlignment = { .y = CLAY_ALIGN_Y_CENTER } }
            }) {
                CLAY_TEXT(S(TR(STR_LORENTZ_TRACER)), CLAY_TEXT_CONFIG({
                    .fontId = FONT_UI, .fontSize = UZ(18), .letterSpacing = 1, .textColor = text_c }));
                CLAY_AUTO_ID({ .layout = { .sizing = { .width = CLAY_SIZING_GROW(0) } } }) {}
                /* Settings button */
                {
                    Clay_ElementId sbid = CLAY_ID("SettingsBtn");
                    CLAY(sbid, {
                        .layout = { .sizing = { CLAY_SIZING_FIXED(22), CLAY_SIZING_FIXED(18) },
                                     .childAlignment = { .x = CLAY_ALIGN_X_CENTER, .y = CLAY_ALIGN_Y_CENTER } },
                        .backgroundColor = Clay_Hovered() ? fill_c : track_c,
                        .border = { .width = CLAY_BORDER_OUTSIDE(1), .color = {80,80,90,120} },
                        .cornerRadius = CLAY_CORNER_RADIUS(3)
                    }) {
                        CLAY_TEXT(CLAY_STRING("*"), CLAY_TEXT_CONFIG({
                            .fontId = FONT_UI, .fontSize = UZ(14), .letterSpacing = 0, .textColor = text_c }));
                    }
                    if (Clay_PointerOver(sbid) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
                        app->show_settings = !app->show_settings;
                }
                /* Help button */
                {
                    Clay_ElementId hbid = CLAY_ID("HelpBtn");
                    CLAY(hbid, {
                        .layout = { .sizing = { CLAY_SIZING_FIXED(22), CLAY_SIZING_FIXED(18) },
                                     .childAlignment = { .x = CLAY_ALIGN_X_CENTER, .y = CLAY_ALIGN_Y_CENTER } },
                        .backgroundColor = Clay_Hovered() ? fill_c : track_c,
                        .border = { .width = CLAY_BORDER_OUTSIDE(1), .color = {80,80,90,120} },
                        .cornerRadius = CLAY_CORNER_RADIUS(3)
                    }) {
                        CLAY_TEXT(CLAY_STRING("?"), CLAY_TEXT_CONFIG({
                            .fontId = FONT_UI, .fontSize = UZ(13), .letterSpacing = 0, .textColor = text_c }));
                    }
                    if (Clay_PointerOver(hbid) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
                        app->show_help = 1;
                }
            }

            /* Scrollable content area */
            CLAY(CLAY_ID("PanelScroll"), {
                .layout = { .layoutDirection = CLAY_TOP_TO_BOTTOM,
                             .sizing = { CLAY_SIZING_GROW(0), CLAY_SIZING_GROW(0) },
                             .childGap = 6 },
                .clip = { .vertical = true, .childOffset = Clay_GetScrollOffset() }
            }) {
                if (app->show_settings)
                    section_settings(app, text_c, dim_c, track_c, fill_c, &wcfg);
                else {
                    section_explore(app, text_c, dim_c, track_c, fill_c, &wcfg);
                    section_model(app, text_c, dim_c, track_c, fill_c, &wcfg);
                    section_playback(app, text_c, dim_c, track_c, fill_c, &wcfg);
                    section_display(app, text_c, dim_c, track_c, fill_c, &wcfg);
                    section_readouts(app, text_c, dim_c);
                }
            }
        }

        if (panel_left) {
            CLAY(CLAY_ID("SceneSpacer"), {
                .layout = { .sizing = { CLAY_SIZING_GROW(0), CLAY_SIZING_GROW(0) } }
            }) {}
        }

        /* Floating dropdown overlays */
        declare_dropdowns(app, text_c, dim_c, bg);

        /* Floating color picker tooltip */
        declare_color_picker(bg, text_c);
    }
}

/* ======== First-run language picker ======== */

static void declare_lang_picker(AppState *app)
{
    int dark = app->dark_mode;
    Clay_Color panel_c = dark
        ? (Clay_Color){22, 22, 30, 245} : (Clay_Color){242, 242, 248, 245};
    Clay_Color text_c = dark
        ? (Clay_Color){230, 230, 230, 255} : (Clay_Color){30, 30, 30, 255};
    Clay_Color dim_c = dark
        ? (Clay_Color){160, 160, 170, 255} : (Clay_Color){100, 100, 110, 255};
    Clay_Color accent = dark
        ? (Clay_Color){60, 100, 180, 255} : (Clay_Color){40, 80, 160, 255};
    Clay_Color accent_hover = dark
        ? (Clay_Color){80, 120, 200, 255} : (Clay_Color){60, 100, 180, 255};
    Clay_Color item_bg = dark
        ? (Clay_Color){30, 30, 40, 255} : (Clay_Color){235, 235, 240, 255};
    Clay_Color item_hover = dark
        ? (Clay_Color){50, 50, 65, 255} : (Clay_Color){210, 210, 225, 255};

    CLAY(CLAY_ID("LangPickRoot"), {
        .layout = { .sizing = { CLAY_SIZING_GROW(0), CLAY_SIZING_GROW(0) },
                     .childAlignment = { .x = CLAY_ALIGN_X_CENTER,
                                          .y = CLAY_ALIGN_Y_CENTER } },
        .backgroundColor = {0, 0, 0, 160}
    }) {
        CLAY(CLAY_ID("LangPickPanel"), {
            .layout = { .layoutDirection = CLAY_TOP_TO_BOTTOM,
                         .sizing = { CLAY_SIZING_FIXED(400),
                                     CLAY_SIZING_FIT(.max = (float)GetScreenHeight() - 40) },
                         .padding = {20, 20, 16, 16}, .childGap = 10 },
            .backgroundColor = panel_c,
            .cornerRadius = CLAY_CORNER_RADIUS(8)
        }) {
            /* Title */
            CLAY_TEXT(S(TR(STR_LANG_TITLE)),
                CLAY_TEXT_CONFIG({ .fontId = FONT_UI, .fontSize = 18,
                    .letterSpacing = 1, .textColor = text_c }));

            /* Language grid (scrollable if tall) */
            LangId cur = i18n_get_lang();
            CLAY(CLAY_ID("LangPickScroll"), {
                .layout = { .layoutDirection = CLAY_TOP_TO_BOTTOM,
                             .sizing = { CLAY_SIZING_GROW(0), CLAY_SIZING_GROW(0) },
                             .childGap = 4 },
                .clip = { .vertical = true, .childOffset = Clay_GetScrollOffset() }
            }) {
            for (int i = 0; i < LANG__COUNT; i++) {
                const LangInfo *li = i18n_lang_info((LangId)i);
                Clay_ElementId lid = CLAY_IDI("LPItem", i);
                int selected = (i == (int)cur);
                Clay_Color bg = selected ? accent :
                    (Clay_Hovered() ? item_hover : item_bg);
                Clay_Color tc = selected
                    ? (Clay_Color){255,255,255,255} : text_c;

                CLAY(lid, {
                    .layout = { .sizing = { CLAY_SIZING_GROW(0), CLAY_SIZING_FIXED(26) },
                                 .padding = {10, 10, 3, 3},
                                 .childAlignment = { .y = CLAY_ALIGN_Y_CENTER } },
                    .backgroundColor = bg,
                    .cornerRadius = CLAY_CORNER_RADIUS(3)
                }) {
                    CLAY_TEXT(S(li->name_en), CLAY_TEXT_CONFIG({
                        .fontId = FONT_UI, .fontSize = 14,
                        .letterSpacing = 1, .textColor = tc }));
                }
                if (Clay_PointerOver(lid) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                    i18n_set_lang((LangId)i);
                    app->language = i;
                    field_retranslate(app->models);
                }
            }
            } /* end LangPickScroll */

            /* Continue button */
            Clay_ElementId cid = CLAY_ID("LPContinue");
            CLAY(cid, {
                .layout = { .sizing = { CLAY_SIZING_GROW(0), CLAY_SIZING_FIXED(32) },
                             .childAlignment = { .x = CLAY_ALIGN_X_CENTER,
                                                  .y = CLAY_ALIGN_Y_CENTER } },
                .backgroundColor = Clay_Hovered() ? accent_hover : accent,
                .cornerRadius = CLAY_CORNER_RADIUS(4)
            }) {
                CLAY_TEXT(S(TR(STR_LANG_CONTINUE)), CLAY_TEXT_CONFIG({
                    .fontId = FONT_UI, .fontSize = 14, .letterSpacing = 1,
                    .textColor = (Clay_Color){255,255,255,255} }));
            }
            if (Clay_PointerOver(cid) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
                app->show_lang_picker = 0;
        }
    }
}

/* ======== Public API ======== */

void ui_update(struct AppState *app)
{
    /* Handle clay reinitialization on capacity overflow */
    /* Deferred font reload (must happen outside Clay layout pass) */
    if (app->needs_font_reload) {
        app->needs_font_reload = 0;
        app_reload_fonts(app);
    }

    if (g_reinit_clay) {
        uint64_t sz = Clay_MinMemorySize();
        free(g_clay_mem);
        g_clay_mem = malloc(sz);
        Clay_Arena arena = Clay_CreateArenaWithCapacityAndMemory(sz, g_clay_mem);
        Clay_Initialize(arena, (Clay_Dimensions){(float)app->win_w, (float)app->win_h},
                        (Clay_ErrorHandler){clay_error, 0});
        Clay_SetMeasureTextFunction(measure_text, g_fonts);
        g_reinit_clay = false;
    }

    g_sp = 0; /* reset string pool */
    if (!IsMouseButtonDown(MOUSE_BUTTON_LEFT)) g_active_slider = 0;

    Clay_SetPointerState(
        (Clay_Vector2){GetMousePosition().x, GetMousePosition().y},
        IsMouseButtonDown(MOUSE_BUTTON_LEFT));
    Clay_SetLayoutDimensions(
        (Clay_Dimensions){(float)app->win_w, (float)app->win_h});
    /* Pass scroll input to clay for scroll containers (help, picker, panel) */
    float wheel_y = 0;
    int mouse_over_panel = 0;
    if (app->show_help || app->playback.show_picker) {
        wheel_y = GetMouseWheelMoveV().y * 3.0f;
        if (IsKeyDown(KEY_DOWN))      wheel_y -= 3.0f;
        if (IsKeyDown(KEY_UP))        wheel_y += 3.0f;
        if (IsKeyPressed(KEY_PAGE_DOWN)) wheel_y -= 20.0f;
        if (IsKeyPressed(KEY_PAGE_UP))   wheel_y += 20.0f;
    } else if (app->ui_visible) {
        /* Check if mouse is over the panel area */
        Vector2 m = GetMousePosition();
        float pw = app->ui_panel_width;
        if (app->ui_edge == 0)
            mouse_over_panel = (m.x < pw);
        else
            mouse_over_panel = (m.x >= app->win_w - pw);
        if (mouse_over_panel)
            wheel_y = GetMouseWheelMoveV().y * 3.0f;
    }
    app->mouse_over_panel = mouse_over_panel;
    Clay_UpdateScrollContainers(true, (Clay_Vector2){0, wheel_y}, GetFrameTime());

    Clay_BeginLayout();
    if (app->show_lang_picker)
        declare_lang_picker(app);
    else if (app->playback.show_picker)
        playback_declare_layout(app);
    else if (app->show_help)
        help_declare_layout(app);
    else {
        declare_layout(app);
        if (app->tutorial.active)
            tutorial_declare_layout(app);
        explorer_declare_dialog(app);
    }
    if (app->explorer.show_picker)
        explorer_declare_picker(app);
    g_cmds = Clay_EndLayout();

    /* Dismiss floating elements on click outside */
    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        if (g_field_dd_open &&
            !Clay_PointerOver(CLAY_ID("FieldDDList")) &&
            !Clay_PointerOver(CLAY_ID("FieldDDBtn")))
            g_field_dd_open = 0;
        if (g_species_dd_open &&
            !Clay_PointerOver(CLAY_ID("SpeciesDDList")) &&
            !Clay_PointerOver(CLAY_ID("SpeciesDDBtn")))
            g_species_dd_open = 0;
        if (g_stel_dd_open &&
            !Clay_PointerOver(CLAY_ID("StelDDList")) &&
            !Clay_PointerOver(CLAY_ID("StelDDBtn")))
            g_stel_dd_open = 0;
        if (g_lang_dd_open &&
            !Clay_PointerOver(CLAY_ID("LangDDList")) &&
            !Clay_PointerOver(CLAY_ID("LangDDBtn")))
            g_lang_dd_open = 0;
        if (g_cpick_id &&
            !Clay_PointerOver(CLAY_ID("ColorPicker"))) {
            /* Don't dismiss if click was on the swatch that owns this picker */
            Clay_ElementId owner = { .id = g_cpick_id };
            if (!Clay_PointerOver(owner)) {
                g_cpick_target = NULL; g_cpick_id = 0;
            }
        }
        if (app->playback.show_picker &&
            !Clay_PointerOver(CLAY_ID("PickerPanel")))
            app->playback.show_picker = 0;
    }
}

void ui_render(struct AppState *app)
{
    (void)app;
    clay_render_cmds(g_cmds);
}
