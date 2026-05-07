#ifndef RENDER3D_H
#define RENDER3D_H

#include "vec3.h"

/* Forward declaration to avoid pulling raylib into every header */
struct AppState;

void render3d_draw(const struct AppState *app);

/* Read-only access to the cached field-line polylines built during the last
 * render. Returns 0 if the cache is empty. The cache is rebuilt lazily by
 * render3d_draw whenever the model or its parameters change, so callers
 * should expect this to be current right after the screen frame is drawn. */
int render3d_field_line_count(void);
const Vec3 *render3d_field_line_get(int idx, int *out_n);

#endif
