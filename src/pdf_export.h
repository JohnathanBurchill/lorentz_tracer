#ifndef PDF_EXPORT_H
#define PDF_EXPORT_H

struct AppState;

/* Export the current view (3D scene + plot strip + Gij + initial-conditions
 * overlays) to a vector PDF. The UI panel and HUD elements are excluded.
 * Returns 0 on success, non-zero on failure. */
int pdf_export(const struct AppState *app, const char *path);

#endif
