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

}  // namespace

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
    EXPECT_EQ(processor.binCount(), 513);
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
}

TEST(FFTProcessorTest, RejectsInvalidSampleBlocks) {
    std::vector<float> samples(1024, 0.0f);
    chord::FFTProcessor processor(1024, 48000);

    EXPECT_THROW(processor.computeMagnitudeSpectrum(nullptr, 1024), std::invalid_argument);
    EXPECT_THROW(processor.computeMagnitudeSpectrum(samples.data(), 0), std::invalid_argument);
    EXPECT_THROW(processor.computeMagnitudeSpectrum(samples.data(), -1), std::invalid_argument);
}
