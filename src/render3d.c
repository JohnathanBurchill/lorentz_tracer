#include "render3d.h"
#include "app.h"
#include "nax.h"
#include "rlgl.h"
#include <math.h>
#include <string.h>

/* Defined in field_stellarator.c */
extern const NAXConfig *stellarator_get_nax(void);

static Vector3 v3(Vec3 v) { return (Vector3){(float)v.x, (float)v.y, (float)v.z}; }

static float g_arrow_head_size = 0.012f;
static int g_arrow_lw = 1;
static Vec3 g_cam_pos;     /* camera position for distance-based offset */
static float g_px;         /* world-space size of 1 pixel at distance 1 */

/* Draw a single line segment with multi-pass offset for thickness > 1.
 * Each pass is 1 pixel apart on screen (constant screen-space width). */
static void draw_thick_line(Vec3 a, Vec3 b, int width, Color c)
{
    DrawLine3D(v3(a), v3(b), c);
    if (width <= 1) return;
    Vec3 mid = vec3_scale(0.5, vec3_add(a, b));
    Vec3 view_dir = vec3_sub(mid, g_cam_pos);
    float dist = (float)vec3_len(view_dir);
    Vec3 line_dir = vec3_sub(b, a);
    Vec3 offset_dir = vec3_cross(line_dir, view_dir);
    double olen = vec3_len(offset_dir);
    if (olen < 1e-12) return;
    offset_dir = vec3_scale(1.0 / olen, offset_dir);
    for (int k = 1; k < width; k++) {
        Vec3 da = vec3_scale(k * 0.5 * g_px * dist, offset_dir);
        DrawLine3D(v3(vec3_add(a, da)), v3(vec3_add(b, da)), c);
        DrawLine3D(v3(vec3_sub(a, da)), v3(vec3_sub(b, da)), c);
    }
}

/* Draw a line strip with thickness. Offset per segment scales with distance. */
static void draw_thick_strip(Vec3 *pts, int n, int stride, int width, Color c)
{
    rlBegin(RL_LINES);
    int prev = 0;
    for (int i = stride; i < n; i += stride) {
        rlColor4ub(c.r, c.g, c.b, c.a);
        rlVertex3f((float)pts[prev].x, (float)pts[prev].y, (float)pts[prev].z);
        rlVertex3f((float)pts[i].x, (float)pts[i].y, (float)pts[i].z);
        if (width > 1) {
            Vec3 seg_dir = vec3_sub(pts[i], pts[prev]);
            Vec3 view_dir = vec3_sub(pts[prev], g_cam_pos);
            float dist = (float)vec3_len(view_dir);
            Vec3 offset_dir = vec3_cross(seg_dir, view_dir);
            double olen = vec3_len(offset_dir);
            if (olen > 1e-12) {
                offset_dir = vec3_scale(1.0 / olen, offset_dir);
                for (int k = 1; k < width; k++) {
                    Vec3 da = vec3_scale(k * 0.5 * g_px * dist, offset_dir);
                    rlColor4ub(c.r, c.g, c.b, c.a);
                    rlVertex3f((float)(pts[prev].x+da.x), (float)(pts[prev].y+da.y), (float)(pts[prev].z+da.z));
                    rlVertex3f((float)(pts[i].x+da.x), (float)(pts[i].y+da.y), (float)(pts[i].z+da.z));
                    rlColor4ub(c.r, c.g, c.b, c.a);
                    rlVertex3f((float)(pts[prev].x-da.x), (float)(pts[prev].y-da.y), (float)(pts[prev].z-da.z));
                    rlVertex3f((float)(pts[i].x-da.x), (float)(pts[i].y-da.y), (float)(pts[i].z-da.z));
                }
            }
        }
        prev = i;
    }
    /* Always include the final point */
    if (prev < n - 1) {
        int e = n - 1;
        rlColor4ub(c.r, c.g, c.b, c.a);
        rlVertex3f((float)pts[prev].x, (float)pts[prev].y, (float)pts[prev].z);
        rlVertex3f((float)pts[e].x, (float)pts[e].y, (float)pts[e].z);
        if (width > 1) {
            Vec3 seg_dir = vec3_sub(pts[e], pts[prev]);
            Vec3 view_dir = vec3_sub(pts[prev], g_cam_pos);
            float dist = (float)vec3_len(view_dir);
            Vec3 offset_dir = vec3_cross(seg_dir, view_dir);
            double olen = vec3_len(offset_dir);
            if (olen > 1e-12) {
                offset_dir = vec3_scale(1.0 / olen, offset_dir);
                for (int k = 1; k < width; k++) {
                    Vec3 da = vec3_scale(k * 0.5 * g_px * dist, offset_dir);
                    rlColor4ub(c.r, c.g, c.b, c.a);
                    rlVertex3f((float)(pts[prev].x+da.x), (float)(pts[prev].y+da.y), (float)(pts[prev].z+da.z));
                    rlVertex3f((float)(pts[e].x+da.x), (float)(pts[e].y+da.y), (float)(pts[e].z+da.z));
                    rlColor4ub(c.r, c.g, c.b, c.a);
                    rlVertex3f((float)(pts[prev].x-da.x), (float)(pts[prev].y-da.y), (float)(pts[prev].z-da.z));
                    rlVertex3f((float)(pts[e].x-da.x), (float)(pts[e].y-da.y), (float)(pts[e].z-da.z));
                }
            }
        }
    }
    rlEnd();
}

