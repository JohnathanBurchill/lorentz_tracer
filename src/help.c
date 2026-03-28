/* help.c — Help overlay for Lorentz Tracer
 * Uses clay.h layout engine for text wrapping and scroll containers.
 * Five tabs: Keys, Physics, Fields, Interface, About. */

#include "help.h"
#include "i18n.h"
#include "app.h"
#include "clay.h"
#include "raylib.h"
#include <string.h>
#include <math.h>

#define FONT_UI   0
#define FONT_MONO 1

/* ---- Content helpers (used as shorthand in tab functions) ---- */

/* Help text macros — font sizes scale by g_help_zoom.
 * H/P/M/ML accept const char* (not just literals) so they work with tr(). */
static float g_help_zoom = 1.0f;

static Clay_String S(const char *s) {
    return (Clay_String){ .length = (int32_t)strlen(s), .chars = s };
}

#define H(str) CLAY_TEXT(S(str), CLAY_TEXT_CONFIG({ \
    .fontId = FONT_UI, .fontSize = (uint16_t)(19 * g_help_zoom), \
    .letterSpacing = 1, .textColor = hc, \
    .lineHeight = (uint16_t)(26 * g_help_zoom) }))

#define P(str) CLAY_TEXT(S(str), CLAY_TEXT_CONFIG({ \
    .fontId = FONT_UI, .fontSize = (uint16_t)(16 * g_help_zoom), \
    .letterSpacing = 1, .textColor = bc, \
    .wrapMode = CLAY_TEXT_WRAP_WORDS, \
    .lineHeight = (uint16_t)(23 * g_help_zoom) }))

/* Single-line equation (no wrap) */
#define M(str) CLAY_TEXT(S(str), CLAY_TEXT_CONFIG({ \
    .fontId = FONT_MONO, .fontSize = (uint16_t)(15 * g_help_zoom), \
    .letterSpacing = 1, .textColor = mc, \
    .wrapMode = CLAY_TEXT_WRAP_NONE, \
    .lineHeight = (uint16_t)(20 * g_help_zoom) }))

/* Multi-line preformatted text (break on \n only) */
#define ML(str) CLAY_TEXT(S(str), CLAY_TEXT_CONFIG({ \
    .fontId = FONT_MONO, .fontSize = (uint16_t)(15 * g_help_zoom), \
    .letterSpacing = 1, .textColor = mc, \
    .wrapMode = CLAY_TEXT_WRAP_NEWLINES, \
    .lineHeight = (uint16_t)(20 * g_help_zoom) }))

#define RULE() CLAY_AUTO_ID({ \
    .layout = { .sizing = { CLAY_SIZING_GROW(0), CLAY_SIZING_FIXED(1) } }, \
    .backgroundColor = dc }) {}

#define GAP(h) CLAY_AUTO_ID({ \
    .layout = { .sizing = { CLAY_SIZING_GROW(0), CLAY_SIZING_FIXED(h) } } }) {}

/* ================================================================
 *  TAB 0: QUICK REFERENCE
 * ================================================================ */
static void tab_keys(Clay_Color hc, Clay_Color bc, Clay_Color mc, Clay_Color dc)
{
    H("KEYBOARD");
    ML("Space           Pause / resume simulation\n"
       "N               Step forward one timestep (when paused)\n"
       "P               Step back one timestep (when paused)\n"
       "R               Reset particle (start/stop recording if armed)\n"
       "F               Toggle follow-camera mode\n"
       "V               Cycle field-aligned camera: free > +b > -b\n"
       "Shift+V         Reverse cycle: free > -b > +b\n"
       "Shift+P         Open recording file picker\n"
       "G               Toggle G_ij tensor display\n"
       "U               Toggle UI panel visibility\n"
       "3               Toggle side-by-side stereoscopic 3D\n"
       "[ / ]           Adjust stereo image separation\n"
       "+ / -           Double / halve playback speed\n"
       "0               Reset camera to origin\n"
       "Cmd+F           Toggle fullscreen (Ctrl+F on Linux/Windows)\n"
       "F1              Open / close this help\n"
       "Esc             Close this help");
    GAP(8);
    H("MOUSE");
    ML("Right-drag            Orbit camera around target\n"
       "Shift + right-drag    Orbit entire scene around origin\n"
       "RShift + left-drag    Pan camera in view plane\n"
       "Scroll wheel          Zoom in / out\n"
       "LShift + hover        Preview particle placement point\n"
       "LShift + click        Place particle at preview point");
    GAP(8);
    H("TOUCH");
    ML("1-finger drag         Orbit camera\n"
       "2-finger pinch        Zoom in / out\n"
       "2-finger drag         Pan camera\n"
       "Double-tap            Pause / resume");
    GAP(8);
    H("HELP NAVIGATION");
    ML("Left / Right          Previous / next tab\n"
       "Tab / Shift+Tab       Cycle tabs\n"
       "Scroll wheel          Scroll content\n"
       "Esc or F1             Close help");
}

/* ================================================================
 *  TAB 1: PHYSICS
 * ================================================================ */
