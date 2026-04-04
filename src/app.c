#include "app.h"
#include "i18n.h"
#include "i18n_cjk_codepoints.h"
#include "render3d.h"
#include "render2d.h"
#include "ui.h"
#include "rlgl.h"
#include "raymath.h"
#include <math.h>
#include <string.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#ifdef _WIN32
#include <direct.h>  /* _mkdir */
#endif

const Theme THEME_DARK = {
    .bg         = {0, 0, 0, 255},
    .panel_bg   = {25, 25, 30, 240},
    .text       = {230, 230, 230, 255},
    .text_dim   = {150, 150, 160, 255},
    .trail      = {100, 200, 255, 255},
    .particle   = {255, 255, 100, 255},
    .field_line = {180, 120, 60, 120},
    .grid       = {60, 60, 70, 100},
    .plot_bg    = {20, 20, 25, 255},
    .plot_line  = {100, 200, 255, 255},
};

const Theme THEME_LIGHT = {
    .bg         = {250, 250, 255, 255},
    .panel_bg   = {220, 220, 225, 240},
    .text       = {30, 30, 30, 255},
    .text_dim   = {100, 100, 110, 255},
    .trail      = {30, 100, 200, 255},
    .particle   = {200, 50, 50, 255},
    .field_line = {160, 100, 40, 80},
    .grid       = {180, 180, 190, 100},
    .plot_bg    = {250, 250, 255, 255},
    .plot_line  = {30, 100, 200, 255},
};

/* --- Camera transition helpers --- */

static float smoothstep(float t)
{
    if (t <= 0.0f) return 0.0f;
    if (t >= 1.0f) return 1.0f;
    return t * t * (3.0f - 2.0f * t);
}

static Vector3 v3f_lerp(Vector3 a, Vector3 b, float t)
{
    return (Vector3){
        a.x + (b.x - a.x) * t,
        a.y + (b.y - a.y) * t,
        a.z + (b.z - a.z) * t,
    };
}

static Vector3 v3f_slerp(Vector3 a, Vector3 b, float t)
{
    float dot = a.x*b.x + a.y*b.y + a.z*b.z;
    if (dot < 0.0f) { /* take shorter arc */
        b.x = -b.x; b.y = -b.y; b.z = -b.z;
        dot = -dot;
    }
    if (dot > 0.9995f) { /* nearly parallel: lerp + normalize */
        Vector3 r = v3f_lerp(a, b, t);
        float len = sqrtf(r.x*r.x + r.y*r.y + r.z*r.z);
        if (len > 1e-8f) { r.x /= len; r.y /= len; r.z /= len; }
        return r;
    }
    float omega = acosf(dot);
    float so = sinf(omega);
    float sa = sinf((1.0f - t) * omega) / so;
    float sb = sinf(t * omega) / so;
    return (Vector3){sa*a.x + sb*b.x, sa*a.y + sb*b.y, sa*a.z + sb*b.z};
}

static Vector3 v3f_normalize(Vector3 v)
{
    float len = sqrtf(v.x*v.x + v.y*v.y + v.z*v.z);
    if (len > 1e-8f) return (Vector3){v.x/len, v.y/len, v.z/len};
    return v;
}

static void cam_trans_start(AppState *app, int mode, float duration)
{
    app->cam_trans.active = 1;
    app->cam_trans.mode = mode;
    app->cam_trans.elapsed = 0.0f;
    app->cam_trans.duration = duration;
    app->cam_trans.start_pos = app->camera.position;
    app->cam_trans.start_target = app->camera.target;
    app->cam_trans.start_up = v3f_normalize(app->camera.up);
}

/* Pack/unpack the 9 named color fields into/from an array */
static void colors_to_array(const AppState *app, Color out[NUM_USER_COLORS])
{
    out[0] = app->color_vel;
    out[1] = app->color_B;
    out[2] = app->color_F;
    out[3] = app->color_kappa;
    out[4] = app->color_binormal;
    out[5] = app->color_field_lines;
    out[6] = app->color_gc_fl;
    out[7] = app->color_pos_fl;
    out[8] = app->color_axes;
}

static void colors_from_array(AppState *app, const Color in[NUM_USER_COLORS])
{
    app->color_vel         = in[0];
    app->color_B           = in[1];
    app->color_F           = in[2];
    app->color_kappa       = in[3];
    app->color_binormal    = in[4];
    app->color_field_lines = in[5];
    app->color_gc_fl       = in[6];
    app->color_pos_fl      = in[7];
    app->color_axes        = in[8];
}

void app_apply_theme(AppState *app)
{
    app->theme = app->dark_mode ? THEME_DARK : THEME_LIGHT;
}

/* Save active colors into the storage for the given mode, load the other set */
void app_switch_colors(AppState *app, int old_dark_mode)
{
    /* Save current colors into the old theme's slot */
    if (old_dark_mode) {
        colors_to_array(app, app->dark_colors);
        memcpy(app->dark_species, app->species_colors, sizeof(app->dark_species));
    } else {
        colors_to_array(app, app->light_colors);
        memcpy(app->light_species, app->species_colors, sizeof(app->light_species));
    }

    /* Load colors for the new theme */
    if (app->dark_mode) {
        colors_from_array(app, app->dark_colors);
        memcpy(app->species_colors, app->dark_species, sizeof(app->species_colors));
    } else {
        colors_from_array(app, app->light_colors);
        memcpy(app->species_colors, app->light_species, sizeof(app->species_colors));
    }
}

void app_scene_rect(const AppState *app, float *x, float *y, float *w, float *h)
{
    float pw = app->ui_visible ? app->ui_panel_width : 0.0f;
    float plot_h = app->show_plots ? app->win_h * app->plots_height_frac : 0.0f;
    float sw = app->win_w - pw;
    float sh = app->win_h - plot_h;
    float sx = 0, sy = 0;
    if (app->ui_visible && app->ui_edge == 0) sx = pw; /* left panel */
    if (app->show_plots && app->plots_edge == 1) sy = plot_h; /* plots on top */
    *x = sx; *y = sy; *w = sw; *h = sh;
}

int app_point_in_scene(const AppState *app, float px, float py)
{
    float sx, sy, sw, sh;
    app_scene_rect(app, &sx, &sy, &sw, &sh);
    return (px >= sx && px < sx + sw && py >= sy && py < sy + sh);
}

void app_init(AppState *app)
{
    /* Window */
    app->win_w = 1280;
    app->win_h = 720;
    app->ui_panel_width = 320.0f;
    app->ui_visible = 1;
    app->ui_edge = 0; /* left */
    app->ui_zoom = 1.0f;
    app->show_plots = 1;
    app->plots_edge = 0; /* bottom */
    app->plot_zoom = 1.0f;
    app->plots_height_frac = 0.32f;

    /* Theme */
    app->dark_mode = 1;
    app_apply_theme(app);

    /* Fields */
    field_init_all(app->models);
    app->current_model = 0;

    /* Particle defaults */
    app->species = SPECIES_PROTON;
    app->E_keV = 1.0;
    app->energy_range = 0;
    app->pitch_deg = 45.0;
    app->custom_q = 1.0;
    app->custom_m = 1.0;
    app->custom_speed = 1.0;

    /* Simulation */
    app->paused = 0;
    app->needs_reset = 2; /* first init: use model default pos */
    app->sim_time = 0.0;
    app->playback_speed = 1.0;
    app->speed_range = 1; /* x1 */
    app->diag_sample_interval = 4;
    app->step_counter = 0;

    /* Multi-particle */
    app->n_particles = 1;
    app->selected_particle = 0;
    app->undo_count = 0;
    app->redo_count = 0;

    /* Trail and diagnostics */
    trail_init(&app->trails[0], TRAIL_MAX_POINTS);
    trail_init(&app->gc_trails[0], TRAIL_MAX_POINTS);
    diag_init(&app->diags[0], DIAG_MAX_SAMPLES);

    /* 3D view */
    app->show_field_lines = 1;
    app->show_gc_field_line = 0;
    app->show_pos_field_line = 0;
    app->gc_fl_length = 5.0;  /* default 5 m total arc length */
    app->trail_thickness = 1.0f;
    app->axis_scale = 1.0f;
    app->radiation_loss = 0;
    app->radiation_mult = 0.0f;  /* log10 scale: 0=x1, 6=x10^6, etc. */
    app->relativistic = 0;
    app->show_velocity_vec = 1;
    app->show_F_vec = 0;
    app->show_scale_bar = 1;
    app->vec_scaled = 0;
    app->vec_scale_v = 0.0f;
    app->vec_scale_B = 0.0f;
    app->vec_scale_F = 0.0f;
    app->arrow_head_size = 0.012f;
    app->lw_field_lines = 1;
    app->lw_gc_fl = 1;
    app->lw_pos_fl = 1;
    app->lw_arrows = 1;
    app->lw_axes = 1;
    app->lw_triad = 1;

    /* Dark-mode color defaults */
    app->dark_colors[0] = (Color){0, 255, 255, 200};    /* vel */
    app->dark_colors[1] = (Color){255, 0, 255, 200};    /* B */
    app->dark_colors[2] = (Color){255, 120, 0, 200};    /* F */
    app->dark_colors[3] = (Color){255, 160, 0, 200};    /* kappa */
    app->dark_colors[4] = (Color){0, 200, 80, 200};     /* binormal */
    app->dark_colors[5] = (Color){180, 120, 60, 120};   /* field lines */
    app->dark_colors[6] = (Color){180, 120, 60, 255};   /* gc fl */
    app->dark_colors[7] = (Color){180, 120, 60, 180};   /* pos fl */
    app->dark_colors[8] = (Color){100, 100, 110, 200};  /* axes */
    app->dark_species[0] = (Color){80, 160, 255, 255};  /* electron */
    app->dark_species[1] = (Color){220, 110, 170, 255}; /* positron */
    app->dark_species[2] = (Color){220, 220, 80, 255};  /* proton */
    app->dark_species[3] = (Color){80, 220, 100, 255};  /* alpha */
    app->dark_species[4] = (Color){220, 80, 100, 255};  /* O+ */
    app->dark_species[5] = (Color){160, 110, 220, 255}; /* custom */

    /* Light-mode color defaults (darker, more saturated for visibility on white) */
    app->light_colors[0] = (Color){0, 160, 180, 220};   /* vel */
    app->light_colors[1] = (Color){180, 0, 180, 220};   /* B */
    app->light_colors[2] = (Color){200, 80, 0, 220};    /* F */
    app->light_colors[3] = (Color){200, 120, 0, 220};   /* kappa */
    app->light_colors[4] = (Color){0, 150, 60, 220};    /* binormal */
    app->light_colors[5] = (Color){140, 90, 40, 100};   /* field lines */
    app->light_colors[6] = (Color){140, 90, 40, 255};   /* gc fl */
    app->light_colors[7] = (Color){140, 90, 40, 160};   /* pos fl */
    app->light_colors[8] = (Color){80, 80, 90, 200};    /* axes */
    app->light_species[0] = (Color){30, 100, 220, 255}; /* electron */
    app->light_species[1] = (Color){180, 60, 120, 255}; /* positron */
    app->light_species[2] = (Color){180, 160, 0, 255};  /* proton */
    app->light_species[3] = (Color){30, 160, 60, 255};  /* alpha */
    app->light_species[4] = (Color){200, 40, 60, 255};  /* O+ */
    app->light_species[5] = (Color){120, 70, 180, 255}; /* custom */

    /* Load active colors from the current theme (dark by default) */
    colors_from_array(app, app->dark_colors);
    memcpy(app->species_colors, app->dark_species, sizeof(app->species_colors));

    app->show_gc_trajectory = 0;
    app->gc_symplectic = 0;
    app->show_trail = 1;
    app->trail_fade = 0.7f;  /* default: moderate fade */
    app->show_B_vec = 1;
    app->plot_range = 0;
    app->pitch_autoscale = 0;
    app->follow_particle = 0;
    app->cam_follow_dist = 0.0f;
    app->cam_field_aligned = 0;
    app->kappa_screen_dir = 0;  /* kappa-hat up on screen */
    app->frame_e1 = (Vec3){1.0, 0.0, 0.0};
    app->frame_e1_init = 0;
    app->bloom_enabled = 0;
    app->stereo_3d = 0;
    app->stereo_separation = 0.03f;
    app->stereo_gap = 0.0f;
    app->show_Gij = 0;
    app->Gij_valid = 0;
    app->gij_zoom = 1.0f;

    /* UI sections: all open by default */
    for (int i = 0; i < 5; i++) app->ui_section_open[i] = 1;

    /* Help overlay */
    app->show_help = 0;
    app->help_tab = 0;
    app->help_zoom = 1.0f;
    app->show_settings = 0;

    /* Tutorial */
    tutorial_init(&app->tutorial);
    app->tutorial_seen = 0;

    /* Playback */
    playback_init(&app->playback);

    /* Explorer */
    explorer_init(&app->explorer);

    /* Recorder */
    app->recorder.active = 0;
    app->trigger_record = 0;

    /* DPI */
    app->dpi_scale = 1.0f;

    /* Camera (set properly in reset) */
    app->camera.projection = CAMERA_PERSPECTIVE;
    app->camera.fovy = 45.0f;
    app->camera.up = (Vector3){0.0f, 0.0f, 1.0f};
}

/* ---- Event log for reproducible recordings ---- */

static void event_log_snapshot(AppState *app)
{
    /* Capture current state so we can detect changes next frame */
    app->prev_current_model = app->current_model;
    app->prev_species = app->species;
    app->prev_E_keV = app->E_keV;
    app->prev_pitch_deg = app->pitch_deg;
    app->prev_init_pos = app->init_pos;
    app->prev_custom_q = app->custom_q;
    app->prev_custom_m = app->custom_m;
    app->prev_custom_speed = app->custom_speed;
    app->prev_playback_speed = app->playback_speed;
    app->prev_speed_range = app->speed_range;
    app->prev_paused = app->paused;
    app->prev_dark_mode = app->dark_mode;
    app->prev_show_field_lines = app->show_field_lines;
    app->prev_show_gc_field_line = app->show_gc_field_line;
    app->prev_gc_fl_length = app->gc_fl_length;
    app->prev_show_velocity_vec = app->show_velocity_vec;
    app->prev_show_B_vec = app->show_B_vec;
    app->prev_plot_range = app->plot_range;
    app->prev_pitch_autoscale = app->pitch_autoscale;
    app->prev_follow_particle = app->follow_particle;
    app->prev_cam_field_aligned = app->cam_field_aligned;
    app->prev_show_Gij = app->show_Gij;
    app->prev_ui_visible = app->ui_visible;
    app->prev_axis_scale = app->axis_scale;
    app->prev_radiation_loss = app->radiation_loss;
    app->prev_relativistic = app->relativistic;
    for (int m = 0; m < FIELD_NUM_MODELS; m++)
        for (int p = 0; p < app->models[m].n_params; p++)
            app->prev_model_params[m][p] = app->models[m].params[p];
    app->prev_cam_pos = app->camera.position;
    app->prev_cam_target = app->camera.target;
    app->prev_cam_fovy = app->camera.fovy;
}

