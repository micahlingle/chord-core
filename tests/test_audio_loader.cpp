#include "audio_loader.h"

#include <sndfile.h>

#include <filesystem>
#include <stdexcept>
#include <string>
#include <vector>

#include <gtest/gtest.h>

namespace {

std::filesystem::path makeTempWavPath() {
    const auto testInfo = ::testing::UnitTest::GetInstance()->current_test_info();
    const std::string fileName = std::string("chord_core_") + testInfo->test_suite_name() + "_" +
                                 testInfo->name() + ".wav";
    return std::filesystem::temp_directory_path() / fileName;
}

void writeFloatWav(const std::filesystem::path& path,
                   const std::vector<float>& interleavedSamples,
                   int sampleRate,
                   int channelCount) {
    SF_INFO fileInfo{};
    fileInfo.samplerate = sampleRate;
    fileInfo.channels = channelCount;
    fileInfo.format = SF_FORMAT_WAV | SF_FORMAT_FLOAT;

    SNDFILE* file = sf_open(path.string().c_str(), SFM_WRITE, &fileInfo);
    ASSERT_NE(file, nullptr);

    const auto frameCount = static_cast<sf_count_t>(interleavedSamples.size() /
                                                   static_cast<std::size_t>(channelCount));
    const sf_count_t framesWritten = sf_writef_float(file, interleavedSamples.data(), frameCount);
    sf_close(file);

    ASSERT_EQ(framesWritten, frameCount);
}

}  // namespace

TEST(AudioFileDataTest, DefaultsToEmptyAudio) {
    const chord::AudioFileData audioFileData;

    EXPECT_TRUE(audioFileData.samples.empty());
    EXPECT_EQ(audioFileData.sampleRate, 0);
    EXPECT_EQ(audioFileData.channelCount, 0);
}

TEST(AudioLoaderTest, DownMixesStereoInterleavedSamplesToMono) {
    const std::vector<float> stereoSamples = {
        0.20f, 0.40f,
        -0.50f, 0.10f,
        1.00f, -1.00f,
    };

    const std::vector<float> monoSamples =
        chord::downMixInterleavedToMono(stereoSamples.data(), 3, 2);

    ASSERT_EQ(monoSamples.size(), 3U);
    EXPECT_FLOAT_EQ(monoSamples[0], 0.30f);
    EXPECT_FLOAT_EQ(monoSamples[1], -0.20f);
    EXPECT_FLOAT_EQ(monoSamples[2], 0.00f);
}

TEST(AudioLoaderTest, RejectsInvalidDownMixInputs) {
    const std::vector<float> monoSamples = {0.1f, 0.2f};

    EXPECT_THROW(chord::downMixInterleavedToMono(monoSamples.data(), monoSamples.size(), 0),
                 std::invalid_argument);
    EXPECT_THROW(chord::downMixInterleavedToMono(nullptr, 1, 1), std::invalid_argument);
}

TEST(AudioLoaderTest, LoadsWavAndDownMixesToMono) {
    const std::filesystem::path path = makeTempWavPath();
    const std::vector<float> stereoSamples = {
        0.20f, 0.40f,
        -0.50f, 0.10f,
        0.75f, 0.25f,
    };

    writeFloatWav(path, stereoSamples, 48000, 2);

    chord::AudioLoader loader;
    const chord::AudioFileData loadedFile = loader.loadWavFile(path.string());

    EXPECT_EQ(loadedFile.sampleRate, 48000);
    EXPECT_EQ(loadedFile.channelCount, 2);
    ASSERT_EQ(loadedFile.samples.size(), 3U);
    EXPECT_NEAR(loadedFile.samples[0], 0.30f, 1.0e-6f);
    EXPECT_NEAR(loadedFile.samples[1], -0.20f, 1.0e-6f);
    EXPECT_NEAR(loadedFile.samples[2], 0.50f, 1.0e-6f);

    std::filesystem::remove(path);
}
