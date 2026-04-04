#include "gc.h"
#include "boris.h"   /* QE, MP, SPEED_OF_LIGHT, Particle */
#include <math.h>

#define C2 (SPEED_OF_LIGHT * SPEED_OF_LIGHT)

/* ---------- helpers ---------------------------------------------------- */

/* Field-line curvature: kappa = (bhat . nabla) bhat, via central differences
 * along bhat.  Returns the curvature vector (1/m units). */
static Vec3 field_curvature(const FieldModel *fm, Vec3 pos, Vec3 bhat)
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

/* ---------- initialisation --------------------------------------------- */

/* Compute instantaneous GC position from particle state (used per-sample). */
static Vec3 gc_pos_from_particle(const Particle *p, Vec3 bhat,
                                 double Bmag, int relativistic)
{
    Vec3 vxb = vec3_cross(p->vel, bhat);
    double meff = p->m;
    if (relativistic) {
        double v2 = vec3_dot(p->vel, p->vel);
        meff *= 1.0 / sqrt(1.0 - v2 / C2);   /* gamma * m */
    }
    return vec3_add(p->pos, vec3_scale(meff / (p->q * Bmag), vxb));
}

void gc_init_from_particle(GCParticle *gc, const Particle *p,
                           const FieldModel *fm, int relativistic,
                           double dt)
{
    (void)dt;
    gc->q = p->q;
    gc->m = p->m;

    Vec3 B = fm->eval_B(fm->params, p->pos);
    double Bmag = vec3_len(B);
    if (Bmag < 1e-30 || fabs(p->q) < 1e-30) {
        gc->pos = p->pos;
        gc->p_par = 0.0;
        gc->mu = 0.0;
        return;
    }

    Vec3 bhat = vec3_scale(1.0 / Bmag, B);
    gc->pos = gc_pos_from_particle(p, bhat, Bmag, relativistic);

    double v_par = vec3_dot(p->vel, bhat);
    double v2 = vec3_dot(p->vel, p->vel);
    double v_perp2 = v2 - v_par * v_par;
    if (v_perp2 < 0.0) v_perp2 = 0.0;

    if (!relativistic) {
        gc->p_par = v_par;
        gc->mu = p->m * v_perp2 / (2.0 * Bmag);
    } else {
        double gamma = 1.0 / sqrt(1.0 - v2 / C2);
        gc->p_par = gamma * p->m * v_par;
        gc->mu = gamma * gamma * p->m * v_perp2 / (2.0 * Bmag);
    }
}

/* Curl of bhat via finite differences: ∇ × b̂.
 * Evaluates B at 6 offset points (±h in x, y, z). */
static Vec3 field_curl_bhat(const FieldModel *fm, Vec3 pos)
{
    double h = 1e-5;
    /* b̂ at ±h in each direction */
    Vec3 bxp, bxm, byp, bym, bzp, bzm;
    {
        Vec3 Bt;
        double Bm;
        Bt = fm->eval_B(fm->params, (Vec3){pos.x+h, pos.y, pos.z});
        Bm = vec3_len(Bt); bxp = (Bm > 1e-30) ? vec3_scale(1.0/Bm, Bt) : (Vec3){0,0,0};
        Bt = fm->eval_B(fm->params, (Vec3){pos.x-h, pos.y, pos.z});
        Bm = vec3_len(Bt); bxm = (Bm > 1e-30) ? vec3_scale(1.0/Bm, Bt) : (Vec3){0,0,0};
        Bt = fm->eval_B(fm->params, (Vec3){pos.x, pos.y+h, pos.z});
        Bm = vec3_len(Bt); byp = (Bm > 1e-30) ? vec3_scale(1.0/Bm, Bt) : (Vec3){0,0,0};
        Bt = fm->eval_B(fm->params, (Vec3){pos.x, pos.y-h, pos.z});
        Bm = vec3_len(Bt); bym = (Bm > 1e-30) ? vec3_scale(1.0/Bm, Bt) : (Vec3){0,0,0};
        Bt = fm->eval_B(fm->params, (Vec3){pos.x, pos.y, pos.z+h});
        Bm = vec3_len(Bt); bzp = (Bm > 1e-30) ? vec3_scale(1.0/Bm, Bt) : (Vec3){0,0,0};
        Bt = fm->eval_B(fm->params, (Vec3){pos.x, pos.y, pos.z-h});
        Bm = vec3_len(Bt); bzm = (Bm > 1e-30) ? vec3_scale(1.0/Bm, Bt) : (Vec3){0,0,0};
    }
    double inv2h = 0.5 / h;
    /* ∂b̂_i/∂x_j via central differences */
    double dbz_dy = (byp.z - bym.z) * inv2h;
    double dby_dz = (bzp.y - bzm.y) * inv2h;
    double dbx_dz = (bzp.x - bzm.x) * inv2h;
    double dbz_dx = (bxp.z - bxm.z) * inv2h;
    double dby_dx = (bxp.y - bxm.y) * inv2h;
    double dbx_dy = (byp.x - bym.x) * inv2h;
    return (Vec3){
        dbz_dy - dby_dz,
        dbx_dz - dbz_dx,
        dby_dx - dbx_dy
    };
}