static void event_log_write_state(AppState *app, const char *txt_path)
{
    app->event_log = fopen(txt_path, "w");
    if (!app->event_log) return;

    FILE *f = app->event_log;
    const FieldModel *fm = &app->models[app->current_model];
    const char *species_names[] = {"Electron", "Proton", "Alpha", "O+", "Custom"};

    fprintf(f, "# Lorentz Tracer event log\n");
    fprintf(f, "# File: %s\n", txt_path);
    fprintf(f, "# Resolution: %dx%d @ 60 fps\n\n",
            GetRenderWidth(), GetRenderHeight());

    fprintf(f, "# Initial state\n");
    fprintf(f, "model %d  # %s (%s)\n", app->current_model, fm->name, fm->code);
    for (int i = 0; i < fm->n_params; i++)
        fprintf(f, "param %d %.15g  # %s\n", i, fm->params[i], fm->param_names[i]);

    fprintf(f, "species %d  # %s\n", app->species, species_names[app->species]);
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
    fprintf(f, "dark_mode %d\n", app->dark_mode);
    fprintf(f, "show_field_lines %d\n", app->show_field_lines);
    fprintf(f, "show_gc_field_line %d\n", app->show_gc_field_line);
    fprintf(f, "show_gc_trajectory %d\n", app->show_gc_trajectory);
    fprintf(f, "gc_symplectic %d\n", app->gc_symplectic);
    fprintf(f, "gc_fl_length %.15g\n", app->gc_fl_length);
    fprintf(f, "show_velocity_vec %d\n", app->show_velocity_vec);
    fprintf(f, "show_B_vec %d\n", app->show_B_vec);
    fprintf(f, "show_Gij %d\n", app->show_Gij);
    fprintf(f, "plot_range %d\n", app->plot_range);
    fprintf(f, "pitch_autoscale %d\n", app->pitch_autoscale);
    fprintf(f, "follow_particle %d\n", app->follow_particle);
    fprintf(f, "cam_field_aligned %d\n", app->cam_field_aligned);
    fprintf(f, "kappa_screen_dir %d\n", app->kappa_screen_dir);
    fprintf(f, "ui_visible %d\n", app->ui_visible);
    fprintf(f, "axis_scale %.9g\n", app->axis_scale);
    fprintf(f, "radiation_loss %d\n", app->radiation_loss);
    fprintf(f, "relativistic %d\n", app->relativistic);
    fprintf(f, "camera_pos %.9g %.9g %.9g\n",
            app->camera.position.x, app->camera.position.y, app->camera.position.z);
    fprintf(f, "camera_target %.9g %.9g %.9g\n",
            app->camera.target.x, app->camera.target.y, app->camera.target.z);
    fprintf(f, "camera_fovy %.9g\n", app->camera.fovy);

    /* All model parameters (for reproducibility) */
    for (int m = 0; m < FIELD_NUM_MODELS; m++) {
        const FieldModel *fm2 = &app->models[m];
        for (int p = 0; p < fm2->n_params; p++)
            fprintf(f, "model_%d_param_%d %.15g\n", m, p, fm2->params[p]);
    }

    fprintf(f, "fps 60\n");
    fprintf(f, "\n# Events: frame real_time sim_time key value\n");
    app->event_frame = 0;
    event_log_snapshot(app);
    fflush(f);
}

