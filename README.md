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
