// Copyright The Mumble Developers. All rights reserved.
// Use of this source code is governed by a BSD-style license
// that can be found in the LICENSE file at the root of the
// Mumble source tree or at <https://www.mumble.info/LICENSE>.

#ifndef MUMBLE_MUMBLE_AUDIOPROCESSINGADAPTER_H_
#define MUMBLE_MUMBLE_AUDIOPROCESSINGADAPTER_H_

#include <cstdint>
#include <memory>

#ifdef USE_OPUS_AUDIO_PROCESSING
#include "OpusResampler.h"
#include "OpusAudioPreprocessor.h"
#ifdef USE_WEBRTC_AEC
#include "OpusEchoCanceller.h"
#endif
#else
#include <speex/speex_resampler.h>
#include <speex/speex_echo.h>
#include "AudioPreprocessor.h"
#endif

/**
 * Adapter class that provides a unified interface for audio processing
 * regardless of whether SpeexDSP or Opus-based processing is used.
 * 
 * This adapter allows the existing Mumble codebase to work with either
 * the legacy SpeexDSP implementation or the new Opus-based audio processing
 * pipeline without requiring extensive code changes.
 */
class AudioProcessingAdapter {
public:
    enum class ProcessingBackend {
        SpeexDSP,
        Opus
    };

    enum class ResamplerQuality {
        Quick = 0,
        Low = 1,
        Medium = 3,
        High = 5,
        VeryHigh = 10
    };

    /**
     * Get the active processing backend.
     */
    static ProcessingBackend getBackend() {
#ifdef USE_OPUS_AUDIO_PROCESSING
        return ProcessingBackend::Opus;
#else
        return ProcessingBackend::SpeexDSP;
#endif
    }

    /**
     * Resampler wrapper that works with both backends.
     */
    class Resampler {
    public:
        static std::unique_ptr<Resampler> create(
            std::uint32_t inputRate,
            std::uint32_t outputRate,
            std::uint32_t channels,
            ResamplerQuality quality = ResamplerQuality::Medium
        );

        ~Resampler();

        // Disable copy, enable move
        Resampler(const Resampler&) = delete;
        Resampler& operator=(const Resampler&) = delete;
        Resampler(Resampler&&) noexcept;
        Resampler& operator=(Resampler&&) noexcept;

        /**
         * Process floating-point samples (single channel).
         */
        bool processFloat(
            const float* input,
            std::uint32_t* inputFrames,
            float* output,
            std::uint32_t* outputFrames
        );

        /**
         * Process floating-point samples (interleaved multi-channel).
         */
        bool processInterleavedFloat(
            const float* input,
            std::uint32_t* inputFrames,
            float* output,
            std::uint32_t* outputFrames
        );

        /**
         * Reset the resampler state.
         */
        void reset();

        /**
         * Check if the resampler is valid.
         */
        bool isValid() const;

    private:
        Resampler(
            std::uint32_t inputRate,
            std::uint32_t outputRate,
            std::uint32_t channels,
            ResamplerQuality quality
        );

        bool initialize();

        std::uint32_t m_inputRate;
        std::uint32_t m_outputRate;
        std::uint32_t m_channels;
        ResamplerQuality m_quality;

#ifdef USE_OPUS_AUDIO_PROCESSING
        std::unique_ptr<OpusResampler> m_opusResampler;
#else
        SpeexResamplerState* m_speexResampler;
#endif
    };

    /**
     * Echo canceller wrapper that works with both backends.
     */
    class EchoCanceller {
    public:
        static std::unique_ptr<EchoCanceller> create(
            std::uint32_t frameSize,
            std::uint32_t filterLength,
            std::uint32_t sampleRate,
            std::uint32_t channels = 1
        );

        ~EchoCanceller();

        // Disable copy, enable move
        EchoCanceller(const EchoCanceller&) = delete;
        EchoCanceller& operator=(const EchoCanceller&) = delete;
        EchoCanceller(EchoCanceller&&) noexcept;
        EchoCanceller& operator=(EchoCanceller&&) noexcept;

        /**
         * Process echo cancellation.
         */
        bool processEchoCancellation(
            const std::int16_t* nearEnd,
            const std::int16_t* farEnd,
            std::int16_t* output,
            std::uint32_t frameCount
        );

        /**
         * Process only the far-end (speaker) stream.
         */
        bool processReverseStream(
            const std::int16_t* farEnd,
            std::uint32_t frameCount
        );

        /**
         * Reset the echo canceller state.
         */
        void reset();

        /**
         * Check if the echo canceller is valid.
         */
        bool isValid() const;

