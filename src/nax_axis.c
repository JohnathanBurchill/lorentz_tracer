#include "nax_axis.h"
#include <math.h>
#include <stddef.h>

void axis_eval_cyl(const AxisFourier *af, double phi,
                   double *R, double *dR, double *ddR, double *dddR,
                   double *Z, double *dZ, double *ddZ, double *dddZ)
{
    double r = 0, dr = 0, ddr = 0, dddr = 0;
    double z = 0, dz = 0, ddz = 0, dddz = 0;

    for (int n = 0; n < af->n_rc; n++) {
        double m = n * af->nfp;
        double c = cos(m * phi);
        double s = sin(m * phi);
        r    +=  af->rc[n] * c;
        dr   += -af->rc[n] * m * s;
        ddr  += -af->rc[n] * m * m * c;
        dddr +=  af->rc[n] * m * m * m * s;
    }
    for (int n = 0; n < af->n_zs; n++) {
        double m = n * af->nfp;
        double s = sin(m * phi);
        double c = cos(m * phi);
        z    +=  af->zs[n] * s;
        dz   +=  af->zs[n] * m * c;
        ddz  += -af->zs[n] * m * m * s;
        dddz += -af->zs[n] * m * m * m * c;
    }

    *R = r; *dR = dr; *ddR = ddr; *dddR = dddr;
    *Z = z; *dZ = dz; *ddZ = ddz; *dddZ = dddz;
}

Vec3 axis_pos(const AxisFourier *af, double phi)
{
    double R, dR, ddR, dddR, Z, dZ, ddZ, dddZ;
    axis_eval_cyl(af, phi, &R, &dR, &ddR, &dddR, &Z, &dZ, &ddZ, &dddZ);
    return (Vec3){R * cos(phi), R * sin(phi), Z};
}

void axis_frenet(const AxisFourier *af, double phi,
                 Vec3 *pos, Vec3 *tangent, Vec3 *normal, Vec3 *binormal,
                 double *kappa, double *torsion, double *dl_dphi)
{
    double R, dR, ddR, dddR, Z, dZ, ddZ, dddZ;
    axis_eval_cyl(af, phi, &R, &dR, &ddR, &dddR, &Z, &dZ, &ddZ, &dddZ);

    double cp = cos(phi), sp = sin(phi);

    /* Cartesian position */
    double x = R * cp, y = R * sp, z = Z;

    /* First derivative dr/dphi */
    double dx = dR*cp - R*sp;
    double dy = dR*sp + R*cp;
    double dz_val = dZ;

    /* Second derivative d^2r/dphi^2 */
    double ddx = ddR*cp - 2*dR*sp - R*cp;
    double ddy = ddR*sp + 2*dR*cp - R*sp;
    double ddz_val = ddZ;

    /* Third derivative d^3r/dphi^3 */
    double dddx = dddR*cp - 3*ddR*sp - 3*dR*cp + R*sp;
    double dddy = dddR*sp + 3*ddR*cp - 3*dR*sp - R*cp;
    double dddz_val = dddZ;

    Vec3 r1 = {dx, dy, dz_val};
    Vec3 r2 = {ddx, ddy, ddz_val};
    Vec3 r3 = {dddx, dddy, dddz_val};

    double len1 = vec3_len(r1);
    Vec3 t = vec3_scale(1.0/len1, r1);

    Vec3 cross12 = vec3_cross(r1, r2);
    double len_cross = vec3_len(cross12);

    double k = len_cross / (len1 * len1 * len1);

    Vec3 n_vec = vec3_sub(vec3_scale(1.0/(len1*len1), r2),
                          vec3_scale(vec3_dot(r1,r2)/(len1*len1*len1*len1), r1));
    double len_n = vec3_len(n_vec);
    if (len_n > 1e-15) {
        n_vec = vec3_scale(1.0/len_n, n_vec);
    }

    Vec3 b = vec3_cross(t, n_vec);

    /* Torsion: tau = (r' x r'') . r''' / |r' x r''|^2 */
    double tau = vec3_dot(cross12, r3) / (len_cross * len_cross);

    if (pos) *pos = (Vec3){x, y, z};
    if (tangent) *tangent = t;
    if (normal) *normal = n_vec;
    if (binormal) *binormal = b;
    if (kappa) *kappa = k;
    if (torsion) *torsion = tau;
    if (dl_dphi) *dl_dphi = len1;
}

double axis_length(const AxisFourier *af, int npts)
{
    double L = 0;
    double dphi = 2.0 * M_PI / npts;
    for (int i = 0; i < npts; i++) {
        double phi = i * dphi;
        double dl;
        axis_frenet(af, phi, NULL, NULL, NULL, NULL, NULL, NULL, &dl);
        L += dl * dphi;
    }
    return L;
}
