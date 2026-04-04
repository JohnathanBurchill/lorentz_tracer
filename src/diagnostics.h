#ifndef DIAGNOSTICS_H
#define DIAGNOSTICS_H

#define DIAG_MAX_SAMPLES 65536

typedef struct {
    double time[DIAG_MAX_SAMPLES];
    double pitch_angle[DIAG_MAX_SAMPLES];
    double mu[DIAG_MAX_SAMPLES];
    double energy_keV[DIAG_MAX_SAMPLES];
    double gc_pitch_angle[DIAG_MAX_SAMPLES];  /* GC pitch angle (degrees) */
    double gc_mu[DIAG_MAX_SAMPLES];           /* GC adiabatic invariant */
    int head;
    int count;
    int capacity;
} DiagTimeSeries;

void diag_init(DiagTimeSeries *d, int capacity);
void diag_push(DiagTimeSeries *d, double t, double pitch, double mu, double E);
void diag_clear(DiagTimeSeries *d);

/* Access by age (0 = newest). Returns 0 on success, -1 if age >= count. */
int diag_get(const DiagTimeSeries *d, int age,
             double *t, double *pitch, double *mu, double *E);

#endif
