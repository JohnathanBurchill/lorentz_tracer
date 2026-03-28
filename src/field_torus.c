#include "field.h"
#include "i18n.h"
#include <math.h>
#include <stdio.h>
#include <string.h>

/* Toroidal solenoid: B_phi = B0 * R0 / R inside the torus,
 * zero outside. Torus defined by major radius R0 and minor radius a.
 * Particles exhibit curvature + grad-B drift vertically (along z). */

static Vec3 torus_B(const double *params, Vec3 pos)
{
    double B0 = params[0];
    double R0 = params[1];
    double a  = params[2];

    double R = sqrt(pos.x * pos.x + pos.y * pos.y);
    if (R < 0.01) R = 0.01;
    double Z = pos.z;

    /* Distance from magnetic axis in the poloidal plane */
    double dR = R - R0;
    double rho = sqrt(dR * dR + Z * Z);
    if (rho > a) return (Vec3){0.0, 0.0, 0.0};

    double Bphi = B0 * R0 / R;
    double cp = pos.x / R;
    double sp = pos.y / R;

    Vec3 B;
    B.x = -Bphi * sp;
    B.y =  Bphi * cp;
    B.z = 0.0;
    return B;
}

void field_init_torus(FieldModel *fm)
{
    memset(fm, 0, sizeof(*fm));
    snprintf(fm->code, sizeof(fm->code), "TOR");
    snprintf(fm->name, FIELD_MAX_NAME, "%s", TR(STR_MODEL_TORUS));
    fm->n_params = 3;
    snprintf(fm->param_names[0], FIELD_MAX_PNAME, "%s", TR(STR_PARAM_B0));
    fm->params[0] = 1.0; fm->param_min[0] = 0.1; fm->param_max[0] = 20.0;
    snprintf(fm->param_names[1], FIELD_MAX_PNAME, "%s", TR(STR_PARAM_R0));
    fm->params[1] = 3.0; fm->param_min[1] = 0.5; fm->param_max[1] = 20.0;
    snprintf(fm->param_names[2], FIELD_MAX_PNAME, "%s", TR(STR_PARAM_A_MINOR));
    fm->params[2] = 0.75; fm->param_min[2] = 0.1; fm->param_max[2] = 5.0;
    fm->eval_B = torus_B;
    fm->eval_E = NULL;
    fm->default_pos = (Vec3){3.5, 0.0, 0.0};
    fm->default_vel_dir = (Vec3){0.0, 1.0, 0.0};
    fm->default_camera_dist = 10.0;
}
