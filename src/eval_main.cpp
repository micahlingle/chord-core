#include "audio_loader.h"
#include "chord_detector.h"
#include "evaluation.h"

#include <algorithm>
#include <exception>
#include <iomanip>
#include <iostream>
#include <optional>
#include <string>

namespace {

constexpr int kDefaultBlockSize = 1024;
constexpr double kDefaultMinimumAccuracy = 0.70;

struct EvalOptions {
    std::string wavPath;
    std::string labelPath;
    std::optional<std::string> predictedCsvOutputPath;
    int blockSize = kDefaultBlockSize;
    double minimumAccuracy = kDefaultMinimumAccuracy;
};

void printUsage() {
    std::cerr << "Usage: chord_core_eval --wav <path> --labels <path> "
                 "[--block-size <samples>] [--min-accuracy <0..1>] [--dump-predicted-csv <path>]\n";
}

EvalOptions parseArgs(int argc, char** argv) {
    EvalOptions options;

    for (int index = 1; index < argc; ++index) {
        const std::string argument = argv[index];
        if (argument == "--wav" && index + 1 < argc) {
            options.wavPath = argv[++index];
        } else if (argument == "--labels" && index + 1 < argc) {
            options.labelPath = argv[++index];
        } else if (argument == "--block-size" && index + 1 < argc) {
            options.blockSize = std::stoi(argv[++index]);
        } else if (argument == "--min-accuracy" && index + 1 < argc) {
            options.minimumAccuracy = std::stod(argv[++index]);
        } else if (argument == "--dump-predicted-csv" && index + 1 < argc) {
            options.predictedCsvOutputPath = argv[++index];
        } else {
            throw std::invalid_argument("Unknown or incomplete argument: " + argument);
        }
    }

    if (options.wavPath.empty() || options.labelPath.empty()) {
        throw std::invalid_argument("--wav and --labels are required");
    }

    return options;
}

} // namespace

int main(int argc, char** argv) {
    try {
        const EvalOptions options = parseArgs(argc, argv);

        chord::AudioLoader loader;
        const chord::AudioFileData audioFile = loader.loadWavFile(options.wavPath);
        const std::vector<chord::ChordSpan> expectedSpans = chord::loadChordLabelCsv(options.labelPath);
        chord::ChordDetector detector(chord::ChordDetector::kDefaultFftSize, audioFile.sampleRate);
        const std::vector<chord::ChordSpan> predictedSpans =
            chord::buildPredictedChordSpans(audioFile, detector, options.blockSize, false);
        const chord::EvaluationMetrics metrics = chord::evaluateChordSpans(expectedSpans, predictedSpans);

        if (options.predictedCsvOutputPath.has_value()) {
            chord::writeChordLabelCsv(*options.predictedCsvOutputPath, predictedSpans);
        }

        std::cout << "WAV: " << options.wavPath << '\n';
        std::cout << "Labels: " << options.labelPath << '\n';
        std::cout << "Block size: " << options.blockSize << '\n';
        std::cout << "Labeled duration: " << std::fixed << std::setprecision(3) << metrics.labeledDurationSeconds
                  << "s\n";
        std::cout << "Matching duration: " << metrics.matchingDurationSeconds << "s\n";
        std::cout << "Overlap accuracy: " << std::setprecision(4) << metrics.overlapAccuracy << '\n';
        std::cout << "Minimum required accuracy: " << options.minimumAccuracy << '\n';
        std::cout << "Predicted spans: " << predictedSpans.size() << '\n';
        std::cout << "Onset matches: " << metrics.onsetDeltas.size() << '\n';

        if (!metrics.confusions.empty()) {
            std::cout << "Top confusions:\n";
            const std::size_t confusionCount = std::min<std::size_t>(5, metrics.confusions.size());
            for (std::size_t index = 0; index < confusionCount; ++index) {
                const chord::ChordConfusionDuration& confusion = metrics.confusions[index];
                std::cout << "  expected=\"" << confusion.expectedChord << "\" predicted=\"" << confusion.predictedChord
                          << "\" duration=" << std::setprecision(3) << confusion.durationSeconds << "s\n";
            }
        }

        if (!metrics.onsetDeltas.empty()) {
            std::cout << "Sample onset deltas:\n";
            const std::size_t onsetCount = std::min<std::size_t>(5, metrics.onsetDeltas.size());
            for (std::size_t index = 0; index < onsetCount; ++index) {
                const chord::ChordOnsetDelta& onsetDelta = metrics.onsetDeltas[index];
                std::cout << "  chord=\"" << onsetDelta.chord << "\" expected=" << onsetDelta.expectedStartSeconds
                          << "s predicted=" << onsetDelta.predictedStartSeconds << "s delta=" << onsetDelta.deltaSeconds
                          << "s\n";
            }
        }

        if (!metrics.mismatches.empty()) {
            std::cout << "Unmatched labeled spans:\n";
            const std::size_t mismatchCount = std::min<std::size_t>(5, metrics.mismatches.size());
            for (std::size_t index = 0; index < mismatchCount; ++index) {
                const chord::ChordSpan& mismatch = metrics.mismatches[index];
                std::cout << "  chord=\"" << mismatch.chord << "\" start=" << mismatch.startSeconds
                          << "s end=" << mismatch.endSeconds << "s\n";
            }
        }

        if (!chord::meetsMinimumAccuracy(metrics, options.minimumAccuracy)) {
            std::cerr << "Dataset evaluation failed: overlap accuracy " << metrics.overlapAccuracy
                      << " is below the required threshold of " << options.minimumAccuracy << '\n';
            return 1;
        }

        return 0;
    } catch (const std::exception& exception) {
        printUsage();
        std::cerr << "Error: " << exception.what() << '\n';
        return 1;
    }
}
