#include "chord_template_matcher.h"

#include <array>
#include <cmath>
#include <stdexcept>

namespace chord {

namespace {

constexpr int kPitchClassCount = 12;

constexpr std::array<const char*, kPitchClassCount> kPitchClassNames = {
    "C",
    "C#",
    "D",
    "D#",
    "E",
    "F",
    "F#",
    "G",
    "G#",
    "A",
    "A#",
    "B",
};

constexpr std::array<int, 3> kMajorTriadOffsets = {0, 4, 7};
constexpr std::array<int, 3> kMinorTriadOffsets = {0, 3, 7};

int wrapPitchClass(int pitchClass) {
    const int wrapped = pitchClass % kPitchClassCount;
    return wrapped < 0 ? wrapped + kPitchClassCount : wrapped;
}

bool isTriadPitchClass(int pitchClass, int rootPitchClass, const std::array<int, 3>& offsets) {
    for (int offset : offsets) {
        if (pitchClass == wrapPitchClass(rootPitchClass + offset)) {
            return true;
        }
    }
    return false;
}

float computeChromaMagnitude(const ChromaVector& chroma) {
    float sumSquares = 0.0f;
    for (float value : chroma) {
        if (!std::isfinite(value) || value < 0.0f) {
            throw std::invalid_argument("chroma values must be finite and non-negative");
        }
        sumSquares += value * value;
    }
    return std::sqrt(sumSquares);
}

float computeTemplateSimilarity(const ChromaVector& chroma,
                                int rootPitchClass,
                                const std::array<int, 3>& offsets,
                                float chromaMagnitude) {
    // A triad template is a 12-bin vector with 1.0 at the root, third, and
    // fifth, and 0.0 elsewhere. The dot product measures how much chroma energy
    // lands on those three chord tones.
    float dotProduct = 0.0f;
    for (int pitchClass = 0; pitchClass < kPitchClassCount; ++pitchClass) {
        if (isTriadPitchClass(pitchClass, rootPitchClass, offsets)) {
            dotProduct += chroma[static_cast<std::size_t>(pitchClass)];
        }
    }

    // Cosine similarity keeps confidence in the 0..1 range for non-negative
    // chroma and makes the score independent of overall signal volume.
    constexpr float kTemplateMagnitude = 1.7320508f; // sqrt(3 triad tones)
    return dotProduct / (chromaMagnitude * kTemplateMagnitude);
}

std::string makeChordName(int rootPitchClass, bool isMinor) {
    std::string name = kPitchClassNames[static_cast<std::size_t>(rootPitchClass)];
    name += isMinor ? " minor" : " major";
    return name;
}

} // namespace

ChordResult ChordTemplateMatcher::match(const ChromaVector& chroma) const {
    const float chromaMagnitude = computeChromaMagnitude(chroma);
    if (chromaMagnitude <= 0.0f) {
        return {};
    }

    ChordResult bestResult{};
    for (int rootPitchClass = 0; rootPitchClass < kPitchClassCount; ++rootPitchClass) {
        const float majorConfidence =
            computeTemplateSimilarity(chroma, rootPitchClass, kMajorTriadOffsets, chromaMagnitude);
        if (majorConfidence > bestResult.confidence) {
            bestResult.name = makeChordName(rootPitchClass, false);
            bestResult.confidence = majorConfidence;
        }

        const float minorConfidence =
            computeTemplateSimilarity(chroma, rootPitchClass, kMinorTriadOffsets, chromaMagnitude);
        if (minorConfidence > bestResult.confidence) {
            bestResult.name = makeChordName(rootPitchClass, true);
            bestResult.confidence = minorConfidence;
        }
    }

    return bestResult;
}

} // namespace chord
