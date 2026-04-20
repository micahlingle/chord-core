#include "chord_detector.h"

#include <cmath>
#include <stdexcept>
#include <vector>

#include <gtest/gtest.h>

namespace {

constexpr float kPi = 3.14159265358979323846f;

void addSineWave(std::vector<float>& samples, int sampleRate, float frequencyHz, float amplitude) {
    for (std::size_t index = 0; index < samples.size(); ++index) {
        const float timeSeconds = static_cast<float>(index) / static_cast<float>(sampleRate);
        samples[index] += amplitude * std::sin(2.0f * kPi * frequencyHz * timeSeconds);
    }
}

}  // namespace

TEST(ChordDetectorTest, DefaultChordIsUnknown) {
    chord::ChordDetector detector;
    const chord::ChordResult result = detector.getCurrentChord();

    EXPECT_EQ(result.name, "Unknown");
    EXPECT_FLOAT_EQ(result.confidence, 0.0f);
}

TEST(ChordDetectorTest, RejectsInvalidConfiguration) {
    EXPECT_THROW(chord::ChordDetector(0, 48000), std::invalid_argument);
    EXPECT_THROW(chord::ChordDetector(4096, 0), std::invalid_argument);
    EXPECT_THROW(chord::ChordDetector(4096, 48000, -0.1f), std::invalid_argument);
    EXPECT_THROW(chord::ChordDetector(4096, 48000, 0.01f, -1.0f, 5000.0f), std::invalid_argument);
    EXPECT_THROW(chord::ChordDetector(4096, 48000, 0.01f, 5000.0f, 75.0f), std::invalid_argument);
}

TEST(ChordDetectorTest, RejectsInvalidSampleBlocks) {
    std::vector<float> samples(1024, 0.0f);
    chord::ChordDetector detector(4096, 48000);

    EXPECT_THROW(detector.processBlock(nullptr, 1024), std::invalid_argument);
    EXPECT_THROW(detector.processBlock(samples.data(), 0), std::invalid_argument);
    EXPECT_THROW(detector.processBlock(samples.data(), -1), std::invalid_argument);
}

TEST(ChordDetectorTest, QuietBlockResetsChordToUnknown) {
    std::vector<float> samples(4096, 0.0f);
    chord::ChordDetector detector(4096, 48000, 0.01f);

    detector.processBlock(samples.data(), static_cast<int>(samples.size()));
    const chord::ChordResult result = detector.getCurrentChord();

    EXPECT_EQ(result.name, "Unknown");
    EXPECT_FLOAT_EQ(result.confidence, 0.0f);
}

TEST(ChordDetectorTest, DetectsMajorChordFromSyntheticTriad) {
    constexpr int kFftSize = 16384;
    constexpr int kSampleRate = 48000;

    std::vector<float> samples(static_cast<std::size_t>(kFftSize), 0.0f);
    addSineWave(samples, kSampleRate, 261.63f, 0.3f);  // C4
    addSineWave(samples, kSampleRate, 329.63f, 0.3f);  // E4
    addSineWave(samples, kSampleRate, 392.00f, 0.3f);  // G4

    chord::ChordDetector detector(kFftSize, kSampleRate, 0.001f);
    detector.processBlock(samples.data(), static_cast<int>(samples.size()));
    const chord::ChordResult result = detector.getCurrentChord();

    EXPECT_EQ(result.name, "C major");
    EXPECT_GT(result.confidence, 0.8f);
}

TEST(ChordDetectorTest, DetectsMinorChordFromSyntheticTriad) {
    constexpr int kFftSize = 16384;
    constexpr int kSampleRate = 48000;

    std::vector<float> samples(static_cast<std::size_t>(kFftSize), 0.0f);
    addSineWave(samples, kSampleRate, 220.00f, 0.3f);  // A3
    addSineWave(samples, kSampleRate, 261.63f, 0.3f);  // C4
    addSineWave(samples, kSampleRate, 329.63f, 0.3f);  // E4

    chord::ChordDetector detector(kFftSize, kSampleRate, 0.001f);
    detector.processBlock(samples.data(), static_cast<int>(samples.size()));
    const chord::ChordResult result = detector.getCurrentChord();

    EXPECT_EQ(result.name, "A minor");
    EXPECT_GT(result.confidence, 0.8f);
}
