#include "chord_detector.h"

#include <cmath>
#include <stdexcept>
#include <string>
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

void addSineWaveRange(std::vector<float>& samples,
                      int sampleRate,
                      int startSample,
                      int endSample,
                      float frequencyHz,
                      float amplitude) {
    for (int index = startSample; index < endSample; ++index) {
        const float timeSeconds = static_cast<float>(index) / static_cast<float>(sampleRate);
        samples[static_cast<std::size_t>(index)] += amplitude * std::sin(2.0f * kPi * frequencyHz * timeSeconds);
    }
}

void addChordRange(std::vector<float>& samples,
                   int sampleRate,
                   int startSample,
                   int endSample,
                   float rootFrequencyHz,
                   float thirdFrequencyHz,
                   float fifthFrequencyHz) {
    addSineWaveRange(samples, sampleRate, startSample, endSample, rootFrequencyHz, 0.3f);
    addSineWaveRange(samples, sampleRate, startSample, endSample, thirdFrequencyHz, 0.3f);
    addSineWaveRange(samples, sampleRate, startSample, endSample, fifthFrequencyHz, 0.3f);
}

int findFirstDetectedBlock(std::vector<chord::ChordResult>* blockResults, const std::string& chordName) {
    for (std::size_t index = 0; index < blockResults->size(); ++index) {
        if ((*blockResults)[index].name == chordName) {
            return static_cast<int>(index);
        }
    }

    return -1;
}

} // namespace

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
    addSineWave(samples, kSampleRate, 261.63f, 0.3f); // C4
    addSineWave(samples, kSampleRate, 329.63f, 0.3f); // E4
    addSineWave(samples, kSampleRate, 392.00f, 0.3f); // G4

    chord::ChordDetector detector(kFftSize, kSampleRate, 0.001f, 75.0f, 5000.0f, 1);
    detector.processBlock(samples.data(), static_cast<int>(samples.size()));
    const chord::ChordResult result = detector.getCurrentChord();

    EXPECT_EQ(result.name, "C major");
    EXPECT_GT(result.confidence, 0.8f);
}

TEST(ChordDetectorTest, DetectsMinorChordFromSyntheticTriad) {
    constexpr int kFftSize = 16384;
    constexpr int kSampleRate = 48000;

    std::vector<float> samples(static_cast<std::size_t>(kFftSize), 0.0f);
    addSineWave(samples, kSampleRate, 220.00f, 0.3f); // A3
    addSineWave(samples, kSampleRate, 261.63f, 0.3f); // C4
    addSineWave(samples, kSampleRate, 329.63f, 0.3f); // E4

    chord::ChordDetector detector(kFftSize, kSampleRate, 0.001f, 75.0f, 5000.0f, 1);
    detector.processBlock(samples.data(), static_cast<int>(samples.size()));
    const chord::ChordResult result = detector.getCurrentChord();

    EXPECT_EQ(result.name, "A minor");
    EXPECT_GT(result.confidence, 0.8f);
}

TEST(ChordDetectorTest, AppliesDefaultTemporalSmoothing) {
    constexpr int kFftSize = 16384;
    constexpr int kSampleRate = 48000;

    std::vector<float> samples(static_cast<std::size_t>(kFftSize), 0.0f);
    addSineWave(samples, kSampleRate, 261.63f, 0.3f); // C4
    addSineWave(samples, kSampleRate, 329.63f, 0.3f); // E4
    addSineWave(samples, kSampleRate, 392.00f, 0.3f); // G4

    chord::ChordDetector detector(kFftSize, kSampleRate, 0.001f);

    detector.processBlock(samples.data(), static_cast<int>(samples.size()));
    EXPECT_EQ(detector.getCurrentChord().name, "Unknown");

    detector.processBlock(samples.data(), static_cast<int>(samples.size()));
    EXPECT_EQ(detector.getCurrentChord().name, "Unknown");

    detector.processBlock(samples.data(), static_cast<int>(samples.size()));
    EXPECT_EQ(detector.getCurrentChord().name, "C major");
}

TEST(ChordDetectorTest, DetectsChordAcrossStreamingBlocksWithRollingWindow) {
    constexpr int kFftSize = 16384;
    constexpr int kSampleRate = 48000;
    constexpr int kBlockSize = 1024;

    std::vector<float> samples(static_cast<std::size_t>(kFftSize), 0.0f);
    addSineWave(samples, kSampleRate, 261.63f, 0.3f); // C4
    addSineWave(samples, kSampleRate, 329.63f, 0.3f); // E4
    addSineWave(samples, kSampleRate, 392.00f, 0.3f); // G4

    chord::ChordDetector detector(kFftSize, kSampleRate, 0.001f, 75.0f, 5000.0f, 1);

    for (int start = 0; start < kFftSize; start += kBlockSize) {
        detector.processBlock(samples.data() + start, kBlockSize);
    }

    const chord::ChordResult result = detector.getCurrentChord();
    EXPECT_EQ(result.name, "C major");
    EXPECT_GT(result.confidence, 0.8f);
}

TEST(ChordDetectorTest, CausalWindowDetectsChordChangeEarlierThanSymmetricWindow) {
    constexpr int kFftSize = 16384;
    constexpr int kSampleRate = 48000;
    constexpr int kBlockSize = 1024;
    constexpr int kBlocksPerPhase = 16;
    constexpr int kTotalBlocks = kBlocksPerPhase * 2;

    std::vector<float> streamSamples(static_cast<std::size_t>(kTotalBlocks * kBlockSize), 0.0f);
    addChordRange(streamSamples, kSampleRate, 0, kBlocksPerPhase * kBlockSize, 261.63f, 329.63f, 392.00f); // C major
    addChordRange(streamSamples,
                  kSampleRate,
                  kBlocksPerPhase * kBlockSize,
                  kTotalBlocks * kBlockSize,
                  293.66f,
                  369.99f,
                  440.00f); // D major

    chord::ChordDetector
        causalDetector(kFftSize, kSampleRate, 0.001f, 75.0f, 5000.0f, 1, chord::FFTWindowMode::CausalRightBiased);
    chord::ChordDetector
        symmetricDetector(kFftSize, kSampleRate, 0.001f, 75.0f, 5000.0f, 1, chord::FFTWindowMode::SymmetricHann);

    std::vector<chord::ChordResult> causalResults;
    std::vector<chord::ChordResult> symmetricResults;
    causalResults.reserve(kTotalBlocks);
    symmetricResults.reserve(kTotalBlocks);

    for (int blockIndex = 0; blockIndex < kTotalBlocks; ++blockIndex) {
        const int blockStart = blockIndex * kBlockSize;
        const float* blockSamples = streamSamples.data() + blockStart;

        causalDetector.processBlock(blockSamples, kBlockSize);
        symmetricDetector.processBlock(blockSamples, kBlockSize);

        causalResults.push_back(causalDetector.getCurrentChord());
        symmetricResults.push_back(symmetricDetector.getCurrentChord());
    }

    const int causalDetectionBlock = findFirstDetectedBlock(&causalResults, "D major");
    const int symmetricDetectionBlock = findFirstDetectedBlock(&symmetricResults, "D major");

    ASSERT_GE(causalDetectionBlock, 0);
    ASSERT_GE(symmetricDetectionBlock, 0);
    EXPECT_LT(causalDetectionBlock, symmetricDetectionBlock);
}