/* ---------- RHS of the GC equations ------------------------------------ */

/* Evaluate dR/dt and dp_par/dt.  Writes to *dR and *dp.
 * symplectic: 0 = Alfvén drifts, 1 = Littlejohn B* form. */
static void gc_rhs(Vec3 pos, double p_par, double mu, double q, double m,
                   const FieldModel *fm, int relativistic, int symplectic,
                   Vec3 *dR, double *dp)
{
    Vec3 B = fm->eval_B(fm->params, pos);
    double Bmag = vec3_len(B);
    if (Bmag < 1e-30 || fabs(q) < 1e-30) {
        *dR = (Vec3){0, 0, 0};
        *dp = 0.0;
        return;
    }
    Vec3 bhat = vec3_scale(1.0 / Bmag, B);
    Vec3 gradB = field_gradB(fm, pos);

    /* E field (if present) */
    Vec3 E = {0, 0, 0};
    if (fm->eval_E)
        E = fm->eval_E(fm->params, pos);

    double v_par, gamma;
    if (!relativistic) {
        v_par = p_par;
        gamma = 1.0;
    } else {
        gamma = sqrt(1.0 + (p_par * p_par) / (m * m * C2)
                         + 2.0 * mu * Bmag / (m * C2));
        v_par = p_par / (gamma * m);
    }

    double meff = relativistic ? gamma * m : m;
    double coeff_gradB = relativistic ? mu / gamma : mu;

    if (symplectic) {
        /* Littlejohn B* symplectic form (Hamiltonian structure).
         * B* = B b̂ + (m_eff v_∥ / q)(∇ × b̂)
         * B*_∥ = b̂ · B*
         * dR/dt = (v_∥ B* + (1/q) b̂ × (μ∇B + qE)) / B*_∥
         * dp_∥/dt = B* · (qE - μ∇B) / (m_eff B*_∥)
         * Curvature drift is implicit in v_∥ B*. */
        Vec3 curl_bhat = field_curl_bhat(fm, pos);
        double Bstar_par = Bmag + (meff * v_par / q) * vec3_dot(bhat, curl_bhat);
        if (fabs(Bstar_par) < 1e-30 * Bmag) Bstar_par = Bmag;

        Vec3 Bstar = vec3_add(vec3_scale(Bmag, bhat),
                              vec3_scale(meff * v_par / q, curl_bhat));
        Vec3 perp_force = vec3_add(vec3_scale(coeff_gradB, gradB),
                                   vec3_scale(q, E));
        *dR = vec3_scale(1.0 / Bstar_par,
                  vec3_add(vec3_scale(v_par, Bstar),
                           vec3_scale(1.0 / q, vec3_cross(bhat, perp_force))));

        Vec3 par_force = vec3_add(vec3_scale(-coeff_gradB, gradB),
                                  vec3_scale(q, E));
        *dp = vec3_dot(Bstar, par_force) / (meff * Bstar_par);
    } else {
        /* Alfvén drift equations (explicit grad-B + curvature drifts).
         * dR/dt = v_∥ b̂ + (b̂/(qB)) × (μ∇B + mv_∥²κ + qE)
         * dv_∥/dt = -(μ/m)(b̂·∇B) + (q/m)(E·b̂) */
        Vec3 kappa = field_curvature(fm, pos, bhat);
        double coeff_curv = relativistic ? gamma * m * v_par * v_par
                                         : m * v_par * v_par;

        Vec3 force = vec3_add(vec3_scale(coeff_gradB, gradB),
                              vec3_scale(coeff_curv, kappa));
        force = vec3_add(force, vec3_scale(q, E));
        *dR = vec3_add(vec3_scale(v_par, bhat),
                       vec3_scale(1.0 / (q * Bmag), vec3_cross(bhat, force)));

        double bdotgB = vec3_dot(bhat, gradB);
        double Edotb = vec3_dot(E, bhat);
        *dp = -(coeff_gradB / meff) * bdotgB + (q / meff) * Edotb;
    }
}

