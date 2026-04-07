#include "audio_loader.h"

#include <sndfile.h>

#include <cstddef>
#include <stdexcept>
#include <vector>

namespace chord {

std::vector<float> downMixInterleavedToMono(const float* interleavedSamples,
                                           std::size_t frameCount,
                                           int channelCount) {
    if (channelCount <= 0) {
        throw std::invalid_argument("channelCount must be positive");
    }

    if (interleavedSamples == nullptr && frameCount > 0U) {
        throw std::invalid_argument("interleavedSamples must not be null when frameCount is non-zero");
    }

    std::vector<float> monoSamples(frameCount);

    // Average channels per frame. This keeps the loader deterministic and gives
    // later DSP stages one mono stream regardless of source channel layout.
    for (std::size_t frame = 0; frame < frameCount; ++frame) {
        float mixedSample = 0.0f;
        for (int channel = 0; channel < channelCount; ++channel) {
            const std::size_t index = frame * static_cast<std::size_t>(channelCount) +
                                      static_cast<std::size_t>(channel);
            mixedSample += interleavedSamples[index];
        }

        monoSamples[frame] = mixedSample / static_cast<float>(channelCount);
    }

    return monoSamples;
}

AudioFileData AudioLoader::loadWavFile(const std::string& path) const {
    SF_INFO fileInfo{};
    SNDFILE* file = sf_open(path.c_str(), SFM_READ, &fileInfo);
    if (file == nullptr) {
        throw std::runtime_error("Failed to open audio file: " + path);
    }

    const sf_count_t frameCount = fileInfo.frames;
    const int channelCount = fileInfo.channels;
    if (frameCount < 0) {
        sf_close(file);
        throw std::runtime_error("Audio file reported a negative frame count: " + path);
    }

    std::vector<float> interleavedSamples(static_cast<std::size_t>(frameCount * channelCount));
    const sf_count_t samplesRead = sf_readf_float(file, interleavedSamples.data(), frameCount);
    sf_close(file);

    if (samplesRead != frameCount) {
        throw std::runtime_error("Failed to read all audio frames from: " + path);
    }

    AudioFileData result;
    result.sampleRate = fileInfo.samplerate;
    result.channelCount = channelCount;
    result.samples = downMixInterleavedToMono(interleavedSamples.data(),
                                             static_cast<std::size_t>(frameCount),
                                             channelCount);

    return result;
}

}  // namespace chord
