# Lorentz Tracer

Interactive charged-particle orbit simulation. Solves the full Lorentz force equation in real time across thirteen magnetic field models, with 3D visualization and live diagnostics.

[Try the web demo](https://johnathanburchill.github.io/lorentz_tracer/)

## Features

- Boris integrator (energy-conserving to machine precision)
- Thirteen magnetic field models: circular (constant |B|), linear/quadratic/sinusoidal grad-B, nonphysical (div B != 0), magnetic dipole, magnetic bottle, tokamak, stellarator (near-axis), torus, Harris current sheet, isotropic turbulence (power-law spectrum), Earth's magnetosphere (IGRF-14 + Tsyganenko T89c inside a Shue magnetopause, uniform IMF outside, in Earth radii)
- Multiple particles with independent species (charge, mass)
- 3D orbit visualization with field lines and particle trails
- Live diagnostic plots: pitch angle, magnetic moment, energy, position
- Event recording and playback
- Explorer: guided scenario mini-tutorials for classroom demonstration
- Dark and light themes with customizable colors
- Localized in 13 languages

## Build

Requires CMake 3.20+ and a C99 compiler. Uses an installed raylib 6.0+ if CMake can find one; otherwise raylib 6.2 is downloaded automatically.

```sh
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

On macOS this produces a `.app` bundle in `build/`. On Linux and Windows it produces a standalone executable.

Cross-compilation (macOS host):

```sh
./build-cross.sh all      # builds macOS, Windows (mingw-w64), Linux (Docker)
```

WebAssembly (requires Emscripten):

```sh
emcmake cmake -B build-wasm -DCMAKE_BUILD_TYPE=Release
cmake --build build-wasm
```

## On a beamer slide

The beameup presenter console can run Lorentz Tracer live in a box on a slide, where it takes the mouse and keys while the pointer is over the box. Build it with the option:

```sh
cmake -B build -DCMAKE_BUILD_TYPE=Release -DBEAMEUP_EMBED=ON
cmake --build build
```

The slide links a script, and the script starts the program:

```latex
\href{run:lorentz_tracer.sh?app}{\includegraphics[width=\textwidth]{lorentz-poster}}
```

```sh
cd ~/src/lorentz_tracer && exec build/lorentz_tracer.app/Contents/MacOS/lorentz_tracer
```

On the slide the window is hidden and sized to the box, there is no splash, and Cmd+F does nothing. It shows the settings the app was last closed with, and leaves them as they were. Run outside beameup, the same build behaves as usual.

The option is off by default because it compiles in `third_party/beameup_embed.h`, beameup's header, which is GPL-3.0-or-later: a binary built with it carries GPL code. The copy must match the version of beameup that runs it. It needs POSIX shared memory, so it does nothing on Windows or the web.

## Acknowledgments

The near-axis stellarator field uses configurations and the expansion method from:

- M. Landreman and W. Sengupta, "Direct construction of optimized stellarator shapes. Part 1," J. Plasma Phys. **85**, 905850103 (2019).
- M. Landreman and E. J. Paul, "Magnetic fields with precise quasisymmetry for plasma confinement," Phys. Rev. Lett. **128**, 035001 (2022).

The implementation follows [pyQSC](https://github.com/landreman/pyQSC).

## License

MIT. See [LICENSE](LICENSE).
