#include "field.h"
#include <math.h>

double field_Bmag(const FieldModel *fm, Vec3 pos)
{
    Vec3 B = fm->eval_B(fm->params, pos);
    return vec3_len(B);
}

Vec3 field_gradB(const FieldModel *fm, Vec3 pos)
{
    double h = 1e-5;
    double Bxp = field_Bmag(fm, (Vec3){pos.x+h, pos.y, pos.z});
    double Bxm = field_Bmag(fm, (Vec3){pos.x-h, pos.y, pos.z});
    double Byp = field_Bmag(fm, (Vec3){pos.x, pos.y+h, pos.z});
    double Bym = field_Bmag(fm, (Vec3){pos.x, pos.y-h, pos.z});
    double Bzp = field_Bmag(fm, (Vec3){pos.x, pos.y, pos.z+h});
    double Bzm = field_Bmag(fm, (Vec3){pos.x, pos.y, pos.z-h});
    return (Vec3){
        (Bxp - Bxm) / (2*h),
        (Byp - Bym) / (2*h),
        (Bzp - Bzm) / (2*h)
    };
}

Vec3 field_curvature(const FieldModel *fm, Vec3 pos, Vec3 bhat)
{
    double h = 1e-5;
    Vec3 fwd = vec3_add(pos, vec3_scale(h, bhat));
    Vec3 bwd = vec3_sub(pos, vec3_scale(h, bhat));
    Vec3 Bf = fm->eval_B(fm->params, fwd);
    Vec3 Bb = fm->eval_B(fm->params, bwd);
    double Bf_mag = vec3_len(Bf);
    double Bb_mag = vec3_len(Bb);
    if (Bf_mag < 1e-30 || Bb_mag < 1e-30) return (Vec3){0, 0, 0};
    Vec3 bf = vec3_scale(1.0 / Bf_mag, Bf);
    Vec3 bb = vec3_scale(1.0 / Bb_mag, Bb);
    return vec3_scale(0.5 / h, vec3_sub(bf, bb));
}

double field_kappa_bz(const FieldModel *fm, Vec3 pos, double p_mag, double q)
{
    Vec3 B = fm->eval_B(fm->params, pos);
    double Bmag = vec3_len(B);
    if (Bmag <= 0.0) return NAN;
    double curv = vec3_len(field_curvature(fm, pos, vec3_scale(1.0 / Bmag, B)));
    if (curv <= 0.0) return INFINITY;
    double rho = p_mag / (fabs(q) * Bmag);
    return sqrt(1.0 / (curv * rho));
}

static Vec3 unit_B(const FieldModel *fm, Vec3 pos)
{
    Vec3 B = fm->eval_B(fm->params, pos);
    double Bmag = vec3_len(B);
    return Bmag > 0.0 ? vec3_scale(1.0 / Bmag, B) : vec3_zero();
}

int field_L_shell(const FieldModel *fm, Vec3 pos, double *L)
{
    Vec3 pmin;
    if (!field_min_B_point(fm, pos, 0.01, &pmin)) return 0;
    *L = vec3_len(pmin);
    return 1;
}

int field_min_B_point(const FieldModel *fm, Vec3 pos, double step, Vec3 *pmin_out)
{
    if (fm->body_radius <= 0.0 || field_inside_body(fm, pos)) return 0;

    double Bmin = INFINITY;
    Vec3 pmin = pos;
    for (int dir = -1; dir <= 1; dir += 2) {
        Vec3 p = pos;
        int closed = 0;
        /* RK4 along b, step a fraction of the geocentric distance */
        for (int i = 0; i < 20000; i++) {
            Vec3 B = fm->eval_B(fm->params, p);
            double Bmag = vec3_len(B);
            if (Bmag <= 0.0) break;
            if (Bmag < Bmin) { Bmin = Bmag; pmin = p; }
            if (field_inside_body(fm, p)) { closed = 1; break; }
            if (fm->inside && !fm->inside(fm->params, p)) break;   /* left it: open */
            double r = vec3_len(p);
            if (r > 100.0) break;
            double h = dir * step * r;
            Vec3 k1 = vec3_scale(1.0 / Bmag, B);
            Vec3 k2 = unit_B(fm, vec3_add(p, vec3_scale(0.5 * h, k1)));
            Vec3 k3 = unit_B(fm, vec3_add(p, vec3_scale(0.5 * h, k2)));
            Vec3 k4 = unit_B(fm, vec3_add(p, vec3_scale(h, k3)));
            Vec3 k = vec3_add(vec3_add(k1, k4), vec3_scale(2.0, vec3_add(k2, k3)));
            p = vec3_add(p, vec3_scale(h / 6.0, k));
        }
        if (!closed) return 0;
    }
    *pmin_out = pmin;
    return 1;
}

/* Minimum of |B| near sample i, from the parabola through samples i-1, i,
 * i+1, spaced about equally along the field line */
