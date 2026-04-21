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

Status: complete

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
- `signal_analysis` provides RMS and activity-threshold helpers.
- `chord_core_demo` provides the current file-based RMS demo.

## Milestone 2: FFT And Frequency Analysis

Status: complete

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
- `FFTProcessor` now supports both a symmetric Hann window and a right-biased causal window for lower-latency streaming detection.
- Frequency peak searches are currently constrained to 75 Hz through 5 kHz for guitar-focused diagnostics.
- Demo frequency reporting is gated by a `0.01` RMS activity threshold.
- Top-N peak reporting is a diagnostic tool; chroma extraction should use the full FFT magnitude spectrum.

## Milestone 3: Chroma Extraction

Status: complete

Goals:

- Implement `ChromaExtractor`.
- Map FFT bins to pitch classes.
- Normalize chroma vectors.
- Add tests for known frequency-to-pitch-class mappings.

Current notes:

- `ChromaExtractor` maps each FFT magnitude bin into the nearest equal-tempered pitch class using A4 = 440 Hz.
- Chroma vectors are normalized by the strongest pitch class so later template matching can compare pitch-class shape instead of absolute signal level.
- The demo prints chroma vectors for active blocks using the same 75 Hz to 5 kHz analysis band as the frequency diagnostics.

## Milestone 4: Chord Template Matching

Status: complete

Goals:

- Implement major and minor chord templates.
- Compare normalized chroma vectors against templates.
- Return chord name and confidence.
- Add tests for known major/minor synthetic inputs.

Current notes:

- `ChordTemplateMatcher` compares chroma against 24 major/minor triad templates.
- Matching uses cosine similarity so confidence is based on pitch-class shape rather than absolute level.
- The public demo exercises `ChordDetector` and prints concise chord/confidence output.
- `chord_core_analyze` prints RMS, activity, FFT peaks, chroma, chord, and confidence for DSP debugging and tuning.
- `ChordDetector` now runs the reusable block-processing path: RMS/activity gate, FFT, chroma extraction, and template matching.
- `ChordDetector` keeps a rolling FFT analysis window so streaming block input uses the most recent `fftSize` samples instead of analyzing only the latest short block.
- `ChordDetector` uses the right-biased causal FFT window by default so recent samples influence chord changes sooner without reducing FFT size.
- Temporal smoothing is intentionally not part of this milestone and remains the next stabilization layer.

## Milestone 5: Temporal Smoothing

Status: complete

Goals:

- Implement `TemporalSmoother`.
- Stabilize chord output across adjacent blocks.
- Avoid allocations in real-time processing paths.
- Add tests for smoothing behavior across short chord sequences.

Current notes:

- `TemporalSmoother` requires the same non-Unknown chord to appear for a configurable number of consecutive frames before reporting it.
- Silence or inactive input resets the smoothed result to `Unknown` immediately.
- Some residual output delay still comes from the 1024-sample block hop and the smoothing requirement, but the largest FFT-window lag is reduced through causal window shaping instead of shrinking `fftSize`.

## Local Evaluation

Status: complete

Goals:

- Compare `ChordDetector` output against labeled WAV files outside the unit-test fixture set.
- Measure overlap accuracy over labeled chord spans.
- Fail a dedicated local regression target when accuracy drops below a configured threshold.

Current notes:

- `chord_core_eval` loads one local WAV file and one label CSV in `start_seconds,end_seconds,chord` format.
- The evaluator scores overlap accuracy only across labeled time ranges, so unlabeled gaps are ignored.
- The build now auto-discovers tracked `datasets/*.wav` files that have matching `datasets/*.labels.csv` files, validates each CSV first, and evaluates each pair during the normal build.
- Matching discovered datasets are also registered as dedicated CTest entries.
- The default required overlap accuracy is `0.70`, and each dataset can override that with an optional sibling `*.min_accuracy.txt` file.
- Predicted spans can be dumped back to CSV for side-by-side comparison during tuning.

## Next Optimization Directions

Status: pending

Goals:

- Keep the current low-frequency resolution while reducing CPU cost in the analysis path.
- Reduce visible chord-change latency without giving up the current FFT-size-based frequency granularity.
- Use the dataset evaluation workflow to measure whether each DSP change is actually helpful.

Current notes:

- The leading CPU-reduction idea is a multirate front end: low-pass around the useful chord band, decimate, and then run a smaller FFT that preserves the current bin spacing.
- The leading latency-reduction idea is a smaller processing hop size, so `ChordDetector` can update more often without shrinking the main analysis window.
- These two directions should be evaluated separately when possible so CPU savings and latency improvements do not get conflated in the regression results.

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
