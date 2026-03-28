/* i18n.c — Internationalization string tables for Lorentz Tracer.
 * One static array per language; NULL entries fall back to English. */

#include "i18n.h"
#include <stddef.h>
#include <string.h>

static LangId g_lang = LANG_EN;

/* ================================================================
 *  Language metadata
 * ================================================================ */

static const LangInfo g_lang_info[] = {
    /* Real languages */
    { LANG_EN,    "en",   "English",              "English",              NULL, NULL, 0 },
    { LANG_FR,    "fr",   "Fran\xc3\xa7" "ais",  "French",              NULL, NULL, 0 },
    { LANG_DE,    "de",   "Deutsch",              "German",              NULL, NULL, 0 },
    { LANG_ES,    "es",   "Espa\xc3\xb1ol",      "Spanish",             NULL, NULL, 0 },
    { LANG_PT,    "pt",   "Portugu\xc3\xaas",    "Portuguese",          NULL, NULL, 0 },
    { LANG_IT,    "it",   "Italiano",             "Italian",             NULL, NULL, 0 },
    { LANG_RU,    "ru",   "\xd0\xa0\xd1\x83\xd1\x81\xd1\x81\xd0\xba\xd0\xb8\xd0\xb9", "Russian", NULL, NULL, 0 },
    { LANG_JA,    "ja",   "\xe6\x97\xa5\xe6\x9c\xac\xe8\xaa\x9e",       "Japanese",
      "NotoSansJP-subset.ttf", "NotoSansJP-subset.ttf", 1 },
    { LANG_ZH_CN, "zh-CN","\xe7\xae\x80\xe4\xbd\x93\xe4\xb8\xad\xe6\x96\x87", "Chinese (Simplified)",
      "NotoSansSC-subset.ttf", "NotoSansSC-subset.ttf", 1 },
    { LANG_ZH_TW, "zh-TW","\xe7\xb9\x81\xe9\xab\x94\xe4\xb8\xad\xe6\x96\x87", "Chinese (Traditional)",
      "NotoSansTC-subset.ttf", "NotoSansTC-subset.ttf", 1 },
    { LANG_KO,    "ko",   "\xed\x95\x9c\xea\xb5\xad\xec\x96\xb4",     "Korean",
      "NotoSansKR-subset.ttf", "NotoSansKR-subset.ttf", 1 },
    { LANG_EO,    "eo",   "Esperanto",            "Esperanto",           NULL, NULL, 0 },
    /* Fun languages */
    { LANG_TLH,   "tlh",  "tlhIngan Hol",         "Klingon",            NULL, NULL, 0 },
    { LANG_VUL,   "vul",  "Technobabble",         "Technobabble",       NULL, NULL, 0 },
    { LANG_MORSE, "mo",   "-- --- .-. ... .",     "Morse Code",          NULL, NULL, 0 },
    { LANG_PIRATE,"arr",  "Pirate",               "Pirate",             NULL, NULL, 0 },
    { LANG_LEET,  "1337", "1337 5p34k",           "Leet Speak",         NULL, NULL, 0 },
    { LANG_SHAKESPEARE,"shk","Shakespearean",      "Shakespearean",      NULL, NULL, 0 },
    { LANG_OLD_ENGLISH,"oe","Eald Englisc",       "Old English",        NULL, NULL, 0 },
};

/* ================================================================
 *  ENGLISH (base language — every ID must have an entry)
 * ================================================================ */

