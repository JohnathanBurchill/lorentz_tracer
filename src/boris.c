#include "boris.h"
#include <math.h>
#include <stddef.h>

#define C2 (SPEED_OF_LIGHT * SPEED_OF_LIGHT)
#define BATCH_MAX 16

static double lorentz_gamma(Vec3 v)
{
    double v2 = vec3_dot(v, v);
    return 1.0 / sqrt(1.0 - v2 / C2);
}

void boris_set_species(Particle *p, int species)
{
    p->species = species;
    switch (species) {
    case SPECIES_ELECTRON: p->q = -QE; p->m = ME; break;
    case SPECIES_POSITRON: p->q =  QE; p->m = ME; break;
    case SPECIES_PROTON:   p->q =  QE; p->m = MP; break;
    case SPECIES_ALPHA:    p->q = 2*QE; p->m = 4*MP; break;
    case SPECIES_OPLUS:    p->q = QE; p->m = 16*MP; break;
    default:               break; /* custom: leave q,m as set */
    }
}

void boris_init_particle(Particle *p, int species, double E_keV,
                         double pitch_angle_deg, Vec3 pos,
                         const FieldModel *fm, int relativistic)
{
    boris_set_species(p, species);
    p->E_keV = E_keV;
    p->pitch = cos(pitch_angle_deg * M_PI / 180.0);
    p->pos = pos;

    double E_J = E_keV * KEV_TO_J;
    double v;
    if (relativistic) {
        /* E_kinetic = (gamma - 1) m c^2, so gamma = 1 + E/(mc^2) */
        double gamma = 1.0 + E_J / (p->m * C2);
        double v2 = C2 * (1.0 - 1.0 / (gamma * gamma));
        v = sqrt(v2);
    } else {
        v = sqrt(2.0 * E_J / p->m);
    }
    double v_par = v * p->pitch;
    double v_perp = v * sqrt(1.0 - p->pitch * p->pitch);

    Vec3 B = fm->eval_B(fm->params, pos);
    Vec3 bhat = vec3_norm(B);

    /* Find a perpendicular direction */
    Vec3 ref = (Vec3){0.0, 0.0, 1.0};
    if (fabs(vec3_dot(bhat, ref)) > 0.9)
        ref = (Vec3){1.0, 0.0, 0.0};
    Vec3 perp = vec3_sub(ref, vec3_scale(vec3_dot(ref, bhat), bhat));
    perp = vec3_norm(perp);

    p->vel = vec3_add(vec3_scale(v_par, bhat), vec3_scale(v_perp, perp));
}

void boris_step(Particle *p, const FieldModel *fm, double dt, int relativistic)
{
    double qom = p->q / p->m;
    double hdt = 0.5 * dt;

    if (!relativistic) {
        /* Standard non-relativistic Boris */
        Vec3 vm = p->vel;
        if (fm->eval_E) {
            Vec3 E = fm->eval_E(fm->params, p->pos);
            vm = vec3_add(vm, vec3_scale(qom * hdt, E));
        }
        Vec3 B = fm->eval_B(fm->params, p->pos);
        Vec3 t_vec = vec3_scale(qom * hdt, B);
        double t2 = vec3_dot(t_vec, t_vec);
        Vec3 vprime = vec3_add(vm, vec3_cross(vm, t_vec));
        Vec3 s_vec = vec3_scale(2.0 / (1.0 + t2), t_vec);
        Vec3 vp = vec3_add(vm, vec3_cross(vprime, s_vec));
        if (fm->eval_E) {
            Vec3 E = fm->eval_E(fm->params, p->pos);
            vp = vec3_add(vp, vec3_scale(qom * hdt, E));
        }
        p->vel = vp;
        p->pos = vec3_add(p->pos, vec3_scale(dt, p->vel));
    } else {
        /* Relativistic Boris: work with u = gamma*v */
        double gamma = lorentz_gamma(p->vel);
        Vec3 u = vec3_scale(gamma, p->vel);

        /* Half E-field kick on u */
        if (fm->eval_E) {
            Vec3 E = fm->eval_E(fm->params, p->pos);
            u = vec3_add(u, vec3_scale(qom * hdt, E));
        }

        /* Compute gamma at u_minus */
        double u2 = vec3_dot(u, u);
        double gamma_m = sqrt(1.0 + u2 / C2);

        /* Boris rotation in u-space */
        Vec3 B = fm->eval_B(fm->params, p->pos);
        Vec3 t_vec = vec3_scale(qom * hdt / gamma_m, B);
        double t2 = vec3_dot(t_vec, t_vec);
        Vec3 uprime = vec3_add(u, vec3_cross(u, t_vec));
        Vec3 s_vec = vec3_scale(2.0 / (1.0 + t2), t_vec);
        Vec3 up = vec3_add(u, vec3_cross(uprime, s_vec));

        /* Half E-field kick */
        if (fm->eval_E) {
            Vec3 E = fm->eval_E(fm->params, p->pos);
            up = vec3_add(up, vec3_scale(qom * hdt, E));
        }

        /* Convert back to 3-velocity */
        double up2 = vec3_dot(up, up);
        double gamma_new = sqrt(1.0 + up2 / C2);
        p->vel = vec3_scale(1.0 / gamma_new, up);
        p->pos = vec3_add(p->pos, vec3_scale(dt, p->vel));
    }
}

