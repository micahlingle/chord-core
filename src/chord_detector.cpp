#include "chord_detector.h"

#include "signal_analysis.h"

#include <stdexcept>
#include <vector>

namespace chord {

ChordDetector::ChordDetector()
    : ChordDetector(kDefaultFftSize,
                    kDefaultSampleRate,
                    kDefaultActivityThreshold,
                    kDefaultMinFrequencyHz,
                    kDefaultMaxFrequencyHz) {}

ChordDetector::ChordDetector(int fftSize,
                             int sampleRate,
                             float activityThreshold,
                             float minFrequencyHz,
                             float maxFrequencyHz)
    : fftProcessor_(fftSize, sampleRate),
      chromaExtractor_(fftSize, sampleRate),
      activityThreshold_(activityThreshold),
      minFrequencyHz_(minFrequencyHz),
      maxFrequencyHz_(maxFrequencyHz) {
    if (activityThreshold_ < 0.0f) {
        throw std::invalid_argument("activityThreshold must be non-negative");
    }

    if (minFrequencyHz_ < 0.0f || maxFrequencyHz_ <= minFrequencyHz_) {
        throw std::invalid_argument("frequency range must be non-negative and increasing");
    }
}

void ChordDetector::processBlock(const float* samples, int numSamples) {
    if (samples == nullptr) {
        throw std::invalid_argument("samples must not be null");
    }

    if (numSamples <= 0) {
        throw std::invalid_argument("numSamples must be positive");
    }

    const float rms = computeRms(samples, numSamples);
    if (!isSignalActive(rms, activityThreshold_)) {
        currentChord_ = {};
        return;
    }

    const std::vector<float>& magnitudes = fftProcessor_.computeMagnitudeSpectrum(samples, numSamples);
    const ChromaVector chroma = chromaExtractor_.extract(magnitudes, minFrequencyHz_, maxFrequencyHz_);
    currentChord_ = chordMatcher_.match(chroma);
}

ChordResult ChordDetector::getCurrentChord() const {
    return currentChord_;
}

}  // namespace chord
