/* SPDX-License-Identifier: LGPL-3.0-or-later
 *
 * Copyright 1995, 1996 N. A. Tsyganenko (Tsyganenko96.f, IRBEM-LIB)
 * C port copyright 2026 Johnathan K. Burchill
 *
 * This file is free software: you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation, either version 3 of the License, or (at your option)
 * any later version. It is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Lesser
 * General Public License for more details. The licence texts are in
 * LICENSES/LGPL-3.0-or-later.txt and LICENSES/GPL-3.0-or-later.txt, and at
 * <https://www.gnu.org/licenses/>.
 *
 * Unlike the rest of Lorentz Tracer, which is under the MIT licence, this
 * file is under the LGPL because it is a port of LGPL code from IRBEM-LIB. */

#include "t96.h"
#include <math.h>

/* Tsyganenko (1996) data-based model of the magnetospheric field, T96_01,
 * in GSM coordinates, driven by the solar wind dynamic pressure, Dst and the
 * IMF By and Bz.
 *
 * The field is the sum of the shielded fields of the ring current, two tail
 * modes, the Region 1 and Region 2 Birkeland currents, the Chapman-Ferraro
 * (dipole shielding) field and a potential interconnection field from the
 * IMF, confined within an ellipsoid-plus-cylinder magnetopause whose size
 * scales as (Pdyn / 2 nPa)^-0.14. Across a thin boundary layer the model's
 * total field (with its own dipole) is blended into the interconnection
 * field, which is what remains outside.
 *
 * References: N. A. Tsyganenko and D. P. Stern, A new-generation global
 * magnetosphere field model, based on spacecraft magnetometer data, ISTP
 * Newsletter 6(1), 21 (1996); N. A. Tsyganenko, Modeling the Earth's
 * magnetospheric magnetic field confined within a realistic magnetopause,
 * J. Geophys. Res. 100, 5599 (1995); N. A. Tsyganenko and M. Peredo,
 * Analytical models of the magnetic field of disk-shaped current sheets,
 * J. Geophys. Res. 99, 199 (1994).
 *
 * This is a C port of Tsyganenko96.f from IRBEM-LIB (N. A. Tsyganenko's
 * release of June 22, 1996, with T. Sotirelis's corrections of April 1997).
 * Routines keep their Fortran names in lower case. The arithmetic follows the
 * Fortran operation for operation, including its three single-precision
 * literals and the multiplication chains gfortran uses for integer powers
 * (x**5 = x**2 * (x * x**2)), so results agree with the compiled Fortran to
 * rounding. IRBEM's version reads the tilt in degrees from COMMON /dip_ang/;
 * here it is an argument in radians. The Fortran's COMMON blocks of
 * constants are local constants, its /WARP/ block is a struct that tailrc96
 * passes down, and the sin/cos values that DIPOLE and R2_BIRK cache between
 * calls are recomputed, so there is no global state. */

/* The Fortran's REAL*4 literals 0.8 and 0.1666666667 */
#define SP_0_8      ((double)0.8f)
#define SP_SIXTH    ((double)0.1666666667f)

/* x^n for a run-time n >= 0, by binary powering as gfortran evaluates X**M
 * for an integer variable M (libgcc's __powidf2) */
static double ipow(double x, int n)
{
    double y = (n & 1) ? x : 1.0;
    while (n >>= 1) {
        x = x * x;
        if (n & 1) y = y * x;
    }
    return y;
}

/* ---- Earth's dipole and its shielding field ---- */

/* Dipole field (nT) with the 1980 moment and tilt ps (DIPOLE) */
static void dipole(double ps, double x, double y, double z, double b[3])
{
    double sps = sin(ps), cps = cos(ps);
    double p = x * x, u = z * z, v = 3.0 * z * x, t = y * y;
    double s = sqrt(p + t + u), s2 = s * s;
    double q = 30574.0 / (s2 * (s * s2));
    b[0] = q * ((t + u - 2.0 * p) * sps - v * cps);
    b[1] = -3.0 * y * q * (x * sps + z * cps);
    b[2] = q * ((p + t - 2.0 * u) * cps - v * sps);
}

/* Bessel functions J0 and J1: polynomial fits below 3 and asymptotic forms
 * above (BES0, BES1; BES only ever calls these two) */
static double bes0(double x)
{
    if (fabs(x) < 3.0) {
        double x32 = (x / 3.0) * (x / 3.0);
        return 1.0 - x32 * (2.2499997 - x32 * (1.2656208 - x32 * (0.3163866
               - x32 * (0.0444479 - x32 * (0.0039444 - x32 * 0.00021)))));
    }
    double xd3 = 3.0 / x;
    double f0 = 0.79788456 - xd3 * (0.00000077 + xd3 * (0.00552740 + xd3
              * (0.00009512 - xd3 * (0.00137237 - xd3 * (0.00072805
              - xd3 * 0.00014476)))));
    double t0 = x - 0.78539816 - xd3 * (0.04166397 + xd3 * (0.00003954 - xd3
              * (0.00262573 - xd3 * (0.00054125 + xd3 * (0.00029333
              - xd3 * 0.00013558)))));
    return f0 / sqrt(x) * cos(t0);
}

static double bes1(double x)
{
    if (fabs(x) < 3.0) {
        double x32 = (x / 3.0) * (x / 3.0);
        double bes1xm1 = 0.5 - x32 * (0.56249985 - x32 * (0.21093573 - x32
                       * (0.03954289 - x32 * (0.00443319 - x32 * (0.00031761
                       - x32 * 0.00001109)))));
        return bes1xm1 * x;
    }
    double xd3 = 3.0 / x;
    double f1 = 0.79788456 + xd3 * (0.00000156 + xd3 * (0.01659667 + xd3
              * (0.00017105 - xd3 * (0.00249511 - xd3 * (0.00113653
              - xd3 * 0.00020033)))));
    double t1 = x - 2.35619449 + xd3 * (0.12499612 + xd3 * (0.0000565 - xd3
              * (0.00637879 - xd3 * (0.00074348 + xd3 * (0.00079824
              - xd3 * 0.00029166)))));
    return f1 / sqrt(x) * cos(t1);
}

/* Shielding field of the perpendicular dipole as 6 cylindrical harmonics
 * about the x axis; a: amplitudes, then scale lengths (CYLHARM) */
static void cylharm(const double a[12], double x, double y, double z, double b[3])
{
    double rho = sqrt(y * y + z * z), sinfi, cosfi;
    if (rho < 1.0e-8) {
        sinfi = 1.0; cosfi = 0.0; rho = 1.0e-8;
    } else {
        sinfi = z / rho; cosfi = y / rho;
    }
    double sinfi2 = sinfi * sinfi;
    double si2co2 = sinfi2 - cosfi * cosfi;
    double bx = 0.0, by = 0.0, bz = 0.0;
    for (int i = 0; i < 3; i++) {
        double dzeta = rho / a[i + 6];
        double xj0 = bes0(dzeta), xj1 = bes1(dzeta);
        double xexp = exp(x / a[i + 6]);
        bx = bx - a[i] * xj1 * xexp * sinfi;
        by = by + a[i] * (2.0 * xj1 / dzeta - xj0) * xexp * sinfi * cosfi;
        bz = bz + a[i] * (xj1 / dzeta * si2co2 - xj0 * sinfi2) * xexp;
    }
    for (int i = 3; i < 6; i++) {
        double dzeta = rho / a[i + 6];
        double xksi = x / a[i + 6];
        double xj0 = bes0(dzeta), xj1 = bes1(dzeta);
        double xexp = exp(xksi);
        double brho = (xksi * xj0 - (dzeta * dzeta + xksi - 1.0) * xj1 / dzeta)
                    * xexp * sinfi;
        double bphi = (xj0 + xj1 / dzeta * (xksi - 1.0)) * xexp * cosfi;
        bx = bx + a[i] * (dzeta * xj0 + xksi * xj1) * xexp * sinfi;
        by = by + a[i] * (brho * cosfi - bphi * sinfi);
        bz = bz + a[i] * (brho * sinfi + bphi * cosfi);
    }
    b[0] = bx; b[1] = by; b[2] = bz;
}

/* The same for the parallel dipole (CYLHAR1) */
static void cylhar1(const double a[12], double x, double y, double z, double b[3])
{
    double rho = sqrt(y * y + z * z), sinfi, cosfi;
    if (rho < 1.0e-10) {
        sinfi = 1.0; cosfi = 0.0;
    } else {
        sinfi = z / rho; cosfi = y / rho;
    }
    double bx = 0.0, by = 0.0, bz = 0.0;
    for (int i = 0; i < 3; i++) {
        double dzeta = rho / a[i + 6];
        double xksi = x / a[i + 6];
        double xj0 = bes0(dzeta), xj1 = bes1(dzeta);
        double xexp = exp(xksi);
        double brho = xj1 * xexp;
        bx = bx - a[i] * xj0 * xexp;
        by = by + a[i] * brho * cosfi;
        bz = bz + a[i] * brho * sinfi;
    }
    for (int i = 3; i < 6; i++) {
        double dzeta = rho / a[i + 6];
        double xksi = x / a[i + 6];
        double xj0 = bes0(dzeta), xj1 = bes1(dzeta);
        double xexp = exp(xksi);
        double brho = (dzeta * xj0 + xksi * xj1) * xexp;
        bx = bx + a[i] * (dzeta * xj1 - xj0 * (xksi + 1.0)) * xexp;
        by = by + a[i] * brho * cosfi;
        bz = bz + a[i] * brho * sinfi;
    }
    b[0] = bx; b[1] = by; b[2] = bz;
}

/* Chapman-Ferraro field that shields the dipole of tilt ps (DIPSHLD) */
static void dipshld(double ps, double x, double y, double z, double b[3])
{
    static const double a1[12] = {
        0.24777, -27.003, -0.46815, 7.0637, -1.5918, -0.90317e-01,
        57.522, 13.757, 2.0100, 10.458, 4.5798, 2.1695 };
    static const double a2[12] = {
        -0.65385, -18.061, -0.40457, -5.0995, 1.2846, 0.78231e-01,
        39.592, 13.291, 1.9970, 10.062, 4.5140, 2.1558 };
    double cps = cos(ps), sps = sin(ps), h[3], f[3];
    cylharm(a1, x, y, z, h);
    cylhar1(a2, x, y, z, f);
    for (int j = 0; j < 3; j++) b[j] = h[j] * cps + f[j] * sps;
}