static void event_log_check(AppState *app)
{
    if (!app->event_log) return;
    FILE *f = app->event_log;
    int fr = app->event_frame;
    double rt = fr / 60.0;  /* real time in seconds */
    double st = app->sim_time;

#define LOG_INT(name, field) \
    if (app->field != app->prev_##field) \
        fprintf(f, "%d %.6f %.9g %s %d\n", fr, rt, st, name, app->field);
#define LOG_DBL(name, field) \
    if (app->field != app->prev_##field) \
        fprintf(f, "%d %.6f %.9g %s %.15g\n", fr, rt, st, name, app->field);

    LOG_INT("model", current_model)
    LOG_INT("species", species)
    LOG_DBL("E_keV", E_keV)
    LOG_DBL("pitch_deg", pitch_deg)
    if (app->init_pos.x != app->prev_init_pos.x ||
        app->init_pos.y != app->prev_init_pos.y ||
        app->init_pos.z != app->prev_init_pos.z)
        fprintf(f, "%d %.6f %.9g init_pos %.15g %.15g %.15g\n",
                fr, rt, st, app->init_pos.x, app->init_pos.y, app->init_pos.z);
    LOG_DBL("custom_q", custom_q)
    LOG_DBL("custom_m", custom_m)
    LOG_DBL("custom_speed", custom_speed)
    LOG_DBL("playback_speed", playback_speed)
    LOG_INT("speed_range", speed_range)
    LOG_INT("paused", paused)
    LOG_INT("dark_mode", dark_mode)
    LOG_INT("show_field_lines", show_field_lines)
    LOG_INT("show_gc_field_line", show_gc_field_line)
    LOG_DBL("gc_fl_length", gc_fl_length)
    LOG_INT("show_velocity_vec", show_velocity_vec)
    LOG_INT("show_B_vec", show_B_vec)
    LOG_INT("plot_range", plot_range)
    LOG_INT("pitch_autoscale", pitch_autoscale)
    LOG_INT("follow_particle", follow_particle)
    LOG_INT("cam_field_aligned", cam_field_aligned)
    LOG_INT("show_Gij", show_Gij)
    LOG_INT("ui_visible", ui_visible)
    if (app->axis_scale != app->prev_axis_scale)
        fprintf(f, "%d %.6f %.9g axis_scale %.9g\n", fr, rt, st, app->axis_scale);
    LOG_INT("radiation_loss", radiation_loss)
    LOG_INT("relativistic", relativistic)

    /* Per-model parameter changes */
    for (int m = 0; m < FIELD_NUM_MODELS; m++)
        for (int p = 0; p < app->models[m].n_params; p++)
            if (app->models[m].params[p] != app->prev_model_params[m][p])
                fprintf(f, "%d %.6f %.9g model_%d_param_%d %.15g\n",
                        fr, rt, st, m, p, app->models[m].params[p]);

    /* Camera changes */
    if (app->camera.position.x != app->prev_cam_pos.x ||
        app->camera.position.y != app->prev_cam_pos.y ||
        app->camera.position.z != app->prev_cam_pos.z)
        fprintf(f, "%d %.6f %.9g camera_pos %.9g %.9g %.9g\n",
                fr, rt, st, app->camera.position.x,
                app->camera.position.y, app->camera.position.z);
    if (app->camera.target.x != app->prev_cam_target.x ||
        app->camera.target.y != app->prev_cam_target.y ||
        app->camera.target.z != app->prev_cam_target.z)
        fprintf(f, "%d %.6f %.9g camera_target %.9g %.9g %.9g\n",
                fr, rt, st, app->camera.target.x,
                app->camera.target.y, app->camera.target.z);
    if (app->camera.fovy != app->prev_cam_fovy)
        fprintf(f, "%d %.6f %.9g camera_fovy %.9g\n", fr, rt, st, app->camera.fovy);

    /* Log reset events */
    if (app->needs_reset)
        fprintf(f, "%d %.6f %.9g reset %d\n", fr, rt, st, app->needs_reset);

#undef LOG_INT
#undef LOG_DBL

    event_log_snapshot(app);
    app->event_frame++;
    fflush(f);
}

static void event_log_close(AppState *app)
{
    if (app->event_log) {
        fprintf(app->event_log, "%d %.6f %.9g end\n",
                app->event_frame, app->event_frame / 60.0, app->sim_time);
        fclose(app->event_log);
        app->event_log = NULL;
    }
}

/* Ensure output directory exists and return path prefix */
static void resolve_movie_dir(char *buf, int buflen)
{
#ifdef __EMSCRIPTEN__
    snprintf(buf, buflen, "/tmp");
    return;
#elif defined(_WIN32)
    const char *videos = getenv("USERPROFILE");
    if (videos) {
        snprintf(buf, buflen, "%s\\Videos\\Lorentz_Tracer", videos);
        _mkdir(buf);
        return;
    }
#else
    const char *home = getenv("HOME");
    if (home) {
#ifdef __APPLE__
        /* ~/Movies/Lorentz_Tracer */
        snprintf(buf, buflen, "%s/Movies/Lorentz_Tracer", home);
#else
        /* XDG_VIDEOS_DIR or ~/Videos/Lorentz_Tracer */
        const char *xdg = getenv("XDG_VIDEOS_DIR");
        if (xdg)
            snprintf(buf, buflen, "%s/Lorentz_Tracer", xdg);
        else
            snprintf(buf, buflen, "%s/Videos/Lorentz_Tracer", home);
#endif
        mkdir(buf, 0755);
        return;
    }
#endif
    snprintf(buf, buflen, ".");
}

static void start_recording(AppState *app)
{
    time_t now = time(NULL);
    struct tm *tm = localtime(&now);
    const FieldModel *fm = &app->models[app->current_model];

    char movie_dir[512];
    resolve_movie_dir(movie_dir, sizeof(movie_dir));

    /* Base filename (no extension) */
    char base[768];
    snprintf(base, sizeof(base),
             "%s/%04d-%02d-%02d_%02d%02d%02d_%s",
             movie_dir,
             tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday,
             tm->tm_hour, tm->tm_min, tm->tm_sec, fm->code);

    /* Always write the event log */
    char txt_name[800];
    snprintf(txt_name, sizeof(txt_name), "%s.txt", base);
    event_log_write_state(app, txt_name);

    /* Try mp4 recording if libav is available */
    if (recorder_available()) {
        char mp4_name[800];
        snprintf(mp4_name, sizeof(mp4_name), "%s.mp4", base);
        int w = GetRenderWidth();
        int h = GetRenderHeight();
        recorder_start(&app->recorder, w, h, 60, mp4_name);
    }
}

void app_reset_particle(AppState *app)
{
    app->n_particles = 1;
    app->selected_particle = 0;
    app->undo_count = 0;
    app->redo_count = 0;

    FieldModel *fm = &app->models[app->current_model];
    if (app->needs_reset == 2) {
        /* Model changed: use model default position */
        app->init_pos = fm->default_pos;
        app->has_last_drop = 0;
    } else if (app->has_last_drop) {
        /* Normal reset: restart from last manually-placed particle position */
        app->init_pos = app->last_drop_pos;
    }

    /* Verify B is nonzero at init_pos; if not, nudge toward model default
     * or try a small offset. Prevents NaN from starting in a zero-field region. */
    double Bcheck = field_Bmag(fm, app->init_pos);
    if (!(Bcheck > 1e-15)) {
        app->init_pos = fm->default_pos;
        Bcheck = field_Bmag(fm, app->init_pos);
    }
    if (!(Bcheck > 1e-15)) {
        /* Default pos also has B=0; try small offsets */
        Vec3 dp = fm->default_pos;
        double offsets[] = {0.01, 0.02, 0.05, 0.1};
        for (int i = 0; i < 4 && !(Bcheck > 1e-15); i++) {
            app->init_pos = (Vec3){dp.x + offsets[i], dp.y, dp.z};
            Bcheck = field_Bmag(fm, app->init_pos);
        }
    }

    boris_init_particle(&app->particles[0], app->species, app->E_keV,
                        app->pitch_deg, app->init_pos, fm,
                        app->relativistic);
    if (app->species == SPECIES_CUSTOM) {
        app->particles[0].q = app->custom_q;
        app->particles[0].m = app->custom_m;
        /* Reinit velocity with custom speed */
        double v = app->custom_speed;
        double cos_a = cos(app->pitch_deg * M_PI / 180.0);
        double sin_a = sin(app->pitch_deg * M_PI / 180.0);
        Vec3 B = fm->eval_B(fm->params, app->init_pos);
        Vec3 bhat = vec3_norm(B);
        Vec3 ref = (Vec3){0, 0, 1};
        if (fabs(vec3_dot(bhat, ref)) > 0.9) ref = (Vec3){1, 0, 0};
        Vec3 perp = vec3_sub(ref, vec3_scale(vec3_dot(ref, bhat), bhat));
        perp = vec3_norm(perp);
        app->particles[0].vel = vec3_add(vec3_scale(v * cos_a, bhat),
                                     vec3_scale(v * sin_a, perp));
    }

    /* Magnetic bottle: offset from axis by 1 Larmor radius on model change */
    if (app->needs_reset == 2 && app->current_model == 6) {
        double Bmag0 = field_Bmag(fm, app->init_pos);
        double v_tot = vec3_len(app->particles[0].vel);
        if (Bmag0 > 1e-30 && fabs(app->particles[0].q) > 1e-30 && v_tot > 1e-30) {
            double rL = app->particles[0].m * v_tot / (fabs(app->particles[0].q) * Bmag0);
            app->init_pos = (Vec3){rL, 0.0, 0.0};
            /* Reinit at offset position */
            boris_init_particle(&app->particles[0], app->species, app->E_keV,
                                app->pitch_deg, app->init_pos, fm,
                                app->relativistic);
            if (app->species == SPECIES_CUSTOM) {
                app->particles[0].q = app->custom_q;
                app->particles[0].m = app->custom_m;
                double spd = app->custom_speed;
                double ca = cos(app->pitch_deg * M_PI / 180.0);
                double sa = sin(app->pitch_deg * M_PI / 180.0);
                Vec3 B = fm->eval_B(fm->params, app->init_pos);
                Vec3 bh = vec3_norm(B);
                Vec3 rf = {0, 0, 1};
                if (fabs(vec3_dot(bh, rf)) > 0.9) rf = (Vec3){1, 0, 0};
                Vec3 pp = vec3_sub(rf, vec3_scale(vec3_dot(rf, bh), bh));
                pp = vec3_norm(pp);
                app->particles[0].vel = vec3_add(vec3_scale(spd * ca, bh),
                                             vec3_scale(spd * sa, pp));
            }
        }
    }

    /* Compute timestep: 256 steps per gyroperiod for smooth trails */
    double T_gyro = boris_gyro_period(&app->particles[0], fm, app->relativistic);
    app->dt = T_gyro / 256.0;
    app->particle_dt[0] = app->dt;
    app->step_interval[0] = 1;
    app->steps_per_frame = 256;

    /* Reset state */
    int reset_type = app->needs_reset;
    app->sim_time = 0.0;
    app->step_counter = 0;
    for (int i = 0; i < MAX_PARTICLES; i++) {
        trail_clear(&app->trails[i]);
        trail_init(&app->gc_trails[i], TRAIL_MAX_POINTS);
        trail_clear(&app->gc_trails[i]);
        diag_clear(&app->diags[i]);
    }
    gc_init_from_particle(&app->gc_particles[0], &app->particles[0],
                          &app->models[app->current_model], app->relativistic,
                          app->dt);
    app->needs_reset = 0;
    app->step_hist_head = 0;
    app->step_hist_count = 0;
    app->E0_keVs[0] = boris_energy_keV(&app->particles[0], app->relativistic);

    /* Only reset camera and follow distance on model change (reset_type == 2) */
    if (reset_type == 2) {
        app->cam_trans.active = 0;
        double Bmag = field_Bmag(fm, app->init_pos);
        double v = vec3_len(app->particles[0].vel);
        double rL = (Bmag > 1e-30 && fabs(app->particles[0].q) > 1e-30)
            ? app->particles[0].m * v / (fabs(app->particles[0].q) * Bmag) : 1.0;
        app->cam_follow_dist = (float)(rL * 20.0);
        if (app->cam_follow_dist < 0.5f) app->cam_follow_dist = 0.5f;
        if (app->cam_follow_dist > fm->default_camera_dist * 2.0f)
            app->cam_follow_dist = (float)(fm->default_camera_dist * 2.0f);
        float d = (float)fm->default_camera_dist;
        app->camera.target = (Vector3){0.0f, 0.0f, 0.0f};
        app->camera.position = (Vector3){d * 0.7f, -d * 0.7f, d * 0.5f};
        app->camera.up = (Vector3){0.0f, 0.0f, 1.0f};
    }
}

/* Build a picking ray from screen coords, handling both perspective
 * and orthographic projections. */
static Ray build_pick_ray(const AppState *a, Vector2 ms,
                           float sx, float sy, float sw, float sh)
{
    float ndc_x = (ms.x - sx) / sw * 2.0f - 1.0f;
    float ndc_y = 1.0f - (ms.y - sy) / sh * 2.0f;
    float aspect = sw / sh;
    float half_v = tanf(a->camera.fovy * 0.5f * DEG2RAD);
    float half_h = half_v * aspect;
    Matrix view = MatrixLookAt(a->camera.position, a->camera.target, a->camera.up);
    Matrix inv_view = MatrixInvert(view);

    if (a->camera.projection == CAMERA_ORTHOGRAPHIC) {
        /* Ortho: ray origin shifts, direction is constant (camera forward) */
        Vector3 dv = { a->camera.target.x - a->camera.position.x,
                       a->camera.target.y - a->camera.position.y,
                       a->camera.target.z - a->camera.position.z };
        float dlen = sqrtf(dv.x*dv.x + dv.y*dv.y + dv.z*dv.z);
        if (dlen > 1e-8f) { dv.x /= dlen; dv.y /= dlen; dv.z /= dlen; }
        float oh = dlen * half_v;
        float ow = oh * aspect;
        /* Offset in view space, then transform to world */
        Vector3 off_view = { ndc_x * ow, ndc_y * oh, 0.0f };
        Vector3 wo = Vector3Transform(off_view, inv_view);
        wo = (Vector3){ wo.x - inv_view.m12, wo.y - inv_view.m13, wo.z - inv_view.m14 };
        Vector3 ro = { a->camera.position.x + wo.x,
                       a->camera.position.y + wo.y,
                       a->camera.position.z + wo.z };
        return (Ray){ .position = ro, .direction = dv };
    }
    /* Perspective */
    Vector3 rd_view = { ndc_x * half_h, ndc_y * half_v, -1.0f };
    Vector3 rd = Vector3Transform(rd_view, inv_view);
    rd = (Vector3){ rd.x - inv_view.m12, rd.y - inv_view.m13, rd.z - inv_view.m14 };
    float len = sqrtf(rd.x*rd.x + rd.y*rd.y + rd.z*rd.z);
    if (len > 1e-8f) { rd.x /= len; rd.y /= len; rd.z /= len; }
    return (Ray){ .position = a->camera.position, .direction = rd };
}

static void recompute_dt(AppState *app);
#ifndef __EMSCRIPTEN__
static void state_file_path(char *buf, int buflen);
#endif

void app_add_particle(AppState *app, Vec3 pos)
{
    if (app->n_particles >= MAX_PARTICLES) return;
    int idx = app->n_particles++;
    FieldModel *fm = &app->models[app->current_model];

    boris_init_particle(&app->particles[idx], app->species, app->E_keV,
                        app->pitch_deg, pos, fm, app->relativistic);
    if (app->species == SPECIES_CUSTOM) {
        app->particles[idx].q = app->custom_q;
        app->particles[idx].m = app->custom_m;
        /* Reinit velocity with custom speed */
        double v = app->custom_speed;
        double cos_a = cos(app->pitch_deg * M_PI / 180.0);
        double sin_a = sin(app->pitch_deg * M_PI / 180.0);
        Vec3 B = fm->eval_B(fm->params, pos);
        Vec3 bhat = vec3_norm(B);
        Vec3 ref = (Vec3){0, 0, 1};
        if (fabs(vec3_dot(bhat, ref)) > 0.9) ref = (Vec3){1, 0, 0};
        Vec3 perp = vec3_sub(ref, vec3_scale(vec3_dot(ref, bhat), bhat));
        perp = vec3_norm(perp);
        app->particles[idx].vel = vec3_add(vec3_scale(v * cos_a, bhat),
                                           vec3_scale(v * sin_a, perp));
    }

    trail_init(&app->trails[idx], TRAIL_MAX_POINTS);
    trail_clear(&app->trails[idx]);
    diag_init(&app->diags[idx], DIAG_MAX_SAMPLES);
    diag_clear(&app->diags[idx]);
    gc_init_from_particle(&app->gc_particles[idx], &app->particles[idx], fm,
                          app->relativistic, app->dt);
    trail_init(&app->gc_trails[idx], TRAIL_MAX_POINTS);
    trail_clear(&app->gc_trails[idx]);
    app->E0_keVs[idx] = boris_energy_keV(&app->particles[idx], app->relativistic);

    /* Record for undo */
    app->undo_stack[app->undo_count++] = (ParticlePlacement){
        .pos = pos, .species = app->species,
        .E_keV = app->E_keV, .pitch_deg = app->pitch_deg,
        .custom_q = app->custom_q, .custom_m = app->custom_m,
        .custom_speed = app->custom_speed
    };
    app->redo_count = 0;  /* new placement clears redo history */

    /* Remember for reset */
    app->last_drop_pos = pos;
    app->has_last_drop = 1;

    app->selected_particle = idx;

    recompute_dt(app);
}

static void recompute_dt(AppState *app)
{
    FieldModel *fm = &app->models[app->current_model];
    double T_sel = boris_gyro_period(&app->particles[app->selected_particle],
                                      fm, app->relativistic);
    /* Start with dt sized for 256 steps per selected particle's orbit */
    double dt = T_sel / 256.0;

    /* Cap so every particle gets at least 64 steps per orbit (visually smooth,
     * Boris is stable). This bounds steps_per_frame to at most 4× base (1024),
     * keeping cost linear in N instead of blowing up with field-strength ratios. */
    for (int i = 0; i < app->n_particles; i++) {
        double T = boris_gyro_period(&app->particles[i], fm, app->relativistic);
        double max_dt = T / 64.0;
        if (dt > max_dt) dt = max_dt;
    }
    app->dt = dt;
    app->steps_per_frame = (int)(T_sel / dt);
    if (app->steps_per_frame < 1) app->steps_per_frame = 1;
}

static void app_undo_particle(AppState *app)
{
    if (app->undo_count <= 0 || app->n_particles <= 1) return;
    app->n_particles--;
    app->undo_count--;
    app->redo_count++;
    if (app->selected_particle >= app->n_particles)
        app->selected_particle = app->n_particles - 1;
    recompute_dt(app);
}

static void app_redo_particle(AppState *app)
{
    if (app->redo_count <= 0 || app->n_particles >= MAX_PARTICLES) return;
    int ri = app->undo_count;  /* redo entry is at undo_count (just past current top) */
    ParticlePlacement *pp = &app->undo_stack[ri];
    /* Temporarily set template to the saved placement */
    int saved_species = app->species;
    double saved_E = app->E_keV, saved_pitch = app->pitch_deg;
    double saved_q = app->custom_q, saved_m = app->custom_m, saved_spd = app->custom_speed;
    app->species = pp->species;
    app->E_keV = pp->E_keV;
    app->pitch_deg = pp->pitch_deg;
    app->custom_q = pp->custom_q;
    app->custom_m = pp->custom_m;
    app->custom_speed = pp->custom_speed;
    /* undo_count and redo_count will be updated by app_add_particle, but we need
     * to adjust: add_particle increments undo_count and clears redo_count.
     * We want undo_count to go up by 1 and redo_count to go down by 1. */
    int saved_redo = app->redo_count - 1;
    app_add_particle(app, pp->pos);
    app->redo_count = saved_redo;
    /* Restore template */
    app->species = saved_species;
    app->E_keV = saved_E;
    app->pitch_deg = saved_pitch;
    app->custom_q = saved_q;
    app->custom_m = saved_m;
    app->custom_speed = saved_spd;
}

void app_update(AppState *app)
{
    /* Factory reset: delete state file, reinit to defaults */
    if (app->factory_reset) {
        app->factory_reset = 0;
#ifndef __EMSCRIPTEN__
        char path[600];
        state_file_path(path, sizeof(path));
        remove(path);
#endif
        /* Preserve resources that survive reset */
        Font font_ui = app->font_ui;
        Font font_mono = app->font_mono;
        float dpi = app->dpi_scale;
        int win_w = app->win_w, win_h = app->win_h;
        app_init(app);
        app->font_ui = font_ui;
        app->font_mono = font_mono;
        app->dpi_scale = dpi;
        app->win_w = win_w;
        app->win_h = win_h;
        app->show_help = 0;
        app_apply_theme(app);
    }

    if (app->needs_reset)
        app_reset_particle(app);

    /* System shortcuts: always active (Cmd+F fullscreen, Cmd+Q handled by raylib) */
#ifdef __APPLE__
    if (IsKeyPressed(KEY_F) && IsKeyDown(KEY_LEFT_SUPER))
#else
    if (IsKeyPressed(KEY_F) && IsKeyDown(KEY_LEFT_CONTROL))
#endif
    {
        ToggleBorderlessWindowed();
#ifdef __APPLE__
        extern void macos_fix_window_switching(void *nswindow);
        macos_fix_window_switching(GetWindowHandle());
#endif
    }

    /* Help overlay captures all input when visible */
    if (app->show_help) {
        if (IsKeyPressed(KEY_F1) || IsKeyPressed(KEY_ESCAPE))
            app->show_help = 0;
        if (IsKeyPressed(KEY_LEFT) ||
            (IsKeyPressed(KEY_TAB) && (IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT))))
            app->help_tab = (app->help_tab + 4) % 5;
        if (IsKeyPressed(KEY_RIGHT) ||
            (IsKeyPressed(KEY_TAB) && !IsKeyDown(KEY_LEFT_SHIFT) && !IsKeyDown(KEY_RIGHT_SHIFT)))
            app->help_tab = (app->help_tab + 1) % 5;
        /* Cmd+/- (macOS) or Ctrl+/- zoom help text */
#ifdef __APPLE__
        int zmod = IsKeyDown(KEY_LEFT_SUPER) || IsKeyDown(KEY_RIGHT_SUPER);
#else
        int zmod = IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_RIGHT_CONTROL);
#endif
        if (zmod && (IsKeyPressed(KEY_EQUAL) || IsKeyPressed(KEY_KP_ADD)))
            app->help_zoom = fminf(app->help_zoom + 0.1f, 2.5f);
        if (zmod && (IsKeyPressed(KEY_MINUS) || IsKeyPressed(KEY_KP_SUBTRACT)))
            app->help_zoom = fmaxf(app->help_zoom - 0.1f, 0.5f);
        if (zmod && IsKeyPressed(KEY_ZERO))
            app->help_zoom = 1.0f;
        return;
    }

    /* File picker captures input when visible */
    if (app->playback.show_picker) {
        if (IsKeyPressed(KEY_ESCAPE))
            app->playback.show_picker = 0;
        if (IsKeyPressed(KEY_ENTER) && app->playback.selected >= 0) {
            char path[768];
            snprintf(path, sizeof(path), "%s/%s",
                     app->playback.dir, app->playback.files[app->playback.selected]);
            if (playback_load(app, path) == 0)
                playback_start(app);
        }
        if (IsKeyPressed(KEY_UP) && app->playback.selected > 0)
            app->playback.selected--;
        if (IsKeyPressed(KEY_DOWN) && app->playback.selected < app->playback.n_files - 1)
            app->playback.selected++;
        return;
    }

    /* Tutorial input capture */
    if (app->tutorial.active) {
        if (app->tutorial.step < 0) return; /* choice screens: buttons only */
        if (IsKeyPressed(KEY_ESCAPE)) { tutorial_stop(app); return; }
        /* Allow through: Space, F, V, U, 0, Tab */
        if (IsKeyPressed(KEY_SPACE)) app->paused = !app->paused;
        if (IsKeyPressed(KEY_F) && !IsKeyDown(KEY_LEFT_SUPER) && !IsKeyDown(KEY_LEFT_CONTROL))
            app->follow_particle = !app->follow_particle;
        if (IsKeyPressed(KEY_V)) {
            if (IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT))
                app->cam_field_aligned = (app->cam_field_aligned + 2) % 3;
            else
                app->cam_field_aligned = (app->cam_field_aligned + 1) % 3;
            if (app->cam_field_aligned) { app->follow_particle = 1; app->frame_e1_init = 0; }
        }
        if (IsKeyPressed(KEY_U)) app->ui_visible = !app->ui_visible;
        /* R key: reset particle */
        if (IsKeyPressed(KEY_R)) app->needs_reset = 1;
        /* 0 key: reset camera view */
        if (IsKeyPressed(KEY_ZERO)) {
            FieldModel *fm0 = &app->models[app->current_model];
            float d = (float)fm0->default_camera_dist;
            cam_trans_start(app, CAM_TRANS_RESET, 0.4f);
            app->cam_trans.dest_target = (Vector3){0,0,0};
            app->cam_trans.dest_pos = (Vector3){d*0.7f, -d*0.7f, d*0.5f};
            app->cam_trans.dest_up = (Vector3){0,0,1};
            app->cam_follow_dist = d;
            app->follow_particle = 0;
            app->cam_field_aligned = 0;
            app->frame_e1_init = 0;
        }
        /* Tab: cycle selected particle */
        if (IsKeyPressed(KEY_TAB) && app->n_particles > 1)
            app->selected_particle = (app->selected_particle + 1) % app->n_particles;
        /* Camera orbit (right-drag) */
        if (IsMouseButtonDown(MOUSE_BUTTON_RIGHT)) {
            Vector2 delta = GetMouseDelta();
            if (delta.x * delta.x + delta.y * delta.y > 1.0f)
                app->tutorial.camera_moved = 1;
            app->cam_trans.active = 0;
            UpdateCameraPro(&app->camera,
                (Vector3){0,0,0}, (Vector3){delta.x*0.3f, delta.y*0.3f, 0}, 0);
        }
        /* Scroll zoom */
        float tw = GetMouseWheelMove();
        if (tw != 0.0f && !app->mouse_over_panel) {
            app->tutorial.camera_moved = 1;
            app->cam_trans.active = 0;
            float factor = 1.0f - tw * 0.1f;
            if (factor < 0.5f) factor = 0.5f;
            if (factor > 2.0f) factor = 2.0f;
            if (app->follow_particle) {
                app->cam_follow_dist *= factor;
                if (app->cam_follow_dist < 0.01f) app->cam_follow_dist = 0.01f;
            } else {
                Vector3 d = {
                    app->camera.position.x - app->camera.target.x,
                    app->camera.position.y - app->camera.target.y,
                    app->camera.position.z - app->camera.target.z,
                };
                app->camera.position.x = app->camera.target.x + d.x * factor;
                app->camera.position.y = app->camera.target.y + d.y * factor;
                app->camera.position.z = app->camera.target.z + d.z * factor;
            }
        }
        tutorial_snapshot(app);
        goto placement;
    }

    /* During explorer scenario: Esc stops it */
    if (app->explorer.active) {
        if (IsKeyPressed(KEY_ESCAPE)) {
            explorer_stop(app);
        } else {
            explorer_step(app);
        }
        /* Fall through to physics below */
    }

    /* During playback: only Esc stops it */
    if (app->playback.active) {
        if (IsKeyPressed(KEY_ESCAPE)) {
            playback_stop(app);
        } else {
            playback_step(app);
        }
        /* Fall through to physics below (playback applies events, physics runs normally) */
    }

    /* Keyboard */
    if (!app->playback.active) {
        if (IsKeyPressed(KEY_F1)) app->show_help = 1;
        if (IsKeyPressed(KEY_P) && !app->recorder.active &&
            (IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT))) {
            playback_scan_files(&app->playback);
            app->playback.show_picker = 1;
        }
    }
    if (IsKeyPressed(KEY_SPACE) && !app->playback.active) app->paused = !app->paused;

    /* Single-step when paused: n = forward, p = back.
     * Tap: 1 step. Hold: advance at current playback speed (steps_per_frame * speed). */
    if (app->paused && !app->playback.active) {
        int n_down = IsKeyDown(KEY_N);
        int p_down = IsKeyDown(KEY_P);
        int n_press = IsKeyPressed(KEY_N);
        int p_press = IsKeyPressed(KEY_P);
        int steps = 0;
        if (n_press || p_press) steps = 1;
        else if (n_down || p_down) {
            steps = (int)(app->steps_per_frame * app->playback_speed);
            if (steps < 1) steps = 1;
        }
        FieldModel *fm = &app->models[app->current_model];
        for (int s = 0; s < steps && n_down; s++) {
            int hi = app->step_hist_head;
            for (int pi = 0; pi < app->n_particles; pi++) {
                app->step_hist_pos[hi][pi] = app->particles[pi].pos;
                app->step_hist_vel[hi][pi] = app->particles[pi].vel;
                app->step_hist_gc_pos[hi][pi] = app->gc_particles[pi].pos;
                app->step_hist_gc_ppar[hi][pi] = app->gc_particles[pi].p_par;
            }
            app->step_hist_time[hi] = app->sim_time;
            app->step_hist_head = (hi + 1) % STEP_HISTORY_MAX;
            if (app->step_hist_count < STEP_HISTORY_MAX)
                app->step_hist_count++;
            boris_step_batch(app->particles, app->n_particles, fm,
                             app->dt, app->relativistic);
            gc_step_batch(app->gc_particles, app->n_particles, fm,
                          app->dt, app->relativistic, app->gc_symplectic);
            for (int pi = 0; pi < app->n_particles; pi++) {
                trail_push(&app->trails[pi], app->particles[pi].pos);
                trail_push(&app->gc_trails[pi], app->gc_particles[pi].pos);
            }
            app->sim_time += app->dt;
            app->step_counter++;
        }
        for (int s = 0; s < steps && p_down && app->step_hist_count > 0; s++) {
            app->step_hist_head = (app->step_hist_head - 1 + STEP_HISTORY_MAX) % STEP_HISTORY_MAX;
            app->step_hist_count--;
            int hi = app->step_hist_head;
            for (int pi = 0; pi < app->n_particles; pi++) {
                app->particles[pi].pos = app->step_hist_pos[hi][pi];
                app->particles[pi].vel = app->step_hist_vel[hi][pi];
                app->gc_particles[pi].pos = app->step_hist_gc_pos[hi][pi];
                app->gc_particles[pi].p_par = app->step_hist_gc_ppar[hi][pi];
                trail_pop(&app->trails[pi]);
                trail_pop(&app->gc_trails[pi]);
            }
            app->sim_time = app->step_hist_time[hi];
            if (app->step_counter > 0) app->step_counter--;
        }
    }

    if (app->playback.active) goto physics;  /* skip interactive input during playback */
    if (IsKeyPressed(KEY_TAB) && app->n_particles > 1) {
        int n = app->n_particles;
        if (IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT))
            app->selected_particle = (app->selected_particle + n - 1) % n;
        else
            app->selected_particle = (app->selected_particle + 1) % n;
        if (app->follow_particle)
            cam_trans_start(app, CAM_TRANS_FOLLOW, 0.35f);
    }

    /* Undo/redo particle placement: Cmd-Z / Cmd-Shift-Z (macOS), Ctrl-Z / Ctrl-Shift-Z */
    if (IsKeyPressed(KEY_Z)) {
#ifdef __APPLE__
        int mod = IsKeyDown(KEY_LEFT_SUPER) || IsKeyDown(KEY_RIGHT_SUPER);
#else
        int mod = IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_RIGHT_CONTROL);