/* ---------- RK4 integrator --------------------------------------------- */

void gc_step(GCParticle *gc, const FieldModel *fm, double dt,
             int relativistic, int symplectic)
{
    Vec3 pos = gc->pos;
    double pp = gc->p_par;
    double mu = gc->mu;
    double q = gc->q, m = gc->m;

    Vec3 k1r, k2r, k3r, k4r;
    double k1p, k2p, k3p, k4p;

    gc_rhs(pos, pp, mu, q, m, fm, relativistic, symplectic, &k1r, &k1p);

    Vec3 p2 = vec3_add(pos, vec3_scale(0.5 * dt, k1r));
    double pp2 = pp + 0.5 * dt * k1p;
    gc_rhs(p2, pp2, mu, q, m, fm, relativistic, symplectic, &k2r, &k2p);

    Vec3 p3 = vec3_add(pos, vec3_scale(0.5 * dt, k2r));
    double pp3 = pp + 0.5 * dt * k2p;
    gc_rhs(p3, pp3, mu, q, m, fm, relativistic, symplectic, &k3r, &k3p);

    Vec3 p4 = vec3_add(pos, vec3_scale(dt, k3r));
    double pp4 = pp + dt * k3p;
    gc_rhs(p4, pp4, mu, q, m, fm, relativistic, symplectic, &k4r, &k4p);

    gc->pos = vec3_add(pos, vec3_scale(dt / 6.0,
                 vec3_add(k1r, vec3_add(vec3_scale(2.0, k2r),
                     vec3_add(vec3_scale(2.0, k3r), k4r)))));
    gc->p_par = pp + (dt / 6.0) * (k1p + 2.0 * k2p + 2.0 * k3p + k4p);
}

void gc_step_batch(GCParticle *gc, int n, const FieldModel *fm,
                   double dt, int relativistic, int symplectic)
{
    for (int i = 0; i < n; i++)
        gc_step(&gc[i], fm, dt, relativistic, symplectic);
}

/* ---------- diagnostics ------------------------------------------------ */

double gc_pitch_angle(const GCParticle *gc, const FieldModel *fm,
                      int relativistic)
{
    double Bmag = field_Bmag(fm, gc->pos);
    if (Bmag < 1e-30) return 0.0;

    double v_par, v_perp2;
    if (!relativistic) {
        v_par = gc->p_par;
        v_perp2 = 2.0 * gc->mu * Bmag / gc->m;
    } else {
        double gamma = sqrt(1.0 + (gc->p_par * gc->p_par) / (gc->m * gc->m * C2)
                                + 2.0 * gc->mu * Bmag / (gc->m * C2));
        v_par = gc->p_par / (gamma * gc->m);
        /* p_perp^2 = 2 m mu B, so v_perp^2 = p_perp^2/(gamma m)^2 */
        v_perp2 = 2.0 * gc->mu * Bmag / (gamma * gamma * gc->m);
    }
    double v2 = v_par * v_par + v_perp2;
    if (v2 < 1e-60) return 0.0;
    double sin_alpha = sqrt(v_perp2 / v2);
    if (sin_alpha > 1.0) sin_alpha = 1.0;
    double alpha = asin(sin_alpha) * 180.0 / M_PI;
    return (v_par < 0.0) ? 180.0 - alpha : alpha;
}

