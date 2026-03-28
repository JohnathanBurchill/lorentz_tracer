/* tutorial.c — Interactive tutorial for Lorentz Tracer
 * Walks users through the interface progressively with a floating panel.
 * Uses clay.h for layout, same patterns as help.c and playback.c. */

#include "tutorial.h"
#include "i18n.h"
#include "app.h"
#include "clay.h"
#include "raylib.h"
#include <string.h>
#include <math.h>

#define FONT_UI   0
#define FONT_MONO 1

/* ---- Step definitions ---- */

static const TutStep g_steps[] = {
    { /* 0: pause/play */
        .text_brief_id = STR_TUT_S0_BRIEF, .text_detail_id = STR_TUT_S0_DETAIL,
        .action = TUT_ACT_PRESS_SPACE,
        .set_paused = 1, .set_show_vel = -1, .set_show_B = -1,
        .set_show_F = -1, .set_show_fl = -1, .set_ui_visible = -1
    },
    { /* 1: camera orbit */
        .text_brief_id = STR_TUT_S1_BRIEF, .text_detail_id = STR_TUT_S1_DETAIL,
        .action = TUT_ACT_ORBIT_CAMERA,
        .set_paused = -1, .set_show_vel = 0, .set_show_B = 0,
        .set_show_F = 0, .set_show_fl = -1, .set_ui_visible = -1
    },
    { /* 2: field lines */
        .text_brief_id = STR_TUT_S2_BRIEF, .text_detail_id = STR_TUT_S2_DETAIL,
        .action = TUT_ACT_NONE,
        .set_paused = -1, .set_show_vel = -1, .set_show_B = -1,
        .set_show_F = -1, .set_show_fl = 1, .set_ui_visible = -1
    },
    { /* 3: follow mode */
        .text_brief_id = STR_TUT_S3_BRIEF, .text_detail_id = STR_TUT_S3_DETAIL,
        .action = TUT_ACT_FOLLOW_ON,
        .set_paused = -1, .set_show_vel = -1, .set_show_B = -1,
        .set_show_F = -1, .set_show_fl = -1, .set_ui_visible = -1
    },
    { /* 4: velocity and B vectors */
        .text_brief_id = STR_TUT_S4_BRIEF, .text_detail_id = STR_TUT_S4_DETAIL,
        .action = TUT_ACT_NONE,
        .set_paused = -1, .set_show_vel = 1, .set_show_B = 1,
        .set_show_F = -1, .set_show_fl = -1, .set_ui_visible = -1
    },
    { /* 5: Lorentz force */
        .text_brief_id = STR_TUT_S5_BRIEF, .text_detail_id = STR_TUT_S5_DETAIL,
        .action = TUT_ACT_NONE,
        .set_paused = -1, .set_show_vel = -1, .set_show_B = -1,
        .set_show_F = 1, .set_show_fl = -1, .set_ui_visible = -1
    },
    { /* 6: field-aligned view */
        .text_brief_id = STR_TUT_S6_BRIEF, .text_detail_id = STR_TUT_S6_DETAIL,
        .action = TUT_ACT_FIELD_ALIGNED,
        .set_paused = -1, .set_show_vel = -1, .set_show_B = -1,
        .set_show_F = -1, .set_show_fl = -1, .set_ui_visible = -1
    },
    { /* 7: UI panel */
        .text_brief_id = STR_TUT_S7_BRIEF, .text_detail_id = STR_TUT_S7_DETAIL,
        .action = TUT_ACT_NONE,
        .set_paused = -1, .set_show_vel = -1, .set_show_B = -1,
        .set_show_F = -1, .set_show_fl = -1, .set_ui_visible = 1
    },
    { /* 8: particle parameters */
        .text_brief_id = STR_TUT_S8_BRIEF, .text_detail_id = STR_TUT_S8_DETAIL,
        .action = TUT_ACT_NONE,
        .set_paused = -1, .set_show_vel = -1, .set_show_B = -1,
        .set_show_F = -1, .set_show_fl = -1, .set_ui_visible = 1
    },
    { /* 9: change model */
        .text_brief_id = STR_TUT_S9_BRIEF, .text_detail_id = STR_TUT_S9_DETAIL,
        .action = TUT_ACT_CHANGE_MODEL,
        .set_paused = -1, .set_show_vel = -1, .set_show_B = -1,
        .set_show_F = -1, .set_show_fl = -1, .set_ui_visible = 1
    },
    { /* 10: plots */
        .text_brief_id = STR_TUT_S10_BRIEF, .text_detail_id = STR_TUT_S10_DETAIL,
        .action = TUT_ACT_NONE,
        .set_paused = -1, .set_show_vel = -1, .set_show_B = -1,
        .set_show_F = -1, .set_show_fl = -1, .set_ui_visible = -1
    },
    { /* 11: multi-particle */
        .text_brief_id = STR_TUT_S11_BRIEF, .text_detail_id = STR_TUT_S11_DETAIL,
        .action = TUT_ACT_NONE,
        .set_paused = -1, .set_show_vel = -1, .set_show_B = -1,
        .set_show_F = -1, .set_show_fl = -1, .set_ui_visible = -1
    },
    { /* 12: reset */
        .text_brief_id = STR_TUT_S12_BRIEF, .text_detail_id = STR_TUT_S12_DETAIL,
        .action = TUT_ACT_NONE,
        .set_paused = -1, .set_show_vel = -1, .set_show_B = -1,
        .set_show_F = -1, .set_show_fl = -1, .set_ui_visible = -1
    },
    { /* 13: recording */
        .text_brief_id = STR_TUT_S13_BRIEF, .text_detail_id = STR_TUT_S13_DETAIL,
        .action = TUT_ACT_NONE,
        .set_paused = -1, .set_show_vel = -1, .set_show_B = -1,
        .set_show_F = -1, .set_show_fl = -1, .set_ui_visible = -1
    },
    { /* 14: done */
        .text_brief_id = STR_TUT_S14_BRIEF, .text_detail_id = STR_TUT_S14_DETAIL,
        .action = TUT_ACT_NONE,
        .set_paused = -1, .set_show_vel = -1, .set_show_B = -1,
        .set_show_F = -1, .set_show_fl = -1, .set_ui_visible = -1
    },
};

