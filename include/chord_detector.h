#pragma once

#include <string>

namespace chord {

struct ChordResult {
    std::string name = "Unknown";
    float confidence = 0.0f;
};

class ChordDetector {
public:
    void processBlock(const float* samples, int numSamples);
    ChordResult getCurrentChord() const;

private:
    ChordResult currentChord_{};
};

}  // namespace chord
