#include "field.h"
#include "i18n.h"
#include <math.h>
#include <stdio.h>
#include <string.h>

/* Uniform background field of adjustable magnitude and orientation plus a
 * static isotropic turbulent fluctuation:
 *
 *   B(x) = B0 b0  +  dB(x)
 *   dB(x) = sum_n A_n e_n cos(k_n . x + phi_n),   with  e_n . k_n = 0
 *
 * The wavevector directions are uniform on the sphere (isotropic) and the
 * magnitudes lie on a logarithmic grid between k1 and k2. Amplitudes follow
 * an omnidirectional power spectrum P(k) ~ k^-s over that band (s = 5/3 is
 * Kolmogorov, 3/2 Kraichnan, 2 shock-dominated), normalized so that
 * <|dB|^2> = (db B0)^2 with db = dB_rms/B0. This is the Giacalone & Jokipii
 * (1999) synthetic-turbulence construction. Every polarization e_n is
 * perpendicular to its own k_n, so div dB = 0 exactly.
 *
 * The realization is frozen (no time dependence) and there is no E field, so
 * particle energy is conserved to machine precision and the turbulence
 * scatters pitch angle only. */

#define TURB_NMODES 64

typedef struct {
    Vec3 k;         /* wavevector (rad/m) */
    Vec3 e;         /* unit polarization, perpendicular to k */
    double amp;     /* amplitude for unit dB_rms */
    double phase;
} TurbMode;

static TurbMode s_modes[TURB_NMODES];
static int s_have_modes = 0;
static double s_cached_s = 0.0, s_cached_k1 = 0.0, s_cached_k2 = 0.0;
static int s_cached_seed = -1;

/* Deterministic LCG so a given seed reproduces the same realization on
 * every platform and every run. */
static unsigned int s_rng;

static double urand(void)
{
    s_rng = s_rng * 1664525u + 1013904223u;
    return (double)(s_rng >> 8) / 16777216.0;   /* [0, 1) */
}

static void build_modes(double s, double k1, double k2, int seed)
{
    if (k1 < 1e-6) k1 = 1e-6;
    if (k2 < k1 * 1.001) k2 = k1 * 1.001;

    s_rng = 12345u + 2654435761u * (unsigned int)seed;
    for (int i = 0; i < 8; i++) urand();     /* discard the first few */

    double dlnk = log(k2 / k1) / (TURB_NMODES - 1);
    double wsum = 0.0;

    for (int n = 0; n < TURB_NMODES; n++) {
        double kmag = k1 * exp(n * dlnk);
        /* variance carried by this mode: P(k) dk ~ k^-s (k dlnk) */
        double w = pow(kmag, 1.0 - s);
        wsum += w;

        /* Isotropic wavevector direction */
        double mu = 2.0 * urand() - 1.0;
        double sm = sqrt(1.0 - mu * mu);
        double az = 2.0 * M_PI * urand();
        Vec3 khat = {sm * cos(az), sm * sin(az), mu};

        /* Random polarization in the plane perpendicular to k */
        Vec3 ref = (fabs(khat.z) < 0.9) ? (Vec3){0.0, 0.0, 1.0} : (Vec3){1.0, 0.0, 0.0};
        Vec3 e1 = vec3_norm(vec3_cross(khat, ref));
        Vec3 e2 = vec3_cross(khat, e1);
        double pol = 2.0 * M_PI * urand();

        s_modes[n].k = vec3_scale(kmag, khat);
        s_modes[n].e = vec3_add(vec3_scale(cos(pol), e1), vec3_scale(sin(pol), e2));
        s_modes[n].amp = w;                  /* weight; normalized below */
        s_modes[n].phase = 2.0 * M_PI * urand();
    }

    /* <|dB|^2> = sum A_n^2 / 2 = 1  =>  A_n = sqrt(2 w_n / sum w) */
    for (int n = 0; n < TURB_NMODES; n++)
        s_modes[n].amp = sqrt(2.0 * s_modes[n].amp / wsum);

    s_have_modes = 1;
}

