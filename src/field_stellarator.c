#include "field.h"
#include "i18n.h"
#include "nax.h"
#include <math.h>
#include <stdio.h>
#include <string.h>

/* Near-axis stellarator field using NAE expansion.
 * Config index (param 0) selects preset:
 *   0 = Landreman-Sengupta 2019 "r2 section 5.1" (QA)
 *   1 = Landreman-Paul 2022 "Precise QA"
 * R0_phys and B0_phys are adjustable.
 * Field is zero outside minor radius a. */

static NAXConfig s_nax;
static int s_nax_initialized = 0;
static int s_last_config_idx = -1;
static double s_last_R0 = -1.0;
static double s_last_B0 = -1.0;

static void ensure_nax_init(const double *params)
{
    int config_idx = (int)params[0];
    if (config_idx < 0) config_idx = 0;
    if (config_idx > 1) config_idx = 1;
    double R0 = params[1];
    double B0 = params[2];

    if (s_nax_initialized &&
        config_idx == s_last_config_idx &&
        R0 == s_last_R0 &&
        B0 == s_last_B0) {
        return;
    }

    if (config_idx == 1) {
        nax_config_precise_qa(&s_nax);
    } else {
        nax_config_r2s51(&s_nax);
    }

    s_nax.R0_phys = R0;
    s_nax.B0_phys = B0;
    nax_init(&s_nax);

    s_nax_initialized = 1;
    s_last_config_idx = config_idx;
    s_last_R0 = R0;
    s_last_B0 = B0;
}

/* Pointer to the FieldModel so we can update default_pos dynamically */
static FieldModel *s_fm_ptr = NULL;

static Vec3 stellarator_B(const double *params, Vec3 pos)
{
    ensure_nax_init(params);
    /* Update default_pos to match current R0_phys */
    if (s_fm_ptr && s_nax_initialized) {
        Vec3 axis0 = vec3_scale(s_nax.R0_phys, axis_pos(&s_nax.af, 0.0));
        s_fm_ptr->default_pos = (Vec3){axis0.x + 0.05, axis0.y, axis0.z};
        s_fm_ptr->default_camera_dist = (float)(s_nax.R0_phys * 3.0);
    }

    double a = params[3];

    /* Check if outside minor radius by finding nearest axis point */
    double best_phi = 0;
    double best_dist2 = 1e30;
    for (int i = 0; i < 64; i++) {
        double phi = 2.0 * M_PI * i / 64;
        Vec3 r0 = vec3_scale(s_nax.R0_phys, axis_pos(&s_nax.af, phi));
        double d2 = vec3_dot(vec3_sub(pos, r0), vec3_sub(pos, r0));
        if (d2 < best_dist2) { best_dist2 = d2; best_phi = phi; }
    }
    double phi_star = nax_nearest_phi(&s_nax, pos, best_phi);
    Vec3 r0 = vec3_scale(s_nax.R0_phys, axis_pos(&s_nax.af, phi_star));
    double dist = vec3_len(vec3_sub(pos, r0));

    if (dist > a) return (Vec3){0.0, 0.0, 0.0};

    return nax_B(&s_nax, pos);
}

void field_init_stellarator(FieldModel *fm)
{
    memset(fm, 0, sizeof(*fm));
    snprintf(fm->code, sizeof(fm->code), "STEL");
    snprintf(fm->name, FIELD_MAX_NAME, "%s", TR(STR_MODEL_STELLARATOR));
    fm->n_params = 4;

    snprintf(fm->param_names[0], FIELD_MAX_PNAME, "%s", TR(STR_PARAM_CONFIG));
    fm->params[0] = 0.0; fm->param_min[0] = 0.0; fm->param_max[0] = 1.0;

    snprintf(fm->param_names[1], FIELD_MAX_PNAME, "%s", TR(STR_PARAM_R0_PHYS));
    fm->params[1] = 1.0; fm->param_min[1] = 0.2; fm->param_max[1] = 10.0;

    snprintf(fm->param_names[2], FIELD_MAX_PNAME, "%s", TR(STR_PARAM_B0_PHYS));
    fm->params[2] = 5.0; fm->param_min[2] = 0.1; fm->param_max[2] = 20.0;

    snprintf(fm->param_names[3], FIELD_MAX_PNAME, "%s", TR(STR_PARAM_A_MINOR));
    fm->params[3] = 0.15; fm->param_min[3] = 0.01; fm->param_max[3] = 0.5;

    fm->eval_B = stellarator_B;
    fm->eval_E = NULL;

    /* Default position: offset from axis at phi=0 by ~0.05 m in the radial direction */
    fm->default_pos = (Vec3){1.05, 0.0, 0.0};
    fm->default_vel_dir = (Vec3){0.0, 1.0, 0.2};
    fm->default_camera_dist = 3.0;

    /* Store pointer for dynamic default_pos updates */
    s_fm_ptr = fm;

    /* Trigger NAX initialization on first B evaluation */
    s_nax_initialized = 0;
    s_last_config_idx = -1;
}

/* Accessor for the NAX config, used by render3d for axis drawing */
const NAXConfig *stellarator_get_nax(void)
{
    if (!s_nax_initialized) return NULL;
    return &s_nax;
}