static void tab_physics(Clay_Color hc, Clay_Color bc, Clay_Color mc, Clay_Color dc)
{
    H("CHARGED PARTICLE MOTION");
    P("A charged particle in a magnetic field experiences the Lorentz force "
      "F = q(v x B). Because this force is always perpendicular to the "
      "velocity, it changes the particle's direction but not its speed. The "
      "result is gyration: the particle spirals around the magnetic field "
      "direction with a characteristic radius and frequency. Stormer "
      "(1907) computed the first trajectories in a dipole field; "
      "Alfv\xc3\xa9n (1950) introduced the guiding-center concept.");
    GAP(4);
    M("  Larmor radius:  r_L = m v_perp / (|q| B)");
    M("  Gyrofrequency:  \xcf\x89_c = |q| B / m");
    GAP(4);
    P("The gyroperiod sets the fundamental timescale. This simulation uses "
      "256 substeps per gyroperiod, giving smooth trails and energy "
      "conservation to machine precision.");
    GAP(8); RULE(); GAP(4);

    H("PITCH ANGLE AND MAGNETIC MOMENT");
    P("The pitch angle alpha is the angle between the velocity and the "
      "magnetic field direction. The magnetic moment mu is an adiabatic "
      "invariant: nearly conserved when the field changes slowly compared "
      "to the gyration. The systematic theory of adiabatic invariants "
      "was developed by Kruskal (1965) and Northrop (1963).");
    GAP(4);
    M("  cos(\xce\xb1) = v\xe2\x80\x96 / v");
    M("  \xce\xbc = m v_perp\xc2\xb2 / (2B)");
    GAP(4);
    P("Watch the mu readout as a particle moves through an inhomogeneous "
      "field: it stays remarkably constant even as the pitch angle and "
      "field strength change.");
    GAP(8); RULE(); GAP(4);

    H("CURVATURE DRIFT");
    P("Conventional explanation (Northrop 1963; Bellan 2006; Baumjohann "
      "and Treumann 1996): One transforms to a frame following the "
      "guiding center along the curved field line. The parallel velocity "
      "along this curved path produces a centrifugal force that drives a "
      "drift perpendicular to both B and the curvature:");
    M("  v_curv = (m v\xe2\x80\x96\xc2\xb2 / q) (\xce\xba \xc3\x97 B) / B\xc2\xb2");
    GAP(4);
    P("The paper's approach: Write the exact Lorentz force equation in the "
      "field-aligned triad (e\xe2\x82\x81, e\xe2\x82\x82, b^). "
      "The perpendicular momentum components couple to the parallel "
      "momentum through the rate of change of b^ along the "
      "trajectory. This involves only field-direction gradients: the "
      "curvature \xce\xba and the tensor G_ij. What textbooks attribute "
      "to a centrifugal pseudo-force is simply the Lorentz force "
      "resolved in a frame that rotates with b^. There is no "
      "fictitious force; the drift is a direct consequence of v \xc3\x97 B "
      "acting on a gyrating particle whose field direction changes along "
      "the orbit.");
    GAP(4);
    P("The circular field model (constant |B|, pure curvature) isolates "
      "this drift cleanly: no grad-B contribution, just the curvature "
      "term.");
    GAP(8); RULE(); GAP(4);

    H("GRAD-B DRIFT");
    P("Conventional explanation (Cully and Donovan 1999 give a particularly "
      "clear derivation using energy conservation): When |B| varies "
      "across the orbit, the Larmor radius is larger on the weak-field "
      "side. The orbit does not close; the particle drifts perpendicular "
      "to both B and grad|B|:");
    M("  v_gradB = (m v_perp\xc2\xb2 / 2q) (B \xc3\x97 grad|B|) / B\xc2\xb2");
    GAP(4);
    P("The paper's approach: The grad-B drift arises from the "
      "perpendicular-block trace of the field-direction gradient tensor, "
      "G\xe2\x82\x81\xe2\x82\x81 + G\xe2\x82\x82\xe2\x82\x82. "
      "This trace measures how the field direction converges or diverges "
      "perpendicular to B. The connection to |B| gradients is not "
      "fundamental; it comes from div B = 0, which "
      "relates field-direction convergence to field-strength variation. "
      "The \"asymmetric Larmor radius\" picture is a shorthand for what is "
      "really a net Lorentz force arising because b^ points in "
      "slightly different directions at different points around the orbit.");
    GAP(4);
    P("The linear grad-B model isolates this drift: straight field lines "
      "(no curvature) with |B| varying linearly in y.");
    GAP(8); RULE(); GAP(4);

    H("MIRROR REFLECTION");
    P("Conventional explanation (Northrop 1963; Kulsrud 2005): "
      "Conservation of \xce\xbc requires v_perp to increase as "
      "|B| grows. Since kinetic energy is conserved, v\xe2\x80\x96 must "
      "decrease. If |B| grows enough, v\xe2\x80\x96 reaches zero and the "
      "particle reflects:");
    M("  B_mirror = B\xe2\x82\x80 / sin\xc2\xb2(\xce\xb1\xe2\x82\x80)");
    GAP(4);
    P("The paper's approach: The parallel momentum p\xe2\x80\x96 has no "
      "Lorentz force component along B (v \xc3\x97 B is always "
      "perpendicular to B). Changes in p\xe2\x80\x96 come entirely from "
      "the rotation of b^: the field direction is slightly "
      "different at each point around the orbit, so the perpendicular "
      "Lorentz forces at each gyrophase do not cancel along the mean "
      "field direction. The secular rate of change is:");
    M("  dp\xe2\x80\x96/dt = (1/2) m v_perp\xc2\xb2 (G\xe2\x82\x81\xe2\x82\x81 + G\xe2\x82\x82\xe2\x82\x82)");
    GAP(4);
    P("This is a pure field-direction effect: the trace "
      "G\xe2\x82\x81\xe2\x82\x81 + G\xe2\x82\x82\xe2\x82\x82 measures "
      "how b^ converges or diverges around the orbit. "
      "The familiar form "
      "dp\xe2\x80\x96/dt = \xe2\x88\x92\xce\xbc grad\xe2\x80\x96 B "
      "follows only after invoking div B = 0, "
      "which relates the direction changes to a magnitude gradient. "
      "From this perspective, \xce\xbc conservation is a consequence of "
      "the field-direction geometry, not the starting point. A field "
      "that violates div B = 0 (model 4, "
      "\"Nonphysical\") has no direction changes along the axis and "
      "therefore no mirror force, even though |B| varies.");
    GAP(4);
    P("Particles with pitch angles near 90\xc2\xb0 reflect sooner. Those "
      "with small pitch angles can escape; in a dipole, these precipitate "
      "into the atmosphere. The magnetic bottle and dipole models both "
      "demonstrate this.");
    GAP(8); RULE(); GAP(4);

    H("FIELD LINES: A VISUAL AID");
    P("The field lines drawn in the 3D view are visualization aids, not "
      "physical objects. A charged particle does not \"follow\" a field "
      "line; it gyrates around the local field direction with a finite "
      "Larmor radius. The guiding center approximately follows the field "
      "line, but drifts across it due to gradients and curvature.");
    GAP(4);
    P("The distinction matters: the drift velocity is perpendicular to B, "
      "so the guiding center moves off the field line it started on. "
      "Field lines are useful for visualizing the geometry of B, but "
      "the physics comes from the Lorentz force, not from any property "
      "intrinsic to the lines themselves.");
    GAP(8); RULE(); GAP(4);

    H("THE G_ij TENSOR");
    P("The field-direction gradient tensor "
      "G_ij = e_i \xc2\xb7 (e_j \xc2\xb7 grad) b^ "
      "encodes how b^ rotates when one moves in each direction. "
      "It is expressed in the field-aligned triad "
      "(e\xe2\x82\x81 = curvature, e\xe2\x82\x82 = binormal, "
      "e\xe2\x82\x83 = b^). "
      "Press G to see it evolve in real time along the orbit.");
    GAP(4);
    P("The perpendicular-block trace "
      "G\xe2\x82\x81\xe2\x82\x81 + G\xe2\x82\x82\xe2\x82\x82 "
      "drives the secular mirror force and, through "
      "div B = 0, equals "
      "\xe2\x88\x92grad\xe2\x80\x96 B / B. "
      "The traceless part "
      "(G\xe2\x82\x81\xe2\x82\x81 \xe2\x88\x92 G\xe2\x82\x82\xe2\x82\x82 "
      "and G\xe2\x82\x81\xe2\x82\x82) "
      "produces a bounded oscillation at twice the gyrofrequency, "
      "averaged away in standard guiding-center theory. "
      "The off-diagonal components G\xe2\x82\x81\xe2\x82\x83 and "
      "G\xe2\x82\x82\xe2\x82\x83 encode the curvature itself.");
    GAP(4);
    P("This decomposition parallels the tidal tensor in general "
      "relativity: the trace (Ricci-like) part is fixed by a divergence "
      "constraint, while the traceless (Weyl-like) part is free. "
      "Littlejohn (1983) developed the Lagrangian guiding-center theory; "
      "Brizard and Hahm (2007) review the modern nonlinear gyrokinetic "
      "framework.");
    GAP(8); RULE(); GAP(4);

    H("THE BORIS ALGORITHM");
    P("The simulation uses the Boris pusher, a symplectic leapfrog "
      "integrator that exactly preserves speed during the magnetic "
      "rotation step:");
    GAP(2);
    M("  1. Half electric-field kick");
    M("  2. Boris magnetic rotation (exact angle)");
    M("  3. Half electric-field kick");
    GAP(4);
    P("In pure magnetic fields (no E), energy conservation is exact to "
      "machine precision: |dE|/E ~ 10^-15.");
    GAP(8); RULE(); GAP(4);

    H("RADIATION AND RELATIVISTIC EFFECTS");
    P("Accelerating charges radiate (Larmor formula: "
      "P = q^2 a^2 / (6 pi eps_0 c^3)). The Radiation toggle enables "
      "this loss; the Rad x slider applies a multiplier (up to 10^12) "
      "to make the effect visible on short timescales.");
    GAP(4);
    P("The relativistic Boris pusher works with 4-momentum u = gamma v. "
      "At high energy the gyrofrequency decreases and the Larmor radius "
      "grows visibly. The Lienard formula replaces Larmor for radiation "
      "at relativistic speeds.");
    GAP(8); RULE(); GAP(4);

    H("REFERENCES");
    P("- H. Alfv\xc3\xa9n, Cosmical Electrodynamics (Clarendon Press, 1950).");
    P("- W. Baumjohann and R. A. Treumann, Basic Space Plasma Physics (Imperial College Press, 1996).");
    P("- P. M. Bellan, Fundamentals of Plasma Physics (Cambridge, 2006).");
    P("- A. J. Brizard and T. S. Hahm, \"Foundations of nonlinear gyrokinetic theory,\" Rev. Mod. Phys. 79, 421 (2007).");
    P("- J. Buchner and L. M. Zelenyi, \"Regular and chaotic charged particle motion in magnetotaillike field reversals,\" J. Geophys. Res. 94, 11821 (1989).");
    P("- C. M. Cully and E. F. Donovan, \"A derivation of the gradient drift based on energy conservation,\" Am. J. Phys. 67, 909 (1999).");
    P("- M. Kruskal, \"Elementary orbit and drift theory,\" Plasma Physics (IAEA, 1965).");
    P("- R. M. Kulsrud, Plasma Physics for Astrophysics (Princeton, 2005).");
    P("- R. G. Littlejohn, \"Variational principles of guiding centre motion,\" J. Plasma Phys. 29, 111 (1983).");
    P("- T. G. Northrop, The Adiabatic Motion of Charged Particles (Interscience, 1963).");
    P("- C. Stormer, \"Sur les trajectoires des corpuscules electrises,\" Arch. Sci. Phys. Nat. 24, 5 (1907).");
}