double boris_gyro_period(const Particle *p, const FieldModel *fm, int relativistic)
{
    double Bmag = field_Bmag(fm, p->pos);
    if (Bmag < 1e-30) return 1e30;
    double omega_c = fabs(p->q) * Bmag / p->m;
    if (relativistic) {
        double gamma = lorentz_gamma(p->vel);
        omega_c /= gamma;  /* relativistic gyrofrequency */
    }
    return 2.0 * M_PI / omega_c;
}

double boris_pitch_angle(const Particle *p, const FieldModel *fm)
{
    Vec3 B = fm->eval_B(fm->params, p->pos);
    double Bmag = vec3_len(B);
    if (Bmag < 1e-30) return NAN;
    Vec3 bhat = vec3_scale(1.0 / Bmag, B);
    double v = vec3_len(p->vel);
    if (v < 1e-30) return 0.0;
    double cos_alpha = vec3_dot(p->vel, bhat) / v;
    if (cos_alpha > 1.0) cos_alpha = 1.0;
    if (cos_alpha < -1.0) cos_alpha = -1.0;
    return acos(cos_alpha) * 180.0 / M_PI;
}

double boris_mu(const Particle *p, const FieldModel *fm)
{
    Vec3 B = fm->eval_B(fm->params, p->pos);
    double Bmag = vec3_len(B);
    if (Bmag < 1e-30) return 0.0;
    Vec3 bhat = vec3_scale(1.0 / Bmag, B);
    double v_par = vec3_dot(p->vel, bhat);
    double v2 = vec3_dot(p->vel, p->vel);
    double v_perp2 = v2 - v_par * v_par;
    if (v_perp2 < 0.0) v_perp2 = 0.0;
    return p->m * v_perp2 / (2.0 * Bmag);
}

double boris_energy_keV(const Particle *p, int relativistic)
{
    double v2 = vec3_dot(p->vel, p->vel);
    if (relativistic) {
        double gamma = 1.0 / sqrt(1.0 - v2 / C2);
        return (gamma - 1.0) * p->m * C2 / KEV_TO_J;
    }
    return 0.5 * p->m * v2 / KEV_TO_J;
}