#define TUT_NUM_STEPS (int)(sizeof(g_steps) / sizeof(g_steps[0]))

/* ---- State management ---- */

void tutorial_init(Tutorial *tut)
{
    memset(tut, 0, sizeof(*tut));
    tut->n_steps = TUT_NUM_STEPS;
}

void tutorial_start(AppState *app, int mode)
{
    app->tutorial.active = 1;
    app->tutorial.mode = mode;
    app->tutorial.step = 0;
    app->tutorial.camera_moved = 0;

    /* Select the custom particle species and reset the scene */
    app->species = SPECIES_CUSTOM;
    app->custom_speed = 0.2;
    app->pitch_deg = 0.0;
    app->current_model = 0; /* circular field */
    app->playback_speed = 0.05;
    app->speed_range = 1; /* x1 range */
    app->follow_particle = 0;
    app->cam_field_aligned = 0;
    app->needs_reset = 2;
    app_reset_particle(app);

    tutorial_apply_state(app);
    tutorial_snapshot(app);
}

void tutorial_stop(AppState *app)
{
    app->tutorial.active = 0;
    app->tutorial.step = 0;
}

void tutorial_advance(AppState *app)
{
    Tutorial *tut = &app->tutorial;
    tut->step++;
    tut->camera_moved = 0;
    if (tut->step >= tut->n_steps) {
        tutorial_stop(app);
        return;
    }
    tutorial_apply_state(app);
    tutorial_snapshot(app);
}

void tutorial_apply_state(AppState *app)
{
    const TutStep *s = &g_steps[app->tutorial.step];
    if (s->set_paused >= 0)     app->paused = s->set_paused;
    if (s->set_show_vel >= 0)   app->show_velocity_vec = s->set_show_vel;
    if (s->set_show_B >= 0)     app->show_B_vec = s->set_show_B;
    if (s->set_show_F >= 0)     app->show_F_vec = s->set_show_F;
    if (s->set_show_fl >= 0)    app->show_field_lines = s->set_show_fl;
    if (s->set_ui_visible >= 0) app->ui_visible = s->set_ui_visible;
}

