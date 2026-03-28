/* explorer.c — Guided scenario explorer for Lorentz Tracer
 * Combines event-log playback with dialog/prompt flow control. */

#include "explorer.h"
#include "i18n.h"
#include "app.h"
#include "playback.h"
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

/* ---- Init / free ---- */

void explorer_init(Explorer *ex)
{
    memset(ex, 0, sizeof(*ex));
    ex->selected = -1;
}

void explorer_free(Explorer *ex)
{
    free(ex->events);
    ex->events = NULL;
    ex->n_events = 0;
    ex->capacity = 0;
}

/* ---- Event array helper ---- */

static void push_event(Explorer *ex, ExplorerEvent ev)
{
    if (ex->n_events >= ex->capacity) {
        ex->capacity = ex->capacity ? ex->capacity * 2 : 256;
        ex->events = realloc(ex->events, ex->capacity * sizeof(ExplorerEvent));
    }
    ex->events[ex->n_events++] = ev;
}

/* ---- Directory scanning ---- */

static int cmp_files(const void *a, const void *b) {
    return strcmp((const char *)a, (const char *)b);
}

/* Parse title from first few lines of a scenario file */
static void parse_title_from_file(const char *dir, const char *fname, char *title, int title_len)
{
    char path[768];
    snprintf(path, sizeof(path), "%s/%s", dir, fname);
    FILE *f = fopen(path, "r");
    title[0] = '\0';
    if (!f) return;

    char line[512];
    while (fgets(line, sizeof(line), f)) {
        int len = (int)strlen(line);
        while (len > 0 && (line[len-1] == '\n' || line[len-1] == '\r')) line[--len] = '\0';
        if (line[0] == '#') continue;
        if (len == 0) continue;

        char key[64];
        if (sscanf(line, "%63s", key) == 1 && strcmp(key, "title") == 0) {
            /* Read next line as title */
            if (fgets(line, sizeof(line), f)) {
                len = (int)strlen(line);
                while (len > 0 && (line[len-1] == '\n' || line[len-1] == '\r')) line[--len] = '\0';
                snprintf(title, title_len, "%s", line);
            }
            break;
        }
        /* If we hit model/species/param before title, no title in file */
        if (strcmp(key, "model") == 0 || strcmp(key, "species") == 0 ||
            strcmp(key, "param") == 0 || strcmp(key, "fps") == 0) break;
    }
    fclose(f);
}