static Vec3 refine_min(const Vec3 *p, const double *b, int i)
{
    double den = b[i - 1] - 2.0 * b[i] + b[i + 1];
    double t = den > 0.0 ? 0.5 * (b[i - 1] - b[i + 1]) / den : 0.0;
    if (t < -0.5) t = -0.5;
    if (t > 0.5) t = 0.5;
    Vec3 d1 = vec3_scale(0.5, vec3_sub(p[i + 1], p[i - 1]));
    Vec3 d2 = vec3_scale(0.5, vec3_add(vec3_sub(p[i + 1], vec3_scale(2.0, p[i])), p[i - 1]));
    return vec3_add(p[i], vec3_add(vec3_scale(t, d1), vec3_scale(t * t, d2)));
}

int field_line_minima(const FieldModel *fm, Vec3 foot, double step, Vec3 mins[2])
{
    enum { NMAX = 20000 };
    static Vec3 p[NMAX];
    static double b[NMAX];
    if (fm->body_radius <= 0.0) return 0;

    /* Trace away from the body until the line comes back down */
    Vec3 x = foot;
    Vec3 B0 = fm->eval_B(fm->params, x);
    double dir = vec3_dot(B0, x) > 0.0 ? 1.0 : -1.0;
    int n = 0, closed = 0;
    while (n < NMAX) {
        Vec3 B = fm->eval_B(fm->params, x);
        double Bmag = vec3_len(B);
        if (Bmag <= 0.0) break;
        p[n] = x;
        b[n++] = Bmag;
        if (n > 1 && field_inside_body(fm, x)) { closed = 1; break; }
        if (fm->inside && !fm->inside(fm->params, x)) break;     /* left it: open */
        double r = vec3_len(x);
        if (r > 100.0) break;
        double h = dir * step * r;
        Vec3 k1 = vec3_scale(1.0 / Bmag, B);
        Vec3 k2 = unit_B(fm, vec3_add(x, vec3_scale(0.5 * h, k1)));
        Vec3 k3 = unit_B(fm, vec3_add(x, vec3_scale(0.5 * h, k2)));
        Vec3 k4 = unit_B(fm, vec3_add(x, vec3_scale(h, k3)));
        Vec3 k = vec3_add(vec3_add(k1, k4), vec3_scale(2.0, vec3_add(k2, k3)));
        x = vec3_add(x, vec3_scale(h / 6.0, k));
    }
    if (!closed || n < 3) return 0;

    /* The deepest dip, then the deepest other one walled off from it by a
     * rise of more than 0.1% above both */
    int g = 1;
    for (int i = 1; i + 1 < n; i++)
        if (b[i] < b[g]) g = i;
    int second = -1;
    for (int i = 1; i + 1 < n; i++) {
        if (i == g || !(b[i] < b[i - 1] && b[i] <= b[i + 1])) continue;
        int lo = i < g ? i : g, hi = i < g ? g : i;
        double wall = 0.0;
        for (int j = lo + 1; j < hi; j++)
            if (b[j] > wall) wall = b[j];
        if (wall - b[i] > 1e-3 * wall && (second < 0 || b[i] < b[second]))
            second = i;
    }
    if (second < 0) {
        mins[0] = refine_min(p, b, g);
        return 1;
    }
    int first = second < g ? second : g, last = second < g ? g : second;
    mins[0] = refine_min(p, b, first);
    mins[1] = refine_min(p, b, last);
    return 2;
}

void field_init_all(FieldModel models[FIELD_NUM_MODELS])
{
    field_init_circular(&models[0]);
    field_init_grad_b(&models[1]);
    field_init_higher_grad(&models[2]);
    field_init_sinusoidal(&models[3]);
    field_init_nonphysical(&models[4]);
    field_init_dipole(&models[5]);
    field_init_bottle(&models[6]);
    field_init_tokamak(&models[7]);
    field_init_stellarator(&models[8]);
    field_init_torus(&models[9]);
    field_init_harris(&models[10]);
    field_init_turbulence(&models[11]);
    field_init_t89(&models[12]);
}

void field_retranslate(FieldModel models[FIELD_NUM_MODELS])
{
    /* Save current params, re-init (gets new translated names), restore params */
    for (int m = 0; m < FIELD_NUM_MODELS; m++) {
        double saved[FIELD_MAX_PARAMS];
        for (int i = 0; i < models[m].n_params; i++)
            saved[i] = models[m].params[i];
        int np = models[m].n_params;

        switch (m) {
        case 0: field_init_circular(&models[m]); break;
        case 1: field_init_grad_b(&models[m]); break;
        case 2: field_init_higher_grad(&models[m]); break;
        case 3: field_init_sinusoidal(&models[m]); break;
        case 4: field_init_nonphysical(&models[m]); break;
        case 5: field_init_dipole(&models[m]); break;
        case 6: field_init_bottle(&models[m]); break;
        case 7: field_init_tokamak(&models[m]); break;
        case 8: field_init_stellarator(&models[m]); break;
        case 9: field_init_torus(&models[m]); break;
        case 10: field_init_harris(&models[m]); break;
        case 11: field_init_turbulence(&models[m]); break;
        case 12: field_init_t89(&models[m]); break;
        }

        for (int i = 0; i < np; i++)
            models[m].params[i] = saved[i];
    }
}
