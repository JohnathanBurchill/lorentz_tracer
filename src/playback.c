/* playback.c — Event log playback and file picker for Lorentz Tracer */

#include "playback.h"
#include "i18n.h"
#include "app.h"
#include "clay.h"
#include "raylib.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define NOGDI
#define NOUSER
#include <windows.h>
#undef near
#undef far
#else
#include <dirent.h>
#include <sys/stat.h>
#endif

#define FONT_UI   0
#define FONT_MONO 1

static Clay_String S(const char *s) {
    return (Clay_String){ .length = (int32_t)strlen(s), .chars = s };
}

/* ---- resolve_movie_dir (same logic as app.c) ---- */
static void pb_movie_dir(char *buf, int buflen)
{
#ifdef _WIN32
    const char *home = getenv("USERPROFILE");
    if (home) { snprintf(buf, buflen, "%s\\Videos\\Lorentz_Tracer", home); return; }
#else
    const char *home = getenv("HOME");
    if (home) {
#ifdef __APPLE__
        snprintf(buf, buflen, "%s/Movies/Lorentz_Tracer", home);
#else
        const char *xdg = getenv("XDG_VIDEOS_DIR");
        if (xdg) snprintf(buf, buflen, "%s/Lorentz_Tracer", xdg);
        else snprintf(buf, buflen, "%s/Videos/Lorentz_Tracer", home);
#endif
        return;
    }
#endif
    snprintf(buf, buflen, ".");
}

/* ---- Init / free ---- */

void playback_init(Playback *pb)
{
    memset(pb, 0, sizeof(*pb));
    pb->selected = -1;
}

void playback_free(Playback *pb)
{
    free(pb->events);
    pb->events = NULL;
    pb->n_events = 0;
    pb->capacity = 0;
}

/* ---- Directory scanning ---- */

static int cmp_files_desc(const void *a, const void *b) {
    return strcmp((const char *)b, (const char *)a); /* reverse alphabetical = newest first */
}

