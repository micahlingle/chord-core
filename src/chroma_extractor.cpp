#include "chroma_extractor.h"

#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace chord {

namespace {

// MIDI note 69 is A4, conventionally tuned to 440 Hz. We use that reference to
// convert arbitrary frequencies into the nearest equal-tempered semitone.
constexpr float kReferenceFrequencyHz = 440.0f;
constexpr int kReferenceMidiNote = 69;
constexpr int kPitchClassCount = 12;

}  // namespace

ChromaExtractor::ChromaExtractor(int fftSize, int sampleRate)
    : fftSize_(fftSize), sampleRate_(sampleRate) {
    if (fftSize_ <= 0) {
        throw std::invalid_argument("fftSize must be positive");
    }

    if (sampleRate_ <= 0) {
        throw std::invalid_argument("sampleRate must be positive");
    }

    binCount_ = fftSize_ / 2 + 1;
}

ChromaVector ChromaExtractor::extract(const std::vector<float>& magnitudes,
                                      float minFrequencyHz,
                                      float maxFrequencyHz) const {
    if (static_cast<int>(magnitudes.size()) != binCount_) {
        throw std::invalid_argument("magnitude vector size must match FFT bin count");
    }

    if (minFrequencyHz < 0.0f || maxFrequencyHz <= minFrequencyHz) {
        throw std::invalid_argument("frequency range must be non-negative and increasing");
    }

    // Nyquist frequency is the max frequency of a digitally-represented wave.
    // For a 48kHz sample, the max frequency an oscillating wave (with peaks and troughs)
    // would be 24kHz (where one sample is high and the next is low).
    const float nyquistHz = static_cast<float>(sampleRate_) * 0.5f;

    // Each element is one pitch class: C, C#, D, ..., B. The values start at
    // zero and collect magnitude energy from all FFT bins that map to that note.
    ChromaVector chroma{};
    const float clampedMaxFrequencyHz = std::min(maxFrequencyHz, nyquistHz);
    if (clampedMaxFrequencyHz <= minFrequencyHz) {
        return chroma;
    }

    const float binWidthHz = static_cast<float>(sampleRate_) / static_cast<float>(fftSize_);
    // Skip bin 0 because it is DC offset. The min/max bins let callers focus on
    // the useful musical band and ignore rumble or very high-frequency noise.
    const int minBin = std::max(1, static_cast<int>(std::ceil(minFrequencyHz / binWidthHz)));
    const int maxBin = std::min(binCount_ - 1,
                                static_cast<int>(std::floor(clampedMaxFrequencyHz / binWidthHz)));

    for (int bin = minBin; bin <= maxBin; ++bin) {
        const float magnitude = magnitudes[static_cast<std::size_t>(bin)];
        if (magnitude <= 0.0f) {
            continue;
        }

        const float frequencyHz = static_cast<float>(bin) * binWidthHz;
        const int pitchClass = frequencyToPitchClass(frequencyHz);
        // Chroma intentionally collapses octaves: energy at A2, A3, and A4 all
        // contributes to the same A bin. This is why it is useful for chords.
        chroma[static_cast<std::size_t>(pitchClass)] += magnitude;
    }

    const auto maxElement = std::max_element(chroma.begin(), chroma.end());
    if (maxElement == chroma.end() || *maxElement <= 0.0f) {
        return chroma;
    }

    // Normalize by the strongest pitch class so downstream chord matching can
    // compare relative pitch-class shape instead of absolute recording volume.
    const float maxValue = *maxElement;
    for (float& value : chroma) {
        value /= maxValue;
    }

    return chroma;
}

int ChromaExtractor::frequencyToPitchClass(float frequencyHz) const {
    if (frequencyHz <= 0.0f) {
        throw std::invalid_argument("frequencyHz must be positive");
    }

    // Equal temperament is logarithmic: multiplying frequency by 2 raises the
    // note by one octave, or 12 semitones. log2(f / 440) measures that distance
    // in octaves, then multiplying by 12 converts it to semitones from A4.
    const float midiNoteFloat =
        static_cast<float>(kReferenceMidiNote) + 12.0f * std::log2(frequencyHz / kReferenceFrequencyHz);
    const int midiNote = static_cast<int>(std::lround(midiNoteFloat));
    // MIDI pitch classes use C = 0, C# = 1, ..., B = 11. Modulo 12 removes the
    // octave while keeping the note name.
    const int pitchClass = midiNote % kPitchClassCount;
    return pitchClass < 0 ? pitchClass + kPitchClassCount : pitchClass;
}

int ChromaExtractor::fftSize() const {
    return fftSize_;
}

int ChromaExtractor::sampleRate() const {
    return sampleRate_;
}

int ChromaExtractor::binCount() const {
    return binCount_;
}

}  // namespace chord
