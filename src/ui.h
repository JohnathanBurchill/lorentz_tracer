#ifndef UI_H
#define UI_H

struct AppState;

void ui_init(struct AppState *app);
void ui_sync_from_app(const struct AppState *app);
void ui_update(struct AppState *app);   /* layout + interaction (before BeginDrawing) */
void ui_render(struct AppState *app);   /* draw render commands (inside BeginDrawing) */

#endif
