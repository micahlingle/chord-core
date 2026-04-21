#include "evaluation.h"

#include <algorithm>
#include <cmath>
#include <fstream>
#include <map>
#include <iomanip>
#include <sstream>
#include <stdexcept>
#include <utility>

namespace chord {

namespace {

constexpr double kDefaultOnsetMatchToleranceSeconds = 0.5;

std::vector<std::string> splitCsvLine(const std::string& line) {
    std::vector<std::string> fields;
    std::stringstream lineStream(line);
    std::string field;
    while (std::getline(lineStream, field, ',')) {
        fields.push_back(field);
    }
    return fields;
}

double parseTimeSeconds(const std::string& field, const char* label) {
    std::size_t processedCharacters = 0;
    const double value = std::stod(field, &processedCharacters);
    if (processedCharacters != field.size()) {
        throw std::invalid_argument(std::string(label) + " must be a numeric value");
    }
    return value;
}

void validateSpan(const ChordSpan& span) {
    if (span.startSeconds < 0.0 || span.endSeconds <= span.startSeconds) {
        throw std::invalid_argument("Chord spans must have non-negative, increasing times");
    }

    if (span.chord.empty()) {
        throw std::invalid_argument("Chord spans must have a chord name");
    }
}

std::string trimTrailingCarriageReturn(std::string value) {
    if (!value.empty() && value.back() == '\r') {
        value.pop_back();
    }
    return value;
}

double overlapDurationSeconds(const ChordSpan& left, const ChordSpan& right) {
    const double overlapStart = std::max(left.startSeconds, right.startSeconds);
    const double overlapEnd = std::min(left.endSeconds, right.endSeconds);
    return std::max(0.0, overlapEnd - overlapStart);
}

std::vector<ChordOnsetDelta> computeOnsetDeltas(const std::vector<ChordSpan>& expectedSpans,
                                                const std::vector<ChordSpan>& predictedSpans) {
    std::vector<ChordOnsetDelta> onsetDeltas;
    std::vector<bool> predictedUsed(predictedSpans.size(), false);

    for (const ChordSpan& expected : expectedSpans) {
        double bestAbsoluteDelta = kDefaultOnsetMatchToleranceSeconds;
        int bestIndex = -1;

        for (std::size_t predictedIndex = 0; predictedIndex < predictedSpans.size(); ++predictedIndex) {
            if (predictedUsed[predictedIndex]) {
                continue;
            }

            const ChordSpan& predicted = predictedSpans[predictedIndex];
            if (predicted.chord != expected.chord) {
                continue;
            }

            const double deltaSeconds = predicted.startSeconds - expected.startSeconds;
            const double absoluteDelta = std::fabs(deltaSeconds);
            if (absoluteDelta > bestAbsoluteDelta) {
                continue;
            }

            bestAbsoluteDelta = absoluteDelta;
            bestIndex = static_cast<int>(predictedIndex);
        }

        if (bestIndex >= 0) {
            predictedUsed[static_cast<std::size_t>(bestIndex)] = true;
            const ChordSpan& predicted = predictedSpans[static_cast<std::size_t>(bestIndex)];
            onsetDeltas.push_back(ChordOnsetDelta{expected.chord,
                                                  expected.startSeconds,
                                                  predicted.startSeconds,
                                                  predicted.startSeconds - expected.startSeconds});
        }
    }

    return onsetDeltas;
}

} // namespace

std::vector<ChordSpan> loadChordLabelCsv(const std::string& path) {
    std::ifstream input(path);
    if (!input.is_open()) {
        throw std::runtime_error("Failed to open label CSV: " + path);
    }

    std::string line;
    int lineNumber = 1;
    if (!std::getline(input, line)) {
        throw std::invalid_argument("Label CSV is empty");
    }

    const std::vector<std::string> header = splitCsvLine(trimTrailingCarriageReturn(line));
    if (header.size() != 3U || header[0] != "start_seconds" || header[1] != "end_seconds" || header[2] != "chord") {
        throw std::invalid_argument("Label CSV line 1 must be exactly: start_seconds,end_seconds,chord");
    }

    std::vector<ChordSpan> spans;
    double previousStartSeconds = -1.0;
    while (std::getline(input, line)) {
        ++lineNumber;
        line = trimTrailingCarriageReturn(line);
        if (line.empty()) {
            continue;
        }

        const std::vector<std::string> fields = splitCsvLine(line);
        if (fields.size() != 3U) {
            throw std::invalid_argument("Label CSV line " + std::to_string(lineNumber) +
                                        " must contain exactly three fields: " + line);
        }

        ChordSpan span{};
        try {
            span = ChordSpan{
                parseTimeSeconds(fields[0], "start_seconds"),
                parseTimeSeconds(fields[1], "end_seconds"),
                fields[2],
            };
            validateSpan(span);
        } catch (const std::exception& exception) {
            throw std::invalid_argument("Label CSV line " + std::to_string(lineNumber) + " is invalid: " + line + " (" +
                                        exception.what() + ")");
        }

        if (!spans.empty() && span.startSeconds < previousStartSeconds) {
            std::ostringstream message;
            message << std::fixed << std::setprecision(3);
            message << "Label CSV line " << lineNumber << " starts at " << span.startSeconds
                    << "s, which is earlier than the previous start time of " << previousStartSeconds << "s: " << line;
            throw std::invalid_argument(message.str());
        }

        previousStartSeconds = span.startSeconds;
        spans.push_back(span);
    }

    return spans;
}

LabelCsvValidationSummary summarizeChordSpans(const std::vector<ChordSpan>& spans) {
    LabelCsvValidationSummary summary;
    if (spans.empty()) {
        return summary;
    }

    for (const ChordSpan& span : spans) {
        validateSpan(span);
        summary.labeledDurationSeconds += span.endSeconds - span.startSeconds;
    }

    summary.spanCount = spans.size();
    summary.firstStartSeconds = spans.front().startSeconds;
    summary.lastEndSeconds = spans.back().endSeconds;
    return summary;
}

std::vector<ChordSpan> mergeAdjacentChordSpans(const std::vector<ChordSpan>& spans, bool includeUnknown) {
    std::vector<ChordSpan> merged;
    merged.reserve(spans.size());

    for (const ChordSpan& span : spans) {
        validateSpan(span);

        if (span.chord == "Unknown" && !includeUnknown) {
            continue;
        }

        if (!merged.empty() && merged.back().chord == span.chord &&
            std::fabs(merged.back().endSeconds - span.startSeconds) < 1.0e-9) {
            merged.back().endSeconds = span.endSeconds;
            continue;
        }

        merged.push_back(span);
    }

    return merged;
}

std::vector<ChordSpan>
buildPredictedChordSpans(const AudioFileData& audioFile, ChordDetector& detector, int blockSize, bool includeUnknown) {
    if (blockSize <= 0) {
        throw std::invalid_argument("blockSize must be positive");
    }

    if (audioFile.sampleRate <= 0) {
        throw std::invalid_argument("audioFile.sampleRate must be positive");
    }

    std::vector<ChordSpan> blockSpans;
    const std::size_t totalBlocks =
        (audioFile.samples.size() + static_cast<std::size_t>(blockSize) - 1U) / static_cast<std::size_t>(blockSize);
    blockSpans.reserve(totalBlocks);

    for (std::size_t blockIndex = 0; blockIndex < totalBlocks; ++blockIndex) {
        const std::size_t start = blockIndex * static_cast<std::size_t>(blockSize);
        const std::size_t remaining = audioFile.samples.size() - start;
        const int blockSamples = static_cast<int>(
            remaining < static_cast<std::size_t>(blockSize) ? remaining : static_cast<std::size_t>(blockSize));

        detector.processBlock(audioFile.samples.data() + start, blockSamples);
        const ChordResult result = detector.getCurrentChord();
        const double startSeconds = static_cast<double>(start) / static_cast<double>(audioFile.sampleRate);
        const double endSeconds = static_cast<double>(start + static_cast<std::size_t>(blockSamples)) /
                                  static_cast<double>(audioFile.sampleRate);
        blockSpans.push_back(ChordSpan{startSeconds, endSeconds, result.name});
    }

    return mergeAdjacentChordSpans(blockSpans, includeUnknown);
}

void writeChordLabelCsv(const std::string& path, const std::vector<ChordSpan>& spans) {
    std::ofstream output(path);
    if (!output.is_open()) {
        throw std::runtime_error("Failed to open CSV output path: " + path);
    }

    output << "start_seconds,end_seconds,chord\n";
    output.setf(std::ios::fixed);
    output.precision(3);
    for (const ChordSpan& span : spans) {
        validateSpan(span);
        output << span.startSeconds << ',' << span.endSeconds << ',' << span.chord << '\n';
    }
}

EvaluationMetrics evaluateChordSpans(const std::vector<ChordSpan>& expectedSpans,
                                     const std::vector<ChordSpan>& predictedSpans) {
    for (const ChordSpan& span : expectedSpans) {
        validateSpan(span);
    }
    for (const ChordSpan& span : predictedSpans) {
        validateSpan(span);
    }

    EvaluationMetrics metrics;
    metrics.onsetDeltas = computeOnsetDeltas(expectedSpans, predictedSpans);

    std::map<std::pair<std::string, std::string>, double> confusionDurations;
    std::size_t predictedIndex = 0;
    for (const ChordSpan& expected : expectedSpans) {
        metrics.labeledDurationSeconds += expected.endSeconds - expected.startSeconds;

        while (predictedIndex < predictedSpans.size() &&
               predictedSpans[predictedIndex].endSeconds <= expected.startSeconds) {
            ++predictedIndex;
        }

        std::size_t scanIndex = predictedIndex;
        bool matchedExpectedSpan = false;
        while (scanIndex < predictedSpans.size() && predictedSpans[scanIndex].startSeconds < expected.endSeconds) {
            const ChordSpan& predicted = predictedSpans[scanIndex];
            const double overlapSeconds = overlapDurationSeconds(expected, predicted);
            if (overlapSeconds <= 0.0) {
                ++scanIndex;
                continue;
            }

            if (predicted.chord == expected.chord) {
                metrics.matchingDurationSeconds += overlapSeconds;
                matchedExpectedSpan = true;
            } else {
                confusionDurations[{expected.chord, predicted.chord}] += overlapSeconds;
            }

            ++scanIndex;
        }

        if (!matchedExpectedSpan) {
            metrics.mismatches.push_back(expected);
        }
    }

    metrics.overlapAccuracy =
        metrics.labeledDurationSeconds > 0.0 ? metrics.matchingDurationSeconds / metrics.labeledDurationSeconds : 0.0;

    for (const auto& confusion : confusionDurations) {
        metrics.confusions.push_back(
            ChordConfusionDuration{confusion.first.first, confusion.first.second, confusion.second});
    }

    std::sort(metrics.confusions.begin(),
              metrics.confusions.end(),
              [](const ChordConfusionDuration& left, const ChordConfusionDuration& right) {
                  return left.durationSeconds > right.durationSeconds;
              });

    return metrics;
}

bool meetsMinimumAccuracy(const EvaluationMetrics& metrics, double minimumAccuracy) {
    if (minimumAccuracy < 0.0 || minimumAccuracy > 1.0) {
        throw std::invalid_argument("minimumAccuracy must be within [0, 1]");
    }

    return metrics.overlapAccuracy >= minimumAccuracy;
}

} // namespace chord
