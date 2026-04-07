#include "chord_detector.h"

namespace chord {

void ChordDetector::processBlock(const float* samples, int numSamples) {
    (void)samples;
    (void)numSamples;
}

ChordResult ChordDetector::getCurrentChord() const {
    return currentChord_;
}

}  // namespace chord