void tutorial_snapshot(AppState *app)
{
    Tutorial *tut = &app->tutorial;
    tut->prev_paused = app->paused;
    tut->prev_show_vel = app->show_velocity_vec;
    tut->prev_show_B = app->show_B_vec;
    tut->prev_show_fl = app->show_field_lines;
    tut->prev_follow = app->follow_particle;
    tut->prev_cam_aligned = app->cam_field_aligned;
    tut->prev_model = app->current_model;
    tut->prev_pitch = app->pitch_deg;
}

int tutorial_check_action(AppState *app)
{
    Tutorial *tut = &app->tutorial;
    if (tut->step < 0 || tut->step >= tut->n_steps) return 0;
    switch (g_steps[tut->step].action) {
    case TUT_ACT_NONE:          return 0;
    case TUT_ACT_PRESS_SPACE:   return app->paused != tut->prev_paused;
    case TUT_ACT_TOGGLE_VEL:    return app->show_velocity_vec != tut->prev_show_vel;
    case TUT_ACT_TOGGLE_B:      return app->show_B_vec != tut->prev_show_B;
    case TUT_ACT_TOGGLE_FL:     return app->show_field_lines != tut->prev_show_fl;
    case TUT_ACT_ORBIT_CAMERA:  return tut->camera_moved;
    case TUT_ACT_FOLLOW_ON:     return app->follow_particle && !tut->prev_follow;
    case TUT_ACT_FIELD_ALIGNED: return app->cam_field_aligned && !tut->prev_cam_aligned;
    case TUT_ACT_CHANGE_MODEL:  return app->current_model != tut->prev_model;
    case TUT_ACT_CHANGE_PITCH:  return fabs(app->pitch_deg - tut->prev_pitch) > 0.5;
    }
    return 0;
}

/* ---- Clay layout ---- */

static Clay_String S(const char *s) {
    return (Clay_String){ .length = (int32_t)strlen(s), .chars = s };
}

/* Simple button: returns 1 if clicked */
static int tut_button(Clay_ElementId eid, const char *label,
                      Clay_Color bg, Clay_Color hover_bg, Clay_Color text_c)
{
    CLAY(eid, {
        .layout = { .sizing = { CLAY_SIZING_FIT(.min = 80), CLAY_SIZING_FIXED(28) },
                     .childAlignment = { .x = CLAY_ALIGN_X_CENTER, .y = CLAY_ALIGN_Y_CENTER },
                     .padding = {12, 12, 0, 0} },
        .backgroundColor = Clay_Hovered() ? hover_bg : bg,
        .border = { .width = CLAY_BORDER_OUTSIDE(1), .color = {100,100,110,120} },
        .cornerRadius = CLAY_CORNER_RADIUS(4)
    }) {
        CLAY_TEXT(S(label), CLAY_TEXT_CONFIG({
            .fontId = FONT_UI, .fontSize = 14, .letterSpacing = 1,
            .textColor = text_c }));
    }
    return Clay_PointerOver(eid) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
}

