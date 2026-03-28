/* fun_tables.c — Fun language translation tables for Lorentz Tracer.
 * Seven novelty languages: Klingon, Vulcan, Morse Code, Pirate,
 * Leet Speak, Shakespearean, Old English.
 * NULL entries fall back to English. */

/* ================================================================
 *  KLINGON (tlhIngan Hol)
 * ================================================================ */

static const char *g_strings_tlh[STR__COUNT] = {
    [STR_MODEL_PARTICLE] = "Dotlh / Hap mach",
    [STR_FIELD_MODEL] = "HoS lang Dotlh",
    [STR_CONFIG] = "tay':",
    [STR_PARTICLE] = "Hap mach",
    [STR_SPECIES_ELECTRON] = "'er'In",
    [STR_SPECIES_POSITRON] = "'er'In QaQ",
    [STR_SPECIES_PROTON] = "ngoDwI'",
    [STR_SPECIES_ALPHA] = "logh'a'",
    [STR_SPECIES_CUSTOM] = "pIm",
    [STR_CHARGE] = "HoS'a'",
    [STR_MASS] = "'ugh",
    [STR_SPEED] = "Do",
    [STR_ENERGY] = "HoS",
    [STR_PITCH] = "juch",
    [STR_PLAYBACK_RECORD] = "much / qon",
    [STR_TRIGGER_REC] = "qon yIchu' (R)",
    [STR_TRIGGER_REC_ACTIVE] = "qon (qonlI'...)",
    [STR_PAUSE] = "yImev",
    [STR_RESUME] = "yItaH",
    [STR_RESET] = "yIchelmoH",
    [STR_DISPLAY] = "'ang",
    [STR_TRAIL] = "ghIq",
    [STR_FIELD_LINES] = "HoS lang tlhegh",
    [STR_SCALED] = "chenmoHta'",
    [STR_SCALE_BAR] = "chenmoH Degh",
    [STR_AXES] = "tlhegh'a'",
    [STR_PLOTS] = "ngo'mey",
    [STR_BOTTOM] = "bIng",
    [STR_TOP] = "Dung",
    [STR_AUTOSCALE] = "chenmoH automatic",
    [STR_RADIATION] = "wovmoHwI'",
    [STR_RELATIVISTIC] = "Do'Ha' relativity",
    [STR_FOLLOW] = "tlha' (F)",
    [STR_CAM_FREE] = "tlhab (V)",
    [STR_READOUTS] = "De'",
    [STR_SETTINGS] = "choHmeH",
    [STR_DARK_MODE] = "Hurgh mIw",
    [STR_PANEL] = "beQ",
    [STR_LEFT] = "poS",
    [STR_RIGHT] = "nIH",
    [STR_STEREO_3D] = "wej'Ir Stereo",
    [STR_SEPARATION] = "chegh",
    [STR_GAP] = "DaH",
    [STR_ARROWHEAD] = "naQjej nach",
    [STR_LINE_WIDTHS] = "tlhegh woch",
    [STR_COLORS] = "nguv",
    [STR_PARTICLES] = "Hap mach'mey",
    [STR_LW_TRAIL] = "ghIq",
    [STR_LW_ARROWS] = "naQjej",
    [STR_LW_AXES] = "tlhegh'a'",
    [STR_COLOR_FIELD_LINES] = "HoS lang tlhegh",
    [STR_COLOR_AXES] = "tlhegh'a'",
    [STR_LANGUAGE] = "Hol",
    [STR_CANCEL] = "yIbotIH",
    [STR_SHOW_UI] = "U: HaSta yI'ang",
    [STR_ARMED] = "SuH!",
    [STR_TAP_CONTINUE] = "yIchov 'ej yItaH",
    [STR_PLOT_PITCH_ANGLE] = "juch Degh",
    [STR_PLOT_MAG_MOMENT] = "HoS lang bIH",
    [STR_PLOT_TIME] = "poH (s)",
    [STR_OPEN_RECORDING] = "qon yIpoSmoH",
    [STR_NO_RECORDINGS] = "qon tu'lu'be'.",
    [STR_OPEN] = "yIpoSmoH",
    [STR_PICKER_HELP] = "yIwIv, cha'logh yIchov pagh yIpoSmoH  |  Esc yImev  |  Enter yIpoSmoH",
    [STR_MODEL_CIRCULAR] = "gho B (taQ |B|)",
    [STR_MODEL_GRAD_B] = "tlhegh grad-B",
    [STR_MODEL_QUAD_GRAD] = "loS reH grad-B",
    [STR_MODEL_SINUSOIDAL] = "qul reH grad-B",
    [STR_MODEL_NONPHYSICAL] = "nap'a' (div B != 0)",
    [STR_MODEL_DIPOLE] = "HoS lang cha'logh",
    [STR_MODEL_BOTTLE] = "HoS lang bal",
    [STR_PARAM_AMPLITUDE] = "woch a",
    [STR_PARAM_Q_SAFETY] = "Hung factor q",
    [STR_PARAM_CONFIG] = "tay'",
    [STR_TUT_PROMPT] = "ghojmeH mIw DaneH'a'?",
    [STR_TUT_PROMPT_SUB] = "HaSta je SeH lughojmoH.",
    [STR_TUT_MODE_Q] = "De' 'ar DaneH?",
    [STR_TUT_BRIEF] = "bItlh 'ang",
    [STR_TUT_DETAILED] = "Hoch De' 'ang",
    [STR_TUT_NO_THANKS] = "ghobe'",
    [STR_TUT_YES] = "HIja'",
    [STR_TUT_STOP] = "yImev",
    [STR_TUT_CONTINUE] = "yItaH",
    [STR_TUT_DONE_HINT] = "Qapla'! yItaH DaneHchugh yIchov.",
    [STR_TUT_S0_BRIEF] = "Simulation mevta'. Spacebar yIchov 'ej yItaH pagh yImev.",
    [STR_TUT_S0_DETAIL] = "Simulation mevta'. Spacebar yIchov 'ej yItaH pagh yImev. "
        "mevDI', N yIchov wa' tup yIghoS, pagh P yIchov bIchegh. "
        "ghIq yIchov Do taH.",
    [STR_TUT_S1_BRIEF] = "nIH chov yIchov 'ej yIqeng jIHtaHbogh yIghIr. yIjIr 'ej yInaD. "
        "tlhegh nguv 'ang x (Doq), y (SuD), z (SuDqu').",
    [STR_TUT_S1_DETAIL] = "nIH chov yIchov 'ej yIqeng jIHtaHbogh DoS yIghIr. yIjIr 'ej yInaD. "
        "tlhegh nguv 'ang x (Doq), y (SuD), z (SuDqu'). "
        "Shift+nIH-qeng ghIr waw'. 0 yIchov leghmeH yIchelmoH. "
        "naQjej So'ta' DaH; Dotlh legh DaneHmo'.",
    [STR_TUT_S2_BRIEF] = "HoS lang tlhegh bIH tlhegh'e'. HoS lang lurgh 'ej Dotlh 'ang.",
    [STR_TUT_S2_DETAIL] = "HoS lang tlhegh B lurgh latlh Sep Hoch 'ang. "
        "Hap mach gho 'ej vIH HoS lang tlhegh. "
        "Display yI'ang GC fl 'ej FL pos je.",
    [STR_TUT_S3_BRIEF] = "F yIchov Hap mach yItlha'. yIjIr 'ej yInaD orbit Sum.",
    [STR_TUT_S3_DETAIL] = "F yIchov tlha' mIw yIchoH. jIHtaHbogh Hap mach "
        "tlha' 'ach ghIr naD laH. tlha'DI', yIjIr Sum Hap mach. "
        "naQjej tlhegh 'angmeH mIw veb bIH, vaj Sum qaD.",
    [STR_TUT_S4_BRIEF] = "naQjej Hap mach Do (v) 'ej HoS lang lurgh (B) 'ang.",
    [STR_TUT_S4_DETAIL] = "Do 'ej B-HoS lang naQjej DaH 'anglu'. "
        "Do naQjej (v) Hap mach vIHtaHghach DaH lurgh 'ang. "
        "HoS lang naQjej (B) HoS lang lurgh 'ang. "
        "cha' wa' naQjej chenlu'; Scaled yIchu' 'ej rab 'ang.",
    [STR_TUT_S5_BRIEF] = "Lorentz HoS F = q(v \xc3\x97 B) Do B je Sar yoS. "
        "DaH naQjej 'anglu'.",
    [STR_TUT_S5_DETAIL] = "Lorentz HoS F = q(v \xc3\x97 B) reH Do HoS lang je Sar yoS. "
        "Hap mach He helix chenmoH. DaH naQjej 'anglu'. "
        "v, B, F naQjej Display yIchoH.",
    [STR_TUT_S6_BRIEF] = "V yIchov HoS lang lurgh yIlegh. "
        "naQjej QIb lurgh choH.",
    [STR_TUT_S6_DETAIL] = "V yIchov tlhab jIHtaHbogh, +B legh, -B legh. "
        "HoS lang leghlu'DI', curvature (\xce\xba) 'ej binormal 'anglu'. "
        "naQjej QIb \xce\xba lurgh choH. vIH B Sar 'ang.",
    [STR_TUT_S7_BRIEF] = "beQ (U yIchoH) qay'mey ghaj: Dotlh/Hap mach, much, 'ang, De'.",
    [STR_TUT_S7_DETAIL] = "U yIchov beQ 'ang pagh So'. loS qay'mey ghaj. "
        "Dotlh/Hap mach HoS lang Dotlh 'ej Hap mach wIv. "
        "much Do SeH 'ej qon. 'ang yIchoH. De' DaH 'ang. "
        "choHmeH nguv tlhegh woch gear icon.",
    [STR_TUT_S8_BRIEF] = "Hap mach HoS (pagh Do Custom) 'ej juch Degh Reset (R) "
        "pagh Shift+chov Hap mach chu' cher.",
    [STR_TUT_S8_DETAIL] = "Dotlh/Hap mach qay', Hap mach wIv, HoS (pagh Do Custom), "
        "juch Degh choH. tay' bIH: R yIchov reset, pagh Shift+chov "
        "Hap mach chu' cher. Hap mach DaH taH choH'e' choHbe'.",
    [STR_TUT_S9_BRIEF] = "Dipole HoS lang Dotlh wIv yInID. "
        "Dotlh choHDI' legh chelmoH.",
    [STR_TUT_S9_DETAIL] = "HoS lang Dotlh wa'maH Dotlh ghaj: "
        "gho, grad-B, cha'logh, bal, tokamak, latlh je. "
        "Hoch Dotlh choHlaH. Dotlh chu' wIvDI' Hap mach 'ej jIH chelmoH. "
        "Dipole Dotlh yInID Hap mach vonlu'.",
    [STR_TUT_S10_BRIEF] = "ngo'mey juch Degh 'ej HoS lang bIH poH 'ang.",
    [STR_TUT_S10_DETAIL] = "bIng ngo'mey juch Degh 'ej HoS lang bIH (\xce\xbc) poH 'ang. "
        "HoS lang neH, \xce\xbc adiabatic invariant taQ. "
        "ngo' range button (x1/x10/x100) qun 'ar SeH. "
        "Autoscale juch Degh chuq choH.",
    [STR_TUT_S11_BRIEF] = "0 yIchov jIH chelmoH. Shift poS yIchov 'ej "
        "Hap mach chu' cher. Tab wIv choH.",
    [STR_TUT_S11_DETAIL] = "0 yIchov jIH chelmoH. Shift poS yIchov 'ej "
        "3D Hap mach chu' cher. wa'maH jav Hap mach taH laH. "
        "Tab yIchov Hap mach wIv choH 'ej De' choH.",
    [STR_TUT_S12_BRIEF] = "R yIchov simulation chelmoH wa' Hap mach.",
    [STR_TUT_S12_DETAIL] = "R yIchov simulation chelmoH. Hoch Hap mach teq 'ej "
        "wa' Hap mach chegh tay' DaH. choH Hap mach, HoS, Do, "
        "juch Degh beQ lo'lu' chelmoH.",
    [STR_TUT_S13_BRIEF] = "Trigger rec much yIchu', R yIchov qon yIghoS yImev. "
        "Shift-P qon much.",
    [STR_TUT_S13_DETAIL] = "much qay', Trigger rec yIchu'. R yIchov simulation chelmoH "
        "'ej H.264 video qon. R yIchov yImev. qon event log "
        "pollu'. Shift-P yIchov qon much, simulation rap chenmoH.",
    [STR_TUT_S14_BRIEF] = "Hoch yoD! F1 yIchov QaH yIpoSmoH.",
    [STR_TUT_S14_DETAIL] = "Hoch yoD! F1 yIchov QaH yIpoSmoH. "
        "keyboard, physics, HoS lang Dotlh, HaSta De' Hoch ghaj. "
        "choHmeH (gear icon) nguv tlhegh woch choH. "
        "yISuv 'ej yISam. Qapla'!",
    [STR_HELP_TITLE] = "Lorentz Tracer QaH",
    [STR_HELP_TAB_KEYS] = "chov",
    [STR_HELP_TAB_PHYSICS] = "meqbogh",
    [STR_HELP_TAB_FIELDS] = "HoS lang",
    [STR_HELP_TAB_INTERFACE] = "HaSta",
    [STR_HELP_TAB_ABOUT] = "wej",
    [STR_HELP_FOOTER] = "Esc pagh F1 SoQmoH  |  naQjej pagh Tab qay' choH  |  yIjIr yIghoS",
    [STR_HELP_RUN_TUTORIAL] = "ghojmeH mIw yIghoS",
    [STR_HELP_RUN_TUTORIAL_DESC] = "HaSta 'ej SeH wa' tup ghojmoH.",
    [STR_HELP_FONT_SIZE] = "GHITLH RAB",
    [STR_HELP_FONT_SIZE_DESC] = "Cmd+Plus / Cmd+Minus ghItlh rab choH beQ, QaH, ngo'mey pIm, "
        "jIH Daq. Cmd+0 chelmoH. ghItlh rab pollu' session.",
    [STR_HELP_FACTORY_RESET] = "wa'DIch chelmoH",
    [STR_HELP_FACTORY_RESET_DESC] = "choHmeH pollu' teq 'ej wa'DIch chelmoH.",
    [STR_LANG_CONTINUE] = "yItaH",
};

