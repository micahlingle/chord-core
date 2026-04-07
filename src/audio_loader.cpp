#include "audio_loader.h"

#include <sndfile.h>

#include <stdexcept>
#include <vector>

namespace chord {

AudioFileData AudioLoader::loadWavFile(const std::string& path) const {
    SF_INFO fileInfo{};
    SNDFILE* file = sf_open(path.c_str(), SFM_READ, &fileInfo);
    if (file == nullptr) {
        throw std::runtime_error("Failed to open audio file: " + path);
    }

    const sf_count_t frameCount = fileInfo.frames;
    const int channelCount = fileInfo.channels;

    std::vector<float> interleavedSamples(static_cast<std::size_t>(frameCount * channelCount));
    const sf_count_t samplesRead = sf_readf_float(file, interleavedSamples.data(), frameCount);
    sf_close(file);

    if (samplesRead != frameCount) {
        throw std::runtime_error("Failed to read all audio frames from: " + path);
    }

    AudioFileData result;
    result.sampleRate = fileInfo.samplerate;
    result.channelCount = channelCount;
    result.samples.resize(static_cast<std::size_t>(frameCount));

    // Down-mix to mono so the later DSP stages operate on one deterministic stream.
    for (sf_count_t frame = 0; frame < frameCount; ++frame) {
        float mixedSample = 0.0f;
        for (int channel = 0; channel < channelCount; ++channel) {
            const std::size_t index = static_cast<std::size_t>(frame * channelCount + channel);
            mixedSample += interleavedSamples[index];
        }

        result.samples[static_cast<std::size_t>(frame)] = mixedSample / static_cast<float>(channelCount);
    }

    return result;
}

}  // namespace chord
