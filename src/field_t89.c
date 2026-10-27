#include "field.h"
#include "i18n.h"
#include "igrf14.h"
#include "t96.h"
#include <math.h>
#include <stdio.h>
#include <string.h>

/* Earth's magnetosphere ("Terrestrial Magnetosphere"): the IGRF-14 internal
 * field plus an external field, chosen between the Tsyganenko (1989) T89c
 * model inside a Shue et al. (1998) magnetopause, described below, and the
 * Tsyganenko (1995, 1996) T96 model with its own magnetopause (t96.c), in
 * GSM coordinates.
 *
 * T89c models the ring current, the warped and tilt-hinged tail current
 * sheet with its closure currents, and the Chapman-Ferraro field, fitted to
 * IMP, HEOS and ISEE data in seven Kp bins (the 1992 revision adds the two
 * tilt-squared tail terms). It has no explicit magnetopause and is valid
 * inside about 70 RE. The port follows Tsyganenko's Fortran T89 subroutine,
 * with its fitting derivatives folded into the field sums.
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

/* T89c coefficients A(1)-A(30) for the seven Kp bins (IOPT = 1..7). */
static const double t89_param[7][30] = {
    { /* Kp = 0, 0+ */
      -116.53, -10719., 42.375, 59.753, -11363., 1.7844,
      30.268, -0.35372e-01, -0.66832e-01, 0.16456e-01, -1.3024, 0.16529e-02,
      0.20293e-02, 20.289, -0.25203e-01, 224.91, -9234.8, 22.788,
      7.8813, 1.8362, -0.27228, 8.8184, 2.8714, 14.468,
      32.177, 0.01, 0.0, 7.0459, 4.0, 20.0 },
    { /* Kp = 1-, 1, 1+ */
      -55.553, -13198., 60.647, 61.072, -16064., 2.2534,
      34.407, -0.38887e-01, -0.94571e-01, 0.27154e-01, -1.3901, 0.13460e-02,
      0.13238e-02, 23.005, -0.30565e-01, 55.047, -3875.7, 20.178,
      7.9693, 1.4575, 0.89471, 9.4039, 3.5215, 14.474,
      36.555, 0.01, 0.0, 7.0787, 4.0, 20.0 },
    { /* Kp = 2-, 2, 2+ */
      -101.34, -13480., 111.35, 12.386, -24699., 2.6459,
      38.948, -0.34080e-01, -0.12404, 0.29702e-01, -1.4052, 0.12103e-02,
      0.16381e-02, 24.49, -0.37705e-01, -298.32, 4400.9, 18.692,
      7.9064, 1.3047, 2.4541, 9.7012, 7.1624, 14.288,
      33.822, 0.01, 0.0, 6.7442, 4.0, 20.0 },
    { /* Kp = 3-, 3, 3+ */
      -181.69, -12320., 173.79, -96.664, -39051., 3.2633,
      44.968, -0.46377e-01, -0.16686, 0.048298, -1.5473, 0.10277e-02,
      0.31632e-02, 27.341, -0.50655e-01, -514.10, 12482., 16.257,
      8.5834, 1.0194, 3.6148, 8.6042, 5.5057, 13.778,
      32.373, 0.01, 0.0, 7.3195, 4.0, 20.0 },
    { /* Kp = 4-, 4, 4+ */
      -436.54, -9001.0, 323.66, -410.08, -50340., 3.9932,
      58.524, -0.38519e-01, -0.26822, 0.74528e-01, -1.4268, -0.10985e-02,
      0.96613e-02, 27.557, -0.56522e-01, -867.03, 20652., 14.101,
      8.3501, 0.72996, 3.8149, 9.2908, 6.4674, 13.729,
      28.353, 0.01, 0.0, 7.4237, 4.0, 20.0 },
    { /* Kp = 5-, 5, 5+ */
      -707.77, -4471.9, 432.81, -435.51, -60400., 4.6229,
      68.178, -0.88245e-01, -0.21002, 0.11846, -2.6711, 0.22305e-02,
      0.10910e-01, 27.547, -0.54080e-01, -424.23, 1100.2, 13.954,
      7.5337, 0.89714, 3.7813, 8.2945, 5.174, 14.213,
      25.237, 0.01, 0.0, 7.0037, 4.0, 20.0 },
    { /* Kp >= 6- */
      -1190.4, 2749.9, 742.56, -1110.3, -77193., 7.6727,
      102.05, -0.96015e-01, -0.74507, 0.11214, -1.3614, 0.15157e-02,
      0.22283e-01, 23.164, -0.74146e-01, -2219.1, 48253., 12.714,
      7.6777, 0.57138, 2.9633, 9.3909, 9.7263, 11.123,
      21.558, 0.01, 0.0, 4.4518, 4.0, 20.0 }
};

