// Copyright The Mumble Developers. All rights reserved.
// Use of this source code is governed by a BSD-style license
// that can be found in the LICENSE file at the root of the
// Mumble source tree or at <https://www.mumble.info/LICENSE>.

#ifndef MUMBLE_MUMBLE_OPUSAUDIOPREPROCESSOR_H_
#define MUMBLE_MUMBLE_OPUSAUDIOPREPROCESSOR_H_

#include <cstdint>
#include <memory>
#include <vector>

// Forward declarations
class OpusEchoCanceller;

#ifdef USE_RNNOISE
struct DenoiseState;
#endif

namespace webrtc {
class AudioProcessing;
class GainControl;
class NoiseSuppression;
class VoiceDetection;
}

/**
 * Unified audio preprocessor combining multiple audio enhancement technologies.
 * 
 * This class replaces the SpeexDSP preprocessor with a more modern and flexible
 * audio processing pipeline that integrates:
 * - WebRTC Audio Processing Module (APM) for VAD, AGC, and basic noise suppression
 * - RNNoise for advanced ML-based noise suppression
 * - Optional integration with echo cancellation
 * 
 * The preprocessor is designed to work seamlessly with Opus encoding while
 * providing superior audio quality compared to the legacy SpeexDSP implementation.
 */
class OpusAudioPreprocessor {
public:
    enum class Error {
        None = 0,
        InvalidParameters,
        InitializationFailed,
        ProcessingError,
        InvalidSampleRate,
        InvalidFrameSize,
        FeatureNotSupported
    };

    enum class NoiseReductionMode {
        Disabled,           ///< No noise reduction
        WebRTCBasic,       ///< Basic WebRTC noise suppression
        WebRTCModerate,    ///< Moderate WebRTC noise suppression
        WebRTCHigh,        ///< High WebRTC noise suppression
        RNNoise,           ///< RNNoise ML-based noise reduction
        Hybrid             ///< Combination of WebRTC and RNNoise
    };

    enum class AgcMode {
        Disabled,          ///< No automatic gain control
        AdaptiveAnalog,    ///< Adaptive analog AGC
        AdaptiveDigital,   ///< Adaptive digital AGC (recommended)
        FixedDigital      ///< Fixed digital AGC
    };

    enum class VadMode {
        Disabled,          ///< No voice activity detection
        Conservative,      ///< Conservative VAD - fewer false negatives
        Normal,           ///< Normal VAD - balanced
        Aggressive,       ///< Aggressive VAD - fewer false positives
        VeryAggressive    ///< Very aggressive VAD - maximum speech detection
    };

    /**
     * Configuration parameters for the audio preprocessor.
     */
    struct Config {
        std::uint32_t sampleRate = 48000;          ///< Sample rate in Hz (must match Opus)
        std::uint32_t framesPerBuffer = 480;       ///< Frames per processing buffer (10ms at 48kHz)
        std::uint32_t channels = 1;                ///< Number of channels (1 for mono)
        
        // Noise reduction configuration
        NoiseReductionMode noiseReduction = NoiseReductionMode::RNNoise;
        float noiseSuppressionLevel = 0.8f;        ///< Noise suppression strength [0.0, 1.0]
        
        // Automatic Gain Control
        AgcMode agcMode = AgcMode::AdaptiveDigital;
        float agcTargetLevel = -18.0f;             ///< AGC target level in dBFS
        std::uint32_t agcCompressionGain = 9;      ///< Compression gain in dB
        std::uint32_t agcMaxGain = 30;             ///< Maximum gain in dB
        bool agcLimiterEnabled = true;             ///< Enable AGC limiter
        
        // Voice Activity Detection
        VadMode vadMode = VadMode::Normal;
        float vadThreshold = 0.5f;                 ///< VAD threshold [0.0, 1.0]
        
        // High-pass filter
        bool enableHighPassFilter = true;         ///< Remove low-frequency noise
        float highPassCutoff = 85.0f;             ///< High-pass cutoff frequency in Hz
        
        // Advanced features
        bool enableLevelEstimator = true;         ///< Enable level estimation
        bool enableDerivativeFilter = false;      ///< Enable derivative filter (dereverb)
        bool enableSpectralSubtraction = false;   ///< Enable spectral subtraction
        
        // Echo cancellation integration
        bool integrateWithEchoCanceller = true;   ///< Integrate with echo canceller
    };