/* ---- IMF interconnection field ---- */

/* Potential interconnection field inside a magnetosphere of standard size
 * for a unit IMF along z, in the frame rotated by the IMF clock angle, as 9
 * Cartesian harmonics (INTERCON) */
static void intercon(double x, double y, double z, double b[3])
{
    static const double a[15] = {
        -8.411078731, 5932254.951, -9073284.93, -11.68794634,
        6027598.824, -9218378.368, -6.508798398, -11824.42793,
        18015.66212, 7.99754043, 13.9669886, 90.24475036,
        16.75728834, 1015.645781, 1553.493216 };
    double rp[3], rr[3];
    for (int i = 0; i < 3; i++) {
        rp[i] = 1.0 / a[9 + i];
        rr[i] = 1.0 / a[12 + i];
    }
    double bx = 0.0, by = 0.0, bz = 0.0;
    int l = 0;
    for (int i = 0; i < 3; i++) {
        double cypi = cos(y * rp[i]), sypi = sin(y * rp[i]);
        for (int k = 0; k < 3; k++) {
            double szrk = sin(z * rr[k]), czrk = cos(z * rr[k]);
            double sqpr = sqrt(rp[i] * rp[i] + rr[k] * rr[k]);
            double epr = exp(x * sqpr);
            double hx = -sqpr * epr * cypi * szrk;
            double hy = rp[i] * epr * sypi * szrk;
            double hz = -rr[k] * epr * cypi * czrk;
            bx = bx + a[l] * hx;
            by = by + a[l] * hy;
            bz = bz + a[l] * hz;
            l++;
        }
    }
    b[0] = bx; b[1] = by; b[2] = bz;
}

/* ---- Shielding fields of Cartesian harmonics ---- */

/* Field of n x n Cartesian harmonics of each symmetry: SHLCAR3X3 (n = 3)
 * and BIRK2SHL (n = 2). a holds 4 n^2 amplitudes in pairs, then the scales
 * P, R, Q, S (n each). The perpendicular terms go as a(l) + a(l+1) cos ps
 * and the parallel ones as (a(l) + a(l+1) sin 3ps / sin ps) sin ps. */
static void shlcar(const double *a, int n, double x, double y, double z,
                   double sps, double cps, double b[3])
{
    const double *p = a + 4 * n * n, *r = p + n, *q = r + n, *s = q + n;
    double s3ps = 4.0 * (cps * cps) - 1.0;
    double hx = 0.0, hy = 0.0, hz = 0.0;
    int l = 0;
    for (int i = 0; i < n; i++) {
        double cypi = cos(y / p[i]), sypi = sin(y / p[i]);
        for (int k = 0; k < n; k++) {
            double szrk = sin(z / r[k]), czrk = cos(z / r[k]);
            double sqpr = sqrt(1.0 / (p[i] * p[i]) + 1.0 / (r[k] * r[k]));
            double epr = exp(x * sqpr);
            double dx = -sqpr * epr * cypi * szrk;
            double dy = epr / p[i] * sypi * szrk;
            double dz = -epr / r[k] * cypi * czrk;
            hx = hx + a[l] * dx; hy = hy + a[l] * dy; hz = hz + a[l] * dz;
            dx = dx * cps; dy = dy * cps; dz = dz * cps;
            hx = hx + a[l + 1] * dx; hy = hy + a[l + 1] * dy; hz = hz + a[l + 1] * dz;
            l += 2;
        }
    }
    for (int i = 0; i < n; i++) {
        double cyqi = cos(y / q[i]), syqi = sin(y / q[i]);
        for (int k = 0; k < n; k++) {
            double czsk = cos(z / s[k]), szsk = sin(z / s[k]);
            double sqqs = sqrt(1.0 / (q[i] * q[i]) + 1.0 / (s[k] * s[k]));
            double eqs = exp(x * sqqs);
            double dx = -sps * sqqs * eqs * cyqi * czsk;
            double dy = sps * eqs / q[i] * syqi * czsk;
            double dz = sps * eqs / s[k] * cyqi * szsk;
            hx = hx + a[l] * dx; hy = hy + a[l] * dy; hz = hz + a[l] * dz;
            dx = dx * s3ps; dy = dy * s3ps; dz = dz * s3ps;
            hx = hx + a[l + 1] * dx; hy = hy + a[l + 1] * dy; hz = hz + a[l + 1] * dz;
            l += 2;
        }
    }
    b[0] = hx; b[1] = hy; b[2] = hz;
}

/* Shielding field of the Region 1 currents: 4 x 4 harmonics of each
 * symmetry, scales applied as reciprocals (BIRK1SHLD) */
static void birk1shld(double ps, double x, double y, double z, double b[3])
{
    static const double a[80] = {
        1.174198045, -1.463820502, 4.840161537, -3.674506864,
        82.18368896, -94.94071588, -4122.331796, 4670.278676,
        -21.54975037, 26.72661293, -72.81365728, 44.09887902,
        40.08073706, -51.23563510, 1955.348537, -1940.971550,
        794.0496433, -982.2441344, 1889.837171, -558.9779727,
        -1260.543238, 1260.063802, -293.5942373, 344.7250789,
        -773.7002492, 957.0094135, -1824.143669, 520.7994379,
        1192.484774, -1192.184565, 89.15537624, -98.52042999,
        -0.8168777675e-01, 0.4255969908e-01, 0.3155237661, -0.3841755213,
        2.494553332, -0.6571440817e-01, -2.765661310, 0.4331001908,
        0.1099181537, -0.6154126980e-01, -0.3258649260, 0.6698439193,
        -5.542735524, 0.1604203535, 5.854456934, -0.8323632049,
        3.732608869, -3.130002153, 107.0972607, -32.28483411,
        -115.2389298, 54.45064360, -0.5826853320, -3.582482231,
        -4.046544561, 3.311978102, -104.0839563, 30.26401293,
        97.29109008, -50.62370872, -296.3734955, 127.7872523,
        /* P(4), R(4), Q(4), S(4) */
        5.303648988, 10.40368955, 69.65230348, 466.5099509,
        1.645049286, 3.825838190, 11.66675599, 558.9781177,
        1.826531343, 2.066018073, 25.40971369, 990.2795225,
        2.319489258, 4.555148484, 9.691185703, 591.8280358 };
    double cps = cos(ps), sps = sin(ps);
    double s3ps = 4.0 * (cps * cps) - 1.0;
    double rp[4], rr[4], rq[4], rs[4];
    for (int i = 0; i < 4; i++) {
        rp[i] = 1.0 / a[64 + i];
        rr[i] = 1.0 / a[68 + i];
        rq[i] = 1.0 / a[72 + i];
        rs[i] = 1.0 / a[76 + i];
    }
    double bx = 0.0, by = 0.0, bz = 0.0;
    int l = 0;
    for (int i = 0; i < 4; i++) {
        double cypi = cos(y * rp[i]), sypi = sin(y * rp[i]);
        for (int k = 0; k < 4; k++) {
            double szrk = sin(z * rr[k]), czrk = cos(z * rr[k]);
            double sqpr = sqrt(rp[i] * rp[i] + rr[k] * rr[k]);
            double epr = exp(x * sqpr);
            double hx = -sqpr * epr * cypi * szrk;
            double hy = rp[i] * epr * sypi * szrk;
            double hz = -rr[k] * epr * cypi * czrk;
            bx = bx + a[l] * hx; by = by + a[l] * hy; bz = bz + a[l] * hz;
            hx = hx * cps; hy = hy * cps; hz = hz * cps;
            bx = bx + a[l + 1] * hx; by = by + a[l + 1] * hy; bz = bz + a[l + 1] * hz;
            l += 2;
        }
    }
    for (int i = 0; i < 4; i++) {
        double cyqi = cos(y * rq[i]), syqi = sin(y * rq[i]);
        for (int k = 0; k < 4; k++) {
            double czsk = cos(z * rs[k]), szsk = sin(z * rs[k]);
            double sqqs = sqrt(rq[i] * rq[i] + rs[k] * rs[k]);
            double eqs = exp(x * sqqs);
            double hx = -sps * sqqs * eqs * cyqi * czsk;
            double hy = sps * rq[i] * eqs * syqi * czsk;
            double hz = sps * rs[k] * eqs * cyqi * szsk;
            bx = bx + a[l] * hx; by = by + a[l] * hy; bz = bz + a[l] * hz;
            hx = hx * s3ps; hy = hy * s3ps; hz = hz * s3ps;
            bx = bx + a[l + 1] * hx; by = by + a[l + 1] * hy; bz = bz + a[l + 1] * hz;
            l += 2;
        }
    }
    b[0] = bx; b[1] = by; b[2] = bz;
}

/* ---- Ring current and tail ---- */

/* Tilt-warped coordinates of the current sheets (COMMON /WARP/), which
 * tailrc96 computes for the ring current and tail modules: rotated
 * coordinates xs and zsww (without the y-z warp) with their gradients, the
 * spread-out sheet coordinate dzetas of the tail modes with its gradient,
 * and the hinge shift rps and flank warp of the 1987 tail. */
typedef struct {
    double cpss, spss, dpsrr, rps, warp, xs, zsww;
    double dxsx, dxsy, dxsz, dzsx, dzsz;
    double dzetas, ddzetadx, ddzetady, ddzetadz;
} Warp;

/* Field of n Tsyganenko-Peredo (1994) disk-sheet terms with amplitudes f
 * (premultiplied by the scales beta) about the axis x = xshift of the warped
 * system, given the sheet coordinate dzetas and its gradient ddz: the common
 * body of RINGCURR96 (xshift = 0) and TAILDISK */
