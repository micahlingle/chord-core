#include "fft_processor.h"

#include <cmath>
#include <stdexcept>
#include <vector>

#include <gtest/gtest.h>

namespace {

constexpr float kPi = 3.14159265358979323846f;

std::vector<float> makeSineWave(int sampleRate, int sampleCount, float frequencyHz) {
    std::vector<float> samples(static_cast<std::size_t>(sampleCount));
    for (int index = 0; index < sampleCount; ++index) {
        const float timeSeconds = static_cast<float>(index) / static_cast<float>(sampleRate);
        samples[static_cast<std::size_t>(index)] = std::sin(2.0f * kPi * frequencyHz * timeSeconds);
    }

    return samples;
}

void addSineWave(std::vector<float>& samples, int sampleRate, float frequencyHz, float amplitude) {
    for (std::size_t index = 0; index < samples.size(); ++index) {
        const float timeSeconds = static_cast<float>(index) / static_cast<float>(sampleRate);
        samples[index] += amplitude * std::sin(2.0f * kPi * frequencyHz * timeSeconds);
    }
}

bool isNearFrequency(float actualFrequencyHz, float expectedFrequencyHz, float toleranceHz) {
    return std::fabs(actualFrequencyHz - expectedFrequencyHz) <= toleranceHz;
}

} // namespace

TEST(FFTProcessorTest, RejectsInvalidConfiguration) {
    EXPECT_THROW(chord::FFTProcessor(0, 48000), std::invalid_argument);
    EXPECT_THROW(chord::FFTProcessor(-1024, 48000), std::invalid_argument);
    EXPECT_THROW(chord::FFTProcessor(1024, 0), std::invalid_argument);
    EXPECT_THROW(chord::FFTProcessor(1024, -48000), std::invalid_argument);
}

TEST(FFTProcessorTest, ReportsExpectedBinCount) {
    chord::FFTProcessor processor(1024, 48000);

    EXPECT_EQ(processor.fftSize(), 1024);
    EXPECT_EQ(processor.sampleRate(), 48000);
    EXPECT_EQ(processor.windowMode(), chord::FFTWindowMode::SymmetricHann);
    EXPECT_EQ(processor.binCount(), 513);
}

TEST(FFTProcessorTest, BuildsSymmetricHannWindowByDefault) {
    chord::FFTProcessor processor(16, 48000);

    EXPECT_FLOAT_EQ(processor.windowWeight(0), 0.0f);
    EXPECT_FLOAT_EQ(processor.windowWeight(15), 0.0f);
    EXPECT_GT(processor.windowWeight(8), processor.windowWeight(4));
    EXPECT_GT(processor.windowWeight(7), processor.windowWeight(0));
}

TEST(FFTProcessorTest, BuildsCausalWindowThatFavorsRecentSamples) {
    chord::FFTProcessor processor(16, 48000, chord::FFTWindowMode::CausalRightBiased);

    EXPECT_EQ(processor.windowMode(), chord::FFTWindowMode::CausalRightBiased);
    EXPECT_FLOAT_EQ(processor.windowWeight(0), 0.0f);
    EXPECT_NEAR(processor.windowWeight(15), 1.0f, 1.0e-6f);
    EXPECT_LT(processor.windowWeight(4), processor.windowWeight(8));
    EXPECT_LT(processor.windowWeight(8), processor.windowWeight(12));
}

TEST(FFTProcessorTest, DetectsDominantFrequencyForSineWave) {
    constexpr int kFftSize = 4096;
    constexpr int kSampleRate = 48000;
    constexpr float kFrequencyHz = 440.0f;

    const std::vector<float> samples = makeSineWave(kSampleRate, kFftSize, kFrequencyHz);
    chord::FFTProcessor processor(kFftSize, kSampleRate);

    const std::vector<float>& magnitudes =
        processor.computeMagnitudeSpectrum(samples.data(), static_cast<int>(samples.size()));

    const float binWidthHz = static_cast<float>(kSampleRate) / static_cast<float>(kFftSize);
    EXPECT_EQ(magnitudes.size(), static_cast<std::size_t>(processor.binCount()));
    EXPECT_NEAR(processor.findDominantFrequencyHz(), kFrequencyHz, binWidthHz);
}

