#!/bin/bash
# Cross-compile lorentz_tracer for Windows, Linux, and macOS.
# Usage: ./build-cross.sh [windows|linux|macos|all]
#
# Requires:
#   Windows: brew install mingw-w64
#   Linux:   docker (Docker Desktop or colima)
#   macOS:   Xcode command line tools

set -e
cd "$(dirname "$0")"

TARGET="${1:-all}"

build_windows() {
    echo "=== Building Windows x86_64 (mingw-w64) ==="
    cmake -B build-win64 \
        -DCMAKE_TOOLCHAIN_FILE=cmake/mingw-w64.cmake \
        -DCMAKE_BUILD_TYPE=Release
    cmake --build build-win64 -j$(sysctl -n hw.ncpu)

    # Package into a zip: user extracts folder, double-clicks exe
    rm -rf dist/Lorentz_Tracer_Windows
    mkdir -p dist/Lorentz_Tracer_Windows
    cp build-win64/lorentz_tracer.exe dist/Lorentz_Tracer_Windows/
    cp -r resources dist/Lorentz_Tracer_Windows/
    cp LICENSE dist/Lorentz_Tracer_Windows/ 2>/dev/null || true
    (cd dist && zip -qr Lorentz_Tracer_Windows.zip Lorentz_Tracer_Windows/)
    echo "  -> dist/Lorentz_Tracer_Windows.zip"
}

build_linux() {
    echo "=== Building Linux x86_64 (Docker) ==="
    docker build --platform linux/amd64 \
        -f Dockerfile.linux -t lorentz_tracer_linux .

    # Extract and package as tarball
    rm -rf dist/Lorentz_Tracer_Linux
    mkdir -p dist/Lorentz_Tracer_Linux
    CONTAINER=$(docker create --platform linux/amd64 lorentz_tracer_linux)
    docker cp "$CONTAINER:/lorentz_tracer" dist/Lorentz_Tracer_Linux/lorentz_tracer
    docker cp "$CONTAINER:/resources" dist/Lorentz_Tracer_Linux/resources
    docker rm "$CONTAINER" >/dev/null
    cp LICENSE dist/Lorentz_Tracer_Linux/ 2>/dev/null || true
    chmod +x dist/Lorentz_Tracer_Linux/lorentz_tracer
    (cd dist && tar czf Lorentz_Tracer_Linux.tar.gz Lorentz_Tracer_Linux/)
    echo "  -> dist/Lorentz_Tracer_Linux.tar.gz"
}

build_macos() {
    echo "=== Building macOS (native) ==="
    cmake -B build-release -DCMAKE_BUILD_TYPE=Release
    cmake --build build-release -j$(sysctl -n hw.ncpu)

    # Package the .app bundle into a zip (preserves structure, no DMG needed)
    rm -rf dist/Lorentz_Tracer_macOS
    mkdir -p dist/Lorentz_Tracer_macOS
    cp -R build-release/lorentz_tracer.app dist/Lorentz_Tracer_macOS/
    cp LICENSE dist/Lorentz_Tracer_macOS/ 2>/dev/null || true
    (cd dist && zip -qr Lorentz_Tracer_macOS.zip Lorentz_Tracer_macOS/)
    echo "  -> dist/Lorentz_Tracer_macOS.zip"
}

case "$TARGET" in
    windows) build_windows ;;
    linux)   build_linux ;;
    macos)   build_macos ;;
    all)     build_macos; build_windows; build_linux ;;
    *)       echo "Usage: $0 [windows|linux|macos|all]"; exit 1 ;;
esac

echo "Done."