static void disk_sheet(const Warp *w, int n, const double *f, const double *beta,
                       double xshift, double dzetas, const double ddz[3],
                       double x, double y, double z, double b[3])
{
    double xsx = w->xs - xshift;
    double rhos = sqrt(xsx * xsx + y * y);
    double drh[3];
    if (rhos < 1.0e-5) {
        drh[0] = 0.0; drh[1] = copysign(1.0, y); drh[2] = 0.0;
    } else {
        drh[0] = xsx * w->dxsx / rhos;
        drh[1] = (xsx * w->dxsy + y) / rhos;
        drh[2] = xsx * w->dxsz / rhos;
    }
    double bx = 0.0, by = 0.0, bz = 0.0;
    for (int i = 0; i < n; i++) {
        double bi = beta[i];
        double s1 = sqrt((dzetas + bi) * (dzetas + bi) + (rhos + bi) * (rhos + bi));
        double s2 = sqrt((dzetas + bi) * (dzetas + bi) + (rhos - bi) * (rhos - bi));
        double ds1ddz = (dzetas + bi) / s1, ds2ddz = (dzetas + bi) / s2;
        double ds1drhos = (rhos + bi) / s1, ds2drhos = (rhos - bi) / s2;
        double s1ts2 = s1 * s2, s1ps2 = s1 + s2, s1ps2sq = s1ps2 * s1ps2;
        double fac1 = sqrt(s1ps2sq - (2.0 * bi) * (2.0 * bi));
        double as = fac1 / (s1ts2 * s1ps2sq);
        double term1 = 1.0 / (s1ts2 * s1ps2 * fac1);
        double fac2 = as / s1ps2sq;
        double dasds1 = term1 - fac2 / s1 * (s2 * s2 + s1 * (3.0 * s1 + 4.0 * s2));
        double dasds2 = term1 - fac2 / s2 * (s1 * s1 + s2 * (3.0 * s2 + 4.0 * s1));
        double das[3];
        for (int j = 0; j < 3; j++) {
            double ds1 = ds1ddz * ddz[j] + ds1drhos * drh[j];
            double ds2 = ds2ddz * ddz[j] + ds2drhos * drh[j];
            das[j] = dasds1 * ds1 + dasds2 * ds2;
        }
        bx = bx + f[i] * ((2.0 * as + y * das[1]) * w->spss - xsx * das[2]
                          + as * w->dpsrr * (y * y * w->cpss + z * w->zsww));
        /* unshifted xs here, as in the Fortran: it keeps div B = 0 */
        by = by - f[i] * y * (as * w->dpsrr * w->xs + das[2] * w->cpss + das[0] * w->spss);
        bz = bz + f[i] * ((2.0 * as + y * das[1]) * w->cpss + xsx * das[0]
                          - as * w->dpsrr * (x * w->zsww + y * y * w->spss));
    }
    b[0] = bx; b[1] = by; b[2] = bz;
}

/* Ring current, 2 terms, normalised to Bz = -1 nT at the origin, in a sheet
 * of uniform half-thickness 2 RE (D0, with DELTADX = 0) warped in x-z only
 * (RINGCURR96) */
static void ringcurr96(const Warp *w, double x, double y, double z, double b[3])
{
    static const double f[2] = {569.895366, -1603.386993};
    static const double beta[2] = {2.722188, 3.766875};
    const double D = 2.0;
    double zs = w->zsww;
    double dzsy = w->xs * y * w->dpsrr;
    double dzetas = sqrt(zs * zs + D * D);
    double ddz[3] = {zs * w->dzsx / dzetas, zs * dzsy / dzetas, zs * w->dzsz / dzetas};
    disk_sheet(w, 2, f, beta, 0.0, dzetas, ddz, x, y, z, b);
}

/* Tail current, 4 terms, axis shifted to xs = 4.5 RE (TAILDISK) */
static void taildisk(const Warp *w, double x, double y, double z, double b[3])
{
    static const double f[4] = {-745796.7338, 1176470.141, -444610.529, -57508.01028};
    static const double beta[4] = {7.9250000, 8.0850000, 8.4712500, 27.89500};
    double ddz[3] = {w->ddzetadx, w->ddzetady, w->ddzetadz};
    disk_sheet(w, 4, f, beta, 4.5, w->dzetas, ddz, x, y, z, b);
}

/* Long version of the 1987 tail model (Tsyganenko 1987, PSS 35, 1347), a
 * sheet of half-thickness 3 RE shifted by the hinge and flank warp, with
 * image sheets at z = +-40 RE; normalised to Bx = 1 at x = -200 RE (TAIL87) */
static void tail87(const Warp *w, double x, double z, double *bx, double *bz)
{
    const double DD = 3.0, HPI = 1.5707963, RT = 40.0, XN = -10.0;
    const double X1 = -1.261, X2 = -0.663;
    const double B0 = 0.391734, B1 = 5.89715, B2 = 24.6833;
    const double XN21 = 76.37, XNR = -0.1071, ADLN = 0.13238005;
    double zk[3] = {z - w->rps + w->warp, z - RT, z + RT};
    double xnx = XN - x, xnx2 = xnx * xnx;
    double xc1 = x - X1, xc2 = x - X2;
    double xc22 = xc2 * xc2, xr2 = xc2 * XNR, xc12 = xc1 * xc1;
    double d2 = DD * DD;
    double s0[3], s1[3], s2[3], g1[3], g2[3], xln1[3];
    for (int k = 0; k < 3; k++) {
        double b20 = zk[k] * zk[k] + d2;
        double b = sqrt(b20);
        double xa1 = xc12 + b20;
        double xa2 = 1.0 / (xc22 + b20);
        double xna = xnx2 + b20;
        double f = b20 - xc22;
        xln1[k] = log(XN21 / xna);
        double xln2 = xln1[k] + ADLN;
        s0[k] = (atan(xnx / b) + HPI) / b;
        s1[k] = (xln1[k] * 0.5 + xc1 * s0[k]) / xa1;
        s2[k] = (xc2 * xa2 * xln2 - XNR - f * xa2 * s0[k]) * xa2;
        g1[k] = (b20 * s0[k] - 0.5 * xc1 * xln1[k]) / xa1;
        g2[k] = ((0.5 * f * xln2 + 2.0 * s0[k] * b20 * xc2) * xa2 + xr2) * xa2;
    }
    double aln = 0.25 * (xln1[1] + xln1[2] - 2.0 * xln1[0]);
    *bx = B0 * (zk[0] * s0[0] - 0.5 * (zk[1] * s0[1] + zk[2] * s0[2]))
        + B1 * (zk[0] * s1[0] - 0.5 * (zk[1] * s1[1] + zk[2] * s1[2]))
        + B2 * (zk[0] * s2[0] - 0.5 * (zk[1] * s2[1] + zk[2] * s2[2]));
    *bz = B0 * aln + B1 * (g1[0] - 0.5 * (g1[1] + g1[2]))
        + B2 * (g2[0] - 0.5 * (g2[1] + g2[2]));
}

/* Shielded ring current (Bz = -1 nT at the origin) and the two tail modes
 * (Bx = 1 nT just above the sheet) with unit amplitudes, for sin(tilt) sps
 * (TAILRC96) */
static void tailrc96(double sps, double x, double y, double z,
                     double brc[3], double bt2[3], double bt3[3])
{
    static const double arc[48] = {
        -3.087699646, 3.516259114, 18.81380577, -13.95772338,
        -5.497076303, 0.1712890838, 2.392629189, -2.728020808,
        -14.79349936, 11.08738083, 4.388174084, 0.2492163197e-01,
        0.7030375685, -0.7966023165, -3.835041334, 2.642228681,
        -0.2405352424, -0.7297705678, -0.3680255045, 0.1333685557,
        2.795140897, -1.078379954, 0.8014028630, 0.1245825565,
        0.6149982835, -0.2207267314, -4.424578723, 1.730471572,
        -1.716313926, -0.2306302941, -0.2450342688, 0.8617173961e-01,
        1.54697858, -0.6569391113, -0.6537525353, 0.2079417515,
        12.75434981, 11.37659788, 636.4346279, 1.752483754,
        3.604231143, 12.83078674, 7.412066636, 9.434625736,
        676.7557193, 1.701162737, 3.580307144, 14.64298662 };
    static const double atail2[48] = {
        0.8747515218, -0.9116821411, 2.209365387, -2.159059518,
        -7.059828867, 5.924671028, -1.916935691, 1.996707344,
        -3.877101873, 3.947666061, 11.38715899, -8.343210833,
        1.194109867, -1.244316975, 3.73895491, -4.406522465,
        -20.66884863, 3.020952989, 0.2189908481, -0.09942543549,
        -0.927225562, 0.1555224669, 0.6994137909, -0.08111721003,
        -0.7565493881, 0.4686588792, 4.266058082, -0.3717470262,
        -3.920787807, 0.02298569870, 0.7039506341, -0.5498352719,
        -6.675140817, 0.8279283559, -2.234773608, -1.622656137,
        5.187666221, 6.802472048, 39.13543412, 2.784722096,
        6.979576616, 25.71716760, 4.495005873, 8.068408272,
        93.47887103, 4.158030104, 9.313492566, 57.18240483 };
    static const double atail3[48] = {
        -19091.95061, -3011.613928, 20582.16203, 4242.918430,
        -2377.091102, -1504.820043, 19884.04650, 2725.150544,
        -21389.04845, -3990.475093, 2401.610097, 1548.171792,
        -946.5493963, 490.1528941, 986.9156625, -489.3265930,
        -67.99278499, 8.711175710, -45.15734260, -10.76106500,
        210.7927312, 11.41764141, -178.0262808, 0.7558830028,
        339.3806753, 9.904695974, 69.50583193, -118.0271581,
        22.85935896, 45.91014857, -425.6607164, 15.47250738,
        118.2988915, 65.58594397, -201.4478068, -14.57062940,
        19.69877970, 20.30095680, 86.45407420, 22.50403727,
        23.41617329, 48.48140573, 24.61031329, 123.5395974,
        223.5367692, 39.50824342, 65.83385762, 266.2948657 };
    const double RH = 9.0, DR = 4.0, G = 10.0, D0 = 2.0, DELTADY = 10.0;
    Warp w;

    /* The sheet tilt bends from the dipole tilt near Earth to zero beyond
     * the hinging distance RH */
    double dr2 = DR * DR;
    double c11 = sqrt((1.0 + RH) * (1.0 + RH) + dr2);
    double c12 = sqrt((1.0 - RH) * (1.0 - RH) + dr2);
    double c1 = c11 - c12;
    double spsc1 = sps / c1;
    w.rps = 0.5 * (c11 + c12) * sps;
    double r = sqrt(x * x + y * y + z * z);
    double sq1 = sqrt((r + RH) * (r + RH) + dr2);
    double sq2 = sqrt((r - RH) * (r - RH) + dr2);
    double c = sq1 - sq2;
    double cs = (r + RH) / sq1 - (r - RH) / sq2;
    w.spss = spsc1 / r * c;
    w.cpss = sqrt(1.0 - w.spss * w.spss);
    w.dpsrr = sps / (r * r) * (cs * r - c)
            / sqrt((r * c1) * (r * c1) - (c * sps) * (c * sps));

    /* Warping of the flanks in y-z */
    double y2 = y * y;
    double wfac = y / (y2 * y2 + 1.0e4);
    double ww = wfac * (y * y2);
    double ws = 4.0e4 * y * (wfac * wfac);
    w.warp = G * sps * ww;
    w.xs = x * w.cpss - z * w.spss;
    w.zsww = z * w.cpss + x * w.spss;
    double zs = w.zsww + w.warp;
    w.dxsx = w.cpss - x * w.zsww * w.dpsrr;
    w.dxsy = -y * w.zsww * w.dpsrr;
    w.dxsz = -w.spss - z * w.zsww * w.dpsrr;
    w.dzsx = w.spss + x * w.xs * w.dpsrr;
    double dzsy = w.xs * y * w.dpsrr + G * sps * ws;
    w.dzsz = w.cpss + w.xs * z * w.dpsrr;

    /* Tail-mode sheet half-thickness, growing toward the flanks */
    double d = D0 + DELTADY * ((y / 20.0) * (y / 20.0));
    double dddy = DELTADY * y * 0.005;
    w.dzetas = sqrt(zs * zs + d * d);
    w.ddzetadx = zs * w.dzsx / w.dzetas;
    w.ddzetady = (zs * dzsy + d * dddy) / w.dzetas;
    w.ddzetadz = zs * w.dzsz / w.dzetas;

    double cps = sqrt(1.0 - sps * sps), s[3], h[3];
    shlcar(arc, 3, x, y, z, sps, cps, s);
    ringcurr96(&w, x, y, z, h);
    for (int j = 0; j < 3; j++) brc[j] = s[j] + h[j];

    shlcar(atail2, 3, x, y, z, sps, cps, s);
    taildisk(&w, x, y, z, h);
    for (int j = 0; j < 3; j++) bt2[j] = s[j] + h[j];

    shlcar(atail3, 3, x, y, z, sps, cps, s);
    tail87(&w, x, z, &h[0], &h[2]);
    bt3[0] = s[0] + h[0];
    bt3[1] = s[1];
    bt3[2] = s[2] + h[2];
}

