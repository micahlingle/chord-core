#pragma once

#include <cstddef>
#include <string>
#include <vector>

namespace chord {

struct AudioFileData {
    std::vector<float> samples;
    int sampleRate = 0;
    int channelCount = 0;
};

std::vector<float> downMixInterleavedToMono(const float* interleavedSamples, std::size_t frameCount, int channelCount);

class AudioLoader {
  public:
    AudioFileData loadWavFile(const std::string& path) const;
};

} // namespace chord
