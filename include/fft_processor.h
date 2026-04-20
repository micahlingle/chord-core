#pragma once

#include <fftw3.h>

#include <vector>

namespace chord {

struct FrequencyPeak {
    int binIndex = 0;
    float frequencyHz = 0.0f;
    float magnitude = 0.0f;
};

// Converts a fixed-size block of time-domain audio samples into frequency-bin
// magnitudes. This is the first DSP stage needed before chroma extraction.
class FFTProcessor {
  public:
    static constexpr float kDefaultMinFrequencyHz = 75.0f;
    static constexpr float kDefaultMaxFrequencyHz = 5000.0f;

    // fftSize is the number of time-domain samples analyzed per transform.
    // sampleRate is needed to convert FFT bin indices back into Hertz.
    FFTProcessor(int fftSize, int sampleRate);
    ~FFTProcessor();

    // FFTW plans own internal setup data and are tied to specific input/output
    // buffers, so this class is intentionally non-copyable and non-movable.
    FFTProcessor(const FFTProcessor&) = delete;
    FFTProcessor& operator=(const FFTProcessor&) = delete;
    FFTProcessor(FFTProcessor&&) = delete;
    FFTProcessor& operator=(FFTProcessor&&) = delete;

    // Returns one magnitude per FFT bin. Bin 0 is DC offset; higher bins map to
    // frequencies at binIndex * sampleRate / fftSize.
    const std::vector<float>& computeMagnitudeSpectrum(const float* samples, int numSamples);

    // Finds local maxima in the most recent spectrum within a frequency band.
    // The caller owns the output buffer so this can run without allocation.
    int findTopFrequencyPeaks(FrequencyPeak* peaks, int maxPeaks, float minFrequencyHz, float maxFrequencyHz) const;

    // Returns the loudest guitar-range peak from the most recent spectrum.
    float findDominantFrequencyHz() const;

    int fftSize() const;
    int sampleRate() const;

    // Real-input FFTs only produce fftSize / 2 + 1 unique bins because the
    // negative-frequency half is redundant for real-valued audio.
    int binCount() const;

  private:
    int fftSize_ = 0;
    int sampleRate_ = 0;
    int binCount_ = 0;
    float* fftInput_ = nullptr;
    fftwf_complex* fftOutput_ = nullptr;

    // FFTW "plan" means a precomputed execution recipe for a particular
    // transform size and buffer layout. Building it can allocate and inspect CPU
    // capabilities, so it is created up front instead of during audio processing.
    fftwf_plan fftPlan_ = nullptr;
    std::vector<float> window_;
    std::vector<float> magnitudes_;
};

} // namespace chord
