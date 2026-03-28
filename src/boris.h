#ifndef BORIS_H
#define BORIS_H

#include "vec3.h"
#include "field.h"

/* Physical constants */
#define QE  1.602176634e-19     /* elementary charge (C) */
#define MP  1.67262192e-27      /* proton mass (kg) */
#define ME  9.1093837015e-31    /* electron mass (kg) */
#define KEV_TO_J (QE * 1000.0)
#define SPEED_OF_LIGHT 299792458.0
#define EPSILON_0 8.8541878128e-12

#define SPECIES_ELECTRON  0
#define SPECIES_POSITRON  1
#define SPECIES_PROTON    2
#define SPECIES_ALPHA     3
#define SPECIES_OPLUS     4
#define SPECIES_CUSTOM    5

typedef struct {
    int species;
    double q;       /* charge (C) */
    double m;       /* mass (kg) */
    double E_keV;
    double pitch;   /* cos(pitch_angle) at launch */
    Vec3 pos;
    Vec3 vel;       /* 3-velocity v (always stored, even in rel mode) */
} Particle;

/* Set mass and charge from species index */
void boris_set_species(Particle *p, int species);

/* Initialize particle state from energy, pitch angle, position, and field.
 * If relativistic, E_keV is relativistic kinetic energy (gamma-1)*mc^2. */
void boris_init_particle(Particle *p, int species, double E_keV,
                         double pitch_angle_deg, Vec3 pos,
                         const FieldModel *fm, int relativistic);

/* Single Boris step. If relativistic, uses 4-velocity internally. */
void boris_step(Particle *p, const FieldModel *fm, double dt, int relativistic);

/* Gyro period at current position */
double boris_gyro_period(const Particle *p, const FieldModel *fm, int relativistic);

/* Pitch angle in degrees at current position */
double boris_pitch_angle(const Particle *p, const FieldModel *fm);

/* Magnetic moment mu = m*v_perp^2 / (2B) */
double boris_mu(const Particle *p, const FieldModel *fm);

/* Kinetic energy in keV (non-rel: mv^2/2, rel: (gamma-1)mc^2) */
double boris_energy_keV(const Particle *p, int relativistic);

/* Radiation loss. Non-rel: Larmor. Rel: Lienard with gamma factors. */
double boris_radiation_step(Particle *p, const FieldModel *fm, double dt,
                            int relativistic);

/* Batch Boris step for n particles (SoA internally, auto-vectorized).
 * Non-relativistic path uses SIMD-friendly split loops; relativistic
 * falls back to per-particle boris_step. */
void boris_step_batch(Particle *particles, int n,
                      const FieldModel *fm, double dt, int relativistic);

#endif
