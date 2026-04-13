#include "fft_processor.h"

#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace chord {

namespace {

constexpr float kPi = 3.14159265358979323846f;

}  // namespace

FFTProcessor::FFTProcessor(int fftSize, int sampleRate)
    : fftSize_(fftSize), sampleRate_(sampleRate) {
    if (fftSize_ <= 0) {
        throw std::invalid_argument("fftSize must be positive");
    }

    if (sampleRate_ <= 0) {
        throw std::invalid_argument("sampleRate must be positive");
    }

    binCount_ = fftSize_ / 2 + 1;
    window_.resize(static_cast<std::size_t>(fftSize_));
    magnitudes_.resize(static_cast<std::size_t>(binCount_));

    fftInput_ = fftwf_alloc_real(static_cast<std::size_t>(fftSize_));
    fftOutput_ = fftwf_alloc_complex(static_cast<std::size_t>(binCount_));

    if (fftInput_ == nullptr || fftOutput_ == nullptr) {
        fftwf_free(fftInput_);
        fftwf_free(fftOutput_);
        throw std::runtime_error("Failed to allocate FFTW buffers");
    }

    fftPlan_ = fftwf_plan_dft_r2c_1d(fftSize_, fftInput_, fftOutput_, FFTW_ESTIMATE);
    if (fftPlan_ == nullptr) {
        fftwf_free(fftInput_);
        fftwf_free(fftOutput_);
        throw std::runtime_error("Failed to create FFTW plan");
    }

    // A Hann window tapers the block edges toward zero. This reduces spectral
    // leakage when a note does not fit perfectly inside one analysis block.
    if (fftSize_ == 1) {
        window_[0] = 1.0f;
    } else {
        for (int index = 0; index < fftSize_; ++index) {
            const float phase = (2.0f * kPi * static_cast<float>(index)) /
                                static_cast<float>(fftSize_ - 1);
            window_[static_cast<std::size_t>(index)] = 0.5f * (1.0f - std::cos(phase));
        }
    }
}

FFTProcessor::~FFTProcessor() {
    if (fftPlan_ != nullptr) {
        fftwf_destroy_plan(fftPlan_);
    }

    fftwf_free(fftOutput_);
    fftwf_free(fftInput_);
}

const std::vector<float>& FFTProcessor::computeMagnitudeSpectrum(const float* samples, int numSamples) {
    if (samples == nullptr) {
        throw std::invalid_argument("samples must not be null");
    }

    if (numSamples <= 0) {
        throw std::invalid_argument("numSamples must be positive");
    }

    // Use at most one fixed-size FFT block. Short final blocks are padded with
    // silence; larger buffers are truncated so processing cost stays bounded.
    const int samplesToCopy = std::min(numSamples, fftSize_);
    for (int index = 0; index < samplesToCopy; ++index) {
        fftInput_[index] = samples[index] * window_[static_cast<std::size_t>(index)];
    }

    std::fill(fftInput_ + samplesToCopy, fftInput_ + fftSize_, 0.0f);

    fftwf_execute(fftPlan_);

    // FFTW stores each frequency bin as a complex number. Its magnitude is the
    // bin's energy-like strength, independent of phase.
    for (int bin = 0; bin < binCount_; ++bin) {
        const float real = fftOutput_[bin][0];
        const float imaginary = fftOutput_[bin][1];
        magnitudes_[static_cast<std::size_t>(bin)] = std::sqrt(real * real + imaginary * imaginary);
    }

    return magnitudes_;
}

float FFTProcessor::findDominantFrequencyHz() const {
    FrequencyPeak peak;
    const int peakCount =
        findTopFrequencyPeaks(&peak, 1, kDefaultMinFrequencyHz, kDefaultMaxFrequencyHz);
    if (peakCount == 0) {
        return 0.0f;
    }

    return peak.frequencyHz;
}

int FFTProcessor::findTopFrequencyPeaks(FrequencyPeak* peaks,
                                        int maxPeaks,
                                        float minFrequencyHz,
                                        float maxFrequencyHz) const {
    if (maxPeaks <= 0) {
        return 0;
    }

    if (peaks == nullptr) {
        throw std::invalid_argument("peaks must not be null when maxPeaks is positive");
    }

    if (minFrequencyHz < 0.0f || maxFrequencyHz <= minFrequencyHz) {
        throw std::invalid_argument("frequency range must be non-negative and increasing");
    }

    if (magnitudes_.size() <= 2U) {
        return 0;
    }

    const float nyquistHz = static_cast<float>(sampleRate_) * 0.5f;
    const float clampedMaxFrequencyHz = std::min(maxFrequencyHz, nyquistHz);
    if (clampedMaxFrequencyHz <= minFrequencyHz) {
        return 0;
    }

    const float binWidthHz = static_cast<float>(sampleRate_) / static_cast<float>(fftSize_);
    const int minBin = std::max(1, static_cast<int>(std::ceil(minFrequencyHz / binWidthHz)));
    const int maxBin = std::min(binCount_ - 2,
                                static_cast<int>(std::floor(clampedMaxFrequencyHz / binWidthHz)));

    int peakCount = 0;
    for (int bin = minBin; bin <= maxBin; ++bin) {
        const std::size_t binIndex = static_cast<std::size_t>(bin);
        const float magnitude = magnitudes_[binIndex];
        if (magnitude <= 0.0f) {
            continue;
        }

        // A local maximum is louder than its immediate neighbors. This avoids
        // reporting several adjacent bins from one broad spectral hotspot.
        if (magnitude <= magnitudes_[binIndex - 1U] || magnitude < magnitudes_[binIndex + 1U]) {
            continue;
        }

        const FrequencyPeak candidate{
            bin,
            static_cast<float>(bin) * binWidthHz,
            magnitude,
        };

        int insertIndex = peakCount;
        while (insertIndex > 0 && peaks[insertIndex - 1].magnitude < candidate.magnitude) {
            if (insertIndex < maxPeaks) {
                peaks[insertIndex] = peaks[insertIndex - 1];
            }
            --insertIndex;
        }

        if (insertIndex < maxPeaks) {
            peaks[insertIndex] = candidate;
            if (peakCount < maxPeaks) {
                ++peakCount;
            }
        }
    }

    return peakCount;
}

int FFTProcessor::fftSize() const {
    return fftSize_;
}

int FFTProcessor::sampleRate() const {
    return sampleRate_;
}

int FFTProcessor::binCount() const {
    return binCount_;
}

}  // namespace chord