#endif
        if (mod) {
            if (IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT))
                app_redo_particle(app);
            else
                app_undo_particle(app);
        }
    }

    if (IsKeyPressed(KEY_R)) {
        if (app->trigger_record) {
            if (app->recorder.active || app->event_log) {
                event_log_close(app);
                if (app->recorder.active) recorder_stop(&app->recorder);
            } else
                start_recording(app);
        }
        app->needs_reset = 1;
        app->drop_mode = 0;
    }
    /* Cmd+/- zoom: route to plot zoom if mouse over plots, else UI zoom.
     * Bare +/- control playback speed. */
    {
#ifdef __APPLE__
        int zmod = IsKeyDown(KEY_LEFT_SUPER) || IsKeyDown(KEY_RIGHT_SUPER);
#else
        int zmod = IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_RIGHT_CONTROL);
#endif
        int zplus = IsKeyPressed(KEY_EQUAL) || IsKeyPressed(KEY_KP_ADD);
        int zminus = IsKeyPressed(KEY_MINUS) || IsKeyPressed(KEY_KP_SUBTRACT);
        int zzero = IsKeyPressed(KEY_ZERO);
        if (zmod && (zplus || zminus || zzero)) {
            Vector2 m = GetMousePosition();
            /* Check if mouse is over Gij overlay */
            int over_gij = 0;
            if (app->show_Gij && app->Gij_valid && app->gij_rect[2] > 0) {
                over_gij = (m.x >= app->gij_rect[0] &&
                            m.x < app->gij_rect[0] + app->gij_rect[2] &&
                            m.y >= app->gij_rect[1] &&
                            m.y < app->gij_rect[1] + app->gij_rect[3]);
            }
            /* Check if mouse is over plots area */
            int over_plots = 0;
            if (!over_gij && app->show_plots) {
                float sx, sy, sw, sh;
                app_scene_rect(app, &sx, &sy, &sw, &sh);
                float ph = app->win_h * app->plots_height_frac;
                float py = (app->plots_edge == 1) ? 0.0f : sy + sh;
                over_plots = (m.x >= sx && m.x < sx + sw && m.y >= py && m.y < py + ph);
            }
            float *z = over_gij ? &app->gij_zoom :
                        over_plots ? &app->plot_zoom : &app->ui_zoom;
            if (zplus) *z = fminf(*z + 0.1f, 2.5f);
            else if (zminus) *z = fmaxf(*z - 0.1f, 0.5f);
            else if (zzero) *z = 1.0f;
        }
        else if (zplus && !app->drop_mode) {
            float max_speed[] = {0.1f, 1.0f, 10.0f, 100.0f};
            app->playback_speed *= 2.0;
            if (app->playback_speed > max_speed[app->speed_range])
                app->playback_speed = max_speed[app->speed_range];
        }
        else if (zminus && !app->drop_mode) {
            app->playback_speed *= 0.5;
            if (app->playback_speed < 0.01) app->playback_speed = 0.01;
        }
    }

    /* Toggle UI visibility */
    if (IsKeyPressed(KEY_U)) app->ui_visible = !app->ui_visible;

    /* Toggle field-aligned camera: V cycles off → +bhat → -bhat → off
     * Shift+V cycles in reverse: off → -bhat → +bhat → off */
    if (IsKeyPressed(KEY_V)) {
        if (IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT))
            app->cam_field_aligned = (app->cam_field_aligned + 2) % 3;
        else
            app->cam_field_aligned = (app->cam_field_aligned + 1) % 3;
        if (app->cam_field_aligned) app->follow_particle = 1;
        app->frame_e1_init = 0;
        cam_trans_start(app, CAM_TRANS_FIELD, 0.35f);
    }

    /* Arrow keys: set kappa-hat screen direction in field-aligned view */
    if (app->cam_field_aligned) {
        int new_dir = -1;
        if (IsKeyPressed(KEY_UP))    new_dir = 0;
        if (IsKeyPressed(KEY_RIGHT)) new_dir = 1;
        if (IsKeyPressed(KEY_DOWN))  new_dir = 2;
        if (IsKeyPressed(KEY_LEFT))  new_dir = 3;
        if (new_dir >= 0 && new_dir != app->kappa_screen_dir) {
            app->kappa_screen_dir = new_dir;
            cam_trans_start(app, CAM_TRANS_FIELD, 0.25f);
        }
    }

    /* Right-drag: orbit camera view direction (around target) */
    if (IsMouseButtonDown(MOUSE_BUTTON_RIGHT) && !IsKeyDown(KEY_LEFT_SHIFT) && !IsKeyDown(KEY_RIGHT_SHIFT)) {
        app->cam_trans.active = 0;
        Vector2 delta = GetMouseDelta();
        UpdateCameraPro(&app->camera,
            (Vector3){0.0f, 0.0f, 0.0f},
            (Vector3){delta.x * 0.3f, delta.y * 0.3f, 0.0f},
            0.0f);
    }
    /* Shift + right-drag: orbit camera position and target around the origin */
    if (IsMouseButtonDown(MOUSE_BUTTON_RIGHT) && (IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT))) {
        app->cam_trans.active = 0;
        Vector2 delta = GetMouseDelta();
        float angle_h = -delta.x * 0.005f;
        float angle_v = -delta.y * 0.005f;

        Vector3 p = app->camera.position;
        Vector3 t = app->camera.target;

        /* Horizontal rotation about z-axis */
        float ch = cosf(angle_h), sh = sinf(angle_h);
        float px = p.x * ch - p.y * sh;
        float py = p.x * sh + p.y * ch;
        p.x = px; p.y = py;
        float tx = t.x * ch - t.y * sh;
        float ty = t.x * sh + t.y * ch;
        t.x = tx; t.y = ty;

        /* Vertical rotation about the camera's right vector */
        Vector3 fwd = {-p.x, -p.y, -p.z};
        float fwd_len = sqrtf(fwd.x*fwd.x + fwd.y*fwd.y + fwd.z*fwd.z);
        if (fwd_len > 1e-6f) {
            fwd.x /= fwd_len; fwd.y /= fwd_len; fwd.z /= fwd_len;
        }
        Vector3 up = app->camera.up;
        Vector3 right = {
            fwd.y*up.z - fwd.z*up.y,
            fwd.z*up.x - fwd.x*up.z,
            fwd.x*up.y - fwd.y*up.x,
        };
        float cv = cosf(angle_v), sv = sinf(angle_v);

        /* Rotate position */
        float dot_p = right.x*p.x + right.y*p.y + right.z*p.z;
        Vector3 cross_p = {
            right.y*p.z - right.z*p.y,
            right.z*p.x - right.x*p.z,
            right.x*p.y - right.y*p.x,
        };
        p.x = p.x*cv + cross_p.x*sv + right.x*dot_p*(1-cv);
        p.y = p.y*cv + cross_p.y*sv + right.y*dot_p*(1-cv);
        p.z = p.z*cv + cross_p.z*sv + right.z*dot_p*(1-cv);

        /* Rotate target with the same rotation */
        float dot_t = right.x*t.x + right.y*t.y + right.z*t.z;
        Vector3 cross_t = {
            right.y*t.z - right.z*t.y,
            right.z*t.x - right.x*t.z,
            right.x*t.y - right.y*t.x,
        };
        t.x = t.x*cv + cross_t.x*sv + right.x*dot_t*(1-cv);
        t.y = t.y*cv + cross_t.y*sv + right.y*dot_t*(1-cv);
        t.z = t.z*cv + cross_t.z*sv + right.z*dot_t*(1-cv);

        app->camera.position = p;
        app->camera.target = t;
    }
    /* Right-shift + left-drag: pan camera in the view plane */
    if (IsMouseButtonDown(MOUSE_BUTTON_LEFT) && IsKeyDown(KEY_RIGHT_SHIFT)) {
        app->cam_trans.active = 0;
        Vector2 delta = GetMouseDelta();
        Vector3 fwd = {
            app->camera.target.x - app->camera.position.x,
            app->camera.target.y - app->camera.position.y,
            app->camera.target.z - app->camera.position.z,
        };
        float fwd_len = sqrtf(fwd.x*fwd.x + fwd.y*fwd.y + fwd.z*fwd.z);
        if (fwd_len > 1e-6f) {
            fwd.x /= fwd_len; fwd.y /= fwd_len; fwd.z /= fwd_len;
        }
        Vector3 up = app->camera.up;
        Vector3 right = {
            fwd.y*up.z - fwd.z*up.y,
            fwd.z*up.x - fwd.x*up.z,
            fwd.x*up.y - fwd.y*up.x,
        };
        Vector3 cam_up = {
            right.y*fwd.z - right.z*fwd.y,
            right.z*fwd.x - right.x*fwd.z,
            right.x*fwd.y - right.y*fwd.x,
        };
        float pan_scale = fwd_len * 0.002f;
        Vector3 pan = {
            (-delta.x * right.x + delta.y * cam_up.x) * pan_scale,
            (-delta.x * right.y + delta.y * cam_up.y) * pan_scale,
            (-delta.x * right.z + delta.y * cam_up.z) * pan_scale,
        };
        app->camera.position.x += pan.x;
        app->camera.position.y += pan.y;
        app->camera.position.z += pan.z;
        app->camera.target.x += pan.x;
        app->camera.target.y += pan.y;
        app->camera.target.z += pan.z;
    }
    /* Toggle follow mode (plain F, not Cmd/Ctrl+F) */
    if (IsKeyPressed(KEY_F) && !IsKeyDown(KEY_LEFT_SUPER) && !IsKeyDown(KEY_LEFT_CONTROL)) {
        if (IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT)) {
            /* Shift+F: toggle follow GC (only meaningful when following) */
            if (app->follow_particle) {
                app->follow_gc = !app->follow_gc;
                cam_trans_start(app, CAM_TRANS_FOLLOW, 0.35f);
            }
        } else {
            app->follow_particle = !app->follow_particle;
            if (!app->follow_particle) app->follow_gc = 0;
            cam_trans_start(app, CAM_TRANS_FOLLOW, 0.35f);
        }
    }
    if (IsKeyPressed(KEY_G)) app->show_Gij = !app->show_Gij;
    if (IsKeyPressed(KEY_THREE)) app->stereo_3d = !app->stereo_3d;
    if (app->stereo_3d) {
        if (IsKeyPressed(KEY_RIGHT_BRACKET)) {
            app->stereo_gap += 0.05f;
            if (app->stereo_gap > 0.8f) app->stereo_gap = 0.8f;
        }
        if (IsKeyPressed(KEY_LEFT_BRACKET)) {
            app->stereo_gap -= 0.05f;
            if (app->stereo_gap < -0.5f) app->stereo_gap = -0.5f;
        }
    }

    /* Reset camera to look at origin */
    if (IsKeyPressed(KEY_ZERO)) {
        FieldModel *fm0 = &app->models[app->current_model];
        float d = (float)fm0->default_camera_dist;
        cam_trans_start(app, CAM_TRANS_RESET, 0.4f);
        app->cam_trans.dest_target = (Vector3){0.0f, 0.0f, 0.0f};
        app->cam_trans.dest_pos = (Vector3){d * 0.7f, -d * 0.7f, d * 0.5f};
        app->cam_trans.dest_up = (Vector3){0.0f, 0.0f, 1.0f};
        app->cam_follow_dist = d;
        app->follow_particle = 0;
        app->cam_field_aligned = 0;
        app->frame_e1_init = 0;
    }

    /* Enter drop-particle mode */
    if (IsKeyPressed(KEY_D) && !app->drop_mode && app->n_particles < MAX_PARTICLES) {
        app->drop_mode = 1;
        /* Reference z from selected particle, or model default */
        if (app->n_particles > 0)
            app->drop_height = app->particles[app->selected_particle].pos.z;
        else
            app->drop_height = app->models[app->current_model].default_pos.z;
    }

