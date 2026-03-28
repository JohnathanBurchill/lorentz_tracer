#include "render2d.h"
#include "i18n.h"
#include "app.h"
#include <math.h>
#include <stdio.h>

static float map_x(float px, float pw, double xmin, double xmax, double xv)
{
    if (xmax - xmin < 1e-30) return px;
    return px + (float)((xv - xmin) / (xmax - xmin)) * pw;
}

static float map_y(float py, float ph, double ymin, double ymax, double yv)
{
    if (ymax - ymin < 1e-30) return py + ph;
    return py + ph - (float)((yv - ymin) / (ymax - ymin)) * ph;
}

/* Draw a single data series on the plot area */
static void draw_series(const struct AppState *app,
                        const double *t, const double *y, int count, int head, int cap,
                        int max_show, double tmin, double tmax, double ymin, double ymax,
                        float px, float py, float pw, float ph, Color color)
{
    (void)app;
    if (count < 2) return;
    int show = count < max_show ? count : max_show;

    /* Estimate typical sample interval for gap detection */
    int idx_new = (head - 1 + cap) % cap;
    int idx_old = (head - 1 - (show - 1) + cap) % cap;
    double dt_span = t[idx_new] - t[idx_old];
    double gap_thresh = (show > 1 && dt_span > 0) ? dt_span / show * 10.0 : 1e30;

    for (int i = 1; i < show; i++) {
        int i0 = (head - 1 - (i - 1) + cap) % cap;
        int i1 = (head - 1 - i + cap) % cap;
        if (isnan(y[i0]) || isnan(y[i1])) continue;
        /* Skip if time goes backward or has a large gap (reset) */
        double dt = t[i0] - t[i1];
        if (dt < 0 || dt > gap_thresh) continue;
        float x1 = map_x(px, pw, tmin, tmax, t[i0]);
        float y1 = map_y(py, ph, ymin, ymax, y[i0]);
        float x2 = map_x(px, pw, tmin, tmax, t[i1]);
        float y2 = map_y(py, ph, ymin, ymax, y[i1]);
        /* Clip to plot area */
        if (x1 < px && x2 < px) continue;
        if (x1 > px + pw && x2 > px + pw) continue;
        DrawLineEx((Vector2){x1, y1}, (Vector2){x2, y2}, 2.0f, color);
    }
}

/* Draw a complete plot panel with axes, labels, and multiple particle series.
 * field_sel: 0=pitch_angle, 1=mu */
