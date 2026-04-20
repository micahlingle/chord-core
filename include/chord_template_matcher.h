#pragma once

#include "chord_result.h"
#include "chroma_extractor.h"

namespace chord {

// Compares a normalized chroma vector against simple major/minor triad
// templates. This is intentionally small and deterministic: no learned model,
// no heap-backed template tables, and no history.
class ChordTemplateMatcher {
  public:
    ChordResult match(const ChromaVector& chroma) const;
};

} // namespace chord
