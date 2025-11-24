// Copyright The Mumble Developers. All rights reserved.
// Use of this source code is governed by a BSD-style license
// that can be found in the LICENSE file at the root of the
// Mumble source tree or at <https://www.mumble.info/LICENSE>.

#ifndef MUMBLE_MUMBLE_OPUSRESAMPLER_H_
#define MUMBLE_MUMBLE_OPUSRESAMPLER_H_

#include <cstdint>
#include <memory>

// Forward declarations to avoid including heavy headers
struct soxr;
typedef struct soxr *soxr_t;

/**
 * High-quality audio resampler using libsoxr as replacement for SpeexDSP resampler.
 * 
 * This class provides a clean C++ interface around libsoxr, offering superior
 * quality and performance compared to the legacy Speex resampler.
 * 
 * Features:
 * - High-quality resampling with minimal artifacts
 * - Configurable quality levels
 * - Support for both integer and floating-point samples
 * - Multi-channel support
 * - Low latency operation
 */
class OpusResampler {
public:
    enum class Quality {
        Quick,      ///< Fastest, lowest quality - suitable for real-time processing
        Low,        ///< Low quality - good balance of speed and quality
        Medium,     ///< Medium quality - default for most use cases
        High,       ///< High quality - better quality, higher CPU usage
        VeryHigh    ///< Very high quality - best quality, highest CPU usage
    };

    enum class Error {
        None = 0,
        InvalidParameters,
        OutOfMemory,
        InternalError,
        InvalidQuality,
        ProcessingError
    };

    /**
     * Create a new resampler instance.
     * 
     * @param inputSampleRate Input sample rate in Hz
     * @param outputSampleRate Output sample rate in Hz
     * @param channels Number of audio channels (1 for mono, 2 for stereo)
     * @param quality Resampling quality level
     * @param error Optional pointer to receive error code
     * @return Unique pointer to OpusResampler instance, or nullptr on failure
     */
    static std::unique_ptr<OpusResampler> create(
        std::uint32_t inputSampleRate,
        std::uint32_t outputSampleRate,
        std::uint32_t channels,
        Quality quality = Quality::Medium,
        Error *error = nullptr
    );

    /**
     * Destructor - cleans up libsoxr resources
     */
    ~OpusResampler();

    // Disable copy construction and assignment
    OpusResampler(const OpusResampler &) = delete;
    OpusResampler &operator=(const OpusResampler &) = delete;

    // Enable move construction and assignment
    OpusResampler(OpusResampler &&other) noexcept;
    OpusResampler &operator=(OpusResampler &&other) noexcept;

    /**
     * Process floating-point samples (single channel).
     * 
     * @param input Input sample buffer
     * @param inputFrames Number of input frames available
     * @param output Output sample buffer
     * @param outputFrames Maximum number of output frames that can be written
     * @param inputUsed Will be set to actual number of input frames consumed
     * @param outputGenerated Will be set to actual number of output frames generated
     * @return Error code indicating success or failure
     */
    Error processFloat(
        const float *input,
        std::size_t inputFrames,
        float *output,
        std::size_t outputFrames,
        std::size_t *inputUsed,
        std::size_t *outputGenerated
    );

    /**
     * Process floating-point samples (interleaved multi-channel).
     * 
     * @param input Input sample buffer (interleaved channels)
     * @param inputFrames Number of input frames available
     * @param output Output sample buffer (interleaved channels)
     * @param outputFrames Maximum number of output frames that can be written
     * @param inputUsed Will be set to actual number of input frames consumed
     * @param outputGenerated Will be set to actual number of output frames generated
     * @return Error code indicating success or failure
     */
    Error processInterleavedFloat(
        const float *input,
        std::size_t inputFrames,
        float *output,
        std::size_t outputFrames,
        std::size_t *inputUsed,
        std::size_t *outputGenerated
    );

    /**
     * Process 16-bit integer samples (single channel).
     * 
     * @param input Input sample buffer
     * @param inputFrames Number of input frames available
     * @param output Output sample buffer
     * @param outputFrames Maximum number of output frames that can be written
     * @param inputUsed Will be set to actual number of input frames consumed
     * @param outputGenerated Will be set to actual number of output frames generated
     * @return Error code indicating success or failure
     */
    Error processInt16(
        const std::int16_t *input,
        std::size_t inputFrames,
        std::int16_t *output,
        std::size_t outputFrames,
        std::size_t *inputUsed,
        std::size_t *outputGenerated
    );

    /**
     * Process 16-bit integer samples (interleaved multi-channel).
     * 
     * @param input Input sample buffer (interleaved channels)
     * @param inputFrames Number of input frames available
     * @param output Output sample buffer (interleaved channels)
     * @param outputFrames Maximum number of output frames that can be written
     * @param inputUsed Will be set to actual number of input frames consumed
     * @param outputGenerated Will be set to actual number of output frames generated
     * @return Error code indicating success or failure
     */
    Error processInterleavedInt16(
        const std::int16_t *input,
        std::size_t inputFrames,
        std::int16_t *output,
        std::size_t outputFrames,
        std::size_t *inputUsed,
        std::size_t *outputGenerated
    );

    /**
     * Reset the internal state of the resampler.
     * This should be called when there are discontinuities in the audio stream.
     */
    void reset();

    /**
     * Get the input sample rate.
     * @return Input sample rate in Hz
     */
    std::uint32_t getInputSampleRate() const { return m_inputSampleRate; }

    /**
     * Get the output sample rate.
     * @return Output sample rate in Hz
     */
    std::uint32_t getOutputSampleRate() const { return m_outputSampleRate; }

    /**
     * Get the number of channels.
     * @return Number of audio channels
     */
    std::uint32_t getChannels() const { return m_channels; }

    /**
     * Get the current quality setting.
     * @return Quality level
     */
    Quality getQuality() const { return m_quality; }

    /**
     * Calculate the expected number of output frames for a given number of input frames.
     * This is an approximation and the actual number may vary slightly.
     * 
     * @param inputFrames Number of input frames
     * @return Expected number of output frames
     */
    std::size_t getExpectedOutputFrames(std::size_t inputFrames) const;

    /**
     * Get the algorithmic delay introduced by the resampler in output frames.
     * @return Delay in output frames
     */
    std::size_t getDelay() const;

    /**
     * Check if the resampler is in a valid state.
     * @return true if the resampler is ready to process samples
     */
    bool isValid() const { return m_soxr != nullptr; }

    /**
     * Get a human-readable description of an error code.
     * @param error Error code to describe
     * @return String description of the error
     */
    static const char *getErrorDescription(Error error);

private:
    /**
     * Private constructor - use create() static method instead.
     */
    OpusResampler(
        std::uint32_t inputSampleRate,
        std::uint32_t outputSampleRate,
        std::uint32_t channels,
        Quality quality
    );

    /**
     * Initialize the libsoxr instance.
     * @return Error code indicating success or failure
     */
    Error initialize();

    /**
     * Convert Quality enum to libsoxr quality specification.
     * @param quality Quality level
     * @return libsoxr quality specification
     */
    static unsigned long qualityToSoxrSpec(Quality quality);

    std::uint32_t m_inputSampleRate;
    std::uint32_t m_outputSampleRate;
    std::uint32_t m_channels;
    Quality m_quality;
    soxr_t m_soxr;
};

#endif // MUMBLE_MUMBLE_OPUSRESAMPLER_H_