    /**
     * Audio processing statistics and metrics.
     */
    struct Statistics {
        float inputLevel = 0.0f;                  ///< Input RMS level in dBFS
        float outputLevel = 0.0f;                 ///< Output RMS level in dBFS
        float gainApplied = 0.0f;                 ///< AGC gain applied in dB
        float noiseLevel = 0.0f;                  ///< Estimated noise level in dBFS
        float voiceActivityProbability = 0.0f;    ///< VAD probability [0.0, 1.0]
        float speechProbability = 0.0f;           ///< Speech probability [0.0, 1.0]
        std::uint64_t framesProcessed = 0;        ///< Total frames processed
        std::uint64_t speechFrames = 0;           ///< Frames detected as speech
    };

    /**
     * Power Spectral Density data for visualization and analysis.
     */
    using PsdData = std::vector<float>;

    /**
     * Create a new audio preprocessor instance.
     * 
     * @param config Configuration parameters
     * @param error Optional pointer to receive error code
     * @return Unique pointer to OpusAudioPreprocessor instance, or nullptr on failure
     */
    static std::unique_ptr<OpusAudioPreprocessor> create(
        const Config &config = Config{},
        Error *error = nullptr
    );

    /**
     * Destructor - cleans up all processing resources
     */
    ~OpusAudioPreprocessor();

    // Disable copy construction and assignment
    OpusAudioPreprocessor(const OpusAudioPreprocessor &) = delete;
    OpusAudioPreprocessor &operator=(const OpusAudioPreprocessor &) = delete;

    // Enable move construction and assignment
    OpusAudioPreprocessor(OpusAudioPreprocessor &&other) noexcept;
    OpusAudioPreprocessor &operator=(OpusAudioPreprocessor &&other) noexcept;

    /**
     * Process an audio frame through the preprocessing pipeline.
     * 
     * @param samples Input/output audio samples (in-place processing)
     * @param frameCount Number of frames to process
     * @return Error code indicating success or failure
     * 
     * @note The samples buffer must contain exactly frameCount * channels samples
     * @note Samples are processed in-place (input is overwritten with processed output)
     */
    Error processFrame(std::int16_t *samples, std::size_t frameCount);

    /**
     * Process an audio frame through the preprocessing pipeline (floating-point version).
     * 
     * @param samples Input/output audio samples (in-place processing)
     * @param frameCount Number of frames to process
     * @return Error code indicating success or failure
     * 
     * @note The samples buffer must contain exactly frameCount * channels samples
     * @note Samples are expected to be in range [-1.0, 1.0]
     */
    Error processFrameFloat(float *samples, std::size_t frameCount);

    /**
     * Run only voice activity detection on the given samples.
     * 
     * @param samples Input audio samples
     * @param frameCount Number of frames
     * @return Voice activity probability [0.0, 1.0], or -1.0 on error
     */
    float detectVoiceActivity(const std::int16_t *samples, std::size_t frameCount);

    /**
     * Run only voice activity detection (floating-point version).
     * 
     * @param samples Input audio samples
     * @param frameCount Number of frames
     * @return Voice activity probability [0.0, 1.0], or -1.0 on error
     */
    float detectVoiceActivityFloat(const float *samples, std::size_t frameCount);

    /**
     * Reset the preprocessor state.
     * Should be called when there are discontinuities in the audio stream.
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
     * Get power spectral density of the input signal.
     * 
     * @return Vector containing PSD data, empty on error
     */
    PsdData getInputPSD() const;

    /**
     * Get power spectral density of the noise estimate.
     * 
     * @return Vector containing noise PSD data, empty on error
     */
    PsdData getNoisePSD() const;

    /**
     * Set the echo canceller for integration.
     * The preprocessor will coordinate with the echo canceller for optimal processing.
     * 
     * @param echoCanceller Pointer to echo canceller instance (can be nullptr)
     * @return Error code indicating success or failure
     */
    Error setEchoCanceller(std::shared_ptr<OpusEchoCanceller> echoCanceller);

    /**
     * Enable or disable noise reduction.
     * 
     * @param enabled True to enable, false to disable
     * @return Error code indicating success or failure
     */
    Error setNoiseReductionEnabled(bool enabled);

    /**
     * Set the noise reduction mode.
     * 
     * @param mode Noise reduction mode
     * @return Error code indicating success or failure
     */
    Error setNoiseReductionMode(NoiseReductionMode mode);

