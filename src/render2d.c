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

/* Draw a dashed data series (8px on, 4px off in screen space) */
static void draw_series_dashed(const struct AppState *app,
                               const double *t, const double *y, int count, int head, int cap,
                               int max_show, double tmin, double tmax, double ymin, double ymax,
                               float px, float py, float pw, float ph, Color color)
{
    (void)app;
    if (count < 2) return;
    int show = count < max_show ? count : max_show;

    int idx_new = (head - 1 + cap) % cap;
    int idx_old = (head - 1 - (show - 1) + cap) % cap;
    double dt_span = t[idx_new] - t[idx_old];
    double gap_thresh = (show > 1 && dt_span > 0) ? dt_span / show * 10.0 : 1e30;

    float accum = 0.0f;  /* accumulated screen-space distance */
    float dash_on = 8.0f, dash_off = 4.0f;
    float dash_cycle = dash_on + dash_off;

    for (int i = 1; i < show; i++) {
        int i0 = (head - 1 - (i - 1) + cap) % cap;
        int i1 = (head - 1 - i + cap) % cap;
        if (isnan(y[i0]) || isnan(y[i1])) continue;
        double dt = t[i0] - t[i1];
        if (dt < 0 || dt > gap_thresh) { accum = 0.0f; continue; }
        float x1 = map_x(px, pw, tmin, tmax, t[i0]);
        float y1f = map_y(py, ph, ymin, ymax, y[i0]);
        float x2 = map_x(px, pw, tmin, tmax, t[i1]);
        float y2f = map_y(py, ph, ymin, ymax, y[i1]);
        if (x1 < px && x2 < px) continue;
        if (x1 > px + pw && x2 > px + pw) continue;
        /* Skip segments that go outside the vertical plot area */
        if (y[i0] < ymin || y[i0] > ymax || y[i1] < ymin || y[i1] > ymax) continue;
        float dx = x2 - x1, dy = y2f - y1f;
        float seg_len = sqrtf(dx*dx + dy*dy);
        float phase = fmodf(accum, dash_cycle);
        if (phase < dash_on)
            DrawLineEx((Vector2){x1, y1f}, (Vector2){x2, y2f}, 2.0f, color);
        accum += seg_len;
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
    float margin_l = fmaxf(70 * pz, 50), margin_r = 4;
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

    /* GC traces (underlaid, dashed, darker) */
    if (app->show_gc_trajectory) {
        for (int pi = 0; pi < app->n_particles; pi++) {
            const DiagTimeSeries *di = &app->diags[pi];
            const double *gc_y = (field_sel == 0) ? di->gc_pitch_angle : di->gc_mu;
            int sp = app->particles[pi].species;
            if (sp < 0 || sp >= NUM_SPECIES) sp = NUM_SPECIES - 1;
            Color c = app->species_colors[sp];
            c.r = (unsigned char)(c.r * 0.55f);
            c.g = (unsigned char)(c.g * 0.55f);
            c.b = (unsigned char)(c.b * 0.55f);
            if (pi != sel) c.a = 60;
            draw_series_dashed(app, di->time, gc_y, di->count, di->head, di->capacity,
                               max_show, tmin, tmax, ymin, ymax, px, py, pw, ph, c);
        }
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
               (Vector2){px - bw.x - 7, py + ph - bw.y}, tsz, 1, th->text_dim);

    snprintf(buf, sizeof(buf), "%.3g", ymax);
    bw = MeasureTextEx(app->font_mono, buf, tsz, 1);
    DrawTextEx(app->font_mono, buf,
               (Vector2){px - bw.x - 7, py}, tsz, 1, th->text_dim);

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

static void draw_phase_panel(struct AppState *app, Rectangle bounds);

/* Width of "v_X (units)" label with Greek X rendered as subscript. */
static float v_label_width(Font font, const char *gk, const char *units, float fsz)
{
    float sub_fsz = fsz * 0.75f;
    Vector2 vsz = MeasureTextEx(font, "v", fsz, 1);
    Vector2 gsz = MeasureTextEx(font, gk, sub_fsz, 1);
    Vector2 usz = MeasureTextEx(font, units, fsz, 1);
    return vsz.x + gsz.x + usz.x;
}

/* Draw "v_X (units)" with Greek X subscripted, anchored at top-left (x,y). */
static void draw_v_label(Font font, const char *gk, const char *units,
                         float x, float y, float fsz, Color color)
{
    float sub_fsz = fsz * 0.75f;
    float sub_dy = fsz * 0.3f;
    Vector2 vsz = MeasureTextEx(font, "v", fsz, 1);
    DrawTextEx(font, "v", (Vector2){x, y}, fsz, 1, color);
    Vector2 gsz = MeasureTextEx(font, gk, sub_fsz, 1);
    DrawTextEx(font, gk, (Vector2){x + vsz.x, y + sub_dy},
               sub_fsz, 1, color);
    DrawTextEx(font, units, (Vector2){x + vsz.x + gsz.x, y},
               fsz, 1, color);
}

/* Draw "v_X (units)" rotated -90° (reading bottom→top), anchored at the
 * bottom-left of the rotated baseline (which is on the screen). */
static void draw_v_label_rot(Font font, const char *gk, const char *units,
                             float anchor_x, float anchor_y,
                             float fsz, Color color)
{
    float sub_fsz = fsz * 0.75f;
    float sub_dy = fsz * 0.3f;
    Vector2 vsz = MeasureTextEx(font, "v", fsz, 1);
    Vector2 gsz = MeasureTextEx(font, gk, sub_fsz, 1);
    /* Local (lx, ly) → screen (anchor_x + ly, anchor_y - lx) for -90° */
    DrawTextPro(font, "v", (Vector2){anchor_x, anchor_y},
                (Vector2){0, 0}, -90.0f, fsz, 1, color);
    DrawTextPro(font, gk,
                (Vector2){anchor_x + sub_dy, anchor_y - vsz.x},
                (Vector2){0, 0}, -90.0f, sub_fsz, 1, color);
    DrawTextPro(font, units,
                (Vector2){anchor_x, anchor_y - vsz.x - gsz.x},
                (Vector2){0, 0}, -90.0f, fsz, 1, color);
}

void render2d_draw(struct AppState *app)
{
    float sx, sy, sw, sh;
    app_scene_rect(app, &sx, &sy, &sw, &sh);
    float plot_h = app->win_h * app->plots_height_frac;
    /* plots_edge: 0=bottom (below scene), 1=top (above scene) */
    float y0 = (app->plots_edge == 1) ? 0.0f : sy + sh;

    /* Outer padding for symmetric visual margins on both ends of the strip */
    float outer_pad = 10.0f;
    float strip_x = sx + outer_pad;
    float strip_w = sw - 2 * outer_pad;
    if (strip_w < 100) strip_w = 100;

    /* Match the margin formulas in draw_panel_multi/draw_phase_panel
     * so the phase panel's inner data area is exactly square. */
    float pz = app->plot_zoom;
    float m_l = fmaxf(70 * pz, 50);
    float m_r = 4;
    float m_t = fmaxf(22 * pz, 18);
    float m_b = fmaxf(60 * pz, 50);
    float inner_h = plot_h - m_t - m_b;
    if (inner_h < 1) inner_h = 1;
    float phase_w = inner_h + m_l + m_r;
    if (phase_w > strip_w * 0.5f) phase_w = strip_w * 0.5f;
    float ts_w = (strip_w - phase_w) / 2.0f;
    if (ts_w < 50) ts_w = 50;

    int pitch_fixed = !app->pitch_autoscale;
    draw_panel_multi(app, TR(STR_PLOT_PITCH_ANGLE), "\xce\xb1 (\xc2\xb0)",
                     0, pitch_fixed, 0.0, 180.0,
                     (Rectangle){strip_x, y0, ts_w, plot_h});

    draw_panel_multi(app, TR(STR_PLOT_MAG_MOMENT), "\xce\xbc (J/T)",
                     1, 0, 0.0, 0.0,
                     (Rectangle){strip_x + ts_w, y0, ts_w, plot_h});

    draw_phase_panel(app,
                     (Rectangle){strip_x + 2 * ts_w, y0, phase_w, plot_h});
}

static void draw_phase_panel(struct AppState *app, Rectangle bounds)
{
    const Theme *th = &app->theme;
    float pz = app->plot_zoom;
    float margin_l = fmaxf(70 * pz, 50);
    float margin_r = 4;
    float margin_t = fmaxf(22 * pz, 18);
    float margin_b = fmaxf(60 * pz, 50);
    float px = bounds.x + margin_l;
    float py = bounds.y + margin_t;
    float pw = bounds.width - margin_l - margin_r;
    float ph = bounds.height - margin_t - margin_b;

    DrawRectangleRec(bounds, th->plot_bg);
    DrawRectangle((int)px, (int)py, (int)pw, (int)ph,
                  (Color){th->plot_bg.r-5, th->plot_bg.g-5, th->plot_bg.b-5, 255});
    DrawRectangleLines((int)px, (int)py, (int)pw, (int)ph, th->text_dim);

    /* Store panel and inner rects for hit testing */
    app->phase_rect[0] = bounds.x;
    app->phase_rect[1] = bounds.y;
    app->phase_rect[2] = bounds.width;
    app->phase_rect[3] = bounds.height;
    app->phase_inner[0] = px;
    app->phase_inner[1] = py;
    app->phase_inner[2] = pw;
    app->phase_inner[3] = ph;

    /* Greek letters on each axis based on swap state */
    const char *x_gk = app->phase_swap_axes ? "\xce\xba" : "\xcf\x84";
    const char *y_gk = app->phase_swap_axes ? "\xcf\x84" : "\xce\xba";

    float fsz = 14 * pz;
    float title_fsz = 14 * pz;
    /* Title: "v_<y> vs v_<x>" with subscripts */
    {
        float sub_fsz = title_fsz * 0.75f;
        float sub_dy = title_fsz * 0.3f;
        Vector2 vsz = MeasureTextEx(app->font_ui, "v", title_fsz, 1);
        Vector2 ksz = MeasureTextEx(app->font_ui, y_gk, sub_fsz, 1);
        Vector2 vssz = MeasureTextEx(app->font_ui, " vs ", title_fsz, 1);
        Vector2 tszg = MeasureTextEx(app->font_ui, x_gk, sub_fsz, 1);
        float total = 2*vsz.x + ksz.x + tszg.x + vssz.x;
        float tx = px + pw/2 - total/2;
        float ty = bounds.y + 4;
        DrawTextEx(app->font_ui, "v", (Vector2){tx, ty},
                   title_fsz, 1, th->text_dim);
        tx += vsz.x;
        DrawTextEx(app->font_ui, y_gk, (Vector2){tx, ty + sub_dy},
                   sub_fsz, 1, th->text_dim);
        tx += ksz.x;
        DrawTextEx(app->font_ui, " vs ", (Vector2){tx, ty},
                   title_fsz, 1, th->text_dim);
        tx += vssz.x;
        DrawTextEx(app->font_ui, "v", (Vector2){tx, ty},
                   title_fsz, 1, th->text_dim);
        tx += vsz.x;
        DrawTextEx(app->font_ui, x_gk, (Vector2){tx, ty + sub_dy},
                   sub_fsz, 1, th->text_dim);
    }

    /* Selected particle only */
    int sel = app->selected_particle;
    if (sel < 0 || sel >= app->n_particles) return;
    const DiagTimeSeries *d = &app->diags[sel];

    int max_show_table[] = {480, 4800, 48000};
    int pr = app->plot_range;
    if (pr < 0) pr = 0;
    if (pr > 2) pr = 2;
    int max_show = max_show_table[pr];
    if (max_show > d->capacity) max_show = d->capacity;
    int show = d->count < max_show ? d->count : max_show;
    if (show < 2) return;

    /* Symmetric range: max |v_kappa|, |v_tau| over the window */
    double absmax = 0;
    for (int i = 0; i < show; i++) {
        int idx = (d->head - 1 - i + d->capacity) % d->capacity;
        if (isnan(d->v_kappa[idx]) || isnan(d->v_tau[idx])) continue;
        double a = fabs(d->v_kappa[idx]);
        double b = fabs(d->v_tau[idx]);
        if (a > absmax) absmax = a;
        if (b > absmax) absmax = b;
    }
    if (!(absmax > 0.0)) absmax = 1.0;
    /* x-axis: v_τ; y-axis: v_κ. Both share symmetric range. */
    double ymin = -absmax * 1.05, ymax = absmax * 1.05;
    double xmin = ymin, xmax = ymax;

    /* Y-axis label rotated 90° */
    {
        float total_w = v_label_width(app->font_ui, y_gk, " (m/s)", fsz);
        float anchor_y = py + ph/2 + total_w/2;
        draw_v_label_rot(app->font_ui, y_gk, " (m/s)",
                         px - 50, anchor_y, fsz, th->text_dim);
    }

    /* Trajectory (selected particle only). Apply swap and flip flags. */
    int sp = app->particles[sel].species;
    if (sp < 0 || sp >= NUM_SPECIES) sp = NUM_SPECIES - 1;
    Color c = app->species_colors[sp];

    int idx_new = (d->head - 1 + d->capacity) % d->capacity;
    int idx_old = (d->head - 1 - (show - 1) + d->capacity) % d->capacity;
    double dt_span = d->time[idx_new] - d->time[idx_old];
    double gap_thresh = (show > 1 && dt_span > 0)
                        ? dt_span / show * 10.0 : 1e30;

    double sx_sign = app->phase_flip_x ? -1.0 : 1.0;
    double sy_sign = app->phase_flip_y ? -1.0 : 1.0;

    for (int i = 1; i < show; i++) {
        int i0 = (d->head - 1 - (i - 1) + d->capacity) % d->capacity;
        int i1 = (d->head - 1 - i + d->capacity) % d->capacity;
        if (isnan(d->v_kappa[i0]) || isnan(d->v_kappa[i1])) continue;
        if (isnan(d->v_tau[i0]) || isnan(d->v_tau[i1])) continue;
        double dt = d->time[i0] - d->time[i1];
        if (dt < 0 || dt > gap_thresh) continue;
        /* Default: x = v_τ, y = v_κ. Swap if requested. */
        double xv0 = app->phase_swap_axes ? d->v_kappa[i0] : d->v_tau[i0];
        double yv0 = app->phase_swap_axes ? d->v_tau[i0]   : d->v_kappa[i0];
        double xv1 = app->phase_swap_axes ? d->v_kappa[i1] : d->v_tau[i1];
        double yv1 = app->phase_swap_axes ? d->v_tau[i1]   : d->v_kappa[i1];
        xv0 *= sx_sign; yv0 *= sy_sign;
        xv1 *= sx_sign; yv1 *= sy_sign;
        float x1 = map_x(px, pw, xmin, xmax, xv0);
        float y1 = map_y(py, ph, ymin, ymax, yv0);
        float x2 = map_x(px, pw, xmin, xmax, xv1);
        float y2 = map_y(py, ph, ymin, ymax, yv1);
        DrawLineEx((Vector2){x1, y1}, (Vector2){x2, y2}, 2.0f, c);
    }

    /* Direction arrow (triangle) at the current point. Direction taken from
     * the last drawn segment so the arrow's centerline matches the local
     * trail tangent (avoids drift on curving trajectories). Falls back to
     * earlier samples if consecutive ones are too close in screen space. */
    {
        int i_now = (d->head - 1 + d->capacity) % d->capacity;
        int max_back = show - 1;
        if (max_back > 8) max_back = 8;
        float dx = 0, dy = 0, mag = 0;
        float sx_now = 0, sy_now = 0;
        int found = 0;

        if (!isnan(d->v_kappa[i_now]) && !isnan(d->v_tau[i_now])) {
            double xv = (app->phase_swap_axes ? d->v_kappa[i_now]
                                              : d->v_tau[i_now]) * sx_sign;
            double yv = (app->phase_swap_axes ? d->v_tau[i_now]
                                              : d->v_kappa[i_now]) * sy_sign;
            sx_now = map_x(px, pw, xmin, xmax, xv);
            sy_now = map_y(py, ph, ymin, ymax, yv);

            for (int back = 1; back <= max_back; back++) {
                int i_old = (d->head - 1 - back + d->capacity) % d->capacity;
                if (isnan(d->v_kappa[i_old]) || isnan(d->v_tau[i_old]))
                    continue;
                double xv_o = (app->phase_swap_axes ? d->v_kappa[i_old]
                                                    : d->v_tau[i_old]) * sx_sign;
                double yv_o = (app->phase_swap_axes ? d->v_tau[i_old]
                                                    : d->v_kappa[i_old]) * sy_sign;
                float sx_old = map_x(px, pw, xmin, xmax, xv_o);
                float sy_old = map_y(py, ph, ymin, ymax, yv_o);
                dx = sx_now - sx_old;
                dy = sy_now - sy_old;
                mag = sqrtf(dx*dx + dy*dy);
                if (mag > 1.0f) { found = 1; break; }
            }
        }

        if (found) {
            float dirx = dx / mag, diry = dy / mag;
            float perpx = -diry, perpy = dirx;
            float arrow_len = 10.0f;
            float arrow_half_w = 5.0f;
            Vector2 tip = {sx_now, sy_now};
            Vector2 base_l = {sx_now - dirx*arrow_len + perpx*arrow_half_w,
                              sy_now - diry*arrow_len + perpy*arrow_half_w};
            Vector2 base_r = {sx_now - dirx*arrow_len - perpx*arrow_half_w,
                              sy_now - diry*arrow_len - perpy*arrow_half_w};
            DrawTriangle(tip, base_l, base_r, c);
            DrawTriangle(tip, base_r, base_l, c);
        }
    }

    /* Zero crosshair */
    {
        float x0 = map_x(px, pw, xmin, xmax, 0.0);
        float ycen = map_y(py, ph, ymin, ymax, 0.0);
        Color gc = th->text_dim;
        gc.a = 60;
        DrawLine((int)px, (int)ycen, (int)(px + pw), (int)ycen, gc);
        DrawLine((int)x0, (int)py, (int)x0, (int)(py + ph), gc);
    }

    /* Tick labels — match displayed direction (flip swaps the labels) */
    float tsz = 11 * pz;
    char buf[32];
    double y_top    = app->phase_flip_y ? -absmax :  absmax;
    double y_bottom = app->phase_flip_y ?  absmax : -absmax;
    double x_left   = app->phase_flip_x ?  absmax : -absmax;
    double x_right  = app->phase_flip_x ? -absmax :  absmax;

    snprintf(buf, sizeof(buf), "%+.2g", y_bottom);
    Vector2 bw0 = MeasureTextEx(app->font_mono, buf, tsz, 1);
    DrawTextEx(app->font_mono, buf,
               (Vector2){px - bw0.x - 7, py + ph - bw0.y},
               tsz, 1, th->text_dim);
    snprintf(buf, sizeof(buf), "%+.2g", y_top);
    Vector2 bw1 = MeasureTextEx(app->font_mono, buf, tsz, 1);
    DrawTextEx(app->font_mono, buf,
               (Vector2){px - bw1.x - 7, py},
               tsz, 1, th->text_dim);

    float tick_y = py + ph + 3;
    snprintf(buf, sizeof(buf), "%+.2g", x_left);
    DrawTextEx(app->font_mono, buf,
               (Vector2){px, tick_y}, tsz, 1, th->text_dim);
    snprintf(buf, sizeof(buf), "%+.2g", x_right);
    Vector2 bw = MeasureTextEx(app->font_mono, buf, tsz, 1);
    DrawTextEx(app->font_mono, buf,
               (Vector2){px + pw - bw.x, tick_y},
               tsz, 1, th->text_dim);

    /* Tick marks */
    DrawLine((int)px, (int)(py + ph), (int)px, (int)(py + ph + 3),
             th->text_dim);
    DrawLine((int)(px + pw), (int)(py + ph), (int)(px + pw), (int)(py + ph + 3),
             th->text_dim);

    /* X-axis label */
    {
        float total_w = v_label_width(app->font_ui, x_gk, " (m/s)", tsz);
        float xlx = px + pw/2 - total_w/2;
        float xly = tick_y + tsz + 2;
        draw_v_label(app->font_ui, x_gk, " (m/s)",
                     xlx, xly, tsz, th->text_dim);
    }
}
