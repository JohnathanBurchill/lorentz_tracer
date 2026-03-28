#include "field.h"
#include "i18n.h"
#include <stdio.h>
#include <string.h>

/* B = (B0 + lambda * z) * ez
 * Violates div B = 0. Paper Problem 4. */

static Vec3 nonphysical_B(const double *params, Vec3 pos)
{
    double B0 = params[0];
    double lambda = params[1];
    return (Vec3){0.0, 0.0, B0 + lambda * pos.z};
}

void field_init_nonphysical(FieldModel *fm)
{
    memset(fm, 0, sizeof(*fm));
    snprintf(fm->code, sizeof(fm->code), "NPH");
    snprintf(fm->name, FIELD_MAX_NAME, "%s", TR(STR_MODEL_NONPHYSICAL));
    fm->n_params = 2;
    snprintf(fm->param_names[0], FIELD_MAX_PNAME, "%s", TR(STR_PARAM_B0));
    fm->params[0] = 1.0; fm->param_min[0] = 0.01; fm->param_max[0] = 2.0;
    snprintf(fm->param_names[1], FIELD_MAX_PNAME, "%s", TR(STR_PARAM_LAMBDA_TM));
    fm->params[1] = 0.5; fm->param_min[1] = 0.01; fm->param_max[1] = 5.0;
    fm->eval_B = nonphysical_B;
    fm->eval_E = NULL;
    fm->default_pos = (Vec3){0.0, 0.0, 0.0};
    fm->default_vel_dir = (Vec3){1.0, 0.0, 0.0};
    fm->default_camera_dist = 8.0;
}
