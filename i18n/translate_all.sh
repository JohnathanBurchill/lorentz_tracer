#!/bin/bash
# translate_all.sh — Batch-translate all i18n strings via translate-shell
# Usage: ./translate_all.sh <lang_code>   e.g. ./translate_all.sh fr
# Requires: brew install translate-shell
# Output: i18n/all_<lang>.txt  (review before pasting into i18n.c)

set -e

LANG_CODE="${1:?Usage: $0 <lang_code>  (e.g. fr, de, es, pt, it, ru, ja, zh-CN, zh-TW, ko, eo)}"
OUT="i18n/all_${LANG_CODE}.txt"

echo "# All string translations: en → ${LANG_CODE}" > "$OUT"
echo "# Generated on $(date +%Y-%m-%d)" >> "$OUT"
echo "# Review carefully. Preserve format specifiers (%d, %.2f, etc.)" >> "$OUT"
echo "# Preserve keyboard shortcuts (F1, R, V, F, U, Tab, Space, etc.)" >> "$OUT"
echo "# Preserve physics symbols (v, B, F, κ, μ, α)" >> "$OUT"
echo "" >> "$OUT"

# Strings that should NOT be translated (technical abbreviations, physics notation)
SKIP_KEYS="STR_GC_FL STR_FL_POS STR_GC_LEN STR_LW_FL_GC STR_LW_FL_POS \
STR_COLOR_FL_GC STR_COLOR_FL_POS STR_GIJ STR_CAM_PB STR_CAM_MB \
STR_RAD_X STR_V_SCALE STR_B_SCALE STR_F_SCALE STR_OK \
STR_STEL_CFG_R2 STR_STEL_CFG_LP22 STR_SPECIES_OPLUS \
STR_COLOR_V_ARROW STR_COLOR_B_ARROW STR_COLOR_F_ARROW \
STR_LW_B_LINES STR_LW_KAPPA STR_COLOR_KAPPA STR_COLOR_BINORMAL \
STR_PARAM_B0 STR_PARAM_LAMBDA_1M STR_PARAM_LAMBDA_1M2 \
STR_PARAM_WAVENUMBER STR_PARAM_LAMBDA_TM STR_PARAM_M_DIPOLE \
STR_PARAM_B0_BOTTLE STR_PARAM_LM STR_PARAM_R0 STR_PARAM_A_MINOR \
STR_PARAM_R0_PHYS STR_PARAM_B0_PHYS STR_LORENTZ_TRACER \
STR_MODEL_TOKAMAK STR_MODEL_STELLARATOR STR_MODEL_TORUS \
STR_LANG_TITLE"

is_skip() {
    for s in $SKIP_KEYS; do [ "$1" = "$s" ] && return 0; done
    return 1
}

translate() {
    local key="$1" en="$2"

    if is_skip "$key"; then
        echo "[$key] = \"$en\"  # KEEP" >> "$OUT"
        return
    fi

    local translated
    translated=$(trans -b ":${LANG_CODE}" "$en" 2>/dev/null || echo "???")
    echo "[$key] = \"$translated\"" >> "$OUT"
    sleep 0.3
}

# ---- All strings, in order ----

echo "## UI: Model / Particle" >> "$OUT"
translate STR_MODEL_PARTICLE "Model / Particle"
translate STR_FIELD_MODEL "Field Model"
translate STR_CONFIG "Config:"
translate STR_PARTICLE "Particle"
translate STR_SPECIES_ELECTRON "Electron"
translate STR_SPECIES_POSITRON "Positron"
translate STR_SPECIES_PROTON "Proton"
translate STR_SPECIES_ALPHA "Alpha"
translate STR_SPECIES_OPLUS "O+"
translate STR_SPECIES_CUSTOM "Custom"
translate STR_CHARGE "Charge"
translate STR_MASS "Mass"
translate STR_SPEED "Speed"
translate STR_ENERGY "Energy"
translate STR_PITCH "Pitch"

