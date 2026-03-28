#ifndef I18N_H
#define I18N_H

/* ---- String IDs ---- */

typedef enum {
    /* UI panel: Model / Particle (ui.c) */
    STR_LORENTZ_TRACER,         /* "Lorentz Tracer" */
    STR_MODEL_PARTICLE,         /* "Model / Particle" */
    STR_FIELD_MODEL,            /* "Field Model" */
    STR_CONFIG,                 /* "Config:" */
    STR_PARTICLE,               /* "Particle" */
    STR_SPECIES_ELECTRON,       /* "Electron" */
    STR_SPECIES_POSITRON,       /* "Positron" */
    STR_SPECIES_PROTON,         /* "Proton" */
    STR_SPECIES_ALPHA,          /* "Alpha" */
    STR_SPECIES_OPLUS,          /* "O+" */
    STR_SPECIES_CUSTOM,         /* "Custom" */
    STR_CHARGE,                 /* "Charge" */
    STR_MASS,                   /* "Mass" */
    STR_SPEED,                  /* "Speed" */
    STR_ENERGY,                 /* "Energy" */
    STR_PITCH,                  /* "Pitch" */

    /* UI: Playback / Record */
    STR_PLAYBACK_RECORD,        /* "Playback / Record" */
    STR_TRIGGER_REC,            /* "Trigger rec (R)" */
    STR_TRIGGER_REC_ACTIVE,     /* "Trigger rec (recording...)" */
    STR_PAUSE,                  /* "Pause" */
    STR_RESUME,                 /* "Resume" */
    STR_RESET,                  /* "Reset" */

    /* UI: Display */
    STR_DISPLAY,                /* "Display" */
    STR_TRAIL,                  /* "Trail" */
    STR_TRAIL_FADE,             /* "Fade" */
    STR_FIELD_LINES,            /* "Field lines" */
    STR_GC_FL,                  /* "GC fl" */
    STR_FL_POS,                 /* "FL pos" */
    STR_GC_LEN,                /* "GC len" */
    STR_SCALED,                 /* "Scaled" */
    STR_V_SCALE,                /* "v scale" */
    STR_B_SCALE,                /* "B scale" */
    STR_F_SCALE,                /* "F scale" */
    STR_GIJ,                    /* "G_ij (G)" */
    STR_SCALE_BAR,              /* "Scale bar" */
    STR_AXES,                   /* "Axes" */
    STR_PLOTS,                  /* "Plots" */
    STR_BOTTOM,                 /* "Bottom" */
    STR_TOP,                    /* "Top" */
    STR_AUTOSCALE,              /* "Autoscale" */
    STR_RADIATION,              /* "Radiation" */
    STR_RAD_X,                  /* "Rad x" */
    STR_RELATIVISTIC,           /* "Relativistic" */
    STR_FOLLOW,                 /* "Follow (F)" */
    STR_CAM_FREE,               /* "Free (V)" */
    STR_CAM_PB,                 /* "+b (V)" */
    STR_CAM_MB,                 /* "-b (V)" */

    /* UI: Readouts */
    STR_READOUTS,               /* "Readouts" */
    STR_FMT_PARTICLE_N,         /* "particle %d / %d (Tab)" */
    STR_FMT_TIME,               /* "t = %.4g s" */
    STR_FMT_ENERGY_ERR,         /* "|dE|/E = %.2e" */
    STR_FMT_PITCH,              /* "pitch = %.1f deg" */
    STR_FMT_MU,                 /* "mu = %.3e J/T" */
    STR_FMT_BMAG,               /* "|B| = %.3e T" */
    STR_FMT_ENERGY_MEV,         /* "E = %.3f MeV" */
    STR_FMT_ENERGY_KEV,         /* "E = %.3f keV" */
    STR_FMT_PRAD,               /* "P_rad = %.3e W" */
    STR_FMT_DEDT,               /* "dE/dt = %.3e keV/s" */
    STR_FMT_DEDT_MULT,          /* "dE/dt = %.3e keV/s (x%.3g)" */

    /* UI: Settings */
    STR_SETTINGS,               /* "Settings" */
    STR_DARK_MODE,              /* "Dark mode" */
    STR_PANEL,                  /* "Panel" */
    STR_LEFT,                   /* "Left" */
    STR_RIGHT,                  /* "Right" */
    STR_STEREO_3D,              /* "Stereo 3D" */
    STR_SEPARATION,             /* "Separation" */
    STR_GAP,                    /* "Gap" */
    STR_ARROWHEAD,              /* "Arrowhead" */
    STR_LINE_WIDTHS,            /* "Line widths" */
    STR_COLORS,                 /* "Colors" */
    STR_PARTICLES,              /* "Particles" */
    STR_LW_TRAIL,               /* "Trail" (line width row) */
    STR_LW_B_LINES,             /* "B lines" */
    STR_LW_FL_GC,               /* "FL GC" */
    STR_LW_FL_POS,              /* "FL pos" */
    STR_LW_ARROWS,              /* "Arrows" */
    STR_LW_KAPPA,               /* "κ / e₂" */
    STR_LW_AXES,                /* "Axes" */
    STR_COLOR_V_ARROW,          /* "v arrow" */
    STR_COLOR_B_ARROW,          /* "B arrow" */
    STR_COLOR_F_ARROW,          /* "F arrow" */
    STR_COLOR_FIELD_LINES,      /* "Field lines" */
    STR_COLOR_FL_GC,            /* "FL GC" */
    STR_COLOR_FL_POS,           /* "FL pos" */
    STR_COLOR_KAPPA,            /* "κ-hat" */
    STR_COLOR_BINORMAL,         /* "Binormal" */
    STR_COLOR_AXES,             /* "Axes" */
    STR_LANGUAGE,               /* "Language" */

    /* Color picker */
    STR_OK,                     /* "OK" */
    STR_CANCEL,                 /* "Cancel" */

    /* HUD overlays (app.c) */
    STR_SHOW_UI,                /* "U: show UI" */
    STR_ARMED,                  /* "ARMED" */

    /* Splash / main */
    STR_TAP_CONTINUE,           /* "tap to continue" */

    /* Plots (render2d.c) */
    STR_PLOT_PITCH_ANGLE,       /* "Pitch angle" */
    STR_PLOT_MAG_MOMENT,        /* "Magnetic moment" */
    STR_PLOT_TIME,              /* "Time (s)" */

    /* Playback file picker */
    STR_OPEN_RECORDING,         /* "Open Recording" */
    STR_NO_RECORDINGS,          /* "No recordings found." */
    STR_OPEN,                   /* "Open" */
    STR_PICKER_HELP,            /* "Click to select..." */

    /* Field model names */
    STR_MODEL_CIRCULAR,         /* "Circular B (const |B|)" */
    STR_MODEL_GRAD_B,           /* "Linear grad-B" */
    STR_MODEL_QUAD_GRAD,        /* "Quadratic grad-B" */
    STR_MODEL_SINUSOIDAL,       /* "Sinusoidal grad-B" */
    STR_MODEL_NONPHYSICAL,      /* "Nonphysical (div B != 0)" */
    STR_MODEL_DIPOLE,           /* "Magnetic dipole" */
    STR_MODEL_BOTTLE,           /* "Magnetic bottle" */
    STR_MODEL_TOKAMAK,          /* "Tokamak" */
    STR_MODEL_STELLARATOR,      /* "Stellarator (NAX)" */
    STR_MODEL_TORUS,            /* "Torus" */

    /* Field parameter names */
    STR_PARAM_B0,               /* "B0 (T)" */
    STR_PARAM_LAMBDA_1M,        /* "lambda (1/m)" */
    STR_PARAM_LAMBDA_1M2,       /* "lambda (1/m²)" */
    STR_PARAM_AMPLITUDE,        /* "Amplitude a" */
    STR_PARAM_WAVENUMBER,       /* "Wavenumber k (1/m)" */
    STR_PARAM_LAMBDA_TM,        /* "lambda (T/m)" */
    STR_PARAM_M_DIPOLE,         /* "M (T*m³)" */
    STR_PARAM_B0_BOTTLE,        /* "B0 (T)" (bottle) */
    STR_PARAM_LM,               /* "Lm (m)" */
    STR_PARAM_R0,               /* "R0 (m)" */
    STR_PARAM_Q_SAFETY,         /* "Safety factor q" */
    STR_PARAM_A_MINOR,          /* "a (m)" */
    STR_PARAM_CONFIG,           /* "Config" */
    STR_PARAM_R0_PHYS,          /* "R0_phys (m)" */
    STR_PARAM_B0_PHYS,          /* "B0_phys (T)" */

    /* Stellarator config names */
    STR_STEL_CFG_R2,            /* "r2 s5.1 (QA)" */
    STR_STEL_CFG_LP22,          /* "Precise QA (LP22)" */

    /* ---- Tutorial (tutorial.c) ---- */
    STR_TUT_PROMPT,             /* "Would you like to see the tutorial?" */
    STR_TUT_PROMPT_SUB,         /* "A short walkthrough..." */
    STR_TUT_MODE_Q,             /* "How much detail would you like?" */
    STR_TUT_BRIEF,              /* "Brief overview" */
    STR_TUT_DETAILED,           /* "Detailed walkthrough" */
    STR_TUT_NO_THANKS,          /* "No thanks" */
    STR_TUT_YES,                /* "Yes" */
    STR_TUT_STOP,               /* "Stop" */
    STR_TUT_CONTINUE,           /* "Continue" */
    STR_TUT_DONE_HINT,          /* "Done! Press Continue when ready." */
    /* 15 steps x 2 modes = 30 strings */
    STR_TUT_S0_BRIEF, STR_TUT_S0_DETAIL,
    STR_TUT_S1_BRIEF, STR_TUT_S1_DETAIL,
    STR_TUT_S2_BRIEF, STR_TUT_S2_DETAIL,
    STR_TUT_S3_BRIEF, STR_TUT_S3_DETAIL,
    STR_TUT_S4_BRIEF, STR_TUT_S4_DETAIL,
    STR_TUT_S5_BRIEF, STR_TUT_S5_DETAIL,
    STR_TUT_S6_BRIEF, STR_TUT_S6_DETAIL,
    STR_TUT_S7_BRIEF, STR_TUT_S7_DETAIL,
    STR_TUT_S8_BRIEF, STR_TUT_S8_DETAIL,
    STR_TUT_S9_BRIEF, STR_TUT_S9_DETAIL,
    STR_TUT_S10_BRIEF, STR_TUT_S10_DETAIL,
    STR_TUT_S11_BRIEF, STR_TUT_S11_DETAIL,
    STR_TUT_S12_BRIEF, STR_TUT_S12_DETAIL,
    STR_TUT_S13_BRIEF, STR_TUT_S13_DETAIL,
    STR_TUT_S14_BRIEF, STR_TUT_S14_DETAIL,

    /* ---- Help (help.c) ---- */
    STR_HELP_TITLE,             /* "Lorentz Tracer Help" */
    STR_HELP_TAB_KEYS,          /* "Keys" */
    STR_HELP_TAB_PHYSICS,       /* "Physics" */
    STR_HELP_TAB_FIELDS,        /* "Fields" */
    STR_HELP_TAB_INTERFACE,     /* "Interface" */
    STR_HELP_TAB_ABOUT,         /* "About" */
    STR_HELP_FOOTER,            /* "Esc or F1 to close | ..." */
    STR_HELP_RUN_TUTORIAL,      /* "Run Tutorial" */
    STR_HELP_RUN_TUTORIAL_DESC, /* "Walk through the interface..." */
    STR_HELP_FONT_SIZE,         /* "FONT SIZE" */
    STR_HELP_FONT_SIZE_DESC,    /* font size description */
    STR_HELP_FACTORY_RESET,     /* "Factory Reset" */
    STR_HELP_FACTORY_RESET_DESC,/* "Removes saved settings..." */

    /* Help tab content is bulk text — use sequential IDs.
     * Each H/P/M/ML call gets its own ID. The exact mapping
     * is established when converting help.c. IDs are reserved
     * here in blocks. */
    STR_HELP_CONTENT_START,
    /* Reserve 250 IDs for help content */
    STR_HELP_CONTENT_END = STR_HELP_CONTENT_START + 249,

    /* Explorer */
    STR_EXPLORE,                /* "Explore" */
    STR_EXPLORE_LOAD,           /* "Load Scenario" */
    STR_EXPLORE_EXPORT,         /* "Export Template" */
    STR_EXPLORE_OPEN,           /* "Open Scenario" */
    STR_EXPLORE_NONE,           /* "No scenarios found." */
    STR_EXPLORE_PICKER_HELP,    /* "Click to select, double-click to open." */

    /* Language selection */
    STR_LANG_TITLE,             /* "Language / Langue / Sprache" */
    STR_LANG_CONTINUE,          /* "Continue" */

    STR__COUNT
} StrId;