static Vec3 turbulence_B(const double *params, Vec3 pos)
{
    double B0    = params[0];
    double theta = params[1] * (M_PI / 180.0);
    double phi   = params[2] * (M_PI / 180.0);
    double db    = params[3];
    double s     = params[4];
    double k1    = params[5];
    double k2    = params[6];
    int    seed  = (int)(params[7] + 0.5);

    if (!s_have_modes || s != s_cached_s || k1 != s_cached_k1 ||
        k2 != s_cached_k2 || seed != s_cached_seed) {
        build_modes(s, k1, k2, seed);
        s_cached_s = s; s_cached_k1 = k1; s_cached_k2 = k2; s_cached_seed = seed;
    }

    double st = sin(theta);
    Vec3 B = {B0 * st * cos(phi), B0 * st * sin(phi), B0 * cos(theta)};

    double a = db * B0;
    if (a != 0.0) {
        for (int n = 0; n < TURB_NMODES; n++) {
            double c = a * s_modes[n].amp *
                       cos(vec3_dot(s_modes[n].k, pos) + s_modes[n].phase);
            B.x += c * s_modes[n].e.x;
            B.y += c * s_modes[n].e.y;
            B.z += c * s_modes[n].e.z;
        }
    }
    return B;
}

void field_init_turbulence(FieldModel *fm)
{
    memset(fm, 0, sizeof(*fm));
    snprintf(fm->code, sizeof(fm->code), "TRB");
    snprintf(fm->name, FIELD_MAX_NAME, "%s", TR(STR_MODEL_TURBULENCE));
    fm->n_params = 8;
    snprintf(fm->param_names[0], FIELD_MAX_PNAME, "%s", TR(STR_PARAM_B0));
    fm->params[0] = 1.0;      fm->param_min[0] = 0.01; fm->param_max[0] = 2.0;
    snprintf(fm->param_names[1], FIELD_MAX_PNAME, "%s", TR(STR_PARAM_B0_THETA));
    fm->params[1] = 0.0;      fm->param_min[1] = 0.0;  fm->param_max[1] = 180.0;
    snprintf(fm->param_names[2], FIELD_MAX_PNAME, "%s", TR(STR_PARAM_B0_PHI));
    fm->params[2] = 0.0;      fm->param_min[2] = 0.0;  fm->param_max[2] = 360.0;
    snprintf(fm->param_names[3], FIELD_MAX_PNAME, "%s", TR(STR_PARAM_DB_B0));
    fm->params[3] = 0.3;      fm->param_min[3] = 0.0;  fm->param_max[3] = 2.0;
    snprintf(fm->param_names[4], FIELD_MAX_PNAME, "%s", TR(STR_PARAM_SPEC_S));
    fm->params[4] = 5.0 / 3.0; fm->param_min[4] = 0.0; fm->param_max[4] = 4.0;
    /* k1, k2 span several decades, so their sliders are logarithmic. */
    snprintf(fm->param_names[5], FIELD_MAX_PNAME, "%s", TR(STR_PARAM_K1));
    fm->params[5] = 0.5;      fm->param_min[5] = 0.01; fm->param_max[5] = 50.0;
    fm->param_log[5] = 1;
    snprintf(fm->param_names[6], FIELD_MAX_PNAME, "%s", TR(STR_PARAM_K2));
    fm->params[6] = 10.0;     fm->param_min[6] = 0.05; fm->param_max[6] = 1000.0;
    fm->param_log[6] = 1;
    snprintf(fm->param_names[7], FIELD_MAX_PNAME, "%s", TR(STR_PARAM_SEED));
    fm->params[7] = 1.0;      fm->param_min[7] = 1.0;  fm->param_max[7] = 64.0;
    fm->eval_B = turbulence_B;
    fm->eval_E = NULL;
    fm->default_pos = (Vec3){0.0, 0.0, 0.0};
    fm->default_vel_dir = (Vec3){1.0, 0.0, 0.0};
    fm->default_camera_dist = 12.0;
}
