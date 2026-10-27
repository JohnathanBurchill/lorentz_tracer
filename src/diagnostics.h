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
    double v_kappa[DIAG_MAX_SAMPLES];         /* v · κ̂ */
    double v_tau[DIAG_MAX_SAMPLES];           /* v · (b̂ × κ̂) */
    double lshell[DIAG_MAX_SAMPLES];          /* L of the GC's field line */
    double kappa[DIAG_MAX_SAMPLES];           /* Büchner-Zelenyi κ */
    int head;
    int count;
    int capacity;
} DiagTimeSeries;

/* The time series a plot can show */
enum { DIAG_PITCH, DIAG_MU, DIAG_LSHELL, DIAG_KAPPA };

/* Samples of series kind, and in *gc its guiding-centre counterpart
 * (NULL when the series has none) */
const double *diag_series(const DiagTimeSeries *d, int kind, const double **gc);

void diag_init(DiagTimeSeries *d, int capacity);
void diag_push(DiagTimeSeries *d, double t, double pitch, double mu, double E);
void diag_clear(DiagTimeSeries *d);

/* Access by age (0 = newest). Returns 0 on success, -1 if age >= count. */
int diag_get(const DiagTimeSeries *d, int age,
             double *t, double *pitch, double *mu, double *E);

#endif
