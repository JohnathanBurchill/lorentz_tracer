#include "field.h"
#include "i18n.h"
#include "igrf14.h"
#include "t89.h"
#include "t96.h"
#include <math.h>
#include <stdio.h>
#include <string.h>

/* Earth's magnetosphere ("Terrestrial Magnetosphere"): the IGRF-14 internal
 * field plus an external field, chosen between the Tsyganenko (1989) T89c
 * model (t89.c) inside a Shue et al. (1998) magnetopause and the Tsyganenko
 * (1995, 1996) T96 model with its own magnetopause (t96.c), in GSM
 * coordinates. t89.c and t96.c are ports of LGPL code and carry its licence.
 *
 * IGRF is evaluated in geographic (GEO) coordinates with the Legendre
 * recursion of Geopack-2008 (IGRF_GSW_08), and the GEO-GSM rotation and the
 * dipole tilt follow RECALC_08 and SUN_08 for the chosen year, day and UT.
 *
 * Both apply inside the Shue et al. (1998) magnetopause, set by the solar
 * wind dynamic pressure and IMF Bz. Outside it the field is the uniform IMF,
 * with no magnetosheath compression or draping, so B jumps at the boundary
 * and particles cross it freely.
 *
 * Units: the scene is similarity-scaled so that 1 scene metre is 1 RE. The
 * Lorentz-force equation is invariant under x -> x/L, t -> t/L, B -> L B
 * with v unchanged, so orbits in RE are exact at the physical energy and
 * pitch angle; the readouts convert time, B and mu back (length_unit). */

#define RE_M      6371.2e3              /* Earth radius, m (IGRF reference) */
#define NT_SCENE  (1.0e-9 * RE_M)       /* nT -> scene tesla */
#define LOSS_ALT  100.0e3               /* particles below this altitude (m)
                                           are lost to the atmosphere */

/* The external field model, params[9] */
enum { EXT_T89, EXT_T96 };
static const char *const ext_field_names[] = {"T89c + Shue", "T96", NULL};

/* IGRF coefficients and GEO-GSM rotation for one date and time. */
typedef struct {
    double year, doy, ut;           /* parameters these were computed for */
    double g[IGRF_NCOEF], h[IGRF_NCOEF];  /* Schmidt factors folded in */
    double rec[IGRF_NCOEF];         /* Legendre recursion constants */
    double a[3][3];                 /* rows: GSM unit vectors in GEO */
    double psi;                     /* dipole tilt angle, rad */
} EarthState;

static EarthState s_es = { .year = -1.0 };

/* Greenwich sidereal time and the Sun's right ascension and declination
 * (rad), from Geopack's SUN_08 (after Russell 1971) with UT in hours. */
static void sun_position(int iyear, int iday, double ut,
                         double *gst, double *srasn, double *sdec)
{
    const double RAD = 57.295779513;
    double fday = ut / 24.0;
    double dj = 365.0 * (iyear - 1900) + (double)((iyear - 1901) / 4)
              + iday - 0.5 + fday;
    double t = dj / 36525.0;
    double vl = fmod(279.696678 + 0.9856473354 * dj, 360.0);
    *gst = fmod(279.690983 + 0.9856473354 * dj + 360.0 * fday + 180.0, 360.0) / RAD;
    double g = fmod(358.475845 + 0.985600267 * dj, 360.0) / RAD;
    double slong = (vl + (1.91946 - 0.004789 * t) * sin(g)
                    + 0.020094 * sin(2.0 * g)) / RAD;
    if (slong > 6.2831853) slong -= 6.283185307;
    if (slong < 0.0) slong += 6.283185307;
    double obliq = (23.45229 - 0.0130125 * t) / RAD;
    double sob = sin(obliq);
    double slp = slong - 9.924e-5;     /* aberration of the Earth's motion */
    double sind = sob * sin(slp);
    double cosd = sqrt(1.0 - sind * sind);
    double sc = sind / cosd;
    *sdec = atan(sc);
    *srasn = 3.141592654 - atan2(cos(obliq) / sob * sc, -cos(slp) / cosd);
}