static const char *g_strings_en[STR__COUNT] = {
    /* UI: Model / Particle */
    [STR_LORENTZ_TRACER]    = "Lorentz Tracer",
    [STR_MODEL_PARTICLE]    = "Model / Particle",
    [STR_FIELD_MODEL]       = "Field Model",
    [STR_CONFIG]            = "Config:",
    [STR_PARTICLE]          = "Particle",
    [STR_SPECIES_ELECTRON]  = "Electron",
    [STR_SPECIES_POSITRON]  = "Positron",
    [STR_SPECIES_PROTON]    = "Proton",
    [STR_SPECIES_ALPHA]     = "Alpha",
    [STR_SPECIES_OPLUS]     = "O+",
    [STR_SPECIES_CUSTOM]    = "Custom",
    [STR_CHARGE]            = "Charge",
    [STR_MASS]              = "Mass",
    [STR_SPEED]             = "Speed",
    [STR_ENERGY]            = "Energy",
    [STR_PITCH]             = "Pitch",

    /* UI: Playback / Record */
    [STR_PLAYBACK_RECORD]   = "Playback / Record",
    [STR_TRIGGER_REC]       = "Trigger rec (R)",
    [STR_TRIGGER_REC_ACTIVE]= "Trigger rec (recording...)",
    [STR_PAUSE]             = "Pause",
    [STR_RESUME]            = "Resume",
    [STR_RESET]             = "Reset",

    /* UI: Display */
    [STR_DISPLAY]           = "Display",
    [STR_TRAIL]             = "Trail",
    [STR_TRAIL_FADE]        = "Fade",
    [STR_FIELD_LINES]       = "Field lines",
    [STR_GC_FL]             = "GC fl",
    [STR_FL_POS]            = "FL pos",
    [STR_GC_LEN]            = "GC len",
    [STR_SCALED]            = "Scaled",
    [STR_V_SCALE]           = "v scale",
    [STR_B_SCALE]           = "B scale",
    [STR_F_SCALE]           = "F scale",
    [STR_GIJ]               = "G_ij (G)",
    [STR_SCALE_BAR]         = "Scale bar",
    [STR_AXES]              = "Axes",
    [STR_PLOTS]             = "Plots",
    [STR_BOTTOM]            = "Bottom",
    [STR_TOP]               = "Top",
    [STR_AUTOSCALE]         = "Autoscale",
    [STR_RADIATION]         = "Radiation",
    [STR_RAD_X]             = "Rad x",
    [STR_RELATIVISTIC]      = "Relativistic",
    [STR_FOLLOW]            = "Follow (F)",
    [STR_CAM_FREE]          = "Free (V)",
    [STR_CAM_PB]            = "+b (V)",
    [STR_CAM_MB]            = "-b (V)",

    /* UI: Readouts */
    [STR_READOUTS]          = "Readouts",
    [STR_FMT_PARTICLE_N]    = "particle %d / %d (Tab)",
    [STR_FMT_TIME]          = "t = %.4g s",
    [STR_FMT_ENERGY_ERR]    = "|dE|/E = %.2e",
    [STR_FMT_PITCH]         = "pitch = %.1f deg",
    [STR_FMT_MU]            = "mu = %.3e J/T",
    [STR_FMT_BMAG]          = "|B| = %.3e T",
    [STR_FMT_ENERGY_MEV]    = "E = %.3f MeV",
    [STR_FMT_ENERGY_KEV]    = "E = %.3f keV",
    [STR_FMT_PRAD]          = "P_rad = %.3e W",
    [STR_FMT_DEDT]          = "dE/dt = %.3e keV/s",
    [STR_FMT_DEDT_MULT]     = "dE/dt = %.3e keV/s (x%.3g)",

    /* UI: Settings */
    [STR_SETTINGS]          = "Settings",
    [STR_DARK_MODE]         = "Dark mode",
    [STR_PANEL]             = "Panel",
    [STR_LEFT]              = "Left",
    [STR_RIGHT]             = "Right",
    [STR_STEREO_3D]         = "Stereo 3D",
    [STR_SEPARATION]        = "Separation",
    [STR_GAP]               = "Gap",
    [STR_ARROWHEAD]         = "Arrowhead",
    [STR_LINE_WIDTHS]       = "Line widths",
    [STR_COLORS]            = "Colors",
    [STR_PARTICLES]         = "Particles",
    [STR_LW_TRAIL]          = "Trail",
    [STR_LW_B_LINES]        = "B lines",
    [STR_LW_FL_GC]          = "FL GC",
    [STR_LW_FL_POS]         = "FL pos",
    [STR_LW_ARROWS]         = "Arrows",
    [STR_LW_KAPPA]          = "\xce\xba / e\xe2\x82\x82",
    [STR_LW_AXES]           = "Axes",
    [STR_COLOR_V_ARROW]     = "v arrow",
    [STR_COLOR_B_ARROW]     = "B arrow",
    [STR_COLOR_F_ARROW]     = "F arrow",
    [STR_COLOR_FIELD_LINES] = "Field lines",
    [STR_COLOR_FL_GC]       = "FL GC",
    [STR_COLOR_FL_POS]      = "FL pos",
    [STR_COLOR_KAPPA]       = "\xce\xba-hat",
    [STR_COLOR_BINORMAL]    = "Binormal",
    [STR_COLOR_AXES]        = "Axes",
    [STR_LANGUAGE]          = "Language",

    /* Color picker */
    [STR_OK]                = "OK",
    [STR_CANCEL]            = "Cancel",

    /* HUD */
    [STR_SHOW_UI]           = "U: show UI",
    [STR_ARMED]             = "ARMED",

    /* Splash */
    [STR_TAP_CONTINUE]      = "tap to continue",

    /* Plots */
    [STR_PLOT_PITCH_ANGLE]  = "Pitch angle",
    [STR_PLOT_MAG_MOMENT]   = "Magnetic moment",
    [STR_PLOT_TIME]         = "Time (s)",

    /* Playback picker */
    [STR_OPEN_RECORDING]    = "Open Recording",
    [STR_NO_RECORDINGS]     = "No recordings found.",
    [STR_OPEN]              = "Open",
    [STR_PICKER_HELP]       = "Click to select, double-click or Open to play  |  Esc to cancel  |  Enter to open",

    /* Field model names */
    [STR_MODEL_CIRCULAR]    = "Circular B (const |B|)",
    [STR_MODEL_GRAD_B]      = "Linear grad-B",
    [STR_MODEL_QUAD_GRAD]   = "Quadratic grad-B",
    [STR_MODEL_SINUSOIDAL]  = "Sinusoidal grad-B",
    [STR_MODEL_NONPHYSICAL] = "Nonphysical (div B != 0)",
    [STR_MODEL_DIPOLE]      = "Magnetic dipole",
    [STR_MODEL_BOTTLE]      = "Magnetic bottle",
    [STR_MODEL_TOKAMAK]     = "Tokamak",
    [STR_MODEL_STELLARATOR] = "Stellarator (NAX)",
    [STR_MODEL_TORUS]       = "Torus",

    /* Field parameter names */
    [STR_PARAM_B0]          = "B0 (T)",
    [STR_PARAM_LAMBDA_1M]   = "lambda (1/m)",
    [STR_PARAM_LAMBDA_1M2]  = "lambda (1/m\xc2\xb2)",
    [STR_PARAM_AMPLITUDE]   = "Amplitude a",
    [STR_PARAM_WAVENUMBER]  = "Wavenumber k (1/m)",
    [STR_PARAM_LAMBDA_TM]   = "lambda (T/m)",
    [STR_PARAM_M_DIPOLE]    = "M (T\xc2\xb7m\xc2\xb3)",
    [STR_PARAM_B0_BOTTLE]   = "B0 (T)",
    [STR_PARAM_LM]          = "Lm (m)",
    [STR_PARAM_R0]          = "R0 (m)",
    [STR_PARAM_Q_SAFETY]    = "Safety factor q",
    [STR_PARAM_A_MINOR]     = "a (m)",
    [STR_PARAM_CONFIG]      = "Config",
    [STR_PARAM_R0_PHYS]     = "R0_phys (m)",
    [STR_PARAM_B0_PHYS]     = "B0_phys (T)",
    [STR_STEL_CFG_R2]       = "r2 s5.1 (QA)",
    [STR_STEL_CFG_LP22]     = "Precise QA (LP22)",

    /* Tutorial */
    [STR_TUT_PROMPT]        = "Would you like to see the tutorial?",
    [STR_TUT_PROMPT_SUB]    = "A short walkthrough of the interface and controls.",
    [STR_TUT_MODE_Q]        = "How much detail would you like?",
    [STR_TUT_BRIEF]         = "Brief overview",
    [STR_TUT_DETAILED]      = "Detailed walkthrough",
    [STR_TUT_NO_THANKS]     = "No thanks",
    [STR_TUT_YES]           = "Yes",
    [STR_TUT_STOP]          = "Stop",
    [STR_TUT_CONTINUE]      = "Continue",
    [STR_TUT_DONE_HINT]     = "Done! Press Continue when ready.",

    [STR_TUT_S0_BRIEF]  = "The simulation is paused. Press Spacebar to play or pause.",
    [STR_TUT_S0_DETAIL] = "The simulation is paused. Press Spacebar to play or pause. "
        "While paused, press N to step forward one timestep, or P to "
        "step backward. Hold either key to advance continuously at "
        "the current playback speed.",
    [STR_TUT_S1_BRIEF]  = "Right-click and drag to orbit the camera. Scroll to zoom. "
        "The colored axes show x (red), y (green), z (blue).",
    [STR_TUT_S1_DETAIL] = "Right-click and drag to orbit the camera around the target "
        "point. Scroll to zoom in and out. The colored axes at the "
        "origin show x (red), y (green), z (blue). Shift+right-drag "
        "orbits around the origin. Press 0 to re-center the view. "
        "The vector arrows are hidden for now so you can see the "
        "orbit and field geometry clearly.",
    [STR_TUT_S2_BRIEF]  = "The curves in the scene are magnetic field lines. They show "
        "the direction and geometry of the magnetic field.",
    [STR_TUT_S2_DETAIL] = "Field lines trace the direction of B throughout the domain. "
        "The particle gyrates around and drifts along these lines. "
        "In the Display section you can also enable the guiding-center "
        "field line (GC fl), which follows the averaged orbit center, "
        "and the position field line (FL pos) through the particle's "
        "current location.",
    [STR_TUT_S3_BRIEF]  = "Press F to follow the particle with the camera. "
        "Scroll to zoom in close to the orbit.",
    [STR_TUT_S3_DETAIL] = "Press F to toggle follow mode. The camera tracks the "
        "particle's position while you can still orbit and zoom. "
        "Once following, scroll to zoom in close to the particle. "
        "The next steps will turn on vector arrows at the particle "
        "position, so getting close makes them easier to see.",
    [STR_TUT_S4_BRIEF]  = "The arrows at the particle show the velocity (v) and the "
        "magnetic field direction (B).",
    [STR_TUT_S4_DETAIL] = "The velocity and B-field arrows are now turned on. "
        "The velocity vector (v) shows the particle's instantaneous "
        "direction of motion. The magnetic field vector (B) shows "
        "the local field direction. Both are drawn as unit vectors "
        "by default; enable Scaled mode in the Display section to "
        "show their magnitudes instead.",
    [STR_TUT_S5_BRIEF]  = "The Lorentz force F = q(v \xc3\x97 B) is perpendicular to both "
        "v and B. Its unit vector is now shown as an arrow.",
    [STR_TUT_S5_DETAIL] = "The Lorentz force F = q(v \xc3\x97 B) is always perpendicular "
        "to both the velocity and the magnetic field. This is what "
        "curves the particle's path into a helix. Its unit vector is "
        "now shown as an arrow. You can toggle any of the v, B, and "
        "F vectors individually in the Display section of the side panel.",
    [STR_TUT_S6_BRIEF]  = "Press V to look along the magnetic field direction. "
        "Arrow keys rotate the view orientation.",
    [STR_TUT_S6_DETAIL] = "Press V to cycle between free camera, looking along +B, "
        "and looking along -B. In field-aligned views, the "
        "curvature vector (\xce\xba) and binormal are shown as arrows. "
        "Arrow keys rotate which direction \xce\xba points on screen. "
        "This view reveals the drift motion perpendicular to B.",
    [STR_TUT_S7_BRIEF]  = "The side panel (U to toggle) has collapsible sections: "
        "Model/Particle, Playback, Display, and Readouts.",
    [STR_TUT_S7_DETAIL] = "Press U to show or hide the side panel. It has four "
        "collapsible sections. Model/Particle selects the magnetic "
        "field geometry and particle species, energy, and pitch angle. "
        "Playback controls simulation speed and recording. Display "
        "toggles visual elements. Readouts show live diagnostics. "
        "The gear button opens Settings for colors and line widths.",
    [STR_TUT_S8_BRIEF]  = "Particle settings like energy (or speed for Custom) and "
        "pitch angle take effect on reset (R) or when placing a "
        "new particle with Shift+click.",
    [STR_TUT_S8_DETAIL] = "In the Model/Particle section, you can change the species, "
        "energy (or speed for the Custom species), and pitch angle. "
        "These are templates: they take effect the next time you "
        "press R to reset, or when you place a new particle with "
        "Left Shift + click. Existing particles keep their original "
        "parameters.",
    [STR_TUT_S9_BRIEF]  = "Try selecting the Dipole field model from the dropdown. "
        "The view resets automatically when switching models.",
    [STR_TUT_S9_DETAIL] = "The Field Model dropdown lists 10 magnetic field geometries: "
        "circular, grad-B, dipole, magnetic bottle, tokamak, and more. "
        "Each model has adjustable parameters. Selecting a new model "
        "resets the particle and camera to defaults. Try selecting "
        "the Dipole model to see trapped particle motion.",
    [STR_TUT_S10_BRIEF] = "The telemetry plots show pitch angle and magnetic moment "
        "versus time.",
    [STR_TUT_S10_DETAIL]= "The bottom plots show pitch angle and magnetic moment "
        "(\xce\xbc) as functions of time. In a pure magnetic field, "
        "\xce\xbc is an adiabatic invariant and should stay nearly "
        "constant. The plot range buttons (x1/x10/x100) control "
        "how much history is shown. Autoscale adjusts the pitch "
        "axis range automatically.",
    [STR_TUT_S11_BRIEF] = "Press 0 to reset the camera view. Then hold Left Shift "
        "and click in the scene to place additional particles. "
        "Tab cycles selection.",
    [STR_TUT_S11_DETAIL]= "Press 0 to reset the camera to the default view. Then "
        "hold Left Shift and click in the 3D scene to place a "
        "new particle with the current species, energy, and pitch "
        "settings. Up to 16 particles can be traced simultaneously. "
        "Press Tab to cycle which particle the camera follows and "
        "whose diagnostics are shown.",
    [STR_TUT_S12_BRIEF] = "Press R to reset the simulation back to a single particle.",
    [STR_TUT_S12_DETAIL]= "Press R to reset the simulation. This clears all placed "
        "particles and returns to a single particle with the current "
        "settings. Any changes you made to species, energy, speed, "
        "or pitch angle in the panel are applied on reset.",
    [STR_TUT_S13_BRIEF] = "Enable Trigger rec in Playback, then press R to start "
        "and stop video recording. Shift-P replays event logs.",
    [STR_TUT_S13_DETAIL]= "In the Playback section, enable Trigger rec. Then press R "
        "to reset the simulation and begin recording an H.264 video. "
        "Press R again to stop. An event log is saved alongside the "
        "video. Press Shift-P to open and replay a previous recording, "
        "reproducing the exact simulation.",
    [STR_TUT_S14_BRIEF] = "That covers the essentials. Press F1 anytime to open "
        "the help reference.",
    [STR_TUT_S14_DETAIL]= "That covers the essentials. Press F1 anytime to open "
        "the help reference, which has detailed documentation on "
        "keyboard shortcuts, physics, field models, and the interface. "
        "The Settings panel (gear icon) has color and line width "
        "customization. Enjoy exploring.",

    /* Help */
    [STR_HELP_TITLE]        = "Lorentz Tracer Help",
    [STR_HELP_TAB_KEYS]     = "Keys",
    [STR_HELP_TAB_PHYSICS]  = "Physics",
    [STR_HELP_TAB_FIELDS]   = "Fields",
    [STR_HELP_TAB_INTERFACE]= "Interface",
    [STR_HELP_TAB_ABOUT]    = "About",
    [STR_HELP_FOOTER]       = "Esc or F1 to close  |  Arrow keys or Tab to switch tabs  |  Scroll to navigate",
    [STR_HELP_RUN_TUTORIAL] = "Run Tutorial",
    [STR_HELP_RUN_TUTORIAL_DESC] = "Walk through the interface and controls step by step.",
    [STR_HELP_FONT_SIZE]    = "FONT SIZE",
#ifdef __APPLE__
    [STR_HELP_FONT_SIZE_DESC] = "Cmd+Plus / Cmd+Minus adjusts the font size for the UI panel, "
        "help overlay, and telemetry plots independently depending on "
        "which area the mouse is over. Cmd+0 resets to default. "
        "All font sizes are saved between sessions.",
#else
    [STR_HELP_FONT_SIZE_DESC] = "Ctrl+Plus / Ctrl+Minus adjusts the font size for the UI panel, "
        "help overlay, and telemetry plots independently depending on "
        "which area the mouse is over. Ctrl+0 resets to default. "
        "All font sizes are saved between sessions.",
#endif
    [STR_HELP_FACTORY_RESET]= "Factory Reset",
    [STR_HELP_FACTORY_RESET_DESC] = "Removes saved settings and restores all defaults.",

    /* Help content: reserved block — filled in when help.c is converted */

    /* Explorer */
    [STR_EXPLORE]            = "Explore",
    [STR_EXPLORE_LOAD]       = "Load Scenario",
    [STR_EXPLORE_EXPORT]     = "Export Template",
    [STR_EXPLORE_OPEN]       = "Open Scenario",
    [STR_EXPLORE_NONE]       = "No scenarios found.",
    [STR_EXPLORE_PICKER_HELP] = "Click to select, double-click to open.",

    /* Language selection */
    [STR_LANG_TITLE]        = "Language (beta translations, help welcome)",
    [STR_LANG_CONTINUE]     = "Continue",
};

