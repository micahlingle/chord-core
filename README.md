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
- Chroma extraction that maps FFT-bin energy into 12 normalized pitch classes
- Chord detection through the reusable `ChordDetector` API
- Temporal smoothing to reduce frame-to-frame chord jitter
- A verbose analyzer executable for DSP debugging and tuning
- Automatic dataset evaluation for labeled WAV regression checks
- A helper script to bootstrap label CSVs from demo output

![Chord Core Milestone 1 architecture](docs/chord-core-milestone1-architecture.svg)

## Dependencies

- CMake 3.16+
- A C++17 compiler
- `libsndfile`
- FFTW single-precision library (`fftw3f`)
- clang-format
- GoogleTest (only needed when `CHORD_CORE_BUILD_TESTS=ON`)

Examples:

- macOS (Homebrew): `brew install libsndfile fftw googletest clang-format`
- Ubuntu/Debian: `sudo apt-get install libsndfile1-dev libfftw3-dev libgtest-dev clang-format`

## Build

```bash
cmake -S . -B build -DCHORD_CORE_BUILD_TESTS=OFF
cmake --build build
```

If `datasets/` contains one or more `*.wav` files with matching `*.labels.csv` files, the normal build automatically runs `chord_core_eval` on each discovered pair and fails the build if any file falls below the configured minimum overlap accuracy.

## Run

Use `chord_core_demo` for the public API example. It loads a WAV file, feeds fixed-size blocks into `ChordDetector`, and prints the current chord result:

```bash
./build/chord_core_demo /path/to/file.wav
```

```text
Block 273 start=5.824s samples=1024 chord="E minor" confidence=0.79
```

Use `chord_core_analyze` when you need detailed DSP diagnostics:

```bash
./build/chord_core_analyze /path/to/file.wav
```

The analyzer prints one line per 1024-sample RMS block. Blocks below the current activity threshold (`0.01` RMS) are marked inactive and do not report FFT peaks or chroma energy. Active-block frequency analysis uses a larger 16384-sample FFT window, reports peaks between 75 Hz and 5 kHz, prints a normalized 12-bin chroma vector, and reports the best major/minor chord-template match:

```text
Block 273 start=5.824s samples=1024 rms=0.016707 active=yes dominant_frequency_hz=164.06 top_frequencies_hz=[164.06,123.05,82.03] chroma=[C=0.09,C#=0.16,D=0.39,D#=0.23,E=1.00,F=0.48,F#=0.25,G=0.55,G#=0.21,A=0.15,A#=0.07,B=0.78] chord="E minor" confidence=0.79
```

## Dataset Evaluation

For local regression analysis, place tracked dataset pairs in `datasets/` using this naming convention:

```text
datasets/<name>.wav
datasets/<name>.labels.csv
datasets/<name>.min_accuracy.txt   # optional per-file threshold override
```

Each label CSV should use:

```csv
start_seconds,end_seconds,chord
```

To generate a starter label CSV from the current detector output, use:

```bash
python3 scripts/generate_label_csv.py datasets/acoustic_mic_0.wav --demo build/chord_core_demo
```

This writes `datasets/acoustic_mic_0.labels.csv` by default. Treat that output as a draft: it is useful for bootstrapping labels quickly, but it should be reviewed and corrected by hand before using it as simulation-test ground truth.

The normal build automatically discovers every matching pair in `datasets/`, validates the CSV before evaluation, and runs the evaluator on each one:

```bash
cmake --fresh -S . -B build-eval \
  -DCHORD_CORE_BUILD_TESTS=ON \
  -DCHORD_CORE_EVAL_MIN_ACCURACY=0.70

cmake --build build-eval
```

The default required overlap accuracy is `0.70`. To override a specific dataset, add a sibling text file such as `datasets/acoustic_mic_0.min_accuracy.txt` containing just one number in `[0, 1]`, for example:

```text
0.80
```

You can also run the evaluator directly and optionally dump predicted spans back to CSV for side-by-side inspection:

```bash
./build-eval/chord_core_eval \
  --wav ./datasets/acoustic_mic_0.wav \
  --labels ./datasets/acoustic_mic_0.labels.csv \
  --min-accuracy 0.70 \
  --dump-predicted-csv /tmp/acoustic_mic_0.predicted.csv
```

If you want to rerun only the dataset evaluation step after a build:

```bash
cmake --build build-eval --target dataset_eval
ctest --test-dir build-eval --output-on-failure -R chord_core_dataset_
```

The label validator prints the exact line number and offending CSV row when it finds malformed data, such as a row whose `end_seconds` is not greater than `start_seconds`. The evaluator then prints matching duration, overlap accuracy, top confusion durations, sample onset deltas, and a short list of unmatched labeled spans before failing if accuracy is below the configured threshold.

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

Dataset evaluation is now part of the normal build whenever matching WAV/label pairs exist in `datasets/`. Coverage remains focused on the portable unit-test suite, so the `coverage` target does not automatically run the dataset evaluator.

## Format

The project requires clang-format at CMake configure time. The normal build automatically formats C++ files under
`include/`, `src/`, and `tests/` before compiling:

```bash
cmake --build build
```

To run only the formatter:

```bash
cmake --build build --target format
```

## Coverage

The `coverage` target wraps the full coverage flow in one command after configuration:

```bash
cmake --build build-coverage --target coverage
```

Coverage must be generated from a build tree configured with `-DCHORD_CORE_ENABLE_COVERAGE=ON`.
On macOS/AppleClang, CMake configures `gcovr` to use `xcrun llvm-cov gcov` automatically.