/* GSM field (nT) of the external current systems at (x, y, z) in RE, for
 * Kp bin kp (0-6, i.e. IOPT - 1) and dipole tilt ps (rad). */
static void t89c(int kp, double ps, double x, double y, double z,
                 double *bx, double *by, double *bz)
{
    const double *a = t89_param[kp];

    /* Shape constants fixed in the final fit */
    const double A02 = 25.0, XLW2 = 170.0, RT = 30.0;
    const double XD = 0.0, XLD2 = 40.0, SXC = 4.0, XLWC2 = 50.0;

    double dx = a[17], adr = a[18], d0 = a[19], dd = a[20], rc = a[21];
    double g = a[22], at = a[23], p = a[24], del = a[25], q = a[26];
    double sx = a[27], gam = a[28], dyc = a[29];

    /* div B = 0 fixes the remaining Chapman-Ferraro z coefficients */
    double w1 = -0.5 / dx, w2 = 2.0 * w1, w3 = -1.0 / (3.0 * dx);
    double ak610 = a[5] * w1 - 0.5 * a[9];
    double ak711 = a[6] * w2 - a[10];
    double ak812 = a[7] * w2 - 3.0 * a[11];
    double ak913 = a[8] * w3 - a[12] / 3.0;

    double tlt2 = ps * ps;
    double sps = sin(ps), cps = cos(ps);
    double x2 = x * x, y2 = y * y, z2 = z * z;
    double htp = 0.5 * sps / cps;
    double xsm = x * cps - z * sps;
    double zsm = x * sps + z * cps;

    /* Warped, tilt-hinged tail current sheet surface z = zs(x, y) */
    double xrc = xsm + rc;
    double sxrc = sqrt(xrc * xrc + 16.0);
    double y4 = y2 * y2;
    double y410 = y4 + 1.0e4;
    double sy4 = sps / y410;
    double zs1 = htp * (xrc - sxrc);
    double dzsx = -zs1 / sxrc;
    double zs = zs1 - g * sy4 * y4;
    double dzsy = -g * sy4 / y410 * 4.0e4 * y2 * y;

    /* Ring current */
    double xsm2 = xsm * xsm;
    double dsqt = sqrt(xsm2 + A02);
    double fa0 = 0.5 * (1.0 + xsm / dsqt);
    double ddr = d0 + dd * fa0;
    double dfa0 = 0.5 * A02 / (dsqt * dsqt * dsqt);
    double zr = zsm - zs;
    double tr = sqrt(zr * zr + ddr * ddr);
    double ro2 = xsm2 + y2;
    double adrt = adr + tr;
    double adrt2 = adrt * adrt;
    double fk = 1.0 / (adrt2 + ro2);
    double fc = fk * fk * sqrt(fk);
    double facxy = 3.0 * adrt * fc / tr;
    double xzr = xsm * zr;
    double yzr = y * zr;
    double xzyz = xsm * dzsx + y * dzsy;
    double faq = zr * xzyz - ddr * dd * dfa0 * xsm;
    double dbxdp = facxy * xzr;
    double dbzdp = fc * (2.0 * adrt2 - ro2) + facxy * faq;
    double rc_x = dbxdp * cps + dbzdp * sps;
    double rc_y = facxy * yzr;
    double rc_z = dbzdp * cps - dbxdp * sps;

    /* Tail current sheet, thickening toward the dayside and the flanks */
    double d = d0 + del * y2;
    double adsl = 0.0;
    if (fabs(gam) >= 1.0e-6) {
        double xxd = xsm - XD;
        double rqd = 1.0 / (xxd * xxd + XLD2);
        double rqds = sqrt(rqd);
        double h = 0.5 * (1.0 + xxd * rqds);
        double hs = 0.5 * XLD2 * rqd * rqds;
        d += gam * h;
        adsl = -d * xsm * gam * hs;
    }
    double t = sqrt(zr * zr + d * d);
    double xsmx = xsm - sx;
    double rdsq2 = 1.0 / (xsmx * xsmx + XLW2);
    double rdsq = sqrt(rdsq2);
    double v = 0.5 * (1.0 - xsmx * rdsq);
    double dvx = -0.5 * XLW2 * rdsq * rdsq2;
    double om = sqrt(sqrt(xsm2 + 16.0) - xsm);
    double oms = -0.5 * om / (om * om + xsm);
    double rdy = 1.0 / (p + q * om);
    double rdy2 = rdy * rdy;
    double fy = 1.0 / (1.0 + y2 * rdy2);
    double w = v * fy;
    double yfy1 = 2.0 * fy * y2 * rdy2;
    double fydy = yfy1 * rdy * fy;
    double dwx = dvx * fy + fydy * q * oms * v;
    double ydwy = -v * yfy1 * fy;
    double att = at + t;
    double s1 = sqrt(att * att + ro2);
    double f5 = 1.0 / s1;
    double f7 = 1.0 / (s1 + att);
    double f1 = f5 * f7;
    double f3 = f5 * f5 * f5;
    double f9 = att * f3;
    double fs = zr * xzyz - 2.0 * d * del * y2 + adsl;
    double xdwx = xsm * dwx + ydwy;
    double wt = w / t;
    double brrz1 = wt * f1, brrz2 = wt * f3;
    double dbxc1 = brrz1 * xzr, dbxc2 = brrz2 * xzr;
    double wtfs = wt * fs;
    double dbzc1 = w * f5 + xdwx * f7 + wtfs * f1;
    double dbzc2 = w * f9 + xdwx * f1 + wtfs * f3;
    /* Two tail modes, each scaled by a constant plus a tilt-squared term */
    double ct1 = a[0] + a[15] * tlt2, ct2 = a[1] + a[16] * tlt2;
    double tl_x = ct1 * (dbxc1 * cps + dbzc1 * sps) + ct2 * (dbxc2 * cps + dbzc2 * sps);
    double tl_y = ct1 * brrz1 * yzr + ct2 * brrz2 * yzr;
    double tl_z = ct1 * (dbzc1 * cps - dbxc1 * sps) + ct2 * (dbzc2 * cps - dbxc2 * sps);

    /* Closure currents of the tail sheet */
    double zpl = z + RT, zmn = z - RT;
    double rogsm2 = x2 + y2;
    double spl = sqrt(zpl * zpl + rogsm2);
    double smn = sqrt(zmn * zmn + rogsm2);
    double xsxc = x - SXC;
    double rqc2 = 1.0 / (xsxc * xsxc + XLWC2);
    double rqc = sqrt(rqc2);
    double rdyc2 = 1.0 / (dyc * dyc);
    double fyc = 1.0 / (1.0 + y2 * rdyc2);
    double wc = 0.5 * (1.0 - xsxc * rqc) * fyc;
    double dwcx = -0.5 * XLWC2 * rqc2 * rqc * fyc;
    double dwcy = -2.0 * rdyc2 * wc * fyc * y;
    double szrp = 1.0 / (spl + zpl);
    double szrm = 1.0 / (smn - zmn);
    double xywc = x * dwcx + y * dwcy;
    double wcsp = wc / spl, wcsm = wc / smn;
    double fxyp = wcsp * szrp, fxym = wcsm * szrm;
    double fxpl = x * fxyp, fxmn = -x * fxym;
    double fypl = y * fxyp, fymn = -y * fxym;
    double fzpl = wcsp + xywc * szrp, fzmn = wcsm + xywc * szrm;
    double cl_x = a[2] * (fxpl + fxmn) + a[3] * (fxpl - fxmn) * sps;
    double cl_y = a[2] * (fypl + fymn) + a[3] * (fypl - fymn) * sps;
    double cl_z = a[2] * (fzpl + fzmn) + a[3] * (fzpl - fzmn) * sps;

    /* Chapman-Ferraro and the remaining large-scale sources */
    double ex = exp(x / dx);
    double ec = ex * cps, es = ex * sps;
    double ecz = ec * z, esz = es * z, esy = es * y;
    double cf_x = a[5] * ecz + a[6] * es + a[7] * esy * y + a[8] * esz * z;
    double cf_y = a[9] * ecz * y + a[10] * esy + a[11] * esy * y2 + a[12] * esy * z2;
    double cf_z = a[13] * ec + a[14] * ec * y2 + ak610 * ecz * z + ak711 * esz
                + ak812 * esz * y2 + ak913 * esz * z2;

    *bx = tl_x + cl_x + a[4] * rc_x + cf_x;
    *by = tl_y + cl_y + a[4] * rc_y + cf_y;
    *bz = tl_z + cl_z + a[4] * rc_z + cf_z;
}

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
