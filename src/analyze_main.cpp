#include "audio_loader.h"
#include "chroma_extractor.h"
#include "chord_template_matcher.h"
#include "fft_processor.h"
#include "signal_analysis.h"

#include <array>
#include <cstddef>
#include <exception>
#include <iomanip>
#include <iostream>
#include <string>
#include <vector>

namespace {

constexpr int kBlockSize = 1024;
constexpr int kFftSize = 16384;
constexpr int kTopFrequencyCount = 3;
constexpr float kMinFrequencyHz = 75.0f;
constexpr float kMaxFrequencyHz = 5000.0f;
constexpr float kActivityRmsThreshold = 0.01f;
constexpr std::array<const char*, 12> kPitchClassNames = {
    "C",
    "C#",
    "D",
    "D#",
    "E",
    "F",
    "F#",
    "G",
    "G#",
    "A",
    "A#",
    "B",
};

} // namespace

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cerr << "Usage: chord_core_analyze <path-to-wav>\n";
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
        chord::ChromaExtractor chromaExtractor(kFftSize, audioFile.sampleRate);
        chord::ChordTemplateMatcher chordMatcher;

        const std::size_t totalBlocks = (audioFile.samples.size() + static_cast<std::size_t>(kBlockSize) - 1U) /
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
            chord::ChromaVector chroma{};
            chord::ChordResult chordResult{};
            if (isActive) {
                const std::vector<float>& magnitudes =
                    fftProcessor.computeMagnitudeSpectrum(audioFile.samples.data() + start, fftSamples);
                dominantFrequencyHz = fftProcessor.findDominantFrequencyHz();
                topPeakCount = fftProcessor.findTopFrequencyPeaks(topPeaks.data(),
                                                                  static_cast<int>(topPeaks.size()),
                                                                  kMinFrequencyHz,
                                                                  kMaxFrequencyHz);
                chroma = chromaExtractor.extract(magnitudes, kMinFrequencyHz, kMaxFrequencyHz);
                chordResult = chordMatcher.match(chroma);
            }
            const double startTimeSeconds = static_cast<double>(start) / static_cast<double>(audioFile.sampleRate);

            // clang-format sees stream output as one long binary expression and
            // tends to regroup fields by column width. Keep this diagnostic
            // output laid out by semantic field instead.
            // clang-format off
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
            std::cout << "] chroma=[";
            for (std::size_t pitchClass = 0; pitchClass < chroma.size(); ++pitchClass) {
                if (pitchClass > 0U) {
                    std::cout << ',';
                }
                std::cout << kPitchClassNames[pitchClass]
                          << '=' << std::setprecision(2) << chroma[pitchClass];
            }
            std::cout << "] chord=\"" << chordResult.name << '"'
                      << " confidence=" << std::setprecision(2) << chordResult.confidence
                      << '\n';
            // clang-format on
        }
    } catch (const std::exception& exception) {
        std::cerr << "Error: " << exception.what() << '\n';
        return 1;
    }

    return 0;
}