static void earth_recalc(EarthState *e, double year, double doy, double ut)
{
    e->year = year; e->doy = doy; e->ut = ut;
    int iy = (int)floor(year + 0.5);
    int id = (int)floor(doy + 0.5);

    /* Coefficients at the decimal year, linear between the 5-year epochs */
    double u = (iy + (id - 1) / 365.25 - IGRF_EPOCH0) / IGRF_DEPOCH;
    int k = (int)floor(u);
    if (k < 0) k = 0;
    if (k > IGRF_NEPOCH - 2) k = IGRF_NEPOCH - 2;
    double f2 = u - k, f1 = 1.0 - f2;
    for (int i = 0; i < IGRF_NCOEF; i++) {
        e->g[i] = f1 * igrf14_g[k][i] + f2 * igrf14_g[k + 1][i];
        e->h[i] = f1 * igrf14_h[k][i] + f2 * igrf14_h[k + 1][i];
    }

    /* Fold the Schmidt quasi-normalization into the coefficients and set the
     * recursion constants of the associated Legendre functions (RECALC_08).
     * Index n(n+1)/2 + m is degree n, order m. */
    double s = 1.0;
    for (int n = 0; n <= IGRF_NMAX; n++) {
        int i0 = n * (n + 1) / 2;
        for (int m = 0; m <= n; m++)
            e->rec[i0 + m] = (double)((n - m) * (n + m)) / ((2 * n + 1) * (2 * n - 1));
        if (n == 0) continue;
        s *= (double)(2 * n - 1) / n;
        e->g[i0] *= s;
        e->h[i0] *= s;
        double p = s;
        for (int m = 1; m <= n; m++) {
            p *= sqrt((m == 1 ? 2.0 : 1.0) * (n - m + 1) / (n + m));
            e->g[i0 + m] *= p;
            e->h[i0 + m] *= p;
        }
    }

    /* Geodipole axis in GEO from g10, g11, h11 */
    double g10 = -e->g[1], g11 = e->g[2], h11 = e->h[2];
    double sqq = sqrt(g11 * g11 + h11 * h11);
    double sqr = sqrt(g10 * g10 + sqq * sqq);
    double sl0 = -h11 / sqq, cl0 = -g11 / sqq;
    double st0 = sqq / sqr, ct0 = g10 / sqr;

    /* GSM axes in GEI: X toward the Sun, Y along dipole x X, Z = X x Y */
    double gst, srasn, sdec;
    sun_position(iy, id, ut, &gst, &srasn, &sdec);
    double cg = cos(gst), sg = sin(gst);
    Vec3 ex = {cos(srasn) * cos(sdec), sin(srasn) * cos(sdec), sin(sdec)};
    Vec3 dip = {st0 * (cl0 * cg - sl0 * sg), st0 * (cl0 * sg + sl0 * cg), ct0};
    Vec3 ey = vec3_norm(vec3_cross(dip, ex));
    Vec3 ez = vec3_cross(ex, ey);
    e->psi = asin(vec3_dot(dip, ex));

    /* Rotate each axis from GEI to GEO (by GST about z) */
    Vec3 ax[3] = {ex, ey, ez};
    for (int i = 0; i < 3; i++) {
        e->a[i][0] = ax[i].x * cg + ax[i].y * sg;
        e->a[i][1] = -ax[i].x * sg + ax[i].y * cg;
        e->a[i][2] = ax[i].z;
    }
}

/* IGRF field (nT) at a GEO Cartesian position (RE), from IGRF_GSW_08. The
 * expansion stops at a degree that falls with distance, as in Geopack,
 * since degree n decays as r^-(n+2). Loop indices nn, mm are Geopack's
 * degree + 1 and order + 1. */
