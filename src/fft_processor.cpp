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
    if (magnitudes_.size() <= 1U) {
        return 0.0f;
    }

    std::size_t dominantBin = 0U;
    float dominantMagnitude = 0.0f;

    // Ignore bin 0 because it represents DC offset, not a musical frequency.
    for (std::size_t bin = 1U; bin < magnitudes_.size(); ++bin) {
        if (magnitudes_[bin] > dominantMagnitude) {
            dominantMagnitude = magnitudes_[bin];
            dominantBin = bin;
        }
    }

    if (dominantBin == 0U || dominantMagnitude == 0.0f) {
        return 0.0f;
    }

    const float binWidthHz = static_cast<float>(sampleRate_) / static_cast<float>(fftSize_);
    // Each FFT bin is evenly spaced in frequency, so bin index * bin width gives
    // the center frequency represented by that bin.
    return static_cast<float>(dominantBin) * binWidthHz;
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
