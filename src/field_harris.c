#include "field.h"
#include "i18n.h"
#include <math.h>
#include <stdio.h>
#include <string.h>

/* Harris current sheet with guide and normal fields:
 *   B = B0 * tanh(z / L) * ex  +  Bg * ey  +  Bn * ez
 * The reversing component Bx depends only on z and Bg, Bn are uniform,
 * so div B = 0. Bg is the guide field (along the sheet, parallel to the
 * current); Bn is the normal field threading the sheet. With Bg = Bn = 0
 * particles near z = 0 execute Speiser/meandering orbits about the
 * neutral plane. */

static Vec3 harris_B(const double *params, Vec3 pos)
{
    double B0 = params[0];
    double L  = params[1];
    double Bg = params[2];
    double Bn = params[3];
    return (Vec3){B0 * tanh(pos.z / L), Bg, Bn};
}

void field_init_harris(FieldModel *fm)
{
    memset(fm, 0, sizeof(*fm));
    snprintf(fm->code, sizeof(fm->code), "HAR");
    snprintf(fm->name, FIELD_MAX_NAME, "%s", TR(STR_MODEL_HARRIS));
    fm->n_params = 4;
    snprintf(fm->param_names[0], FIELD_MAX_PNAME, "%s", TR(STR_PARAM_B0));
    fm->params[0] = 1.0; fm->param_min[0] = 0.01; fm->param_max[0] = 2.0;
    snprintf(fm->param_names[1], FIELD_MAX_PNAME, "%s", TR(STR_PARAM_L_SHEET));
    fm->params[1] = 1.0; fm->param_min[1] = 0.1; fm->param_max[1] = 5.0;
    snprintf(fm->param_names[2], FIELD_MAX_PNAME, "%s", TR(STR_PARAM_BG_GUIDE));
    fm->params[2] = 0.0; fm->param_min[2] = -1.0; fm->param_max[2] = 1.0;
    snprintf(fm->param_names[3], FIELD_MAX_PNAME, "%s", TR(STR_PARAM_BN_NORMAL));
    fm->params[3] = 0.0; fm->param_min[3] = -0.5; fm->param_max[3] = 0.5;
    fm->eval_B = harris_B;
    fm->eval_E = NULL;
    fm->default_pos = (Vec3){0.0, 0.0, 0.5};
    fm->default_vel_dir = (Vec3){0.0, 1.0, 0.0};
    fm->default_camera_dist = 8.0;
}
