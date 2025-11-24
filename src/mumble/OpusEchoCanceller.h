// Copyright The Mumble Developers. All rights reserved.
// Use of this source code is governed by a BSD-style license
// that can be found in the LICENSE file at the root of the
// Mumble source tree or at <https://www.mumble.info/LICENSE>.

#ifndef MUMBLE_MUMBLE_OPUSECHOCANCELLER_H_
#define MUMBLE_MUMBLE_OPUSECHOCANCELLER_H_

#include <cstdint>
#include <memory>

// Forward declarations to avoid including heavy WebRTC headers
namespace webrtc {
class AudioProcessing;
class AudioFrame;
}

/**
 * High-quality echo cancellation using WebRTC Audio Processing Module.
 * 
 * This class provides a replacement for SpeexDSP's echo cancellation
 * functionality using WebRTC's more advanced Acoustic Echo Cancellation (AEC).
 * 
 * Features:
 * - Advanced echo cancellation algorithms (AEC3)
 * - Automatic gain control (AGC)
 * - Noise suppression
 * - Voice activity detection (VAD)
 * - Low latency operation
 * - Adaptive filter lengths
 */
class OpusEchoCanceller {
public:
    enum class Error {
        None = 0,
        InvalidParameters,
        InitializationFailed,
        ProcessingError,
        InvalidSampleRate,
        InvalidChannelCount,
        InvalidFrameSize
    };

    enum class AecMode {
        Disabled,           ///< No echo cancellation
        Conservative,       ///< Conservative suppression - less aggressive
        Moderate,          ///< Moderate suppression - balanced approach
        Aggressive         ///< Aggressive suppression - maximum echo removal
    };

    enum class NoiseSuppressionLevel {
        Disabled,          ///< No noise suppression
        Low,              ///< Low noise suppression
        Moderate,         ///< Moderate noise suppression
        High,             ///< High noise suppression
        VeryHigh          ///< Very high noise suppression
    };

    enum class AgcMode {
        Disabled,          ///< No automatic gain control
        AdaptiveAnalog,    ///< Adaptive analog AGC
        AdaptiveDigital,   ///< Adaptive digital AGC
        FixedDigital      ///< Fixed digital AGC
    };

    /**
     * Configuration parameters for the echo canceller.
     */
    struct Config {
        std::uint32_t sampleRate = 48000;           ///< Sample rate in Hz (8000, 16000, 32000, 48000)
        std::uint32_t channels = 1;                 ///< Number of channels (1 or 2)
        std::uint32_t framesPerBuffer = 480;        ///< Frames per processing buffer (10ms at 48kHz)
        
        AecMode aecMode = AecMode::Moderate;        ///< Echo cancellation mode
        NoiseSuppressionLevel nsLevel = NoiseSuppressionLevel::Moderate; ///< Noise suppression level
        AgcMode agcMode = AgcMode::AdaptiveDigital; ///< AGC mode
        
        bool enableHighPassFilter = true;          ///< Enable high-pass filter
        bool enableLevelEstimator = true;          ///< Enable level estimator
        bool enableVoiceDetection = true;          ///< Enable voice activity detection
        
        float agcTargetLevel = 3.0f;               ///< AGC target level in dBFs
        std::uint32_t agcCompressionGain = 9;      ///< AGC compression gain in dB
        bool agcLimiterEnabled = true;             ///< Enable AGC limiter
        
        std::uint32_t aecDelayMs = 0;              ///< AEC delay in milliseconds (0 = auto)
    };

    /**
     * Audio processing statistics.
     */
    struct Statistics {
        float echoReturnLoss = 0.0f;               ///< Echo return loss in dB
        float echoReturnLossEnhancement = 0.0f;    ///< Echo return loss enhancement in dB
        float divergentFilterFraction = 0.0f;      ///< Fraction of divergent filter
        std::int32_t delayMs = 0;                  ///< Estimated delay in milliseconds
        float voiceActivityProbability = 0.0f;     ///< Voice activity probability [0.0, 1.0]
        float noiseLevelDb = 0.0f;                 ///< Estimated noise level in dB
    };

    /**
     * Create a new echo canceller instance.
     * 
     * @param config Configuration parameters
     * @param error Optional pointer to receive error code
     * @return Unique pointer to OpusEchoCanceller instance, or nullptr on failure
     */
    static std::unique_ptr<OpusEchoCanceller> create(
        const Config &config = Config{},
        Error *error = nullptr
    );

    /**
     * Destructor - cleans up WebRTC resources
     */
    ~OpusEchoCanceller();

    // Disable copy construction and assignment
    OpusEchoCanceller(const OpusEchoCanceller &) = delete;
    OpusEchoCanceller &operator=(const OpusEchoCanceller &) = delete;

    // Enable move construction and assignment
    OpusEchoCanceller(OpusEchoCanceller &&other) noexcept;
    OpusEchoCanceller &operator=(OpusEchoCanceller &&other) noexcept;

    /**
     * Process audio frames with echo cancellation.
     * 
     * @param nearEnd Near-end (microphone) audio samples
     * @param farEnd Far-end (speaker/reference) audio samples
     * @param output Processed near-end audio samples
     * @param frameCount Number of frames to process
     * @return Error code indicating success or failure
     * 
     * @note nearEnd, farEnd, and output buffers must be at least frameCount * channels in size
     * @note All buffers use 16-bit signed integer samples
     */
    Error process(
        const std::int16_t *nearEnd,
        const std::int16_t *farEnd,
        std::int16_t *output,
        std::size_t frameCount
    );

