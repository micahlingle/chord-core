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
                    kDefaultMaxFrequencyHz,
                    kDefaultRequiredStableFrames,
                    kDefaultWindowMode) {}

ChordDetector::ChordDetector(int fftSize,
                             int sampleRate,
                             float activityThreshold,
                             float minFrequencyHz,
                             float maxFrequencyHz,
                             int requiredStableFrames,
                             FFTWindowMode windowMode)
    : fftProcessor_(fftSize, sampleRate, windowMode),
      chromaExtractor_(fftSize, sampleRate),
      temporalSmoother_(requiredStableFrames),
      activityThreshold_(activityThreshold),
      minFrequencyHz_(minFrequencyHz),
      maxFrequencyHz_(maxFrequencyHz),
      sampleHistory_(static_cast<std::size_t>(fftSize), 0.0f),
      analysisWindow_(static_cast<std::size_t>(fftSize), 0.0f) {
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

    appendSamples(samples, numSamples);

    const float rms = computeRms(samples, numSamples);
    if (!isSignalActive(rms, activityThreshold_)) {
        currentChord_ = temporalSmoother_.smooth({});
        return;
    }

    const int analysisSamples = copyRecentSamplesToAnalysisWindow();
    const std::vector<float>& magnitudes =
        fftProcessor_.computeMagnitudeSpectrum(analysisWindow_.data(), analysisSamples);
    const ChromaVector chroma = chromaExtractor_.extract(magnitudes, minFrequencyHz_, maxFrequencyHz_);
    currentChord_ = temporalSmoother_.smooth(chordMatcher_.match(chroma));
}

ChordResult ChordDetector::getCurrentChord() const {
    return currentChord_;
}

void ChordDetector::appendSamples(const float* samples, int numSamples) {
    const int fftSize = fftProcessor_.fftSize();
    for (int sampleIndex = 0; sampleIndex < numSamples; ++sampleIndex) {
        sampleHistory_[static_cast<std::size_t>(historyWriteIndex_)] = samples[sampleIndex];
        historyWriteIndex_ = (historyWriteIndex_ + 1) % fftSize;
        if (historyCount_ < fftSize) {
            ++historyCount_;
        }
    }
}

int ChordDetector::copyRecentSamplesToAnalysisWindow() {
    if (historyCount_ <= 0) {
        return 0;
    }

    const int fftSize = fftProcessor_.fftSize();
    if (historyCount_ < fftSize) {
        // Before the rolling history fills, the contiguous prefix already holds
        // samples in time order, so we can copy that prefix directly.
        for (int sampleIndex = 0; sampleIndex < historyCount_; ++sampleIndex) {
            analysisWindow_[static_cast<std::size_t>(sampleIndex)] =
                sampleHistory_[static_cast<std::size_t>(sampleIndex)];
        }
        return historyCount_;
    }

    // Once the circular history is full, historyWriteIndex_ points to the
    // oldest sample slot. Walking forward from there rebuilds the analysis
    // window from oldest to newest so the FFT window can emphasize recent
    // samples near the end of the frame.
    for (int sampleIndex = 0; sampleIndex < fftSize; ++sampleIndex) {
        const int historyIndex = (historyWriteIndex_ + sampleIndex) % fftSize;
        analysisWindow_[static_cast<std::size_t>(sampleIndex)] = sampleHistory_[static_cast<std::size_t>(historyIndex)];
    }
    return fftSize;
}

} // namespace chord
