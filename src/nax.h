#ifndef NAX_H
#define NAX_H

#include "nax_axis.h"

#define NAX_NPHI 256

typedef struct {
    char name[64];
    AxisFourier af;
    double etabar;
    double B0;

    /* Computed on-axis arrays (functions of phi on [0, 2pi/nfp)) */
    int nphi;
    double phi[NAX_NPHI];
    double kappa[NAX_NPHI];
    double torsion[NAX_NPHI];
    double dl_dphi[NAX_NPHI];
    double sigma[NAX_NPHI];
    double iota;
    double L_per_period;  /* axis arclength per field period */

    /* First-order shaping (functions of phi) */
    double X1c[NAX_NPHI], X1s[NAX_NPHI];
    double Y1c[NAX_NPHI], Y1s[NAX_NPHI];

    /* Physical scaling */
    double R0_phys;  /* major radius in meters */
    double B0_phys;  /* field in Tesla */
} NAXConfig;

/* Initialize: compute sigma, shaping */
void nax_init(NAXConfig *cfg);

/* Evaluate B field at Cartesian point (x,y,z) in physical units */
Vec3 nax_B(const NAXConfig *cfg, Vec3 pos);

/* Find nearest axis phi for a given Cartesian point */
double nax_nearest_phi(const NAXConfig *cfg, Vec3 pos, double phi_guess);

/* Interpolate on-axis arrays at arbitrary phi */
double nax_interp(const double *arr, int nphi, int nfp, double phi);

/* Standard configs */
void nax_config_r2s51(NAXConfig *cfg);       /* Landreman-Sengupta 2019 r2 s5.1 */
void nax_config_precise_qa(NAXConfig *cfg);  /* Landreman-Paul 2022 precise QA */

#endif
