#include "signal_analysis.h"

#include <cmath>
#include <stdexcept>

namespace chord {

float computeRms(const float* samples, int numSamples) {
    if (samples == nullptr) {
        throw std::invalid_argument("samples must not be null");
    }

    if (numSamples <= 0) {
        throw std::invalid_argument("numSamples must be positive");
    }

    float sumSquares = 0.0f;
    for (int index = 0; index < numSamples; ++index) {
        const float sample = samples[index];
        sumSquares += sample * sample;
    }

    return std::sqrt(sumSquares / static_cast<float>(numSamples));
}

bool isSignalActive(float rms, float threshold) {
    if (threshold < 0.0f) {
        throw std::invalid_argument("threshold must be non-negative");
    }

    return rms >= threshold;
}

} // namespace chord
