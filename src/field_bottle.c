#include "field.h"
#include "i18n.h"
#include <math.h>
#include <stdio.h>
#include <string.h>

/* Magnetic bottle (mirror machine):
 * B_z = B0 * (1 + z^2 / Lm^2)
 * B_r = -B0 * r * z / Lm^2     (from div B = 0)
 * Axis along z. */

static Vec3 bottle_B(const double *params, Vec3 pos)
{
    double B0 = params[0];
    double Lm = params[1];
    double L2 = Lm * Lm;
    double r = sqrt(pos.x * pos.x + pos.y * pos.y);
    double z = pos.z;

    double Bz = B0 * (1.0 + z * z / L2);
    double Br = -B0 * r * z / L2;

    Vec3 B;
    if (r > 1e-12) {
        B.x = Br * pos.x / r;
        B.y = Br * pos.y / r;
    } else {
        B.x = 0.0;
        B.y = 0.0;
    }
    B.z = Bz;
    return B;
}

void field_init_bottle(FieldModel *fm)
{
    memset(fm, 0, sizeof(*fm));
    snprintf(fm->code, sizeof(fm->code), "BOT");
    snprintf(fm->name, FIELD_MAX_NAME, "%s", TR(STR_MODEL_BOTTLE));
    fm->n_params = 2;
    snprintf(fm->param_names[0], FIELD_MAX_PNAME, "%s", TR(STR_PARAM_B0_BOTTLE));
    fm->params[0] = 1.0; fm->param_min[0] = 0.01; fm->param_max[0] = 2.0;
    snprintf(fm->param_names[1], FIELD_MAX_PNAME, "%s", TR(STR_PARAM_LM));
    fm->params[1] = 5.0; fm->param_min[1] = 0.5; fm->param_max[1] = 50.0;
    fm->eval_B = bottle_B;
    fm->eval_E = NULL;
    fm->default_pos = (Vec3){0.0, 0.0, 0.0};
    fm->default_vel_dir = (Vec3){0.0, 0.0, 1.0};
    fm->default_camera_dist = 10.0;
}