/* ---- Region 1 Birkeland currents ---- */

/* Field of a circular current loop of radius rl about the z axis, with the
 * elliptic integrals from Abramowitz and Stegun's polynomial fits (CIRCLE) */
static void circle(double x, double y, double z, double rl, double b[3])
{
    double pi = atan(1.0) * 4;
    double rho2 = x * x + y * y;
    double rho = sqrt(rho2);
    double r22 = z * z + (rho + rl) * (rho + rl);
    double r2 = sqrt(r22);
    double r12 = r22 - 4.0 * rho * rl;
    double r32 = 0.5 * (r12 + r22);
    double xk2 = 1.0 - r12 / r22;
    double xk2s = 1.0 - xk2;
    double dl = log(1.0 / xk2s);
    double k = 1.38629436112 + xk2s * (0.09666344259 + xk2s * (0.03590092383
             + xk2s * (0.03742563713 + xk2s * 0.01451196212))) + dl
             * (0.5 + xk2s * (0.12498593597 + xk2s * (0.06880248576
             + xk2s * (0.03328355346 + xk2s * 0.00441787012))));
    double e = 1.0 + xk2s * (0.44325141463 + xk2s * (0.0626060122 + xk2s
             * (0.04757383546 + xk2s * 0.01736506451))) + dl
             * xk2s * (0.2499836831 + xk2s * (0.09200180037 + xk2s
             * (0.04069697526 + xk2s * 0.00526449639)));
    double brho;    /* B_rho / rho */
    if (rho > 1.0e-6)
        brho = z / (rho2 * r2) * (r32 / r12 * e - k);
    else
        brho = pi * rl / r2 * (rl - rho) / r12 * z / (r32 - rho2);
    b[0] = brho * x;
    b[1] = brho * y;
    b[2] = (k - e * (r32 - 2.0 * rl * rl) / r12) / r2;
}

/* Pair of loops of radius rl sharing the diameter along x, centred at
 * x = xc and inclined by +-al to the equator (CROSSLP) */
static void crosslp(double x, double y, double z, double xc, double rl,
                    double al, double b[3])
{
    double cal = cos(al), sal = sin(al);
    double y1 = y * cal - z * sal, z1 = y * sal + z * cal;
    double y2 = y * cal + z * sal, z2 = -y * sal + z * cal;
    double b1[3], b2[3];
    circle(x - xc, y1, z1, rl, b1);
    circle(x - xc, y2, z2, rl, b2);
    b[0] = b1[0] + b2[0];
    b[1] = (b1[1] + b2[1]) * cal + (b1[2] - b2[2]) * sal;
    b[2] = -(b1[1] - b2[1]) * sal + (b1[2] + b2[2]) * cal;
}

/* Fields of unit-moment (Earth's) dipoles along x, y and z at the origin:
 * b[j] is the field of the dipole along axis j (DIPXYZ) */
static void dipxyz(double x, double y, double z, double b[3][3])
{
    double x2 = x * x, y2 = y * y, z2 = z * z;
    double r2 = x2 + y2 + z2;
    double xmr5 = 30574.0 / (r2 * r2 * sqrt(r2));
    double xmr53 = 3.0 * xmr5;
    b[0][0] = xmr5 * (3.0 * x2 - r2);
    b[0][1] = xmr53 * x * y;
    b[0][2] = xmr53 * x * z;
    b[1][0] = b[0][1];
    b[1][1] = xmr5 * (3.0 * y2 - r2);
    b[1][2] = xmr53 * y * z;
    b[2][0] = b[0][2];
    b[2][1] = b[1][2];
    b[2][2] = xmr5 * (3.0 * z2 - r2);
}

/* Sine of the tilt of the R1 system at distance r, bending from the dipole
 * tilt near Earth toward zero beyond the hinge RH (BIRK1TOT_02, DIPLOOP1) */
static double r1_sin_tilt(double sps, double r)
{
    const double RH = 9.0, DR = 4.0;
    double dr2 = DR * DR;
    double rmrh = r - RH, rprh = r + RH;
    double sqm = sqrt(rmrh * rmrh + dr2), sqp = sqrt(rprh * rprh + dr2);
    double c = sqp - sqm;
    double q = sqrt((RH + 1.0) * (RH + 1.0) + dr2) - sqrt((RH - 1.0) * (RH - 1.0) + dr2);
    return sps / r * c / q;
}

/* Basis fields of the high-latitude R1 model: 12 pairs of dipoles inside
 * the R1 shell (z and x moments) and two loop systems (DIPLOOP1) */
static void diploop1(double x, double y, double z, double ps, double d[26][3])
{
    static const double xx[12] = {-11.0, -7.0, -7.0, -3.0, -3.0, 1.0, 1.0, 1.0,
                                  5.0, 5.0, 9.0, 9.0};
    static const double yy[12] = {2.0, 0.0, 4.0, 2.0, 6.0, 0.0, 4.0, 8.0,
                                  2.0, 6.0, 0.0, 4.0};
    /* COMMON /LOOPDIP1/ */
    const double TILT = 1.00891, XCENTRE[2] = {2.28397, -5.60831};
    const double RADIUS[2] = {1.86106, 7.83281}, DIPX = 1.12541, DIPY = 0.945719;
    double sps = sin(ps);
    for (int i = 0; i < 12; i++) {
        double r2 = (xx[i] * DIPX) * (xx[i] * DIPX) + (yy[i] * DIPY) * (yy[i] * DIPY);
        double spsas = r1_sin_tilt(sps, sqrt(r2));
        double cpsas = sqrt(1.0 - spsas * spsas);
        double xd = (xx[i] * DIPX) * cpsas;
        double yd = yy[i] * DIPY;
        double zd = -(xx[i] * DIPX) * spsas;
        double b1[3][3], b2[3][3] = {{0.0}};
        dipxyz(x - xd, y - yd, z - zd, b1);
        if (fabs(yd) > 1.0e-10) dipxyz(x - xd, y + yd, z - zd, b2);
        for (int j = 0; j < 3; j++) {
            d[i][j] = b1[2][j] + b2[2][j];
            d[i + 12][j] = (b1[0][j] + b2[0][j]) * sps;
        }
    }

    double b[3];
    double r = sqrt((XCENTRE[0] + RADIUS[0]) * (XCENTRE[0] + RADIUS[0]));
    double spsas = r1_sin_tilt(sps, r), cpsas = sqrt(1.0 - spsas * spsas);
    crosslp(x * cpsas - z * spsas, y, x * spsas + z * cpsas,
            XCENTRE[0], RADIUS[0], TILT, b);
    d[24][0] = b[0] * cpsas + b[2] * spsas;
    d[24][1] = b[1];
    d[24][2] = -b[0] * spsas + b[2] * cpsas;

    r = sqrt((RADIUS[1] - XCENTRE[1]) * (RADIUS[1] - XCENTRE[1]));
    spsas = r1_sin_tilt(sps, r);
    cpsas = sqrt(1.0 - spsas * spsas);
    circle(x * cpsas - z * spsas - XCENTRE[1], y, x * spsas + z * cpsas, RADIUS[1], b);
    d[25][0] = b[0] * cpsas + b[2] * spsas;
    d[25][1] = b[1];
    d[25][2] = -b[0] * spsas + b[2] * cpsas;
}

/* One column of CONDIP1's basis: the image-dipole fields b1 + s2 b2 + s3 b3
 * + s4 b4 rotated from SM to GSM and scaled by f */
