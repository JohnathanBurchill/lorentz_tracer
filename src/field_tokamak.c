#include "field.h"
#include "i18n.h"
#include <math.h>
#include <stdio.h>
#include <string.h>

/* Large-aspect-ratio tokamak with circular flux surfaces.
 * Toroidal: B_phi = B0 * R0 / R
 * Poloidal: B_pol = (B0 / (q * R)) * (R - R0)  (simplified)
 * Field confined within minor radius a of the magnetic axis.
 * Ported from code/tokamak.c */

static Vec3 tokamak_B(const double *params, Vec3 pos)
{
    double B0 = params[0];
    double R0 = params[1];
    double q = params[2];
    double a = params[3];

    double R = sqrt(pos.x * pos.x + pos.y * pos.y);
    if (R < 0.01) R = 0.01;
    double Z = pos.z;
    double Rrel = R - R0;

    /* Check if outside minor radius */
    double rho = sqrt(Rrel * Rrel + Z * Z);
    if (rho > a) return (Vec3){0.0, 0.0, 0.0};

    double Bphi = B0 * R0 / R;
    double coeff = B0 / (q * R);
    double BR = -coeff * Z;
    double BZ = coeff * Rrel;

    double cp = pos.x / R;
    double sp = pos.y / R;

    Vec3 B;
    B.x = BR * cp - Bphi * sp;
    B.y = BR * sp + Bphi * cp;
    B.z = BZ;
    return B;
}

void field_init_tokamak(FieldModel *fm)
{
    memset(fm, 0, sizeof(*fm));
    snprintf(fm->code, sizeof(fm->code), "TOK");
    snprintf(fm->name, FIELD_MAX_NAME, "%s", TR(STR_MODEL_TOKAMAK));
    fm->n_params = 4;
    snprintf(fm->param_names[0], FIELD_MAX_PNAME, "%s", TR(STR_PARAM_B0));
    fm->params[0] = 5.0; fm->param_min[0] = 0.1; fm->param_max[0] = 20.0;
    snprintf(fm->param_names[1], FIELD_MAX_PNAME, "%s", TR(STR_PARAM_R0));
    fm->params[1] = 6.0; fm->param_min[1] = 1.0; fm->param_max[1] = 20.0;
    snprintf(fm->param_names[2], FIELD_MAX_PNAME, "%s", TR(STR_PARAM_Q_SAFETY));
    fm->params[2] = 2.0; fm->param_min[2] = 0.5; fm->param_max[2] = 10.0;
    snprintf(fm->param_names[3], FIELD_MAX_PNAME, "%s", TR(STR_PARAM_A_MINOR));
    fm->params[3] = 2.0; fm->param_min[3] = 0.1; fm->param_max[3] = 5.0;
    fm->eval_B = tokamak_B;
    fm->eval_E = NULL;
    fm->default_pos = (Vec3){7.0, 0.0, 0.0};
    fm->default_vel_dir = (Vec3){0.0, 1.0, 0.0};
    fm->default_camera_dist = 15.0;
}
