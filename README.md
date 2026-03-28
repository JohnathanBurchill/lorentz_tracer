# Lorentz Tracer

Interactive charged-particle orbit simulation. Solves the full Lorentz force equation in real time across ten magnetic field models, with 3D visualization and live diagnostics.

Companion to: J. K. Burchill, "What causes the magnetic curvature drift?", submitted to American Journal of Physics (2026).

## Features

- Boris integrator (energy-conserving to machine precision)
- Ten magnetic field models: circular (constant |B|), linear/quadratic/sinusoidal grad-B, nonphysical (div B != 0), magnetic dipole, magnetic bottle, tokamak, stellarator (near-axis), torus
- Multiple particles with independent species (charge, mass)
- 3D orbit visualization with field lines and particle trails
- Live diagnostic plots: pitch angle, magnetic moment, energy, position
- Event recording and playback
- Explorer: guided scenario mini-tutorials for classroom demonstration
- Dark and light themes with customizable colors
- Localized in 13 languages

## Build

Requires CMake 3.20+ and a C99 compiler. raylib 5.5 is fetched automatically.

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

## Acknowledgments

The near-axis stellarator field uses configurations and the expansion method from:

- M. Landreman and W. Sengupta, "Direct construction of optimized stellarator shapes. Part 1," J. Plasma Phys. **85**, 905850103 (2019).
- M. Landreman and E. J. Paul, "Magnetic fields with precise quasisymmetry for plasma confinement," Phys. Rev. Lett. **128**, 035001 (2022).

The implementation follows [pyQSC](https://github.com/landreman/pyQSC).

## License

MIT. See [LICENSE](LICENSE).