static void condip_store(double d[3], const double b1[3], const double b2[3],
                         const double b3[3], const double b4[3],
                         double s2, double s3, double s4, double f,
                         double cps, double sps)
{
    double bx = b1[0] + s2 * b2[0] + s3 * b3[0] + s4 * b4[0];
    double by = b1[1] + s2 * b2[1] + s3 * b3[1] + s4 * b4[1];
    double bz = b1[2] + s2 * b2[2] + s3 * b3[2] + s4 * b4[2];
    d[0] = f * (bx * cps + bz * sps);
    d[1] = f * by;
    d[2] = f * (bz * cps - bx * sps);
}

/* Basis fields of the plasma-sheet R1 model: 5 conical harmonics and 37
 * dipole sets, each with a tilt-independent and a tilt-odd part (CONDIP1) */
static void condip1(double x, double y, double z, double ps, double d[79][3])
{
    /* COMMON /COORD21/ (the last five XX, YY are zero) and /DX1/ */
    static const double xx[9] = {-10.0, -7.0, -4.0, -4.0, 0.0, 4.0, 4.0, 7.0, 10.0};
    static const double yy[9] = {3.0, 6.0, 3.0, 9.0, 6.0, 3.0, 9.0, 6.0, 3.0};
    static const double zz[14] = {20.0, 20.0, 4.0, 20.0, 4.0, 4.0, 20.0, 20.0,
                                  20.0, 2.0, 3.0, 4.5, 7.0, 10.0};
    static const double zero[3] = {0.0, 0.0, 0.0};
    const double DX = -0.16, SCALEIN = 0.08, SCALEOUT = 0.4;
    double sps = sin(ps), cps = cos(ps);

    double xsm = x * cps - z * sps - DX;
    double zsm = z * cps + x * sps;
    double ro2 = xsm * xsm + y * y, ro = sqrt(ro2);
    double cf[5], sf[5];
    cf[0] = xsm / ro;
    sf[0] = y / ro;
    cf[1] = cf[0] * cf[0] - sf[0] * sf[0];
    sf[1] = 2.0 * sf[0] * cf[0];
    for (int m = 2; m < 5; m++) {
        cf[m] = cf[m - 1] * cf[0] - sf[m - 1] * sf[0];
        sf[m] = sf[m - 1] * cf[0] + cf[m - 1] * sf[0];
    }
    double r2 = ro2 + zsm * zsm, r = sqrt(r2);
    double c = zsm / r, s = ro / r;
    double ch = sqrt(0.5 * (1.0 + c)), sh = sqrt(0.5 * (1.0 - c));
    double tnh = sh / ch, cnh = 1.0 / tnh;
    for (int m = 1; m <= 5; m++) {
        double bt = m * cf[m - 1] / (r * s) * (ipow(tnh, m) + ipow(cnh, m));
        double bf = -0.5 * m * sf[m - 1] / r
                  * (ipow(tnh, m - 1) / (ch * ch) - ipow(cnh, m - 1) / (sh * sh));
        double bxsm = bt * c * cf[0] - bf * sf[0];
        double by = bt * c * sf[0] + bf * cf[0];
        double bzsm = -bt * s;
        d[m - 1][0] = bxsm * cps + bzsm * sps;
        d[m - 1][1] = by;
        d[m - 1][2] = -bxsm * sps + bzsm * cps;
    }

    xsm = x * cps - z * sps;
    zsm = z * cps + x * sps;
    double b1[3][3], b2[3][3], b3[3][3], b4[3][3];
    for (int i = 0; i < 9; i++) {
        double scale = (i == 2 || i == 4 || i == 5) ? SCALEIN : SCALEOUT;
        double xd = xx[i] * scale, yd = yy[i] * scale, zd = zz[i];
        dipxyz(xsm - xd, y - yd, zsm - zd, b1);
        dipxyz(xsm - xd, y + yd, zsm - zd, b2);
        dipxyz(xsm - xd, y - yd, zsm + zd, b3);
        dipxyz(xsm - xd, y + yd, zsm + zd, b4);
        int ix = 3 * i + 5;     /* Fortran IX = 3 I + 3 */
        condip_store(d[ix], b1[0], b2[0], b3[0], b4[0], 1, -1, -1, 1.0, cps, sps);
        condip_store(d[ix + 1], b1[1], b2[1], b3[1], b4[1], -1, -1, 1, 1.0, cps, sps);
        condip_store(d[ix + 2], b1[2], b2[2], b3[2], b4[2], 1, 1, 1, 1.0, cps, sps);
        condip_store(d[ix + 27], b1[0], b2[0], b3[0], b4[0], 1, 1, 1, sps, cps, sps);
        condip_store(d[ix + 28], b1[1], b2[1], b3[1], b4[1], -1, 1, -1, sps, cps, sps);
        condip_store(d[ix + 29], b1[2], b2[2], b3[2], b4[2], 1, -1, -1, sps, cps, sps);
    }
    for (int i = 0; i < 5; i++) {
        double zd = zz[i + 9];
        dipxyz(xsm, y, zsm - zd, b1);
        dipxyz(xsm, y, zsm + zd, b2);
        int ix = 2 * i + 59;    /* Fortran IX = 58 + 2 I */
        condip_store(d[ix], b1[0], b2[0], zero, zero, -1, 0, 0, 1.0, cps, sps);
        condip_store(d[ix + 1], b1[2], b2[2], zero, zero, 1, 0, 0, 1.0, cps, sps);
        condip_store(d[ix + 10], b1[0], b2[0], zero, zero, 1, 0, 0, sps, cps, sps);
        condip_store(d[ix + 11], b1[2], b2[2], zero, zero, -1, 0, 0, sps, cps, sps);
    }
}

/* Unshielded R1 field from the high-latitude model (diploop1 basis) */
static void r1_high(double x, double y, double z, double ps, double b[3])
{
    static const double c1[26] = {
        -0.911582e-03, -0.376654e-02, -0.727423e-02, -0.270084e-02,
        -0.123899e-02, -0.154387e-02, -0.340040e-02, -0.191858e-01,
        -0.518979e-01, 0.635061e-01, 0.440680, -0.396570,
        0.561238e-02, 0.160938e-02, -0.451229e-02, -0.251810e-02,
        -0.151599e-02, -0.133665e-02, -0.962089e-03, -0.272085e-01,
        -0.524319e-01, 0.717024e-01, 0.523439, -0.405015,
        -89.5587, 23.2806 };
    double d[26][3];
    diploop1(x, y, z, ps, d);
    b[0] = b[1] = b[2] = 0.0;
    for (int i = 0; i < 26; i++)
        for (int j = 0; j < 3; j++) b[j] = b[j] + c1[i] * d[i][j];
}

/* Unshielded R1 field from the plasma-sheet model (condip1 basis) */
static void r1_sheet(double x, double y, double z, double ps, double b[3])
{
    static const double c2[79] = {
        6.04133, 0.305415, 0.606066e-02, 0.128379e-03,
        -0.179406e-04, 1.41714, -27.2586, -4.28833,
        -1.30675, 35.5607, 8.95792, 0.961617e-03,
        -0.801477e-03, -0.782795e-03, -1.65242, -16.5242,
        -5.33798, 0.424878e-03, 0.331787e-03, -0.704305e-03,
        0.844342e-03, 0.953682e-04, 0.886271e-03, 25.1120,
        20.9299, 5.14569, -44.1670, -51.0672,
        -1.87725, 20.2998, 48.7505, -2.97415,
        3.35184, -54.2921, -0.838712, -10.5123,
        70.7594, -4.94104, 0.106166e-03, 0.465791e-03,
        -0.193719e-03, 10.8439, -29.7968, 8.08068,
        0.463507e-03, -0.224475e-04, 0.177035e-03, -0.317581e-03,
        -0.264487e-03, 0.102075e-03, 7.71390, 10.1915,
        -4.99797, -23.1114, -29.2043, 12.2928,
        10.9542, 33.6671, -9.3851, 0.174615e-03,
        -0.789777e-06, 0.686047e-03, 0.460104e-04, -0.345216e-02,
        0.221871e-02, 0.110078e-01, -0.661373e-02, 0.249201e-02,
        0.343978e-01, -0.193145e-05, 0.493963e-05, -0.535748e-04,
        0.191833e-04, -0.100496e-03, -0.210103e-03, -0.232195e-02,
        0.315335e-02, -0.134320e-01, -0.263222e-01 };
    double d[79][3];
    condip1(x, y, z, ps, d);
    b[0] = b[1] = b[2] = 0.0;
    for (int i = 0; i < 79; i++)
        for (int j = 0; j < 3; j++) b[j] = b[j] + c2[i] * d[i][j];
}

/* Region 1 field with its shielding (BIRK1TOT_02). The point is mapped
 * along dipole-like lines to an ionospheric colatitude; poleward of the R1
 * oval the high-latitude model applies, equatorward the plasma-sheet model,
 * and across the oval (the plasma sheet boundary layers, PSBL) the field is
 * interpolated between the two along the line of constant r and longitude. */