void tutorial_declare_layout(AppState *app)
{
    Tutorial *tut = &app->tutorial;
    if (!tut->active) return;

    int dark = app->dark_mode;
    Clay_Color panel_c = dark
        ? (Clay_Color){22, 22, 30, 245} : (Clay_Color){242, 242, 248, 245};
    Clay_Color text_c = dark
        ? (Clay_Color){230, 230, 230, 255} : (Clay_Color){30, 30, 30, 255};
    Clay_Color dim_c = dark
        ? (Clay_Color){160, 160, 170, 255} : (Clay_Color){100, 100, 110, 255};
    Clay_Color btn_c = dark
        ? (Clay_Color){50, 50, 65, 255} : (Clay_Color){210, 210, 220, 255};
    Clay_Color btn_hover = dark
        ? (Clay_Color){70, 70, 90, 255} : (Clay_Color){190, 190, 205, 255};
    Clay_Color accent = dark
        ? (Clay_Color){60, 100, 180, 255} : (Clay_Color){40, 80, 160, 255};
    Clay_Color accent_hover = dark
        ? (Clay_Color){80, 120, 200, 255} : (Clay_Color){60, 100, 180, 255};
    Clay_Color white = {255, 255, 255, 255};

    Clay_TextElementConfig body_cfg = {
        .fontId = FONT_UI, .fontSize = 16, .letterSpacing = 1,
        .textColor = text_c, .wrapMode = CLAY_TEXT_WRAP_WORDS,
        .lineHeight = 23
    };

    if (tut->step == -2) {
        /* Splash prompt: "Would you like to see the tutorial?" */
        CLAY(CLAY_ID("TutRoot"), {
            .layout = { .sizing = { CLAY_SIZING_GROW(0), CLAY_SIZING_GROW(0) },
                         .childAlignment = { .x = CLAY_ALIGN_X_CENTER,
                                              .y = CLAY_ALIGN_Y_CENTER } },
            .backgroundColor = {0, 0, 0, 160}
        }) {
            CLAY(CLAY_ID("TutPrompt"), {
                .layout = { .layoutDirection = CLAY_TOP_TO_BOTTOM,
                             .sizing = { CLAY_SIZING_FIXED(420), CLAY_SIZING_FIT(0) },
                             .padding = {24, 24, 20, 20}, .childGap = 16 },
                .backgroundColor = panel_c,
                .cornerRadius = CLAY_CORNER_RADIUS(8)
            }) {
                CLAY_TEXT(S(TR(STR_TUT_PROMPT)),
                          CLAY_TEXT_CONFIG({ .fontId = FONT_UI, .fontSize = 20,
                              .letterSpacing = 1, .textColor = text_c }));
                CLAY_TEXT(S(TR(STR_TUT_PROMPT_SUB)),
                          CLAY_TEXT_CONFIG(body_cfg));
                CLAY(CLAY_ID("TutPromptBtns"), {
                    .layout = { .childGap = 8,
                        .sizing = { CLAY_SIZING_GROW(0), CLAY_SIZING_FIT(0) },
                        .childAlignment = { .x = CLAY_ALIGN_X_RIGHT } }
                }) {
                    if (tut_button(CLAY_ID("TutNo"), TR(STR_TUT_NO_THANKS), btn_c, btn_hover, text_c))
                        tutorial_stop(app);
                    if (tut_button(CLAY_ID("TutYes"), TR(STR_TUT_YES), accent, accent_hover, white)) {
                        tut->step = -1;
                    }
                }
            }
        }
        return;
    }

    if (tut->step == -1) {
        /* Mode choice: brief or detailed */
        CLAY(CLAY_ID("TutRoot"), {
            .layout = { .sizing = { CLAY_SIZING_GROW(0), CLAY_SIZING_GROW(0) },
                         .childAlignment = { .x = CLAY_ALIGN_X_CENTER,
                                              .y = CLAY_ALIGN_Y_CENTER } },
            .backgroundColor = {0, 0, 0, 160}
        }) {
            CLAY(CLAY_ID("TutChoice"), {
                .layout = { .layoutDirection = CLAY_TOP_TO_BOTTOM,
                             .sizing = { CLAY_SIZING_FIXED(420), CLAY_SIZING_FIT(0) },
                             .padding = {24, 24, 20, 20}, .childGap = 16 },
                .backgroundColor = panel_c,
                .cornerRadius = CLAY_CORNER_RADIUS(8)
            }) {
                CLAY_TEXT(S(TR(STR_TUT_MODE_Q)),
                          CLAY_TEXT_CONFIG({ .fontId = FONT_UI, .fontSize = 20,
                              .letterSpacing = 1, .textColor = text_c }));
                CLAY(CLAY_ID("TutChoiceBtns"), {
                    .layout = { .childGap = 8,
                        .sizing = { CLAY_SIZING_GROW(0), CLAY_SIZING_FIT(0) } }
                }) {
                    if (tut_button(CLAY_ID("TutBrief"), TR(STR_TUT_BRIEF), accent, accent_hover, white))
                        tutorial_start(app, 0);
                    if (tut_button(CLAY_ID("TutDetail"), TR(STR_TUT_DETAILED), accent, accent_hover, white))
                        tutorial_start(app, 1);
                }
            }
        }
        return;
    }

    /* Active step: floating panel at bottom-center */
    if (tut->step < 0 || tut->step >= tut->n_steps) return;

    const TutStep *s = &g_steps[tut->step];
    const char *msg = tr(tut->mode ? s->text_detail_id : s->text_brief_id);

    float pw = 500;
    if (pw > app->win_w - 40) pw = app->win_w - 40;
    float ox = (app->win_w - pw) * 0.5f;
    /* Position panel so its bottom is ~10px above the window bottom.
     * Estimate height from text length: ~23px per 60 chars + buttons + padding. */
    int msg_len = (int)strlen(msg);
    float lines = 1.0f + (float)(msg_len * 16) / (pw - 32); /* approx wrapped lines */
    float est_h = lines * 23.0f + 80.0f; /* text + buttons + padding + hint */
    float oy = app->win_h - est_h - 10.0f;
    if (oy < 40) oy = 40;

    CLAY(CLAY_ID("TutPanel"), {
        .floating = { .attachTo = CLAY_ATTACH_TO_ROOT, .zIndex = 30,
                      .offset = { .x = ox, .y = oy } },
        .layout = { .layoutDirection = CLAY_TOP_TO_BOTTOM,
                     .sizing = { CLAY_SIZING_FIXED(pw), CLAY_SIZING_FIT(0) },
                     .padding = {16, 16, 12, 12}, .childGap = 10 },
        .backgroundColor = panel_c,
        .border = { .width = CLAY_BORDER_OUTSIDE(1), .color = {100,100,110,160} },
        .cornerRadius = CLAY_CORNER_RADIUS(8)
    }) {
        /* Message text */
        CLAY_TEXT(S(msg), CLAY_TEXT_CONFIG(body_cfg));

        /* Check if action is complete (for visual feedback only) */
        int action_done = (s->action != TUT_ACT_NONE) && tutorial_check_action(app);

        /* Action hint or completion indicator */
        if (s->action != TUT_ACT_NONE) {
            if (action_done) {
                Clay_Color done_c = {80, 200, 100, 255};
                CLAY_TEXT(S(TR(STR_TUT_DONE_HINT)),
                          CLAY_TEXT_CONFIG({ .fontId = FONT_UI, .fontSize = 12,
                              .letterSpacing = 1, .textColor = done_c }));
            }
        }

        /* Button row: [Stop] ... [3/12] ... [Continue] */
        CLAY(CLAY_ID("TutBtnRow"), {
            .layout = { .childGap = 8,
                .sizing = { CLAY_SIZING_GROW(0), CLAY_SIZING_FIT(0) },
                .childAlignment = { .y = CLAY_ALIGN_Y_CENTER } }
        }) {
            if (tut_button(CLAY_ID("TutStop"), TR(STR_TUT_STOP), btn_c, btn_hover, dim_c))
                tutorial_stop(app);

            /* Spacer */
            CLAY_AUTO_ID({ .layout = { .sizing = { .width = CLAY_SIZING_GROW(0) } } }) {}

            /* Step counter */
            {
                char buf[16];
                int n = snprintf(buf, sizeof(buf), "%d / %d", tut->step + 1, tut->n_steps);
                CLAY_TEXT(((Clay_String){.length = n, .chars = buf}), CLAY_TEXT_CONFIG({
                    .fontId = FONT_MONO, .fontSize = 12, .letterSpacing = 1,
                    .textColor = dim_c }));
            }

            /* Spacer */
            CLAY_AUTO_ID({ .layout = { .sizing = { .width = CLAY_SIZING_GROW(0) } } }) {}

            if (tut_button(CLAY_ID("TutContinue"), TR(STR_TUT_CONTINUE), accent, accent_hover, white))
                tutorial_advance(app);
        }
    }
}
