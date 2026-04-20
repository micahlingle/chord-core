#pragma once

#include "chord_result.h"

namespace chord {

// Stabilizes block-by-block chord guesses by requiring a chord to appear in
// consecutive frames before it replaces the currently reported chord.
class TemporalSmoother {
  public:
    explicit TemporalSmoother(int requiredStableFrames = 3);

    ChordResult smooth(const ChordResult& rawResult);
    void reset();

    int requiredStableFrames() const;

  private:
    int requiredStableFrames_ = 3;
    int candidateFrameCount_ = 0;
    ChordResult stableResult_{};
    ChordResult candidateResult_{};
};

} // namespace chord
