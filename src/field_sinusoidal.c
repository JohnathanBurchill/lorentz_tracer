#include "field.h"
#include "i18n.h"
#include <math.h>
#include <stdio.h>
#include <string.h>

/* B = B0 * (1 + a * sin(k * y)) * ez
 * Sinusoidal modulation of |B|. Satisfies div B = 0 (no z-dependence). */

static Vec3 sinusoidal_B(const double *params, Vec3 pos)
{
    double B0 = params[0];
    double a = params[1];
    double k = params[2];
    double Bz = B0 * (1.0 + a * sin(k * pos.y));
    return (Vec3){0.0, 0.0, Bz};
}

void field_init_sinusoidal(FieldModel *fm)
{
    memset(fm, 0, sizeof(*fm));
    snprintf(fm->code, sizeof(fm->code), "SIN");
    snprintf(fm->name, FIELD_MAX_NAME, "%s", TR(STR_MODEL_SINUSOIDAL));
    fm->n_params = 3;
    snprintf(fm->param_names[0], FIELD_MAX_PNAME, "%s", TR(STR_PARAM_B0));
    fm->params[0] = 1.0; fm->param_min[0] = 0.01; fm->param_max[0] = 2.0;
    snprintf(fm->param_names[1], FIELD_MAX_PNAME, "%s", TR(STR_PARAM_AMPLITUDE));
    fm->params[1] = 0.3; fm->param_min[1] = 0.01; fm->param_max[1] = 0.99;
    snprintf(fm->param_names[2], FIELD_MAX_PNAME, "%s", TR(STR_PARAM_WAVENUMBER));
    fm->params[2] = 1.0; fm->param_min[2] = 0.1; fm->param_max[2] = 10.0;
    fm->eval_B = sinusoidal_B;
    fm->eval_E = NULL;
    fm->default_pos = (Vec3){0.0, 0.0, 0.0};
    fm->default_vel_dir = (Vec3){0.0, 1.0, 0.0};
    fm->default_camera_dist = 8.0;
}