/* ================================================================
 *  TECHNOBABBLE (Star Trek style)
 * ================================================================ */

static const char *g_strings_vul[STR__COUNT] = {
    [STR_MODEL_PARTICLE] = "Subspace Emitter Config",
    [STR_FIELD_MODEL] = "Plasma Conduit Matrix",
    [STR_CONFIG] = "EPS Routing:",
    [STR_PARTICLE] = "Tachyon Pulse",
    [STR_SPECIES_ELECTRON] = "Electron",
    [STR_SPECIES_POSITRON] = "Antimatter Particle",
    [STR_SPECIES_PROTON] = "Proton",
    [STR_SPECIES_ALPHA] = "Helium Nucleus",
    [STR_SPECIES_CUSTOM] = "Reconfigured",
    [STR_CHARGE] = "Charge Polarity",
    [STR_MASS] = "Inertial Mass",
    [STR_SPEED] = "Impulse Velocity",
    [STR_ENERGY] = "Terajoule Output",
    [STR_PITCH] = "Deflector Angle",
    [STR_PLAYBACK_RECORD] = "Temporal Log / Capture",
    [STR_TRIGGER_REC] = "Arm sensor array (R)",
    [STR_TRIGGER_REC_ACTIVE] = "Sensor array recording...",
    [STR_PAUSE] = "Disengage",
    [STR_RESUME] = "Re-engage",
    [STR_RESET] = "Reinitialize Gel Packs",
    [STR_DISPLAY] = "Main Viewscreen",
    [STR_TRAIL] = "Ion Trail",
    [STR_FIELD_LINES] = "Flux Conduit Lines",
    [STR_SCALED] = "Magnitude Compensated",
    [STR_SCALE_BAR] = "Reference Calibration",
    [STR_AXES] = "Navigational Axes",
    [STR_PLOTS] = "Sensor Telemetry",
    [STR_BOTTOM] = "Ventral",
    [STR_TOP] = "Dorsal",
    [STR_AUTOSCALE] = "Auto-Compensate",
    [STR_RADIATION] = "Bremsstrahlung Emission",
    [STR_RELATIVISTIC] = "Relativistic Correction",
    [STR_FOLLOW] = "Lock On (F)",
    [STR_CAM_FREE] = "Free Scan (V)",
    [STR_READOUTS] = "Sensor Readout",
    [STR_SETTINGS] = "Deflector Control",
    [STR_DARK_MODE] = "Red Alert Lighting",
    [STR_PANEL] = "Console",
    [STR_LEFT] = "Port",
    [STR_RIGHT] = "Starboard",
    [STR_STEREO_3D] = "Holographic Projection",
    [STR_SEPARATION] = "Binocular Parallax",
    [STR_GAP] = "Inter-emitter Gap",
    [STR_ARROWHEAD] = "Vector Indicator",
    [STR_LINE_WIDTHS] = "Beam Widths",
    [STR_COLORS] = "Chromatic Assignments",
    [STR_PARTICLES] = "Tachyon Signatures",
    [STR_LW_TRAIL] = "Ion Trail",
    [STR_LW_ARROWS] = "Vectors",
    [STR_LW_AXES] = "Nav Axes",
    [STR_COLOR_FIELD_LINES] = "Flux Lines",
    [STR_COLOR_AXES] = "Nav Axes",
    [STR_LANGUAGE] = "Universal Translator",
    [STR_CANCEL] = "Abort",
    [STR_SHOW_UI] = "U: raise viewscreen",
    [STR_ARMED] = "ARRAY ARMED",
    [STR_TAP_CONTINUE] = "engage to continue",
    [STR_PLOT_PITCH_ANGLE] = "Deflector Angle",
    [STR_PLOT_MAG_MOMENT] = "Magnetic Flux Moment",
    [STR_PLOT_TIME] = "Stardate (s)",
    [STR_OPEN_RECORDING] = "Retrieve Log",
    [STR_NO_RECORDINGS] = "No sensor logs found in the databanks.",
    [STR_OPEN] = "Retrieve",
    [STR_PICKER_HELP] = "Click to select, double-click or Retrieve to replay  |  Esc to abort  |  Enter to retrieve",
    [STR_MODEL_CIRCULAR] = "Circular Plasma Conduit",
    [STR_MODEL_GRAD_B] = "Linear Flux Gradient",
    [STR_MODEL_QUAD_GRAD] = "Quadratic Flux Gradient",
    [STR_MODEL_SINUSOIDAL] = "Sinusoidal Flux Gradient",
    [STR_MODEL_NONPHYSICAL] = "Inverted Polarity (div B != 0)",
    [STR_MODEL_DIPOLE] = "Planetary Magnetic Dipole",
    [STR_MODEL_BOTTLE] = "Magnetic Confinement Vessel",
    [STR_PARAM_AMPLITUDE] = "Amplitude a",
    [STR_PARAM_Q_SAFETY] = "Safety interlock q",
    [STR_PARAM_CONFIG] = "EPS Configuration",
    [STR_TUT_PROMPT] = "Shall we run a Level 1 diagnostic of the controls?",
    [STR_TUT_PROMPT_SUB] = "A quick walkthrough of the bridge interface.",
    [STR_TUT_MODE_Q] = "What diagnostic level do you require?",
    [STR_TUT_BRIEF] = "Level 3 (brief)",
    [STR_TUT_DETAILED] = "Level 1 (comprehensive)",
    [STR_TUT_NO_THANKS] = "Belay that",
    [STR_TUT_YES] = "Engage",
    [STR_TUT_STOP] = "Stand down",
    [STR_TUT_CONTINUE] = "Continue",
    [STR_TUT_DONE_HINT] = "Diagnostic complete. Press Continue when ready.",
    [STR_TUT_S0_BRIEF] = "The simulation is at full stop. Press Spacebar to go to impulse or hold position.",
    [STR_TUT_S0_DETAIL] = "The simulation is at full stop. Press Spacebar to go to impulse or hold position. "
        "While holding, press N to advance one temporal frame, or P to run it back. "
        "Hold either key to advance at the current time compression ratio.",
    [STR_TUT_S1_BRIEF] = "Right-click and drag to reorient the viewscreen. Scroll to adjust magnification. "
        "The navigational axes show x (red), y (green), z (blue).",
    [STR_TUT_S1_DETAIL] = "Right-click and drag to orbit the viewscreen around the target coordinates. "
        "Scroll to increase or decrease magnification. Navigational axes at the origin show "
        "x (red), y (green), z (blue). Shift+right-drag orbits around the origin. "
        "Press 0 to reset to default scanning position. Vector overlays are suppressed "
        "so you can get a clear reading on the field geometry.",
    [STR_TUT_S2_BRIEF] = "The curves on the viewscreen are magnetic flux conduit lines. They show "
        "the direction and geometry of the plasma field.",
    [STR_TUT_S2_DETAIL] = "Flux conduit lines trace the direction of B throughout the sector. "
        "The tachyon pulse gyrates around and drifts along these conduits. "
        "In the Main Viewscreen section you can also enable the guiding-center "
        "flux line (GC fl) and the position flux line (FL pos).",
    [STR_TUT_S3_BRIEF] = "Press F to lock sensors on the tachyon pulse. "
        "Scroll to magnify the orbital pattern.",
    [STR_TUT_S3_DETAIL] = "Press F to engage target lock. The viewscreen tracks the "
        "tachyon pulse while you retain full navigational control. "
        "Once locked on, scroll to magnify the region near the pulse. "
        "The next steps will bring vector overlays online at the pulse position.",
    [STR_TUT_S4_BRIEF] = "The vectors at the tachyon pulse show velocity (v) and "
        "the magnetic flux direction (B).",
    [STR_TUT_S4_DETAIL] = "The velocity and B-field vectors are now online. "
        "The velocity vector (v) shows the instantaneous heading. "
        "The magnetic flux vector (B) shows the local field bearing. "
        "Both render as unit vectors by default; enable Magnitude Compensated mode "
        "on the Main Viewscreen to display their true magnitudes.",
    [STR_TUT_S5_BRIEF] = "The Lorentz force F = q(v \xc3\x97 B) is perpendicular to both "
        "v and B. Its unit vector is now displayed on the viewscreen.",
    [STR_TUT_S5_DETAIL] = "The Lorentz force F = q(v \xc3\x97 B) is always perpendicular "
        "to both the velocity and the magnetic flux. This is what "
        "curves the tachyon pulse into a helical trajectory. You can toggle "
        "the v, B, and F vectors independently on the Main Viewscreen.",
    [STR_TUT_S6_BRIEF] = "Press V to align the viewscreen along the magnetic flux vector. "
        "Arrow keys rotate the scanning orientation.",
    [STR_TUT_S6_DETAIL] = "Press V to cycle between free scan, alignment along +B, "
        "and alignment along -B. In flux-aligned views, the curvature vector (\xce\xba) "
        "and binormal appear as overlays. Arrow keys rotate which direction \xce\xba points on screen. "
        "This view reveals the cross-field drift pattern.",
    [STR_TUT_S7_BRIEF] = "The side console (U to toggle) has collapsible sections: "
        "Subspace Emitter, Temporal Log, Main Viewscreen, and Sensor Readout.",
    [STR_TUT_S7_DETAIL] = "Press U to raise or lower the side console. It has four "
        "collapsible sections. Subspace Emitter Config sets the plasma conduit geometry "
        "and tachyon species, energy output, and deflector angle. Temporal Log controls "
        "time compression and capture. Main Viewscreen toggles visual overlays. "
        "Sensor Readout shows live telemetry. The gear icon opens Deflector Control.",
    [STR_TUT_S8_BRIEF] = "Tachyon parameters like energy and deflector angle take effect "
        "on reinitialization (R) or when deploying a new pulse with Shift+click.",
    [STR_TUT_S8_DETAIL] = "In the Subspace Emitter section, you can reconfigure the species, "
        "energy (or impulse velocity for Reconfigured), and deflector angle. "
        "These are presets: they take effect the next time you press R to reinitialize "
        "the gel packs, or when you deploy a new tachyon pulse with Left Shift + click. "
        "Existing pulses retain their original configuration.",
    [STR_TUT_S9_BRIEF] = "Try selecting the Dipole plasma conduit from the dropdown. "
        "The viewscreen reinitializes automatically when you reroute the conduits.",
    [STR_TUT_S9_DETAIL] = "The Plasma Conduit Matrix dropdown lists 10 field geometries: "
        "circular, flux gradient, dipole, confinement vessel, tokamak, and more. "
        "Each configuration has tunable parameters. Selecting a new matrix "
        "reinitializes the tachyon pulse and viewscreen. Try selecting the Dipole "
        "to observe confined particle motion in a planetary magnetosphere.",
    [STR_TUT_S10_BRIEF] = "The sensor telemetry shows deflector angle and magnetic flux moment "
        "versus stardate.",
    [STR_TUT_S10_DETAIL] = "The ventral telemetry plots show deflector angle and magnetic flux moment "
        "(\xce\xbc) as functions of stardate. In a pure magnetic field, "
        "\xce\xbc is an adiabatic invariant and should hold steady. "
        "The range controls (x1/x10/x100) adjust the temporal scan window. "
        "Auto-Compensate adjusts the deflector angle axis automatically.",
    [STR_TUT_S11_BRIEF] = "Press 0 to reset the viewscreen. Then hold Left Shift "
        "and click to deploy additional tachyon pulses. Tab cycles target lock.",
    [STR_TUT_S11_DETAIL] = "Press 0 to reset the viewscreen to default scanning position. Then "
        "hold Left Shift and click in the sector to deploy a new tachyon pulse "
        "with the current species, energy, and deflector settings. Up to 16 pulses "
        "can be tracked simultaneously. Press Tab to cycle which pulse the sensors follow.",
    [STR_TUT_S12_BRIEF] = "Press R to reinitialize the gel packs and return to a single tachyon pulse.",
    [STR_TUT_S12_DETAIL] = "Press R to reinitialize. This purges all deployed pulses and returns to "
        "a single tachyon pulse with the current parameters. Any changes you made to "
        "species, energy, velocity, or deflector angle in the console are applied on reinitialization.",
    [STR_TUT_S13_BRIEF] = "Arm the sensor array in Temporal Log, then press R to start "
        "and stop holographic capture. Shift-P replays sensor logs.",
    [STR_TUT_S13_DETAIL] = "In the Temporal Log section, arm the sensor array. Then press R "
        "to reinitialize and begin recording an H.264 holographic capture. "
        "Press R again to stop. A sensor log is saved alongside the capture. "
        "Press Shift-P to retrieve and replay a previous log entry.",
    [STR_TUT_S14_BRIEF] = "That concludes the diagnostic. Press F1 at any time to access "
        "the technical reference database.",
    [STR_TUT_S14_DETAIL] = "That concludes the diagnostic. Press F1 at any time to access "
        "the technical reference database, which contains full documentation on "
        "bridge controls, physics subroutines, conduit models, and the interface. "
        "The Deflector Control panel (gear icon) has chromatic and beam width tuning. "
        "Make it so.",
    [STR_HELP_TITLE] = "Lorentz Tracer Technical Database",
    [STR_HELP_TAB_KEYS] = "Bridge Controls",
    [STR_HELP_TAB_PHYSICS] = "Subspace Physics",
    [STR_HELP_TAB_FIELDS] = "Conduit Models",
    [STR_HELP_TAB_INTERFACE] = "Console Layout",
    [STR_HELP_TAB_ABOUT] = "Starship Log",
    [STR_HELP_FOOTER] = "Esc or F1 to close  |  Arrow keys or Tab to switch sections  |  Scroll to navigate",
    [STR_HELP_RUN_TUTORIAL] = "Run Diagnostic",
    [STR_HELP_RUN_TUTORIAL_DESC] = "Walk through the bridge interface and sensor controls.",
    [STR_HELP_FONT_SIZE] = "DISPLAY RESOLUTION",
    [STR_HELP_FONT_SIZE_DESC] = "Cmd+Plus / Cmd+Minus adjusts display resolution for the console panel, "
        "reference overlay, and sensor telemetry independently depending on cursor position. "
        "Cmd+0 resets to standard. All display settings persist between duty shifts.",
    [STR_HELP_FACTORY_RESET] = "Purge and Restore Defaults",
    [STR_HELP_FACTORY_RESET_DESC] = "Wipes all stored configurations and restores factory settings from the isolinear backup.",
    [STR_LANG_CONTINUE] = "Engage",
};

