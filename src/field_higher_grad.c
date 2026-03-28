#include "field.h"
#include "i18n.h"
#include <math.h>
#include <stdio.h>
#include <string.h>

/* B = B0 * (1 + lambda * y^2) * ez
 * Quadratic variation of |B| with y. Satisfies div B = 0. */

static Vec3 higher_grad_B(const double *params, Vec3 pos)
{
    double B0 = params[0];
    double lambda = params[1];
    double Bz = B0 * (1.0 + lambda * pos.y * pos.y);
    return (Vec3){0.0, 0.0, Bz};
}

void field_init_higher_grad(FieldModel *fm)
{
    memset(fm, 0, sizeof(*fm));
    snprintf(fm->code, sizeof(fm->code), "QGR");
    snprintf(fm->name, FIELD_MAX_NAME, "%s", TR(STR_MODEL_QUAD_GRAD));
    fm->n_params = 2;
    snprintf(fm->param_names[0], FIELD_MAX_PNAME, "%s", TR(STR_PARAM_B0));
    fm->params[0] = 1.0; fm->param_min[0] = 0.01; fm->param_max[0] = 2.0;
    snprintf(fm->param_names[1], FIELD_MAX_PNAME, "%s", TR(STR_PARAM_LAMBDA_1M2));
    fm->params[1] = 0.05; fm->param_min[1] = 0.001; fm->param_max[1] = 1.0;
    fm->eval_B = higher_grad_B;
    fm->eval_E = NULL;
    fm->default_pos = (Vec3){0.0, 0.0, 0.0};
    fm->default_vel_dir = (Vec3){0.0, 1.0, 0.0};
    fm->default_camera_dist = 8.0;
}
