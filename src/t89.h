/* SPDX-License-Identifier: LGPL-3.0-or-later
 * Copyright 1989, 1992 N. A. Tsyganenko; C port copyright 2026 Johnathan K.
 * Burchill. See t89.c for the licence notice. */

#ifndef T89_H
#define T89_H

/* External (magnetospheric) field of T89c, GSM nT, at GSM (x, y, z) in RE,
 * for Kp bin kp (0-6, i.e. IOPT - 1) and dipole tilt ps (rad). */
void t89c(int kp, double ps, double x, double y, double z,
          double *bx, double *by, double *bz);

#endif