static void birk1tot_02(double ps, double x, double y, double z, double b[3])
{
    const double XLTDAY = 78.0, XLTNGHT = 70.0;   /* oval latitudes, noon and midnight */
    const double DTET0 = 0.034906;                 /* half-width of the oval */
    double tnoonn = (90.0 - XLTDAY) * 0.01745329;
    double tnoons = 3.141592654 - tnoonn;
    double dtetdn = (XLTDAY - XLTNGHT) * 0.01745329;
    double sps = sin(ps);
    double r2 = x * x + y * y + z * z;
    double r = sqrt(r2);
    double r3 = r * r2;
    double spsas = r1_sin_tilt(sps, r);
    double cpsas = sqrt(1.0 - spsas * spsas);
    double xas = x * cpsas - z * spsas;
    double zas = x * spsas + z * cpsas;
    double pas = (xas != 0.0 || y != 0.0) ? atan2(y, xas) : 0.0;
    double tas = atan2(sqrt(xas * xas + y * y), zas);
    double stas = sin(tas), stas3 = stas * (stas * stas);
    double f = stas / pow(stas3 * stas3 * (1.0 - r3) + r3, 0.1666666667);
    double tet0 = asin(f);
    if (tas > 1.5707963) tet0 = 3.141592654 - tet0;
    double sp2 = sin(pas * 0.5);
    double dtet = dtetdn * (sp2 * sp2);
    double tetr1n = tnoonn + dtet;
    double tetr1s = tnoons - dtet;

    int loc = 0;
    if (tet0 < tetr1n - DTET0 || tet0 > tetr1s + DTET0) loc = 1;   /* high latitudes */
    if (tet0 > tetr1n + DTET0 && tet0 < tetr1s - DTET0) loc = 2;   /* plasma sheet */
    if (tet0 >= tetr1n - DTET0 && tet0 <= tetr1n + DTET0) loc = 3; /* north PSBL */
    if (tet0 >= tetr1s - DTET0 && tet0 <= tetr1s + DTET0) loc = 4; /* south PSBL */

    if (loc == 1) {
        r1_high(x, y, z, ps, b);
    } else if (loc == 2) {
        r1_sheet(x, y, z, ps, b);
    } else {
        /* Boundary points at the oval's edges, poleward (1 in the north, 2
         * in the south) and equatorward; 1/6 is single precision here */
        double tc = (loc == 3) ? tetr1n : tetr1s;
        double sgn = (loc == 3) ? 1.0 : -1.0;
        double sqr = sqrt(r);
        double s01 = sin(tc - DTET0), s013 = s01 * (s01 * s01);
        double s02 = sin(tc + DTET0), s023 = s02 * (s02 * s02);
        double st01as = sqr / pow(r3 + 1.0 / (s013 * s013) - 1.0, SP_SIXTH);
        double st02as = sqr / pow(r3 + 1.0 / (s023 * s023) - 1.0, SP_SIXTH);
        double ct01as = sgn * sqrt(1.0 - st01as * st01as);
        double ct02as = sgn * sqrt(1.0 - st02as * st02as);
        double xas1 = r * st01as * cos(pas), y1 = r * st01as * sin(pas);
        double zas1 = r * ct01as;
        double x1 = xas1 * cpsas + zas1 * spsas, z1 = -xas1 * spsas + zas1 * cpsas;
        double xas2 = r * st02as * cos(pas), y2 = r * st02as * sin(pas);
        double zas2 = r * ct02as;
        double x2 = xas2 * cpsas + zas2 * spsas, z2 = -xas2 * spsas + zas2 * cpsas;
        double b1[3], b2[3];
        if (loc == 3) {
            r1_high(x1, y1, z1, ps, b1);
            r1_sheet(x2, y2, z2, ps, b2);
        } else {
            r1_sheet(x1, y1, z1, ps, b1);
            r1_high(x2, y2, z2, ps, b2);
        }
        double ss = sqrt((x2 - x1) * (x2 - x1) + (y2 - y1) * (y2 - y1) + (z2 - z1) * (z2 - z1));
        double ds = sqrt((x - x1) * (x - x1) + (y - y1) * (y - y1) + (z - z1) * (z - z1));
        double frac = ds / ss;
        for (int j = 0; j < 3; j++) b[j] = b1[j] * (1.0 - frac) + b2[j] * frac;
    }

    double bs[3];
    birk1shld(ps, x, y, z, bs);
    for (int j = 0; j < 3; j++) b[j] = b[j] + bs[j];
}

/* ---- Region 2 Birkeland currents ---- */

/* Conical harmonics of orders 1..nmax about the z axis (BCONIC) */
static void bconic(double x, double y, double z, int nmax,
                   double cbx[], double cby[], double cbz[])
{
    double ro2 = x * x + y * y, ro = sqrt(ro2);
    double cf = x / ro, sf = y / ro;
    double cfm1 = 1.0, sfm1 = 0.0;
    double r2 = ro2 + z * z, r = sqrt(r2);
    double c = z / r, s = ro / r;
    double ch = sqrt(0.5 * (1.0 + c)), sh = sqrt(0.5 * (1.0 - c));
    double tnhm1 = 1.0, cnhm1 = 1.0;
    double tnh = sh / ch, cnh = 1.0 / tnh;
    for (int m = 1; m <= nmax; m++) {
        double cfm = cfm1 * cf - sfm1 * sf;
        double sfm = cfm1 * sf + sfm1 * cf;
        cfm1 = cfm;
        sfm1 = sfm;
        double tnhm = tnhm1 * tnh, cnhm = cnhm1 * cnh;
        double bt = m * cfm / (r * s) * (tnhm + cnhm);
        double bf = -0.5 * m * sfm / r * (tnhm1 / (ch * ch) - cnhm1 / (sh * sh));
        tnhm1 = tnhm;
        cnhm1 = cnhm;
        cbx[m - 1] = bt * c * cf - bf * sf;
        cby[m - 1] = bt * c * sf + bf * cf;
        cbz[m - 1] = -bt * s;
    }
}

/* Field of a line of x-dipoles on the z axis whose moment density is a step
 * in z (mode 0) or linear in z (mode 1) (DIPDISTR) */
static void dipdistr(double x, double y, double z, int mode, double b[3])
{
    double x2 = x * x;
    double rho2 = x2 + y * y;
    double r2 = rho2 + z * z;
    double r3 = r2 * sqrt(r2);
    if (mode == 0) {
        b[0] = z / (rho2 * rho2) * (r2 * (y * y - x2) - rho2 * x2) / r3;
        b[1] = -x * y * z / (rho2 * rho2) * (2.0 * r2 + rho2) / r3;
        b[2] = x / r3;
    } else {
        b[0] = z / (rho2 * rho2) * (y * y - x2);
        b[1] = -2.0 * x * y * z / (rho2 * rho2);
        b[2] = x / rho2;
    }
}

/* One loop of loops4, its normal turned by theta from z in its local frame */
static void loop_tilted(double xs, double yss, double zs, double r, double ct,
                        double st, double *bxs, double *bys, double *bz)
{
    double xss = xs * ct - zs * st;
    double zss = zs * ct + xs * st;
    double b[3];
    circle(xss, yss, zss, r, b);
    *bxs = b[0] * ct + b[2] * st;
    *bys = b[1];
    *bz = b[2] * ct - b[0] * st;
}

/* Four loops of radius r placed symmetrically about the noon-midnight
 * meridian and the equator; the first-quadrant loop is centred at
 * (xc, yc, zc) with its normal set by theta and phi (LOOPS4) */
static void loops4(double x, double y, double z, double xc, double yc, double zc,
                   double r, double theta, double phi, double b[3])
{
    double ct = cos(theta), st = sin(theta), cp = cos(phi), sp = sin(phi);
    double bxs, bys, bz1, bz2, bz3, bz4;

    loop_tilted((x - xc) * cp + (y - yc) * sp, (y - yc) * cp - (x - xc) * sp,
                z - zc, r, ct, st, &bxs, &bys, &bz1);
    double bx1 = bxs * cp - bys * sp, by1 = bxs * sp + bys * cp;

    loop_tilted((x - xc) * cp - (y + yc) * sp, (y + yc) * cp + (x - xc) * sp,
                z - zc, r, ct, st, &bxs, &bys, &bz2);
    double bx2 = bxs * cp + bys * sp, by2 = -bxs * sp + bys * cp;

    loop_tilted(-(x - xc) * cp + (y + yc) * sp, -(y + yc) * cp - (x - xc) * sp,
                z + zc, r, ct, st, &bxs, &bys, &bz3);
    double bx3 = -bxs * cp - bys * sp, by3 = bxs * sp - bys * cp;

    loop_tilted(-(x - xc) * cp - (y - yc) * sp, -(y - yc) * cp + (x - xc) * sp,
                z + zc, r, ct, st, &bxs, &bys, &bz4);
    double bx4 = -bxs * cp + bys * sp, by4 = -bxs * sp - bys * cp;

    b[0] = bx1 + bx2 + bx3 + bx4;
    b[1] = by1 + by2 + by3 + by4;
    b[2] = bz1 + bz2 + bz3 + bz4;
}

/* R2 field earthward of the R2 sheet, in SM (R2INNER) */
static void r2inner(double x, double y, double z, double b[3])
{
    const double PL1 = 154.185, PL2 = -2.12446, PL3 = 0.601735e-01;
    const double PL4 = -0.153954e-02, PL5 = 0.355077e-04, PL6 = 29.9996;
    const double PL7 = 262.886, PL8 = 99.9132;
    const double PN1 = -8.1902, PN2 = 6.5239, PN3 = 5.504, PN4 = 7.7815;
    const double PN5 = 0.8573, PN6 = 3.0986, PN7 = 0.0774, PN8 = -0.038;
    double cbx[5], cby[5], cbz[5], d6[3], d7[3], d8[3];
    bconic(x, y, z, 5, cbx, cby, cbz);
    loops4(x, y, z, PN1, PN2, PN3, PN4, PN5, PN6, d8);
    dipdistr(x - PN7, y, z, 0, d6);
    dipdistr(x - PN8, y, z, 1, d7);
    b[0] = PL1 * cbx[0] + PL2 * cbx[1] + PL3 * cbx[2] + PL4 * cbx[3] + PL5 * cbx[4]
         + PL6 * d6[0] + PL7 * d7[0] + PL8 * d8[0];
    b[1] = PL1 * cby[0] + PL2 * cby[1] + PL3 * cby[2] + PL4 * cby[3] + PL5 * cby[4]
         + PL6 * d6[1] + PL7 * d7[1] + PL8 * d8[1];
    b[2] = PL1 * cbz[0] + PL2 * cbz[1] + PL3 * cbz[2] + PL4 * cbz[3] + PL5 * cbz[4]
         + PL6 * d6[2] + PL7 * d7[2] + PL8 * d8[2];
}

/* R2 field poleward of the R2 sheet, in SM: three crossed loop pairs, a
 * nightside equatorial loop and a 4-loop system (R2OUTER) */
