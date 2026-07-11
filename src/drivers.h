#ifndef DRIVERS_H
#define DRIVERS_H

#include "vec3.h"
#include "field.h"
#include "boris.h"

/* Burchill 2026 exact Lorentz-force driver channels.
 *
 * The perpendicular projection of the differentiated Lorentz force obeys,
 * exactly, (v..)_perp = -Omega0^2 [v_perp - sum_i v_i(v)], with one named
 * driver-velocity channel per member of Bdot = Bdot*bhat + B*bhatdot:
 *
 *   v_gradB    = (m/(q B0^2)) (v.gradB) v x bhat
 *                - (2 dB/B0 + dB^2/B0^2) v_perp        [field strength]
 *   v_gradbhat = (m B/(q B0^2)) [v x (v.grad)bhat]_perp [field direction]
 *
 * and the gyration remainder w~ = v_perp - v_gradB - v_gradbhat is the
 * oscillation left after the drivers. B0 is the trailing one-gyroperiod
 * average of |B| along the orbit (a smooth reference clock; dB = B - B0
 * is the parametric remainder). The gyroaverages of the two channels are
 * the gradient-B and curvature drifts. */

#define B0WIN_CAP 1024

/* Sliding one-gyroperiod window of |B| samples, tagged by accumulated
 * |gyrophase|. Degrades to a long-time mean when a full 2*pi never
 * accumulates within the buffer cap (deeply non-adiabatic). */
typedef struct {
    double psi[B0WIN_CAP];   /* cumulative |gyrophase| at each sample */
    double bmag[B0WIN_CAP];
    double sum;              /* running sum of windowed bmag samples */
    double psi_now;
    int head, count;
} B0Window;

void b0window_reset(B0Window *w);
/* Append a |B| sample after advancing the accumulated gyrophase by dpsi;
 * evicts samples older than one gyroperiod (2*pi). */
void b0window_push(B0Window *w, double bmag, double dpsi);
/* Trailing-window mean of |B| (partial-window mean during the first turn);
 * 0 if the window is empty. */
double b0window_avg(const B0Window *w);

typedef struct {
    Vec3 v_gradbhat;
    Vec3 v_gradB;
    Vec3 w_tilde;
    int valid;               /* 0 near a field null or before B0 exists */
} DriverChannels;

/* gamma*m in relativistic mode, m otherwise. */
double drivers_eff_mass(const Particle *p, int relativistic);

/* Evaluate the driver channels at the particle with reference strength B0. */
void drivers_eval(const FieldModel *fm, const Particle *p, double B0,
                  int relativistic, DriverChannels *out);

#endif
