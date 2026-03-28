#include "diagnostics.h"

void diag_init(DiagTimeSeries *d, int capacity)
{
    d->head = 0;
    d->count = 0;
    d->capacity = capacity;
    if (d->capacity > DIAG_MAX_SAMPLES)
        d->capacity = DIAG_MAX_SAMPLES;
}

void diag_push(DiagTimeSeries *d, double t, double pitch, double mu, double E)
{
    d->time[d->head] = t;
    d->pitch_angle[d->head] = pitch;
    d->mu[d->head] = mu;
    d->energy_keV[d->head] = E;
    d->head = (d->head + 1) % d->capacity;
    if (d->count < d->capacity)
        d->count++;
}

void diag_clear(DiagTimeSeries *d)
{
    d->head = 0;
    d->count = 0;
}

int diag_get(const DiagTimeSeries *d, int age,
             double *t, double *pitch, double *mu, double *E)
{
    if (age >= d->count) return -1;
    int idx = (d->head - 1 - age + d->capacity) % d->capacity;
    *t = d->time[idx];
    *pitch = d->pitch_angle[idx];
    *mu = d->mu[idx];
    *E = d->energy_keV[idx];
    return 0;
}