/* ================================================================
 *  TAB 2: FIELD MODELS
 * ================================================================ */
static void tab_fields(Clay_Color hc, Clay_Color bc, Clay_Color mc, Clay_Color dc)
{
    H("0. CIRCULAR B  (constant |B|, pure curvature)");
    M("  B = B\xe2\x82\x80 (-y, x, 0) / \xe2\x88\x9a(x\xc2\xb2 + y\xc2\xb2)");
    P("An azimuthal field whose magnitude is constant everywhere. Since "
      "|B| is uniform there is no grad-B drift: only curvature drift. "
      "This is the cleanest demonstration of pure curvature drift and "
      "reproduces the paper's main simulation. Parameters: B0.");
    GAP(6); RULE(); GAP(4);

    H("1. LINEAR GRAD-B  (pure grad-B drift)");
    M("  B = B\xe2\x82\x80 (1 + \xce\xbb y) \xc3\xaa_z");
    P("A z-directed field whose magnitude varies linearly with y. "
      "Satisfies div B = 0. Straight field lines, so there is no "
      "curvature drift: only grad-B drift. Parameters: B0, lambda.");
    GAP(6); RULE(); GAP(4);

    H("2. QUADRATIC GRAD-B");
    M("  B = B\xe2\x82\x80 (1 + \xce\xbb y\xc2\xb2) \xc3\xaa_z");
    P("Higher-order (quadratic) gradient of |B|. Satisfies div B = 0. "
      "Shows how drift changes when the gradient itself varies in space. "
      "Parameters: B0, lambda.");
    GAP(6); RULE(); GAP(4);

    H("3. SINUSOIDAL GRAD-B");
    M("  B = B\xe2\x82\x80 (1 + a sin(ky)) \xc3\xaa_z");
    P("Periodic modulation of field strength. Satisfies div B = 0. "
      "The particle drifts through alternating regions of stronger and "
      "weaker field. Parameters: B0, amplitude a, wavenumber k.");
    GAP(6); RULE(); GAP(4);

    H("4. NONPHYSICAL  (div B != 0)");
    M("  B = (B\xe2\x82\x80 + \xce\xbb z) \xc3\xaa_z");
    P("Deliberately violates Maxwell's equations (div B = 0 is not "
      "satisfied). Problem 4 in the paper. The Boris pusher still "
      "conserves energy, but the drift behavior is qualitatively wrong. "
      "This illustrates why physical fields matter: the conventional "
      "drift formulas implicitly rely on div B = 0, and the Lorentz "
      "force analysis makes the role of this constraint explicit. "
      "Parameters: B0, lambda.");
    GAP(6); RULE(); GAP(4);

    H("5. MAGNETIC DIPOLE  (Earth-like field)");
    M("  B = (M / r\xc2\xb3) [3(\xc3\xa2\xe2\x82\x98 \xc2\xb7 \xc3\xa2\xe2\x82\x99) \xc3\xa2\xe2\x82\x99 - \xc3\xa2\xe2\x82\x98]");
    P("Realistic dipole field, the geometry studied by Stormer (1907). "
      "Combined curvature and grad-B drift produces azimuthal motion "
      "(the ring current). Mirror reflection near the poles traps "
      "particles in the radiation belts (Northrop 1963). Set pitch "
      "angle near 90 deg to see trapping; near 0 deg to see "
      "precipitation along field lines into the atmosphere. "
      "Parameters: dipole moment M.");
    GAP(6); RULE(); GAP(4);

    H("6. MAGNETIC BOTTLE  (mirror machine)");
    M("  Bz = B\xe2\x82\x80 (1 + z\xc2\xb2/Lm\xc2\xb2)");
    M("  Br = -B\xe2\x82\x80 r z / Lm\xc2\xb2   (from div B = 0)");
    P("A cylindrical mirror field. Particles with sufficient pitch angle "
      "reflect between the two high-field regions at large |z|. The "
      "radial component is required by div B = 0. On model selection, "
      "the particle is automatically placed one Larmor radius from the "
      "axis. Parameters: B0, mirror length Lm.");
    GAP(6); RULE(); GAP(4);

    H("7. TOKAMAK  (toroidal confinement)");
    M("  B\xcf\x86 = B\xe2\x82\x80 R\xe2\x82\x80 / R   (toroidal)");
    M("  Poloidal field from safety factor q");
    P("A large-aspect-ratio tokamak with circular flux surfaces. The 1/R "
      "toroidal field produces vertical curvature+grad-B drift. The "
      "poloidal field (controlled by q) twists field lines around the "
      "torus and can produce banana orbits when the pitch angle is right. "
      "See Kulsrud (2005) for the connection between single-particle "
      "drifts and plasma confinement. Parameters: B0, major radius R0, "
      "safety factor q, minor radius a.");
    GAP(6); RULE(); GAP(4);

    H("8. STELLARATOR  (near-axis quasi-axisymmetric)");
    M("  Near-axis expansion (Landreman-Sengupta / Landreman-Paul)");
    P("A 3D quasi-axisymmetric stellarator field computed via the "
      "near-axis expansion. Two configurations: r2 section 5.1 (QA) "
      "and Precise QA (LP22). The axis geometry and field shape update "
      "dynamically with R0_phys. Parameters: config preset, R0_phys, "
      "B0_phys, minor radius a.");
    GAP(6); RULE(); GAP(4);

    H("9. TORUS  (toroidal solenoid)");
    M("  B\xcf\x86 = B\xe2\x82\x80 R\xe2\x82\x80 / R   (inside torus, 0 outside)");
    P("A pure toroidal field. The 1/R dependence gives vertical "
      "curvature+grad-B drift that eventually carries the particle out "
      "of confinement. This demonstrates why a poloidal field component "
      "(as in a tokamak) is needed to confine plasma toroidally. "
      "Parameters: B0, major radius R0, minor radius a.");
}