static Vec3 igrf_geo(const EarthState *e, double x, double y, double z)
{
    double rho2 = x * x + y * y;
    double r = sqrt(rho2 + z * z);
    if (r < 0.01) r = 0.01;    /* deep inside the Earth: keep it finite */
    double c = z / r;
    double rho = sqrt(rho2);
    double s = rho / r;
    double cf = 1.0, sf = 0.0;
    if (s >= 1.0e-10) { cf = x / rho; sf = y / rho; }

    int nm = 3 + 30 / (int)(r + 2.0);
    if (nm > IGRF_NMAX) nm = IGRF_NMAX;
    int k = nm + 1;
    double an[IGRF_NMAX + 2], bn[IGRF_NMAX + 2];
    double pp = 1.0 / r, p = pp;
    for (int nn = 1; nn <= k; nn++) {
        p *= pp;
        an[nn] = p;
        bn[nn] = p * nn;
    }

    double d = 0.0, bbr = 0.0, bbt = 0.0, bbf = 0.0;
    double sm = 0.0, cm = 1.0;          /* sin(m phi), cos(m phi) */
    p = 1.0;
    for (int mm = 1; mm <= k; mm++) {
        if (mm > 1) {
            double w = sm;
            sm = w * cf + cm * sf;
            cm = cm * cf - w * sf;
        }
        double q = p, zz = d, bi = 0.0, p2 = 0.0, d2 = 0.0;
        for (int nn = mm; nn <= k; nn++) {
            int i = nn * (nn - 1) / 2 + mm - 1;
            double gg = e->g[i], hh = e->h[i];
            double w = gg * cm + hh * sm;
            bbr += bn[nn] * w * q;
            bbt -= an[nn] * w * zz;
            if (mm > 1)
                bi += an[nn] * (gg * sm - hh * cm) * (s < 1.0e-10 ? zz : q);
            double dp = c * zz - s * q - e->rec[i] * d2;
            double pm = c * q - e->rec[i] * p2;
            d2 = zz; p2 = q;
            zz = dp; q = pm;
        }
        d = s * d + c * p;
        p = s * p;
        if (mm > 1) bbf += bi * (mm - 1);
    }

    double bf = (s < 1.0e-10) ? (c < 0.0 ? -bbf : bbf) : bbf / s;
    double he = bbr * s + bbt * c;
    return (Vec3){he * cf - bf * sf, he * sf + bf * cf, bbr * c - bbt * s};
}

/* Shue et al. (1998) magnetopause, r = r0 (2 / (1 + cos theta))^alpha with
 * theta from the Sun-Earth line, for IMF Bz (nT) and dynamic pressure (nPa).
 * As in Geopack's SHUETAL_MGNP_08, boundary points count as inside. */
static int inside_magnetopause(double bz, double pdyn, Vec3 pos)
{
    static double c_bz, c_pdyn = 0.0, r0, alpha;    /* pdyn 0: none cached */
    if (bz != c_bz || pdyn != c_pdyn) {
        c_bz = bz; c_pdyn = pdyn;
        r0 = (10.22 + 1.29 * tanh(0.184 * (bz + 8.14))) * pow(pdyn, -1.0 / 6.6);
        alpha = (0.58 - 0.007 * bz) * (1.0 + 0.024 * log(pdyn));
    }
    double r = vec3_len(pos);
    if (r <= r0) return 1;           /* the boundary is nowhere closer */
    return r <= r0 * pow(2.0 / (1.0 + pos.x / r), alpha);
}

static Vec3 t89_B(const double *params, Vec3 pos)
{
    if (!inside_magnetopause(params[6], params[7], pos))
        return (Vec3){params[4] * NT_SCENE, params[5] * NT_SCENE, params[6] * NT_SCENE};

    int kp = (int)floor(params[0] + 0.5);
    if (kp < 0) kp = 0;
    if (kp > 6) kp = 6;
    if (params[1] != s_es.year || params[2] != s_es.doy || params[3] != s_es.ut)
        earth_recalc(&s_es, params[1], params[2], params[3]);
    const double (*a)[3] = s_es.a;

    /* GSM -> GEO is the transpose of the GEO -> GSM rows */
    Vec3 bg = igrf_geo(&s_es,
                       a[0][0] * pos.x + a[1][0] * pos.y + a[2][0] * pos.z,
                       a[0][1] * pos.x + a[1][1] * pos.y + a[2][1] * pos.z,
                       a[0][2] * pos.x + a[1][2] * pos.y + a[2][2] * pos.z);
    double bx, by, bz;
    t89c(kp, s_es.psi, pos.x, pos.y, pos.z, &bx, &by, &bz);
    bx += a[0][0] * bg.x + a[0][1] * bg.y + a[0][2] * bg.z;
    by += a[1][0] * bg.x + a[1][1] * bg.y + a[1][2] * bg.z;
    bz += a[2][0] * bg.x + a[2][1] * bg.y + a[2][2] * bg.z;
    return (Vec3){bx * NT_SCENE, by * NT_SCENE, bz * NT_SCENE};
}

