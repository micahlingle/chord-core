# Chord Core Milestones

This roadmap is the working plan for Chord Core. Update it as implementation details change.

## Project Goal

Create a reusable C++ library that:

- Accepts audio input from WAV files or streaming buffers.
- Processes audio in fixed-size blocks.
- Extracts frequency-domain features with an FFT.
- Computes 12-bin chroma vectors.
- Matches chroma vectors against chord templates.
- Applies temporal smoothing.
- Outputs the detected chord and a confidence score.

The library should be deterministic, efficient, portable, and safe for real-time use.

## Core Components

- `AudioBuffer`: data container for block-based audio.
- `FFTProcessor`: FFT setup and execution.
- `ChromaExtractor`: maps frequency energy to 12 pitch classes.
- `ChordTemplateMatcher`: matches chroma vectors against chord templates.
- `TemporalSmoother`: stabilizes chord output over time.
- `ChordDetector`: main streaming interface.

## Main API Target

```cpp
struct ChordResult {
    std::string name;
    float confidence;
};

class ChordDetector {
public:
    void processBlock(const float* samples, int numSamples);
    ChordResult getCurrentChord() const;
};
```

## Milestone 1: WAV Loading And RMS

Status: in progress

Goals:

- Set up a minimal CMake project.
- Load WAV files with `libsndfile`.
- Process audio in fixed-size blocks, starting with 1024 samples.
- Compute RMS for each block.
- Print block results to the console.
- Add basic GoogleTest coverage for the audio data/loading path.

Current notes:

- `AudioLoader` loads and downmixes file input to mono.
- `downMixInterleavedToMono` is testable independently from file I/O.
- `chord_core_demo` provides the current file-based RMS demo.

## Milestone 2: FFT And Frequency Analysis

Status: in progress

Goals:

- Add FFTW as a dependency.
- Implement `FFTProcessor`.
- Apply a simple analysis window before FFT.
- Compute magnitude spectrum per block.
- Print dominant and top frequencies per block in the demo.
- Add tests that validate FFT output on known sine-wave inputs.

Current notes:

- `FFTProcessor` uses single-precision FFTW (`fftw3f`).
- The demo uses 1024-sample RMS blocks and a 16384-sample FFT window.
- Frequency peak searches are currently constrained to 75 Hz through 5 kHz for guitar-focused diagnostics.
- Top-N peak reporting is a diagnostic tool; chroma extraction should use the full FFT magnitude spectrum.

## Milestone 3: Chroma Extraction

Status: pending

Goals:

- Implement `ChromaExtractor`.
- Map FFT bins to pitch classes.
- Normalize chroma vectors.
- Add tests for known frequency-to-pitch-class mappings.

## Milestone 4: Chord Template Matching

Status: pending

Goals:

- Implement major and minor chord templates.
- Compare normalized chroma vectors against templates.
- Return chord name and confidence.
- Add tests for known major/minor synthetic inputs.

## Milestone 5: Temporal Smoothing

Status: pending

Goals:

- Implement `TemporalSmoother`.
- Stabilize chord output across adjacent blocks.
- Avoid allocations in real-time processing paths.
- Add tests for smoothing behavior across short chord sequences.

## Testing Plan

- Use GoogleTest for unit tests.
- Validate FFT output with generated synthetic signals.
- Validate chroma mapping with known frequencies.
- Validate chord detection on known chroma vectors and eventually known audio snippets.
- Track optional line and branch coverage with `gcovr`.

## Constraints

- Use C++17 and CMake.
- Use FFTW for FFT.
- Use libsndfile for file-based audio loading.
- Avoid dynamic allocation inside `ChordDetector::processBlock`.
- Keep code modular and testable.
- Do not use machine learning.
- Do not add Python runtime dependencies.
- Avoid unnecessary abstraction.
