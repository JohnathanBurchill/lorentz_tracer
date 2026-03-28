#include "field.h"
#include "i18n.h"
#include <math.h>
#include <stdio.h>
#include <string.h>

/* Magnetic dipole aligned with z-axis.
 * In spherical: B_r = -2M cos(theta) / r^3, B_theta = -M sin(theta) / r^3
 * Converted to Cartesian. */

static Vec3 dipole_B(const double *params, Vec3 pos)
{
    double M = params[0]; /* dipole moment (T * m^3) */
    double x = pos.x, y = pos.y, z = pos.z;
    double r2 = x*x + y*y + z*z;
    double r = sqrt(r2);
    if (r < 1e-6) r = 1e-6;
    double r5 = r2 * r2 * r;

    /* B = (3(m·r)r - r^2 m) / r^5, with m = M*ez */
    double mdotr = M * z;
    Vec3 B;
    B.x = 3.0 * mdotr * x / r5;
    B.y = 3.0 * mdotr * y / r5;
    B.z = (3.0 * mdotr * z - r2 * M) / r5;
    return B;
}

void field_init_dipole(FieldModel *fm)
{
    memset(fm, 0, sizeof(*fm));
    snprintf(fm->code, sizeof(fm->code), "DIP");
    snprintf(fm->name, FIELD_MAX_NAME, "%s", TR(STR_MODEL_DIPOLE));
    fm->n_params = 1;
    snprintf(fm->param_names[0], FIELD_MAX_PNAME, "%s", TR(STR_PARAM_M_DIPOLE));
    fm->params[0] = 100.0; fm->param_min[0] = 1.0; fm->param_max[0] = 1000.0;
    fm->eval_B = dipole_B;
    fm->eval_E = NULL;
    fm->default_pos = (Vec3){5.0, 0.0, 0.0};
    fm->default_vel_dir = (Vec3){0.0, 0.0, 1.0};
    fm->default_camera_dist = 15.0;
}