/* IGRF plus T96 everywhere, as Geopack users add them. T96 confines its
 * own field: inside its magnetopause it is the magnetospheric currents;
 * across a boundary layer it blends into, and outside it is, the model's
 * interconnection field 0.785 (0, By, Bz) exp(x/20 - (ys/10)^2) less its
 * 1980 dipole, so adding IGRF there leaves under 1 nT of IGRF minus that
 * dipole beyond about 10 RE. */
static Vec3 t96_B(const double *params, Vec3 pos)
{
    if (params[1] != s_es.year || params[2] != s_es.doy || params[3] != s_es.ut)
        earth_recalc(&s_es, params[1], params[2], params[3]);
    const double (*a)[3] = s_es.a;
    Vec3 bg = igrf_geo(&s_es,
                       a[0][0] * pos.x + a[1][0] * pos.y + a[2][0] * pos.z,
                       a[0][1] * pos.x + a[1][1] * pos.y + a[2][1] * pos.z,
                       a[0][2] * pos.x + a[1][2] * pos.y + a[2][2] * pos.z);
    /* Pdyn, Dst, IMF By, IMF Bz */
    const double parmod[4] = {params[7], params[8], params[5], params[6]};
    double bx, by, bz;
    t96_field(parmod, s_es.psi, pos.x, pos.y, pos.z, &bx, &by, &bz);
    bx += a[0][0] * bg.x + a[0][1] * bg.y + a[0][2] * bg.z;
    by += a[1][0] * bg.x + a[1][1] * bg.y + a[1][2] * bg.z;
    bz += a[2][0] * bg.x + a[2][1] * bg.y + a[2][2] * bg.z;
    return (Vec3){bx * NT_SCENE, by * NT_SCENE, bz * NT_SCENE};
}

static Vec3 earth_B(const double *params, Vec3 pos)
{
    return params[9] == EXT_T96 ? t96_B(params, pos) : t89_B(params, pos);
}

/* Inside the magnetopause of the external model in use: T96's own (where
 * it gives its full model field, not its boundary layer), or Shue's */
static int earth_inside(const double *params, Vec3 pos)
{
    if (params[9] == EXT_T96) {
        const double parmod[4] = {params[7], params[8], params[5], params[6]};
        return t96_inside(parmod, pos.x, pos.y, pos.z);
    }
    return inside_magnetopause(params[6], params[7], pos);
}

/* T89c takes Kp and, outside the Shue magnetopause, the whole IMF; T96
 * takes Dst and only IMF By and Bz */
static int earth_param_shown(const double *params, int i)
{
    int t96 = params[9] == EXT_T96;
    if (i == 0 || i == 4) return !t96;   /* Kp, IMF Bx */
    if (i == 8) return t96;              /* Dst */
    return 1;
}

/* Dipole tilt angle (rad, positive with the north pole toward the Sun) for
 * the year, day and UT in params. */
double t89_dipole_tilt(const double *params)
{
    if (params[1] != s_es.year || params[2] != s_es.doy || params[3] != s_es.ut)
        earth_recalc(&s_es, params[1], params[2], params[3]);
    return s_es.psi;
}

/* GEO -> GSM rotation for the year, day and UT in params, for drawing the
 * Earth's surface in place */
