#include "signal_analysis.h"

#include <stdexcept>
#include <vector>

#include <gtest/gtest.h>

TEST(SignalAnalysisTest, ComputesRmsForKnownSamples) {
    const std::vector<float> samples = {1.0f, -1.0f, 1.0f, -1.0f};

    EXPECT_FLOAT_EQ(chord::computeRms(samples.data(), static_cast<int>(samples.size())), 1.0f);
}

TEST(SignalAnalysisTest, ComputesRmsForSilence) {
    const std::vector<float> samples(8, 0.0f);

    EXPECT_FLOAT_EQ(chord::computeRms(samples.data(), static_cast<int>(samples.size())), 0.0f);
}

TEST(SignalAnalysisTest, RejectsInvalidRmsInputs) {
    const std::vector<float> samples = {0.0f};

    EXPECT_THROW(chord::computeRms(nullptr, 1), std::invalid_argument);
    EXPECT_THROW(chord::computeRms(samples.data(), 0), std::invalid_argument);
    EXPECT_THROW(chord::computeRms(samples.data(), -1), std::invalid_argument);
}

TEST(SignalAnalysisTest, DetectsActiveSignalFromThreshold) {
    EXPECT_FALSE(chord::isSignalActive(0.009f, 0.01f));
    EXPECT_TRUE(chord::isSignalActive(0.01f, 0.01f));
    EXPECT_TRUE(chord::isSignalActive(0.02f, 0.01f));
}

TEST(SignalAnalysisTest, RejectsNegativeActivityThreshold) {
    EXPECT_THROW(chord::isSignalActive(0.0f, -0.1f), std::invalid_argument);
}
