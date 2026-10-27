/* SPDX-License-Identifier: LGPL-3.0-or-later
 *
 * Copyright 1989, 1992 N. A. Tsyganenko (Tsyganenko89.f, IRBEM-LIB)
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

#include "t89.h"
#include <math.h>

/* Tsyganenko (1989) data-based model of the magnetospheric field, T89c, in
 * GSM coordinates.
 *
 * T89c models the ring current, the warped and tilt-hinged tail current
 * sheet with its closure currents, and the Chapman-Ferraro field, fitted to
 * IMP, HEOS and ISEE data in seven Kp bins (the 1992 revision adds the two
 * tilt-squared tail terms). It has no explicit magnetopause and is valid
 * inside about 70 RE.
 *
 * This is a C port of the T89 subroutine in Tsyganenko89.f from IRBEM-LIB,
 * with its fitting derivatives folded into the field sums. */

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
void t89c(int kp, double ps, double x, double y, double z,
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