echo "" >> "$OUT"
echo "## UI: Playback / Record" >> "$OUT"
translate STR_PLAYBACK_RECORD "Playback / Record"
translate STR_TRIGGER_REC "Trigger rec (R)"
translate STR_TRIGGER_REC_ACTIVE "Trigger rec (recording...)"
translate STR_PAUSE "Pause"
translate STR_RESUME "Resume"
translate STR_RESET "Reset"

echo "" >> "$OUT"
echo "## UI: Display" >> "$OUT"
translate STR_DISPLAY "Display"
translate STR_TRAIL "Trail"
translate STR_FIELD_LINES "Field lines"
translate STR_GC_FL "GC fl"
translate STR_FL_POS "FL pos"
translate STR_GC_LEN "GC len"
translate STR_SCALED "Scaled"
translate STR_V_SCALE "v scale"
translate STR_B_SCALE "B scale"
translate STR_F_SCALE "F scale"
translate STR_GIJ "G_ij (G)"
translate STR_SCALE_BAR "Scale bar"
translate STR_AXES "Axes"
translate STR_PLOTS "Plots"
translate STR_BOTTOM "Bottom"
translate STR_TOP "Top"
translate STR_AUTOSCALE "Autoscale"
translate STR_RADIATION "Radiation"
translate STR_RAD_X "Rad x"
translate STR_RELATIVISTIC "Relativistic"
translate STR_FOLLOW "Follow (F)"
translate STR_CAM_FREE "Free (V)"
translate STR_CAM_PB "+b (V)"
translate STR_CAM_MB "-b (V)"

echo "" >> "$OUT"
echo "## UI: Readouts" >> "$OUT"
translate STR_READOUTS "Readouts"
translate STR_FMT_PARTICLE_N "particle %d / %d (Tab)"
translate STR_FMT_TIME "t = %.4g s"
translate STR_FMT_ENERGY_ERR "|dE|/E = %.2e"
translate STR_FMT_PITCH "pitch = %.1f deg"
translate STR_FMT_MU "mu = %.3e J/T"
translate STR_FMT_BMAG "|B| = %.3e T"
translate STR_FMT_ENERGY_MEV "E = %.3f MeV"
translate STR_FMT_ENERGY_KEV "E = %.3f keV"
translate STR_FMT_PRAD "P_rad = %.3e W"
translate STR_FMT_DEDT "dE/dt = %.3e keV/s"
translate STR_FMT_DEDT_MULT "dE/dt = %.3e keV/s (x%.3g)"

echo "" >> "$OUT"
echo "## UI: Settings" >> "$OUT"
translate STR_SETTINGS "Settings"
translate STR_DARK_MODE "Dark mode"
translate STR_PANEL "Panel"
translate STR_LEFT "Left"
translate STR_RIGHT "Right"
translate STR_STEREO_3D "Stereo 3D"
translate STR_SEPARATION "Separation"
translate STR_GAP "Gap"
translate STR_ARROWHEAD "Arrowhead"
translate STR_LINE_WIDTHS "Line widths"
translate STR_COLORS "Colors"
translate STR_PARTICLES "Particles"
translate STR_LW_TRAIL "Trail"
translate STR_LW_B_LINES "B lines"
translate STR_LW_FL_GC "FL GC"
translate STR_LW_FL_POS "FL pos"
translate STR_LW_ARROWS "Arrows"
translate STR_LW_KAPPA "κ / e₂"
translate STR_LW_AXES "Axes"
translate STR_COLOR_V_ARROW "v arrow"
translate STR_COLOR_B_ARROW "B arrow"
translate STR_COLOR_F_ARROW "F arrow"
translate STR_COLOR_FIELD_LINES "Field lines"
translate STR_COLOR_FL_GC "FL GC"
translate STR_COLOR_FL_POS "FL pos"
translate STR_COLOR_KAPPA "κ-hat"
translate STR_COLOR_BINORMAL "Binormal"
translate STR_COLOR_AXES "Axes"
translate STR_LANGUAGE "Language"

