#ifndef GC_H
#define GC_H

#include "vec3.h"
#include "field.h"
#include "boris.h"

/* Guiding-centre particle state.
 * p_par: non-rel = v_parallel; rel = p_parallel = gamma*m*v_parallel.
 * mu:    non-rel = m*v_perp^2/(2B); rel = p_perp^2/(2mB). */
typedef struct {
    Vec3 pos;
    double p_par;
    double mu;
    double q, m;
} GCParticle;

/* Initialise GC state from a full-orbit particle.
 * dt is the simulation timestep (used for orbit-averaging). */
void gc_init_from_particle(GCParticle *gc, const Particle *p,
                           const FieldModel *fm, int relativistic,
                           double dt);

/* Single RK4 step of the GC drift equations.
 * symplectic: 0 = Alfvén drifts, 1 = B* symplectic form. */
void gc_step(GCParticle *gc, const FieldModel *fm, double dt,
             int relativistic, int symplectic);

/* Batch step for n GC particles. */
void gc_step_batch(GCParticle *gc, int n, const FieldModel *fm,
                   double dt, int relativistic, int symplectic);

/* GC pitch angle in degrees. */
double gc_pitch_angle(const GCParticle *gc, const FieldModel *fm,
                      int relativistic);


#endif
