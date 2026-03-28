#ifndef APP_H
#define APP_H

#include <stdio.h>
#include "vec3.h"
#include "field.h"
#include "playback.h"
#include "boris.h"
#include "trail.h"
#include "diagnostics.h"
#include "recorder.h"
#include "tutorial.h"
#include "explorer.h"
#include "raylib.h"

typedef struct {
    Color bg;
    Color panel_bg;
    Color text;
    Color text_dim;
    Color trail;
    Color particle;
    Color field_line;
    Color grid;
    Color plot_bg;
    Color plot_line;
} Theme;

#define CAM_TRANS_RESET   0
#define CAM_TRANS_FOLLOW  1
#define CAM_TRANS_FIELD   2

typedef struct {
    int active;
    int mode;            /* CAM_TRANS_RESET / FOLLOW / FIELD */
    float elapsed;
    float duration;
    Vector3 start_pos, start_target, start_up;
    Vector3 dest_pos, dest_target, dest_up;  /* used for RESET (fixed dest) */
} CamTransition;

typedef struct {
    Vec3 pos;
    int species;
    double E_keV, pitch_deg;
    double custom_q, custom_m, custom_speed;
} ParticlePlacement;

typedef struct AppState {
    /* Field models */
    FieldModel models[FIELD_NUM_MODELS];
    int current_model;

    /* Particle template (UI settings for next placement) */
    int species;
    double E_keV;
    int energy_range;        /* 0=x1 (0-100keV), 1=x10 (0-1MeV), 2=x100 (0-10MeV) */
    double pitch_deg;
    Vec3 init_pos;
    double custom_q;     /* charge in C */
    double custom_m;     /* mass in kg */
    double custom_speed; /* speed in m/s */

    /* Multi-particle */
    #define MAX_PARTICLES 16
    int n_particles;
    int selected_particle;   /* index for camera follow, readouts, plots */
    Particle particles[MAX_PARTICLES];
    OrbitTrail trails[MAX_PARTICLES];
    DiagTimeSeries diags[MAX_PARTICLES];
    double E0_keVs[MAX_PARTICLES]; /* initial kinetic energy per particle */
    int step_interval[MAX_PARTICLES]; /* subcycling: step every Nth master substep */
    double particle_dt[MAX_PARTICLES]; /* per-particle timestep */

    /* Step history for pause-step-back (ring buffer of pos/vel snapshots) */
    #define STEP_HISTORY_MAX 4096
    Vec3 step_hist_pos[STEP_HISTORY_MAX][MAX_PARTICLES];
    Vec3 step_hist_vel[STEP_HISTORY_MAX][MAX_PARTICLES];
    double step_hist_time[STEP_HISTORY_MAX];
    int step_hist_head;   /* next write index */
    int step_hist_count;  /* number of valid entries */

    /* Undo/redo for particle placement */
    ParticlePlacement undo_stack[MAX_PARTICLES];
    int undo_count;     /* placements that can be undone */
    int redo_count;     /* placements that can be redone */

    /* Simulation */
    int paused;
    int needs_reset;
    double sim_time;
    double playback_speed;
    int speed_range;         /* 0=x1, 1=x10, 2=x100 */
    int steps_per_frame;
    double dt;               /* unified timestep (tightest constraint) */

    /* Diagnostics */
    int diag_sample_interval;
    int step_counter;

    /* 3D view */
    Camera3D camera;
    CamTransition cam_trans;
    int show_field_lines;
    int show_gc_field_line;  /* draw instantaneous field line through GC */
    int show_pos_field_line; /* draw field line through particle position */
    float trail_thickness;   /* OpenGL line width for orbit trails (1-5) */
    float axis_scale;        /* coordinate axis length, 0 = hidden */
    int radiation_loss;      /* apply Larmor radiation damping */
    float radiation_mult;    /* log10 multiplier on radiated power (0=x1, 3=x1000, etc.) */
    double rad_power;        /* last computed radiated power (W, physical, before mult) */
    int relativistic;        /* use relativistic Boris pusher */
    double gc_fl_length;     /* total arc length of GC field line (m) */
    int show_velocity_vec;
    int show_B_vec;
    int show_F_vec;      /* Lorentz force vector q(v×B) */
    int show_scale_bar;
    int vec_scaled;      /* 0 = unit vectors, 1 = scaled by magnitude */
    float vec_scale_v;   /* log10 scale for velocity arrow */
    float vec_scale_B;   /* log10 scale for B-field arrow */
    float vec_scale_F;   /* log10 scale for Lorentz force arrow */
    float arrow_head_size; /* arrowhead cone height (meters) */
    int lw_field_lines;    /* line width: model field lines (1-5) */
    int lw_gc_fl;          /* line width: GC field line (1-5) */
    int lw_pos_fl;         /* line width: position field line (1-5) */
    int lw_arrows;         /* line width: vector arrow shafts (1-5) */
    int lw_axes;           /* line width: coordinate axes (1-5) */
    int lw_triad;          /* line width: kappa/binormal arrows (1-5) */

    /* User-configurable colors (persisted per theme) */
    Color color_vel;         /* velocity vector arrow */
    Color color_B;           /* B-field vector arrow */
    Color color_F;           /* Lorentz force vector arrow */
    Color color_kappa;       /* kappa-hat arrow */
    Color color_binormal;    /* binormal arrow */
    Color color_field_lines; /* model field lines */
    Color color_gc_fl;       /* GC field line */
    Color color_pos_fl;      /* position field line */
    Color color_axes;        /* coordinate axes */
    #define NUM_SPECIES 6
    Color species_colors[NUM_SPECIES]; /* per-species trail/sphere */

    /* Per-theme color storage (the fields above are the "active" set;
     * on theme switch, active colors are saved here and the other set loaded) */
    #define NUM_USER_COLORS 9
    Color dark_colors[NUM_USER_COLORS];
    Color dark_species[NUM_SPECIES];
    Color light_colors[NUM_USER_COLORS];
    Color light_species[NUM_SPECIES];

    int show_trail;          /* orbit trail visible */
    float trail_fade;        /* trail alpha falloff: 0=no fade, 1=full fade to transparent */
    int plot_range;          /* 0=x1 (~30 gyro), 1=x10 (~300), 2=x100 (~3000) */
    int pitch_autoscale;     /* 0 = fixed 0-180, 1 = autoscale */
    int follow_particle;
    float cam_follow_dist;   /* distance from particle in follow mode */
    int cam_field_aligned;   /* 0=off, 1=look along +bhat, 2=look along -bhat */
    int kappa_screen_dir;    /* screen direction of kappa-hat: 0=up 1=right 2=down 3=left */
    Vec3 frame_e1;           /* kappa-hat (curvature direction) */
    Vec3 frame_e2;           /* binormal: bhat x e1 */
    int frame_e1_init;       /* has frame_e1 been initialized? */

    /* Fonts */
    Font font_ui;       /* Inter — labels, titles */
    Font font_mono;     /* Source Code Pro — numbers, readouts */
    char res_dir[1024]; /* resource directory path */

    /* Rendering */
    float dpi_scale;
    int bloom_enabled;
    int dark_mode;
    int stereo_3d;           /* side-by-side stereoscopic rendering */
    float stereo_separation; /* eye separation as fraction of cam distance */
    float stereo_gap;        /* image gap: fraction of half-width (0=touching, 1=max) */
    Theme theme;

    /* Video recording */
    Recorder recorder;
    int trigger_record;      /* trigger mode: reset starts/stops recording */
    FILE *event_log;         /* event log file for reproducible recordings */
    int event_frame;         /* frame counter within current recording */

    /* Previous-frame snapshot for change detection during recording */
    int prev_current_model;
    int prev_species;
    double prev_E_keV;
    double prev_pitch_deg;
    Vec3 prev_init_pos;
    double prev_custom_q, prev_custom_m, prev_custom_speed;
    double prev_playback_speed;
    int prev_speed_range;
    int prev_paused;
    int prev_dark_mode;
    int prev_show_field_lines, prev_show_gc_field_line;
    double prev_gc_fl_length;
    int prev_show_velocity_vec, prev_show_B_vec;
    int prev_plot_range, prev_pitch_autoscale;
    int prev_follow_particle, prev_cam_field_aligned;
    int prev_show_Gij, prev_ui_visible;
    float prev_axis_scale;
    int prev_radiation_loss;
    int prev_relativistic;
    double prev_model_params[FIELD_NUM_MODELS][FIELD_MAX_PARAMS];
    Vector3 prev_cam_pos, prev_cam_target;
    float prev_cam_fovy;

    /* Particle placement preview */
    int place_preview_valid;
    Vec3 place_preview_pos;

    /* G_ij tensor */
    double Gij[3][3];
    int show_Gij;
    int Gij_valid;

    /* Window */
    int win_w;
    int win_h;
    float ui_panel_width;
    int ui_visible;
    int ui_edge;              /* 0=left, 1=right */
    int mouse_over_panel;     /* set by ui_update each frame */
    float ui_zoom;            /* font scale for UI panel: 1.0 = default */
    int show_plots;           /* telemetry plots visible (independent of UI) */
    int plots_edge;           /* 0=bottom, 1=top */
    float plot_zoom;          /* font scale for telemetry plots */
    float plots_height_frac;  /* fraction of window for plots (default 0.225) */

    /* Collapsible UI sections: 0=Explore, 1=Model+Particle, 2=Playback+Record, 3=Display, 4=Readouts */
    int ui_section_open[5];

    /* Help overlay */
    int show_help;
    int help_tab;
    float help_zoom;     /* font scale for help: 1.0 = default, persisted */
    int factory_reset;   /* set by About tab button; handled in app_update */
    int show_settings;   /* settings overlay visible */

    /* Tutorial */
    Tutorial tutorial;
    int tutorial_seen;   /* persisted: 1 after first tutorial prompt shown */

    /* Language */
    int language;        /* LangId enum value, persisted as code string */
    int show_lang_picker; /* 1 = first-run language picker active */
    int needs_font_reload; /* deferred font reload (set in UI, applied next frame) */

    /* Event log playback */
    Playback playback;

    /* Scenario explorer */
    Explorer explorer;
} AppState;

extern const Theme THEME_DARK;
extern const Theme THEME_LIGHT;

void app_init(AppState *app);
void app_update(AppState *app);
void app_render(AppState *app);
void app_render_inner(AppState *app);
void app_reset_particle(AppState *app);
void app_add_particle(AppState *app, Vec3 pos);
void app_shutdown(AppState *app);
void app_apply_theme(AppState *app);
void app_switch_colors(AppState *app, int old_dark_mode);
void app_save_state(const AppState *app);
void app_load_state(AppState *app);
void app_reload_fonts(AppState *app);  /* reload fonts for current language */

/* Scene geometry helpers (account for ui_edge) */
void app_scene_rect(const AppState *app, float *x, float *y, float *w, float *h);
int  app_point_in_scene(const AppState *app, float px, float py);

#endif
