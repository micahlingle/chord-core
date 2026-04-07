# chord-core

C++ library to analyze audio and provide chord names.

## Milestone 1

This repository currently provides:

- A C++17 CMake project
- WAV loading through `libsndfile`
- Block-based file processing with a default block size of 1024 samples
- RMS calculation and console output for each block
- Stubbed library components for the later FFT, chroma, and chord-detection milestones

![Chord Core Milestone 1 architecture](docs/chord-core-milestone1-architecture.svg)

## Dependencies

- CMake 3.16+
- A C++17 compiler
- `libsndfile`
- GoogleTest (only needed when `CHORD_CORE_BUILD_TESTS=ON`)

Examples:

- macOS (Homebrew): `brew install libsndfile googletest`
- Ubuntu/Debian: `sudo apt-get install libsndfile1-dev libgtest-dev`

## Build

```bash
cmake -S . -B build -DCHORD_CORE_BUILD_TESTS=OFF
cmake --build build
```

## Run

```bash
./build/chord_core_demo /path/to/file.wav
```

## Test

```bash
cmake -S . -B build -DCHORD_CORE_BUILD_TESTS=ON
cmake --build build
ctest --test-dir build --output-on-failure
```

## Coverage

Line and branch coverage can be quantified with compiler coverage instrumentation plus `gcovr`.
Coverage must be generated from the `build-coverage` tree, not the regular `build` tree.

Optional dependency:

- macOS (Homebrew): `brew install gcovr`
- Ubuntu/Debian: `sudo apt-get install gcovr`

If you are using AppleClang on macOS, pass `--gcov-executable 'xcrun llvm-cov gcov'` so `gcovr` can find Apple's LLVM coverage tool and read Clang's coverage data correctly.

```bash
cmake --fresh -S . -B build-coverage \
  -DCHORD_CORE_BUILD_TESTS=ON \
  -DCHORD_CORE_ENABLE_COVERAGE=ON \
  -DCMAKE_BUILD_TYPE=Debug

cmake --build build-coverage
ctest --test-dir build-coverage --output-on-failure

gcovr --root . \
  --object-directory build-coverage \
  --filter 'src/.*' \
  --filter 'include/.*' \
  --exclude 'tests/.*' \
  --txt-metric branch \
  --gcov-executable 'xcrun llvm-cov gcov' \
  --print-summary
```

For an HTML report:

```bash
gcovr --root . \
  --object-directory build-coverage \
  --filter 'src/.*' \
  --filter 'include/.*' \
  --exclude 'tests/.*' \
  --txt-metric branch \
  --gcov-executable 'xcrun llvm-cov gcov' \
  --html-details build-coverage/coverage.html
```