void explorer_scan_files(AppState *app)
{
    Explorer *ex = &app->explorer;
    snprintf(ex->dir, sizeof(ex->dir), "%s/scenarios", app->res_dir);
    ex->n_files = 0;
    ex->selected = -1;

#ifdef _WIN32
    char pattern[600];
    snprintf(pattern, sizeof(pattern), "%s\\*.txt", ex->dir);
    WIN32_FIND_DATAA fd;
    HANDLE h = FindFirstFileA(pattern, &fd);
    if (h == INVALID_HANDLE_VALUE) return;
    do {
        if (ex->n_files >= EXPLORE_MAX_FILES) break;
        if (!(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
            strncpy(ex->files[ex->n_files], fd.cFileName, 255);
            ex->files[ex->n_files][255] = '\0';
            ex->n_files++;
        }
    } while (FindNextFileA(h, &fd));
    FindClose(h);
#else
    DIR *d = opendir(ex->dir);
    if (!d) return;
    struct dirent *ent;
    while ((ent = readdir(d)) != NULL) {
        if (ex->n_files >= EXPLORE_MAX_FILES) break;
        int len = (int)strlen(ent->d_name);
        if (len > 4 && strcmp(ent->d_name + len - 4, ".txt") == 0) {
            strncpy(ex->files[ex->n_files], ent->d_name, 255);
            ex->files[ex->n_files][255] = '\0';
            ex->n_files++;
        }
    }
    closedir(d);
#endif

    qsort(ex->files, ex->n_files, 256, cmp_files);

    /* Parse titles */
    for (int i = 0; i < ex->n_files; i++)
        parse_title_from_file(ex->dir, ex->files[i], ex->titles[i], 128);
}

/* ---- Parse scenario file ---- */

int explorer_load(AppState *app, const char *path)
{
    FILE *f = fopen(path, "r");
    if (!f) return -1;

    Explorer *ex = &app->explorer;
    explorer_free(ex);
    ex->title[0] = '\0';
    ex->description[0] = '\0';

    char line[1024];
    int in_events = 0;
    int in_description = 0;

    while (fgets(line, sizeof(line), f)) {
        int len = (int)strlen(line);
        while (len > 0 && (line[len-1] == '\n' || line[len-1] == '\r')) line[--len] = '\0';

        /* Handle multi-line blocks */
        if (in_description) {
            if (strcmp(line, "end_description") == 0) {
                in_description = 0;
                continue;
            }
            /* Append to description */
            int dlen = (int)strlen(ex->description);
            if (dlen > 0 && dlen < (int)sizeof(ex->description) - 2) {
                ex->description[dlen++] = '\n';
                ex->description[dlen] = '\0';
            }
            int rem = (int)sizeof(ex->description) - dlen - 1;
            if (rem > 0) strncat(ex->description + dlen, line, rem);
            continue;
        }

        if (line[0] == '#') {
            if (strstr(line, "# Events:")) in_events = 1;
            continue;
        }
        if (len == 0) continue;

        if (!in_events) {
            /* Pre-event section: initial state + metadata */
            char key[128], rest[512];
            rest[0] = '\0';
            if (sscanf(line, "%127s %511[^\n]", key, rest) < 1) continue;

            if (strcmp(key, "title") == 0) {
                /* Title is on the next line */
                if (fgets(line, sizeof(line), f)) {
                    len = (int)strlen(line);
                    while (len > 0 && (line[len-1] == '\n' || line[len-1] == '\r')) line[--len] = '\0';
                    snprintf(ex->title, sizeof(ex->title), "%s", line);
                }
            } else if (strcmp(key, "description") == 0) {
                in_description = 1;
            } else if (strcmp(key, "model") == 0) {
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
                int idx; double val;
                sscanf(line, "param %d %lf", &idx, &val);
                if (idx >= 0 && idx < FIELD_MAX_PARAMS)
                    app->models[app->current_model].params[idx] = val;
            } else if (strncmp(key, "model_", 6) == 0) {
                int m, p; double val;
                if (sscanf(key, "model_%d_param_%d", &m, &p) == 2) {
                    val = atof(rest);
                    if (m >= 0 && m < FIELD_NUM_MODELS && p >= 0 && p < FIELD_MAX_PARAMS)
                        app->models[m].params[p] = val;
                }
            }
        } else {
            /* Event stream */
            ExplorerEvent ev = {0};
            char key[32];
            double rt, st;
            char val_rest[256] = "";
            int n = sscanf(line, "%d %lf %lf %31s %255[^\n]",
                           &ev.frame, &rt, &st, key, val_rest);
            if (n < 4) continue;
            strncpy(ev.key, key, 31);
            ev.key[31] = '\0';

            if (strcmp(key, "end") == 0) {
                push_event(ex, ev);
                break;
            }

            if (strcmp(key, "dialog") == 0) {
                /* Read multi-line text until end_dialog */
                ev.text[0] = '\0';
                while (fgets(line, sizeof(line), f)) {
                    len = (int)strlen(line);
                    while (len > 0 && (line[len-1] == '\n' || line[len-1] == '\r')) line[--len] = '\0';
                    if (strcmp(line, "end_dialog") == 0) break;
                    int tlen = (int)strlen(ev.text);
                    if (tlen > 0 && tlen < EXPLORE_TEXT_MAX - 2) {
                        ev.text[tlen++] = ' ';
                        ev.text[tlen] = '\0';
                    }
                    int rem = EXPLORE_TEXT_MAX - (int)strlen(ev.text) - 1;
                    if (rem > 0) strncat(ev.text, line, rem);
                }
                push_event(ex, ev);
            } else if (strcmp(key, "wait_action") == 0) {
                strncpy(ev.action_name, val_rest, 31);
                ev.action_name[31] = '\0';
                push_event(ex, ev);
            } else if (strcmp(key, "wait_frames") == 0) {
                ev.wait_count = atoi(val_rest);
                push_event(ex, ev);
            } else if (strcmp(key, "wait_seconds") == 0) {
                ev.wait_seconds = atof(val_rest);
                ev.wait_count = (int)(ev.wait_seconds * 60.0 + 0.5); /* 60 fps */
                push_event(ex, ev);
            } else {
                /* Standard event (same as playback) */
                if (strcmp(key, "init_pos") == 0 || strcmp(key, "camera_pos") == 0 ||
                    strcmp(key, "camera_target") == 0) {
                    ev.n_values = 3;
                    sscanf(val_rest, "%lf %lf %lf", &ev.values[0], &ev.values[1], &ev.values[2]);
                } else if (strlen(val_rest) > 0) {
                    ev.n_values = 1;
                    ev.values[0] = atof(val_rest);
                }
                push_event(ex, ev);
            }
        }
    }

    fclose(f);
    return 0;
}

/* ---- Action detection (mirrors tutorial.c pattern) ---- */

void explorer_snapshot(AppState *app)
{
    Explorer *ex = &app->explorer;
    ex->prev_paused = app->paused;
    ex->prev_show_vel = app->show_velocity_vec;
    ex->prev_show_B = app->show_B_vec;
    ex->prev_show_fl = app->show_field_lines;
    ex->prev_follow = app->follow_particle;
    ex->prev_cam_aligned = app->cam_field_aligned;
    ex->prev_model = app->current_model;
    ex->prev_pitch = app->pitch_deg;
    ex->prev_cam_pos = app->camera.position;
    ex->camera_moved = 0;
}

static const char *action_hint(const char *action_name)
{
    if (strcmp(action_name, "press_space") == 0)    return "Press Space";
    if (strcmp(action_name, "toggle_vel") == 0)     return "Toggle velocity vector (1)";
    if (strcmp(action_name, "toggle_B") == 0)       return "Toggle B vector (2)";
    if (strcmp(action_name, "toggle_fl") == 0)      return "Toggle field lines (3)";
    if (strcmp(action_name, "orbit_camera") == 0)   return "Orbit the camera (right-drag)";
    if (strcmp(action_name, "follow_on") == 0)      return "Press F to follow";
    if (strcmp(action_name, "field_aligned") == 0)  return "Press V for field-aligned view";
    if (strcmp(action_name, "change_model") == 0)   return "Select a different field model";
    if (strcmp(action_name, "change_pitch") == 0)   return "Adjust the pitch angle";
    return "";
}

int explorer_check_action(AppState *app)
{
    Explorer *ex = &app->explorer;
    const char *act = ex->events[ex->event_cursor].action_name;

    if (strcmp(act, "press_space") == 0)    return app->paused != ex->prev_paused;
    if (strcmp(act, "toggle_vel") == 0)     return app->show_velocity_vec != ex->prev_show_vel;
    if (strcmp(act, "toggle_B") == 0)       return app->show_B_vec != ex->prev_show_B;
    if (strcmp(act, "toggle_fl") == 0)      return app->show_field_lines != ex->prev_show_fl;
    if (strcmp(act, "orbit_camera") == 0)   return ex->camera_moved;
    if (strcmp(act, "follow_on") == 0)      return app->follow_particle && !ex->prev_follow;
    if (strcmp(act, "field_aligned") == 0)  return app->cam_field_aligned && !ex->prev_cam_aligned;
    if (strcmp(act, "change_model") == 0)   return app->current_model != ex->prev_model;
    if (strcmp(act, "change_pitch") == 0)   return fabs(app->pitch_deg - ex->prev_pitch) > 0.5;
    return 0;
}

/* ---- Playback control ---- */

void explorer_start(AppState *app)
{
    Explorer *ex = &app->explorer;
    ex->active = 1;
    ex->current_frame = 0;
    ex->event_cursor = 0;
    ex->show_picker = 0;
    ex->paused_for_dialog = 0;
    ex->paused_for_action = 0;
    ex->wait_frames_remaining = 0;
    ex->dialog_text[0] = '\0';
    ex->action_hint[0] = '\0';

    /* Stop tutorial and playback */
    if (app->tutorial.active) app->tutorial.active = 0;
    if (app->playback.active) app->playback.active = 0;

    app->needs_reset = 2;
    explorer_snapshot(app);
}

void explorer_stop(AppState *app)
{
    Explorer *ex = &app->explorer;
    ex->active = 0;
    ex->paused_for_dialog = 0;
    ex->paused_for_action = 0;
    ex->wait_frames_remaining = 0;
}

void explorer_step(AppState *app)
{
    Explorer *ex = &app->explorer;
    if (!ex->active) return;

    /* Detect camera movement for orbit_camera action */
    Vector3 cp = app->camera.position;
    float dx = cp.x - ex->prev_cam_pos.x;
    float dy = cp.y - ex->prev_cam_pos.y;
    float dz = cp.z - ex->prev_cam_pos.z;
    if (dx*dx + dy*dy + dz*dz > 0.01f) ex->camera_moved = 1;

    /* Wait states */
    if (ex->wait_frames_remaining > 0) {
        ex->wait_frames_remaining--;
        return;
    }
    if (ex->paused_for_dialog) return;  /* waiting for Continue click */
    if (ex->paused_for_action) {
        if (explorer_check_action(app)) {
            ex->paused_for_action = 0;
            ex->dialog_text[0] = '\0';
            ex->action_hint[0] = '\0';
            explorer_snapshot(app);
            ex->event_cursor++;
        }
        return;
    }

    /* Process events at current_frame */
    while (ex->event_cursor < ex->n_events) {
        ExplorerEvent *ev = &ex->events[ex->event_cursor];
        if (ev->frame > ex->current_frame) break;

        if (strcmp(ev->key, "end") == 0) {
            explorer_stop(app);
            return;
        }

        if (strcmp(ev->key, "dialog") == 0) {
            snprintf(ex->dialog_text, sizeof(ex->dialog_text), "%s", ev->text);
            ex->paused_for_dialog = 1;
            ex->event_cursor++;
            return;
        }

        if (strcmp(ev->key, "wait_action") == 0) {
            snprintf(ex->action_hint, sizeof(ex->action_hint), "%s",
                     action_hint(ev->action_name));
            ex->paused_for_action = 1;
            explorer_snapshot(app);
            return;  /* don't increment cursor; check_action uses it */
        }

        if (strcmp(ev->key, "wait_frames") == 0 || strcmp(ev->key, "wait_seconds") == 0) {
            ex->wait_frames_remaining = ev->wait_count;
            ex->event_cursor++;
            return;
        }

        /* Standard event: convert to PlaybackEvent and apply */
        PlaybackEvent pev = {0};
        pev.frame = ev->frame;
        strncpy(pev.key, ev->key, 31);
        pev.key[31] = '\0';
        pev.values[0] = ev->values[0];
        pev.values[1] = ev->values[1];
        pev.values[2] = ev->values[2];
        pev.n_values = ev->n_values;
        apply_event(app, &pev);

        ex->event_cursor++;
    }

    if (ex->event_cursor >= ex->n_events) {
        explorer_stop(app);
        return;
    }

    ex->current_frame++;
}

/* ---- File picker overlay ---- */

#define RC(c) ((Clay_Color){(float)(c).r, (float)(c).g, (float)(c).b, (float)(c).a})

void explorer_declare_picker(AppState *app)
{
    Explorer *ex = &app->explorer;
    if (!ex->show_picker) return;

    int dark = app->dark_mode;
    Clay_Color overlay_c = {0, 0, 0, 180};
    Clay_Color panel_c = dark
        ? (Clay_Color){22, 22, 30, 248} : (Clay_Color){242, 242, 248, 248};
    Clay_Color text_c = RC(app->theme.text);
    Clay_Color dim_c = RC(app->theme.text_dim);
    Clay_Color accent = RC(app->theme.trail);
    Clay_Color item_bg = dark
        ? (Clay_Color){30, 30, 38, 255} : (Clay_Color){232, 232, 238, 255};
    Clay_Color item_hover = dark
        ? (Clay_Color){45, 45, 60, 255} : (Clay_Color){210, 210, 225, 255};
    Clay_Color item_sel = dark
        ? (Clay_Color){50, 60, 80, 255} : (Clay_Color){190, 200, 225, 255};
    Clay_Color btn_c = dark
        ? (Clay_Color){50, 50, 65, 255} : (Clay_Color){210, 210, 220, 255};
    Clay_Color btn_hover = dark
        ? (Clay_Color){70, 70, 90, 255} : (Clay_Color){190, 190, 205, 255};

    float pw = app->win_w - 80.0f;
    if (pw > 700) pw = 700;
    float ph = app->win_h - 80.0f;
    if (ph > 550) ph = 550;

    CLAY(CLAY_ID("ExplorePickRoot"), {
        .layout = { .sizing = { CLAY_SIZING_GROW(0), CLAY_SIZING_GROW(0) },
                     .childAlignment = { .x = CLAY_ALIGN_X_CENTER, .y = CLAY_ALIGN_Y_CENTER } },
        .backgroundColor = overlay_c
    }) {
        CLAY(CLAY_ID("ExplorePickPanel"), {
            .layout = { .layoutDirection = CLAY_TOP_TO_BOTTOM,
                         .sizing = { CLAY_SIZING_FIXED(pw), CLAY_SIZING_FIXED(ph) },
                         .padding = {16, 16, 12, 12}, .childGap = 8 },
            .backgroundColor = panel_c,
            .cornerRadius = CLAY_CORNER_RADIUS(6)
        }) {
            /* Title */
            CLAY_TEXT(S(TR(STR_EXPLORE_OPEN)), CLAY_TEXT_CONFIG({
                .fontId = FONT_UI, .fontSize = 18, .letterSpacing = 1, .textColor = text_c }));

            /* Directory */
            {
                Clay_String dir_str = { .length = (int32_t)strlen(ex->dir), .chars = ex->dir };
                CLAY_TEXT(dir_str, CLAY_TEXT_CONFIG({
                    .fontId = FONT_MONO, .fontSize = 11, .letterSpacing = 1, .textColor = dim_c }));
            }

            if (ex->n_files == 0) {
                CLAY_TEXT(S(TR(STR_EXPLORE_NONE)), CLAY_TEXT_CONFIG({
                    .fontId = FONT_UI, .fontSize = 14, .letterSpacing = 1, .textColor = dim_c }));
            } else {
                /* Scrollable file list */
                CLAY(CLAY_ID("ExploreFileList"), {
                    .layout = { .layoutDirection = CLAY_TOP_TO_BOTTOM,
                                 .sizing = { CLAY_SIZING_GROW(0), CLAY_SIZING_GROW(0) },
                                 .childGap = 2 },
                    .clip = { .vertical = true, .childOffset = Clay_GetScrollOffset() }
                }) {
                    for (int i = 0; i < ex->n_files; i++) {
                        Clay_ElementId fid = CLAY_IDI("EFItem", i);
                        int selected = (i == ex->selected);
                        Clay_Color bg = selected ? item_sel :
                            (Clay_Hovered() ? item_hover : item_bg);

                        CLAY(fid, {
                            .layout = { .layoutDirection = CLAY_TOP_TO_BOTTOM,
                                         .sizing = { CLAY_SIZING_GROW(0), CLAY_SIZING_FIT(0) },
                                         .padding = {8, 8, 6, 6}, .childGap = 2 },
                            .backgroundColor = bg,
                            .cornerRadius = CLAY_CORNER_RADIUS(3)
                        }) {
                            /* Show title if available, else filename */
                            const char *display = ex->titles[i][0] ? ex->titles[i] : ex->files[i];
                            CLAY_TEXT(S(display), CLAY_TEXT_CONFIG({
                                .fontId = FONT_UI, .fontSize = 14, .letterSpacing = 1,
                                .textColor = selected ? accent : text_c }));
                            /* Filename below title */
                            if (ex->titles[i][0]) {
                                CLAY_TEXT(S(ex->files[i]), CLAY_TEXT_CONFIG({
                                    .fontId = FONT_MONO, .fontSize = 11, .letterSpacing = 1,
                                    .textColor = dim_c }));
                            }
                        }

                        if (Clay_PointerOver(fid) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                            if (ex->selected == i) {
                                /* Double-click: open */
                                char fullpath[768];
                                snprintf(fullpath, sizeof(fullpath), "%s/%s", ex->dir, ex->files[i]);
                                if (explorer_load(app, fullpath) == 0)
                                    explorer_start(app);
                            } else {
                                ex->selected = i;
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

            /* Buttons */
            CLAY_AUTO_ID({
                .layout = { .sizing = { CLAY_SIZING_GROW(0), CLAY_SIZING_FIT(0) },
                             .childGap = 8, .childAlignment = { .x = CLAY_ALIGN_X_RIGHT } }
            }) {
                /* Open */
                Clay_ElementId open_id = CLAY_ID("EFOpen");
                Clay_Color obg = (ex->selected >= 0)
                    ? (Clay_Hovered() ? btn_hover : btn_c)
                    : (Clay_Color){btn_c.r, btn_c.g, btn_c.b, 100};
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
                if (ex->selected >= 0 && Clay_PointerOver(open_id) &&
                    IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                    char fullpath[768];
                    snprintf(fullpath, sizeof(fullpath), "%s/%s", ex->dir, ex->files[ex->selected]);
                    if (explorer_load(app, fullpath) == 0)
                        explorer_start(app);
                }

                /* Cancel */
                Clay_ElementId cancel_id = CLAY_ID("EFCancel");
                CLAY(cancel_id, {
                    .layout = { .sizing = { CLAY_SIZING_FIXED(90), CLAY_SIZING_FIXED(28) },
                                 .padding = {8, 8, 4, 4},
                                 .childAlignment = { .x = CLAY_ALIGN_X_CENTER, .y = CLAY_ALIGN_Y_CENTER } },
                    .backgroundColor = Clay_Hovered() ? btn_hover : btn_c,
                    .border = { .width = CLAY_BORDER_OUTSIDE(1), .color = {80,80,90,120} },
                    .cornerRadius = CLAY_CORNER_RADIUS(3)
                }) {
                    CLAY_TEXT(S(TR(STR_CANCEL)), CLAY_TEXT_CONFIG({
                        .fontId = FONT_UI, .fontSize = 14, .letterSpacing = 1, .textColor = text_c }));
                }
                if (Clay_PointerOver(cancel_id) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
                    ex->show_picker = 0;
            }

            CLAY_TEXT(S(TR(STR_EXPLORE_PICKER_HELP)),
                CLAY_TEXT_CONFIG({ .fontId = FONT_UI, .fontSize = 12, .letterSpacing = 1, .textColor = dim_c }));
        }
    }

    /* Escape closes picker */
    if (IsKeyPressed(KEY_ESCAPE)) ex->show_picker = 0;
}

/* ---- Dialog overlay ---- */

void explorer_declare_dialog(AppState *app)
{
    Explorer *ex = &app->explorer;
    if (!ex->active) return;
    if (!ex->paused_for_dialog && !ex->paused_for_action) return;

    int dark = app->dark_mode;
    Clay_Color panel_c = dark
        ? (Clay_Color){22, 22, 30, 245} : (Clay_Color){242, 242, 248, 245};
    Clay_Color text_c = dark
        ? (Clay_Color){230, 230, 230, 255} : (Clay_Color){30, 30, 30, 255};
    Clay_Color dim_c = dark
        ? (Clay_Color){160, 160, 170, 255} : (Clay_Color){100, 100, 110, 255};
    Clay_Color btn_c = dark
        ? (Clay_Color){50, 50, 65, 255} : (Clay_Color){210, 210, 220, 255};
    Clay_Color btn_hover = dark
        ? (Clay_Color){70, 70, 90, 255} : (Clay_Color){190, 190, 205, 255};
    Clay_Color accent = dark
        ? (Clay_Color){60, 100, 180, 255} : (Clay_Color){40, 80, 160, 255};
    Clay_Color accent_hover = dark
        ? (Clay_Color){80, 120, 200, 255} : (Clay_Color){60, 100, 180, 255};
    Clay_Color white = {255, 255, 255, 255};
    Clay_Color done_c = {80, 200, 100, 255};

    float pw = 520;
    if (pw > app->win_w - 40) pw = app->win_w - 40;

    /* Estimate height for bottom positioning */
    int msg_len = (int)strlen(ex->dialog_text);
    float lines = 1.0f + (float)(msg_len * 16) / (pw - 32);
    float est_h = lines * 23.0f + 80.0f;
    float oy = app->win_h - est_h - 10.0f;
    if (oy < 40) oy = 40;
    float ox = (app->win_w - pw) * 0.5f;

    CLAY(CLAY_ID("ExploreDialog"), {
        .floating = { .attachTo = CLAY_ATTACH_TO_ROOT, .zIndex = 30,
                      .offset = { .x = ox, .y = oy } },
        .layout = { .layoutDirection = CLAY_TOP_TO_BOTTOM,
                     .sizing = { CLAY_SIZING_FIXED(pw), CLAY_SIZING_FIT(0) },
                     .padding = {16, 16, 12, 12}, .childGap = 10 },
        .backgroundColor = panel_c,
        .border = { .width = CLAY_BORDER_OUTSIDE(1), .color = {100,100,110,160} },
        .cornerRadius = CLAY_CORNER_RADIUS(8)
    }) {
        /* Scenario title */
        if (ex->title[0]) {
            CLAY_TEXT(S(ex->title), CLAY_TEXT_CONFIG({
                .fontId = FONT_UI, .fontSize = 12, .letterSpacing = 1,
                .textColor = dim_c }));
        }

        /* Dialog text */
        if (ex->dialog_text[0]) {
            CLAY_TEXT(S(ex->dialog_text), CLAY_TEXT_CONFIG({
                .fontId = FONT_UI, .fontSize = 16, .letterSpacing = 1,
                .textColor = text_c, .wrapMode = CLAY_TEXT_WRAP_WORDS,
                .lineHeight = 23 }));
        }

        /* Action hint */
        if (ex->paused_for_action) {
            int action_done = explorer_check_action(app);
            if (action_done) {
                CLAY_TEXT(S(TR(STR_TUT_DONE_HINT)), CLAY_TEXT_CONFIG({
                    .fontId = FONT_UI, .fontSize = 12, .letterSpacing = 1,
                    .textColor = done_c }));
            } else if (ex->action_hint[0]) {
                CLAY_TEXT(S(ex->action_hint), CLAY_TEXT_CONFIG({
                    .fontId = FONT_UI, .fontSize = 12, .letterSpacing = 1,
                    .textColor = accent }));
            }
        }

        /* Button row */
        CLAY(CLAY_ID("ExplBtnRow"), {
            .layout = { .childGap = 8,
                .sizing = { CLAY_SIZING_GROW(0), CLAY_SIZING_FIT(0) },
                .childAlignment = { .y = CLAY_ALIGN_Y_CENTER } }
        }) {
            /* Stop button */
            Clay_ElementId stop_id = CLAY_ID("ExplStop");
            CLAY(stop_id, {
                .layout = { .sizing = { CLAY_SIZING_FIT(.min = 80), CLAY_SIZING_FIXED(28) },
                             .childAlignment = { .x = CLAY_ALIGN_X_CENTER, .y = CLAY_ALIGN_Y_CENTER },
                             .padding = {12, 12, 0, 0} },
                .backgroundColor = Clay_Hovered() ? btn_hover : btn_c,
                .border = { .width = CLAY_BORDER_OUTSIDE(1), .color = {100,100,110,120} },
                .cornerRadius = CLAY_CORNER_RADIUS(4)
            }) {
                CLAY_TEXT(S(TR(STR_TUT_STOP)), CLAY_TEXT_CONFIG({
                    .fontId = FONT_UI, .fontSize = 14, .letterSpacing = 1,
                    .textColor = dim_c }));
            }
            if (Clay_PointerOver(stop_id) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
                explorer_stop(app);

            /* Spacer */
            CLAY_AUTO_ID({ .layout = { .sizing = { .width = CLAY_SIZING_GROW(0) } } }) {}

            /* Continue button (only for dialog, not wait_action) */
            if (ex->paused_for_dialog) {
                Clay_ElementId cont_id = CLAY_ID("ExplContinue");
                CLAY(cont_id, {
                    .layout = { .sizing = { CLAY_SIZING_FIT(.min = 80), CLAY_SIZING_FIXED(28) },
                                 .childAlignment = { .x = CLAY_ALIGN_X_CENTER, .y = CLAY_ALIGN_Y_CENTER },
                                 .padding = {12, 12, 0, 0} },
                    .backgroundColor = Clay_Hovered() ? accent_hover : accent,
                    .border = { .width = CLAY_BORDER_OUTSIDE(1), .color = {100,100,110,120} },
                    .cornerRadius = CLAY_CORNER_RADIUS(4)
                }) {
                    CLAY_TEXT(S(TR(STR_TUT_CONTINUE)), CLAY_TEXT_CONFIG({
                        .fontId = FONT_UI, .fontSize = 14, .letterSpacing = 1,
                        .textColor = white }));
                }
                if (Clay_PointerOver(cont_id) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                    ex->paused_for_dialog = 0;
                    ex->dialog_text[0] = '\0';
                }
            }
        }
    }
}

/* ---- Export template ---- */

void explorer_export_template(AppState *app)
{
    char dir[1024];
    snprintf(dir, sizeof(dir), "%s/scenarios", app->res_dir);

    /* Ensure directory exists */
#ifdef _WIN32
    CreateDirectoryA(dir, NULL);
#else
    mkdir(dir, 0755);
#endif

    char path[1280];
    snprintf(path, sizeof(path), "%s/template.txt", dir);

    FILE *f = fopen(path, "w");
    if (!f) return;

    const FieldModel *fm = &app->models[app->current_model];

    fprintf(f, "# Lorentz Tracer scenario template\n");
    fprintf(f, "# Edit this file to create a guided scenario.\n\n");

    fprintf(f, "title My Scenario\n");
    fprintf(f, "description\n");
    fprintf(f, "Describe what the user will learn here.\n");
    fprintf(f, "end_description\n\n");

    fprintf(f, "# Initial state (captured from current app state)\n");
    fprintf(f, "model %d  # %s (%s)\n", app->current_model, fm->name, fm->code);
    for (int i = 0; i < fm->n_params; i++)
        fprintf(f, "param %d %.15g  # %s\n", i, fm->params[i], fm->param_names[i]);

    fprintf(f, "species %d\n", app->species);
    if (app->species == SPECIES_CUSTOM) {
        fprintf(f, "custom_q %.15g\n", app->custom_q);
        fprintf(f, "custom_m %.15g\n", app->custom_m);
        fprintf(f, "custom_speed %.15g\n", app->custom_speed);
    } else {
        fprintf(f, "E_keV %.15g\n", app->E_keV);
    }
    fprintf(f, "pitch_deg %.15g\n", app->pitch_deg);
    fprintf(f, "init_pos %.15g %.15g %.15g\n",
            app->init_pos.x, app->init_pos.y, app->init_pos.z);
    fprintf(f, "playback_speed %.15g\n", app->playback_speed);
    fprintf(f, "speed_range %d\n", app->speed_range);
    fprintf(f, "show_field_lines %d\n", app->show_field_lines);
    fprintf(f, "show_velocity_vec %d\n", app->show_velocity_vec);
    fprintf(f, "show_B_vec %d\n", app->show_B_vec);
    fprintf(f, "follow_particle %d\n", app->follow_particle);
    fprintf(f, "camera_pos %.9g %.9g %.9g\n",
            app->camera.position.x, app->camera.position.y, app->camera.position.z);
    fprintf(f, "camera_target %.9g %.9g %.9g\n",
            app->camera.target.x, app->camera.target.y, app->camera.target.z);
    fprintf(f, "camera_fovy %.9g\n", app->camera.fovy);

    fprintf(f, "\nfps 60\n");
    fprintf(f, "\n# Events: frame real_time sim_time key value\n");
    fprintf(f, "# Add dialog, wait_action, wait_seconds, and state events below.\n\n");

    fprintf(f, "0 0.0 0.0 dialog\n");
    fprintf(f, "Welcome to this scenario. Edit this text to describe\n");
    fprintf(f, "what the user is about to see.\n");
    fprintf(f, "end_dialog\n\n");

    fprintf(f, "0 0.0 0.0 paused 0\n\n");

    fprintf(f, "# wait_seconds 3.0\n");
    fprintf(f, "# wait_action toggle_vel\n\n");

    fprintf(f, "600 10.0 0.0 end\n");

    fclose(f);
}