placement:
    /* Left-shift+click: place particle at max |B| along line of sight.
     * While left-shift is held (without clicking), show a preview field line
     * at the candidate placement point. */
    {
        int any_shift = IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT);
        Vector2 mouse = GetMousePosition();
        float scene_x, scene_y, scene_w, scene_h;
        app_scene_rect(app, &scene_x, &scene_y, &scene_w, &scene_h);
        int in_scene = app_point_in_scene(app, mouse.x, mouse.y);

        /* Suppress preview while actively dragging (orbit or pan) */
        static int mouse_dragging = 0;
        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) || IsMouseButtonPressed(MOUSE_BUTTON_RIGHT))
            mouse_dragging = 0;
        if (IsMouseButtonDown(MOUSE_BUTTON_LEFT) || IsMouseButtonDown(MOUSE_BUTTON_RIGHT)) {
            Vector2 delta = GetMouseDelta();
            if (delta.x * delta.x + delta.y * delta.y > 4.0f)
                mouse_dragging = 1;
        }
        if (!IsMouseButtonDown(MOUSE_BUTTON_LEFT) && !IsMouseButtonDown(MOUSE_BUTTON_RIGHT))
            mouse_dragging = 0;

        if (any_shift && in_scene && !mouse_dragging && !IsMouseButtonDown(MOUSE_BUTTON_RIGHT)) {
            Ray ray = build_pick_ray(app, mouse, scene_x, scene_y, scene_w, scene_h);
            FieldModel *fm = &app->models[app->current_model];

            /* Sample |B| along ray. Two-pass: first find max |B|, then
             * among points with |B| >= 99% of max, pick closest to camera target.
             * This handles both peaked fields (stellarator) and uniform fields
             * (circular B) where we want the point nearest the target. */
            float cam_d = sqrtf(
                (app->camera.position.x - app->camera.target.x) *
                (app->camera.position.x - app->camera.target.x) +
                (app->camera.position.y - app->camera.target.y) *
                (app->camera.position.y - app->camera.target.y) +
                (app->camera.position.z - app->camera.target.z) *
                (app->camera.position.z - app->camera.target.z));
            float ray_max = cam_d * 4.0f;
            int n_samples = 100;

            /* Pass 1: find max |B| along ray */
            double peak_Bmag = 0.0;
            for (int i = 0; i < n_samples; i++) {
                float t = ray_max * (float)i / (float)(n_samples - 1);
                Vec3 p = {
                    ray.position.x + t * ray.direction.x,
                    ray.position.y + t * ray.direction.y,
                    ray.position.z + t * ray.direction.z,
                };
                double Bm = field_Bmag(fm, p);
                if (Bm > peak_Bmag) peak_Bmag = Bm;
            }

            /* Pass 2: among points with |B| >= 99% of peak, pick closest
             * to camera target */
            double best_Bmag = 0.0;
            Vec3 best_pos = {0, 0, 0};
            double best_dist2 = 1e30;
            double threshold = peak_Bmag * 0.99;
            Vec3 tgt = {app->camera.target.x, app->camera.target.y,
                        app->camera.target.z};
            for (int i = 0; i < n_samples; i++) {
                float t = ray_max * (float)i / (float)(n_samples - 1);
                Vec3 p = {
                    ray.position.x + t * ray.direction.x,
                    ray.position.y + t * ray.direction.y,
                    ray.position.z + t * ray.direction.z,
                };
                double Bm = field_Bmag(fm, p);
                if (Bm >= threshold) {
                    double d2 = vec3_dot(vec3_sub(p, tgt), vec3_sub(p, tgt));
                    if (d2 < best_dist2) {
                        best_dist2 = d2;
                        best_Bmag = Bm;
                        best_pos = p;
                    }
                }
            }

            if (best_Bmag > 1e-15 &&
                isfinite(best_pos.x) && isfinite(best_pos.y) && isfinite(best_pos.z)) {

                /* Snap to nearest existing particle if close in screen space */
                Vec3 snap_pos = best_pos;
                {
                    float snap_px = 20.0f; /* snap radius in pixels */
                    float best_screen_d2 = snap_px * snap_px;
                    Vector2 cursor_screen = GetWorldToScreen(
                        (Vector3){(float)best_pos.x, (float)best_pos.y, (float)best_pos.z},
                        app->camera);
                    for (int pi = 0; pi < app->n_particles; pi++) {
                        Vec3 pp = app->particles[pi].pos;
                        Vector2 ps = GetWorldToScreen(
                            (Vector3){(float)pp.x, (float)pp.y, (float)pp.z},
                            app->camera);
                        float sdx = ps.x - cursor_screen.x;
                        float sdy = ps.y - cursor_screen.y;
                        float sd2 = sdx*sdx + sdy*sdy;
                        if (sd2 < best_screen_d2) {
                            best_screen_d2 = sd2;
                            snap_pos = pp;
                        }
                    }
                }

                app->place_preview_valid = 1;
                app->place_preview_pos = snap_pos;

                /* Track drag distance to distinguish click from drag */
                static Vector2 press_pos = {0};
                static int press_valid = 0;
                if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                    press_pos = mouse;
                    press_valid = 1;
                }
                if (press_valid && IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) {
                    float dx = mouse.x - press_pos.x;
                    float dy = mouse.y - press_pos.y;
                    if (dx*dx + dy*dy < 25.0f) {  /* < 5px drag threshold */
                        app_add_particle(app, snap_pos);
                        app->place_preview_valid = 0;
                    }
                    press_valid = 0;
                }
            } else {
                app->place_preview_valid = 0;
            }
        } else if (!app->drop_mode) {
            app->place_preview_valid = 0;
        }
    }

    /* Drop-particle mode: ray intersects z = drop_height plane.
     * +/- adjust z height, Esc cancels, left-click places and exits. */
    if (app->drop_mode) {
        /* +/- adjust drop height (scale step to camera distance) */
        {
            float cam_d = sqrtf(
                (app->camera.position.x - app->camera.target.x) *
                (app->camera.position.x - app->camera.target.x) +
                (app->camera.position.y - app->camera.target.y) *
                (app->camera.position.y - app->camera.target.y) +
                (app->camera.position.z - app->camera.target.z) *
                (app->camera.position.z - app->camera.target.z));
            double step = cam_d * 0.05;
            if (IsKeyDown(KEY_EQUAL) || IsKeyDown(KEY_KP_ADD))
                app->drop_height += step * GetFrameTime() * 5.0;
            if (IsKeyDown(KEY_MINUS) || IsKeyDown(KEY_KP_SUBTRACT))
                app->drop_height -= step * GetFrameTime() * 5.0;
        }

        /* Esc cancels drop mode */
        if (IsKeyPressed(KEY_ESCAPE)) {
            app->drop_mode = 0;
            app->place_preview_valid = 0;
        }

        /* Ray-plane intersection for preview and placement */
        if (app->drop_mode) {
            Vector2 mouse = GetMousePosition();
            float scene_x, scene_y, scene_w, scene_h;
            app_scene_rect(app, &scene_x, &scene_y, &scene_w, &scene_h);
            int in_scene = app_point_in_scene(app, mouse.x, mouse.y);

            if (in_scene) {
                Ray ray = build_pick_ray(app, mouse, scene_x, scene_y, scene_w, scene_h);

                /* Intersect ray with z = drop_height */
                if (fabs(ray.direction.z) > 1e-10) {
                    double t = (app->drop_height - ray.position.z) / ray.direction.z;
                    if (t > 0.0) {
                        Vec3 hit = {
                            ray.position.x + t * ray.direction.x,
                            ray.position.y + t * ray.direction.y,
                            ray.position.z + t * ray.direction.z,
                        };
                        app->place_preview_valid = 1;
                        app->place_preview_pos = hit;

                        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                            app_add_particle(app, hit);
                            app->drop_mode = 0;
                            app->place_preview_valid = 0;
                        }
                    } else {
                        app->place_preview_valid = 0;
                    }
                } else {
                    app->place_preview_valid = 0;
                }
            } else {
                app->place_preview_valid = 0;
            }
        }
    }

    /* Touch input for mobile */
    {
        int touch_count = GetTouchPointCount();
        static Vector2 prev_touch0 = {0};
        static Vector2 prev_touch1 = {0};
        static int prev_touch_count = 0;
        static float prev_pinch_dist = 0;

        float st_x, st_y, st_w, st_h;
        app_scene_rect(app, &st_x, &st_y, &st_w, &st_h);

        if (touch_count == 1) {
            Vector2 tp = GetTouchPosition(0);
            /* Only orbit if touching the 3D scene area */
            if (app_point_in_scene(app, tp.x, tp.y)) {
                if (prev_touch_count == 1) {
                    app->cam_trans.active = 0;
                    Vector2 delta = {tp.x - prev_touch0.x, tp.y - prev_touch0.y};
                    UpdateCameraPro(&app->camera,
                        (Vector3){0.0f, 0.0f, 0.0f},
                        (Vector3){delta.x * 0.3f, delta.y * 0.3f, 0.0f},
                        0.0f);
                }
            }
            prev_touch0 = tp;
        } else if (touch_count >= 2) {
            Vector2 tp0 = GetTouchPosition(0);
            Vector2 tp1 = GetTouchPosition(1);
            float dx = tp1.x - tp0.x;
            float dy = tp1.y - tp0.y;
            float pinch_dist = sqrtf(dx*dx + dy*dy);

            if (prev_touch_count >= 2) {
                /* Pinch zoom */
                if (prev_pinch_dist > 1.0f) {
                    float zoom = (pinch_dist - prev_pinch_dist) * 0.01f;
                    if (app->follow_particle) {
                        app->cam_follow_dist *= (1.0f - zoom);
                        if (app->cam_follow_dist < 0.01f) app->cam_follow_dist = 0.01f;
                    } else {
                        UpdateCameraPro(&app->camera,
                            (Vector3){0.0f, 0.0f, 0.0f},
                            (Vector3){0.0f, 0.0f, 0.0f},
                            zoom * 5.0f);
                    }
                }

                /* Two-finger pan */
                Vector2 mid = {(tp0.x + tp1.x) * 0.5f, (tp0.y + tp1.y) * 0.5f};
                Vector2 prev_mid = {(prev_touch0.x + prev_touch1.x) * 0.5f,
                                    (prev_touch0.y + prev_touch1.y) * 0.5f};
                Vector2 pan_delta = {mid.x - prev_mid.x, mid.y - prev_mid.y};

                if (fabsf(pan_delta.x) > 0.5f || fabsf(pan_delta.y) > 0.5f) {
                    Vector3 fwd = {
                        app->camera.target.x - app->camera.position.x,
                        app->camera.target.y - app->camera.position.y,
                        app->camera.target.z - app->camera.position.z,
                    };
                    float fwd_len = sqrtf(fwd.x*fwd.x + fwd.y*fwd.y + fwd.z*fwd.z);
                    if (fwd_len > 1e-6f) {
                        fwd.x /= fwd_len; fwd.y /= fwd_len; fwd.z /= fwd_len;
                    }
                    Vector3 up = app->camera.up;
                    Vector3 right = {
                        fwd.y*up.z - fwd.z*up.y,
                        fwd.z*up.x - fwd.x*up.z,
                        fwd.x*up.y - fwd.y*up.x,
                    };
                    Vector3 cam_up = {
                        right.y*fwd.z - right.z*fwd.y,
                        right.z*fwd.x - right.x*fwd.z,
                        right.x*fwd.y - right.y*fwd.x,
                    };
                    float pan_scale = fwd_len * 0.002f;
                    app->camera.position.x += (-pan_delta.x * right.x + pan_delta.y * cam_up.x) * pan_scale;
                    app->camera.position.y += (-pan_delta.x * right.y + pan_delta.y * cam_up.y) * pan_scale;
                    app->camera.position.z += (-pan_delta.x * right.z + pan_delta.y * cam_up.z) * pan_scale;
                    app->camera.target.x += (-pan_delta.x * right.x + pan_delta.y * cam_up.x) * pan_scale;
                    app->camera.target.y += (-pan_delta.x * right.y + pan_delta.y * cam_up.y) * pan_scale;
                    app->camera.target.z += (-pan_delta.x * right.z + pan_delta.y * cam_up.z) * pan_scale;
                }
            }
            prev_touch0 = tp0;
            prev_touch1 = tp1;
            prev_pinch_dist = pinch_dist;
        }

        /* Double-tap to pause/unpause */
        if (GetGestureDetected() == GESTURE_DOUBLETAP) {
            Vector2 tp = GetTouchPosition(0);
            if (app_point_in_scene(app, tp.x, tp.y))
                app->paused = !app->paused;
        }

        prev_touch_count = touch_count;
    }

    /* Zoom with scroll: proportional to camera distance for consistent feel.
     * Suppressed when mouse is over the UI panel (scroll goes to panel instead). */
    float wheel = GetMouseWheelMove();
    if (wheel != 0.0f && !app->mouse_over_panel) {
        app->cam_trans.active = 0;
        float factor = 1.0f - wheel * 0.1f;
        if (factor < 0.5f) factor = 0.5f;
        if (factor > 2.0f) factor = 2.0f;
        if (app->follow_particle) {
            app->cam_follow_dist *= factor;
            if (app->cam_follow_dist < 0.01f) app->cam_follow_dist = 0.01f;
        } else {
            /* Scale camera distance from target multiplicatively */
            Vector3 d = {
                app->camera.position.x - app->camera.target.x,
                app->camera.position.y - app->camera.target.y,
                app->camera.position.z - app->camera.target.z,
            };
            app->camera.position.x = app->camera.target.x + d.x * factor;
            app->camera.position.y = app->camera.target.y + d.y * factor;
            app->camera.position.z = app->camera.target.z + d.z * factor;
        }
    }