void playback_scan_files(Playback *pb)
{
    pb_movie_dir(pb->dir, sizeof(pb->dir));
    pb->n_files = 0;
    pb->selected = -1;

#ifdef _WIN32
    char pattern[600];
    snprintf(pattern, sizeof(pattern), "%s\\*.txt", pb->dir);
    WIN32_FIND_DATAA fd;
    HANDLE h = FindFirstFileA(pattern, &fd);
    if (h == INVALID_HANDLE_VALUE) return;
    do {
        if (pb->n_files >= 256) break;
        if (!(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
            strncpy(pb->files[pb->n_files], fd.cFileName, 255);
            pb->files[pb->n_files][255] = '\0';
            pb->n_files++;
        }
    } while (FindNextFileA(h, &fd));
    FindClose(h);
#else
    DIR *d = opendir(pb->dir);
    if (!d) return;
    struct dirent *ent;
    while ((ent = readdir(d)) != NULL) {
        if (pb->n_files >= 256) break;
        int len = (int)strlen(ent->d_name);
        if (len > 4 && strcmp(ent->d_name + len - 4, ".txt") == 0) {
            strncpy(pb->files[pb->n_files], ent->d_name, 255);
            pb->files[pb->n_files][255] = '\0';
            pb->n_files++;
        }
    }
    closedir(d);
#endif

    /* Sort newest first (filenames are date-stamped) */
    qsort(pb->files, pb->n_files, 256, cmp_files_desc);
}

/* ---- Event array helpers ---- */

static void push_event(Playback *pb, PlaybackEvent ev)
{
    if (pb->n_events >= pb->capacity) {
        pb->capacity = pb->capacity ? pb->capacity * 2 : 1024;
        pb->events = realloc(pb->events, pb->capacity * sizeof(PlaybackEvent));
    }
    pb->events[pb->n_events++] = ev;
}

/* ---- Parse event log ---- */

int playback_load(AppState *app, const char *path)
{
    FILE *f = fopen(path, "r");
    if (!f) return -1;

    Playback *pb = &app->playback;
    playback_free(pb);

    char line[1024];
    int in_events = 0;

    while (fgets(line, sizeof(line), f)) {
        /* Strip newline */
        int len = (int)strlen(line);
        while (len > 0 && (line[len-1] == '\n' || line[len-1] == '\r')) line[--len] = '\0';

        if (line[0] == '#') {
            if (strstr(line, "# Events:")) in_events = 1;
            continue;
        }
        if (len == 0) continue;

        if (!in_events) {
            /* Parse initial state */
            char key[128], rest[512];
            if (sscanf(line, "%127s %511[^\n]", key, rest) < 1) continue;

            if (strcmp(key, "model") == 0) {
                app->current_model = atoi(rest);
            } else if (strcmp(key, "species") == 0) {
                app->species = atoi(rest);
            } else if (strcmp(key, "E_keV") == 0) {
                app->E_keV = atof(rest);
            } else if (strcmp(key, "pitch_deg") == 0) {
                app->pitch_deg = atof(rest);
            } else if (strcmp(key, "init_pos") == 0) {
                sscanf(rest, "%lf %lf %lf", &app->init_pos.x, &app->init_pos.y, &app->init_pos.z);
            } else if (strcmp(key, "custom_q") == 0) {
                app->custom_q = atof(rest);
            } else if (strcmp(key, "custom_m") == 0) {
                app->custom_m = atof(rest);
            } else if (strcmp(key, "custom_speed") == 0) {
                app->custom_speed = atof(rest);
            } else if (strcmp(key, "playback_speed") == 0) {
                app->playback_speed = atof(rest);
            } else if (strcmp(key, "speed_range") == 0) {
                app->speed_range = atoi(rest);
            } else if (strcmp(key, "dark_mode") == 0) {
                app->dark_mode = atoi(rest);
                app_apply_theme(app);
            } else if (strcmp(key, "show_field_lines") == 0) {
                app->show_field_lines = atoi(rest);
            } else if (strcmp(key, "show_gc_field_line") == 0) {
                app->show_gc_field_line = atoi(rest);
            } else if (strcmp(key, "show_gc_trajectory") == 0) {
                app->show_gc_trajectory = atoi(rest);
            } else if (strcmp(key, "gc_fl_length") == 0) {
                app->gc_fl_length = atof(rest);
            } else if (strcmp(key, "show_velocity_vec") == 0) {
                app->show_velocity_vec = atoi(rest);
            } else if (strcmp(key, "show_B_vec") == 0) {
                app->show_B_vec = atoi(rest);
            } else if (strcmp(key, "show_Gij") == 0) {
                app->show_Gij = atoi(rest);
            } else if (strcmp(key, "plot_range") == 0) {
                app->plot_range = atoi(rest);
            } else if (strcmp(key, "pitch_autoscale") == 0) {
                app->pitch_autoscale = atoi(rest);
            } else if (strcmp(key, "follow_particle") == 0) {
                app->follow_particle = atoi(rest);
            } else if (strcmp(key, "cam_field_aligned") == 0) {
                app->cam_field_aligned = atoi(rest);
            } else if (strcmp(key, "ui_visible") == 0) {
                app->ui_visible = atoi(rest);
            } else if (strcmp(key, "axis_scale") == 0) {
                app->axis_scale = (float)atof(rest);
            } else if (strcmp(key, "radiation_loss") == 0) {
                app->radiation_loss = atoi(rest);
            } else if (strcmp(key, "relativistic") == 0) {
                app->relativistic = atoi(rest);
            } else if (strcmp(key, "camera_pos") == 0) {
                double x, y, z;
                sscanf(rest, "%lf %lf %lf", &x, &y, &z);
                app->camera.position = (Vector3){(float)x, (float)y, (float)z};
            } else if (strcmp(key, "camera_target") == 0) {
                double x, y, z;
                sscanf(rest, "%lf %lf %lf", &x, &y, &z);
                app->camera.target = (Vector3){(float)x, (float)y, (float)z};
            } else if (strcmp(key, "camera_fovy") == 0) {
                app->camera.fovy = (float)atof(rest);
            } else if (strncmp(key, "param", 5) == 0) {
                /* param <idx> <val> for current model */
                int idx; double val;
                if (sscanf(rest, "%d %lf", &idx, &val) == 2)  // rest = "idx val # comment"
                    ; /* actually key="param", rest="0 1.5e-05  # B0" */
                /* re-parse from line: "param <idx> <val>" */
                sscanf(line, "param %d %lf", &idx, &val);
                if (idx >= 0 && idx < FIELD_MAX_PARAMS)
                    app->models[app->current_model].params[idx] = val;
            } else if (strncmp(key, "model_", 6) == 0) {
                /* model_<m>_param_<p> <val> */
                int m, p; double val;
                if (sscanf(key, "model_%d_param_%d", &m, &p) == 2) {
                    val = atof(rest);
                    if (m >= 0 && m < FIELD_NUM_MODELS && p >= 0 && p < FIELD_MAX_PARAMS)
                        app->models[m].params[p] = val;
                }
            }
            /* Skip: fps, param lines already handled */
        } else {
            /* Parse event stream: frame real_time sim_time key [values...] */
            PlaybackEvent ev = {0};
            char key[32];
            double rt, st;
            /* Try parsing: frame rt st key val1 val2 val3 */
            char val_rest[256] = "";
            int n = sscanf(line, "%d %lf %lf %31s %255[^\n]",
                           &ev.frame, &rt, &st, key, val_rest);
            if (n < 4) continue;
            strncpy(ev.key, key, 31);
            ev.key[31] = '\0';

            if (strcmp(key, "end") == 0) {
                push_event(pb, ev);
                break;
            }

            /* Parse values */
            if (strcmp(key, "init_pos") == 0 || strcmp(key, "camera_pos") == 0 ||
                strcmp(key, "camera_target") == 0) {
                ev.n_values = 3;
                sscanf(val_rest, "%lf %lf %lf", &ev.values[0], &ev.values[1], &ev.values[2]);
            } else if (strlen(val_rest) > 0) {
                ev.n_values = 1;
                ev.values[0] = atof(val_rest);
            }
            push_event(pb, ev);
        }
    }

    fclose(f);
    return 0;
}

/* ---- Apply a single event to AppState ---- */

void apply_event(AppState *app, const PlaybackEvent *ev)
{
    const double *v = ev->values;
    const char *key = ev->key;

    if (strcmp(key, "model") == 0)             { app->current_model = (int)v[0]; app->needs_reset = 2; }
    else if (strcmp(key, "species") == 0)       { app->species = (int)v[0]; app->needs_reset = 1; }
    else if (strcmp(key, "E_keV") == 0)         { app->E_keV = v[0]; app->needs_reset = 1; }
    else if (strcmp(key, "pitch_deg") == 0)     { app->pitch_deg = v[0]; app->needs_reset = 1; }
    else if (strcmp(key, "init_pos") == 0)      { app->init_pos = (Vec3){v[0], v[1], v[2]}; app->needs_reset = 1; }
    else if (strcmp(key, "custom_q") == 0)      { app->custom_q = v[0]; app->needs_reset = 1; }
    else if (strcmp(key, "custom_m") == 0)      { app->custom_m = v[0]; app->needs_reset = 1; }
    else if (strcmp(key, "custom_speed") == 0)  { app->custom_speed = v[0]; app->needs_reset = 1; }
    else if (strcmp(key, "playback_speed") == 0) app->playback_speed = v[0];
    else if (strcmp(key, "speed_range") == 0)    app->speed_range = (int)v[0];
    else if (strcmp(key, "paused") == 0)          app->paused = (int)v[0];
    else if (strcmp(key, "dark_mode") == 0)      { int old = app->dark_mode; app->dark_mode = (int)v[0]; if (app->dark_mode != old) app_switch_colors(app, old); app_apply_theme(app); }
    else if (strcmp(key, "show_field_lines") == 0)    app->show_field_lines = (int)v[0];
    else if (strcmp(key, "show_gc_field_line") == 0)   app->show_gc_field_line = (int)v[0];
    else if (strcmp(key, "show_gc_trajectory") == 0)  app->show_gc_trajectory = (int)v[0];
    else if (strcmp(key, "gc_fl_length") == 0)         app->gc_fl_length = v[0];
    else if (strcmp(key, "show_velocity_vec") == 0)    app->show_velocity_vec = (int)v[0];
    else if (strcmp(key, "show_B_vec") == 0)           app->show_B_vec = (int)v[0];
    else if (strcmp(key, "plot_range") == 0)            app->plot_range = (int)v[0];
    else if (strcmp(key, "pitch_autoscale") == 0)       app->pitch_autoscale = (int)v[0];
    else if (strcmp(key, "follow_particle") == 0)       app->follow_particle = (int)v[0];
    else if (strcmp(key, "cam_field_aligned") == 0)     app->cam_field_aligned = (int)v[0];
    else if (strcmp(key, "show_Gij") == 0)              app->show_Gij = (int)v[0];
    else if (strcmp(key, "ui_visible") == 0)             app->ui_visible = (int)v[0];
    else if (strcmp(key, "axis_scale") == 0)             app->axis_scale = (float)v[0];
    else if (strcmp(key, "radiation_loss") == 0)          app->radiation_loss = (int)v[0];
    else if (strcmp(key, "relativistic") == 0)            app->relativistic = (int)v[0];
    else if (strcmp(key, "camera_pos") == 0)
        app->camera.position = (Vector3){(float)v[0], (float)v[1], (float)v[2]};
    else if (strcmp(key, "camera_target") == 0)
        app->camera.target = (Vector3){(float)v[0], (float)v[1], (float)v[2]};
    else if (strcmp(key, "camera_fovy") == 0)
        app->camera.fovy = (float)v[0];
    else if (strcmp(key, "reset") == 0)
        app->needs_reset = (int)v[0];
    else if (strncmp(key, "model_", 6) == 0) {
        int m, p;
        if (sscanf(key, "model_%d_param_%d", &m, &p) == 2)
            if (m >= 0 && m < FIELD_NUM_MODELS && p >= 0 && p < FIELD_MAX_PARAMS)
                app->models[m].params[p] = v[0];
    }
}

/* ---- Playback control ---- */

void playback_start(AppState *app)
{
    Playback *pb = &app->playback;
    pb->active = 1;
    pb->current_frame = 0;
    pb->event_cursor = 0;
    pb->show_picker = 0;
    app->needs_reset = 2;
}

void playback_stop(AppState *app)
{
    app->playback.active = 0;
}

void playback_step(AppState *app)
{
    Playback *pb = &app->playback;
    if (!pb->active) return;

    /* Apply all events at current_frame */
    while (pb->event_cursor < pb->n_events) {
        PlaybackEvent *ev = &pb->events[pb->event_cursor];
        if (ev->frame > pb->current_frame) break;

        if (strcmp(ev->key, "end") == 0) {
            playback_stop(app);
            return;
        }
        apply_event(app, ev);
        pb->event_cursor++;
    }

    /* Stop if past all events */
    if (pb->event_cursor >= pb->n_events) {
        playback_stop(app);
        return;
    }

    pb->current_frame++;
}

/* ---- File picker clay overlay ---- */

#define RC(c) ((Clay_Color){(float)(c).r, (float)(c).g, (float)(c).b, (float)(c).a})

void playback_declare_layout(AppState *app)
{
    Playback *pb = &app->playback;

    Clay_Color overlay_c = {0, 0, 0, 180};
    Clay_Color panel_c = app->dark_mode
        ? (Clay_Color){22, 22, 30, 248} : (Clay_Color){242, 242, 248, 248};
    Clay_Color text_c = RC(app->theme.text);
    Clay_Color dim_c = RC(app->theme.text_dim);
    Clay_Color accent = RC(app->theme.trail);
    Clay_Color item_bg = app->dark_mode
        ? (Clay_Color){30, 30, 38, 255} : (Clay_Color){232, 232, 238, 255};
    Clay_Color item_hover = app->dark_mode
        ? (Clay_Color){45, 45, 60, 255} : (Clay_Color){210, 210, 225, 255};
    Clay_Color item_sel = app->dark_mode
        ? (Clay_Color){50, 60, 80, 255} : (Clay_Color){190, 200, 225, 255};

    float pw = app->win_w - 80.0f;
    if (pw > 700) pw = 700;
    float ph = app->win_h - 80.0f;
    if (ph > 550) ph = 550;

    CLAY(CLAY_ID("PickerRoot"), {
        .layout = { .sizing = { CLAY_SIZING_GROW(0), CLAY_SIZING_GROW(0) },
                     .childAlignment = { .x = CLAY_ALIGN_X_CENTER, .y = CLAY_ALIGN_Y_CENTER } },
        .backgroundColor = overlay_c
    }) {
        CLAY(CLAY_ID("PickerPanel"), {
            .layout = { .layoutDirection = CLAY_TOP_TO_BOTTOM,
                         .sizing = { CLAY_SIZING_FIXED(pw), CLAY_SIZING_FIXED(ph) },
                         .padding = {16, 16, 12, 12}, .childGap = 8 },
            .backgroundColor = panel_c,
            .cornerRadius = CLAY_CORNER_RADIUS(6)
        }) {
            /* Title */
            CLAY_TEXT(S(TR(STR_OPEN_RECORDING)), CLAY_TEXT_CONFIG({
                .fontId = FONT_UI, .fontSize = 18, .letterSpacing = 1, .textColor = text_c }));

            /* Directory path */
            {
                Clay_String dir_str = { .length = (int32_t)strlen(pb->dir), .chars = pb->dir };
                Clay_TextElementConfig dcfg = { .fontId = FONT_MONO, .fontSize = 11,
                    .letterSpacing = 1, .textColor = dim_c };
                CLAY_TEXT(dir_str, CLAY_TEXT_CONFIG(dcfg));
            }

            if (pb->n_files == 0) {
                CLAY_TEXT(S(TR(STR_NO_RECORDINGS)), CLAY_TEXT_CONFIG({
                    .fontId = FONT_UI, .fontSize = 14, .letterSpacing = 1, .textColor = dim_c }));
            } else {
                /* Scrollable file list */
                CLAY(CLAY_ID("FileList"), {
                    .layout = { .layoutDirection = CLAY_TOP_TO_BOTTOM,
                                 .sizing = { CLAY_SIZING_GROW(0), CLAY_SIZING_GROW(0) },
                                 .childGap = 2 },
                    .clip = { .vertical = true, .childOffset = Clay_GetScrollOffset() }
                }) {
                    for (int i = 0; i < pb->n_files; i++) {
                        Clay_ElementId fid = CLAY_IDI("FPItem", i);
                        int selected = (i == pb->selected);
                        Clay_Color bg = selected ? item_sel :
                            (Clay_Hovered() ? item_hover : item_bg);

                        CLAY(fid, {
                            .layout = { .sizing = { CLAY_SIZING_GROW(0), CLAY_SIZING_FIXED(28) },
                                         .padding = {8, 8, 4, 4},
                                         .childAlignment = { .y = CLAY_ALIGN_Y_CENTER } },
                            .backgroundColor = bg,
                            .cornerRadius = CLAY_CORNER_RADIUS(3)
                        }) {
                            Clay_String fname = { .length = (int32_t)strlen(pb->files[i]),
                                                  .chars = pb->files[i] };
                            Clay_TextElementConfig fcfg = { .fontId = FONT_MONO, .fontSize = 13,
                                .letterSpacing = 1, .textColor = selected ? accent : text_c };
                            CLAY_TEXT(fname, CLAY_TEXT_CONFIG(fcfg));
                        }

                        if (Clay_PointerOver(fid) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                            if (pb->selected == i) {
                                /* Double-click: open */
                                char fullpath[768];
                                snprintf(fullpath, sizeof(fullpath), "%s/%s", pb->dir, pb->files[i]);
                                if (playback_load(app, fullpath) == 0)
                                    playback_start(app);
                            } else {
                                pb->selected = i;
                            }
                        }
                    }
                }
            }

            /* Separator */
            CLAY_AUTO_ID({
                .layout = { .sizing = { CLAY_SIZING_GROW(0), CLAY_SIZING_FIXED(1) } },
                .backgroundColor = dim_c
            }) {}

            /* Buttons row */
            CLAY_AUTO_ID({
                .layout = { .sizing = { CLAY_SIZING_GROW(0), CLAY_SIZING_FIT(0) },
                             .childGap = 8, .childAlignment = { .x = CLAY_ALIGN_X_RIGHT } }
            }) {
                /* Open button */
                Clay_ElementId open_id = CLAY_ID("FPOpen");
                Clay_Color obg = (pb->selected >= 0)
                    ? (Clay_Hovered() ? item_hover : item_bg)
                    : (Clay_Color){item_bg.r, item_bg.g, item_bg.b, 100};
                CLAY(open_id, {
                    .layout = { .sizing = { CLAY_SIZING_FIXED(90), CLAY_SIZING_FIXED(28) },
                                 .padding = {8, 8, 4, 4},
                                 .childAlignment = { .x = CLAY_ALIGN_X_CENTER, .y = CLAY_ALIGN_Y_CENTER } },
                    .backgroundColor = obg,
                    .border = { .width = CLAY_BORDER_OUTSIDE(1), .color = {80,80,90,120} },
                    .cornerRadius = CLAY_CORNER_RADIUS(3)
                }) {
                    CLAY_TEXT(S(TR(STR_OPEN)), CLAY_TEXT_CONFIG({
                        .fontId = FONT_UI, .fontSize = 14, .letterSpacing = 1, .textColor = text_c }));
                }
                if (pb->selected >= 0 && Clay_PointerOver(open_id) &&
                    IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                    char fullpath[768];
                    snprintf(fullpath, sizeof(fullpath), "%s/%s", pb->dir, pb->files[pb->selected]);
                    if (playback_load(app, fullpath) == 0)
                        playback_start(app);
                }

                /* Cancel button */
                Clay_ElementId cancel_id = CLAY_ID("FPCancel");
                CLAY(cancel_id, {
                    .layout = { .sizing = { CLAY_SIZING_FIXED(90), CLAY_SIZING_FIXED(28) },
                                 .padding = {8, 8, 4, 4},
                                 .childAlignment = { .x = CLAY_ALIGN_X_CENTER, .y = CLAY_ALIGN_Y_CENTER } },
                    .backgroundColor = Clay_Hovered() ? item_hover : item_bg,
                    .border = { .width = CLAY_BORDER_OUTSIDE(1), .color = {80,80,90,120} },
                    .cornerRadius = CLAY_CORNER_RADIUS(3)
                }) {
                    CLAY_TEXT(S(TR(STR_CANCEL)), CLAY_TEXT_CONFIG({
                        .fontId = FONT_UI, .fontSize = 14, .letterSpacing = 1, .textColor = text_c }));
                }
                if (Clay_PointerOver(cancel_id) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
                    pb->show_picker = 0;
            }

            /* Footer */
            CLAY_TEXT(S(TR(STR_PICKER_HELP)),
                CLAY_TEXT_CONFIG({ .fontId = FONT_UI, .fontSize = 12, .letterSpacing = 1, .textColor = dim_c }));
        }
    }
}