TEST(FFTProcessorTest, FindsTopFrequencyPeakForSineWave) {
    constexpr int kFftSize = 4096;
    constexpr int kSampleRate = 48000;
    constexpr float kFrequencyHz = 440.0f;

    const std::vector<float> samples = makeSineWave(kSampleRate, kFftSize, kFrequencyHz);
    chord::FFTProcessor processor(kFftSize, kSampleRate);
    processor.computeMagnitudeSpectrum(samples.data(), static_cast<int>(samples.size()));

    chord::FrequencyPeak peaks[1];
    const int peakCount = processor.findTopFrequencyPeaks(peaks, 1, 75.0f, 5000.0f);

    const float binWidthHz = static_cast<float>(kSampleRate) / static_cast<float>(kFftSize);
    ASSERT_EQ(peakCount, 1);
    EXPECT_NEAR(peaks[0].frequencyHz, kFrequencyHz, binWidthHz);
    EXPECT_GT(peaks[0].magnitude, 0.0f);
}

TEST(FFTProcessorTest, FindsMultiplePeaksInDescendingMagnitudeOrder) {
    constexpr int kFftSize = 16384;
    constexpr int kSampleRate = 48000;

    std::vector<float> samples(static_cast<std::size_t>(kFftSize), 0.0f);
    addSineWave(samples, kSampleRate, 220.0f, 1.0f);
    addSineWave(samples, kSampleRate, 440.0f, 0.6f);
    addSineWave(samples, kSampleRate, 880.0f, 0.3f);

    chord::FFTProcessor processor(kFftSize, kSampleRate);
    processor.computeMagnitudeSpectrum(samples.data(), static_cast<int>(samples.size()));

    chord::FrequencyPeak peaks[3];
    const int peakCount = processor.findTopFrequencyPeaks(peaks, 3, 75.0f, 5000.0f);

    const float binWidthHz = static_cast<float>(kSampleRate) / static_cast<float>(kFftSize);
    ASSERT_EQ(peakCount, 3);
    EXPECT_TRUE(isNearFrequency(peaks[0].frequencyHz, 220.0f, binWidthHz));
    EXPECT_TRUE(isNearFrequency(peaks[1].frequencyHz, 440.0f, binWidthHz));
    EXPECT_TRUE(isNearFrequency(peaks[2].frequencyHz, 880.0f, binWidthHz));
    EXPECT_GE(peaks[0].magnitude, peaks[1].magnitude);
    EXPECT_GE(peaks[1].magnitude, peaks[2].magnitude);
}

TEST(FFTProcessorTest, AppliesLowFrequencyFloorWhenFindingPeaks) {
    constexpr int kFftSize = 16384;
    constexpr int kSampleRate = 48000;

    std::vector<float> samples(static_cast<std::size_t>(kFftSize), 0.0f);
    addSineWave(samples, kSampleRate, 40.0f, 1.0f);
    addSineWave(samples, kSampleRate, 110.0f, 0.4f);

    chord::FFTProcessor processor(kFftSize, kSampleRate);
    processor.computeMagnitudeSpectrum(samples.data(), static_cast<int>(samples.size()));

    chord::FrequencyPeak peaks[1];
    const int peakCount = processor.findTopFrequencyPeaks(peaks, 1, 75.0f, 5000.0f);

    const float binWidthHz = static_cast<float>(kSampleRate) / static_cast<float>(kFftSize);
    ASSERT_EQ(peakCount, 1);
    EXPECT_NEAR(peaks[0].frequencyHz, 110.0f, binWidthHz);
}

TEST(FFTProcessorTest, AppliesHighFrequencyCeilingWhenFindingPeaks) {
    constexpr int kFftSize = 16384;
    constexpr int kSampleRate = 48000;

    std::vector<float> samples(static_cast<std::size_t>(kFftSize), 0.0f);
    addSineWave(samples, kSampleRate, 440.0f, 0.4f);
    addSineWave(samples, kSampleRate, 6000.0f, 1.0f);

    chord::FFTProcessor processor(kFftSize, kSampleRate);
    processor.computeMagnitudeSpectrum(samples.data(), static_cast<int>(samples.size()));

    chord::FrequencyPeak peaks[1];
    const int peakCount = processor.findTopFrequencyPeaks(peaks, 1, 75.0f, 5000.0f);

    const float binWidthHz = static_cast<float>(kSampleRate) / static_cast<float>(kFftSize);
    ASSERT_EQ(peakCount, 1);
    EXPECT_NEAR(peaks[0].frequencyHz, 440.0f, binWidthHz);
}

