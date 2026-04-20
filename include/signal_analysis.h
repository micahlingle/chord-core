#pragma once

namespace chord {

// Root-mean-square amplitude for a block of normalized floating-point audio.
// This is a simple loudness proxy used for silence/noise gating.
float computeRms(const float* samples, int numSamples);

// Returns true when a block is loud enough to treat as meaningful audio.
// The threshold is intentionally caller-provided so demos/tests can tune it.
bool isSignalActive(float rms, float threshold);

} // namespace chord