    private:
        EchoCanceller(
            std::uint32_t frameSize,
            std::uint32_t filterLength,
            std::uint32_t sampleRate,
            std::uint32_t channels
        );

        bool initialize();

        std::uint32_t m_frameSize;
        std::uint32_t m_filterLength;
        std::uint32_t m_sampleRate;
        std::uint32_t m_channels;

#ifdef USE_OPUS_AUDIO_PROCESSING
#ifdef USE_WEBRTC_AEC
        std::unique_ptr<OpusEchoCanceller> m_opusEchoCanceller;
#endif
#else
        SpeexEchoState* m_speexEchoState;
#endif
    };

    /**
     * Audio preprocessor wrapper that works with both backends.
     */
    class Preprocessor {
    public:
        static std::unique_ptr<Preprocessor> create(
            std::uint32_t frameSize,
            std::uint32_t sampleRate
        );

        ~Preprocessor();

        // Disable copy, enable move
        Preprocessor(const Preprocessor&) = delete;
        Preprocessor& operator=(const Preprocessor&) = delete;
        Preprocessor(Preprocessor&&) noexcept;
        Preprocessor& operator=(Preprocessor&&) noexcept;

        /**
         * Process audio frame through preprocessing pipeline.
         */
        bool processFrame(std::int16_t* samples, std::uint32_t frameCount);

        /**
         * Run voice activity detection.
         */
        float detectVoiceActivity(const std::int16_t* samples, std::uint32_t frameCount);

        /**
         * Reset the preprocessor state.
         */
        void reset();

        /**
         * Set echo canceller for integration.
         */
        bool setEchoCanceller(std::shared_ptr<EchoCanceller> echoCanceller);

        /**
         * Enable/disable noise suppression.
         */
        bool setNoiseSuppressionEnabled(bool enabled);

        /**
         * Enable/disable automatic gain control.
         */
        bool setAgcEnabled(bool enabled);

        /**
         * Enable/disable voice activity detection.
         */
        bool setVadEnabled(bool enabled);

        /**
         * Set AGC target level.
         */
        bool setAgcTargetLevel(float targetDb);

        /**
         * Set noise suppression level.
         */
        bool setNoiseSuppressionLevel(float level);

        /**
         * Get speech probability from last processed frame.
         */
        float getSpeechProbability() const;

        /**
         * Check if the preprocessor is valid.
         */
        bool isValid() const;

    private:
        Preprocessor(std::uint32_t frameSize, std::uint32_t sampleRate);

        bool initialize();

        std::uint32_t m_frameSize;
        std::uint32_t m_sampleRate;

#ifdef USE_OPUS_AUDIO_PROCESSING
        std::unique_ptr<OpusAudioPreprocessor> m_opusPreprocessor;
        std::shared_ptr<EchoCanceller> m_echoCanceller;
#else
        AudioPreprocessor m_speexPreprocessor;
#endif
        
        mutable float m_lastSpeechProb;
    };

    /**
     * Get recommended buffer size for the current backend.
     */
    static std::uint32_t getRecommendedBufferSize(std::uint32_t sampleRate) {
#ifdef USE_OPUS_AUDIO_PROCESSING
        return sampleRate / 100; // 10ms for Opus (480 samples at 48kHz)
#else
        return sampleRate / 100; // 10ms for Speex as well
#endif
    }

    /**
     * Get the name of the active backend.
     */
    static const char* getBackendName() {
#ifdef USE_OPUS_AUDIO_PROCESSING
        return "Opus Audio Processing";
#else
        return "SpeexDSP";
#endif
    }

    /**
     * Check if a feature is supported by the current backend.
     */
    static bool isFeatureSupported(const char* feature) {
        if (strcmp(feature, "resampling") == 0) {
            return true; // Both backends support resampling
        }
        if (strcmp(feature, "echo_cancellation") == 0) {
#ifdef USE_OPUS_AUDIO_PROCESSING
#ifdef USE_WEBRTC_AEC
            return true;
#else
            return false;
#endif
#else
            return true; // SpeexDSP has echo cancellation
#endif
        }
        if (strcmp(feature, "noise_suppression") == 0) {
            return true; // Both backends support noise suppression
        }
        if (strcmp(feature, "agc") == 0) {
            return true; // Both backends support AGC
        }
        if (strcmp(feature, "vad") == 0) {
            return true; // Both backends support VAD
        }
        if (strcmp(feature, "rnnoise") == 0) {
#ifdef USE_RNNOISE
            return true;
#else
            return false;
#endif
        }
        return false;
    }
};

#endif // MUMBLE_MUMBLE_AUDIOPROCESSINGADAPTER_H_