#include "audio_loader.h"
#include "fft_processor.h"
#include "signal_analysis.h"

#include <array>
#include <cstddef>
#include <exception>
#include <iomanip>
#include <iostream>
#include <string>

namespace {

constexpr int kBlockSize = 1024;
constexpr int kFftSize = 16384;
constexpr int kTopFrequencyCount = 3;
constexpr float kMinFrequencyHz = 75.0f;
constexpr float kMaxFrequencyHz = 5000.0f;
constexpr float kActivityRmsThreshold = 0.01f;

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

        chord::FFTProcessor fftProcessor(kFftSize, audioFile.sampleRate);

        const std::size_t totalBlocks =
            (audioFile.samples.size() + static_cast<std::size_t>(kBlockSize) - 1U) /
            static_cast<std::size_t>(kBlockSize);

        for (std::size_t blockIndex = 0; blockIndex < totalBlocks; ++blockIndex) {
            const std::size_t start = blockIndex * static_cast<std::size_t>(kBlockSize);
            const std::size_t remaining = audioFile.samples.size() - start;
            const int blockSamples =
                static_cast<int>(remaining < static_cast<std::size_t>(kBlockSize) ? remaining : kBlockSize);
            const int fftSamples =
                static_cast<int>(remaining < static_cast<std::size_t>(kFftSize) ? remaining : kFftSize);

            const float rms = chord::computeRms(audioFile.samples.data() + start, blockSamples);
            const bool isActive = chord::isSignalActive(rms, kActivityRmsThreshold);
            float dominantFrequencyHz = 0.0f;
            std::array<chord::FrequencyPeak, kTopFrequencyCount> topPeaks{};
            int topPeakCount = 0;
            if (isActive) {
                fftProcessor.computeMagnitudeSpectrum(audioFile.samples.data() + start, fftSamples);
                dominantFrequencyHz = fftProcessor.findDominantFrequencyHz();
                topPeakCount = fftProcessor.findTopFrequencyPeaks(topPeaks.data(),
                                                                  static_cast<int>(topPeaks.size()),
                                                                  kMinFrequencyHz,
                                                                  kMaxFrequencyHz);
            }
            const double startTimeSeconds =
                static_cast<double>(start) / static_cast<double>(audioFile.sampleRate);

            std::cout << "Block " << blockIndex
                      << " start=" << std::fixed << std::setprecision(3) << startTimeSeconds << "s"
                      << " samples=" << blockSamples
                      << " rms=" << std::setprecision(6) << rms
                      << " active=" << (isActive ? "yes" : "no")
                      << " dominant_frequency_hz=" << std::setprecision(2) << dominantFrequencyHz
                      << " top_frequencies_hz=[";
            for (int peakIndex = 0; peakIndex < topPeakCount; ++peakIndex) {
                if (peakIndex > 0) {
                    std::cout << ',';
                }
                std::cout << std::setprecision(2) << topPeaks[static_cast<std::size_t>(peakIndex)].frequencyHz;
            }
            std::cout << "]\n";
        }
    } catch (const std::exception& exception) {
        std::cerr << "Error: " << exception.what() << '\n';
        return 1;
    }

    return 0;
}
