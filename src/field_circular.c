#include "field.h"
#include "i18n.h"
#include <math.h>
#include <stdio.h>
#include <string.h>

/* B = B0 * (-y, x, 0) / sqrt(x^2 + y^2)
 * |B| = B0 everywhere. Pure curvature drift, no grad-B drift, no mirror force. */

static Vec3 circular_B(const double *params, Vec3 pos)
{
    double B0 = params[0];
    double r = sqrt(pos.x * pos.x + pos.y * pos.y);
    if (r < 1e-12) r = 1e-12;
    return (Vec3){-B0 * pos.y / r, B0 * pos.x / r, 0.0};
}

void field_init_circular(FieldModel *fm)
{
    memset(fm, 0, sizeof(*fm));
    snprintf(fm->code, sizeof(fm->code), "CIR");
    snprintf(fm->name, FIELD_MAX_NAME, "%s", TR(STR_MODEL_CIRCULAR));
    fm->n_params = 1;
    snprintf(fm->param_names[0], FIELD_MAX_PNAME, "%s", TR(STR_PARAM_B0));
    fm->params[0] = 1.0;
    fm->param_min[0] = 0.01;
    fm->param_max[0] = 2.0;
    fm->eval_B = circular_B;
    fm->eval_E = NULL;
    fm->default_pos = (Vec3){5.0, 0.0, 0.0};
    fm->default_vel_dir = (Vec3){0.0, 0.0, 1.0};
    fm->default_camera_dist = 12.0;
}
