#include "evaluation.h"

#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <string>
#include <vector>

#include <gtest/gtest.h>

namespace {

std::filesystem::path writeTemporaryLabelCsv(const std::string& filename, const std::string& contents) {
    const std::filesystem::path path = std::filesystem::temp_directory_path() / filename;
    std::ofstream output(path);
    output << contents;
    output.close();
    return path;
}

} // namespace

TEST(EvaluationTest, ParsesLabelCsv) {
    const std::filesystem::path path =
        writeTemporaryLabelCsv("chord_core_eval_labels.csv", "start_seconds,end_seconds,chord\n0.000,1.000,C major\n");

    const std::vector<chord::ChordSpan> spans = chord::loadChordLabelCsv(path.string());

    ASSERT_EQ(spans.size(), 1U);
    EXPECT_DOUBLE_EQ(spans[0].startSeconds, 0.0);
    EXPECT_DOUBLE_EQ(spans[0].endSeconds, 1.0);
    EXPECT_EQ(spans[0].chord, "C major");
}

TEST(EvaluationTest, RejectsInvalidLabelCsvRows) {
    const std::filesystem::path path = writeTemporaryLabelCsv("chord_core_eval_invalid_labels.csv",
                                                              "start_seconds,end_seconds,chord\n0.500,0.250,C major\n");

    try {
        static_cast<void>(chord::loadChordLabelCsv(path.string()));
        FAIL() << "Expected invalid_argument";
    } catch (const std::invalid_argument& exception) {
        EXPECT_NE(std::string(exception.what()).find("line 2"), std::string::npos);
        EXPECT_NE(std::string(exception.what()).find("0.500,0.250,C major"), std::string::npos);
    }
}

TEST(EvaluationTest, ReportsOutOfOrderLabelLinesClearly) {
    const std::filesystem::path path =
        writeTemporaryLabelCsv("chord_core_eval_unsorted_labels.csv",
                               "start_seconds,end_seconds,chord\n1.000,2.000,C major\n0.500,1.500,G major\n");

    try {
        static_cast<void>(chord::loadChordLabelCsv(path.string()));
        FAIL() << "Expected invalid_argument";
    } catch (const std::invalid_argument& exception) {
        EXPECT_NE(std::string(exception.what()).find("line 3"), std::string::npos);
        EXPECT_NE(std::string(exception.what()).find("0.500,1.500,G major"), std::string::npos);
    }
}

TEST(EvaluationTest, SummarizesValidatedSpans) {
    const std::vector<chord::ChordSpan> spans{
        {0.0, 1.0, "C major"},
        {2.0, 3.5, "G major"},
    };

    const chord::LabelCsvValidationSummary summary = chord::summarizeChordSpans(spans);

    EXPECT_EQ(summary.spanCount, 2U);
    EXPECT_DOUBLE_EQ(summary.labeledDurationSeconds, 2.5);
    EXPECT_DOUBLE_EQ(summary.firstStartSeconds, 0.0);
    EXPECT_DOUBLE_EQ(summary.lastEndSeconds, 3.5);
}

TEST(EvaluationTest, ComputesOverlapAccuracyFromHandcraftedSpans) {
    const std::vector<chord::ChordSpan> expected{
        {0.0, 1.0, "C major"},
        {1.0, 2.0, "G major"},
    };
    const std::vector<chord::ChordSpan> predicted{
        {0.0, 0.5, "C major"},
        {0.5, 1.5, "G major"},
        {1.5, 2.0, "G major"},
    };

    const chord::EvaluationMetrics metrics = chord::evaluateChordSpans(expected, predicted);

    EXPECT_DOUBLE_EQ(metrics.labeledDurationSeconds, 2.0);
    EXPECT_DOUBLE_EQ(metrics.matchingDurationSeconds, 1.5);
    EXPECT_DOUBLE_EQ(metrics.overlapAccuracy, 0.75);
    ASSERT_EQ(metrics.confusions.size(), 1U);
    EXPECT_EQ(metrics.confusions[0].expectedChord, "C major");
    EXPECT_EQ(metrics.confusions[0].predictedChord, "G major");
    EXPECT_DOUBLE_EQ(metrics.confusions[0].durationSeconds, 0.5);
}

TEST(EvaluationTest, IgnoresUnlabeledGaps) {
    const std::vector<chord::ChordSpan> expected{
        {0.0, 1.0, "C major"},
        {2.0, 3.0, "G major"},
    };
    const std::vector<chord::ChordSpan> predicted{
        {0.0, 2.5, "C major"},
        {2.5, 3.0, "G major"},
    };

    const chord::EvaluationMetrics metrics = chord::evaluateChordSpans(expected, predicted);

    EXPECT_DOUBLE_EQ(metrics.labeledDurationSeconds, 2.0);
    EXPECT_DOUBLE_EQ(metrics.matchingDurationSeconds, 1.5);
    EXPECT_DOUBLE_EQ(metrics.overlapAccuracy, 0.75);
}

TEST(EvaluationTest, ReportsOnsetDeltasForMatchingChords) {
    const std::vector<chord::ChordSpan> expected{
        {0.0, 1.0, "C major"},
        {1.0, 2.0, "G major"},
    };
    const std::vector<chord::ChordSpan> predicted{
        {0.064, 1.0, "C major"},
        {1.128, 2.0, "G major"},
    };

    const chord::EvaluationMetrics metrics = chord::evaluateChordSpans(expected, predicted);

    ASSERT_EQ(metrics.onsetDeltas.size(), 2U);
    EXPECT_NEAR(metrics.onsetDeltas[0].deltaSeconds, 0.064, 1.0e-9);
    EXPECT_NEAR(metrics.onsetDeltas[1].deltaSeconds, 0.128, 1.0e-9);
}

TEST(EvaluationTest, MergeAdjacentChordSpansCanDropUnknown) {
    const std::vector<chord::ChordSpan> spans{
        {0.0, 0.5, "Unknown"},
        {0.5, 1.0, "C major"},
        {1.0, 1.5, "C major"},
    };

    const std::vector<chord::ChordSpan> merged = chord::mergeAdjacentChordSpans(spans, false);

    ASSERT_EQ(merged.size(), 1U);
    EXPECT_DOUBLE_EQ(merged[0].startSeconds, 0.5);
    EXPECT_DOUBLE_EQ(merged[0].endSeconds, 1.5);
    EXPECT_EQ(merged[0].chord, "C major");
}

TEST(EvaluationTest, ThresholdHelperUsesConfiguredAccuracy) {
    chord::EvaluationMetrics metrics;
    metrics.overlapAccuracy = 0.80;

    EXPECT_TRUE(chord::meetsMinimumAccuracy(metrics, 0.75));
    EXPECT_FALSE(chord::meetsMinimumAccuracy(metrics, 0.85));
    EXPECT_THROW(chord::meetsMinimumAccuracy(metrics, 1.1), std::invalid_argument);
}
