#pragma once

#include <string>
#include <vector>

namespace chord {

struct AudioFileData {
    std::vector<float> samples;
    int sampleRate = 0;
    int channelCount = 0;
};

class AudioLoader {
public:
    AudioFileData loadWavFile(const std::string& path) const;
};

}  // namespace chord
