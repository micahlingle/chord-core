#include "temporal_smoother.h"

#include <stdexcept>

namespace chord {

namespace {

// Define reportable/stable chord as not unknown
bool isReportableChord(const ChordResult& result) {
    return result.name != "Unknown" && result.confidence > 0.0f;
}

} // namespace

TemporalSmoother::TemporalSmoother(int requiredStableFrames)
    : requiredStableFrames_(requiredStableFrames) {
    if (requiredStableFrames_ <= 0) {
        throw std::invalid_argument("requiredStableFrames must be positive");
    }
}

ChordResult TemporalSmoother::smooth(const ChordResult& rawResult) {
    if (!isReportableChord(rawResult)) {
        reset();
        return stableResult_;
    }

    if (rawResult.name == stableResult_.name) {
        stableResult_ = rawResult;
        candidateResult_ = {};
        candidateFrameCount_ = 0;
        return stableResult_;
    }

    if (rawResult.name == candidateResult_.name) {
        ++candidateFrameCount_;
    } else {
        candidateResult_ = rawResult;
        candidateFrameCount_ = 1;
    }

    if (candidateFrameCount_ >= requiredStableFrames_) {
        stableResult_ = candidateResult_;
        candidateResult_ = {};
        candidateFrameCount_ = 0;
    }

    return stableResult_;
}

void TemporalSmoother::reset() {
    stableResult_ = {};
    candidateResult_ = {};
    candidateFrameCount_ = 0;
}

int TemporalSmoother::requiredStableFrames() const {
    return requiredStableFrames_;
}

} // namespace chord
