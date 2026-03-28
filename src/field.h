#ifndef FIELD_H
#define FIELD_H

#include "vec3.h"

#define FIELD_MAX_PARAMS    8
#define FIELD_MAX_NAME      64
#define FIELD_MAX_PNAME     32
#define FIELD_NUM_MODELS    10

typedef struct {
    char name[FIELD_MAX_NAME];
    char code[8];              /* short code for filenames: CIR, GRB, etc. */
    char param_names[FIELD_MAX_PARAMS][FIELD_MAX_PNAME];
    double params[FIELD_MAX_PARAMS];
    double param_min[FIELD_MAX_PARAMS];
    double param_max[FIELD_MAX_PARAMS];
    int n_params;

    Vec3 (*eval_B)(const double *params, Vec3 pos);
    Vec3 (*eval_E)(const double *params, Vec3 pos); /* NULL = no E field */

    Vec3 default_pos;
    Vec3 default_vel_dir;     /* unit direction for initial velocity */
    double default_camera_dist;
} FieldModel;

/* Initialize all field models */
void field_init_all(FieldModel models[FIELD_NUM_MODELS]);

/* Re-translate model/param names (after language change). Preserves params. */
void field_retranslate(FieldModel models[FIELD_NUM_MODELS]);

/* Shared helpers */
double field_Bmag(const FieldModel *fm, Vec3 pos);
Vec3 field_gradB(const FieldModel *fm, Vec3 pos);

/* Per-model init functions */
void field_init_circular(FieldModel *fm);
void field_init_grad_b(FieldModel *fm);
void field_init_higher_grad(FieldModel *fm);
void field_init_sinusoidal(FieldModel *fm);
void field_init_nonphysical(FieldModel *fm);
void field_init_dipole(FieldModel *fm);
void field_init_bottle(FieldModel *fm);
void field_init_tokamak(FieldModel *fm);
void field_init_stellarator(FieldModel *fm);
void field_init_torus(FieldModel *fm);


#endif
