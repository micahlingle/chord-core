# chord-core

C++ library to analyze audio and provide chord names.

## Current Status

This repository currently provides:

- A C++17 CMake project
- WAV loading through `libsndfile`
- Block-based file processing with a default block size of 1024 samples
- RMS calculation and console output for each block
- RMS-based activity gating for quiet/noisy blocks
- FFTW-based frequency analysis with guitar-focused top-frequency reporting
- Stubbed library components for the later chroma and chord-detection milestones

![Chord Core Milestone 1 architecture](docs/chord-core-milestone1-architecture.svg)

## Dependencies

- CMake 3.16+
- A C++17 compiler
- `libsndfile`
- FFTW single-precision library (`fftw3f`)
- GoogleTest (only needed when `CHORD_CORE_BUILD_TESTS=ON`)

Examples:

- macOS (Homebrew): `brew install libsndfile fftw googletest`
- Ubuntu/Debian: `sudo apt-get install libsndfile1-dev libfftw3-dev libgtest-dev`

## Build

```bash
cmake -S . -B build -DCHORD_CORE_BUILD_TESTS=OFF
cmake --build build
```

## Run

```bash
./build/chord_core_demo /path/to/file.wav
```

The demo prints one line per 1024-sample RMS block. Blocks below the current activity threshold (`0.01` RMS) are marked inactive and do not report FFT peaks. Active-block frequency analysis uses a larger 16384-sample FFT window and reports peaks between 75 Hz and 5 kHz:

```text
Block 273 start=5.824s samples=1024 rms=0.016707 active=yes dominant_frequency_hz=164.06 top_frequencies_hz=[164.06,123.05,82.03]
```

## Test

The standard test workflow includes line and branch coverage:

```bash
cmake --fresh -S . -B build-coverage \
  -DCHORD_CORE_BUILD_TESTS=ON \
  -DCHORD_CORE_ENABLE_COVERAGE=ON \
  -DCMAKE_BUILD_TYPE=Debug

cmake --build build-coverage --target coverage
```

This runs the unit tests, prints a coverage summary, and writes an HTML report to `build-coverage/coverage.html`.

Optional dependency:

- macOS (Homebrew): `brew install gcovr`
- Ubuntu/Debian: `sudo apt-get install gcovr`

For a faster test-only build without coverage:

```bash
cmake -S . -B build -DCHORD_CORE_BUILD_TESTS=ON
cmake --build build
ctest --test-dir build --output-on-failure
```

## Coverage

The `coverage` target wraps the full coverage flow in one command after configuration:

```bash
cmake --build build-coverage --target coverage
```

Coverage must be generated from a build tree configured with `-DCHORD_CORE_ENABLE_COVERAGE=ON`.
On macOS/AppleClang, CMake configures `gcovr` to use `xcrun llvm-cov gcov` automatically.