static void draw_arrow(Vec3 from, Vec3 to, Color c)
{
    Vec3 dir = vec3_sub(to, from);
    double len = vec3_len(dir);
    if (len < 1e-10) return;
    Vec3 dhat = vec3_scale(1.0 / len, dir);
    double cone_h = g_arrow_head_size;
    double cone_r = g_arrow_head_size * 0.3;
    if (cone_h > len) cone_h = len;
    double shaft_len = len - cone_h;
    Vec3 cone_base = vec3_add(from, vec3_scale(shaft_len, dhat));
    /* Subdivide shaft so each segment gets correct per-distance offset */
    int n_segs = (int)(shaft_len / 0.02) + 1;
    if (n_segs < 1) n_segs = 1;
    if (n_segs > 200) n_segs = 200;
    if (n_segs == 1 || g_arrow_lw <= 1) {
        draw_thick_line(from, cone_base, g_arrow_lw, c);
    } else {
        Vec3 pts[201];
        for (int i = 0; i <= n_segs; i++)
            pts[i] = vec3_add(from, vec3_scale(shaft_len * i / n_segs, dhat));
        draw_thick_strip(pts, n_segs + 1, 1, g_arrow_lw, c);
    }
    DrawCylinderEx(v3(cone_base), v3(to), (float)cone_r, 0.0f, 8, c);
}

/* Trace a single field line from seed by integrating ds along B. */
static int trace_field_line(const FieldModel *fm, Vec3 seed, double ds,
                            int max_steps, Vec3 *pts, int max_pts)
{
    int n = 0;
    Vec3 pos = seed;
    if (n < max_pts) pts[n++] = pos;

    for (int i = 0; i < max_steps && n < max_pts; i++) {
        Vec3 B = fm->eval_B(fm->params, pos);
        double Bmag = vec3_len(B);
        if (Bmag < 1e-15) break;
        Vec3 bhat = vec3_scale(1.0 / Bmag, B);
        pos = vec3_add(pos, vec3_scale(ds, bhat));
        pts[n++] = pos;
        if (vec3_len(pos) > 100.0) break;
    }
    return n;
}

/* Trace field line with a maximum arc length instead of step count. */
static int trace_field_line_len(const FieldModel *fm, Vec3 seed, double ds,
                                double max_len, Vec3 *pts, int max_pts)
{
    int n = 0;
    Vec3 pos = seed;
    if (n < max_pts) pts[n++] = pos;
    double arc = 0.0;
    double abs_ds = fabs(ds);

    while (arc < max_len && n < max_pts) {
        Vec3 B = fm->eval_B(fm->params, pos);
        double Bmag = vec3_len(B);
        if (Bmag < 1e-15) break;
        Vec3 bhat = vec3_scale(1.0 / Bmag, B);
        pos = vec3_add(pos, vec3_scale(ds, bhat));
        pts[n++] = pos;
        arc += abs_ds;
        if (vec3_len(pos) > 100.0) break;
    }
    return n;
}

/* Camera distance from target */
static float cam_dist(const struct AppState *app)
{
    float dx = app->camera.position.x - app->camera.target.x;
    float dy = app->camera.position.y - app->camera.target.y;
    float dz = app->camera.position.z - app->camera.target.z;
    return sqrtf(dx*dx + dy*dy + dz*dz);
}

/* ---- Field line cache ---- */
#define FL_MAX_PTS      2048
#define FL_MAX_LINES    40
#define FL_MAX_TOTAL    (FL_MAX_LINES * FL_MAX_PTS)

static Vec3 fl_cache[FL_MAX_TOTAL];
static int fl_line_start[FL_MAX_LINES]; /* index into fl_cache */
static int fl_line_count[FL_MAX_LINES]; /* number of points per line */
static int fl_num_lines = 0;
static int fl_cached_model = -1;
static float fl_cached_dist = -1.0f;
static double fl_cached_params[FIELD_MAX_PARAMS];

static void fl_cache_clear(void)
{
    fl_num_lines = 0;
}

static void fl_cache_add_line(const Vec3 *pts, int n)
{
    if (fl_num_lines >= FL_MAX_LINES || n < 2) return;
    int offset = 0;
    if (fl_num_lines > 0)
        offset = fl_line_start[fl_num_lines - 1] + fl_line_count[fl_num_lines - 1];
    if (offset + n > FL_MAX_TOTAL) return;
    memcpy(&fl_cache[offset], pts, n * sizeof(Vec3));
    fl_line_start[fl_num_lines] = offset;
    fl_line_count[fl_num_lines] = n;
    fl_num_lines++;
}

