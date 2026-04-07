# AGENTS.md

## Project Context

Chord Core is a portable C++17 library for chord detection from audio. It prioritizes deterministic, efficient, real-time-safe DSP code that can later run in an embedded/streaming environment.

Before planning feature work, read `docs/MILESTONES.md`.

## Build And Test

- Configure tests: `cmake -S . -B build -DCHORD_CORE_BUILD_TESTS=ON`
- Build: `cmake --build build`
- Test: `ctest --test-dir build --output-on-failure`

## Coverage

Coverage is optional and should be run from a separate build tree:

- Configure coverage: `cmake --fresh -S . -B build-coverage -DCHORD_CORE_BUILD_TESTS=ON -DCHORD_CORE_ENABLE_COVERAGE=ON -DCMAKE_BUILD_TYPE=Debug`
- Build coverage: `cmake --build build-coverage`
- Test coverage: `ctest --test-dir build-coverage --output-on-failure`
- Report coverage on macOS/AppleClang: `gcovr --root . --object-directory build-coverage --filter 'src/.*' --filter 'include/.*' --exclude 'tests/.*' --txt-metric branch --gcov-executable 'xcrun llvm-cov gcov' --print-summary`

## Engineering Constraints

- Avoid dynamic allocation inside real-time processing paths like `ChordDetector::processBlock`.
- Keep implementations simple, readable, and modular.
- Prefer correctness over completeness.
- Add focused GoogleTest coverage for DSP behavior.
- Do not add machine learning or Python runtime dependencies.
- Keep file-based loading separate from streaming detection logic.

## Code Style

- Use C++17.
- Prefer small concrete classes over heavy abstraction.
- Keep public APIs minimal and stable.
- Add comments only where DSP logic would otherwise be unclear.
- Default to portable standard-library code unless a milestone explicitly requires an external dependency such as FFTW or libsndfile.