static void earth_frame(const double *params, double rot[3][3])
{
    if (params[1] != s_es.year || params[2] != s_es.doy || params[3] != s_es.ut)
        earth_recalc(&s_es, params[1], params[2], params[3]);
    memcpy(rot, s_es.a, sizeof(s_es.a));
}

void field_init_t89(FieldModel *fm)
{
    memset(fm, 0, sizeof(*fm));
    snprintf(fm->code, sizeof(fm->code), "EARTH");
    snprintf(fm->name, FIELD_MAX_NAME, "%s", TR(STR_MODEL_T89));
    fm->n_params = 10;
    snprintf(fm->param_names[0], FIELD_MAX_PNAME, "%s", TR(STR_PARAM_KP));
    fm->params[0] = 2.0;    fm->param_min[0] = 0.0;    fm->param_max[0] = 6.0;
    fm->param_int[0] = 1;
    snprintf(fm->param_names[1], FIELD_MAX_PNAME, "%s", TR(STR_PARAM_YEAR));
    fm->params[1] = 2025.0; fm->param_min[1] = 1900.0; fm->param_max[1] = 2030.0;
    fm->param_int[1] = 1;
    snprintf(fm->param_names[2], FIELD_MAX_PNAME, "%s", TR(STR_PARAM_DOY));
    fm->params[2] = 80.0;   fm->param_min[2] = 1.0;    fm->param_max[2] = 366.0;
    fm->param_int[2] = 1;
    snprintf(fm->param_names[3], FIELD_MAX_PNAME, "%s", TR(STR_PARAM_UT));
    fm->params[3] = 10.8;   fm->param_min[3] = 0.0;    fm->param_max[3] = 24.0;
    snprintf(fm->param_names[4], FIELD_MAX_PNAME, "%s", TR(STR_PARAM_IMF_BX));
    fm->params[4] = 0.0;    fm->param_min[4] = -20.0;  fm->param_max[4] = 20.0;
    snprintf(fm->param_names[5], FIELD_MAX_PNAME, "%s", TR(STR_PARAM_IMF_BY));
    fm->params[5] = 0.0;    fm->param_min[5] = -20.0;  fm->param_max[5] = 20.0;
    snprintf(fm->param_names[6], FIELD_MAX_PNAME, "%s", TR(STR_PARAM_IMF_BZ));
    fm->params[6] = -5.0;   fm->param_min[6] = -20.0;  fm->param_max[6] = 20.0;
    snprintf(fm->param_names[7], FIELD_MAX_PNAME, "%s", TR(STR_PARAM_PDYN));
    fm->params[7] = 2.0;    fm->param_min[7] = 0.5;    fm->param_max[7] = 10.0;
    /* Dst and the external model come last so saved settings keep their
     * slots */
    snprintf(fm->param_names[8], FIELD_MAX_PNAME, "%s", TR(STR_PARAM_DST));
    fm->params[8] = -10.0;  fm->param_min[8] = -300.0; fm->param_max[8] = 50.0;
    fm->param_int[8] = 1;
    snprintf(fm->param_names[9], FIELD_MAX_PNAME, "%s", TR(STR_PARAM_EXT_FIELD));
    fm->params[9] = EXT_T89;
    fm->param_max[9] = EXT_T96;
    fm->param_choices[9] = ext_field_names;
    fm->param_shown = earth_param_shown;
    fm->inside = earth_inside;
    fm->eval_B = earth_B;
    fm->eval_E = NULL;
    fm->length_unit = RE_M;
    snprintf(fm->length_name, sizeof(fm->length_name), "RE");
    /* The Earth is drawn at the top of the atmosphere, where particles are
     * lost and field-line footpoints sit, as in Geopack tracing */
    fm->body_radius = 1.0 + LOSS_ALT / RE_M;
    fm->body_texture = "textures/earth.jpg";   /* NASA Blue Marble */
    fm->body_frame = earth_frame;
    fm->default_pos = (Vec3){-10.0, 0.0, 0.0};
    fm->default_vel_dir = (Vec3){0.0, 0.0, 1.0};
    fm->default_camera_dist = 25.0;
}