static void r2outer(double x, double y, double z, double b[3])
{
    const double PL1 = -34.105, PL2 = -2.00019, PL3 = 628.639, PL4 = 73.4847;
    const double PL5 = 12.5162;
    const double PN1 = 0.55, PN2 = 0.694, PN3 = 0.0031, PN4 = 1.55, PN5 = 2.8;
    const double PN6 = 0.1375, PN7 = -0.7, PN8 = 0.2, PN9 = 0.9625;
    const double PN10 = -2.994, PN11 = 2.925, PN12 = -1.775, PN13 = 4.3;
    const double PN14 = -0.275, PN15 = 2.7, PN16 = 0.4312, PN17 = 1.55;
    double d1[3], d2[3], d3[3], d4[3], d5[3];
    crosslp(x, y, z, PN1, PN2, PN3, d1);
    crosslp(x, y, z, PN4, PN5, PN6, d2);
    crosslp(x, y, z, PN7, PN8, PN9, d3);
    circle(x - PN10, y, z, PN11, d4);
    loops4(x, y, z, PN12, PN13, PN14, PN15, PN16, PN17, d5);
    for (int j = 0; j < 3; j++)
        b[j] = PL1 * d1[j] + PL2 * d2[j] + PL3 * d3[j] + PL4 * d4[j] + PL5 * d5[j];
}

/* Coordinate across the R2 current sheet, from a stretched dipolar
 * colatitude; zero on the sheet (XKSI) */
static double xksi(double x, double y, double z)
{
    const double A11A12 = 0.305662, A21A22 = -0.383593, A41A42 = 0.2677733;
    const double A51A52 = -0.097656, A61A62 = -0.636034, B11B12 = -0.359862;
    const double B21B22 = 0.424706, C61C62 = -0.126366, C71C72 = 0.292578;
    const double R0 = 1.21563, DR = 7.50937;
    const double TNOON = 0.3665191, DTETA = 0.09599309;   /* 69 and 63.5 deg */
    double dr2 = DR * DR;
    double r2 = x * x + y * y + z * z;
    double r = sqrt(r2);
    double xr = x / r, yr = y / r, zr = z / r;
    double pr = (r < R0) ? 0.0 : sqrt((r - R0) * (r - R0) + dr2) - DR;
    double f = x + pr * (A11A12 + A21A22 * xr + A41A42 * xr * xr + A51A52 * yr * yr
                         + A61A62 * zr * zr);
    double g = y + pr * (B11B12 * yr + B21B22 * xr * yr);
    double h = z + pr * (C61C62 * zr + C71C72 * xr * zr);
    double g2 = g * g;
    double fgh = f * f + g2 + h * h;
    double sfgh = sqrt(fgh);
    double fgh32 = sfgh * (sfgh * sfgh);
    double fchsg2 = f * f + g2;
    if (fchsg2 < 1.0e-5) return -1.0;   /* avoids 0/0 on the z axis */
    double sqfchsg2 = sqrt(fchsg2);
    double alpha = fchsg2 / fgh32;
    double theta = TNOON + 0.5 * DTETA * (1.0 - f / sqfchsg2);
    double st = sin(theta);
    return alpha - st * st;
}

/* Latitude profiles of R2SHEET (FEXP, FEXP1) */
static double fexp(double s, double a)
{
    const double E = 2.718281828459;
    if (a < 0.0) return sqrt(-2.0 * a * E) * s * exp(a * s * s);
    return s * exp(a * (s * s - 1.0));
}

static double fexp1(double s, double a)
{
    if (a <= 0.0) return exp(a * s * s);
    return exp(a * (s * s - 1.0));
}

/* Smooth step from 0 to 1 over xks0 +- dxksi (TKSI) */
static double tksi(double xksi, double xks0, double dxksi)
{
    double tdz3 = 2.0 * (dxksi * (dxksi * dxksi));
    double t = 0.0;
    if (xksi - xks0 < -dxksi) t = 0.0;
    if (xksi - xks0 >= dxksi) t = 1.0;
    if (xksi >= xks0 - dxksi && xksi < xks0) {
        double b = xksi - xks0 + dxksi, br3 = b * (b * b);
        t = 1.5 * br3 / (tdz3 + br3);
    }
    if (xksi >= xks0 && xksi < xks0 + dxksi) {
        double b = xksi - xks0 - dxksi, br3 = b * (b * b);
        t = 1.0 + 1.5 * br3 / (tdz3 - br3);
    }
    return t;
}

/* Profiles across the R2 sheet for the scales p6, p7, p8 (R2SHEET's T1, T2
 * and T3) */
static void r2_profiles(double xks, double p6, double p7, double p8, double t[3])
{
    double s7 = sqrt(xks * xks + p7 * p7);
    double s8 = sqrt(xks * xks + p8 * p8), s82 = s8 * s8;
    double p82 = p8 * p8;
    t[0] = xks / sqrt(xks * xks + p6 * p6);
    t[1] = p7 * (p7 * p7) / (s7 * (s7 * s7));
    t[2] = xks / (s82 * (s8 * s82)) * 3.493856 * (p82 * p82);
}

/* One component of R2SHEET: sum over k of s[k] times the azimuthal series
 * w0 (a0 + a1 t1 + a2 t2 + a3 t3) + w1 (a4 + ...) + w2 (...) + w3 (...) */
static double r2_series(const double *a, const double s[5], const double t[3],
                        double w0, double w1, double w2, double w3)
{
    double sum = 0.0;
    for (int k = 0; k < 5; k++) {
        const double *q = a + 16 * k;
        double blk = w0 * (q[0] + q[1] * t[0] + q[2] * t[1] + q[3] * t[2])
                   + w1 * (q[4] + q[5] * t[0] + q[6] * t[1] + q[7] * t[2])
                   + w2 * (q[8] + q[9] * t[0] + q[10] * t[1] + q[11] * t[2])
                   + w3 * (q[12] + q[13] * t[0] + q[14] * t[1] + q[15] * t[2]);
        sum = sum + s[k] * blk;
    }
    return sum;
}

/* R2 field in the transition sheet, in SM (R2SHEET) */
static void r2sheet(double x, double y, double z, double b[3])
{
    /* PNONX1-8, PNONY1-8, PNONZ1-8 */
    static const double pn[3][8] = {
        {-19.0969, -9.28828, -0.129687, 5.58594, 22.5055,
         0.483750e-01, 0.396953e-01, 0.579023e-01},
        {-13.6750, -6.70625, 2.31875, 11.4062, 20.4562,
         0.478750e-01, 0.363750e-01, 0.567500e-01},
        {-16.7125, -16.4625, -0.1625, 5.1, 23.7125,
         0.355625e-01, 0.318750e-01, 0.538750e-01} };
    /* A1-A80; A7 is a single-precision literal in the Fortran */
    static const double a[80] = {
        8.07190, -7.39582, -7.62341, 0.684671,
        -13.5672, 11.6681, (double)13.1154f, -0.890217,
        7.78726, -5.38346, -8.08738, 0.609385,
        -2.70410, 3.53741, 3.15549, -1.11069,
        -8.47555, 0.278122, 2.73514, 4.55625,
        13.1134, 1.15848, -3.52648, -8.24698,
        -6.85710, -2.81369, 2.03795, 4.64383,
        2.49309, -1.22041, -1.67432, -0.422526,
        -5.39796, 7.10326, 5.53730, -13.1918,
        4.67853, -7.60329, -2.53066, 7.76338,
        5.60165, 5.34816, -4.56441, 7.05976,
        -2.62723, -0.529078, 1.42019, -2.93919,
        55.6338, -1.55181, 39.8311, -80.6561,
        -46.9655, 32.8925, -6.32296, 19.7841,
        124.731, 10.4347, -30.7581, 102.680,
        -47.4037, -3.31278, 9.37141, -50.0268,
        -533.319, 110.426, 1000.20, -1051.40,
        1619.48, 589.855, -1462.73, 1087.10,
        -1994.73, -1654.12, 1263.33, -260.210,
        1424.84, 1255.71, -956.733, 219.946 };
    static const double bb[80] = {
        -9.08427, 10.6777, 10.3288, -0.969987,
        6.45257, -8.42508, -7.97464, 1.41996,
        -1.92490, 3.93575, 2.83283, -1.48621,
        0.244033, -0.757941, -0.386557, 0.344566,
        9.56674, -2.5365, -3.32916, -5.86712,
        -6.19625, 1.83879, 2.52772, 4.34417,
        1.87268, -2.13213, -1.69134, -0.176379,
        -0.261359, 0.566419, 0.3138, -0.134699,
        -3.83086, -8.4154, 4.77005, -9.31479,
        37.5715, 19.3992, -17.9582, 36.4604,
        -14.9993, -3.1442, 6.17409, -15.5519,
        2.28621, -0.891549e-2, -0.462912, 2.47314,
        41.7555, 208.614, -45.7861, -77.8687,
        239.357, -67.9226, 66.8743, 238.534,
        -112.136, 16.2069, -40.4706, -134.328,
        21.56, -0.201725, 2.21, 32.5855,
        -108.217, -1005.98, 585.753, 323.668,
        -817.056, 235.750, -560.965, -576.892,
        684.193, 85.0275, 168.394, 477.776,
        -289.253, -123.216, 75.6501, -178.605 };
    static const double cc[80] = {
        1167.61, -917.782, -1253.2, -274.128,
        -1538.75, 1257.62, 1745.07, 113.479,
        393.326, -426.858, -641.1, 190.833,
        -29.9435, -1.04881, 117.125, -25.7663,
        -1168.16, 910.247, 1239.31, 289.515,
        1540.56, -1248.29, -1727.61, -131.785,
        -394.577, 426.163, 637.422, -187.965,
        30.0348, 0.221898, -116.68, 26.0291,
        12.6804, 4.84091, 1.18166, -2.75946,
        -17.9822, -6.80357, -1.47134, 3.02266,
        4.79648, 0.665255, -0.256229, -0.857282e-1,
        -0.588997, 0.634812e-1, 0.164303, -0.15285,
        22.2524, -22.4376, -3.85595, 6.07625,
        -105.959, -41.6698, 0.378615, 1.55958,
        44.3981, 18.8521, 3.19466, 5.89142,
        -8.63227, -2.36418, -1.027, -2.31515,
        1035.38, 2040.66, -131.881, -744.533,
        -3274.93, -4845.61, 482.438, 1567.43,
        1354.02, 2040.47, -151.653, -845.012,
        -111.723, -265.343, -26.1171, 216.632 };
    double xks = xksi(x, y, z);
    double t[3][3];
    for (int j = 0; j < 3; j++) r2_profiles(xks, pn[j][5], pn[j][6], pn[j][7], t[j]);

    double rho2 = x * x + y * y;
    double r = sqrt(rho2 + z * z);
    double rho = sqrt(rho2);
    double c1p = x / rho, s1p = y / rho;
    double s2p = 2.0 * s1p * c1p;
    double c2p = c1p * c1p - s1p * s1p;
    double s3p = s2p * c1p + c2p * s1p;
    double c3p = c2p * c1p - s2p * s1p;
    double s4p = s3p * c1p + c3p * s1p;
    double ct = z / r;

    double s[5];
    for (int k = 0; k < 5; k++) s[k] = fexp(ct, pn[0][k]);
    b[0] = r2_series(a, s, t[0], 1.0, c1p, c2p, c3p);
    for (int k = 0; k < 5; k++) s[k] = fexp(ct, pn[1][k]);
    b[1] = r2_series(bb, s, t[1], s1p, s2p, s3p, s4p);
    for (int k = 0; k < 5; k++) s[k] = fexp1(ct, pn[2][k]);
    b[2] = r2_series(cc, s, t[2], 1.0, c1p, c2p, c3p);
}

