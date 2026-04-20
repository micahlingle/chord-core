#include "chroma_extractor.h"

#include <stdexcept>
#include <vector>

#include <gtest/gtest.h>

namespace {

std::vector<float> makeMagnitudeSpectrum(int fftSize) {
    return std::vector<float>(static_cast<std::size_t>(fftSize / 2 + 1), 0.0f);
}

int frequencyToBin(float frequencyHz, int fftSize, int sampleRate) {
    const float binWidthHz = static_cast<float>(sampleRate) / static_cast<float>(fftSize);
    return static_cast<int>(frequencyHz / binWidthHz + 0.5f);
}

} // namespace

TEST(ChromaExtractorTest, RejectsInvalidConfiguration) {
    EXPECT_THROW(chord::ChromaExtractor(0, 48000), std::invalid_argument);
    EXPECT_THROW(chord::ChromaExtractor(-1024, 48000), std::invalid_argument);
    EXPECT_THROW(chord::ChromaExtractor(1024, 0), std::invalid_argument);
    EXPECT_THROW(chord::ChromaExtractor(1024, -48000), std::invalid_argument);
}

TEST(ChromaExtractorTest, ReportsConfiguration) {
    chord::ChromaExtractor extractor(4096, 48000);

    EXPECT_EQ(extractor.fftSize(), 4096);
    EXPECT_EQ(extractor.sampleRate(), 48000);
    EXPECT_EQ(extractor.binCount(), 2049);
}

TEST(ChromaExtractorTest, MapsReferenceFrequenciesToPitchClasses) {
    chord::ChromaExtractor extractor(4096, 48000);

    EXPECT_EQ(extractor.frequencyToPitchClass(440.0f), chord::kPitchClassA);
    EXPECT_EQ(extractor.frequencyToPitchClass(261.63f), chord::kPitchClassC);
    EXPECT_EQ(extractor.frequencyToPitchClass(329.63f), chord::kPitchClassE);
    EXPECT_EQ(extractor.frequencyToPitchClass(82.41f), chord::kPitchClassE);
}

TEST(ChromaExtractorTest, RejectsInvalidFrequencyForPitchClassMapping) {
    chord::ChromaExtractor extractor(4096, 48000);

    EXPECT_THROW(extractor.frequencyToPitchClass(0.0f), std::invalid_argument);
    EXPECT_THROW(extractor.frequencyToPitchClass(-440.0f), std::invalid_argument);
}

TEST(ChromaExtractorTest, ExtractsNormalizedChromaFromMagnitudeSpectrum) {
    constexpr int kFftSize = 4096;
    constexpr int kSampleRate = 48000;

    chord::ChromaExtractor extractor(kFftSize, kSampleRate);
    std::vector<float> magnitudes = makeMagnitudeSpectrum(kFftSize);

    magnitudes[static_cast<std::size_t>(frequencyToBin(440.0f, kFftSize, kSampleRate))] = 2.0f;
    magnitudes[static_cast<std::size_t>(frequencyToBin(261.63f, kFftSize, kSampleRate))] = 1.0f;

    const chord::ChromaVector chroma = extractor.extract(magnitudes, 75.0f, 5000.0f);

    EXPECT_FLOAT_EQ(chroma[static_cast<std::size_t>(chord::kPitchClassA)], 1.0f);
    EXPECT_GT(chroma[static_cast<std::size_t>(chord::kPitchClassC)], 0.0f);
    EXPECT_LT(chroma[static_cast<std::size_t>(chord::kPitchClassC)], 1.0f);
    for (float value : chroma) {
        EXPECT_LE(value, 1.0f);
    }
}

TEST(ChromaExtractorTest, ReturnsZeroChromaForSilence) {
    chord::ChromaExtractor extractor(4096, 48000);
    const std::vector<float> magnitudes = makeMagnitudeSpectrum(4096);

    const chord::ChromaVector chroma = extractor.extract(magnitudes, 75.0f, 5000.0f);

    for (float value : chroma) {
        EXPECT_FLOAT_EQ(value, 0.0f);
    }
}

TEST(ChromaExtractorTest, AppliesFrequencyRange) {
    constexpr int kFftSize = 4096;
    constexpr int kSampleRate = 48000;

    chord::ChromaExtractor extractor(kFftSize, kSampleRate);
    std::vector<float> magnitudes = makeMagnitudeSpectrum(kFftSize);

    magnitudes[static_cast<std::size_t>(frequencyToBin(40.0f, kFftSize, kSampleRate))] = 10.0f;
    magnitudes[static_cast<std::size_t>(frequencyToBin(440.0f, kFftSize, kSampleRate))] = 1.0f;
    magnitudes[static_cast<std::size_t>(frequencyToBin(6000.0f, kFftSize, kSampleRate))] = 10.0f;

    const chord::ChromaVector chroma = extractor.extract(magnitudes, 75.0f, 5000.0f);

    EXPECT_FLOAT_EQ(chroma[static_cast<std::size_t>(chord::kPitchClassA)], 1.0f);
    EXPECT_FLOAT_EQ(chroma[static_cast<std::size_t>(chord::kPitchClassE)], 0.0f);
}

TEST(ChromaExtractorTest, RejectsInvalidExtractionInputs) {
    chord::ChromaExtractor extractor(4096, 48000);
    const std::vector<float> wrongSizeMagnitudes(8, 0.0f);
    const std::vector<float> magnitudes = makeMagnitudeSpectrum(4096);

    EXPECT_THROW(extractor.extract(wrongSizeMagnitudes, 75.0f, 5000.0f), std::invalid_argument);
    EXPECT_THROW(extractor.extract(magnitudes, -1.0f, 5000.0f), std::invalid_argument);
    EXPECT_THROW(extractor.extract(magnitudes, 5000.0f, 75.0f), std::invalid_argument);
}
