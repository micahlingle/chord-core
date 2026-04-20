#pragma once

#include "chord_result.h"

namespace chord {

class ChordDetector {
public:
    void processBlock(const float* samples, int numSamples);
    ChordResult getCurrentChord() const;

private:
    ChordResult currentChord_{};
};

}  // namespace chord