double boris_radiation_step(Particle *p, const FieldModel *fm, double dt,
                            int relativistic)
{
    /* Lab-frame 3-acceleration: a = dv/dt.
     * Non-rel: a = (q/m)(E + v × B).
     * Rel: from d(γmv)/dt = q(E + v×B), and dγ/dt = qE·v/(γmc²),
     *   a = (q/(γm))(E + v×B) - (q/(γmc²))(E·v)v */
    Vec3 B = fm->eval_B(fm->params, p->pos);
    Vec3 vxB = vec3_cross(p->vel, B);
    Vec3 force = vxB;
    if (fm->eval_E) {
        Vec3 E = fm->eval_E(fm->params, p->pos);
        force = vec3_add(force, E);
    }
    Vec3 accel;
    if (relativistic) {
        double gamma = lorentz_gamma(p->vel);
        accel = vec3_scale(p->q / (gamma * p->m), force);
        if (fm->eval_E) {
            /* Subtract the (E·v)v/(γmc²) term from dγ/dt */
            Vec3 E = fm->eval_E(fm->params, p->pos);
            double Edotv = vec3_dot(E, p->vel);
            accel = vec3_sub(accel,
                vec3_scale(p->q * Edotv / (gamma * p->m * C2), p->vel));
        }
    } else {
        accel = vec3_scale(p->q / p->m, force);
    }
    double a2 = vec3_dot(accel, accel);

    double c3 = SPEED_OF_LIGHT * SPEED_OF_LIGHT * SPEED_OF_LIGHT;
    double P;

    if (relativistic) {
        /* Lienard formula: P = q^2 gamma^6 / (6 pi eps0 c^3) *
         *   [a^2 - |v x a|^2 / c^2] */
        double gamma = lorentz_gamma(p->vel);
        Vec3 vxa = vec3_cross(p->vel, accel);
        double vxa2 = vec3_dot(vxa, vxa);
        double g6 = gamma * gamma * gamma * gamma * gamma * gamma;
        P = p->q * p->q * g6 / (6.0 * M_PI * EPSILON_0 * c3) *
            (a2 - vxa2 / C2);
    } else {
        /* Larmor formula: P = q^2 a^2 / (6 pi eps0 c^3) */
        P = p->q * p->q * a2 / (6.0 * M_PI * EPSILON_0 * c3);
    }

    if (P < 0.0) P = 0.0;
    if (P < 1e-50) return P;

    /* Remove P*dt from total kinetic energy by scaling |v|.
     * The Boris dynamics handle how the loss redistributes between
     * parallel and perpendicular motion through the Lorentz force. */
    double v2 = vec3_dot(p->vel, p->vel);
    if (v2 < 1e-50) return P;

    double KE, dE = P * dt;
    if (relativistic) {
        double gamma = 1.0 / sqrt(1.0 - v2 / C2);
        KE = (gamma - 1.0) * p->m * C2;
    } else {
        KE = 0.5 * p->m * v2;
    }
    if (dE > KE) dE = KE;
    double KE_new = KE - dE;

    double v_new;
    if (relativistic) {
        double gamma_new = 1.0 + KE_new / (p->m * C2);
        v_new = sqrt(fmax(C2 * (1.0 - 1.0 / (gamma_new * gamma_new)), 0.0));
    } else {
        v_new = sqrt(2.0 * KE_new / p->m);
    }

    double scale = v_new / sqrt(v2);
    p->vel = vec3_scale(scale, p->vel);
    return P;
}

/* Batch Boris step: SoA layout with separate loops for auto-vectorization.
 * The compiler can use SSE2/AVX/NEON on each simple loop since iterations
 * are independent. Field evaluation stays scalar (function pointer). */
