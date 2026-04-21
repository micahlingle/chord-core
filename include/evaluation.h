#pragma once

#include "audio_loader.h"
#include "chord_detector.h"

#include <string>
#include <vector>

namespace chord {

struct ChordSpan {
    double startSeconds = 0.0;
    double endSeconds = 0.0;
    std::string chord = "Unknown";
};

struct ChordConfusionDuration {
    std::string expectedChord;
    std::string predictedChord;
    double durationSeconds = 0.0;
};

struct ChordOnsetDelta {
    std::string chord;
    double expectedStartSeconds = 0.0;
    double predictedStartSeconds = 0.0;
    double deltaSeconds = 0.0;
};

struct EvaluationMetrics {
    double labeledDurationSeconds = 0.0;
    double matchingDurationSeconds = 0.0;
    double overlapAccuracy = 0.0;
    std::vector<ChordConfusionDuration> confusions;
    std::vector<ChordSpan> mismatches;
    std::vector<ChordOnsetDelta> onsetDeltas;
};

struct LabelCsvValidationSummary {
    std::size_t spanCount = 0;
    double labeledDurationSeconds = 0.0;
    double firstStartSeconds = 0.0;
    double lastEndSeconds = 0.0;
};

std::vector<ChordSpan> loadChordLabelCsv(const std::string& path);
LabelCsvValidationSummary summarizeChordSpans(const std::vector<ChordSpan>& spans);
std::vector<ChordSpan> buildPredictedChordSpans(const AudioFileData& audioFile,
                                                ChordDetector& detector,
                                                int blockSize,
                                                bool includeUnknown = false);
std::vector<ChordSpan> mergeAdjacentChordSpans(const std::vector<ChordSpan>& spans, bool includeUnknown);
void writeChordLabelCsv(const std::string& path, const std::vector<ChordSpan>& spans);
EvaluationMetrics evaluateChordSpans(const std::vector<ChordSpan>& expectedSpans,
                                     const std::vector<ChordSpan>& predictedSpans);
bool meetsMinimumAccuracy(const EvaluationMetrics& metrics, double minimumAccuracy);

} // namespace chord
