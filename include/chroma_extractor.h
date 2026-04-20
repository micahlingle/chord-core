#pragma once

#include <array>
#include <vector>

namespace chord {

// Pitch classes are note names without octave information. For example, E2,
// E3, and E4 all map to kPitchClassE.
enum PitchClass {
    kPitchClassC = 0,
    kPitchClassCSharp = 1,
    kPitchClassD = 2,
    kPitchClassDSharp = 3,
    kPitchClassE = 4,
    kPitchClassF = 5,
    kPitchClassFSharp = 6,
    kPitchClassG = 7,
    kPitchClassGSharp = 8,
    kPitchClassA = 9,
    kPitchClassASharp = 10,
    kPitchClassB = 11,
};

using ChromaVector = std::array<float, 12>;

// Converts FFT magnitudes into a 12-bin chroma vector. Each chroma bin
// represents the accumulated energy for one pitch class.
class ChromaExtractor {
public:
    ChromaExtractor(int fftSize, int sampleRate);

    // Aggregates magnitudes from FFT bins in the requested frequency range into
    // pitch classes, then normalizes so the strongest pitch class is 1.0.
    ChromaVector extract(const std::vector<float>& magnitudes,
                         float minFrequencyHz,
                         float maxFrequencyHz) const;

    // Maps a frequency in Hertz to one of the 12 pitch classes using A4 = 440 Hz
    // as the tuning reference.
    int frequencyToPitchClass(float frequencyHz) const;

    int fftSize() const;
    int sampleRate() const;
    int binCount() const;

private:
    int fftSize_ = 0;
    int sampleRate_ = 0;
    int binCount_ = 0;
};

}  // namespace chord