echo "" >> "$OUT"
echo "## Color picker" >> "$OUT"
translate STR_OK "OK"
translate STR_CANCEL "Cancel"

echo "" >> "$OUT"
echo "## HUD" >> "$OUT"
translate STR_SHOW_UI "U: show UI"
translate STR_ARMED "ARMED"

echo "" >> "$OUT"
echo "## Splash" >> "$OUT"
translate STR_TAP_CONTINUE "tap to continue"

echo "" >> "$OUT"
echo "## Plots" >> "$OUT"
translate STR_PLOT_PITCH_ANGLE "Pitch angle"
translate STR_PLOT_MAG_MOMENT "Magnetic moment"
translate STR_PLOT_TIME "Time (s)"

echo "" >> "$OUT"
echo "## Playback picker" >> "$OUT"
translate STR_OPEN_RECORDING "Open Recording"
translate STR_NO_RECORDINGS "No recordings found."
translate STR_OPEN "Open"
translate STR_PICKER_HELP "Click to select, double-click or Open to play  |  Esc to cancel  |  Enter to open"

echo "" >> "$OUT"
echo "## Field model names" >> "$OUT"
translate STR_MODEL_CIRCULAR "Circular B (const |B|)"
translate STR_MODEL_GRAD_B "Linear grad-B"
translate STR_MODEL_QUAD_GRAD "Quadratic grad-B"
translate STR_MODEL_SINUSOIDAL "Sinusoidal grad-B"
translate STR_MODEL_NONPHYSICAL "Nonphysical (div B != 0)"
translate STR_MODEL_DIPOLE "Magnetic dipole"
translate STR_MODEL_BOTTLE "Magnetic bottle"
translate STR_MODEL_TOKAMAK "Tokamak"
translate STR_MODEL_STELLARATOR "Stellarator (NAX)"
translate STR_MODEL_TORUS "Torus"

echo "" >> "$OUT"
echo "## Field parameters" >> "$OUT"
translate STR_PARAM_B0 "B0 (T)"
translate STR_PARAM_LAMBDA_1M "lambda (1/m)"
translate STR_PARAM_LAMBDA_1M2 "lambda (1/m²)"
translate STR_PARAM_AMPLITUDE "Amplitude a"
translate STR_PARAM_WAVENUMBER "Wavenumber k (1/m)"
translate STR_PARAM_LAMBDA_TM "lambda (T/m)"
translate STR_PARAM_M_DIPOLE "M (T·m³)"
translate STR_PARAM_B0_BOTTLE "B0 (T)"
translate STR_PARAM_LM "Lm (m)"
translate STR_PARAM_R0 "R0 (m)"
translate STR_PARAM_Q_SAFETY "Safety factor q"
translate STR_PARAM_A_MINOR "a (m)"
translate STR_PARAM_CONFIG "Config"
translate STR_PARAM_R0_PHYS "R0_phys (m)"
translate STR_PARAM_B0_PHYS "B0_phys (T)"
translate STR_STEL_CFG_R2 "r2 s5.1 (QA)"
translate STR_STEL_CFG_LP22 "Precise QA (LP22)"

echo "" >> "$OUT"
echo "## Tutorial prompts" >> "$OUT"
translate STR_TUT_PROMPT "Would you like to see the tutorial?"
translate STR_TUT_PROMPT_SUB "A short walkthrough of the interface and controls."
translate STR_TUT_MODE_Q "How much detail would you like?"
translate STR_TUT_BRIEF "Brief overview"
translate STR_TUT_DETAILED "Detailed walkthrough"
translate STR_TUT_NO_THANKS "No thanks"
translate STR_TUT_YES "Yes"
translate STR_TUT_STOP "Stop"
translate STR_TUT_CONTINUE "Continue"
translate STR_TUT_DONE_HINT "Done! Press Continue when ready."

