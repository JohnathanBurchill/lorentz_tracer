#ifndef PLAYBACK_H
#define PLAYBACK_H

#include "vec3.h"

struct AppState;

/* Single event from the log */
typedef struct {
    int frame;
    char key[32];
    double values[3];
    int n_values;
} PlaybackEvent;

/* Playback state */
typedef struct {
    int active;            /* currently playing back */
    int show_picker;       /* file picker dialog visible */

    /* Parsed event log */
    PlaybackEvent *events;
    int n_events;
    int capacity;

    /* Playback position */
    int current_frame;
    int event_cursor;

    /* File picker state */
    char dir[512];
    char files[256][256];
    int n_files;
    int selected;
} Playback;

void playback_init(Playback *pb);
void playback_free(Playback *pb);

/* Scan recording directory for .txt event logs */
void playback_scan_files(Playback *pb);

/* Parse an event log and apply initial state to app. Returns 0 on success. */
int playback_load(struct AppState *app, const char *path);

/* Start playback after loading */
void playback_start(struct AppState *app);

/* Stop playback */
void playback_stop(struct AppState *app);

/* Apply a single event to AppState (shared with explorer) */
void apply_event(struct AppState *app, const PlaybackEvent *ev);

/* Advance one frame: apply events at current_frame, then increment */
void playback_step(struct AppState *app);

/* Clay overlay for file picker */
void playback_declare_layout(struct AppState *app);

#endif