static void draw_panel_multi(const struct AppState *app,
                             const char *title, const char *ylabel,
                             int field_sel,
                             int fixed_yrange, double fixed_ymin, double fixed_ymax,
                             Rectangle bounds)
{
    const Theme *th = &app->theme;
    float pz = app->plot_zoom;
    float margin_l = fmaxf(70 * pz, 50), margin_r = 10;
    float margin_t = fmaxf(22 * pz, 18), margin_b = fmaxf(60 * pz, 50);
    float px = bounds.x + margin_l;
    float py = bounds.y + margin_t;
    float pw = bounds.width - margin_l - margin_r;
    float ph = bounds.height - margin_t - margin_b;

    DrawRectangleRec(bounds, th->plot_bg);
    DrawRectangle((int)px, (int)py, (int)pw, (int)ph,
                  (Color){th->plot_bg.r - 5, th->plot_bg.g - 5, th->plot_bg.b - 5, 255});
    DrawRectangleLines((int)px, (int)py, (int)pw, (int)ph, th->text_dim);

    float fsz = 14 * pz;
    float title_fsz = 14 * pz;

    /* Title centered above plot */
    Vector2 tw = MeasureTextEx(app->font_ui, title, title_fsz, 1);
    DrawTextEx(app->font_ui, title,
               (Vector2){px + pw/2 - tw.x/2, bounds.y + 4}, title_fsz, 1, th->text_dim);

    /* Y-label rotated 90° */
    {
        Vector2 yw = MeasureTextEx(app->font_ui, ylabel, fsz, 1);
        float label_x = px - 50;
        float label_y = py + ph/2 + yw.x/2;
        DrawTextPro(app->font_ui, ylabel,
                    (Vector2){label_x, label_y},
                    (Vector2){0, 0}, -90.0f, fsz, 1, th->text_dim);
    }

    /* Compute time and y ranges from the selected particle */
    int sel = app->selected_particle;
    const DiagTimeSeries *d = &app->diags[sel];

    int max_show_table[] = {480, 4800, 48000};
    int pr = app->plot_range;
    if (pr < 0) pr = 0;
    if (pr > 2) pr = 2;
    int max_show = max_show_table[pr];
    if (max_show > d->capacity) max_show = d->capacity;
    int show_count = d->count < max_show ? d->count : max_show;

    if (show_count < 2) return;

    /* Time axis from selected particle */
    int idx_newest = (d->head - 1 + d->capacity) % d->capacity;
    int idx_oldest = (d->head - 1 - (d->count - 1) + d->capacity) % d->capacity;
    double t_newest = d->time[idx_newest];
    double t_oldest = d->time[idx_oldest];

    double tmin, tmax;
    if (d->count < max_show) {
        tmin = 0.0;
        double dt_per_sample = (d->count > 1) ? (t_newest - t_oldest) / (d->count - 1) : 1.0;
        tmax = dt_per_sample * (max_show - 1);
        if (tmax < t_newest) tmax = t_newest * 1.1;
    } else {
        int idx_win_oldest = (d->head - 1 - (max_show - 1) + d->capacity) % d->capacity;
        tmin = d->time[idx_win_oldest];
        tmax = t_newest;
    }
    if (tmax - tmin < 1e-30) tmax = tmin + 1.0;

    /* Y range */
    double ymin, ymax;
    const double *sel_y = (field_sel == 0) ? d->pitch_angle : d->mu;
    if (fixed_yrange) {
        ymin = fixed_ymin;
        ymax = fixed_ymax;
    } else {
        double lo = 1e30, hi = -1e30;
        /* Scan all particles for global y range */
        for (int pi = 0; pi < app->n_particles; pi++) {
            const DiagTimeSeries *di = &app->diags[pi];
            const double *yi = (field_sel == 0) ? di->pitch_angle : di->mu;
            int cnt = di->count < max_show ? di->count : max_show;
            for (int i = 0; i < cnt; i++) {
                int idx = (di->head - 1 - i + di->capacity) % di->capacity;
                if (isnan(yi[idx])) continue;
                if (yi[idx] < lo) lo = yi[idx];
                if (yi[idx] > hi) hi = yi[idx];
            }
        }
        double yrange = hi - lo;
        double ypad;
        if (yrange > 0.0) {
            ypad = yrange * 0.05;
        } else {
            double mag = fabs(hi) + fabs(lo);
            ypad = (mag > 0.0) ? mag * 0.1 : 1.0;
        }
        ymin = lo - ypad;
        ymax = hi + ypad;
        if (lo >= 0.0 && ymin < 0.0) ymin = 0.0;
    }

    /* Draw background particles first (dimmed) */
    for (int pi = 0; pi < app->n_particles; pi++) {
        if (pi == sel) continue;
        const DiagTimeSeries *di = &app->diags[pi];
        const double *yi = (field_sel == 0) ? di->pitch_angle : di->mu;
        int sp = app->particles[pi].species;
        if (sp < 0 || sp >= NUM_SPECIES) sp = NUM_SPECIES - 1;
        Color c = app->species_colors[sp];
        c.a = 60;  /* dim background traces */
        draw_series(app, di->time, yi, di->count, di->head, di->capacity,
                    max_show, tmin, tmax, ymin, ymax, px, py, pw, ph, c);
    }

    /* Draw selected particle on top */
    {
        int sp = app->particles[sel].species;
        if (sp < 0 || sp >= NUM_SPECIES) sp = NUM_SPECIES - 1;
        Color c = app->species_colors[sp];
        draw_series(app, d->time, sel_y, d->count, d->head, d->capacity,
                    max_show, tmin, tmax, ymin, ymax, px, py, pw, ph, c);
    }

    /* Y-axis tick labels */
    float tsz = 11 * pz;
    char buf[32];

    snprintf(buf, sizeof(buf), "%.3g", ymin);
    Vector2 bw = MeasureTextEx(app->font_mono, buf, tsz, 1);
    DrawTextEx(app->font_mono, buf,
               (Vector2){px - bw.x - 4, py + ph - bw.y}, tsz, 1, th->text_dim);

    snprintf(buf, sizeof(buf), "%.3g", ymax);
    bw = MeasureTextEx(app->font_mono, buf, tsz, 1);
    DrawTextEx(app->font_mono, buf,
               (Vector2){px - bw.x - 4, py}, tsz, 1, th->text_dim);

    /* X-axis tick labels */
    float tick_y = py + ph + 3;

    snprintf(buf, sizeof(buf), "%.3g", tmin);
    DrawTextEx(app->font_mono, buf,
               (Vector2){px, tick_y}, tsz, 1, th->text_dim);

    snprintf(buf, sizeof(buf), "%.3g", tmax);
    bw = MeasureTextEx(app->font_mono, buf, tsz, 1);
    DrawTextEx(app->font_mono, buf,
               (Vector2){px + pw - bw.x, tick_y}, tsz, 1, th->text_dim);

    /* Tick marks */
    DrawLine((int)px, (int)(py + ph), (int)px, (int)(py + ph + 3), th->text_dim);
    DrawLine((int)(px + pw), (int)(py + ph), (int)(px + pw), (int)(py + ph + 3), th->text_dim);

    /* X-axis label below tick labels */
    Vector2 tlw = MeasureTextEx(app->font_ui, TR(STR_PLOT_TIME), tsz, 1);
    DrawTextEx(app->font_ui, TR(STR_PLOT_TIME),
               (Vector2){px + pw/2 - tlw.x/2, tick_y + tsz + 2}, tsz, 1, th->text_dim);
}

void render2d_draw(const struct AppState *app)
{
    float sx, sy, sw, sh;
    app_scene_rect(app, &sx, &sy, &sw, &sh);
    float plot_h = app->win_h * app->plots_height_frac;
    float plot_w = sw / 2.0f;
    /* plots_edge: 0=bottom (below scene), 1=top (above scene) */
    float y0 = (app->plots_edge == 1) ? 0.0f : sy + sh;

    int pitch_fixed = !app->pitch_autoscale;
    draw_panel_multi(app, TR(STR_PLOT_PITCH_ANGLE), "\xce\xb1 (\xc2\xb0)",
                     0, pitch_fixed, 0.0, 180.0,
                     (Rectangle){sx, y0, plot_w, plot_h});

    draw_panel_multi(app, TR(STR_PLOT_MAG_MOMENT), "\xce\xbc (J/T)",
                     1, 0, 0.0, 0.0,
                     (Rectangle){sx + plot_w, y0, plot_w, plot_h});
}