/* Unshielded R2 field and partial ring current, normalised to Bz = -1 nT at
 * x = -5.3 RE: the inner, sheet and outer models blended across the sheet
 * by tksi (R2_BIRK) */
static void r2_birk(double x, double y, double z, double ps, double b[3])
{
    const double DELARG = 0.030, DELARG1 = 0.015;
    double cps = cos(ps), sps = sin(ps);
    double xsm = x * cps - z * sps;
    double zsm = z * cps + x * sps;
    double xks = xksi(xsm, y, zsm);
    double bsm[3], b1[3], b2[3];
    if (xks < -(DELARG + DELARG1)) {
        r2outer(xsm, y, zsm, bsm);
        for (int j = 0; j < 3; j++) bsm[j] = -bsm[j] * 0.02;
    } else if (xks < -DELARG + DELARG1) {
        r2outer(xsm, y, zsm, b1);
        r2sheet(xsm, y, zsm, b2);
        double f2 = -0.02 * tksi(xks, -DELARG, DELARG1);
        double f1 = -0.02 - f2;
        for (int j = 0; j < 3; j++) bsm[j] = b1[j] * f1 + b2[j] * f2;
    } else if (xks < DELARG - DELARG1) {
        r2sheet(xsm, y, zsm, bsm);
        for (int j = 0; j < 3; j++) bsm[j] = -bsm[j] * 0.02;
    } else if (xks < DELARG + DELARG1) {
        r2inner(xsm, y, zsm, b1);
        r2sheet(xsm, y, zsm, b2);
        double f1 = -0.02 * tksi(xks, DELARG, DELARG1);
        double f2 = -0.02 - f1;
        for (int j = 0; j < 3; j++) bsm[j] = b1[j] * f1 + b2[j] * f2;
    } else {
        r2inner(xsm, y, zsm, bsm);
        for (int j = 0; j < 3; j++) bsm[j] = -bsm[j] * 0.02;
    }
    b[0] = bsm[0] * cps + bsm[2] * sps;
    b[1] = bsm[1];
    b[2] = bsm[2] * cps - bsm[0] * sps;
}

/* Shielded Region 2 field (BIRK2TOT_02) */
static void birk2tot_02(double ps, double x, double y, double z, double b[3])
{
    /* BIRK2SHL: 16 amplitudes, then P(2), R(2), Q(2), S(2) */
    static const double a[24] = {
        -111.6371348, 124.5402702, 110.3735178, -122.0095905,
        111.9448247, -129.1957743, -110.7586562, 126.5649012,
        -0.7865034384, -0.2483462721, 0.8026023894, 0.2531397188,
        10.72890902, 0.8483902118, -10.96884315, -0.8583297219,
        13.85650567, 14.90554500, 10.21914434, 10.09021632,
        6.340382460, 14.40432686, 12.71023437, 12.83966657 };
    double w[3], h[3];
    shlcar(a, 2, x, y, z, sin(ps), cos(ps), w);
    r2_birk(x, y, z, ps, h);
    for (int j = 0; j < 3; j++) b[j] = w[j] + h[j];
}

/* ---- T96_01 ---- */

/* Magnetopause coordinate of T96_01 for the pressure scale xappa: sigma is
 * the prolate spheroidal coordinate with foci at x0 - am +- am on the x
 * axis, so sigma = const is an ellipsoid of revolution, continued tailward
 * of its centre x0 - am as a cylinder. The magnetopause is sigma = S0. */
static double mp_sigma(double xappa, double x, double y, double z)
{
    const double AM0 = 70.0, X00 = 5.48;
    double x0 = X00 / xappa;
    double am = AM0 / xappa;
    double rho2 = y * y + z * z;
    double asq = am * am;
    double xmxm = am + x - x0;
    if (xmxm < 0) xmxm = 0;
    double axx0 = xmxm * xmxm;
    double aro = asq + rho2;
    double t = aro + axx0;
    return sqrt((t + sqrt(t * t - 4 * asq * axx0)) / (2 * asq));
}

#define T96_S0    1.08      /* magnetopause, sigma = S0 */
#define T96_DSIG  0.005     /* half-width of the boundary layer in sigma */

int t96_inside(const double parmod[4], double x, double y, double z)
{
    double xappa = pow(parmod[0] / 2.0, 0.14);
    return mp_sigma(xappa, x, y, z) < T96_S0 - T96_DSIG;
}

void t96_field(const double parmod[4], double ps, double x, double y, double z,
               double *bx, double *by, double *bz)
{
    static const double a[9] = {1.162, 22.344, 18.50, 2.602, 6.903, 5.287,
                                0.5790, 0.4462, 0.7850};
    const double PDYN0 = 2.0, EPS10 = 3630.7;
    const double DELIMFX = 20.0, DELIMFY = 10.0;
    double pdyn = parmod[0], dst = parmod[1];
    double byimf = parmod[2], bzimf = parmod[3];
    double sps = sin(ps);

    /* Total near-Earth depression from Dst and Pdyn (usually < 0) */
    double depr = SP_0_8 * dst - 13.0 * sqrt(pdyn);

    /* IMF clock angle and the solar wind coupling parameter eps */
    double bt = sqrt(byimf * byimf + bzimf * bzimf);
    double theta = 0.0;
    if (!(byimf == 0.0 && bzimf == 0.0)) {
        theta = atan2(byimf, bzimf);
        if (theta <= 0.0) theta = theta + atan(1.0) * 8;
    }
    double ct = cos(theta), st = sin(theta);
    double eps = 718.5 * sqrt(pdyn) * bt * sin(theta / 2);
    double facteps = eps / EPS10 - 1;
    double factpd = sqrt(pdyn / PDYN0) - 1;

    /* Amplitudes: ring current (the depression at the origin), tail modes,
     * R1 and R2 (the R2 total current is 40% of R1), and reconnection
     * efficiency */
    double rcampl = -a[0] * depr;
    double tampl2 = a[1] + a[2] * factpd + a[3] * facteps;
    double tampl3 = a[4] + a[5] * factpd;
    double b1ampl = a[6] + a[7] * facteps;
    double b2ampl = 20.0 * b1ampl;
    double reconn = a[8];

    /* Lengths scale as 1/xappa; ys, zs are rotated by the clock angle */
    double xappa = pow(pdyn / PDYN0, 0.14);
    double xappa3 = xappa * xappa * xappa;
    double ys = y * ct - z * st;
    double zs = z * ct + y * st;
    double q = ys / DELIMFY;
    double factimf = exp(x / DELIMFX - q * q);

    /* Interconnection field outside the boundary layer (with the dipole) */
    double oimf[3] = {0.0, reconn * byimf * factimf, reconn * bzimf * factimf};
    double rimfampl = reconn * bt;
    double xx = x * xappa, yy = y * xappa, zz = z * xappa;
    double sigma = mp_sigma(xappa, x, y, z);
    double qd[3];

    if (sigma < T96_S0 + T96_DSIG) {
        /* Inside the magnetopause or in the boundary layer: the full model */
        double cf[3], brc[3], bt2[3], bt3[3], r1[3], r2[3], rimf[3], f[3];
        dipshld(ps, xx, yy, zz, cf);
        tailrc96(sps, xx, yy, zz, brc, bt2, bt3);
        birk1tot_02(ps, xx, yy, zz, r1);
        birk2tot_02(ps, xx, yy, zz, r2);
        intercon(xx, ys * xappa, zs * xappa, rimf);
        double rimfy = rimf[1] * ct + rimf[2] * st;
        double rimfz = rimf[2] * ct - rimf[1] * st;
        rimf[1] = rimfy;
        rimf[2] = rimfz;
        for (int j = 0; j < 3; j++)
            f[j] = cf[j] * xappa3 + rcampl * brc[j] + tampl2 * bt2[j]
                 + tampl3 * bt3[j] + b1ampl * r1[j] + b2ampl * r2[j]
                 + rimfampl * rimf[j];

        if (sigma < T96_S0 - T96_DSIG) {
            *bx = f[0]; *by = f[1]; *bz = f[2];
        } else {
            /* Boundary layer: blend the total field (model plus dipole)
             * into the interconnection field, then remove the dipole */
            double fint = 0.5 * (1 - (sigma - T96_S0) / T96_DSIG);
            double fext = 0.5 * (1 + (sigma - T96_S0) / T96_DSIG);
            dipole(ps, x, y, z, qd);
            *bx = (f[0] + qd[0]) * fint + oimf[0] * fext - qd[0];
            *by = (f[1] + qd[1]) * fint + oimf[1] * fext - qd[1];
            *bz = (f[2] + qd[2]) * fint + oimf[2] * fext - qd[2];
        }
    } else {
        /* Outside: the interconnection field minus the dipole */
        dipole(ps, x, y, z, qd);
        *bx = oimf[0] - qd[0];
        *by = oimf[1] - qd[1];
        *bz = oimf[2] - qd[2];
    }
}