TEST(FFTProcessorTest, ClampsPeakSearchMaximumToNyquist) {
    constexpr int kFftSize = 4096;
    constexpr int kSampleRate = 8000;
    constexpr float kFrequencyHz = 1000.0f;

    const std::vector<float> samples = makeSineWave(kSampleRate, kFftSize, kFrequencyHz);
    chord::FFTProcessor processor(kFftSize, kSampleRate);
    processor.computeMagnitudeSpectrum(samples.data(), static_cast<int>(samples.size()));

    chord::FrequencyPeak peaks[1];
    const int peakCount = processor.findTopFrequencyPeaks(peaks, 1, 75.0f, 100000.0f);

    const float binWidthHz = static_cast<float>(kSampleRate) / static_cast<float>(kFftSize);
    ASSERT_EQ(peakCount, 1);
    EXPECT_NEAR(peaks[0].frequencyHz, kFrequencyHz, binWidthHz);
}

TEST(FFTProcessorTest, AcceptsPartialBlockWithZeroPadding) {
    constexpr int kFftSize = 1024;
    constexpr int kSampleRate = 48000;
    constexpr float kFrequencyHz = 1000.0f;

    const std::vector<float> samples = makeSineWave(kSampleRate, 256, kFrequencyHz);
    chord::FFTProcessor processor(kFftSize, kSampleRate);

    const std::vector<float>& magnitudes =
        processor.computeMagnitudeSpectrum(samples.data(), static_cast<int>(samples.size()));

    EXPECT_EQ(magnitudes.size(), static_cast<std::size_t>(processor.binCount()));
    EXPECT_GT(processor.findDominantFrequencyHz(), 0.0f);
}

TEST(FFTProcessorTest, SilentBlockHasNoDominantFrequency) {
    std::vector<float> samples(1024, 0.0f);
    chord::FFTProcessor processor(1024, 48000);

    const std::vector<float>& magnitudes =
        processor.computeMagnitudeSpectrum(samples.data(), static_cast<int>(samples.size()));

    EXPECT_EQ(magnitudes.size(), static_cast<std::size_t>(processor.binCount()));
    EXPECT_FLOAT_EQ(processor.findDominantFrequencyHz(), 0.0f);

    chord::FrequencyPeak peaks[3];
    EXPECT_EQ(processor.findTopFrequencyPeaks(peaks, 3, 75.0f, 5000.0f), 0);
}

TEST(FFTProcessorTest, RejectsInvalidSampleBlocks) {
    std::vector<float> samples(1024, 0.0f);
    chord::FFTProcessor processor(1024, 48000);

    EXPECT_THROW(processor.computeMagnitudeSpectrum(nullptr, 1024), std::invalid_argument);
    EXPECT_THROW(processor.computeMagnitudeSpectrum(samples.data(), 0), std::invalid_argument);
    EXPECT_THROW(processor.computeMagnitudeSpectrum(samples.data(), -1), std::invalid_argument);
}

TEST(FFTProcessorTest, RejectsInvalidWindowIndex) {
    chord::FFTProcessor processor(1024, 48000);

    EXPECT_THROW(processor.windowWeight(-1), std::out_of_range);
    EXPECT_THROW(processor.windowWeight(1024), std::out_of_range);
}

TEST(FFTProcessorTest, RejectsInvalidPeakSearchInputs) {
    std::vector<float> samples(1024, 0.0f);
    chord::FFTProcessor processor(1024, 48000);
    processor.computeMagnitudeSpectrum(samples.data(), static_cast<int>(samples.size()));

    chord::FrequencyPeak peaks[1];
    EXPECT_THROW(processor.findTopFrequencyPeaks(nullptr, 1, 75.0f, 5000.0f), std::invalid_argument);
    EXPECT_THROW(processor.findTopFrequencyPeaks(peaks, 1, -1.0f, 5000.0f), std::invalid_argument);
    EXPECT_THROW(processor.findTopFrequencyPeaks(peaks, 1, 5000.0f, 75.0f), std::invalid_argument);
    EXPECT_EQ(processor.findTopFrequencyPeaks(nullptr, 0, 75.0f, 5000.0f), 0);
}
