#ifndef EXPLORER_H
#define EXPLORER_H

#include "vec3.h"
#include "raylib.h"

struct AppState;

#define EXPLORE_TEXT_MAX 512
#define EXPLORE_MAX_FILES 64

/* Single event in a scenario */
typedef struct {
    int frame;
    char key[32];
    double values[3];
    int n_values;
    char text[EXPLORE_TEXT_MAX];   /* dialog text */
    char action_name[32];          /* for wait_action */
    int wait_count;                /* for wait_frames */
    double wait_seconds;           /* for wait_seconds */
} ExplorerEvent;

/* Explorer runtime state */
typedef struct {
    int active;
    int show_picker;
    int paused_for_dialog;
    int paused_for_action;
    int wait_frames_remaining;

    char dialog_text[EXPLORE_TEXT_MAX];
    char action_hint[64];

    /* Parsed scenario */
    ExplorerEvent *events;
    int n_events, capacity;
    int current_frame, event_cursor;

    /* Scenario metadata */
    char title[128];
    char description[512];

    /* File picker */
    char dir[512];
    char files[EXPLORE_MAX_FILES][256];
    char titles[EXPLORE_MAX_FILES][128];
    int n_files, selected;

    /* Action detection snapshots */
    int prev_paused, prev_show_vel, prev_show_B, prev_show_fl;
    int prev_follow, prev_cam_aligned, prev_model;
    double prev_pitch;
    Vector3 prev_cam_pos;
    int camera_moved;
} Explorer;

void explorer_init(Explorer *ex);
void explorer_free(Explorer *ex);
void explorer_scan_files(struct AppState *app);
int  explorer_load(struct AppState *app, const char *path);
void explorer_start(struct AppState *app);
void explorer_stop(struct AppState *app);
void explorer_step(struct AppState *app);
void explorer_snapshot(struct AppState *app);
int  explorer_check_action(struct AppState *app);
void explorer_declare_picker(struct AppState *app);
void explorer_declare_dialog(struct AppState *app);
void explorer_export_template(struct AppState *app);

#endif