/* ================================================================
 *  MORSE CODE
 * ================================================================ */

static const char *g_strings_morse[STR__COUNT] = {
    [STR_MODEL_PARTICLE] = "-- --- -.. . .-.. / .--. .- .-. - .. -.-. .-.. .",
    [STR_FIELD_MODEL] = "..-. .. . .-.. -.. / -- --- -.. . .-..",
    [STR_CONFIG] = "-.-. --- -. ..-. .. --. :",
    [STR_PARTICLE] = ".--. .- .-. - .. -.-. .-.. .",
    [STR_SPECIES_ELECTRON] = ". .-.. . -.-. - .-. --- -.",
    [STR_SPECIES_POSITRON] = ".--. --- ... .. - .-. --- -.",
    [STR_SPECIES_PROTON] = ".--. .-. --- - --- -.",
    [STR_SPECIES_ALPHA] = ".- .-.. .--. .... .-",
    [STR_SPECIES_CUSTOM] = "-.-. ..- ... - --- --",
    [STR_CHARGE] = "-.-. .... .- .-. --. .",
    [STR_MASS] = "-- .- ... ...",
    [STR_SPEED] = "... .--. . . -..",
    [STR_ENERGY] = ". -. . .-. --. -.--",
    [STR_PITCH] = ".--. .. - -.-. ....",
    [STR_PLAYBACK_RECORD] = ".--. .-.. .- -.-- / .-. . -.-.",
    [STR_TRIGGER_REC] = "- .-. .. --. / .-. . -.-. / (R)",
    [STR_TRIGGER_REC_ACTIVE] = ".-. . -.-. --- .-. -.. .. -. --. .-.-.-",
    [STR_PAUSE] = ".--. .- ..- ... .",
    [STR_RESUME] = ".-. . ... ..- -- .",
    [STR_RESET] = ".-. . ... . -",
    [STR_DISPLAY] = "-.. .. ... .--. .-.. .- -.--",
    [STR_TRAIL] = "- .-. .- .. .-..",
    [STR_FIELD_LINES] = "..-. .. . .-.. -.. / .-.. .. -. . ...",
    [STR_SCALED] = "... -.-. .- .-.. . -..",
    [STR_SCALE_BAR] = "... -.-. .- .-.. . / -... .- .-.",
    [STR_AXES] = ".- -..- . ...",
    [STR_PLOTS] = ".--. .-.. --- - ...",
    [STR_BOTTOM] = "-... --- - - --- --",
    [STR_TOP] = "- --- .--.",
    [STR_AUTOSCALE] = ".- ..- - ---",
    [STR_RADIATION] = ".-. .- -..",
    [STR_RELATIVISTIC] = ".-. . .-..",
    [STR_FOLLOW] = "..-. --- .-.. .-.. --- .-- / (F)",
    [STR_CAM_FREE] = "..-. .-. . . / (V)",
    [STR_READOUTS] = ".-. . .- -.. --- ..- - ...",
    [STR_SETTINGS] = "... . - - .. -. --. ...",
    [STR_DARK_MODE] = "-.. .- .-. -.- / -- --- -.. .",
    [STR_PANEL] = ".--. .- -. . .-..",
    [STR_LEFT] = ".-.. . ..-. -",
    [STR_RIGHT] = ".-. .. --. .... -",
    [STR_STEREO_3D] = "...-- -.. / ... - . .-. . ---",
    [STR_SEPARATION] = "... . .--.",
    [STR_GAP] = "--. .- .--.",
    [STR_ARROWHEAD] = ".- .-. .-. --- .--",
    [STR_LINE_WIDTHS] = ".-.. .. -. . / .--",
    [STR_COLORS] = "-.-. --- .-.. --- .-. ...",
    [STR_PARTICLES] = ".--. .- .-. - ... .-.-.-",
    [STR_LW_TRAIL] = "- .-. .- .. .-..",
    [STR_LW_ARROWS] = ".- .-. .-. --- .-- ...",
    [STR_LW_AXES] = ".- -..- . ...",
    [STR_COLOR_FIELD_LINES] = "..-. .-.. -.. / .-.. -. ...",
    [STR_COLOR_AXES] = ".- -..- . ...",
    [STR_LANGUAGE] = ".-.. .- -. --.",
    [STR_CANCEL] = "-.-. .- -. -.-. . .-..",
    [STR_SHOW_UI] = "U: ..- ..",
    [STR_ARMED] = ".- .-. -- . -..",
    [STR_TAP_CONTINUE] = "- .- .--.",
    [STR_PLOT_PITCH_ANGLE] = ".--. .. - -.-. ....",
    [STR_PLOT_MAG_MOMENT] = "-- ..- / -- --- --",
    [STR_PLOT_TIME] = "- .. -- .",
    [STR_OPEN_RECORDING] = "--- .--. . -.",
    [STR_NO_RECORDINGS] = "-. --- -. .",
    [STR_OPEN] = "--- .--. . -.",
    [STR_PICKER_HELP] = NULL,
    [STR_MODEL_CIRCULAR] = "-.-. .. .-. -.-.",
    [STR_MODEL_GRAD_B] = "--. .-. .- -.. / -...",
    [STR_MODEL_QUAD_GRAD] = "--.- ..- .- -..",
    [STR_MODEL_SINUSOIDAL] = "... .. -.",
    [STR_MODEL_NONPHYSICAL] = "-. --- -. .--. .... -.-- ...",
    [STR_MODEL_DIPOLE] = "-.. .. .--. --- .-.. .",
    [STR_MODEL_BOTTLE] = "-... --- - - .-.. .",
    [STR_PARAM_AMPLITUDE] = ".- -- .--.",
    [STR_PARAM_Q_SAFETY] = "--.- / ... .- ..-.",
    [STR_PARAM_CONFIG] = "-.-. --- -. ..-. .. --.",
    [STR_TUT_PROMPT] = NULL,
    [STR_TUT_PROMPT_SUB] = NULL,
    [STR_TUT_MODE_Q] = NULL,
    [STR_TUT_BRIEF] = "-... .-. .. . ..-.",
    [STR_TUT_DETAILED] = "-.. . - .- .. .-..",
    [STR_TUT_NO_THANKS] = "-. ---",
    [STR_TUT_YES] = "-.-- . ...",
    [STR_TUT_STOP] = "... - --- .--.",
    [STR_TUT_CONTINUE] = "--. ---",
    [STR_TUT_DONE_HINT] = NULL,
    [STR_TUT_S0_BRIEF] = NULL,
    [STR_TUT_S0_DETAIL] = NULL,
    [STR_TUT_S1_BRIEF] = NULL,
    [STR_TUT_S1_DETAIL] = NULL,
    [STR_TUT_S2_BRIEF] = NULL,
    [STR_TUT_S2_DETAIL] = NULL,
    [STR_TUT_S3_BRIEF] = NULL,
    [STR_TUT_S3_DETAIL] = NULL,
    [STR_TUT_S4_BRIEF] = NULL,
    [STR_TUT_S4_DETAIL] = NULL,
    [STR_TUT_S5_BRIEF] = NULL,
    [STR_TUT_S5_DETAIL] = NULL,
    [STR_TUT_S6_BRIEF] = NULL,
    [STR_TUT_S6_DETAIL] = NULL,
    [STR_TUT_S7_BRIEF] = NULL,
    [STR_TUT_S7_DETAIL] = NULL,
    [STR_TUT_S8_BRIEF] = NULL,
    [STR_TUT_S8_DETAIL] = NULL,
    [STR_TUT_S9_BRIEF] = NULL,
    [STR_TUT_S9_DETAIL] = NULL,
    [STR_TUT_S10_BRIEF] = NULL,
    [STR_TUT_S10_DETAIL] = NULL,
    [STR_TUT_S11_BRIEF] = NULL,
    [STR_TUT_S11_DETAIL] = NULL,
    [STR_TUT_S12_BRIEF] = NULL,
    [STR_TUT_S12_DETAIL] = NULL,
    [STR_TUT_S13_BRIEF] = NULL,
    [STR_TUT_S13_DETAIL] = NULL,
    [STR_TUT_S14_BRIEF] = NULL,
    [STR_TUT_S14_DETAIL] = NULL,
    [STR_HELP_TITLE] = ".... . .-.. .--.",
    [STR_HELP_TAB_KEYS] = "-.- . -.-- ...",
    [STR_HELP_TAB_PHYSICS] = ".--. .... -.-- ...",
    [STR_HELP_TAB_FIELDS] = "..-. .-.. -.. ...",
    [STR_HELP_TAB_INTERFACE] = "..- ..",
    [STR_HELP_TAB_ABOUT] = ".- -... --- ..- -",
    [STR_HELP_FOOTER] = NULL,
    [STR_HELP_RUN_TUTORIAL] = NULL,
    [STR_HELP_RUN_TUTORIAL_DESC] = NULL,
    [STR_HELP_FONT_SIZE] = "..-. --- -. -",
    [STR_HELP_FONT_SIZE_DESC] = NULL,
    [STR_HELP_FACTORY_RESET] = ".-. . ... . -",
    [STR_HELP_FACTORY_RESET_DESC] = NULL,
    [STR_LANG_CONTINUE] = "--. ---",
};