/* ================================================================
 *  TAB 3: INTERFACE
 * ================================================================ */
static void tab_interface(AppState *app, Clay_Color hc, Clay_Color bc, Clay_Color mc, Clay_Color dc)
{
    /* Run Tutorial button */
    Clay_ElementId tutid = CLAY_ID("RunTutorialBtn");
    int thov = Clay_Hovered();
    CLAY(tutid, {
        .layout = { .sizing = { CLAY_SIZING_FIT(.min = 160), CLAY_SIZING_FIXED(32) },
                     .childAlignment = { .x = CLAY_ALIGN_X_CENTER, .y = CLAY_ALIGN_Y_CENTER },
                     .padding = {12, 12, 0, 0} },
        .backgroundColor = thov ? (Clay_Color){60, 100, 180, 255} : (Clay_Color){40, 80, 160, 255},
        .cornerRadius = CLAY_CORNER_RADIUS(4)
    }) {
        CLAY_TEXT(S(TR(STR_HELP_RUN_TUTORIAL)), CLAY_TEXT_CONFIG({
            .fontId = FONT_UI, .fontSize = 14, .letterSpacing = 1,
            .textColor = (Clay_Color){255, 255, 255, 255} }));
    }
    if (Clay_PointerOver(tutid) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        app->show_help = 0;
        app->tutorial.active = 1;
        app->tutorial.step = -1;
    }
    P("Walk through the interface and controls step by step.");
    GAP(6); RULE(); GAP(4);

#ifdef __APPLE__
    H("FONT SIZE");
    P("Cmd+Plus / Cmd+Minus adjusts the font size for the UI panel, "
      "help overlay, and telemetry plots independently depending on "
      "which area the mouse is over. Cmd+0 resets to default. "
      "All font sizes are saved between sessions.");
#else
    H("FONT SIZE");
    P("Ctrl+Plus / Ctrl+Minus adjusts the font size for the UI panel, "
      "help overlay, and telemetry plots independently depending on "
      "which area the mouse is over. Ctrl+0 resets to default. "
      "All font sizes are saved between sessions.");
#endif
    GAP(6); RULE(); GAP(4);

    H("THE UI PANEL  (U to toggle)");
    P("The side panel has four collapsible sections. Click a section "
      "header to expand or collapse it.");
    GAP(6); RULE(); GAP(4);

    H("MODEL / PARTICLE");
    P("Field Model dropdown: choose from 10 magnetic field geometries. "
      "Each model has adjustable parameters via sliders. Species: "
      "Electron, Proton, Alpha, O+, or Custom (set charge, mass, speed "
      "directly). Energy: kinetic energy in keV with range selector "
      "(x1/x10/x100). Pitch angle: angle between initial velocity and B "
      "(0 = parallel, 90 = perpendicular).");
    GAP(6); RULE(); GAP(4);

    H("PLAYBACK / RECORD");
    P("Speed range (x0.1 to x100) and fine speed slider. Pause/Resume "
      "and Reset buttons. The Trigger rec toggle arms recording mode.");
    GAP(6); RULE(); GAP(4);

    H("RECORDING");
    P("To record a video:");
    GAP(2);
    ML("  1. Toggle \"Trigger rec\" to arm. \"ARMED\" appears top-right.\n"
       "  2. Press R to start recording. The particle resets.\n"
       "     The UI hides so it won't appear in the video.\n"
       "  3. Press R again to stop and save.\n"
       "     The UI reappears. The particle resets.");
    GAP(4);
    P("Output: H.264 mp4 at 60 fps, at screen resolution. Each recording "
      "also produces an event log (.txt) that captures the complete initial "
      "state and a frame-by-frame log of every parameter change, enabling "
      "exact playback reproduction.");
    GAP(4);
    H("Recording directory:");
    ML("  macOS:   ~/Movies/Lorentz_Tracer/\n"
       "  Linux:   ~/Videos/Lorentz_Tracer/\n"
       "  Windows: %USERPROFILE%\\Videos\\Lorentz_Tracer\\");
    GAP(4);
    P("Filename format: YYYY-MM-DD_HHMMSS_MODEL.mp4 (e.g. "
      "2026-03-26_143022_DIP.mp4). The companion event log has the "
      "same name with .txt extension.");
    GAP(6); RULE(); GAP(4);

    H("DISPLAY");
    P("Field lines: magnetic field line visualization. GC fl: instantaneous "
      "guiding-center field line through the particle's GC. Vel/B vectors: "
      "arrows at the particle. G_ij: field-direction gradient tensor. "
      "Axes: coordinate axis markers. Plot range: time window for 2D "
      "diagnostic plots. Autoscale: auto-range pitch angle plot. "
      "Radiation: Larmor radiation damping with adjustable multiplier. "
      "Relativistic: use the relativistic Boris pusher. Follow: camera "
      "tracks the particle (scroll to adjust distance). Camera view: "
      "Free / +b / -b (look along or against B). Dark/Light theme. "
      "Stereo 3D: side-by-side stereoscopic rendering.");
    GAP(6); RULE(); GAP(4);

    H("READOUTS");
    P("t: simulation time. |dE|/E: fractional energy error. pitch: "
      "instantaneous pitch angle. mu: magnetic moment. |B|: field "
      "magnitude at particle position. Position (x,y,z) and distances "
      "(r, rho). When radiation is enabled: kinetic energy, radiated "
      "power, effective energy loss rate.");
    GAP(6); RULE(); GAP(4);

    H("PARTICLE PLACEMENT");
    P("Hold Left Shift and move the mouse over the 3D scene to preview "
      "where the particle will be placed. A preview field line is drawn. "
      "Click to place. The algorithm picks the point along the line of "
      "sight with the strongest |B| (or closest to the camera target in "
      "uniform fields).");
    GAP(6); RULE(); GAP(4);

    H("STATE PERSISTENCE");
    P("All settings are saved on exit and restored on launch.");
    ML("\xc2\xa0\xc2\xa0macOS:\xc2\xa0\xc2\xa0\xc2\xa0~/Library/Application\xc2\xa0Support/Lorentz_Tracer/state.ini");
    ML("\xc2\xa0\xc2\xa0Linux:\xc2\xa0\xc2\xa0\xc2\xa0~/.local/state/lorentz_tracer/state.ini");
    ML("\xc2\xa0\xc2\xa0Windows:\xc2\xa0%LOCALAPPDATA%\\Lorentz_Tracer\\state.ini");
}