static int fl_needs_rebuild(const struct AppState *app)
{
    if (fl_cached_model != app->current_model) return 1;
    const FieldModel *fm = &app->models[app->current_model];
    for (int i = 0; i < fm->n_params; i++) {
        if (fl_cached_params[i] != fm->params[i]) return 1;
    }
    if (fl_cached_model < 0) return 1;
    return 0;
}

static void fl_mark_cached(const struct AppState *app)
{
    fl_cached_model = app->current_model;
    fl_cached_dist = cam_dist(app);
    const FieldModel *fm = &app->models[app->current_model];
    for (int i = 0; i < FIELD_MAX_PARAMS; i++)
        fl_cached_params[i] = fm->params[i];
}

static void fl_draw(Color c, float view_dist, int width)
{
    int stride = (int)(view_dist / 20.0f);
    if (stride < 1) stride = 1;

    for (int l = 0; l < fl_num_lines; l++) {
        int off = fl_line_start[l];
        int n = fl_line_count[l];
        draw_thick_strip(&fl_cache[off], n, stride, width, c);
    }
}

static void build_field_lines_generic(const FieldModel *fm,
                                      const Vec3 *seeds, int n_seeds,
                                      double ds, int steps)
{
    static Vec3 pts[FL_MAX_PTS];
    if (steps > FL_MAX_PTS / 2) steps = FL_MAX_PTS / 2;

    for (int s = 0; s < n_seeds; s++) {
        int n = trace_field_line(fm, seeds[s], ds, steps, pts, FL_MAX_PTS);
        fl_cache_add_line(pts, n);
        n = trace_field_line(fm, seeds[s], -ds, steps, pts, FL_MAX_PTS);
        fl_cache_add_line(pts, n);
    }
}

static void build_field_lines(const struct AppState *app)
{
    const FieldModel *fm = &app->models[app->current_model];
    int model = app->current_model;

    fl_cache_clear();

    if (model == 0) {
        /* Circular B: analytic circles */
        int nlines = 6;
        int npts = 256;
        static Vec3 circle[257];
        for (int j = 0; j < nlines; j++) {
            double r = 2.0 + j * 1.5;
            for (int i = 0; i <= npts; i++) {
                double a = 2.0 * M_PI * i / npts;
                circle[i] = (Vec3){r * cos(a), r * sin(a), 0.0};
            }
            fl_cache_add_line(circle, npts + 1);
        }
    } else if (model == 1 || model == 2 || model == 3 || model == 4) {
        Vec3 seeds[16];
        int ns = 0;
        for (int ix = -1; ix <= 1; ix++)
            for (int iy = -1; iy <= 1; iy++)
                if (ns < 16)
                    seeds[ns++] = (Vec3){ix * 2.0, iy * 2.0, -5.0};
        build_field_lines_generic(fm, seeds, ns, 0.05, 800);
    } else if (model == 5) {
        Vec3 seeds[12];
        int ns = 0;
        for (int i = 0; i < 6; i++) {
            double r = 3.0 + i * 1.5;
            double angle = i * M_PI / 6.0;
            seeds[ns++] = (Vec3){r * cos(angle), r * sin(angle), 0.0};
        }
        for (int i = 0; i < 6; i++) {
            double r = 3.0 + i * 1.5;
            double angle = M_PI + i * M_PI / 6.0;
            seeds[ns++] = (Vec3){r * cos(angle), r * sin(angle), 0.0};
        }
        build_field_lines_generic(fm, seeds, ns, 0.05, 1000);
    } else if (model == 6) {
        Vec3 seeds[12];
        int ns = 0;
        for (int i = 0; i < 8; i++) {
            double angle = 2.0 * M_PI * i / 8.0;
            double r = 0.5 + (i % 3) * 0.5;
            seeds[ns++] = (Vec3){r * cos(angle), r * sin(angle), -3.0};
        }
        seeds[ns++] = (Vec3){0.0, 0.0, -5.0};
        build_field_lines_generic(fm, seeds, ns, 0.05, 1000);
    } else if (model == 7) {
        double R0 = fm->params[1];
        double a = fm->params[3];
        Vec3 seeds[8];
        int ns = 0;
        for (int i = 0; i < 4; i++) {
            double r_off = a * (0.2 + 0.2 * i);
            seeds[ns++] = (Vec3){R0 + r_off, 0.0, 0.0};
            seeds[ns++] = (Vec3){R0 - r_off, 0.0, 0.0};
        }
        build_field_lines_generic(fm, seeds, ns, 0.05, 2000);
    } else if (model == 8) {
        /* NAX stellarator: draw the 3D magnetic axis as a bright line,
         * then trace field lines from seed points offset from the axis.
         * Force NAX init by evaluating B if not yet initialized. */
        const NAXConfig *nax = stellarator_get_nax();
        if (!nax) {
            fm->eval_B(fm->params, fm->default_pos);
            nax = stellarator_get_nax();
        }
        if (nax) {
            /* Draw magnetic axis */
            int naxis = 256;
            static Vec3 axis_pts[257];
            for (int i = 0; i <= naxis; i++) {
                double phi = 2.0 * M_PI * i / naxis;
                Vec3 p = axis_pos(&nax->af, phi);
                axis_pts[i] = vec3_scale(nax->R0_phys, p);
            }
            fl_cache_add_line(axis_pts, naxis + 1);

            /* Seed field lines at several toroidal positions, offset in
             * the Frenet normal and binormal directions */
            double a = fm->params[3];
            int n_toroidal = 8;
            double offsets[] = {0.3, 0.6};
            int n_off = 2;
            Vec3 seeds[40];
            int ns = 0;
            for (int it = 0; it < n_toroidal && ns < 36; it++) {
                double phi = 2.0 * M_PI * it / n_toroidal;
                Vec3 r0, tg, nm, bn;
                double kk, tt, dll;
                axis_frenet(&nax->af, phi, &r0, &tg, &nm, &bn, &kk, &tt, &dll);
                r0 = vec3_scale(nax->R0_phys, r0);
                for (int io = 0; io < n_off && ns < 38; io++) {
                    double off = a * offsets[io];
                    seeds[ns++] = vec3_add(r0, vec3_scale(off, nm));
                    seeds[ns++] = vec3_add(r0, vec3_scale(off, bn));
                }
            }
            build_field_lines_generic(fm, seeds, ns, 0.005, 2000);
        }
    } else if (model == 9) {
        /* Torus: toroidal circles at different poloidal positions
         * within the torus cross-section (minor radius a around R0). */
        double R0 = fm->params[1];
        double a = fm->params[2];  /* minor radius */
        int npts = 256;
        static Vec3 circle[257];
        /* Seed at several poloidal angles and minor radii */
        int n_poloidal = 8;
        double minor_radii[] = {0.0, a * 0.5, a};
        int n_minor = 3;
        for (int im = 0; im < n_minor; im++) {
            double r_minor = minor_radii[im];
            int n_pol = (im == 0) ? 1 : n_poloidal;
            for (int ip = 0; ip < n_pol; ip++) {
                double theta = 2.0 * M_PI * ip / n_pol;
                double R = R0 + r_minor * cos(theta);
                double Z = r_minor * sin(theta);
                if (R < 0.1) continue;
                for (int i = 0; i <= npts; i++) {
                    double phi = 2.0 * M_PI * i / npts;
                    circle[i] = (Vec3){R * cos(phi), R * sin(phi), Z};
                }
                fl_cache_add_line(circle, npts + 1);
            }
        }
    }
}

