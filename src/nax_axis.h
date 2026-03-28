#ifndef NAX_AXIS_H
#define NAX_AXIS_H

#include "vec3.h"

#define AXIS_MAX_FOURIER 12

typedef struct {
    int nfp;
    int n_rc;
    int n_zs;
    double rc[AXIS_MAX_FOURIER];
    double zs[AXIS_MAX_FOURIER];
} AxisFourier;

/* Evaluate axis position R(phi), Z(phi) in cylindrical coords */
void axis_eval_cyl(const AxisFourier *af, double phi,
                   double *R, double *dR, double *ddR, double *dddR,
                   double *Z, double *dZ, double *ddZ, double *dddZ);

/* Evaluate axis position in Cartesian coords */
Vec3 axis_pos(const AxisFourier *af, double phi);

/* Evaluate axis tangent, normal, binormal (Frenet-Serret frame) */
void axis_frenet(const AxisFourier *af, double phi,
                 Vec3 *pos, Vec3 *tangent, Vec3 *normal, Vec3 *binormal,
                 double *kappa, double *torsion, double *dl_dphi);

/* Total axis length */
double axis_length(const AxisFourier *af, int npts);

#endif