void boris_step_batch(Particle *particles, int n,
                      const FieldModel *fm, double dt, int relativistic)
{
    if (n <= 0) return;
    if (n > BATCH_MAX) n = BATCH_MAX;

    /* Relativistic: fall back to per-particle (transcendentals block SIMD) */
    if (relativistic) {
        for (int i = 0; i < n; i++)
            boris_step(&particles[i], fm, dt, relativistic);
        return;
    }

    /* Extract SoA from AoS */
    double px[BATCH_MAX], py[BATCH_MAX], pz[BATCH_MAX];
    double vx[BATCH_MAX], vy[BATCH_MAX], vz[BATCH_MAX];
    double Bx[BATCH_MAX], By[BATCH_MAX], Bz[BATCH_MAX];
    double Ex[BATCH_MAX], Ey[BATCH_MAX], Ez[BATCH_MAX];
    double qom[BATCH_MAX];

    for (int i = 0; i < n; i++) {
        px[i] = particles[i].pos.x;
        py[i] = particles[i].pos.y;
        pz[i] = particles[i].pos.z;
        vx[i] = particles[i].vel.x;
        vy[i] = particles[i].vel.y;
        vz[i] = particles[i].vel.z;
        qom[i] = particles[i].q / particles[i].m;
    }

    /* Field evaluation (serial: function-pointer call per particle) */
    int has_E = (fm->eval_E != NULL);
    for (int i = 0; i < n; i++) {
        Vec3 B = fm->eval_B(fm->params, (Vec3){px[i], py[i], pz[i]});
        Bx[i] = B.x; By[i] = B.y; Bz[i] = B.z;
    }
    if (has_E) {
        for (int i = 0; i < n; i++) {
            Vec3 E = fm->eval_E(fm->params, (Vec3){px[i], py[i], pz[i]});
            Ex[i] = E.x; Ey[i] = E.y; Ez[i] = E.z;
        }
    }

    double hdt = 0.5 * dt;

    /* Step 1: half E-field kick */
    if (has_E) {
        for (int i = 0; i < n; i++) {
            double f = qom[i] * hdt;
            vx[i] += f * Ex[i];
            vy[i] += f * Ey[i];
            vz[i] += f * Ez[i];
        }
    }

    /* Step 2: t-vector and t^2 */
    double tx[BATCH_MAX], ty[BATCH_MAX], tz[BATCH_MAX], t2[BATCH_MAX];
    for (int i = 0; i < n; i++) {
        double f = qom[i] * hdt;
        tx[i] = f * Bx[i];
        ty[i] = f * By[i];
        tz[i] = f * Bz[i];
        t2[i] = tx[i]*tx[i] + ty[i]*ty[i] + tz[i]*tz[i];
    }

    /* Step 3: v' = v_minus + cross(v_minus, t) */
    double vpx[BATCH_MAX], vpy[BATCH_MAX], vpz[BATCH_MAX];
    for (int i = 0; i < n; i++) {
        vpx[i] = vx[i] + (vy[i]*tz[i] - vz[i]*ty[i]);
        vpy[i] = vy[i] + (vz[i]*tx[i] - vx[i]*tz[i]);
        vpz[i] = vz[i] + (vx[i]*ty[i] - vy[i]*tx[i]);
    }

    /* Step 4: s = 2/(1+t^2) * t */
    double sx[BATCH_MAX], sy[BATCH_MAX], sz[BATCH_MAX];
    for (int i = 0; i < n; i++) {
        double sf = 2.0 / (1.0 + t2[i]);
        sx[i] = sf * tx[i];
        sy[i] = sf * ty[i];
        sz[i] = sf * tz[i];
    }

    /* Step 5: v_plus = v_minus + cross(v', s) */
    for (int i = 0; i < n; i++) {
        vx[i] += (vpy[i]*sz[i] - vpz[i]*sy[i]);
        vy[i] += (vpz[i]*sx[i] - vpx[i]*sz[i]);
        vz[i] += (vpx[i]*sy[i] - vpy[i]*sx[i]);
    }

    /* Step 6: second half E-field kick */
    if (has_E) {
        for (int i = 0; i < n; i++) {
            double f = qom[i] * hdt;
            vx[i] += f * Ex[i];
            vy[i] += f * Ey[i];
            vz[i] += f * Ez[i];
        }
    }

    /* Step 7: position update */
    for (int i = 0; i < n; i++) {
        px[i] += dt * vx[i];
        py[i] += dt * vy[i];
        pz[i] += dt * vz[i];
    }

    /* Write back SoA → AoS */
    for (int i = 0; i < n; i++) {
        particles[i].pos = (Vec3){px[i], py[i], pz[i]};
        particles[i].vel = (Vec3){vx[i], vy[i], vz[i]};
    }
}
