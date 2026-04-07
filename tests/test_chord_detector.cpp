#include "chord_detector.h"

#include <gtest/gtest.h>

TEST(ChordDetectorTest, DefaultChordIsUnknown) {
    chord::ChordDetector detector;
    const chord::ChordResult result = detector.getCurrentChord();

    EXPECT_EQ(result.name, "Unknown");
    EXPECT_FLOAT_EQ(result.confidence, 0.0f);
}