/* ---- Language IDs ---- */

typedef enum {
    /* Real languages */
    LANG_EN, LANG_FR, LANG_DE, LANG_ES, LANG_PT, LANG_IT, LANG_RU,
    LANG_JA, LANG_ZH_CN, LANG_ZH_TW, LANG_KO, LANG_EO,
    /* Fun languages */
    LANG_TLH, LANG_VUL, LANG_MORSE, LANG_PIRATE,
    LANG_LEET, LANG_SHAKESPEARE, LANG_OLD_ENGLISH,
    LANG__COUNT
} LangId;

/* ---- Language metadata ---- */

typedef struct {
    LangId id;
    const char *code;       /* "en", "fr", "de", etc. */
    const char *name;       /* native name (UTF-8) */
    const char *name_en;    /* English name */
    const char *font_ui;    /* UI font filename (NULL = use default Inter) */
    const char *font_mono;  /* mono font filename (NULL = use default SourceCodePro) */
    int needs_cjk;          /* 1 if CJK codepoints needed */
} LangInfo;

/* ---- API ---- */

void        i18n_init(void);
void        i18n_set_lang(LangId lang);
LangId      i18n_get_lang(void);
const char *i18n_lang_code(void);
LangId      i18n_lang_from_code(const char *code); /* returns LANG_EN if not found */
const char *tr(StrId id);
const LangInfo *i18n_lang_info(LangId lang);

#define TR(id) tr(id)

#endif
