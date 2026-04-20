#pragma once

#include <string>

namespace chord {

struct ChordResult {
    std::string name = "Unknown";
    float confidence = 0.0f;
};

}  // namespace chord
