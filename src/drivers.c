#include "drivers.h"
#include <math.h>
#include <string.h>

void b0window_reset(B0Window *w)
{
    memset(w, 0, sizeof(*w));
}

void b0window_push(B0Window *w, double bmag, double dpsi)
{
    w->psi_now += dpsi;

    /* Buffer full: evict the oldest sample before overwriting its slot. */
    if (w->count == B0WIN_CAP) {
        int tail = (w->head - w->count + B0WIN_CAP) % B0WIN_CAP;
        w->sum -= w->bmag[tail];
        w->count--;
    }

    w->psi[w->head] = w->psi_now;
    w->bmag[w->head] = bmag;
    w->sum += bmag;
    w->head = (w->head + 1) % B0WIN_CAP;
    w->count++;

    /* Evict samples that have fallen outside one gyroperiod of phase,
     * keeping at least the current sample. */
    while (w->count > 1) {
        int tail = (w->head - w->count + B0WIN_CAP) % B0WIN_CAP;
        if (w->psi_now - w->psi[tail] <= 2.0 * M_PI) break;
        w->sum -= w->bmag[tail];
        w->count--;
    }
}

double b0window_avg(const B0Window *w)
{
    if (w->count <= 0) return 0.0;
    return w->sum / (double)w->count;
}

double drivers_eff_mass(const Particle *p, int relativistic)
{
    if (!relativistic) return p->m;
    double b2 = vec3_dot(p->vel, p->vel)
              / (SPEED_OF_LIGHT * SPEED_OF_LIGHT);
    if (b2 >= 1.0) return p->m;
    return p->m / sqrt(1.0 - b2);
}

void drivers_eval(const FieldModel *fm, const Particle *p, double B0,
                  int relativistic, DriverChannels *out)
{
    memset(out, 0, sizeof(*out));

    Vec3 B = fm->eval_B(fm->params, p->pos);
    double Bm = vec3_len(B);
    if (!(Bm > 1e-30) || !(B0 > 1e-30) || !(fabs(p->q) > 1e-40)) return;

    Vec3 bhat = vec3_scale(1.0 / Bm, B);
    Vec3 v = p->vel;
    double v_mag = vec3_len(v);
    Vec3 v_perp = vec3_sub(v, vec3_scale(vec3_dot(v, bhat), bhat));

    /* (v.grad)bhat by central difference along v, same h as field_gradB. */
    Vec3 vdgb = {0, 0, 0};
    if (v_mag > 1e-30) {
        double h = 1e-5;
        Vec3 vhat = vec3_scale(1.0 / v_mag, v);
        Vec3 Bf = fm->eval_B(fm->params, vec3_add(p->pos, vec3_scale(h, vhat)));
        Vec3 Bb = fm->eval_B(fm->params, vec3_sub(p->pos, vec3_scale(h, vhat)));
        double Bfm = vec3_len(Bf), Bbm = vec3_len(Bb);
        if (Bfm > 1e-30 && Bbm > 1e-30) {
            Vec3 db = vec3_sub(vec3_scale(1.0 / Bfm, Bf),
                               vec3_scale(1.0 / Bbm, Bb));
            vdgb = vec3_scale(v_mag * 0.5 / h, db);
        }
    }

    double m_eff = drivers_eff_mass(p, relativistic);
    double c0 = m_eff / (p->q * B0 * B0);

    /* Field-strength channel: cross-product drive + parametric dB member. */
    Vec3 gradB = field_gradB(fm, p->pos);
    double dB = Bm - B0;
    Vec3 cross_drive = vec3_scale(c0 * vec3_dot(v, gradB),
                                  vec3_cross(v, bhat));
    double parametric = 2.0 * dB / B0 + (dB * dB) / (B0 * B0);
    out->v_gradB = vec3_sub(cross_drive, vec3_scale(parametric, v_perp));

    /* Field-direction channel: perpendicular part of v x (v.grad)bhat. */
    Vec3 cx = vec3_cross(v, vdgb);
    Vec3 cx_perp = vec3_sub(cx, vec3_scale(vec3_dot(cx, bhat), bhat));
    out->v_gradbhat = vec3_scale(c0 * Bm, cx_perp);

    out->w_tilde = vec3_sub(v_perp, vec3_add(out->v_gradB, out->v_gradbhat));
    out->valid = 1;
}