/* ================================================================
 *  Translation tables (real languages — machine-translated, reviewed)
 * ================================================================ */

#include "../i18n/generated_tables.c"

/* Fun languages */
#include "../i18n/fun_tables.c"

/* ================================================================
 *  Dispatch table
 * ================================================================ */

static const char **g_tables[LANG__COUNT] = {
    [LANG_EN]           = g_strings_en,
    [LANG_FR]           = g_strings_fr,
    [LANG_DE]           = g_strings_de,
    [LANG_ES]           = g_strings_es,
    [LANG_PT]           = g_strings_pt,
    [LANG_IT]           = g_strings_it,
    [LANG_RU]           = g_strings_ru,
    [LANG_JA]           = g_strings_ja,
    [LANG_ZH_CN]        = g_strings_zh_cn,
    [LANG_ZH_TW]        = g_strings_zh_tw,
    [LANG_KO]           = g_strings_ko,
    [LANG_EO]           = g_strings_eo,
    [LANG_TLH]          = g_strings_tlh,
    [LANG_VUL]          = g_strings_vul,
    [LANG_MORSE]        = g_strings_morse,
    [LANG_PIRATE]       = g_strings_pirate,
    [LANG_LEET]         = g_strings_leet,
    [LANG_SHAKESPEARE]  = g_strings_shk,
    [LANG_OLD_ENGLISH]  = g_strings_oe,
};

/* ================================================================
 *  API
 * ================================================================ */

void i18n_init(void) { g_lang = LANG_EN; }

void i18n_set_lang(LangId lang)
{
    if (lang >= 0 && lang < LANG__COUNT) g_lang = lang;
}

LangId i18n_get_lang(void) { return g_lang; }

const char *i18n_lang_code(void)
{
    return g_lang_info[g_lang].code;
}

LangId i18n_lang_from_code(const char *code)
{
    for (int i = 0; i < LANG__COUNT; i++)
        if (strcmp(g_lang_info[i].code, code) == 0) return (LangId)i;
    return LANG_EN;
}

const char *tr(StrId id)
{
    if (id < 0 || id >= STR__COUNT) return "???";
    const char *s = g_tables[g_lang] ? g_tables[g_lang][id] : NULL;
    if (!s) s = g_strings_en[id];
    if (!s) return "???";
    return s;
}

const LangInfo *i18n_lang_info(LangId lang)
{
    if (lang < 0 || lang >= LANG__COUNT) return &g_lang_info[LANG_EN];
    return &g_lang_info[lang];
}
