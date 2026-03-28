# Translating Lorentz Tracer

Translations are welcome! The UI has been machine-translated into 11 languages
and reviewed for context errors, but native-speaker corrections are needed.

## Current languages (beta)

French, German, Spanish, Portuguese, Italian, Russian, Japanese,
Chinese (Simplified), Chinese (Traditional), Korean, Esperanto

Help content (Physics, Fields, Interface, About tabs) is English-only.

## How to contribute

1. Open `i18n/generated_tables.c`
2. Find the array for your language (e.g. `g_strings_fr`)
3. Fix any incorrect translations — common issues:
   - Physics terms translated as everyday words
   - Direction words (left/right/top/bottom) given wrong meaning
   - Verbs vs nouns (e.g. "Open" as adjective vs command)
4. Submit a pull request

## Context guide

These terms have specific meanings in the app:

| String | Context |
|--------|---------|
| Charge | Electric charge (coulombs) |
| Pitch | Pitch angle (velocity vs B-field direction) |
| Field | Magnetic field |
| Trail | Particle trajectory trace |
| Axes | Coordinate axes (x, y, z) |
| Plots | Telemetry charts/graphs |
| Scaled | Vectors drawn at physical magnitude |
| Follow | Camera tracks particle |
| Free | Unconstrained camera mode |
| Armed | Recording ready to trigger |
| Custom | User-configurable particle |
| Readouts | Live measurement display |

## Adding a new language

1. Add a `LangId` entry in `src/i18n.h`
2. Add metadata in `g_lang_info[]` in `src/i18n.c`
3. Add a `g_strings_XX` array in `i18n/generated_tables.c`
4. Wire it into `g_tables[]` in `src/i18n.c`

## Fun languages (stubs)

Klingon, Vulcan, Morse Code, Pirate, Leet Speak, Shakespearean,
and Old English are listed but have no translations yet. Creative
contributions welcome.