physics:
    /* Reinitialise GC particles when relativistic mode changes
     * (p_par meaning switches between v_parallel and p_parallel) */
    {
        static int gc_prev_rel = -1;
        if (gc_prev_rel >= 0 && app->relativistic != gc_prev_rel) {
            FieldModel *rfm = &app->models[app->current_model];
            for (int pi = 0; pi < app->n_particles; pi++) {
                gc_init_from_particle(&app->gc_particles[pi],
                                     &app->particles[pi], rfm, app->relativistic,
                                     app->dt);
                trail_clear(&app->gc_trails[pi]);
            }
        }
        gc_prev_rel = app->relativistic;
    }

    /* Physics */
    if (!app->paused) {
        FieldModel *fm = &app->models[app->current_model];
        int substeps = (int)(app->steps_per_frame * app->playback_speed);
        if (substeps < 1) substeps = 1;
        if (substeps > 100000) substeps = 100000;

        /* Trail push interval: target ~256 trail points per frame so the
         * trail buffer covers consistent physical time regardless of dt.
         * Without this, adding particles in stronger fields shrinks dt,
         * inflates substeps, and fills the trail buffer faster. */
        int trail_interval = app->steps_per_frame / 256;
        if (trail_interval < 1) trail_interval = 1;

        for (int i = 0; i < substeps; i++) {
            /* Save state to history at trail-push points (synced with trail) */
            if (i % trail_interval == 0) {
                int hi = app->step_hist_head;
                for (int pi = 0; pi < app->n_particles; pi++) {
                    app->step_hist_pos[hi][pi] = app->particles[pi].pos;
                    app->step_hist_vel[hi][pi] = app->particles[pi].vel;
                    app->step_hist_gc_pos[hi][pi] = app->gc_particles[pi].pos;
                    app->step_hist_gc_ppar[hi][pi] = app->gc_particles[pi].p_par;
                }
                app->step_hist_time[hi] = app->sim_time;
                app->step_hist_head = (hi + 1) % STEP_HISTORY_MAX;
                if (app->step_hist_count < STEP_HISTORY_MAX)
                    app->step_hist_count++;
            }
            boris_step_batch(app->particles, app->n_particles, fm,
                             app->dt, app->relativistic);
            gc_step_batch(app->gc_particles, app->n_particles, fm,
                          app->dt, app->relativistic, app->gc_symplectic);
            for (int pi = 0; pi < app->n_particles; pi++) {
                if (app->radiation_loss) {
                    double rad_dt = app->dt * pow(10.0, (double)app->radiation_mult);
                    double P = boris_radiation_step(&app->particles[pi], fm,
                                        rad_dt, app->relativistic);
                    if (pi == app->selected_particle) app->rad_power = P;
                }
            }
            if (i % trail_interval == 0) {
                for (int pi = 0; pi < app->n_particles; pi++) {
                    trail_push(&app->trails[pi], app->particles[pi].pos);
                    trail_push(&app->gc_trails[pi], app->gc_particles[pi].pos);
                }
            }
            app->sim_time += app->dt;

            app->step_counter++;
            if (app->step_counter % app->diag_sample_interval == 0) {
                for (int pi = 0; pi < app->n_particles; pi++) {
                    double pitch = boris_pitch_angle(&app->particles[pi], fm);
                    double mu = boris_mu(&app->particles[pi], fm);
                    double E = boris_energy_keV(&app->particles[pi], app->relativistic);
                    diag_push(&app->diags[pi], app->sim_time, pitch, mu, E);
                    /* GC diagnostics at same ring-buffer index */
                    int di = (app->diags[pi].head - 1 + app->diags[pi].capacity)
                             % app->diags[pi].capacity;
                    app->diags[pi].gc_pitch_angle[di] =
                        gc_pitch_angle(&app->gc_particles[pi], fm, app->relativistic);
                    app->diags[pi].gc_mu[di] = app->gc_particles[pi].mu;
                }
            }
        }
    }

    /* Compute field-aligned triad (for camera and/or G_ij) */
    {
        Vec3 p = (app->follow_gc)
                 ? app->gc_particles[app->selected_particle].pos
                 : app->particles[app->selected_particle].pos;
        FieldModel *fm = &app->models[app->current_model];
        Vec3 B = fm->eval_B(fm->params, p);
        double Bmag = vec3_len(B);
        Vec3 bhat = (Bmag > 1e-30) ? vec3_scale(1.0/Bmag, B) : (Vec3){0,0,1};

        /* Curvature: kappa = (bhat . nabla) bhat via finite diff */
        double h = 1e-5;
        Vec3 pfwd = vec3_add(p, vec3_scale(h, bhat));
        Vec3 pbwd = vec3_add(p, vec3_scale(-h, bhat));
        Vec3 Bf = fm->eval_B(fm->params, pfwd);
        Vec3 Bb = fm->eval_B(fm->params, pbwd);
        double Bf_mag = vec3_len(Bf), Bb_mag = vec3_len(Bb);
        Vec3 bf = (Bf_mag > 1e-30) ? vec3_scale(1.0/Bf_mag, Bf) : bhat;
        Vec3 bb = (Bb_mag > 1e-30) ? vec3_scale(1.0/Bb_mag, Bb) : bhat;
        Vec3 kappa = vec3_scale(0.5/h, vec3_sub(bf, bb));
        double kappa_mag = vec3_len(kappa);

        Vec3 e1;
        if (kappa_mag > 1e-8) {
            e1 = vec3_scale(1.0/kappa_mag, kappa);
            app->frame_e1 = e1;
            app->frame_e1_init = 1;
        } else {
            if (!app->frame_e1_init) {
                Vec3 ref = {0, 0, 1};
                if (fabs(vec3_dot(bhat, ref)) > 0.9) ref = (Vec3){1, 0, 0};
                e1 = vec3_sub(ref, vec3_scale(vec3_dot(ref, bhat), bhat));
                e1 = vec3_norm(e1);
                app->frame_e1 = e1;
                app->frame_e1_init = 1;
            } else {
                e1 = vec3_sub(app->frame_e1,
                              vec3_scale(vec3_dot(app->frame_e1, bhat), bhat));
                double e1_len = vec3_len(e1);
                if (e1_len > 1e-10) {
                    e1 = vec3_scale(1.0/e1_len, e1);
                } else {
                    Vec3 ref = {0, 0, 1};
                    if (fabs(vec3_dot(bhat, ref)) > 0.9) ref = (Vec3){1, 0, 0};
                    e1 = vec3_sub(ref, vec3_scale(vec3_dot(ref, bhat), bhat));
                    e1 = vec3_norm(e1);
                }
                app->frame_e1 = e1;
            }
        }

        Vec3 e2 = vec3_cross(bhat, e1);
        double e2_len = vec3_len(e2);
        if (e2_len > 1e-10) e2 = vec3_scale(1.0/e2_len, e2);
        app->frame_e2 = e2;

        /* Compute G_ij = e_i . (e_j . nabla) bhat */
        if (app->show_Gij && Bmag > 1e-30) {
            Vec3 e[3] = {e1, e2, bhat};
            for (int j = 0; j < 3; j++) {
                Vec3 pp = vec3_add(p, vec3_scale(h, e[j]));
                Vec3 pm = vec3_add(p, vec3_scale(-h, e[j]));
                Vec3 Bp = fm->eval_B(fm->params, pp);
                Vec3 Bm = fm->eval_B(fm->params, pm);
                double Bp_mag = vec3_len(Bp), Bm_mag = vec3_len(Bm);
                Vec3 bp = (Bp_mag > 1e-30) ? vec3_scale(1.0/Bp_mag, Bp) : bhat;
                Vec3 bm = (Bm_mag > 1e-30) ? vec3_scale(1.0/Bm_mag, Bm) : bhat;
                Vec3 db = vec3_scale(0.5/h, vec3_sub(bp, bm));
                for (int i = 0; i < 3; i++)
                    app->Gij[i][j] = vec3_dot(e[i], db);
            }
            app->Gij_valid = 1;
        }

        /* Compute desired camera state for follow/field-aligned modes */
        Vector3 want_pos = app->camera.position;
        Vector3 want_target = app->camera.target;
        Vector3 want_up = app->camera.up;
        int have_want = 0;

        if (app->follow_particle) {
            /* Camera follows GC position when follow_gc is active */
            Vec3 follow_pos = (app->follow_gc)
                ? app->gc_particles[app->selected_particle].pos : p;
            Vector3 target = {(float)follow_pos.x, (float)follow_pos.y,
                              (float)follow_pos.z};

            if (app->cam_field_aligned) {
                float d = app->cam_follow_dist;
                Vec3 look_dir = (app->cam_field_aligned == 1) ? bhat
                                : vec3_scale(-1.0, bhat);
                Vec3 cam_pos = vec3_sub(p, vec3_scale(d, look_dir));

                /* Camera up from kappa_screen_dir: choose which triad
                 * vector is "up" so kappa appears in the desired screen
                 * direction. e1=kappa, e2=binormal. */
                Vec3 cup;
                switch (app->kappa_screen_dir) {
                case 0:  cup = e1; break;                       /* kappa up */
                case 1:  cup = vec3_scale(-1.0, e2); break;     /* kappa right */
                case 2:  cup = vec3_scale(-1.0, e1); break;     /* kappa down */
                default: cup = e2; break;                        /* kappa left */
                }

                want_target = target;
                want_pos = (Vector3){
                    (float)cam_pos.x, (float)cam_pos.y, (float)cam_pos.z};
                want_up = (Vector3){(float)cup.x, (float)cup.y, (float)cup.z};
            } else {
                Vector3 offset = {
                    app->camera.position.x - app->camera.target.x,
                    app->camera.position.y - app->camera.target.y,
                    app->camera.position.z - app->camera.target.z,
                };
                float olen = sqrtf(offset.x*offset.x + offset.y*offset.y + offset.z*offset.z);
                if (olen > 1e-6f) {
                    float s = app->cam_follow_dist / olen;
                    offset.x *= s; offset.y *= s; offset.z *= s;
                }
                want_target = target;
                want_pos = (Vector3){
                    target.x + offset.x,
                    target.y + offset.y,
                    target.z + offset.z,
                };
            }
            have_want = 1;
        }

        /* Apply camera transition or direct update */
        if (app->cam_trans.active) {
            float dt_frame = GetFrameTime();
            app->cam_trans.elapsed += dt_frame;
            float raw_t = app->cam_trans.elapsed / app->cam_trans.duration;
            if (raw_t >= 1.0f) {
                raw_t = 1.0f;
                app->cam_trans.active = 0;
            }
            float t = smoothstep(raw_t);

            Vector3 dest_pos, dest_target, dest_up;
            if (app->cam_trans.mode == CAM_TRANS_RESET) {
                dest_pos = app->cam_trans.dest_pos;
                dest_target = app->cam_trans.dest_target;
                dest_up = app->cam_trans.dest_up;
            } else {
                dest_pos = want_pos;
                dest_target = want_target;
                dest_up = want_up;
            }

            app->camera.position = v3f_lerp(app->cam_trans.start_pos, dest_pos, t);
            app->camera.target = v3f_lerp(app->cam_trans.start_target, dest_target, t);
            app->camera.up = v3f_slerp(
                v3f_normalize(app->cam_trans.start_up),
                v3f_normalize(dest_up), t);
        } else if (have_want) {
            app->camera.position = want_pos;
            app->camera.target = want_target;
            app->camera.up = want_up;
        }
    }


    /* Log events for reproducible recordings */
    if (app->event_log)
        event_log_check(app);
}

/* Inner rendering (no BeginDrawing/EndDrawing). Called by app_render and
 * splash breakup transition so the program can be composited underneath. */
void app_render_inner(AppState *app)
{
    ClearBackground(app->theme.bg);

    /* 3D scene: full window when UI hidden, otherwise leave room */
    float scene_x, scene_y, scene_w, scene_h;
    app_scene_rect(app, &scene_x, &scene_y, &scene_w, &scene_h);

    if (app->stereo_3d) {
        /* Side-by-side stereoscopic: left eye on left half, right eye on right.
         * Must set projection manually because BeginMode3D uses the full
         * framebuffer aspect ratio, ignoring per-eye viewports. */
        Vector3 fwd = {
            app->camera.target.x - app->camera.position.x,
            app->camera.target.y - app->camera.position.y,
            app->camera.target.z - app->camera.position.z,
        };
        float fwd_len = sqrtf(fwd.x*fwd.x + fwd.y*fwd.y + fwd.z*fwd.z);
        if (fwd_len > 1e-6f) { fwd.x /= fwd_len; fwd.y /= fwd_len; fwd.z /= fwd_len; }
        Vector3 up = app->camera.up;
        Vector3 cam_right = {
            fwd.y*up.z - fwd.z*up.y,
            fwd.z*up.x - fwd.x*up.z,
            fwd.x*up.y - fwd.y*up.x,
        };
        float sep = app->stereo_separation * fwd_len;
        int half_w = (int)(scene_w * 0.5f);

        /* Framebuffer coords for rlViewport (handles HiDPI) */
        int fb_w = GetRenderWidth();
        int fb_h = GetRenderHeight();
        float scale = (float)fb_w / (float)app->win_w;
        int vp_half_w = (int)(half_w * scale);
        int vp_scene_h = (int)(scene_h * scale);
        int vp_y = fb_h - vp_scene_h;
        float aspect = (float)half_w / scene_h;

        /* Gap offset: positive pushes images apart, negative overlaps them.
         * gap is fraction of half_w; offset in screen pixels. */
        int gap_px = (int)(app->stereo_gap * half_w * 0.5f);

        for (int eye = 0; eye < 2; eye++) {
            float sign = (eye == 0) ? -1.0f : 1.0f;
            Camera3D eye_cam = app->camera;
            eye_cam.position.x += sign * sep * 0.5f * cam_right.x;
            eye_cam.position.y += sign * sep * 0.5f * cam_right.y;
            eye_cam.position.z += sign * sep * 0.5f * cam_right.z;

            /* Left eye shifts left by gap, right eye shifts right */
            int x_offset = (eye == 0) ? -gap_px : gap_px;
            int sx = (int)scene_x + eye * half_w + x_offset;
            BeginScissorMode(sx, 0, half_w, (int)scene_h);
            int vp_x = (int)((sx) * scale);
            rlViewport(vp_x, vp_y, vp_half_w, vp_scene_h);

            /* Projection */
            rlDrawRenderBatchActive();
            rlMatrixMode(RL_PROJECTION);
            rlPushMatrix();
            rlLoadIdentity();
            if (app->camera.projection == CAMERA_ORTHOGRAPHIC) {
                Vector3 dv = { eye_cam.position.x - eye_cam.target.x,
                               eye_cam.position.y - eye_cam.target.y,
                               eye_cam.position.z - eye_cam.target.z };
                double cd = sqrt(dv.x*dv.x + dv.y*dv.y + dv.z*dv.z);
                double otop = cd * tan(eye_cam.fovy * 0.5 * DEG2RAD);
                double ort = otop * aspect;
                rlOrtho(-ort, ort, -otop, otop,
                         RL_CULL_DISTANCE_NEAR, RL_CULL_DISTANCE_FAR);
            } else {
                double top = RL_CULL_DISTANCE_NEAR * tan(eye_cam.fovy * 0.5 * DEG2RAD);
                double rt = top * aspect;
                rlFrustum(-rt, rt, -top, top,
                           RL_CULL_DISTANCE_NEAR, RL_CULL_DISTANCE_FAR);
            }

            /* View */
            rlMatrixMode(RL_MODELVIEW);
            rlLoadIdentity();
            Matrix mv = MatrixLookAt(eye_cam.position, eye_cam.target, eye_cam.up);
            rlMultMatrixf(MatrixToFloatV(mv).v);

            rlEnableDepthTest();
            render3d_draw(app);

            rlDrawRenderBatchActive();
            rlDisableDepthTest();
            rlMatrixMode(RL_PROJECTION);
            rlPopMatrix();
            rlMatrixMode(RL_MODELVIEW);
            rlLoadIdentity();

            EndScissorMode();
        }
        /* Restore full viewport */
        rlViewport(0, 0, fb_w, fb_h);
    } else {
        BeginScissorMode((int)scene_x, (int)scene_y, (int)scene_w, (int)scene_h);
        /* Set viewport and projection manually so aspect ratio matches
         * the scene area, not the full window. */
        int fb_w = GetRenderWidth(), fb_h = GetRenderHeight();
        float scale = (float)fb_w / (float)app->win_w;
        int vp_x = (int)(scene_x * scale);
        int vp_y = fb_h - (int)((scene_y + scene_h) * scale);
        int vp_w = (int)(scene_w * scale);
        int vp_h = (int)(scene_h * scale);
        rlViewport(vp_x, vp_y, vp_w, vp_h);

        float aspect = scene_w / scene_h;
        rlDrawRenderBatchActive();
        rlMatrixMode(RL_PROJECTION);
        rlPushMatrix();
        rlLoadIdentity();
        if (app->camera.projection == CAMERA_ORTHOGRAPHIC) {
            /* Ortho half-height from camera distance and fovy so zoom feels
             * consistent when switching between perspective and ortho. */
            Vector3 dv = { app->camera.position.x - app->camera.target.x,
                           app->camera.position.y - app->camera.target.y,
                           app->camera.position.z - app->camera.target.z };
            double cd = sqrt(dv.x*dv.x + dv.y*dv.y + dv.z*dv.z);
            double otop = cd * tan(app->camera.fovy * 0.5 * DEG2RAD);
            double ort = otop * aspect;
            rlOrtho(-ort, ort, -otop, otop, RL_CULL_DISTANCE_NEAR, RL_CULL_DISTANCE_FAR);
        } else {
            double top = RL_CULL_DISTANCE_NEAR * tan(app->camera.fovy * 0.5 * DEG2RAD);
            double rt = top * aspect;
            rlFrustum(-rt, rt, -top, top, RL_CULL_DISTANCE_NEAR, RL_CULL_DISTANCE_FAR);
        }

        rlMatrixMode(RL_MODELVIEW);
        rlLoadIdentity();
        Matrix mv = MatrixLookAt(app->camera.position, app->camera.target, app->camera.up);
        rlMultMatrixf(MatrixToFloatV(mv).v);

        rlEnableDepthTest();
        render3d_draw(app);

        rlDrawRenderBatchActive();
        rlDisableDepthTest();
        rlMatrixMode(RL_PROJECTION);
        rlPopMatrix();
        rlMatrixMode(RL_MODELVIEW);
        rlLoadIdentity();

        rlViewport(0, 0, fb_w, fb_h);
        EndScissorMode();
    }

    /* When help or file picker is shown, draw only the overlay */
    if (app->show_help || app->playback.show_picker) {
        ui_render(app);
        return;
    }

    /* Scale bar: horizontal bar with SI-prefixed length label */
    if (app->show_scale_bar && scene_w > 100 && scene_h > 50) {
        /* Meters per pixel at the camera target distance */
        float cam_dist = sqrtf(
            (app->camera.position.x - app->camera.target.x) *
            (app->camera.position.x - app->camera.target.x) +
            (app->camera.position.y - app->camera.target.y) *
            (app->camera.position.y - app->camera.target.y) +
            (app->camera.position.z - app->camera.target.z) *
            (app->camera.position.z - app->camera.target.z));
        float mpp = 2.0f * cam_dist * tanf(app->camera.fovy * 0.5f * DEG2RAD) / scene_h;

        /* Pick a nice round bar length: target ~120 pixels */
        float target_m = mpp * 120.0f;
        /* Round to 1, 2, or 5 × 10^n */
        double exp = floor(log10(target_m));
        double base = target_m / pow(10.0, exp);
        double nice;
        if (base < 1.5) nice = 1.0;
        else if (base < 3.5) nice = 2.0;
        else if (base < 7.5) nice = 5.0;
        else nice = 10.0;
        double bar_m = nice * pow(10.0, exp);
        float bar_px = (float)(bar_m / mpp);

        /* SI prefix */
        const char *prefixes[] = {"am","fm","pm","nm","\xc2\xb5m","mm","m","km","Mm","Gm","Tm"};
        double scales[] = {1e-18,1e-15,1e-12,1e-9,1e-6,1e-3,1,1e3,1e6,1e9,1e12};
        int pi = 6; /* default: meters */
        for (int i = 0; i < 11; i++) {
            if (bar_m < scales[i] * 1000.0) { pi = i; break; }
        }
        double display_val = bar_m / scales[pi];
        char label[32];
        if (display_val >= 100.0) snprintf(label, sizeof(label), "%.0f %s", display_val, prefixes[pi]);
        else if (display_val >= 10.0) snprintf(label, sizeof(label), "%.0f %s", display_val, prefixes[pi]);
        else if (display_val >= 1.0) snprintf(label, sizeof(label), "%.1g %s", display_val, prefixes[pi]);
        else snprintf(label, sizeof(label), "%.2g %s", display_val, prefixes[pi]);

        /* Draw at bottom-right of 3D scene */
        float margin = 16;
        float bar_y = scene_y + scene_h - margin - 6;
        float bar_x = scene_x + scene_w - margin - bar_px;
        Color bar_c = app->theme.text_dim;
        bar_c.a = 180;
        DrawRectangle((int)bar_x, (int)bar_y, (int)bar_px, 3, bar_c);
        /* End ticks */
        DrawRectangle((int)bar_x, (int)(bar_y - 3), 1, 9, bar_c);
        DrawRectangle((int)(bar_x + bar_px - 1), (int)(bar_y - 3), 1, 9, bar_c);
        /* Label centered above bar */
        float lsz = 16;
        Vector2 lw = MeasureTextEx(app->font_mono, label, lsz, 1);
        DrawTextEx(app->font_mono, label,
                   (Vector2){bar_x + bar_px * 0.5f - lw.x * 0.5f, bar_y - lw.y - 2},
                   lsz, 1, bar_c);
    }

    /* G_ij tensor overlay (upper left of 3D scene) */
    if (app->show_Gij && app->Gij_valid) {
        float gz = app->gij_zoom;
        float gx = scene_x + 14, gy = 10;
        float fsz = 11 * gz;
        float row_sp = 14 * gz;

        /* Find max absolute value to determine scale factor */
        double maxval = 0;
        for (int i = 0; i < 3; i++)
            for (int j = 0; j < 3; j++) {
                double a = fabs(app->Gij[i][j]);
                if (a > maxval) maxval = a;
            }

        int exponent = 0;
        double scale = 1.0;
        if (maxval > 1e-30) {
            double raw = floor(log10(maxval));
            /* Truncate toward zero in steps of 3 */
            exponent = (int)(trunc(raw / 3.0)) * 3;
            scale = pow(10.0, -exponent);
        }

        Color mat_color = app->theme.text_dim;
        float exp_fsz = fsz * 0.75f;

        /* Label: G with subscript ij */
        DrawTextEx(app->font_mono, "G",
                   (Vector2){gx, gy}, fsz, 1, app->theme.text_dim);
        Vector2 gsz = MeasureTextEx(app->font_mono, "G", fsz, 1);
        DrawTextEx(app->font_mono, "ij",
                   (Vector2){gx + gsz.x, gy + fsz * 0.3f}, exp_fsz, 1, app->theme.text_dim);
        gy += row_sp;

        /* Matrix values start near left edge; brackets indent from values */
        float mat_x = gx + 10;
        Vector2 emsz = MeasureTextEx(app->font_mono, "M", fsz, 1);
        float paren_x = mat_x - 3 + emsz.x * 0.75f;
        float mat_w = 0;

        for (int i = 0; i < 3; i++) {
            char row[128];
            snprintf(row, sizeof(row), "%+7.3f %+7.3f %+7.3f",
                     app->Gij[i][0] * scale,
                     app->Gij[i][1] * scale,
                     app->Gij[i][2] * scale);
            DrawTextEx(app->font_mono, row, (Vector2){mat_x, gy + i * row_sp},
                       fsz, 1, mat_color);
            if (i == 0) {
                Vector2 sz = MeasureTextEx(app->font_mono, row, fsz, 1);
                mat_w = sz.x;
            }
        }

        /* Draw parentheses as vertical lines with serifs */
        float py0 = gy - 2 + row_sp * 0.125f;
        float py1 = gy + 3 * row_sp - 4 + row_sp * 0.125f;
        float px_l = paren_x;
        float px_r = mat_x + mat_w + 5 + emsz.x * 0.75f;
        float serif = 4;
        Color pc = mat_color;
        /* Left paren */
        DrawLineEx((Vector2){px_l, py0}, (Vector2){px_l, py1}, 1.0f, pc);
        DrawLineEx((Vector2){px_l, py0}, (Vector2){px_l + serif, py0}, 1.0f, pc);
        DrawLineEx((Vector2){px_l, py1}, (Vector2){px_l + serif, py1}, 1.0f, pc);
        /* Right paren */
        DrawLineEx((Vector2){px_r, py0}, (Vector2){px_r, py1}, 1.0f, pc);
        DrawLineEx((Vector2){px_r - serif, py0}, (Vector2){px_r, py0}, 1.0f, pc);
        DrawLineEx((Vector2){px_r - serif, py1}, (Vector2){px_r, py1}, 1.0f, pc);

        /* Units / scale factor to the right of closing paren */
        float ux = px_r + 6;
        float uy = gy;
        if (exponent != 0) {
            /* "× 10" */
            DrawTextEx(app->font_mono, "\xc3\x97 10",
                       (Vector2){ux, uy}, fsz, 1, app->theme.text_dim);
            Vector2 tsz = MeasureTextEx(app->font_mono, "\xc3\x97 10", fsz, 1);
            /* Superscript exponent */
            char expbuf[16];
            snprintf(expbuf, sizeof(expbuf), "%d", exponent);
            DrawTextEx(app->font_mono, expbuf,
                       (Vector2){ux + tsz.x, uy - exp_fsz * 0.4f}, exp_fsz, 1, app->theme.text_dim);
            Vector2 esz = MeasureTextEx(app->font_mono, expbuf, exp_fsz, 1);
            ux += tsz.x + esz.x + 2;
        }
        /* "m" then superscript "-1" */
        DrawTextEx(app->font_mono, " m",
                   (Vector2){ux, uy}, fsz, 1, app->theme.text_dim);
        Vector2 msz = MeasureTextEx(app->font_mono, " m", fsz, 1);
        DrawTextEx(app->font_mono, "-1",
                   (Vector2){ux + msz.x, uy - exp_fsz * 0.4f}, exp_fsz, 1, app->theme.text_dim);

        gy += 3 * row_sp;

        /* Store bounding rect for mouse-over hit testing */
        float gij_x0 = scene_x + 14;
        float gij_y0 = 10;
        Vector2 m1sz = MeasureTextEx(app->font_mono, "-1", exp_fsz, 1);
        app->gij_rect[0] = gij_x0;
        app->gij_rect[1] = gij_y0;
        app->gij_rect[2] = (ux + msz.x + m1sz.x) - gij_x0;
        app->gij_rect[3] = gy - gij_y0;
    }

    /* 2D telemetry plots (independent of UI panel) */
    if (app->show_plots)
        render2d_draw(app);

    /* UI panel (clay render commands from ui_update) */
    if (app->ui_visible)
        ui_render(app);

    /* Overlays: hidden when recording so they stay out of video */
    if (!app->recorder.active) {
        if (!app->ui_visible) {
            DrawTextEx(app->font_ui, TR(STR_SHOW_UI),
                       (Vector2){8, app->win_h - 20}, 12, 1, app->theme.text_dim);
        }
        if (app->trigger_record) {
            DrawTextEx(app->font_ui, TR(STR_ARMED),
                       (Vector2){app->win_w - 70, 12}, 14, 1, ORANGE);
        }
        if (app->drop_mode) {
            char drop_hud[64];
            snprintf(drop_hud, sizeof(drop_hud), "DROP  z=%.3g", app->drop_height);
            Vector2 dsz = MeasureTextEx(app->font_mono, drop_hud, 14, 1);
            float dx = (app->win_w - dsz.x) * 0.5f;
            DrawTextEx(app->font_mono, drop_hud,
                       (Vector2){dx, 12}, 14, 1, ORANGE);
        }
    }

}

