#include "temporal_smoother.h"

#include <stdexcept>

#include <gtest/gtest.h>

namespace {

chord::ChordResult makeChord(const std::string& name, float confidence) {
    chord::ChordResult result;
    result.name = name;
    result.confidence = confidence;
    return result;
}

} // namespace

TEST(TemporalSmootherTest, RejectsInvalidConfiguration) {
    EXPECT_THROW(chord::TemporalSmoother(0), std::invalid_argument);
    EXPECT_THROW(chord::TemporalSmoother(-1), std::invalid_argument);
}

TEST(TemporalSmootherTest, ReportsConfiguration) {
    chord::TemporalSmoother smoother(4);

    EXPECT_EQ(smoother.requiredStableFrames(), 4);
}

TEST(TemporalSmootherTest, RequiresConsecutiveFramesBeforeReportingNewChord) {
    chord::TemporalSmoother smoother(3);
    const chord::ChordResult cMajor = makeChord("C major", 0.9f);

    EXPECT_EQ(smoother.smooth(cMajor).name, "Unknown");
    EXPECT_EQ(smoother.smooth(cMajor).name, "Unknown");

    const chord::ChordResult smoothed = smoother.smooth(cMajor);
    EXPECT_EQ(smoothed.name, "C major");
    EXPECT_FLOAT_EQ(smoothed.confidence, 0.9f);
}

TEST(TemporalSmootherTest, IgnoresShortFlickerToDifferentChord) {
    chord::TemporalSmoother smoother(3);
    const chord::ChordResult cMajor = makeChord("C major", 0.9f);
    const chord::ChordResult cMinor = makeChord("C minor", 0.8f);

    smoother.smooth(cMajor);
    smoother.smooth(cMajor);
    ASSERT_EQ(smoother.smooth(cMajor).name, "C major");

    EXPECT_EQ(smoother.smooth(cMinor).name, "C major");
    EXPECT_EQ(smoother.smooth(cMinor).name, "C major");
    EXPECT_EQ(smoother.smooth(cMajor).name, "C major");
}

TEST(TemporalSmootherTest, SwitchesAfterNewChordIsStable) {
    chord::TemporalSmoother smoother(2);
    const chord::ChordResult cMajor = makeChord("C major", 0.9f);
    const chord::ChordResult gMajor = makeChord("G major", 0.85f);

    smoother.smooth(cMajor);
    ASSERT_EQ(smoother.smooth(cMajor).name, "C major");

    EXPECT_EQ(smoother.smooth(gMajor).name, "C major");
    const chord::ChordResult smoothed = smoother.smooth(gMajor);

    EXPECT_EQ(smoothed.name, "G major");
    EXPECT_FLOAT_EQ(smoothed.confidence, 0.85f);
}

TEST(TemporalSmootherTest, UpdatesConfidenceForCurrentStableChord) {
    chord::TemporalSmoother smoother(1);
    ASSERT_EQ(smoother.smooth(makeChord("A minor", 0.7f)).name, "A minor");

    const chord::ChordResult smoothed = smoother.smooth(makeChord("A minor", 0.95f));

    EXPECT_EQ(smoothed.name, "A minor");
    EXPECT_FLOAT_EQ(smoothed.confidence, 0.95f);
}

TEST(TemporalSmootherTest, UnknownResetsImmediately) {
    chord::TemporalSmoother smoother(1);
    ASSERT_EQ(smoother.smooth(makeChord("E minor", 0.8f)).name, "E minor");

    const chord::ChordResult smoothed = smoother.smooth(chord::ChordResult{});

    EXPECT_EQ(smoothed.name, "Unknown");
    EXPECT_FLOAT_EQ(smoothed.confidence, 0.0f);
}

TEST(TemporalSmootherTest, NamedChordWithZeroConfidenceResetsImmediately) {
    chord::TemporalSmoother smoother(1);
    ASSERT_EQ(smoother.smooth(makeChord("D major", 0.8f)).name, "D major");

    const chord::ChordResult smoothed = smoother.smooth(makeChord("D major", 0.0f));

    EXPECT_EQ(smoothed.name, "Unknown");
    EXPECT_FLOAT_EQ(smoothed.confidence, 0.0f);
}
