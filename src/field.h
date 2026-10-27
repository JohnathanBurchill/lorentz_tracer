#ifndef FIELD_H
#define FIELD_H

#include "vec3.h"

#define FIELD_MAX_PARAMS    8
#define FIELD_MAX_NAME      64
#define FIELD_MAX_PNAME     32
#define FIELD_NUM_MODELS    13

typedef struct {
    char name[FIELD_MAX_NAME];
    char code[8];              /* short code for filenames: CIR, GRB, etc. */
    char param_names[FIELD_MAX_PARAMS][FIELD_MAX_PNAME];
    double params[FIELD_MAX_PARAMS];
    double param_min[FIELD_MAX_PARAMS];
    double param_max[FIELD_MAX_PARAMS];
    int param_log[FIELD_MAX_PARAMS];  /* 1 = slider is logarithmic in the value */
    int param_int[FIELD_MAX_PARAMS];  /* 1 = slider takes whole-number values */
    int n_params;

    Vec3 (*eval_B)(const double *params, Vec3 pos);
    Vec3 (*eval_E)(const double *params, Vec3 pos); /* NULL = no E field */

    Vec3 default_pos;
    Vec3 default_vel_dir;     /* unit direction for initial velocity */
    double default_camera_dist;

    /* Similarity-scaled models: positions are in units of length_unit metres
     * (0 = plain SI), B is stored multiplied by it and time divided by it, so
     * velocity, energy and pitch angle stay physical. Readouts convert back:
     * physical t = t L, B = B / L, mu = mu L, with L = field_length_unit(). */
    double length_unit;
    char length_name[8];       /* name of one scene length unit, e.g. "RE" */
    double body_radius;        /* solid central body (0 = none): drawn as a
                                  sphere, field lines stop at it, and a
                                  particle that enters it is lost there */
} FieldModel;

/* Initialize all field models */
void field_init_all(FieldModel models[FIELD_NUM_MODELS]);

/* Re-translate model/param names (after language change). Preserves params. */
void field_retranslate(FieldModel models[FIELD_NUM_MODELS]);

/* Shared helpers */
double field_Bmag(const FieldModel *fm, Vec3 pos);
Vec3 field_gradB(const FieldModel *fm, Vec3 pos);

/* Field-line curvature vector (bhat . nabla) bhat at pos, by central
 * differences along bhat, the unit field direction there */
Vec3 field_curvature(const FieldModel *fm, Vec3 pos, Vec3 bhat);

/* Buchner and Zelenyi (1989) kappa = sqrt(R_c / rho) at pos, for a particle
 * of momentum magnitude p_mag (gamma m |v|) and charge q: the field-line
 * curvature radius over the gyroradius at full speed. INFINITY where the
 * field line is straight, NaN where B = 0. Scene units throughout, so the
 * similarity scaling of the length unit cancels. */
double field_kappa_bz(const FieldModel *fm, Vec3 pos, double p_mag, double q);

static inline double field_length_unit(const FieldModel *fm)
{
    return fm->length_unit > 0.0 ? fm->length_unit : 1.0;
}

/* True inside the model's solid central body (never when it has none). */
static inline int field_inside_body(const FieldModel *fm, Vec3 p)
{
    return vec3_len(p) < fm->body_radius;
}

/* L-shell of the field line through pos, in scene length units: the distance
 * from the origin to the point of minimum |B| along it (its magnetic
 * equator). Returns 1 and sets *L when the field line is closed, meaning it
 * reaches the model's central body at both ends; returns 0 for an open field
 * line, a point inside the body, or a model without a body. */
int field_L_shell(const FieldModel *fm, Vec3 pos, double *L);

/* The point of minimum |B| along the field line through pos, its magnetic
 * equator, under the same conditions as field_L_shell (closed lines only) */
int field_min_B_point(const FieldModel *fm, Vec3 pos, Vec3 *pmin);

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
void field_init_harris(FieldModel *fm);
void field_init_turbulence(FieldModel *fm);
void field_init_t89(FieldModel *fm);


#endif
