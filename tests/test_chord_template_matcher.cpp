#include "chord_template_matcher.h"

#include <cmath>
#include <limits>
#include <stdexcept>

#include <gtest/gtest.h>

namespace {

void setPitch(chord::ChromaVector& chroma, chord::PitchClass pitchClass, float value) {
    chroma[static_cast<std::size_t>(pitchClass)] = value;
}

}  // namespace

TEST(ChordTemplateMatcherTest, ReturnsUnknownForSilence) {
    const chord::ChordTemplateMatcher matcher;
    const chord::ChordResult result = matcher.match(chord::ChromaVector{});

    EXPECT_EQ(result.name, "Unknown");
    EXPECT_FLOAT_EQ(result.confidence, 0.0f);
}

TEST(ChordTemplateMatcherTest, MatchesExactMajorTriad) {
    chord::ChromaVector chroma{};
    setPitch(chroma, chord::kPitchClassC, 1.0f);
    setPitch(chroma, chord::kPitchClassE, 1.0f);
    setPitch(chroma, chord::kPitchClassG, 1.0f);

    const chord::ChordTemplateMatcher matcher;
    const chord::ChordResult result = matcher.match(chroma);

    EXPECT_EQ(result.name, "C major");
    EXPECT_NEAR(result.confidence, 1.0f, 0.0001f);
}

TEST(ChordTemplateMatcherTest, MatchesExactMinorTriad) {
    chord::ChromaVector chroma{};
    setPitch(chroma, chord::kPitchClassA, 1.0f);
    setPitch(chroma, chord::kPitchClassC, 1.0f);
    setPitch(chroma, chord::kPitchClassE, 1.0f);

    const chord::ChordTemplateMatcher matcher;
    const chord::ChordResult result = matcher.match(chroma);

    EXPECT_EQ(result.name, "A minor");
    EXPECT_NEAR(result.confidence, 1.0f, 0.0001f);
}

TEST(ChordTemplateMatcherTest, ToleratesExtraNonChordEnergy) {
    chord::ChromaVector chroma{};
    setPitch(chroma, chord::kPitchClassG, 1.0f);
    setPitch(chroma, chord::kPitchClassB, 0.9f);
    setPitch(chroma, chord::kPitchClassD, 0.8f);
    setPitch(chroma, chord::kPitchClassFSharp, 0.2f);

    const chord::ChordTemplateMatcher matcher;
    const chord::ChordResult result = matcher.match(chroma);

    EXPECT_EQ(result.name, "G major");
    EXPECT_GT(result.confidence, 0.9f);
    EXPECT_LT(result.confidence, 1.0f);
}

TEST(ChordTemplateMatcherTest, RejectsInvalidChromaValues) {
    const chord::ChordTemplateMatcher matcher;

    chord::ChromaVector negativeChroma{};
    negativeChroma[0] = -0.1f;
    EXPECT_THROW(matcher.match(negativeChroma), std::invalid_argument);

    chord::ChromaVector infiniteChroma{};
    infiniteChroma[0] = std::numeric_limits<float>::infinity();
    EXPECT_THROW(matcher.match(infiniteChroma), std::invalid_argument);

    chord::ChromaVector nanChroma{};
    nanChroma[0] = std::nanf("");
    EXPECT_THROW(matcher.match(nanChroma), std::invalid_argument);
}