    /**
     * Enable or disable automatic gain control.
     * 
     * @param enabled True to enable, false to disable
     * @return Error code indicating success or failure
     */
    Error setAgcEnabled(bool enabled);

    /**
     * Set the AGC target level.
     * 
     * @param targetLevelDb Target level in dBFS (typically -12 to -30)
     * @return Error code indicating success or failure
     */
    Error setAgcTargetLevel(float targetLevelDb);

    /**
     * Enable or disable voice activity detection.
     * 
     * @param enabled True to enable, false to disable
     * @return Error code indicating success or failure
     */
    Error setVadEnabled(bool enabled);

    /**
     * Set the VAD sensitivity.
     * 
     * @param mode VAD mode/sensitivity
     * @return Error code indicating success or failure
     */
    Error setVadMode(VadMode mode);

    /**
     * Check if the preprocessor is in a valid state.
     * @return true if the preprocessor is ready to process audio
     */
    bool isValid() const;

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
     * Check if RNNoise support is available.
     * @return true if RNNoise was compiled in and is available
     */
    static bool isRNNoiseAvailable();

private:
    /**
     * Private constructor - use create() static method instead.
     */
    explicit OpusAudioPreprocessor(const Config &config);

    /**
     * Initialize all processing components.
     * @return Error code indicating success or failure
     */
    Error initialize();

    /**
     * Initialize WebRTC Audio Processing Module.
     * @return Error code indicating success or failure
     */
    Error initializeWebRTC();

#ifdef USE_RNNOISE
    /**
     * Initialize RNNoise denoising.
     * @return Error code indicating success or failure
     */
    Error initializeRNNoise();

    /**
     * Process audio frame with RNNoise.
     * @param samples Input/output samples (float format)
     * @param frameCount Number of frames
     * @return Error code indicating success or failure
     */
    Error processRNNoise(float *samples, std::size_t frameCount);
#endif

    /**
     * Apply WebRTC audio processing.
     * @param samples Input/output samples (float format)
     * @param frameCount Number of frames
     * @return Error code indicating success or failure
     */
    Error processWebRTC(float *samples, std::size_t frameCount);

    /**
     * Apply high-pass filtering.
     * @param samples Input/output samples
     * @param frameCount Number of frames
     */
    void applyHighPassFilter(float *samples, std::size_t frameCount);

    /**
     * Update processing statistics.
     * @param inputSamples Original input samples
     * @param outputSamples Processed output samples
     * @param frameCount Number of frames
     */
    void updateStatistics(const float *inputSamples, const float *outputSamples, std::size_t frameCount);

    /**
     * Calculate RMS level of audio samples.
     * @param samples Audio samples
     * @param frameCount Number of frames
     * @return RMS level in dBFS
     */
    float calculateRMSLevel(const float *samples, std::size_t frameCount) const;

    /**
     * Convert samples from int16 to float format.
     */
    void convertInt16ToFloat(const std::int16_t *input, float *output, std::size_t sampleCount);

    /**
     * Convert samples from float to int16 format.
     */
    void convertFloatToInt16(const float *input, std::int16_t *output, std::size_t sampleCount);

    Config m_config;
    bool m_initialized;

    // WebRTC Audio Processing
    std::unique_ptr<webrtc::AudioProcessing> m_audioProcessing;
    std::unique_ptr<webrtc::GainControl> m_gainControl;
    std::unique_ptr<webrtc::NoiseSuppression> m_noiseSuppression;
    std::unique_ptr<webrtc::VoiceDetection> m_voiceDetection;

#ifdef USE_RNNOISE
    // RNNoise state
    DenoiseState *m_rnnoise;
#endif

    // Echo canceller integration
    std::weak_ptr<OpusEchoCanceller> m_echoCanceller;

    // Processing buffers
    std::vector<float> m_tempBuffer;
    std::vector<float> m_processBuffer;

    // High-pass filter state (simple single-pole IIR)
    float m_hpFilterState;

    // Statistics
    mutable Statistics m_statistics;
    std::uint64_t m_frameCounter;

    // PSD data for analysis
    mutable std::vector<float> m_inputPSD;
    mutable std::vector<float> m_noisePSD;
};

#endif // MUMBLE_MUMBLE_OPUSAUDIOPREPROCESSOR_H_