/* ================================================================
 *  PIRATE
 * ================================================================ */

static const char *g_strings_pirate[STR__COUNT] = {
    [STR_MODEL_PARTICLE] = "Vessel / Wee Speck",
    [STR_FIELD_MODEL] = "Magnetic Currents Model",
    [STR_CONFIG] = "Config:",
    [STR_PARTICLE] = "Wee Speck",
    [STR_SPECIES_ELECTRON] = "Electron",
    [STR_SPECIES_POSITRON] = "Positron",
    [STR_SPECIES_PROTON] = "Proton",
    [STR_SPECIES_ALPHA] = "Alpha",
    [STR_SPECIES_CUSTOM] = "Custom Rigging",
    [STR_CHARGE] = "Charge",
    [STR_MASS] = "Tonnage",
    [STR_SPEED] = "Knots",
    [STR_ENERGY] = "Grog Power",
    [STR_PITCH] = "Pitch",
    [STR_PLAYBACK_RECORD] = "Playback / Ship's Log",
    [STR_TRIGGER_REC] = "Trigger log entry (R)",
    [STR_TRIGGER_REC_ACTIVE] = "Scribblin' in the log...",
    [STR_PAUSE] = "Drop Anchor",
    [STR_RESUME] = "Hoist Sails",
    [STR_RESET] = "Scuttle",
    [STR_DISPLAY] = "Spyglass",
    [STR_TRAIL] = "Wake",
    [STR_FIELD_LINES] = "Magnetic currents",
    [STR_SCALED] = "Scaled",
    [STR_SCALE_BAR] = "Yardarm measure",
    [STR_AXES] = "Compass bearings",
    [STR_PLOTS] = "Charts",
    [STR_BOTTOM] = "Below deck",
    [STR_TOP] = "Crow's nest",
    [STR_AUTOSCALE] = "Autoscale",
    [STR_RADIATION] = "Radiation",
    [STR_RELATIVISTIC] = "Relativistic",
    [STR_FOLLOW] = "Pursue (F)",
    [STR_CAM_FREE] = "Free sail (V)",
    [STR_READOUTS] = "The Log",
    [STR_SETTINGS] = "Captain's Quarters",
    [STR_DARK_MODE] = "Night Watch",
    [STR_PANEL] = "Bulkhead",
    [STR_LEFT] = "Port",
    [STR_RIGHT] = "Starboard",
    [STR_STEREO_3D] = "Stereo 3D",
    [STR_SEPARATION] = "Separation",
    [STR_GAP] = "Gap",
    [STR_ARROWHEAD] = "Arrowhead",
    [STR_LINE_WIDTHS] = "Rope widths",
    [STR_COLORS] = "Flag colors",
    [STR_PARTICLES] = "Wee Specks",
    [STR_LW_TRAIL] = "Wake",
    [STR_LW_ARROWS] = "Arrows",
    [STR_LW_AXES] = "Bearings",
    [STR_COLOR_FIELD_LINES] = "Magnetic currents",
    [STR_COLOR_AXES] = "Bearings",
    [STR_LANGUAGE] = "Tongue",
    [STR_CANCEL] = "Belay that",
    [STR_SHOW_UI] = "U: show the helm",
    [STR_ARMED] = "LOADED",
    [STR_TAP_CONTINUE] = "tap to set sail",
    [STR_PLOT_PITCH_ANGLE] = "Pitch angle",
    [STR_PLOT_MAG_MOMENT] = "Magnetic moment",
    [STR_PLOT_TIME] = "Time (s)",
    [STR_OPEN_RECORDING] = "Open Ship's Log",
    [STR_NO_RECORDINGS] = "No logs found in the hold, arr!",
    [STR_OPEN] = "Open",
    [STR_PICKER_HELP] = "Click to pick, double-click or Open to play  |  Esc to flee  |  Enter to board",
    [STR_MODEL_CIRCULAR] = "Circular B (steady seas)",
    [STR_MODEL_GRAD_B] = "Linear grad-B",
    [STR_MODEL_QUAD_GRAD] = "Quadratic grad-B",
    [STR_MODEL_SINUSOIDAL] = "Sinusoidal grad-B",
    [STR_MODEL_NONPHYSICAL] = "Cursed field (div B != 0)",
    [STR_MODEL_DIPOLE] = "Magnetic dipole",
    [STR_MODEL_BOTTLE] = "Magnetic bottle",
    [STR_PARAM_AMPLITUDE] = "Amplitude a",
    [STR_PARAM_Q_SAFETY] = "Safety factor q",
    [STR_PARAM_CONFIG] = "Config",
    [STR_TUT_PROMPT] = "Ahoy! Fancy a tour of the ship, matey?",
    [STR_TUT_PROMPT_SUB] = "A quick sail through the helm and rigging.",
    [STR_TUT_MODE_Q] = "How deep d'ye want to dive?",
    [STR_TUT_BRIEF] = "Quick look from the crow's nest",
    [STR_TUT_DETAILED] = "Full tour of the vessel",
    [STR_TUT_NO_THANKS] = "Nay, shove off",
    [STR_TUT_YES] = "Aye!",
    [STR_TUT_STOP] = "Belay",
    [STR_TUT_CONTINUE] = "Onward",
    [STR_TUT_DONE_HINT] = "All hands accounted for! Press Onward when ye be ready.",
    [STR_TUT_S0_BRIEF] = "The ship be anchored. Press Spacebar to hoist sails or drop anchor.",
    [STR_TUT_S0_DETAIL] = "The ship be anchored. Press Spacebar to hoist sails or drop anchor. "
        "While anchored, press N to nudge forward one tick, or P to "
        "back the oars. Hold either key to sail on at the current speed.",
    [STR_TUT_S1_BRIEF] = "Right-click and drag to swing the spyglass about. Scroll to zoom. "
        "The colored bearings show x (red), y (green), z (blue).",
    [STR_TUT_S1_DETAIL] = "Right-click and drag to swing yer spyglass around the target. "
        "Scroll to zoom in close or pull back. The colored bearings at the "
        "origin show x (red), y (green), z (blue). Shift+right-drag "
        "swings around the origin. Press 0 to re-center the view. "
        "The vector arrows be stowed for now so ye can see the "
        "waters and currents clearly.",
    [STR_TUT_S2_BRIEF] = "The curves in the scene be magnetic currents. They show "
        "where the invisible winds of magnetism be blowin'.",
    [STR_TUT_S2_DETAIL] = "The magnetic currents trace the direction of B across the seven seas. "
        "The wee speck gyrates around and drifts along these currents. "
        "In the Spyglass section ye can also hoist the guiding-center "
        "current (GC fl) and the position current (FL pos).",
    [STR_TUT_S3_BRIEF] = "Press F to pursue the wee speck with yer spyglass. "
        "Scroll to zoom in close to the orbit.",
    [STR_TUT_S3_DETAIL] = "Press F to give chase. The spyglass tracks the wee speck "
        "while ye can still swing and zoom. Once in pursuit, "
        "scroll to get close. The next steps will hoist the vector "
        "arrows, so getting close makes 'em easier to spot.",
    [STR_TUT_S4_BRIEF] = "The arrows at the wee speck show the velocity (v) and the "
        "magnetic current direction (B). Arr!",
    [STR_TUT_S4_DETAIL] = "The velocity and B-field arrows be hoisted now. "
        "The velocity vector (v) shows which way the wee speck be sailing. "
        "The magnetic field vector (B) shows the local current direction. "
        "Both be drawn as unit vectors by default; enable Scaled mode "
        "in the Spyglass section to show their true strength.",
    [STR_TUT_S5_BRIEF] = "The Lorentz force F = q(v \xc3\x97 B) be perpendicular to both "
        "v and B. Its arrow be now showin'.",
    [STR_TUT_S5_DETAIL] = "The Lorentz force F = q(v \xc3\x97 B) always blows perpendicular "
        "to both the wind and the current. 'Tis what curves the wee speck's "
        "course into a helix. Ye can toggle v, B, and F arrows "
        "in the Spyglass section of the side bulkhead.",
    [STR_TUT_S6_BRIEF] = "Press V to look down the barrel of the magnetic current. "
        "Arrow keys spin the view.",
    [STR_TUT_S6_DETAIL] = "Press V to cycle between free sail, lookin' along +B, "
        "and lookin' along -B. In current-aligned views, the "
        "curvature (\xce\xba) and binormal be shown as arrows. "
        "Arrow keys spin which way \xce\xba points on deck. "
        "This view reveals the drift across the current.",
    [STR_TUT_S7_BRIEF] = "The side bulkhead (U to toggle) has compartments: "
        "Vessel/Wee Speck, Playback, Spyglass, and The Log.",
    [STR_TUT_S7_DETAIL] = "Press U to show or stow the side bulkhead. It has four "
        "compartments. Vessel/Wee Speck picks the current model "
        "and speck species, grog power, and pitch. "
        "Playback governs speed and the ship's log. Spyglass "
        "toggles what ye see. The Log shows live readings. "
        "The gear button opens the Captain's Quarters.",
    [STR_TUT_S8_BRIEF] = "Speck settings like grog power and pitch take effect "
        "on scuttle (R) or when placin' a new speck with Shift+click.",
    [STR_TUT_S8_DETAIL] = "In the Vessel/Wee Speck section, ye can change the species, "
        "grog power (or knots for Custom Rigging), and pitch. "
        "These be templates: they take effect next time ye press R to scuttle, "
        "or when ye place a new speck with Left Shift + click. "
        "Existing specks keep their original rigging.",
    [STR_TUT_S9_BRIEF] = "Try selectin' the Dipole model from the dropdown. "
        "The view scuttles itself when ye switch models.",
    [STR_TUT_S9_DETAIL] = "The Magnetic Currents Model dropdown lists 10 sea charts: "
        "circular, grad-B, dipole, bottle, tokamak, and more. "
        "Each chart has adjustable rigging. Selectin' a new chart "
        "scuttles the speck and spyglass to defaults. Try selectin' "
        "the Dipole chart to see a trapped speck bouncin' about.",
    [STR_TUT_S10_BRIEF] = "The charts below show pitch angle and magnetic moment "
        "versus time. Keep an eye on 'em!",
    [STR_TUT_S10_DETAIL] = "The bottom charts show pitch angle and magnetic moment "
        "(\xce\xbc) as functions of time. In pure magnetic waters, "
        "\xce\xbc be an adiabatic invariant and should hold steady. "
        "The range buttons (x1/x10/x100) set how much history ye see. "
        "Autoscale adjusts the pitch axis range on its own.",
    [STR_TUT_S11_BRIEF] = "Press 0 to reset the spyglass. Then hold Left Shift "
        "and click to drop more specks in the water. "
        "Tab cycles yer quarry.",
    [STR_TUT_S11_DETAIL] = "Press 0 to reset the spyglass to the default view. Then "
        "hold Left Shift and click in the 3D scene to drop a "
        "new speck with the current species, grog power, and pitch. "
        "Up to 16 specks can sail at once. "
        "Press Tab to cycle which speck yer spyglass follows.",
    [STR_TUT_S12_BRIEF] = "Press R to scuttle and start fresh with one speck.",
    [STR_TUT_S12_DETAIL] = "Press R to scuttle the simulation. This clears all specks "
        "and returns to a single speck with the current settings. "
        "Any changes ye made to species, grog power, knots, "
        "or pitch in the bulkhead take effect on scuttle.",
    [STR_TUT_S13_BRIEF] = "Enable Trigger log entry in Playback, then press R to start "
        "and stop recordin'. Shift-P replays old logs.",
    [STR_TUT_S13_DETAIL] = "In the Playback section, enable Trigger log entry. Then press R "
        "to scuttle and begin recordin' an H.264 video. "
        "Press R again to stop. A ship's log is saved alongside the video. "
        "Press Shift-P to open and replay a previous log, "
        "reproducin' the exact voyage.",
    [STR_TUT_S14_BRIEF] = "That be all ye need to know, matey! Press F1 anytime to open "
        "the captain's manual. Fair winds!",
    [STR_TUT_S14_DETAIL] = "That be all ye need to know! Press F1 anytime to open "
        "the captain's manual, which has charts on keyboard shortcuts, "
        "physics, magnetic current models, and the helm. "
        "The Captain's Quarters (gear icon) lets ye customize "
        "flag colors and rope widths. Now go plunder the seven seas! Arr!",
    [STR_HELP_TITLE] = "Lorentz Tracer Captain's Manual",
    [STR_HELP_TAB_KEYS] = "Controls",
    [STR_HELP_TAB_PHYSICS] = "Physics",
    [STR_HELP_TAB_FIELDS] = "Currents",
    [STR_HELP_TAB_INTERFACE] = "The Helm",
    [STR_HELP_TAB_ABOUT] = "About",
    [STR_HELP_FOOTER] = "Esc or F1 to batten the hatches  |  Arrows or Tab to navigate  |  Scroll to sail through",
    [STR_HELP_RUN_TUTORIAL] = "Run the Tour",
    [STR_HELP_RUN_TUTORIAL_DESC] = "A guided tour of the ship from bow to stern.",
    [STR_HELP_FONT_SIZE] = "LETTER SIZE",
    [STR_HELP_FONT_SIZE_DESC] = "Cmd+Plus / Cmd+Minus adjusts the letter size for the bulkhead, "
        "captain's manual, and charts independently depending on "
        "where yer cursor be parked. Cmd+0 resets to default. "
        "All sizes be saved between voyages.",
    [STR_HELP_FACTORY_RESET] = "Scuttle All Settings",
    [STR_HELP_FACTORY_RESET_DESC] = "Throws all yer saved settings overboard and restores the factory rigging.",
    [STR_LANG_CONTINUE] = "Onward",
};