    /**
     * Process audio frames with echo cancellation (floating-point version).
     * 
     * @param nearEnd Near-end (microphone) audio samples
     * @param farEnd Far-end (speaker/reference) audio samples
     * @param output Processed near-end audio samples
     * @param frameCount Number of frames to process
     * @return Error code indicating success or failure
     * 
     * @note nearEnd, farEnd, and output buffers must be at least frameCount * channels in size
     * @note All buffers use floating-point samples in range [-1.0, 1.0]
     */
    Error processFloat(
        const float *nearEnd,
        const float *farEnd,
        float *output,
        std::size_t frameCount
    );

    /**
     * Process only the render (far-end/speaker) path.
     * Call this when you have speaker audio but no microphone audio to process.
     * 
     * @param farEnd Far-end (speaker/reference) audio samples
     * @param frameCount Number of frames to process
     * @return Error code indicating success or failure
     */
    Error processReverseStream(const std::int16_t *farEnd, std::size_t frameCount);

    /**
     * Process only the render (far-end/speaker) path (floating-point version).
     * 
     * @param farEnd Far-end (speaker/reference) audio samples
     * @param frameCount Number of frames to process
     * @return Error code indicating success or failure
     */
    Error processReverseStreamFloat(const float *farEnd, std::size_t frameCount);

    /**
     * Reset the echo canceller state.
     * Should be called when there are discontinuities in the audio streams.
     */
    void reset();

    /**
     * Update configuration parameters at runtime.
     * 
     * @param config New configuration parameters
     * @return Error code indicating success or failure
     */
    Error updateConfig(const Config &config);

    /**
     * Get current processing statistics.
     * 
     * @param stats Structure to fill with statistics
     * @return Error code indicating success or failure
     */
    Error getStatistics(Statistics &stats) const;

    /**
     * Enable or disable echo cancellation.
     * 
     * @param enabled True to enable, false to disable
     * @return Error code indicating success or failure
     */
    Error setEchoSuppressionEnabled(bool enabled);

    /**
     * Set the echo cancellation suppression level.
     * 
     * @param mode AEC mode/aggressiveness
     * @return Error code indicating success or failure
     */
    Error setEchoSuppressionLevel(AecMode mode);

    /**
     * Enable or disable noise suppression.
     * 
     * @param enabled True to enable, false to disable
     * @return Error code indicating success or failure
     */
    Error setNoiseSuppressionEnabled(bool enabled);

    /**
     * Set the noise suppression level.
     * 
     * @param level Noise suppression level
     * @return Error code indicating success or failure
     */
    Error setNoiseSuppressionLevel(NoiseSuppressionLevel level);

    /**
     * Enable or disable automatic gain control.
     * 
     * @param enabled True to enable, false to disable
     * @return Error code indicating success or failure
     */
    Error setAgcEnabled(bool enabled);

    /**
     * Set the automatic gain control mode.
     * 
     * @param mode AGC mode
     * @return Error code indicating success or failure
     */
    Error setAgcMode(AgcMode mode);

    /**
     * Set the AGC target level.
     * 
     * @param targetLevelDb Target level in dBFs (typically 0 to -31)
     * @return Error code indicating success or failure
     */
    Error setAgcTargetLevel(float targetLevelDb);

    /**
     * Check if the echo canceller is in a valid state.
     * @return true if the echo canceller is ready to process audio
     */
    bool isValid() const { return m_audioProcessing != nullptr; }

    /**
     * Get the current configuration.
     * @return Current configuration parameters
     */
    const Config &getConfig() const { return m_config; }

    /**
     * Get a human-readable description of an error code.
     * @param error Error code to describe
     * @return String description of the error
     */
    static const char *getErrorDescription(Error error);

    /**
     * Get recommended buffer size for the given sample rate.
     * @param sampleRate Sample rate in Hz
     * @return Recommended buffer size in frames (typically 10ms worth)
     */
    static std::uint32_t getRecommendedBufferSize(std::uint32_t sampleRate);

    /**
     * Check if a sample rate is supported.
     * @param sampleRate Sample rate to check
     * @return true if the sample rate is supported
     */
    static bool isSampleRateSupported(std::uint32_t sampleRate);

private:
    /**
     * Private constructor - use create() static method instead.
     */
    explicit OpusEchoCanceller(const Config &config);

    /**
     * Initialize the WebRTC AudioProcessing instance.
     * @return Error code indicating success or failure
     */
    Error initialize();

    /**
     * Convert samples from int16 to float format.
     */
    void convertInt16ToFloat(const std::int16_t *input, float *output, std::size_t sampleCount);

    /**
     * Convert samples from float to int16 format.
     */
    void convertFloatToInt16(const float *input, std::int16_t *output, std::size_t sampleCount);

    /**
     * Update WebRTC AudioProcessing configuration.
     */
    Error applyConfig();

    Config m_config;
    std::unique_ptr<webrtc::AudioProcessing> m_audioProcessing;
    std::unique_ptr<webrtc::AudioFrame> m_nearFrame;
    std::unique_ptr<webrtc::AudioFrame> m_farFrame;
    
    // Temporary buffers for format conversion
    std::vector<float> m_tempNearBuffer;
    std::vector<float> m_tempFarBuffer;
    std::vector<float> m_tempOutputBuffer;
};

#endif // MUMBLE_MUMBLE_OPUSECHOCANCELLER_H_