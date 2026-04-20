#pragma once

#include "chord_template_matcher.h"
#include "chroma_extractor.h"
#include "chord_result.h"
#include "fft_processor.h"

namespace chord {

class ChordDetector {
  public:
    static constexpr int kDefaultFftSize = 16384;
    static constexpr int kDefaultSampleRate = 48000;
    static constexpr float kDefaultActivityThreshold = 0.01f;
    static constexpr float kDefaultMinFrequencyHz = 75.0f;
    static constexpr float kDefaultMaxFrequencyHz = 5000.0f;

    ChordDetector();
    ChordDetector(int fftSize,
                  int sampleRate,
                  float activityThreshold = kDefaultActivityThreshold,
                  float minFrequencyHz = kDefaultMinFrequencyHz,
                  float maxFrequencyHz = kDefaultMaxFrequencyHz);

    void processBlock(const float* samples, int numSamples);
    ChordResult getCurrentChord() const;

  private:
    FFTProcessor fftProcessor_;
    ChromaExtractor chromaExtractor_;
    ChordTemplateMatcher chordMatcher_;
    float activityThreshold_ = kDefaultActivityThreshold;
    float minFrequencyHz_ = kDefaultMinFrequencyHz;
    float maxFrequencyHz_ = kDefaultMaxFrequencyHz;
    ChordResult currentChord_{};
};

} // namespace chord