/* ================================================================
 *  LEET SPEAK (1337)
 * ================================================================ */

static const char *g_strings_leet[STR__COUNT] = {
    [STR_MODEL_PARTICLE] = "M0d3l / P4r71cl3",
    [STR_FIELD_MODEL] = "F13ld M0d3l",
    [STR_CONFIG] = "C0nf19:",
    [STR_PARTICLE] = "P4r71cl3",
    [STR_SPECIES_ELECTRON] = "3l3c7r0n",
    [STR_SPECIES_POSITRON] = "P05i7r0n",
    [STR_SPECIES_PROTON] = "Pr070n",
    [STR_SPECIES_ALPHA] = "4lph4",
    [STR_SPECIES_CUSTOM] = "Cu570m",
    [STR_CHARGE] = "Ch4r93",
    [STR_MASS] = "M455",
    [STR_SPEED] = "5p33d",
    [STR_ENERGY] = "3n3r9y",
    [STR_PITCH] = "P17ch",
    [STR_PLAYBACK_RECORD] = "Pl4y84ck / R3c0rd",
    [STR_TRIGGER_REC] = "7r199er r3c (R)",
    [STR_TRIGGER_REC_ACTIVE] = "7r199er r3c (r3c0rd1n9...)",
    [STR_PAUSE] = "P4u53",
    [STR_RESUME] = "R35um3",
    [STR_RESET] = "R3537",
    [STR_DISPLAY] = "D15pl4y",
    [STR_TRAIL] = "7r41l",
    [STR_FIELD_LINES] = "F13ld l1n35",
    [STR_SCALED] = "5c4l3d",
    [STR_SCALE_BAR] = "5c4l3 84r",
    [STR_AXES] = "4x35",
    [STR_PLOTS] = "Pl075",
    [STR_BOTTOM] = "80770m",
    [STR_TOP] = "70p",
    [STR_AUTOSCALE] = "4u705c4l3",
    [STR_RADIATION] = "R4d1471on",
    [STR_RELATIVISTIC] = "R3l47iv15t1c",
    [STR_FOLLOW] = "F0ll0w (F)",
    [STR_CAM_FREE] = "Fr33 (V)",
    [STR_READOUTS] = "R34d0u75",
    [STR_SETTINGS] = "5377in95",
    [STR_DARK_MODE] = "D4rk m0d3",
    [STR_PANEL] = "P4n3l",
    [STR_LEFT] = "L3f7",
    [STR_RIGHT] = "R19h7",
    [STR_STEREO_3D] = "57er30 3D",
    [STR_SEPARATION] = "53p4r4710n",
    [STR_GAP] = "94p",
    [STR_ARROWHEAD] = "4rr0wh34d",
    [STR_LINE_WIDTHS] = "L1n3 w1d7h5",
    [STR_COLORS] = "C0l0r5",
    [STR_PARTICLES] = "P4r71cl35",
    [STR_LW_TRAIL] = "7r41l",
    [STR_LW_ARROWS] = "4rr0w5",
    [STR_LW_AXES] = "4x35",
    [STR_COLOR_FIELD_LINES] = "F13ld l1n35",
    [STR_COLOR_AXES] = "4x35",
    [STR_LANGUAGE] = "L4n9u493",
    [STR_CANCEL] = "C4nc3l",
    [STR_SHOW_UI] = "U: 5h0w U1",
    [STR_ARMED] = "4RM3D",
    [STR_TAP_CONTINUE] = "74p 70 c0n71nu3",
    [STR_PLOT_PITCH_ANGLE] = "P17ch 4n9l3",
    [STR_PLOT_MAG_MOMENT] = "M49n371c m0m3n7",
    [STR_PLOT_TIME] = "71m3 (5)",
    [STR_OPEN_RECORDING] = "0p3n R3c0rd1n9",
    [STR_NO_RECORDINGS] = "N0 r3c0rd1n95 f0und.",
    [STR_OPEN] = "0p3n",
    [STR_PICKER_HELP] = "Cl1ck 70 53l3c7, d0u8l3-cl1ck 0r 0p3n 70 pl4y  |  Esc 70 c4nc3l  |  3n73r 70 0p3n",
    [STR_MODEL_CIRCULAR] = "C1rcul4r 8 (c0n57 |8|)",
    [STR_MODEL_GRAD_B] = "L1n34r 9r4d-8",
    [STR_MODEL_QUAD_GRAD] = "Qu4dr471c 9r4d-8",
    [STR_MODEL_SINUSOIDAL] = "51nu501d4l 9r4d-8",
    [STR_MODEL_NONPHYSICAL] = "N0nphy51c4l (d1v 8 != 0)",
    [STR_MODEL_DIPOLE] = "M49n371c d1p0l3",
    [STR_MODEL_BOTTLE] = "M49n371c 80773",
    [STR_PARAM_AMPLITUDE] = "4mpl17ud3 4",
    [STR_PARAM_Q_SAFETY] = "54f37y f4c70r q",
    [STR_PARAM_CONFIG] = "C0nf19",
    [STR_TUT_PROMPT] = "W0uld y0u l1k3 70 533 7h3 7u70r14l?",
    [STR_TUT_PROMPT_SUB] = "4 5h0r7 w4lk7hr0u9h 0f 7h3 1n73rf4c3 4nd c0n7r0l5.",
    [STR_TUT_MODE_Q] = "H0w much d3741l w0uld y0u l1k3?",
    [STR_TUT_BRIEF] = "8r13f 0v3rv13w",
    [STR_TUT_DETAILED] = "D3741l3d w4lk7hr0u9h",
    [STR_TUT_NO_THANKS] = "N0 7h4nk5",
    [STR_TUT_YES] = "Y35",
    [STR_TUT_STOP] = "570p",
    [STR_TUT_CONTINUE] = "C0n71nu3",
    [STR_TUT_DONE_HINT] = "D0n3! Pr355 C0n71nu3 wh3n r34dy.",
    [STR_TUT_S0_BRIEF] = "7h3 51mul4710n 15 p4u53d. Pr355 5p4c384r 70 pl4y 0r p4u53.",
    [STR_TUT_S0_DETAIL] = "7h3 51mul4710n 15 p4u53d. Pr355 5p4c384r 70 pl4y 0r p4u53. "
        "Wh1l3 p4u53d, pr355 N 70 573p f0rw4rd 0n3 71m3573p, 0r P 70 "
        "573p 84ckw4rd. H0ld 317h3r k3y 70 4dv4nc3 c0n71nu0u5ly 47 "
        "7h3 curr3n7 pl4y84ck 5p33d.",
    [STR_TUT_S1_BRIEF] = "R19h7-cl1ck 4nd dr49 70 0r817 7h3 c4m3r4. 5cr0ll 70 z00m. "
        "7h3 c0l0r3d 4x35 5h0w x (r3d), y (9r33n), z (8lu3).",
    [STR_TUT_S1_DETAIL] = "R19h7-cl1ck 4nd dr49 70 0r817 7h3 c4m3r4 4r0und 7h3 74r937 "
        "p01n7. 5cr0ll 70 z00m 1n 4nd 0u7. 7h3 c0l0r3d 4x35 47 7h3 "
        "0r191n 5h0w x (r3d), y (9r33n), z (8lu3). 5h1f7+r19h7-dr49 "
        "0r8175 4r0und 7h3 0r191n. Pr355 0 70 r3-c3n73r 7h3 v13w. "
        "7h3 v3c70r 4rr0w5 4r3 h1dd3n f0r n0w.",
    [STR_TUT_S2_BRIEF] = "7h3 curv35 1n 7h3 5c3n3 4r3 m49n371c f13ld l1n35. 7h3y 5h0w "
        "7h3 d1r3c710n 4nd 930m37ry 0f 7h3 m49n371c f13ld.",
    [STR_TUT_S2_DETAIL] = "F13ld l1n35 7r4c3 7h3 d1r3c710n 0f 8 7hr0u9h0u7 7h3 d0m41n. "
        "7h3 p4r71cl3 9yr4735 4r0und 4nd dr1f75 4l0n9 7h353 l1n35. "
        "1n 7h3 D15pl4y 53c710n y0u c4n 4l50 3n48l3 7h3 9u1d1n9-c3n73r "
        "f13ld l1n3 (9C fl) 4nd 7h3 p05171on f13ld l1n3 (FL p05).",
    [STR_TUT_S3_BRIEF] = "Pr355 F 70 f0ll0w 7h3 p4r71cl3 w17h 7h3 c4m3r4. "
        "5cr0ll 70 z00m 1n cl053 70 7h3 0r817.",
    [STR_TUT_S3_DETAIL] = "Pr355 F 70 70991e f0ll0w m0d3. 7h3 c4m3r4 7r4ck5 7h3 "
        "p4r71cl3'5 p05i710n wh1l3 y0u c4n 571ll 0r817 4nd z00m. "
        "0nc3 f0ll0w1n9, 5cr0ll 70 z00m 1n cl053 70 7h3 p4r71cl3.",
    [STR_TUT_S4_BRIEF] = "7h3 4rr0w5 47 7h3 p4r71cl3 5h0w 7h3 v3l0c17y (v) 4nd 7h3 "
        "m49n371c f13ld d1r3c710n (8).",
    [STR_TUT_S4_DETAIL] = "7h3 v3l0c17y 4nd 8-f13ld 4rr0w5 4r3 n0w 7urn3d 0n. "
        "7h3 v3l0c17y v3c70r (v) 5h0w5 7h3 p4r71cl3'5 1n574n74n30u5 "
        "d1r3c710n 0f m0710n. 7h3 m49n371c f13ld v3c70r (8) 5h0w5 "
        "7h3 l0c4l f13ld d1r3c710n.",
    [STR_TUT_S5_BRIEF] = "7h3 L0r3n7z f0rc3 F = q(v \xc3\x97 8) 15 p3rp3nd1cul4r 70 807h "
        "v 4nd 8. 175 un17 v3c70r 15 n0w 5h0wn.",
    [STR_TUT_S5_DETAIL] = "7h3 L0r3n7z f0rc3 F = q(v \xc3\x97 8) 15 4lw4y5 p3rp3nd1cul4r "
        "70 807h 7h3 v3l0c17y 4nd 7h3 m49n371c f13ld. 7h15 15 wh47 "
        "curv35 7h3 p4r71cl3'5 p47h 1n70 4 h3l1x.",
    [STR_TUT_S6_BRIEF] = "Pr355 V 70 l00k 4l0n9 7h3 m49n371c f13ld d1r3c710n. "
        "4rr0w k3y5 r07473 7h3 v13w 0r13n74710n.",
    [STR_TUT_S6_DETAIL] = "Pr355 V 70 cycl3 837w33n fr33 c4m3r4, l00k1n9 4l0n9 +8, "
        "4nd l00k1n9 4l0n9 -8. 1n f13ld-4l19n3d v13w5, 7h3 "
        "curv47ur3 v3c70r 4nd 81n0rm4l 4r3 5h0wn 45 4rr0w5.",
    [STR_TUT_S7_BRIEF] = "7h3 51d3 p4n3l (U 70 70991e) h45 c0ll4p5i8l3 53c710n5: "
        "M0d3l/P4r71cl3, Pl4y84ck, D15pl4y, 4nd R34d0u75.",
    [STR_TUT_S7_DETAIL] = "Pr355 U 70 5h0w 0r h1d3 7h3 51d3 p4n3l. 17 h45 f0ur "
        "c0ll4p5i8l3 53c710n5. M0d3l/P4r71cl3 53l3c75 7h3 m49n371c "
        "f13ld 930m37ry 4nd p4r71cl3 5p3c135, 3n3r9y, 4nd p17ch 4n9l3.",
    [STR_TUT_S8_BRIEF] = "P4r71cl3 5377in95 l1k3 3n3r9y 4nd p17ch 4n9l3 74k3 3ff3c7 "
        "0n r3537 (R) 0r wh3n pl4c1n9 4 n3w p4r71cl3 w17h 5h1f7+cl1ck.",
    [STR_TUT_S8_DETAIL] = "1n 7h3 M0d3l/P4r71cl3 53c710n, y0u c4n ch4n93 7h3 5p3c135, "
        "3n3r9y, 4nd p17ch 4n9l3. 7h353 4r3 73mpl4735: 7h3y 74k3 3ff3c7 "
        "7h3 n3x7 71m3 y0u pr355 R 70 r3537, 0r wh3n y0u pl4c3 4 "
        "n3w p4r71cl3 w17h L3f7 5h1f7 + cl1ck.",
    [STR_TUT_S9_BRIEF] = "7ry 53l3c71n9 7h3 D1p0l3 f13ld m0d3l fr0m 7h3 dr0pd0wn. "
        "7h3 v13w r35375 4u70m471c4lly wh3n 5w17ch1n9 m0d3l5.",
    [STR_TUT_S9_DETAIL] = "7h3 F13ld M0d3l dr0pd0wn l1575 10 m49n371c f13ld 930m37r135. "
        "34ch m0d3l h45 4dju5748l3 p4r4m3737r5. 53l3c71n9 4 n3w m0d3l "
        "r35375 7h3 p4r71cl3 4nd c4m3r4 70 d3f4ul75.",
    [STR_TUT_S10_BRIEF] = "7h3 73l3m37ry pl075 5h0w p17ch 4n9l3 4nd m49n371c m0m3n7 "
        "v3r5u5 71m3.",
    [STR_TUT_S10_DETAIL] = "7h3 80770m pl075 5h0w p17ch 4n9l3 4nd m49n371c m0m3n7 "
        "45 func710n5 0f 71m3. 1n 4 pur3 m49n371c f13ld, "
        "\xce\xbc 15 4n 4d1484t1c 1nv4r14n7 4nd 5h0uld 574y n34rly c0n574n7.",
    [STR_TUT_S11_BRIEF] = "Pr355 0 70 r3537 7h3 c4m3r4 v13w. 7h3n h0ld L3f7 5h1f7 "
        "4nd cl1ck 70 pl4c3 4dd1710n4l p4r71cl35. "
        "748 cycl35 53l3c710n.",
    [STR_TUT_S11_DETAIL] = "Pr355 0 70 r3537 7h3 c4m3r4 70 7h3 d3f4ul7 v13w. 7h3n "
        "h0ld L3f7 5h1f7 4nd cl1ck 1n 7h3 3D 5c3n3 70 pl4c3 4 "
        "n3w p4r71cl3. Up 70 16 p4r71cl35 c4n 83 7r4c3d. "
        "Pr355 748 70 cycl3 wh1ch p4r71cl3 7h3 c4m3r4 f0ll0w5.",
    [STR_TUT_S12_BRIEF] = "Pr355 R 70 r3537 7h3 51mul4710n 84ck 70 4 51n9l3 p4r71cl3.",
    [STR_TUT_S12_DETAIL] = "Pr355 R 70 r3537 7h3 51mul4710n. 7h15 cl34r5 4ll pl4c3d "
        "p4r71cl35 4nd r37urn5 70 4 51n9l3 p4r71cl3 w17h 7h3 curr3n7 "
        "5377in95.",
    [STR_TUT_S13_BRIEF] = "3n48l3 7r199er r3c 1n Pl4y84ck, 7h3n pr355 R 70 574r7 "
        "4nd 570p v1d30 r3c0rd1n9. 5h1f7-P r3pl4y5 3v3n7 l095.",
    [STR_TUT_S13_DETAIL] = "1n 7h3 Pl4y84ck 53c710n, 3n48l3 7r199er r3c. 7h3n pr355 R "
        "70 r3537 7h3 51mul4710n 4nd 8391n r3c0rd1n9 4n H.264 v1d30. "
        "Pr355 R 4941n 70 570p. 4n 3v3n7 l09 15 54v3d 4l0n951d3 7h3 "
        "v1d30. Pr355 5h1f7-P 70 0p3n 4nd r3pl4y 4 pr3v10u5 r3c0rd1n9.",
    [STR_TUT_S14_BRIEF] = "7h47 c0v3r5 7h3 355en714l5. Pr355 F1 4ny71m3 70 0p3n "
        "7h3 h3lp r3f3r3nc3.",
    [STR_TUT_S14_DETAIL] = "7h47 c0v3r5 7h3 355en714l5. Pr355 F1 4ny71m3 70 0p3n "
        "7h3 h3lp r3f3r3nc3, wh1ch h45 d3741l3d d0cum3n74710n 0n "
        "k3y804rd 5h0r7cu75, phy51c5, f13ld m0d3l5, 4nd 7h3 1n73rf4c3. "
        "7h3 5377in95 p4n3l (934r 1c0n) h45 c0l0r 4nd l1n3 w1d7h "
        "cu570m1z4710n. 3nj0y 3xpl0r1n9.",
    [STR_HELP_TITLE] = "L0r3n7z 7r4c3r H3lp",
    [STR_HELP_TAB_KEYS] = "K3y5",
    [STR_HELP_TAB_PHYSICS] = "Phy51c5",
    [STR_HELP_TAB_FIELDS] = "F13ld5",
    [STR_HELP_TAB_INTERFACE] = "1n73rf4c3",
    [STR_HELP_TAB_ABOUT] = "480u7",
    [STR_HELP_FOOTER] = "Esc 0r F1 70 cl053  |  4rr0w k3y5 0r 748 70 5w17ch 7485  |  5cr0ll 70 n4v19473",
    [STR_HELP_RUN_TUTORIAL] = "Run 7u70r14l",
    [STR_HELP_RUN_TUTORIAL_DESC] = "W4lk 7hr0u9h 7h3 1n73rf4c3 4nd c0n7r0l5 573p 8y 573p.",
    [STR_HELP_FONT_SIZE] = "F0N7 51Z3",
    [STR_HELP_FONT_SIZE_DESC] = "Cmd+Plu5 / Cmd+M1nu5 4dju575 7h3 f0n7 51z3 f0r 7h3 U1 p4n3l, "
        "h3lp 0v3rl4y, 4nd 73l3m37ry pl075 1nd3p3nd3n7ly d3p3nd1n9 0n "
        "wh1ch 4r34 7h3 m0u53 15 0v3r. Cmd+0 r35375 70 d3f4ul7. "
        "4ll f0n7 51z35 4r3 54v3d 837w33n 53551on5.",
    [STR_HELP_FACTORY_RESET] = "F4c70ry R3537",
    [STR_HELP_FACTORY_RESET_DESC] = "R3m0v35 54v3d 5377in95 4nd r35t0r35 4ll d3f4ul75.",
    [STR_LANG_CONTINUE] = "C0n71nu3",
};