void render3d_draw(const struct AppState *app)
{
    const FieldModel *fm = &app->models[app->current_model];
    g_arrow_head_size = app->arrow_head_size;
    g_arrow_lw = app->lw_arrows;
    g_cam_pos = (Vec3){app->camera.position.x, app->camera.position.y, app->camera.position.z};
    /* World-space size of 1 pixel at unit distance from camera */
    g_px = 2.0f * tanf(app->camera.fovy * 0.5f * DEG2RAD) / (float)GetScreenHeight();

    /* Coordinate axes (scaled by user parameter, 0 = hidden) */
    if (app->axis_scale > 1e-6f) {
        float s = app->axis_scale;
        Vec3 o = {0,0,0};
        Color ac = app->color_axes;
        Color ax = {(unsigned char)fminf(ac.r * 1.5f, 255), (unsigned char)(ac.g * 0.4f), (unsigned char)(ac.b * 0.4f), ac.a};
        Color ay = {(unsigned char)(ac.r * 0.4f), (unsigned char)fminf(ac.g * 1.5f, 255), (unsigned char)(ac.b * 0.4f), ac.a};
        Color az = {(unsigned char)(ac.r * 0.4f), (unsigned char)(ac.g * 0.4f), (unsigned char)fminf(ac.b * 1.5f, 255), ac.a};
        draw_thick_line(o, (Vec3){s,0,0}, app->lw_axes, ax);
        draw_thick_line(o, (Vec3){0,s,0}, app->lw_axes, ay);
        draw_thick_line(o, (Vec3){0,0,s}, app->lw_axes, az);
    }

    /* Field lines: rebuild only when needed */
    if (app->show_field_lines) {
        if (fl_needs_rebuild(app)) {
            build_field_lines(app);
            fl_mark_cached((struct AppState *)app);
        }
        fl_draw(app->color_field_lines, cam_dist(app), app->lw_field_lines);
    }

    /* Instantaneous field line through guiding center (selected particle) */
    if (app->show_gc_field_line) {
        int si = app->selected_particle;
        Vec3 pos = app->particles[si].pos;
        Vec3 vel = app->particles[si].vel;
        Vec3 B = fm->eval_B(fm->params, pos);
        double Bmag = vec3_len(B);
        if (Bmag > 1e-30) {
            double B2 = Bmag * Bmag;
            double mq = app->particles[si].m / app->particles[si].q;
            Vec3 vxB = vec3_cross(vel, B);
            Vec3 gc = vec3_add(pos, vec3_scale(mq / B2, vxB));

            static Vec3 gc_pts[FL_MAX_PTS];
            double ds = 0.02;
            double half_len = app->gc_fl_length / 2.0;
            Color gc_col = app->color_gc_fl;
            float vd = cam_dist(app);
            int stride = (int)(vd / 5.0f);
            if (stride < 1) stride = 1;
            int n_fwd = trace_field_line_len(fm, gc,  ds, half_len, gc_pts, FL_MAX_PTS);
            draw_thick_strip(gc_pts, n_fwd, stride, app->lw_gc_fl, gc_col);
            int n_bwd = trace_field_line_len(fm, gc, -ds, half_len, gc_pts, FL_MAX_PTS);
            draw_thick_strip(gc_pts, n_bwd, stride, app->lw_gc_fl, gc_col);
        }
    }

    /* Field line through particle position */
    if (app->show_pos_field_line) {
        int si = app->selected_particle;
        Vec3 pos = app->particles[si].pos;
        static Vec3 pos_pts[FL_MAX_PTS];
        double ds = 0.02;
        double half_len = app->gc_fl_length / 2.0;
        Color pos_col = app->color_pos_fl;
        float vd = cam_dist(app);
        int stride = (int)(vd / 5.0f);
        if (stride < 1) stride = 1;
        int n_fwd = trace_field_line_len(fm, pos,  ds, half_len, pos_pts, FL_MAX_PTS);
        draw_thick_strip(pos_pts, n_fwd, stride, app->lw_pos_fl, pos_col);
        int n_bwd = trace_field_line_len(fm, pos, -ds, half_len, pos_pts, FL_MAX_PTS);
        draw_thick_strip(pos_pts, n_bwd, stride, app->lw_pos_fl, pos_col);
    }

    /* Per-species colors from user preferences */
    const Color *sp_colors = app->species_colors;

    /* Placement preview: field line through candidate point */
    if (app->place_preview_valid) {
        static Vec3 prev_pts[FL_MAX_PTS];
        static Vec3 bwd_pts[FL_MAX_PTS];
        double ds = 0.05;
        double half_len = 2.0;  /* short preview */
        /* Bright version of the species color that would be placed */
        int psp = app->species;
        if (psp < 0 || psp >= NUM_SPECIES) psp = NUM_SPECIES - 1;
        Color sc = sp_colors[psp];
        Color prev_col = {
            (unsigned char)(sc.r + (255 - sc.r) / 2),
            (unsigned char)(sc.g + (255 - sc.g) / 2),
            (unsigned char)(sc.b + (255 - sc.b) / 2),
            220
        };

        /* Draw a sphere at the placement point */
        DrawSphere(v3(app->place_preview_pos), 0.015f, prev_col);

        int n_fwd = trace_field_line_len(fm, app->place_preview_pos,  ds, half_len, prev_pts, FL_MAX_PTS);
        for (int i = 1; i < n_fwd; i++)
            DrawLine3D(v3(prev_pts[i - 1]), v3(prev_pts[i]), prev_col);
        int n_bwd = trace_field_line_len(fm, app->place_preview_pos, -ds, half_len, bwd_pts, FL_MAX_PTS);
        for (int i = 1; i < n_bwd; i++)
            DrawLine3D(v3(bwd_pts[i - 1]), v3(bwd_pts[i]), prev_col);

        /* Drop mode: three origin coordinate planes spanning from
         * origin to the drop point, with field line projections. */
        if (app->drop_mode) {
            Vec3 pp = app->place_preview_pos;
            float px = (float)pp.x, py = (float)pp.y, pz = (float)pp.z;

            /* Plane colors: YZ=red, XZ=green, XY=blue */
            Color plane_base[3] = {
                { 180,  60,  60, 18 },  /* YZ (x=const) */
                {  60, 180,  60, 18 },  /* XZ (y=const) */
                {  60,  80, 200, 18 },  /* XY (z=const) */
            };

            /* Three rectangles from origin to the drop point:
             * XY plane: (0,0,0)-(px,0,0)-(px,py,0)-(0,py,0)
             * XZ plane: (0,0,0)-(px,0,0)-(px,0,pz)-(0,0,pz)
             * YZ plane: (0,0,0)-(0,py,0)-(0,py,pz)-(0,0,pz) */
            Vector3 quads[3][4] = {
                { {0,0,0}, {px,0,0}, {px,py,0}, {0,py,0} },   /* XY */
                { {0,0,0}, {px,0,0}, {px,0,pz}, {0,0,pz} },   /* XZ */
                { {0,0,0}, {0,py,0}, {0,py,pz}, {0,0,pz} },   /* YZ */
            };

            for (int ax = 0; ax < 3; ax++) {
                Color pc = plane_base[ax];
                unsigned char proj_alpha = 80;
                Color lc = { pc.r, pc.g, pc.b, proj_alpha };

                /* Double-sided quad */
                rlBegin(RL_QUADS);
                    rlColor4ub(pc.r, pc.g, pc.b, pc.a);
                    for (int v = 0; v < 4; v++)
                        rlVertex3f(quads[ax][v].x, quads[ax][v].y, quads[ax][v].z);
                    for (int v = 3; v >= 0; v--)
                        rlVertex3f(quads[ax][v].x, quads[ax][v].y, quads[ax][v].z);
                rlEnd();

                /* Project field line (fwd + bwd) onto this origin plane */
                for (int pass = 0; pass < 2; pass++) {
                    Vec3 *pts = (pass == 0) ? prev_pts : bwd_pts;
                    int npts  = (pass == 0) ? n_fwd : n_bwd;
                    for (int i = 1; i < npts; i++) {
                        Vec3 pa = pts[i - 1], pb = pts[i];
                        if (ax == 2)      { pa.z = 0; pb.z = 0; }
                        else if (ax == 1) { pa.y = 0; pb.y = 0; }
                        else              { pa.x = 0; pb.x = 0; }
                        DrawLine3D(v3(pa), v3(pb), lc);
                    }
                }
            }

            /* Drop-line from preview point to the XY origin plane */
            Vec3 proj_xy = { pp.x, pp.y, 0 };
            Color dl = { prev_col.r, prev_col.g, prev_col.b, 100 };
            DrawLine3D(v3(pp), v3(proj_xy), dl);
            DrawSphere(v3(proj_xy), 0.008f, dl);
        }
    }

    /* Guiding-centre drift trails (dashed, darker) — drawn first so they
     * appear behind the orbit trails. */
    Vector3 cam_pos = app->camera.position;
    if (app->show_gc_trajectory) {
        int gc_trail_table[] = {7680, 76800, 768000};
        int gc_pr = app->plot_range;
        if (gc_pr < 0) gc_pr = 0;
        if (gc_pr > 2) gc_pr = 2;
        int gc_max_trail = gc_trail_table[gc_pr];
        float gc_fade = app->trail_fade;

        for (int pi = 0; pi < app->n_particles; pi++) {
            int sp = app->particles[pi].species;
            if (sp < 0 || sp >= NUM_SPECIES) sp = NUM_SPECIES - 1;
            int gc_show = app->gc_trails[pi].count < gc_max_trail
                        ? app->gc_trails[pi].count : gc_max_trail;
            if (gc_show < 2) continue;

            /* Darker species color */
            Color gc = sp_colors[sp];
            gc.r = (unsigned char)(gc.r * 0.55f);
            gc.g = (unsigned char)(gc.g * 0.55f);
            gc.b = (unsigned char)(gc.b * 0.55f);

            /* Distance range sampling */
            float gdmin = 1e30f, gdmax = 0.0f;
            int gds = gc_show > 1024 ? gc_show / 256 : 1;
            for (int i = 0; i < gc_show; i += gds) {
                Vec3 p;
                trail_get(&app->gc_trails[pi], i, &p);
                float dx = (float)p.x - cam_pos.x;
                float dy = (float)p.y - cam_pos.y;
                float dz = (float)p.z - cam_pos.z;
                float d = dx*dx + dy*dy + dz*dz;
                if (d < gdmin) gdmin = d;
                if (d > gdmax) gdmax = d;
            }
            gdmin = sqrtf(gdmin); gdmax = sqrtf(gdmax);
            float gdrange = gdmax - gdmin;
            if (gdrange < 1e-10f) gdrange = 1.0f;

            /* Adaptive subsampling */
            float gtw = app->trail_thickness;
            if (gtw < 1.0f) gtw = 1.0f;
            int gn_passes = (gtw <= 1.01f) ? 1 : (int)gtw;
            int gfull_res = gc_show / 5;
            int gold_step = 1;
            if (gc_show > 50000) {
                gold_step = (gc_show - gfull_res) / 30000;
                if (gold_step < 2) gold_step = 2;
            }

            #define DASH_SEGS 4
            rlBegin(RL_LINES);
            int gstep = 1;
            int gprev_i = 0;
            int gc_cap = app->gc_trails[pi].capacity;
            int gc_head = app->gc_trails[pi].head;
            for (int i = 1; i < gc_show; i += gstep) {
                if (i > gfull_res && gold_step > 1) gstep = gold_step;

                /* Dashed pattern anchored to buffer index (stable as trail grows) */
                int buf_idx = (gc_head - 1 - i + gc_cap) % gc_cap;
                int dash_on = (buf_idx / DASH_SEGS) % 2 == 0;
                if (!dash_on) { gprev_i = i; continue; }

                Vec3 a, b;
                trail_get(&app->gc_trails[pi], gprev_i, &a);
                trail_get(&app->gc_trails[pi], i, &b);

                float age = (float)i / (float)gc_show;
                float age_fade = 1.0f - age * gc_fade;

                float dx = (float)a.x - cam_pos.x;
                float dy = (float)a.y - cam_pos.y;
                float dz = (float)a.z - cam_pos.z;
                float seg_dist = sqrtf(dx*dx + dy*dy + dz*dz);
                float t2 = (seg_dist - gdmin) / gdrange;
                float dist_fade = 1.0f - 0.5f * t2;

                unsigned char alpha = (unsigned char)(gc.a * age_fade * dist_fade);

                rlColor4ub(gc.r, gc.g, gc.b, alpha);
                rlVertex3f((float)a.x, (float)a.y, (float)a.z);
                rlVertex3f((float)b.x, (float)b.y, (float)b.z);

                for (int k = 1; k < gn_passes; k++) {
                    Vec3 seg_dir = vec3_sub(b, a);
                    Vec3 view_dir = vec3_sub(a, g_cam_pos);
                    Vec3 od = vec3_cross(seg_dir, view_dir);
                    double olen = vec3_len(od);
                    if (olen < 1e-12) continue;
                    Vec3 da = vec3_scale(k * 0.5 * g_px * seg_dist / olen, od);
                    rlColor4ub(gc.r, gc.g, gc.b, alpha);
                    rlVertex3f((float)(a.x+da.x), (float)(a.y+da.y), (float)(a.z+da.z));
                    rlVertex3f((float)(b.x+da.x), (float)(b.y+da.y), (float)(b.z+da.z));
                    rlColor4ub(gc.r, gc.g, gc.b, alpha);
                    rlVertex3f((float)(a.x-da.x), (float)(a.y-da.y), (float)(a.z-da.z));
                    rlVertex3f((float)(b.x-da.x), (float)(b.y-da.y), (float)(b.z-da.z));
                }

                gprev_i = i;
            }
            rlEnd();
            #undef DASH_SEGS

            /* GC sphere (smaller) */
            DrawSphere(v3(app->gc_particles[pi].pos), 0.003f, gc);
        }
    }

    /* Orbit trails and particle spheres for all particles.
     * x1 shows ~30 gyroperiods, x10 ~300, x100 all available.
     * Fade rate decreases at longer ranges so the tail stays visible. */
    int trail_table[] = {7680, 76800, 768000};
    int pr = app->plot_range;
    if (pr < 0) pr = 0;
    if (pr > 2) pr = 2;
    int max_trail = trail_table[pr];
    float fade_strength = app->trail_fade;

    for (int pi = 0; pi < app->n_particles; pi++) {
        int sp = app->particles[pi].species;
        if (sp < 0 || sp >= NUM_SPECIES) sp = NUM_SPECIES - 1;

        if (!app->show_trail) goto draw_sphere;
        int trail_show = app->trails[pi].count < max_trail ? app->trails[pi].count : max_trail;
        Color tc = sp_colors[sp];

        /* Sample distance range (every ~256th point, not all) */
        float dmin = 1e30f, dmax = 0.0f;
        int dsample = trail_show > 1024 ? trail_show / 256 : 1;
        for (int i = 0; i < trail_show; i += dsample) {
            Vec3 p;
            trail_get(&app->trails[pi], i, &p);
            float dx = (float)p.x - cam_pos.x;
            float dy = (float)p.y - cam_pos.y;
            float dz = (float)p.z - cam_pos.z;
            float d = dx*dx + dy*dy + dz*dz;
            if (d < dmin) dmin = d;
            if (d > dmax) dmax = d;
        }
        dmin = sqrtf(dmin); dmax = sqrtf(dmax);
        float drange = dmax - dmin;
        if (drange < 1e-10f) drange = 1.0f;

        /* Trail drawing: batched rlgl + adaptive subsampling for long trails.
         * Newest 20% at full resolution, older portions progressively coarser.
         * For thickness > 1, draw parallel offset lines (glLineWidth limited
         * to 1.0 on macOS core profile). */
        float tw = app->trail_thickness;
        if (tw < 1.0f) tw = 1.0f;
        int n_passes = (tw <= 1.01f) ? 1 : (int)tw;

        /* Adaptive step: full res for newest portion, subsample the rest */
        int full_res = trail_show / 5;       /* newest 20% at step=1 */
        int old_step = 1;
        if (trail_show > 50000) {
            old_step = (trail_show - full_res) / 30000;
            if (old_step < 2) old_step = 2;
        }

        rlBegin(RL_LINES);
        int step = 1;
        int prev_i = 0;
        for (int i = 1; i < trail_show; i += step) {
            if (i > full_res && old_step > 1) step = old_step;

            Vec3 a, b;
            trail_get(&app->trails[pi], prev_i, &a);
            trail_get(&app->trails[pi], i, &b);

            float age = (float)i / (float)trail_show;
            float age_fade = 1.0f - age * fade_strength;

            float dx = (float)a.x - cam_pos.x;
            float dy = (float)a.y - cam_pos.y;
            float dz = (float)a.z - cam_pos.z;
            float seg_dist = sqrtf(dx*dx + dy*dy + dz*dz);
            float t2 = (seg_dist - dmin) / drange;
            float dist_fade = 1.0f - 0.5f * t2;

            unsigned char alpha = (unsigned char)(tc.a * age_fade * dist_fade);

            rlColor4ub(tc.r, tc.g, tc.b, alpha);
            rlVertex3f((float)a.x, (float)a.y, (float)a.z);
            rlVertex3f((float)b.x, (float)b.y, (float)b.z);

            for (int k = 1; k < n_passes; k++) {
                Vec3 seg_dir = vec3_sub(b, a);
                Vec3 view_dir = vec3_sub(a, g_cam_pos);
                Vec3 od = vec3_cross(seg_dir, view_dir);
                double olen = vec3_len(od);
                if (olen < 1e-12) continue;
                Vec3 da = vec3_scale(k * 0.5 * g_px * seg_dist / olen, od);
                rlColor4ub(tc.r, tc.g, tc.b, alpha);
                rlVertex3f((float)(a.x+da.x), (float)(a.y+da.y), (float)(a.z+da.z));
                rlVertex3f((float)(b.x+da.x), (float)(b.y+da.y), (float)(b.z+da.z));
                rlColor4ub(tc.r, tc.g, tc.b, alpha);
                rlVertex3f((float)(a.x-da.x), (float)(a.y-da.y), (float)(a.z-da.z));
                rlVertex3f((float)(b.x-da.x), (float)(b.y-da.y), (float)(b.z-da.z));
            }

            prev_i = i;
        }
        rlEnd();

        draw_sphere:;
        /* Particle sphere */
        Color pc = sp_colors[sp];
        float sphere_r = (pi == app->selected_particle) ? 0.006f : 0.004f;
        DrawSphere(v3(app->particles[pi].pos), sphere_r, pc);
    }

    /* Velocity, B, and Lorentz force vectors for selected particle */
    {
        int si = app->selected_particle;
        Vec3 pos = app->particles[si].pos;
        Vec3 vel = app->particles[si].vel;
        double v_mag = vec3_len(vel);
        double unit_len = 0.06; /* fixed arrow length for unit mode */

        if (app->show_velocity_vec && v_mag > 1e-30) {
            double len = app->vec_scaled
                ? v_mag * pow(10.0, (double)app->vec_scale_v)
                : unit_len;
            Vec3 vdir = vec3_scale(1.0 / v_mag, vel);
            Vec3 vtip = vec3_add(pos, vec3_scale(len, vdir));
            draw_arrow(pos, vtip, app->color_vel);
        }

        if (app->show_B_vec || app->show_F_vec) {
            Vec3 B = fm->eval_B(fm->params, pos);
            double Bmag = vec3_len(B);
            if (app->show_B_vec && Bmag > 1e-30) {
                double len = app->vec_scaled
                    ? Bmag * pow(10.0, (double)app->vec_scale_B)
                    : unit_len;
                Vec3 bdir = vec3_scale(1.0 / Bmag, B);
                Vec3 btip = vec3_add(pos, vec3_scale(len, bdir));
                draw_arrow(pos, btip, app->color_B);
            }
            if (app->show_F_vec && Bmag > 1e-30 && v_mag > 1e-30) {
                double q = app->particles[si].q;
                Vec3 F = vec3_scale(q, vec3_cross(vel, B)); /* q(v×B) */
                double Fmag = vec3_len(F);
                if (Fmag > 1e-30) {
                    double len = app->vec_scaled
                        ? Fmag * pow(10.0, (double)app->vec_scale_F)
                        : unit_len;
                    Vec3 fdir = vec3_scale(1.0 / Fmag, F);
                    Vec3 ftip = vec3_add(pos, vec3_scale(len, fdir));
                    draw_arrow(pos, ftip, app->color_F);
                }
            }
        }
    }

    /* In field-aligned view, show kappa-hat (e1) and binormal (e2) */
    if (app->cam_field_aligned && app->frame_e1_init) {
        int saved_lw = g_arrow_lw;
        g_arrow_lw = app->lw_triad;
        Vec3 pos = app->particles[app->selected_particle].pos;
        double arrow_len = 0.06;
        Vec3 e1tip = vec3_add(pos, vec3_scale(arrow_len, app->frame_e1));
        draw_arrow(pos, e1tip, app->color_kappa);
        Vec3 e2tip = vec3_add(pos, vec3_scale(arrow_len, app->frame_e2));
        draw_arrow(pos, e2tip, app->color_binormal);
        g_arrow_lw = saved_lw;
    }
}
