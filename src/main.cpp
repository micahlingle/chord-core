#include "audio_loader.h"
#include "chord_detector.h"

#include <cstddef>
#include <exception>
#include <iomanip>
#include <iostream>
#include <string>

namespace {

constexpr int kBlockSize = 1024;

}  // namespace

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cerr << "Usage: chord_core_demo <path-to-wav>\n";
        return 1;
    }

    const std::string inputPath = argv[1];

    try {
        chord::AudioLoader loader;
        const chord::AudioFileData audioFile = loader.loadWavFile(inputPath);

        std::cout << "Loaded " << inputPath << '\n';
        std::cout << "Sample rate: " << audioFile.sampleRate << " Hz\n";
        std::cout << "Channels: " << audioFile.channelCount << " (processed as mono)\n";
        std::cout << "Samples: " << audioFile.samples.size() << '\n';

        chord::ChordDetector detector(chord::ChordDetector::kDefaultFftSize, audioFile.sampleRate);

        const std::size_t totalBlocks =
            (audioFile.samples.size() + static_cast<std::size_t>(kBlockSize) - 1U) /
            static_cast<std::size_t>(kBlockSize);

        for (std::size_t blockIndex = 0; blockIndex < totalBlocks; ++blockIndex) {
            const std::size_t start = blockIndex * static_cast<std::size_t>(kBlockSize);
            const std::size_t remaining = audioFile.samples.size() - start;
            const int blockSamples =
                static_cast<int>(remaining < static_cast<std::size_t>(kBlockSize) ? remaining : kBlockSize);

            detector.processBlock(audioFile.samples.data() + start, blockSamples);
            const chord::ChordResult chordResult = detector.getCurrentChord();
            const double startTimeSeconds =
                static_cast<double>(start) / static_cast<double>(audioFile.sampleRate);

            std::cout << "Block " << blockIndex
                      << " start=" << std::fixed << std::setprecision(3) << startTimeSeconds << "s"
                      << " samples=" << blockSamples
                      << " chord=\"" << chordResult.name << '"'
                      << " confidence=" << std::setprecision(2) << chordResult.confidence
                      << '\n';
        }
    } catch (const std::exception& exception) {
        std::cerr << "Error: " << exception.what() << '\n';
        return 1;
    }

    return 0;
}