/* ================================================================
 *  SHAKESPEAREAN (Early Modern English)
 * ================================================================ */

static const char *g_strings_shk[STR__COUNT] = {
    [STR_MODEL_PARTICLE] = "Model / Mote",
    [STR_FIELD_MODEL] = "Field of Ethereal Force",
    [STR_CONFIG] = "Configuration:",
    [STR_PARTICLE] = "Mote",
    [STR_SPECIES_ELECTRON] = "Electron",
    [STR_SPECIES_POSITRON] = "Positron",
    [STR_SPECIES_PROTON] = "Proton",
    [STR_SPECIES_ALPHA] = "Alpha",
    [STR_SPECIES_CUSTOM] = "Bespoke",
    [STR_CHARGE] = "Charge",
    [STR_MASS] = "Heft",
    [STR_SPEED] = "Swiftness",
    [STR_ENERGY] = "Vigour",
    [STR_PITCH] = "Angle of Inclination",
    [STR_PLAYBACK_RECORD] = "Playback / Chronicle",
    [STR_TRIGGER_REC] = "Trigger chronicle (R)",
    [STR_TRIGGER_REC_ACTIVE] = "Chronicling in progress...",
    [STR_PAUSE] = "Stay",
    [STR_RESUME] = "Proceed",
    [STR_RESET] = "Begin anew",
    [STR_DISPLAY] = "Spectacle",
    [STR_TRAIL] = "Trail of passage",
    [STR_FIELD_LINES] = "Lines of ethereal force",
    [STR_SCALED] = "Proportioned",
    [STR_SCALE_BAR] = "Measure bar",
    [STR_AXES] = "Axes",
    [STR_PLOTS] = "Charts",
    [STR_BOTTOM] = "Below",
    [STR_TOP] = "Above",
    [STR_AUTOSCALE] = "Self-proportioning",
    [STR_RADIATION] = "Radiance",
    [STR_RELATIVISTIC] = "Relativistic",
    [STR_FOLLOW] = "Pursue (F)",
    [STR_CAM_FREE] = "Unfettered (V)",
    [STR_READOUTS] = "Observations",
    [STR_SETTINGS] = "Appointments",
    [STR_DARK_MODE] = "Dark mode",
    [STR_PANEL] = "Panel",
    [STR_LEFT] = "Sinister",
    [STR_RIGHT] = "Dexter",
    [STR_STEREO_3D] = "Stereo 3D",
    [STR_SEPARATION] = "Separation",
    [STR_GAP] = "Gap",
    [STR_ARROWHEAD] = "Arrowhead",
    [STR_LINE_WIDTHS] = "Stroke widths",
    [STR_COLORS] = "Colours",
    [STR_PARTICLES] = "Motes",
    [STR_LW_TRAIL] = "Trail",
    [STR_LW_ARROWS] = "Arrows",
    [STR_LW_AXES] = "Axes",
    [STR_COLOR_FIELD_LINES] = "Lines of ethereal force",
    [STR_COLOR_AXES] = "Axes",
    [STR_LANGUAGE] = "Tongue",
    [STR_CANCEL] = "Forsake",
    [STR_SHOW_UI] = "U: reveal the stage",
    [STR_ARMED] = "AT THE READY",
    [STR_TAP_CONTINUE] = "touch to proceed",
    [STR_PLOT_PITCH_ANGLE] = "Angle of inclination",
    [STR_PLOT_MAG_MOMENT] = "Magnetic virtue",
    [STR_PLOT_TIME] = "Time (s)",
    [STR_OPEN_RECORDING] = "Open Chronicle",
    [STR_NO_RECORDINGS] = "No chronicles were found herein.",
    [STR_OPEN] = "Open",
    [STR_PICKER_HELP] = "Click to select, double-click or Open to play  |  Esc to withdraw  |  Enter to open",
    [STR_MODEL_CIRCULAR] = "Circular B (constant magnitude)",
    [STR_MODEL_GRAD_B] = "Linear grad-B",
    [STR_MODEL_QUAD_GRAD] = "Quadratic grad-B",
    [STR_MODEL_SINUSOIDAL] = "Sinusoidal grad-B",
    [STR_MODEL_NONPHYSICAL] = "Unnatural (div B != 0)",
    [STR_MODEL_DIPOLE] = "Magnetic dipole",
    [STR_MODEL_BOTTLE] = "Magnetic bottle",
    [STR_PARAM_AMPLITUDE] = "Amplitude a",
    [STR_PARAM_Q_SAFETY] = "Safety factor q",
    [STR_PARAM_CONFIG] = "Configuration",
    [STR_TUT_PROMPT] = "Prithee, wouldst thou see the tutorial?",
    [STR_TUT_PROMPT_SUB] = "A brief tour of the stage and its workings.",
    [STR_TUT_MODE_Q] = "How much detail dost thou desire?",
    [STR_TUT_BRIEF] = "Brief overview",
    [STR_TUT_DETAILED] = "Detailed walkthrough",
    [STR_TUT_NO_THANKS] = "Nay, I thank thee",
    [STR_TUT_YES] = "Aye",
    [STR_TUT_STOP] = "Hold",
    [STR_TUT_CONTINUE] = "Onward",
    [STR_TUT_DONE_HINT] = "The lesson is complete! Press Onward when thou art ready.",
    [STR_TUT_S0_BRIEF] = "The proceedings are stayed. Prithee, press the space bar to commence or halt them.",
    [STR_TUT_S0_DETAIL] = "The proceedings are stayed. Prithee, press the space bar to commence or halt them. "
        "Whilst stayed, press N to advance one step, or P to retreat. "
        "Hold either key to advance continuously at the current pace.",
    [STR_TUT_S1_BRIEF] = "Right-click and drag to orbit the vantage. Scroll to draw nearer or farther. "
        "The coloured axes show x (red), y (green), z (blue).",
    [STR_TUT_S1_DETAIL] = "Right-click and drag to orbit the vantage about the target point. "
        "Scroll to draw nearer or farther. The coloured axes at the origin show "
        "x (red), y (green), z (blue). Shift+right-drag orbits about the origin. "
        "Press 0 to restore the vantage. The vector arrows are hidden for now, "
        "that thou mayest see the orbit and field geometry clearly.",
    [STR_TUT_S2_BRIEF] = "The curves upon the stage are lines of ethereal force. They show "
        "the direction and geometry of the magnetic field.",
    [STR_TUT_S2_DETAIL] = "Lines of ethereal force trace the direction of B throughout the domain. "
        "The mote gyrates about and drifts along these lines. "
        "In the Spectacle section thou canst also summon the guiding-center "
        "field line (GC fl) and the position field line (FL pos).",
    [STR_TUT_S3_BRIEF] = "Press F to pursue the mote with thy vantage. "
        "Scroll to draw near unto the orbit.",
    [STR_TUT_S3_DETAIL] = "Press F to engage pursuit. The vantage doth track "
        "the mote's position whilst thou mayest still orbit and magnify. "
        "Once in pursuit, scroll to draw near. The next steps shall "
        "summon vector arrows, so proximity makes them easier to discern.",
    [STR_TUT_S4_BRIEF] = "The arrows at the mote show the velocity (v) and the "
        "direction of ethereal force (B).",
    [STR_TUT_S4_DETAIL] = "The velocity and B-field arrows are now summoned. "
        "The velocity vector (v) shows the mote's instantaneous direction of motion. "
        "The field vector (B) shows the local direction of ethereal force. "
        "Both are drawn as unit vectors by default; enable Proportioned mode "
        "in the Spectacle section to show their true magnitudes.",
    [STR_TUT_S5_BRIEF] = "The Lorentz force F = q(v \xc3\x97 B) is perpendicular to both "
        "v and B. Its arrow is now displayed.",
    [STR_TUT_S5_DETAIL] = "The Lorentz force F = q(v \xc3\x97 B) is ever perpendicular "
        "to both the velocity and the field. 'Tis this force that curves "
        "the mote's path into a helix. Thou mayest toggle the v, B, and "
        "F vectors in the Spectacle section of the side panel.",
    [STR_TUT_S6_BRIEF] = "Press V to gaze along the direction of ethereal force. "
        "Arrow keys rotate the view orientation.",
    [STR_TUT_S6_DETAIL] = "Press V to cycle betwixt unfettered vantage, gazing along +B, "
        "and gazing along -B. In field-aligned views, the curvature "
        "vector (\xce\xba) and binormal appear as arrows. Arrow keys "
        "rotate which direction \xce\xba doth point upon the screen. "
        "This view reveals the drift motion perpendicular to B.",
    [STR_TUT_S7_BRIEF] = "The side panel (U to toggle) hath collapsible sections: "
        "Model/Mote, Playback, Spectacle, and Observations.",
    [STR_TUT_S7_DETAIL] = "Press U to show or conceal the side panel. It hath four "
        "collapsible sections. Model/Mote selects the field geometry "
        "and mote species, vigour, and angle of inclination. "
        "Playback governs the pace and chronicle. Spectacle "
        "toggles what is shown. Observations display live diagnostics. "
        "The gear button opens the Appointments for colours and stroke widths.",
    [STR_TUT_S8_BRIEF] = "Mote appointments such as vigour and angle of inclination take effect "
        "upon beginning anew (R) or when placing a new mote with Shift+click.",
    [STR_TUT_S8_DETAIL] = "In the Model/Mote section, thou canst alter the species, "
        "vigour (or swiftness for Bespoke), and angle of inclination. "
        "These are templates: they take effect the next time thou dost "
        "press R to begin anew, or when thou dost place a new mote with "
        "Left Shift + click. Existing motes retain their original appointments.",
    [STR_TUT_S9_BRIEF] = "Try selecting the Dipole field model from the dropdown. "
        "The view doth reset itself upon switching models.",
    [STR_TUT_S9_DETAIL] = "The Field dropdown presents 10 geometries of ethereal force: "
        "circular, grad-B, dipole, bottle, tokamak, and more. "
        "Each model hath adjustable parameters. Selecting a new model "
        "resets the mote and vantage to defaults. Try selecting "
        "the Dipole model to witness a trapped mote.",
    [STR_TUT_S10_BRIEF] = "The charts show the angle of inclination and magnetic virtue "
        "as functions of time.",
    [STR_TUT_S10_DETAIL] = "The lower charts show the angle of inclination and magnetic virtue "
        "(\xce\xbc) as functions of time. In a pure field of ethereal force, "
        "\xce\xbc is an adiabatic invariant and should remain nearly constant. "
        "The range buttons (x1/x10/x100) govern how much history is shown. "
        "Self-proportioning adjusts the inclination axis range.",
    [STR_TUT_S11_BRIEF] = "Press 0 to restore the vantage. Then hold Left Shift "
        "and click upon the stage to place additional motes. "
        "Tab cycles the selection.",
    [STR_TUT_S11_DETAIL] = "Press 0 to restore the default vantage. Then hold Left Shift "
        "and click in the 3D scene to place a new mote with the current "
        "species, vigour, and inclination. Up to 16 motes may be traced. "
        "Press Tab to cycle which mote the vantage follows.",
    [STR_TUT_S12_BRIEF] = "Press R to begin the proceedings anew with a single mote.",
    [STR_TUT_S12_DETAIL] = "Press R to begin the simulation anew. This clears all placed "
        "motes and returns to a single mote with the current appointments. "
        "Any changes to species, vigour, swiftness, or inclination "
        "are applied upon renewal.",
    [STR_TUT_S13_BRIEF] = "Enable Trigger chronicle in Playback, then press R to start "
        "and stop the recording. Shift-P replays event chronicles.",
    [STR_TUT_S13_DETAIL] = "In the Playback section, enable Trigger chronicle. Then press R "
        "to begin anew and commence recording an H.264 video. "
        "Press R again to cease. An event chronicle is saved alongside the video. "
        "Press Shift-P to open and replay a previous chronicle.",
    [STR_TUT_S14_BRIEF] = "Thus endeth the lesson. Press F1 at any time to open "
        "the help reference. Fare thee well!",
    [STR_TUT_S14_DETAIL] = "Thus endeth the lesson. Press F1 at any time to open "
        "the help reference, which contains documentation on keyboard "
        "shortcuts, physics, field models, and the stage. "
        "The Appointments panel (gear icon) permits customization of "
        "colours and stroke widths. Go forth and explore!",
    [STR_HELP_TITLE] = "Lorentz Tracer: A Compendium",
    [STR_HELP_TAB_KEYS] = "Controls",
    [STR_HELP_TAB_PHYSICS] = "Natural Philosophy",
    [STR_HELP_TAB_FIELDS] = "Fields",
    [STR_HELP_TAB_INTERFACE] = "The Stage",
    [STR_HELP_TAB_ABOUT] = "About",
    [STR_HELP_FOOTER] = "Esc or F1 to close  |  Arrow keys or Tab to change acts  |  Scroll to navigate",
    [STR_HELP_RUN_TUTORIAL] = "Commence the Tutorial",
    [STR_HELP_RUN_TUTORIAL_DESC] = "A guided tour of the stage and its workings, step by step.",
    [STR_HELP_FONT_SIZE] = "LETTER DIMENSIONS",
    [STR_HELP_FONT_SIZE_DESC] = "Cmd+Plus / Cmd+Minus adjusts the letter size for the panel, "
        "help overlay, and charts independently depending on "
        "where the mouse doth rest. Cmd+0 restores the default. "
        "All letter sizes are preserved betwixt sessions.",
    [STR_HELP_FACTORY_RESET] = "Restore to Original State",
    [STR_HELP_FACTORY_RESET_DESC] = "Removes all saved appointments and restores every default. There is no undoing this.",
    [STR_LANG_CONTINUE] = "Onward",
};