void app_render(AppState *app)
{
    /* Clay layout phase (must happen before BeginDrawing) */
    if (app->ui_visible || app->show_help || app->playback.show_picker
        || app->tutorial.active) ui_update(app);

    BeginDrawing();
    app_render_inner(app);
    EndDrawing();

    if (app->recorder.active) {
        Image img = LoadImageFromScreen();
        recorder_write_frame(&app->recorder,
                             (const unsigned char *)img.data,
                             img.width, img.height);
        UnloadImage(img);
    }
}

#ifndef __EMSCRIPTEN__
/* Resolve state directory per platform, create if needed */
static void resolve_state_dir(char *buf, int buflen)
{
#ifdef _WIN32
    const char *appdata = getenv("LOCALAPPDATA");
    if (appdata) {
        snprintf(buf, buflen, "%s\\Lorentz_Tracer", appdata);
        _mkdir(buf);
        return;
    }
#else
    const char *home = getenv("HOME");
    if (home) {
#ifdef __APPLE__
        snprintf(buf, buflen, "%s/Library/Application Support/Lorentz_Tracer", home);
#else
        const char *xdg = getenv("XDG_STATE_HOME");
        if (xdg)
            snprintf(buf, buflen, "%s/lorentz_tracer", xdg);
        else
            snprintf(buf, buflen, "%s/.local/state/lorentz_tracer", home);
#endif
        mkdir(buf, 0755);
        return;
    }
#endif
    snprintf(buf, buflen, ".");
}

static void state_file_path(char *buf, int buflen)
{
    char dir[512];
    resolve_state_dir(dir, sizeof(dir));
#ifdef _WIN32
    snprintf(buf, buflen, "%s\\state.ini", dir);
#else
    snprintf(buf, buflen, "%s/state.ini", dir);
#endif
}

void app_save_state(const AppState *app)
{
    char path[600];
    state_file_path(path, sizeof(path));
    FILE *f = fopen(path, "w");
    if (!f) return;

    fprintf(f, "current_model=%d\n", app->current_model);
    fprintf(f, "species=%d\n", app->species);
    fprintf(f, "E_keV=%.15g\n", app->E_keV);
    fprintf(f, "energy_range=%d\n", app->energy_range);
    fprintf(f, "pitch_deg=%.15g\n", app->pitch_deg);
    fprintf(f, "init_pos=%.15g %.15g %.15g\n",
            app->init_pos.x, app->init_pos.y, app->init_pos.z);
    fprintf(f, "custom_q=%.15g\n", app->custom_q);
    fprintf(f, "custom_m=%.15g\n", app->custom_m);
    fprintf(f, "custom_speed=%.15g\n", app->custom_speed);
    fprintf(f, "playback_speed=%.15g\n", app->playback_speed);
    fprintf(f, "speed_range=%d\n", app->speed_range);
    fprintf(f, "dark_mode=%d\n", app->dark_mode);
    fprintf(f, "show_trail=%d\n", app->show_trail);
    fprintf(f, "show_gc_trajectory=%d\n", app->show_gc_trajectory);
    fprintf(f, "gc_symplectic=%d\n", app->gc_symplectic);
    fprintf(f, "trail_fade=%.6g\n", app->trail_fade);
    fprintf(f, "show_field_lines=%d\n", app->show_field_lines);
    fprintf(f, "show_velocity_vec=%d\n", app->show_velocity_vec);
    fprintf(f, "show_B_vec=%d\n", app->show_B_vec);
    fprintf(f, "show_F_vec=%d\n", app->show_F_vec);
    fprintf(f, "show_scale_bar=%d\n", app->show_scale_bar);
    fprintf(f, "vec_scaled=%d\n", app->vec_scaled);
    fprintf(f, "vec_scale_v=%.6g\n", app->vec_scale_v);
    fprintf(f, "vec_scale_B=%.6g\n", app->vec_scale_B);
    fprintf(f, "vec_scale_F=%.6g\n", app->vec_scale_F);
    fprintf(f, "arrow_head_size=%.6g\n", app->arrow_head_size);
    fprintf(f, "lw_field_lines=%d\n", app->lw_field_lines);
    fprintf(f, "lw_gc_fl=%d\n", app->lw_gc_fl);
    fprintf(f, "lw_pos_fl=%d\n", app->lw_pos_fl);
    fprintf(f, "lw_arrows=%d\n", app->lw_arrows);
    fprintf(f, "lw_axes=%d\n", app->lw_axes);
    fprintf(f, "lw_triad=%d\n", app->lw_triad);
    fprintf(f, "plot_range=%d\n", app->plot_range);
    fprintf(f, "pitch_autoscale=%d\n", app->pitch_autoscale);
    fprintf(f, "follow_particle=%d\n", app->follow_particle);
    fprintf(f, "ui_visible=%d\n", app->ui_visible);
    fprintf(f, "ui_edge=%d\n", app->ui_edge);
    fprintf(f, "ui_zoom=%.6g\n", app->ui_zoom);
    fprintf(f, "show_plots=%d\n", app->show_plots);
    fprintf(f, "plots_edge=%d\n", app->plots_edge);
    fprintf(f, "plot_zoom=%.6g\n", app->plot_zoom);
    fprintf(f, "show_Gij=%d\n", app->show_Gij);
    fprintf(f, "gij_zoom=%.6g\n", app->gij_zoom);
    fprintf(f, "show_gc_field_line=%d\n", app->show_gc_field_line);
    fprintf(f, "show_gc_trajectory=%d\n", app->show_gc_trajectory);
    fprintf(f, "gc_symplectic=%d\n", app->gc_symplectic);
    fprintf(f, "show_pos_field_line=%d\n", app->show_pos_field_line);
    fprintf(f, "gc_fl_length=%.6g\n", app->gc_fl_length);
    fprintf(f, "axis_scale=%.6g\n", app->axis_scale);
    fprintf(f, "trail_thickness=%.6g\n", app->trail_thickness);
    fprintf(f, "help_zoom=%.6g\n", app->help_zoom);
    fprintf(f, "radiation_loss=%d\n", app->radiation_loss);
    fprintf(f, "radiation_mult=%.6g\n", app->radiation_mult);
    fprintf(f, "relativistic=%d\n", app->relativistic);
    fprintf(f, "camera_pos=%.9g %.9g %.9g\n",
            app->camera.position.x, app->camera.position.y, app->camera.position.z);
    fprintf(f, "camera_target=%.9g %.9g %.9g\n",
            app->camera.target.x, app->camera.target.y, app->camera.target.z);
    fprintf(f, "camera_fovy=%.9g\n", app->camera.fovy);
    fprintf(f, "camera_projection=%d\n", app->camera.projection);
    fprintf(f, "cam_field_aligned=%d\n", app->cam_field_aligned);
    fprintf(f, "kappa_screen_dir=%d\n", app->kappa_screen_dir);
    fprintf(f, "tutorial_seen=%d\n", app->tutorial_seen);
    fprintf(f, "language=%s\n", i18n_lang_code());
    for (int i = 0; i < 5; i++)
        fprintf(f, "ui_section_%d=%d\n", i, app->ui_section_open[i]);

    /* User colors — save both dark and light sets.
     * Active colors are the current theme; the other theme is in storage. */
    {
        static const char *cnames[NUM_USER_COLORS] = {
            "color_vel", "color_B", "color_F", "color_kappa", "color_binormal",
            "color_field_lines", "color_gc_fl", "color_pos_fl", "color_axes"
        };
        /* Get active colors into a temp array */
        Color active[NUM_USER_COLORS];
        colors_to_array(app, active);

        const Color *dk_c  = app->dark_mode ? active : app->dark_colors;
        const Color *dk_s  = app->dark_mode ? app->species_colors : app->dark_species;
        const Color *lt_c  = app->dark_mode ? app->light_colors : active;
        const Color *lt_s  = app->dark_mode ? app->light_species : app->species_colors;

        for (int i = 0; i < NUM_USER_COLORS; i++)
            fprintf(f, "dark_%s=%d %d %d %d\n", cnames[i],
                    dk_c[i].r, dk_c[i].g, dk_c[i].b, dk_c[i].a);
        for (int i = 0; i < NUM_SPECIES; i++)
            fprintf(f, "dark_species_color_%d=%d %d %d %d\n", i,
                    dk_s[i].r, dk_s[i].g, dk_s[i].b, dk_s[i].a);
        for (int i = 0; i < NUM_USER_COLORS; i++)
            fprintf(f, "light_%s=%d %d %d %d\n", cnames[i],
                    lt_c[i].r, lt_c[i].g, lt_c[i].b, lt_c[i].a);
        for (int i = 0; i < NUM_SPECIES; i++)
            fprintf(f, "light_species_color_%d=%d %d %d %d\n", i,
                    lt_s[i].r, lt_s[i].g, lt_s[i].b, lt_s[i].a);
    }

    /* Per-model parameters */
    for (int m = 0; m < FIELD_NUM_MODELS; m++) {
        const FieldModel *fm = &app->models[m];
        for (int p = 0; p < fm->n_params; p++)
            fprintf(f, "model_%d_param_%d=%.15g\n", m, p, fm->params[p]);
    }

    fclose(f);
}

