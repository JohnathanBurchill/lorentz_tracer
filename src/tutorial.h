#ifndef TUTORIAL_H
#define TUTORIAL_H

#include "i18n.h"

struct AppState;

/* Action the user must perform to auto-advance */
typedef enum {
    TUT_ACT_NONE,           /* no action required; Continue only */
    TUT_ACT_PRESS_SPACE,    /* press spacebar (pause/unpause) */
    TUT_ACT_TOGGLE_VEL,     /* toggle velocity vector */
    TUT_ACT_TOGGLE_B,       /* toggle B vector */
    TUT_ACT_TOGGLE_FL,      /* toggle field lines */
    TUT_ACT_ORBIT_CAMERA,   /* right-drag or scroll detected */
    TUT_ACT_FOLLOW_ON,      /* enter follow mode */
    TUT_ACT_FIELD_ALIGNED,  /* enter field-aligned view */
    TUT_ACT_CHANGE_MODEL,   /* select a different field model */
    TUT_ACT_CHANGE_PITCH,   /* adjust pitch angle */
} TutActionType;

/* One step of the tutorial */
typedef struct {
    StrId text_brief_id;      /* short message (1-2 sentences) */
    StrId text_detail_id;     /* longer explanation (3-5 sentences) */
    TutActionType action;     /* what completes this step */
    int set_paused;           /* -1=don't touch, 0=unpause, 1=pause */
    int set_show_vel;         /* -1/0/1 */
    int set_show_B;
    int set_show_F;
    int set_show_fl;
    int set_ui_visible;
} TutStep;

/* Tutorial runtime state */
typedef struct {
    int active;
    int mode;               /* 0=brief, 1=detailed */
    int step;               /* -2=splash prompt, -1=mode choice, 0..N=steps */
    int n_steps;
    /* Snapshot for detecting user actions */
    int prev_paused;
    int prev_show_vel;
    int prev_show_B;
    int prev_show_fl;
    int prev_follow;
    int prev_cam_aligned;
    int prev_model;
    double prev_pitch;
    int camera_moved;
} Tutorial;

void tutorial_init(Tutorial *tut);
void tutorial_start(struct AppState *app, int mode);
void tutorial_stop(struct AppState *app);
void tutorial_advance(struct AppState *app);
int  tutorial_check_action(struct AppState *app);
void tutorial_apply_state(struct AppState *app);
void tutorial_snapshot(struct AppState *app);
void tutorial_declare_layout(struct AppState *app);

#endif