/* ================================================================
 *  OLD ENGLISH (Eald Englisc)
 * ================================================================ */

static const char *g_strings_oe[STR__COUNT] = {
    [STR_MODEL_PARTICLE] = "Bisen / Lytelcorn",
    [STR_FIELD_MODEL] = "Cræftfeld Bisen",
    [STR_CONFIG] = "Gesetnys:",
    [STR_PARTICLE] = "Lytelcorn",
    [STR_SPECIES_ELECTRON] = "Electron",
    [STR_SPECIES_POSITRON] = "Positron",
    [STR_SPECIES_PROTON] = "Proton",
    [STR_SPECIES_ALPHA] = "Alpha",
    [STR_SPECIES_CUSTOM] = "Āgen",
    [STR_CHARGE] = "Hlæst",
    [STR_MASS] = "Hefe",
    [STR_SPEED] = "Snellnes",
    [STR_ENERGY] = "Mægen",
    [STR_PITCH] = "Healicnes",
    [STR_PLAYBACK_RECORD] = "Plega / Wrītan",
    [STR_TRIGGER_REC] = "Onginnan wrītan (R)",
    [STR_TRIGGER_REC_ACTIVE] = "Wrītende...",
    [STR_PAUSE] = "Stillan",
    [STR_RESUME] = "Forþgān",
    [STR_RESET] = "Edníwian",
    [STR_DISPLAY] = "Gesewen",
    [STR_TRAIL] = "Swæþ",
    [STR_FIELD_LINES] = "Cræftfeld līnan",
    [STR_SCALED] = "Gemeten",
    [STR_SCALE_BAR] = "Metrod",
    [STR_AXES] = "Eaxan",
    [STR_PLOTS] = "Brycgweorce",
    [STR_BOTTOM] = "Niðer",
    [STR_TOP] = "Ufor",
    [STR_AUTOSCALE] = "Selfmetan",
    [STR_RADIATION] = "Beorhtnes",
    [STR_RELATIVISTIC] = "Relativistic",
    [STR_FOLLOW] = "Fylgan (F)",
    [STR_CAM_FREE] = "Frēo (V)",
    [STR_READOUTS] = "Rǣdinge",
    [STR_SETTINGS] = "Setunge",
    [STR_DARK_MODE] = "Þēostru bisen",
    [STR_PANEL] = "Bord",
    [STR_LEFT] = "Winstra",
    [STR_RIGHT] = "Swiþra",
    [STR_STEREO_3D] = "Stereo 3D",
    [STR_SEPARATION] = "Tōdǣl",
    [STR_GAP] = "Hlid",
    [STR_ARROWHEAD] = "Flānheafod",
    [STR_LINE_WIDTHS] = "Līnbrǣdu",
    [STR_COLORS] = "Blēon",
    [STR_PARTICLES] = "Lytelcorn",
    [STR_LW_TRAIL] = "Swæþ",
    [STR_LW_ARROWS] = "Flān",
    [STR_LW_AXES] = "Eaxan",
    [STR_COLOR_FIELD_LINES] = "Cræftfeld līnan",
    [STR_COLOR_AXES] = "Eaxan",
    [STR_LANGUAGE] = "Sprǣc",
    [STR_CANCEL] = "Wiþcēosan",
    [STR_SHOW_UI] = "U: geswutelian þæt andweorc",
    [STR_ARMED] = "GEARU",
    [STR_TAP_CONTINUE] = "hrīnan tō forþgānne",
    [STR_PLOT_PITCH_ANGLE] = "Healicnes horn",
    [STR_PLOT_MAG_MOMENT] = "Cræftfeld miht",
    [STR_PLOT_TIME] = "Tīd (s)",
    [STR_OPEN_RECORDING] = "Ūndōn wrītunge",
    [STR_NO_RECORDINGS] = "Nān wrītung wæs gefunden.",
    [STR_OPEN] = "Ūndōn",
    [STR_PICKER_HELP] = "Cēos mid smealunge, twā-smealung oþþe Ūndōn  |  Esc tō wiþcēosenne  |  Enter tō ūndōnne",
    [STR_MODEL_CIRCULAR] = "Hringlic B (stæþþig |B|)",
    [STR_MODEL_GRAD_B] = "Rihte grad-B",
    [STR_MODEL_QUAD_GRAD] = "Fēowerecge grad-B",
    [STR_MODEL_SINUSOIDAL] = "Yþlic grad-B",
    [STR_MODEL_NONPHYSICAL] = "Ungecyndelic (div B != 0)",
    [STR_MODEL_DIPOLE] = "Cræftfeld twīpol",
    [STR_MODEL_BOTTLE] = "Cræftfeld bytt",
    [STR_PARAM_AMPLITUDE] = "Micelnes a",
    [STR_PARAM_Q_SAFETY] = "Hǣlu dǣl q",
    [STR_PARAM_CONFIG] = "Gesetnys",
    [STR_TUT_PROMPT] = "Wilt þū sēon þā lāre?",
    [STR_TUT_PROMPT_SUB] = "Scort gerecednes þæs andweorces and þāra rǣda.",
    [STR_TUT_MODE_Q] = "Hū micel genyht wilt þū?",
    [STR_TUT_BRIEF] = "Scort oferscēawung",
    [STR_TUT_DETAILED] = "Full gerecednes",
    [STR_TUT_NO_THANKS] = "Nese, ic þancie þē",
    [STR_TUT_YES] = "Gēa",
    [STR_TUT_STOP] = "Geswīcan",
    [STR_TUT_CONTINUE] = "Forþgān",
    [STR_TUT_DONE_HINT] = "Hit is gefremmed! Þrycce Forþgān þonne þū gearu bist.",
    [STR_TUT_S0_BRIEF] = "Sēo onlīcung is gestilled. Þrycce Spacebar tō plegan oþþe stillan.",
    [STR_TUT_S0_DETAIL] = "Sēo onlīcung is gestilled. Þrycce Spacebar tō plegan oþþe stillan. "
        "Þonne gestilled, þrycce N tō gānne āne stæpe forþ, oþþe P tō "
        "gānne underbæc. Heald ǣghwæþerne tō forþgānne on þǣre nū snelnes.",
    [STR_TUT_S1_BRIEF] = "Swiþra-smealung and dragan tō ymbhweorfenne. Scrolla tō genēalǣcenne. "
        "Þā blēon eaxan geswuteliað x (rēad), y (grēne), z (hæwen).",
    [STR_TUT_S1_DETAIL] = "Swiþra-smealung and dragan tō ymbhweorfenne þæt mǣl. "
        "Scrolla tō genēalǣcenne oþþe feorrian. Þā blēon eaxan "
        "geswuteliað x (rēad), y (grēne), z (hæwen). Shift+swiþra-dragan "
        "ymbhweorfeþ ymbe þone fruman. Þrycce 0 tō edsetenne. "
        "Þā flān sind gehȳdd nū swā þū meaht gesēon þā yrmbcyrf and feld.",
    [STR_TUT_S2_BRIEF] = "Þā bogan on þǣre gesihþe sind cræftfeld līnan. Hīe geswuteliað "
        "þone weg and gesceap þæs cræftfeldes.",
    [STR_TUT_S2_DETAIL] = "Cræftfeld līnan secgaþ þone weg B geond eall þæt land. "
        "Se lytelcorn hweorfeþ ymbe and drīfþ andlang þǣra līnana. "
        "On þǣm Gesewen dǣle þū meaht ēac ātēon þā GC fl "
        "and FL pos līnan.",
    [STR_TUT_S3_BRIEF] = "Þrycce F tō fylgenne þǣm lytelcorne mid þīnre gesihþe. "
        "Scrolla tō genēalǣcenne þǣre yrmbcyrfe.",
    [STR_TUT_S3_DETAIL] = "Þrycce F tō onwendendne fylgende bisen. Sēo gesihþ fylgeþ "
        "þæs lytelcornes stōwe. Þonne fylgende, scrolla tō genēalǣcenne. "
        "Nīehste stæpas gebringaþ flān, swā nēahnes hīe ēaþor gesēon dēþ.",
    [STR_TUT_S4_BRIEF] = "Þā flān æt þǣm lytelcorne geswuteliað þone snelnes (v) and "
        "þone cræftfeld weg (B).",
    [STR_TUT_S4_DETAIL] = "Þā snelnes and B-feld flān sind nū geswutelod. "
        "Se snelnes flā (v) geswutlaþ þæs lytelcornes nū weg. "
        "Se cræftfeld flā (B) geswutlaþ þone stōwlicne feld weg. "
        "Bēgen sind getogen swā ānfealdne; onbryrd Gemeten bisen "
        "on þǣm Gesewen dǣle tō hiera micelnessa geswutelienne.",
    [STR_TUT_S5_BRIEF] = "Sēo Lorentz cræft F = q(v \xc3\x97 B) is þweorh tō bām "
        "v and B. His flā is nū geswutelod.",
    [STR_TUT_S5_DETAIL] = "Sēo Lorentz cræft F = q(v \xc3\x97 B) is ǣfre þweorh "
        "tō bām þǣm snelnesse and þǣm cræftfelde. Þēos cræft "
        "bīegeþ þæs lytelcornes weg on scrūfe. Þū meaht cēosan "
        "v, B, and F flān on þǣm Gesewen dǣle.",
    [STR_TUT_S6_BRIEF] = "Þrycce V tō lōcienne andlang þæs cræftfeldes. "
        "Flān tācna hwearfiað þā gesihþe.",
    [STR_TUT_S6_DETAIL] = "Þrycce V tō hweorfenne betwux frēore gesihþe, +B lōciende, "
        "and -B lōciende. On feld-gerihte gesihþe, se crymbing (\xce\xba) "
        "and binormal bēoþ geswutelod. Flān tācna hwearfiað \xce\xba weg. "
        "Þēos gesihþ geswutlaþ þone drīf þweorh tō B.",
    [STR_TUT_S7_BRIEF] = "Se sīdbord (U tō onwendenne) hæfþ fealgende dǣlas: "
        "Bisen/Lytelcorn, Plega, Gesewen, and Rǣdinge.",
    [STR_TUT_S7_DETAIL] = "Þrycce U tō geswutelienne oþþe behȳdenne þone sīdbord. "
        "He hæfþ fēower fealgende dǣlas. Bisen/Lytelcorn cīest "
        "þone cræftfeld and lytelcornes cynn, mægen, and healicnes. "
        "Plega gewieldeþ snelnes and wrītung. Gesewen onwendeþ "
        "þā gesewenlican þing. Rǣdinge geswutlaþ nū sceawunge. "
        "Se gearwian hæpse ūndēþ setunge.",
    [STR_TUT_S8_BRIEF] = "Lytelcornes setunge swā swā mægen and healicnes nimþ "
        "angin on edníwiunge (R) oþþe nīwes lytelcornes mid Shift+smealung.",
    [STR_TUT_S8_DETAIL] = "On þǣm Bisen/Lytelcorn dǣle þū meaht onwendan cynn, "
        "mægen (oþþe snelnes for Āgen), and healicnes. "
        "Þās sind bisenan: hīe onginnaþ þonne þū R þryccest, "
        "oþþe þonne þū nīwne lytelcorn settest mid Shift+smealung. "
        "Þā nū lytelcorn healdaþ hiera ǣrran setunge.",
    [STR_TUT_S9_BRIEF] = "Cēos þone Dipole cræftfeld bisen. "
        "Sēo gesihþ edníwaþ self þonne bisen onwendeþ.",
    [STR_TUT_S9_DETAIL] = "Se Cræftfeld Bisen tæfl hæfþ 10 cræftfeld gesceapas: "
        "hringlic, grad-B, twīpol, bytt, tokamak, and mā. "
        "Ǣlc bisen hæfþ onwendenlice steallas. Nīwe bisen cēosende "
        "edníwaþ lytelcorn and gesihþe. Cēos þone Dipole bisen "
        "tō gesēonne gefangen lytelcorn.",
    [STR_TUT_S10_BRIEF] = "Þā brycgweorce geswuteliað healicnes horn and cræftfeld miht "
        "wið tīd.",
    [STR_TUT_S10_DETAIL] = "Þā niðeran brycgweorce geswuteliað healicnes horn and cræftfeld miht "
        "(\xce\xbc) wið tīd. On hluttrum cræftfelde, "
        "\xce\xbc is āstæðfæst and sceal wunian nēah stæþþig. "
        "Þā cēapas (x1/x10/x100) gewieldaþ hū micel gewyrd is geswutelod. "
        "Selfmetan ārǣreþ þone healicnes horn mǣd.",
    [STR_TUT_S11_BRIEF] = "Þrycce 0 tō edsetenne þā gesihþe. Heald Winstra Shift "
        "and smēa tō settenne mā lytelcorn. Tab cyrreþ þone cyre.",
    [STR_TUT_S11_DETAIL] = "Þrycce 0 tō edsetenne þā gesihþe. Heald Winstra Shift "
        "and smēa on þǣre 3D gesihþe tō settenne nīwne lytelcorn. "
        "Oþ 16 lytelcorn mæg bēon getracod samod. "
        "Þrycce Tab tō cyrreþ hwylcne lytelcorn sēo gesihþ fylgeþ.",
    [STR_TUT_S12_BRIEF] = "Þrycce R tō edníwienne þā onlīcunge mid ānum lytelcorne.",
    [STR_TUT_S12_DETAIL] = "Þrycce R tō edníwienne þā onlīcunge. Þis āfierþ ealle "
        "lytelcorn and gecyrreþ tō ānum lytelcorne mid nū setungum. "
        "Ǣlc onwending tō cynne, mægene, snelnesse, oþþe healicnesse "
        "is gefremed on edníwiunge.",
    [STR_TUT_S13_BRIEF] = "Onbryrd wrītung on Plega, þonne þrycce R tō onginnenne "
        "and geswīcanne. Shift-P edplægaþ ǣrre wrītunge.",
    [STR_TUT_S13_DETAIL] = "On þǣm Plega dǣle, onbryrd wrītung. Þonne þrycce R "
        "tō edníwienne and onginnenne H.264 fīlm wrītung. "
        "Þrycce R eft tō geswīcanne. Gewyrd stǣr is geheald. "
        "Þrycce Shift-P tō ūndōnne and edplægenne ǣrre wrītunge.",
    [STR_TUT_S14_BRIEF] = "Þæt is þæt nēodþearflic. Þrycce F1 ǣnigne tīd tō ūndōnne "
        "þā fultumrǣdinge. Wes hāl!",
    [STR_TUT_S14_DETAIL] = "Þæt is þæt nēodþearflic. Þrycce F1 ǣnigne tīd tō ūndōnne "
        "þā fultumrǣdinge, sēo hæfþ fullīce gewritu on tācna, "
        "gecyndelic lāre, cræftfeld bisen, and þæt andweorc. "
        "Se Setunge bord (gearwian hæpse) hæfþ blēon and līnbrǣdu. "
        "Gā forþ and sēc. Wes hāl!",
    [STR_HELP_TITLE] = "Lorentz Tracer Fultumrǣding",
    [STR_HELP_TAB_KEYS] = "Tācna",
    [STR_HELP_TAB_PHYSICS] = "Gecyndelāre",
    [STR_HELP_TAB_FIELDS] = "Feldas",
    [STR_HELP_TAB_INTERFACE] = "Andweorc",
    [STR_HELP_TAB_ABOUT] = "Ymbe",
    [STR_HELP_FOOTER] = "Esc oþþe F1 tō lūcenne  |  Flān oþþe Tab tō onwendenne  |  Scrolla tō farenne",
    [STR_HELP_RUN_TUTORIAL] = "Onginnan þā Lāre",
    [STR_HELP_RUN_TUTORIAL_DESC] = "Stæpe be stæpe gerecednes þæs andweorces and þāra rǣda.",
    [STR_HELP_FONT_SIZE] = "STAFMICELNES",
    [STR_HELP_FONT_SIZE_DESC] = "Cmd+Plus / Cmd+Minus onwendeþ stafmicelnes for þǣm borde, "
        "fultumrǣdinge, and brycgweorce; hangað on þǣm stōwe þǣre mūs. "
        "Cmd+0 edseteþ. Ealle stafmicelnes bēoþ geheald betwux situngum.",
    [STR_HELP_FACTORY_RESET] = "Edsetan tō Fruman Setungum",
    [STR_HELP_FACTORY_RESET_DESC] = "Āfierþ ealle gehealdene setunge and edseteþ eall tō fruman.",
    [STR_LANG_CONTINUE] = "Forþgān",
};