echo "" >> "$OUT"
echo "## Tutorial steps (brief)" >> "$OUT"
translate STR_TUT_S0_BRIEF "The simulation is paused. Press Spacebar to play or pause."
translate STR_TUT_S1_BRIEF "Right-click and drag to orbit the camera. Scroll to zoom. The colored axes show x (red), y (green), z (blue)."
translate STR_TUT_S2_BRIEF "The curves in the scene are magnetic field lines. They show the direction and geometry of the magnetic field."
translate STR_TUT_S3_BRIEF "Press F to follow the particle with the camera. Scroll to zoom in close to the orbit."
translate STR_TUT_S4_BRIEF "The arrows at the particle show the velocity (v) and the magnetic field direction (B)."
translate STR_TUT_S5_BRIEF "The Lorentz force F = q(v × B) is perpendicular to both v and B. Its unit vector is now shown as an arrow."
translate STR_TUT_S6_BRIEF "Press V to look along the magnetic field direction. Arrow keys rotate the view orientation."
translate STR_TUT_S7_BRIEF "The side panel (U to toggle) has collapsible sections: Model/Particle, Playback, Display, and Readouts."
translate STR_TUT_S8_BRIEF "Particle settings like energy (or speed for Custom) and pitch angle take effect on reset (R) or when placing a new particle with Shift+click."
translate STR_TUT_S9_BRIEF "Try selecting the Dipole field model from the dropdown. The view resets automatically when switching models."
translate STR_TUT_S10_BRIEF "The telemetry plots show pitch angle and magnetic moment versus time."
translate STR_TUT_S11_BRIEF "Press 0 to reset the camera view. Then hold Left Shift and click in the scene to place additional particles. Tab cycles selection."
translate STR_TUT_S12_BRIEF "Press R to reset the simulation back to a single particle."
translate STR_TUT_S13_BRIEF "Enable Trigger rec in Playback, then press R to start and stop video recording. Shift-P replays event logs."
translate STR_TUT_S14_BRIEF "That covers the essentials. Press F1 anytime to open the help reference."