/* ================================================================
 *  TAB 4: ABOUT
 * ================================================================ */
static void tab_about(AppState *app, Clay_Color hc, Clay_Color bc, Clay_Color mc, Clay_Color dc)
{
    H("LORENTZ TRACER");
    P("Companion software for the American Journal of Physics paper "
      "\"What causes the magnetic curvature drift?\" by Johnathan K. "
      "Burchill, University of Calgary.");
    GAP(4);
    P("The paper derives the grad-curvature drift directly from the "
      "Lorentz force equation, exposing the field-direction gradient "
      "tensor G_ij and its trace/traceless decomposition (a tidal "
      "analogy with general relativity). Lorentz Tracer makes these "
      "ideas tangible: watch the drift happen, see G_ij evolve, and "
      "vary every parameter in real time.");
    GAP(8); RULE(); GAP(4);

    H("DEVELOPMENT");
    P("What started as \"just make the video from the paper interactive\" "
      "became a complete charged-particle visualization laboratory. Every "
      "feature was driven by a concrete pedagogical question.");
    GAP(4);
    P("\"Can the student see the curvature drift?\" led to the circular "
      "field model, where |B| is constant and the drift is pure curvature. "
      "\"Can they see mirror reflection?\" led to the magnetic bottle. "
      "\"What does a realistic dipole look like?\" led to the dipole model. "
      "\"What does G_ij actually look like along an orbit?\" led to the "
      "real-time tensor display.");
    GAP(8); RULE(); GAP(4);

    H("TRIALS AND DETOURS");
    P("The Boris pusher was the foundation. Porting it from the paper's "
      "simulation code and validating |dE|/E ~ 10^-15 was the first "
      "milestone. Getting 256 steps per gyroperiod right matters: too "
      "few and the trail looks jagged, too many and the simulation crawls.");
    GAP(4);
    P("The near-axis stellarator was the most technically demanding field. "
      "The sigma ODE for magnetic shaping must be solved iteratively "
      "(it converges in about 5 iterations). A memorable debugging "
      "session: the rotational transform iota was consistently half the "
      "expected value. Both configurations showed the same factor-of-two "
      "error. The culprit: a single missing nfp factor in a normalizing "
      "expression: L*nfp/(2*pi), not L/(2*pi). One missed multiplication.");
    GAP(4);
    P("The camera system went through three redesigns. The first used "
      "raylib's built-in orbit camera, which worked until field-aligned "
      "mode was added. The second tried to fix the reset by projecting "
      "z onto the screen plane; it worked initially but failed after "
      "exiting field-aligned mode. The third abandoned cleverness and "
      "simply resets to the model's default view, with smooth fly-to "
      "transitions via spherical interpolation (slerp) for the up vector.");
    GAP(4);
    P("Stereoscopic 3D required bypassing raylib's BeginMode3D entirely. "
      "BeginMode3D computes its projection matrix from the full "
      "framebuffer width, ignoring per-eye viewports. The first attempt "
      "rendered a shrunken scene in the lower-left corner. The fix: set "
      "up projection and view matrices manually via rlgl (rlFrustum, "
      "MatrixLookAt), with HiDPI-aware viewport coordinates.");
    GAP(4);
    P("The recording system started as an ffmpeg pipe. The current "
      "version links directly against libav for H.264 encoding. Each "
      "recording gets a companion event log that captures every parameter "
      "change frame-by-frame, enabling bit-exact playback reproduction.");
    GAP(4);
    P("The UI migrated from raygui (immediate-mode widgets) to clay.h "
      "(a declarative layout engine) to get automatic text wrapping and "
      "scroll containers. Clay computes flexbox-style layout; custom "
      "widgets (sliders, toggles, dropdowns) are built on top. This "
      "help system is the direct beneficiary.");
    GAP(8); RULE(); GAP(4);

    H("TECHNICAL FOUNDATION");
    P("Written in C99 with raylib 5.5 for graphics and clay.h for UI "
      "layout. Video encoding via libav (libavcodec, libavformat, "
      "libavutil, libswscale). Builds on macOS, Linux, and Windows "
      "via CMake with FetchContent for raylib.");
    GAP(8); RULE(); GAP(4);

    H("LICENSE");
    P("MIT License");
    GAP(4);
    P("Copyright (c) 2026 Johnathan K. Burchill");
    GAP(4);
    P("Permission is hereby granted, free of charge, to any person obtaining "
      "a copy of this software and associated documentation files (the "
      "\"Software\"), to deal in the Software without restriction, including "
      "without limitation the rights to use, copy, modify, merge, publish, "
      "distribute, sublicense, and/or sell copies of the Software, and to "
      "permit persons to whom the Software is furnished to do so, subject to "
      "the following conditions:");
    GAP(4);
    P("The above copyright notice and this permission notice shall be "
      "included in all copies or substantial portions of the Software.");
    GAP(4);
    P("THE SOFTWARE IS PROVIDED \"AS IS\", WITHOUT WARRANTY OF ANY KIND, "
      "EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF "
      "MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. "
      "IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY "
      "CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, "
      "TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE "
      "SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.");
    GAP(12); RULE(); GAP(8);

    /* Factory reset button */
    Clay_ElementId rid = CLAY_ID("FactoryResetBtn");
    int hover = Clay_Hovered();
    CLAY(rid, {
        .layout = { .sizing = { CLAY_SIZING_FIT(.min = 160), CLAY_SIZING_FIXED(32) },
                     .childAlignment = { .x = CLAY_ALIGN_X_CENTER, .y = CLAY_ALIGN_Y_CENTER },
                     .padding = {12, 12, 0, 0} },
        .backgroundColor = hover ? (Clay_Color){180, 60, 60, 255} : (Clay_Color){140, 40, 40, 255},
        .cornerRadius = CLAY_CORNER_RADIUS(4)
    }) {
        CLAY_TEXT(S(TR(STR_HELP_FACTORY_RESET)), CLAY_TEXT_CONFIG({
            .fontId = FONT_UI, .fontSize = 14, .letterSpacing = 1,
            .textColor = (Clay_Color){255, 255, 255, 255} }));
    }
    if (Clay_PointerOver(rid) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
        app->factory_reset = 1;
    P("Removes saved settings and restores all defaults.");

}

/* ================================================================
 *  MAIN HELP LAYOUT
 * ================================================================ */

void help_declare_layout(AppState *app)
{
    g_help_zoom = app->help_zoom;
    Clay_Color overlay_c = {0, 0, 0, 180};
    Clay_Color panel_c = app->dark_mode
        ? (Clay_Color){22, 22, 30, 248} : (Clay_Color){242, 242, 248, 248};
    Clay_Color hc = {(float)app->theme.text.r, (float)app->theme.text.g,
                     (float)app->theme.text.b, 255};
    Clay_Color bc = hc;
    bc.a = 220;
    Clay_Color mc = {(float)app->theme.text.r * 0.85f, (float)app->theme.text.g * 0.85f,
                     (float)app->theme.text.b * 0.85f, 230};
    Clay_Color dc = {(float)app->theme.text_dim.r, (float)app->theme.text_dim.g,
                     (float)app->theme.text_dim.b, 80};
    Clay_Color accent = {(float)app->theme.trail.r, (float)app->theme.trail.g,
                         (float)app->theme.trail.b, 255};
    Clay_Color accent_bg = accent; accent_bg.a = 50;
    Clay_Color tab_hover = app->dark_mode
        ? (Clay_Color){40, 40, 50, 255} : (Clay_Color){225, 225, 235, 255};

    float pw = app->win_w - 40.0f;
    if (pw > 1400) pw = 1400;
    float ph = app->win_h - 40.0f;

    /* Full-screen overlay */
    CLAY(CLAY_ID("HelpRoot"), {
        .layout = { .sizing = { CLAY_SIZING_GROW(0), CLAY_SIZING_GROW(0) },
                     .childAlignment = { .x = CLAY_ALIGN_X_CENTER, .y = CLAY_ALIGN_Y_CENTER } },
        .backgroundColor = overlay_c
    }) {
        /* Centered panel */
        CLAY(CLAY_ID("HelpPanel"), {
            .layout = { .layoutDirection = CLAY_TOP_TO_BOTTOM,
                         .sizing = { CLAY_SIZING_FIXED(pw), CLAY_SIZING_FIXED(ph) },
                         .padding = {16, 16, 12, 12}, .childGap = 6 },
            .backgroundColor = panel_c,
            .cornerRadius = CLAY_CORNER_RADIUS(6)
        }) {
            /* Title */
            CLAY_TEXT(S(TR(STR_HELP_TITLE)), CLAY_TEXT_CONFIG({
                .fontId = FONT_UI, .fontSize = (uint16_t)(22 * g_help_zoom),
                .letterSpacing = 1, .textColor = hc }));

            /* Tab bar */
            CLAY(CLAY_ID("HTabBar"), {
                .layout = { .childGap = 4, .sizing = { CLAY_SIZING_GROW(0), CLAY_SIZING_FIT(0) } }
            }) {
                const char *names[] = {TR(STR_HELP_TAB_KEYS), TR(STR_HELP_TAB_PHYSICS),
                    TR(STR_HELP_TAB_FIELDS), TR(STR_HELP_TAB_INTERFACE), TR(STR_HELP_TAB_ABOUT)};
                for (int i = 0; i < 5; i++) {
                    Clay_ElementId tid = CLAY_IDI("HTab", i);
                    int active = (i == app->help_tab);
                    CLAY(tid, {
                        .layout = { .sizing = { CLAY_SIZING_GROW(0), CLAY_SIZING_FIXED(34) },
                                     .childAlignment = { .x = CLAY_ALIGN_X_CENTER,
                                                          .y = CLAY_ALIGN_Y_CENTER } },
                        .backgroundColor = active ? accent_bg :
                            (Clay_Hovered() ? tab_hover : (Clay_Color){0,0,0,0}),
                        .border = active
                            ? (Clay_BorderElementConfig){ .width = { .bottom = 2 }, .color = accent }
                            : (Clay_BorderElementConfig){0},
                        .cornerRadius = CLAY_CORNER_RADIUS(3)
                    }) {
                        Clay_Color tc = active ? accent : hc;
                        tc.a = active ? 255 : 180;
                        Clay_String tstr = { .length = (int32_t)strlen(names[i]), .chars = names[i] };
                        Clay_TextElementConfig tcfg = { .fontId = FONT_UI,
                            .fontSize = (uint16_t)(16 * g_help_zoom),
                            .letterSpacing = 1, .textColor = tc };
                        CLAY_TEXT(tstr, CLAY_TEXT_CONFIG(tcfg));
                    }
                    if (Clay_PointerOver(tid) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
                        app->help_tab = i;
                }
            }

            /* Separator under tabs */
            CLAY_AUTO_ID({
                .layout = { .sizing = { CLAY_SIZING_GROW(0), CLAY_SIZING_FIXED(1) } },
                .backgroundColor = dc
            }) {}

            /* Scrollable content area (unique ID per tab for independent scroll) */
            CLAY(CLAY_IDI("HContent", app->help_tab), {
                .layout = { .layoutDirection = CLAY_TOP_TO_BOTTOM,
                             .sizing = { CLAY_SIZING_GROW(0), CLAY_SIZING_GROW(0) },
                             .padding = {0, 0, 4, 4}, .childGap = 3 },
                .clip = { .vertical = true, .childOffset = Clay_GetScrollOffset() }
            }) {
                switch (app->help_tab) {
                case 0: tab_keys(hc, bc, mc, dc); break;
                case 1: tab_physics(hc, bc, mc, dc); break;
                case 2: tab_fields(hc, bc, mc, dc); break;
                case 3: tab_interface(app, hc, bc, mc, dc); break;
                case 4: tab_about(app, hc, bc, mc, dc); break;
                }
            }

            /* Scroll indicator */
            {
                Clay_ElementId cid = CLAY_IDI("HContent", app->help_tab);
                Clay_ScrollContainerData sd = Clay_GetScrollContainerData(cid);
                if (sd.found && sd.contentDimensions.height > sd.scrollContainerDimensions.height) {
                    float ratio = sd.scrollContainerDimensions.height / sd.contentDimensions.height;
                    float thumb_h = ratio * sd.scrollContainerDimensions.height;
                    if (thumb_h < 20) thumb_h = 20;
                    float scroll_frac = (sd.scrollPosition->y != 0)
                        ? -sd.scrollPosition->y /
                          (sd.contentDimensions.height - sd.scrollContainerDimensions.height)
                        : 0;
                    Clay_Color bar_c = dc; bar_c.a = 120;
                    CLAY(CLAY_ID("HScrollBar"), {
                        .floating = {
                            .attachTo = CLAY_ATTACH_TO_ELEMENT_WITH_ID,
                            .parentId = cid.id, .zIndex = 1,
                            .offset = { .y = scroll_frac *
                                (sd.scrollContainerDimensions.height - thumb_h) },
                            .attachPoints = { CLAY_ATTACH_POINT_RIGHT_TOP,
                                              CLAY_ATTACH_POINT_RIGHT_TOP }
                        }
                    }) {
                        CLAY(CLAY_ID("HScrollThumb"), {
                            .layout = { .sizing = { CLAY_SIZING_FIXED(4),
                                CLAY_SIZING_FIXED(thumb_h) } },
                            .backgroundColor = bar_c,
                            .cornerRadius = CLAY_CORNER_RADIUS(2)
                        }) {}
                    }
                }
            }

            /* Footer */
            CLAY_AUTO_ID({
                .layout = { .sizing = { CLAY_SIZING_GROW(0), CLAY_SIZING_FIXED(1) } },
                .backgroundColor = dc
            }) {}
            CLAY_TEXT(S(TR(STR_HELP_FOOTER)),
                CLAY_TEXT_CONFIG({ .fontId = FONT_UI, .fontSize = 14, .letterSpacing = 1,
                    .textColor = dc }));
        }
    }
}
