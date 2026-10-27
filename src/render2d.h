#ifndef RENDER2D_H
#define RENDER2D_H

#include "raylib.h"

struct AppState;

/* Panels of the telemetry strip: a time series (DIAG_* kind) or the phase
 * plot. The phase panel is square at the right; the time series share the
 * rest of the width equally. Fills kinds[] and rects[] (PLOT_SLOTS long)
 * and returns how many panels the strip shows. */
#define PLOT_SLOTS 5
#define PLOT_KIND_PHASE (-1)
int render2d_plot_layout(const struct AppState *app, int kinds[PLOT_SLOTS],
                         Rectangle rects[PLOT_SLOTS]);

/* Title, and y-axis symbol and units, of a time-series panel */
void render2d_series_labels(int kind, const char **title, const char **sym,
                            const char **units);

void render2d_draw(struct AppState *app);

#endif