echo "" >> "$OUT"
echo "## Tutorial steps (detailed)" >> "$OUT"
translate STR_TUT_S0_DETAIL "The simulation is paused. Press Spacebar to play or pause. While paused, press N to step forward one timestep, or P to step backward. Hold either key to advance continuously at the current playback speed."
translate STR_TUT_S1_DETAIL "Right-click and drag to orbit the camera around the target point. Scroll to zoom in and out. The colored axes at the origin show x (red), y (green), z (blue). Shift+right-drag orbits around the origin. Press 0 to re-center the view. The vector arrows are hidden for now so you can see the orbit and field geometry clearly."
translate STR_TUT_S2_DETAIL "Field lines trace the direction of B throughout the domain. The particle gyrates around and drifts along these lines. In the Display section you can also enable the guiding-center field line (GC fl), which follows the averaged orbit center, and the position field line (FL pos) through the particle's current location."
translate STR_TUT_S3_DETAIL "Press F to toggle follow mode. The camera tracks the particle's position while you can still orbit and zoom. Once following, scroll to zoom in close to the particle. The next steps will turn on vector arrows at the particle position, so getting close makes them easier to see."
translate STR_TUT_S4_DETAIL "The velocity and B-field arrows are now turned on. The velocity vector (v) shows the particle's instantaneous direction of motion. The magnetic field vector (B) shows the local field direction. Both are drawn as unit vectors by default; enable Scaled mode in the Display section to show their magnitudes instead."
translate STR_TUT_S5_DETAIL "The Lorentz force F = q(v × B) is always perpendicular to both the velocity and the magnetic field. This is what curves the particle's path into a helix. Its unit vector is now shown as an arrow. You can toggle any of the v, B, and F vectors individually in the Display section of the side panel."
translate STR_TUT_S6_DETAIL "Press V to cycle between free camera, looking along +B, and looking along -B. In field-aligned views, the curvature vector (κ) and binormal are shown as arrows. Arrow keys rotate which direction κ points on screen. This view reveals the drift motion perpendicular to B."
translate STR_TUT_S7_DETAIL "Press U to show or hide the side panel. It has four collapsible sections. Model/Particle selects the magnetic field geometry and particle species, energy, and pitch angle. Playback controls simulation speed and recording. Display toggles visual elements. Readouts show live diagnostics. The gear button opens Settings for colors and line widths."
translate STR_TUT_S8_DETAIL "In the Model/Particle section, you can change the species, energy (or speed for the Custom species), and pitch angle. These are templates: they take effect the next time you press R to reset, or when you place a new particle with Left Shift + click. Existing particles keep their original parameters."
translate STR_TUT_S9_DETAIL "The Field Model dropdown lists 10 magnetic field geometries: circular, grad-B, dipole, magnetic bottle, tokamak, and more. Each model has adjustable parameters. Selecting a new model resets the particle and camera to defaults. Try selecting the Dipole model to see trapped particle motion."
translate STR_TUT_S10_DETAIL "The bottom plots show pitch angle and magnetic moment (μ) as functions of time. In a pure magnetic field, μ is an adiabatic invariant and should stay nearly constant. The plot range buttons (x1/x10/x100) control how much history is shown. Autoscale adjusts the pitch axis range automatically."
translate STR_TUT_S11_DETAIL "Press 0 to reset the camera to the default view. Then hold Left Shift and click in the 3D scene to place a new particle with the current species, energy, and pitch settings. Up to 16 particles can be traced simultaneously. Press Tab to cycle which particle the camera follows and whose diagnostics are shown."
translate STR_TUT_S12_DETAIL "Press R to reset the simulation. This clears all placed particles and returns to a single particle with the current settings. Any changes you made to species, energy, speed, or pitch angle in the panel are applied on reset."
translate STR_TUT_S13_DETAIL "In the Playback section, enable Trigger rec. Then press R to reset the simulation and begin recording an H.264 video. Press R again to stop. An event log is saved alongside the video. Press Shift-P to open and replay a previous recording, reproducing the exact simulation."
translate STR_TUT_S14_DETAIL "That covers the essentials. Press F1 anytime to open the help reference, which has detailed documentation on keyboard shortcuts, physics, field models, and the interface. The Settings panel (gear icon) has color and line width customization. Enjoy exploring."

echo "" >> "$OUT"
echo "## Help structural" >> "$OUT"
translate STR_HELP_TITLE "Lorentz Tracer Help"
translate STR_HELP_TAB_KEYS "Keys"
translate STR_HELP_TAB_PHYSICS "Physics"
translate STR_HELP_TAB_FIELDS "Fields"
translate STR_HELP_TAB_INTERFACE "Interface"
translate STR_HELP_TAB_ABOUT "About"
translate STR_HELP_FOOTER "Esc or F1 to close  |  Arrow keys or Tab to switch tabs  |  Scroll to navigate"
translate STR_HELP_RUN_TUTORIAL "Run Tutorial"
translate STR_HELP_RUN_TUTORIAL_DESC "Walk through the interface and controls step by step."
translate STR_HELP_FONT_SIZE "FONT SIZE"
translate STR_HELP_FONT_SIZE_DESC "Cmd+Plus / Cmd+Minus adjusts the font size for the UI panel, help overlay, and telemetry plots independently depending on which area the mouse is over. Cmd+0 resets to default. All font sizes are saved between sessions."
translate STR_HELP_FACTORY_RESET "Factory Reset"
translate STR_HELP_FACTORY_RESET_DESC "Removes saved settings and restores all defaults."

echo "" >> "$OUT"
echo "## Language selection" >> "$OUT"
translate STR_LANG_TITLE "Language / Langue / Sprache"
translate STR_LANG_CONTINUE "Continue"

echo ""
echo "Done. Output: $OUT"
echo "Review the file, then I'll assemble it into i18n.c"