void app_load_state(AppState *app)
{
    char path[600];
    state_file_path(path, sizeof(path));
    FILE *f = fopen(path, "r");
    if (!f) return;

    char line[256];
    while (fgets(line, sizeof(line), f)) {
        char key[128];
        char val[128];
        if (sscanf(line, "%127[^=]=%127[^\n]", key, val) != 2) continue;

        if (strcmp(key, "current_model") == 0)
            app->current_model = atoi(val);
        else if (strcmp(key, "species") == 0)
            app->species = atoi(val);
        else if (strcmp(key, "E_keV") == 0)
            app->E_keV = atof(val);
        else if (strcmp(key, "energy_range") == 0)
            app->energy_range = atoi(val);
        else if (strcmp(key, "pitch_deg") == 0)
            app->pitch_deg = atof(val);
        else if (strcmp(key, "init_pos") == 0)
            sscanf(val, "%lf %lf %lf",
                   &app->init_pos.x, &app->init_pos.y, &app->init_pos.z);
        else if (strcmp(key, "custom_q") == 0)
            app->custom_q = atof(val);
        else if (strcmp(key, "custom_m") == 0)
            app->custom_m = atof(val);
        else if (strcmp(key, "custom_speed") == 0)
            app->custom_speed = atof(val);
        else if (strcmp(key, "playback_speed") == 0)
            app->playback_speed = atof(val);
        else if (strcmp(key, "speed_range") == 0)
            app->speed_range = atoi(val);
        else if (strcmp(key, "dark_mode") == 0)
            app->dark_mode = atoi(val);
        else if (strcmp(key, "show_field_lines") == 0)
            app->show_field_lines = atoi(val);
        else if (strcmp(key, "show_velocity_vec") == 0)
            app->show_velocity_vec = atoi(val);
        else if (strcmp(key, "show_F_vec") == 0)
            app->show_F_vec = atoi(val);
        else if (strcmp(key, "vec_scaled") == 0)
            app->vec_scaled = atoi(val);
        else if (strcmp(key, "vec_scale_v") == 0)
            app->vec_scale_v = (float)atof(val);
        else if (strcmp(key, "vec_scale_B") == 0)
            app->vec_scale_B = (float)atof(val);
        else if (strcmp(key, "vec_scale_F") == 0)
            app->vec_scale_F = (float)atof(val);
        else if (strcmp(key, "arrow_head_size") == 0)
            app->arrow_head_size = (float)atof(val);
        else if (strcmp(key, "lw_field_lines") == 0)
            app->lw_field_lines = atoi(val);
        else if (strcmp(key, "lw_gc_fl") == 0)
            app->lw_gc_fl = atoi(val);
        else if (strcmp(key, "lw_pos_fl") == 0)
            app->lw_pos_fl = atoi(val);
        else if (strcmp(key, "lw_arrows") == 0)
            app->lw_arrows = atoi(val);
        else if (strcmp(key, "lw_axes") == 0)
            app->lw_axes = atoi(val);
        else if (strcmp(key, "lw_triad") == 0)
            app->lw_triad = atoi(val);
        else if (strcmp(key, "show_scale_bar") == 0)
            app->show_scale_bar = atoi(val);
        else if (strcmp(key, "show_B_vec") == 0)
            app->show_B_vec = atoi(val);
        else if (strcmp(key, "plot_range") == 0)
            app->plot_range = atoi(val);
        else if (strcmp(key, "pitch_autoscale") == 0)
            app->pitch_autoscale = atoi(val);
        else if (strcmp(key, "follow_particle") == 0)
            app->follow_particle = atoi(val);
        else if (strcmp(key, "ui_visible") == 0)
            app->ui_visible = atoi(val);
        else if (strcmp(key, "ui_edge") == 0)
            app->ui_edge = atoi(val);
        else if (strcmp(key, "ui_zoom") == 0) {
            app->ui_zoom = (float)atof(val);
            if (app->ui_zoom < 0.5f) app->ui_zoom = 0.5f;
            if (app->ui_zoom > 2.5f) app->ui_zoom = 2.5f;
        }
        else if (strcmp(key, "show_plots") == 0)
            app->show_plots = atoi(val);
        else if (strcmp(key, "plots_edge") == 0)
            app->plots_edge = atoi(val);
        else if (strcmp(key, "plot_zoom") == 0) {
            app->plot_zoom = (float)atof(val);
            if (app->plot_zoom < 0.5f) app->plot_zoom = 0.5f;
            if (app->plot_zoom > 2.5f) app->plot_zoom = 2.5f;
        }
        else if (strcmp(key, "show_Gij") == 0)
            app->show_Gij = atoi(val);
        else if (strcmp(key, "gij_zoom") == 0) {
            app->gij_zoom = (float)atof(val);
            if (app->gij_zoom < 0.5f) app->gij_zoom = 0.5f;
            if (app->gij_zoom > 2.5f) app->gij_zoom = 2.5f;
        }
        else if (strcmp(key, "show_gc_field_line") == 0)
            app->show_gc_field_line = atoi(val);
        else if (strcmp(key, "show_gc_trajectory") == 0)
            app->show_gc_trajectory = atoi(val);
        else if (strcmp(key, "gc_symplectic") == 0)
            app->gc_symplectic = atoi(val) ? 1 : 0;
        else if (strcmp(key, "show_pos_field_line") == 0)
            app->show_pos_field_line = atoi(val);
        else if (strcmp(key, "gc_fl_length") == 0)
            app->gc_fl_length = atof(val);
        else if (strcmp(key, "axis_scale") == 0)
            app->axis_scale = (float)atof(val);
        else if (strcmp(key, "trail_thickness") == 0) {
            app->trail_thickness = (float)atof(val);
            if (app->trail_thickness < 1.0f) app->trail_thickness = 1.0f;
        }
        else if (strcmp(key, "help_zoom") == 0) {
            app->help_zoom = (float)atof(val);
            if (app->help_zoom < 0.5f) app->help_zoom = 0.5f;
            if (app->help_zoom > 2.5f) app->help_zoom = 2.5f;
        }
        else if (strcmp(key, "radiation_loss") == 0)
            app->radiation_loss = atoi(val);
        else if (strcmp(key, "radiation_mult") == 0)
            app->radiation_mult = (float)atof(val);
        else if (strcmp(key, "relativistic") == 0)
            app->relativistic = atoi(val);
        else if (strcmp(key, "camera_pos") == 0)
            sscanf(val, "%f %f %f",
                   &app->camera.position.x, &app->camera.position.y, &app->camera.position.z);
        else if (strcmp(key, "camera_target") == 0)
            sscanf(val, "%f %f %f",
                   &app->camera.target.x, &app->camera.target.y, &app->camera.target.z);
        else if (strcmp(key, "camera_fovy") == 0)
            app->camera.fovy = (float)atof(val);
        else if (strcmp(key, "camera_projection") == 0) {
            int p = atoi(val);
            app->camera.projection = (p == CAMERA_ORTHOGRAPHIC)
                ? CAMERA_ORTHOGRAPHIC : CAMERA_PERSPECTIVE;
        }
        else if (strcmp(key, "cam_field_aligned") == 0)
            app->cam_field_aligned = atoi(val);
        else if (strcmp(key, "kappa_screen_dir") == 0)
            app->kappa_screen_dir = atoi(val);
        else if (strcmp(key, "tutorial_seen") == 0)
            app->tutorial_seen = atoi(val);
        else if (strcmp(key, "language") == 0) {
            LangId lid = i18n_lang_from_code(val);
            app->language = lid;
            i18n_set_lang(lid);
        }
        else if (strcmp(key, "show_trail") == 0)
            app->show_trail = atoi(val);
        else if (strcmp(key, "trail_fade") == 0)
            app->trail_fade = (float)atof(val);
        else {
            /* Per-model parameters: model_M_param_P */
            int m, p;
            if (sscanf(key, "model_%d_param_%d", &m, &p) == 2) {
                if (m >= 0 && m < FIELD_NUM_MODELS &&
                    p >= 0 && p < app->models[m].n_params)
                    app->models[m].params[p] = atof(val);
            }
            /* Collapsible section state */
            int s;
            if (sscanf(key, "ui_section_%d", &s) == 1 && s >= 0 && s < 5)
                app->ui_section_open[s] = atoi(val);

            /* Per-theme colors (new format: dark_color_vel, light_color_vel, etc.) */
            static const char *cnames[NUM_USER_COLORS] = {
                "color_vel", "color_B", "color_F", "color_kappa", "color_binormal",
                "color_field_lines", "color_gc_fl", "color_pos_fl", "color_axes"
            };
            int cr,cg,cb,ca;
            for (int ci = 0; ci < NUM_USER_COLORS; ci++) {
                char dk[64], lt[64];
                snprintf(dk, sizeof(dk), "dark_%s", cnames[ci]);
                snprintf(lt, sizeof(lt), "light_%s", cnames[ci]);
                if (strcmp(key, dk) == 0 && sscanf(val, "%d %d %d %d", &cr,&cg,&cb,&ca) == 4)
                    app->dark_colors[ci] = (Color){cr, cg, cb, ca};
                if (strcmp(key, lt) == 0 && sscanf(val, "%d %d %d %d", &cr,&cg,&cb,&ca) == 4)
                    app->light_colors[ci] = (Color){cr, cg, cb, ca};
            }
            /* Per-theme species colors */
            int sc;
            if (sscanf(key, "dark_species_color_%d", &sc) == 1 && sc >= 0 && sc < NUM_SPECIES) {
                if (sscanf(val, "%d %d %d %d", &cr,&cg,&cb,&ca) == 4)
                    app->dark_species[sc] = (Color){cr, cg, cb, ca};
            }
            if (sscanf(key, "light_species_color_%d", &sc) == 1 && sc >= 0 && sc < NUM_SPECIES) {
                if (sscanf(val, "%d %d %d %d", &cr,&cg,&cb,&ca) == 4)
                    app->light_species[sc] = (Color){cr, cg, cb, ca};
            }
            /* Backward compat: old unprefixed colors go into both themes */
            for (int ci = 0; ci < NUM_USER_COLORS; ci++) {
                if (strcmp(key, cnames[ci]) == 0 && sscanf(val, "%d %d %d %d", &cr,&cg,&cb,&ca) == 4) {
                    app->dark_colors[ci] = (Color){cr, cg, cb, ca};
                    app->light_colors[ci] = (Color){cr, cg, cb, ca};
                }
            }
            if (sscanf(key, "species_color_%d", &sc) == 1 && sc >= 0 && sc < NUM_SPECIES) {
                if (sscanf(val, "%d %d %d %d", &cr,&cg,&cb,&ca) == 4) {
                    app->dark_species[sc] = (Color){cr, cg, cb, ca};
                    app->light_species[sc] = (Color){cr, cg, cb, ca};
                }
            }
        }
    }

    fclose(f);

    /* Load active colors from the current theme */
    if (app->dark_mode) {
        colors_from_array(app, app->dark_colors);
        memcpy(app->species_colors, app->dark_species, sizeof(app->species_colors));
    } else {
        colors_from_array(app, app->light_colors);
        memcpy(app->species_colors, app->light_species, sizeof(app->species_colors));
    }

    /* Clamp restored values */
    if (app->current_model < 0 || app->current_model >= FIELD_NUM_MODELS)
        app->current_model = 0;
    if (app->species < 0 || app->species > SPECIES_CUSTOM)
        app->species = SPECIES_PROTON;
    if (app->ui_edge < 0 || app->ui_edge > 1)
        app->ui_edge = 0;
    if (app->plots_edge < 0 || app->plots_edge > 1)
        app->plots_edge = 0;

    app_apply_theme(app);

    /* Run initial particle reset (needs_reset=2 for cam_follow_dist etc.),
     * then restore camera if it was saved in the state file. */
    int has_camera = (app->camera.position.x != 0.0f || app->camera.position.y != 0.0f
                   || app->camera.position.z != 0.0f);
    Vector3 cam_pos = app->camera.position;
    Vector3 cam_tgt = app->camera.target;
    float cam_fovy  = app->camera.fovy;
    app->needs_reset = 2;
    app_reset_particle(app);
    if (has_camera) {
        app->camera.position = cam_pos;
        app->camera.target = cam_tgt;
        app->camera.fovy = cam_fovy;
    }
}
#else
/* WASM: no persistent state */
void app_save_state(const AppState *app) { (void)app; }
void app_load_state(AppState *app) { (void)app; }
#endif

void app_reload_fonts(AppState *app)
{
    const LangInfo *li = i18n_lang_info(i18n_get_lang());
    char font_path[1100];
    int font_size = (int)(16 * app->dpi_scale);

    /* Build codepoint list: ASCII + Latin-1 + Latin Extended + Cyrillic + Greek/math */
    int codepoints[1200];
    int ncp = 0;
    for (int i = 32; i < 256; i++) codepoints[ncp++] = i;
    for (int i = 0x100; i <= 0x17F; i++) codepoints[ncp++] = i;
    for (int i = 0x180; i <= 0x24F; i++) codepoints[ncp++] = i;
    for (int i = 0x400; i <= 0x4FF; i++) codepoints[ncp++] = i;
    int greek[] = {0x03B1,0x03B2,0x03B3,0x03B5,0x03BA,0x03BB,0x03BC,0x03C0,0x03C6,0x03C9};
    for (int i = 0; i < 10; i++) codepoints[ncp++] = greek[i];
    int math[] = {0x2202,0x2207,0x00D7,0x2016,0x22A5,0x2081,0x2082,0x2080,0x00B2,0x00B3,
                  0x2212,0x00B7,0x00A0,0x00EA,0x00E2,0x221A,0x2098,0x2099,0x2083,0x2084};
    for (int i = 0; i < 20; i++) codepoints[ncp++] = math[i];

    /* For CJK: use the subsetted Noto font with explicit codepoints */
    if (li->needs_cjk && li->font_ui) {
        const int *cjk_cp = NULL;
        int n_cjk = 0;
        LangId lid = i18n_get_lang();
        if (lid == LANG_JA)    { cjk_cp = g_codepoints_ja;    n_cjk = N_CODEPOINTS_JA; }
        if (lid == LANG_ZH_CN) { cjk_cp = g_codepoints_zh_cn; n_cjk = N_CODEPOINTS_ZH_CN; }
        if (lid == LANG_ZH_TW) { cjk_cp = g_codepoints_zh_tw; n_cjk = N_CODEPOINTS_ZH_TW; }
        if (lid == LANG_KO)    { cjk_cp = g_codepoints_ko;    n_cjk = N_CODEPOINTS_KO; }
        snprintf(font_path, sizeof(font_path), "%s/fonts/%s", app->res_dir, li->font_ui);
        UnloadFont(app->font_ui);
        app->font_ui = LoadFontEx(font_path, font_size, (int *)cjk_cp, n_cjk);
    } else {
        snprintf(font_path, sizeof(font_path), "%s/fonts/Inter-Regular.ttf", app->res_dir);
        UnloadFont(app->font_ui);
        app->font_ui = LoadFontEx(font_path, font_size, codepoints, ncp);
    }

    /* Mono font stays SourceCodePro (readouts are ASCII/math) */
    snprintf(font_path, sizeof(font_path), "%s/fonts/SourceCodePro-Regular.ttf", app->res_dir);
    UnloadFont(app->font_mono);
    app->font_mono = LoadFontEx(font_path, font_size, codepoints, ncp);

    SetTextureFilter(app->font_ui.texture, TEXTURE_FILTER_BILINEAR);
    SetTextureFilter(app->font_mono.texture, TEXTURE_FILTER_BILINEAR);

    /* Update UI with new fonts */
    ui_init(app);
}

void app_shutdown(AppState *app)
{
    app_save_state(app);
    event_log_close(app);
    explorer_free(&app->explorer);
    if (app->recorder.active)
        recorder_stop(&app->recorder);
}
