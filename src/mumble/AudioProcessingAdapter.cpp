// Copyright The Mumble Developers. All rights reserved.
// Use of this source code is governed by a BSD-style license
// that can be found in the LICENSE file at the root of the
// Mumble source tree or at <https://www.mumble.info/LICENSE>.

#include "AudioProcessingAdapter.h"

#include <algorithm>
#include <cstring>

// ================================
// Resampler Implementation
// ================================

std::unique_ptr<AudioProcessingAdapter::Resampler> AudioProcessingAdapter::Resampler::create(
    std::uint32_t inputRate,
    std::uint32_t outputRate,
    std::uint32_t channels,
    ResamplerQuality quality
) {
    auto resampler = std::unique_ptr<Resampler>(
        new Resampler(inputRate, outputRate, channels, quality)
    );
    
    if (!resampler->initialize()) {
        return nullptr;
    }
    
    return resampler;
}

AudioProcessingAdapter::Resampler::Resampler(
    std::uint32_t inputRate,
    std::uint32_t outputRate,
    std::uint32_t channels,
    ResamplerQuality quality
) : m_inputRate(inputRate)
  , m_outputRate(outputRate)
  , m_channels(channels)
  , m_quality(quality)
#ifdef USE_OPUS_AUDIO_PROCESSING
  , m_opusResampler(nullptr)
#else
  , m_speexResampler(nullptr)
#endif
{
}

AudioProcessingAdapter::Resampler::~Resampler() {
#ifdef USE_OPUS_AUDIO_PROCESSING
    // m_opusResampler is automatically cleaned up
#else
    if (m_speexResampler) {
        speex_resampler_destroy(m_speexResampler);
        m_speexResampler = nullptr;
    }
#endif
}

AudioProcessingAdapter::Resampler::Resampler(Resampler&& other) noexcept
    : m_inputRate(other.m_inputRate)
    , m_outputRate(other.m_outputRate)
    , m_channels(other.m_channels)
    , m_quality(other.m_quality)
#ifdef USE_OPUS_AUDIO_PROCESSING
    , m_opusResampler(std::move(other.m_opusResampler))
#else
    , m_speexResampler(other.m_speexResampler)
#endif
{
#ifndef USE_OPUS_AUDIO_PROCESSING
    other.m_speexResampler = nullptr;
#endif
}

AudioProcessingAdapter::Resampler& AudioProcessingAdapter::Resampler::operator=(Resampler&& other) noexcept {
    if (this != &other) {
#ifndef USE_OPUS_AUDIO_PROCESSING
        if (m_speexResampler) {
            speex_resampler_destroy(m_speexResampler);
        }
#endif
        
        m_inputRate = other.m_inputRate;
        m_outputRate = other.m_outputRate;
        m_channels = other.m_channels;
        m_quality = other.m_quality;

#ifdef USE_OPUS_AUDIO_PROCESSING
        m_opusResampler = std::move(other.m_opusResampler);
#else
        m_speexResampler = other.m_speexResampler;
        other.m_speexResampler = nullptr;
#endif
    }
    return *this;
}

bool AudioProcessingAdapter::Resampler::initialize() {
#ifdef USE_OPUS_AUDIO_PROCESSING
    OpusResampler::Quality opusQuality;
    switch (m_quality) {
        case ResamplerQuality::Quick:
            opusQuality = OpusResampler::Quality::Quick;
            break;
        case ResamplerQuality::Low:
            opusQuality = OpusResampler::Quality::Low;
            break;
        case ResamplerQuality::Medium:
            opusQuality = OpusResampler::Quality::Medium;
            break;
        case ResamplerQuality::High:
            opusQuality = OpusResampler::Quality::High;
            break;
        case ResamplerQuality::VeryHigh:
            opusQuality = OpusResampler::Quality::VeryHigh;
            break;
        default:
            opusQuality = OpusResampler::Quality::Medium;
            break;
    }
    
    OpusResampler::Error error;
    m_opusResampler = OpusResampler::create(
        m_inputRate, m_outputRate, m_channels, opusQuality, &error
    );
    
    return m_opusResampler != nullptr;
#else
    int err = 0;
    int qualityValue = static_cast<int>(m_quality);
    
    m_speexResampler = speex_resampler_init(
        m_channels, m_inputRate, m_outputRate, qualityValue, &err
    );
    
    return m_speexResampler != nullptr && err == RESAMPLER_ERR_SUCCESS;
#endif
}

bool AudioProcessingAdapter::Resampler::processFloat(
    const float* input,
    std::uint32_t* inputFrames,
    float* output,
    std::uint32_t* outputFrames
) {
    if (!isValid()) {
        return false;
    }

#ifdef USE_OPUS_AUDIO_PROCESSING
    std::size_t inputUsed = 0, outputGenerated = 0;
    OpusResampler::Error error;
    
    if (m_channels == 1) {
        error = m_opusResampler->processFloat(
            input, *inputFrames, output, *outputFrames,
            &inputUsed, &outputGenerated
        );
    } else {
        error = m_opusResampler->processInterleavedFloat(
            input, *inputFrames, output, *outputFrames,
            &inputUsed, &outputGenerated
        );
    }
    
    *inputFrames = static_cast<std::uint32_t>(inputUsed);
    *outputFrames = static_cast<std::uint32_t>(outputGenerated);
    
    return error == OpusResampler::Error::None;
#else
    spx_uint32_t inLen = *inputFrames;
    spx_uint32_t outLen = *outputFrames;
    
    int result;
    if (m_channels == 1) {
        result = speex_resampler_process_float(
            m_speexResampler, 0, input, &inLen, output, &outLen
        );
    } else {
        result = speex_resampler_process_interleaved_float(
            m_speexResampler, input, &inLen, output, &outLen
        );
    }
    
    *inputFrames = inLen;
    *outputFrames = outLen;
    
    return result == RESAMPLER_ERR_SUCCESS;
#endif
}

bool AudioProcessingAdapter::Resampler::processInterleavedFloat(
    const float* input,
    std::uint32_t* inputFrames,
    float* output,
    std::uint32_t* outputFrames
) {
    if (!isValid()) {
        return false;
    }

#ifdef USE_OPUS_AUDIO_PROCESSING
    std::size_t inputUsed = 0, outputGenerated = 0;
    OpusResampler::Error error = m_opusResampler->processInterleavedFloat(
        input, *inputFrames, output, *outputFrames,
        &inputUsed, &outputGenerated
    );
    
    *inputFrames = static_cast<std::uint32_t>(inputUsed);
    *outputFrames = static_cast<std::uint32_t>(outputGenerated);
    
    return error == OpusResampler::Error::None;
#else
    spx_uint32_t inLen = *inputFrames;
    spx_uint32_t outLen = *outputFrames;
    
    int result = speex_resampler_process_interleaved_float(
        m_speexResampler, input, &inLen, output, &outLen
    );
    
    *inputFrames = inLen;
    *outputFrames = outLen;
    
    return result == RESAMPLER_ERR_SUCCESS;
#endif
}

void AudioProcessingAdapter::Resampler::reset() {
    if (!isValid()) {
        return;
    }

#ifdef USE_OPUS_AUDIO_PROCESSING
    m_opusResampler->reset();
#else
    speex_resampler_reset_mem(m_speexResampler);
#endif
}

bool AudioProcessingAdapter::Resampler::isValid() const {
#ifdef USE_OPUS_AUDIO_PROCESSING
    return m_opusResampler && m_opusResampler->isValid();
#else
    return m_speexResampler != nullptr;
#endif
}

// ================================
// EchoCanceller Implementation
// ================================

std::unique_ptr<AudioProcessingAdapter::EchoCanceller> AudioProcessingAdapter::EchoCanceller::create(
    std::uint32_t frameSize,
    std::uint32_t filterLength,
    std::uint32_t sampleRate,
    std::uint32_t channels
) {
    auto echoCanceller = std::unique_ptr<EchoCanceller>(
        new EchoCanceller(frameSize, filterLength, sampleRate, channels)
    );
    
    if (!echoCanceller->initialize()) {
        return nullptr;
    }
    
    return echoCanceller;
}

AudioProcessingAdapter::EchoCanceller::EchoCanceller(
    std::uint32_t frameSize,
    std::uint32_t filterLength,
    std::uint32_t sampleRate,
    std::uint32_t channels
) : m_frameSize(frameSize)
  , m_filterLength(filterLength)
  , m_sampleRate(sampleRate)
  , m_channels(channels)
#ifdef USE_OPUS_AUDIO_PROCESSING
#ifdef USE_WEBRTC_AEC
  , m_opusEchoCanceller(nullptr)
#endif
#else
  , m_speexEchoState(nullptr)
#endif
{
}

AudioProcessingAdapter::EchoCanceller::~EchoCanceller() {
#ifdef USE_OPUS_AUDIO_PROCESSING
#ifdef USE_WEBRTC_AEC
    // m_opusEchoCanceller is automatically cleaned up
#endif
#else
    if (m_speexEchoState) {
        speex_echo_state_destroy(m_speexEchoState);
        m_speexEchoState = nullptr;
    }
#endif
}

AudioProcessingAdapter::EchoCanceller::EchoCanceller(EchoCanceller&& other) noexcept
    : m_frameSize(other.m_frameSize)
    , m_filterLength(other.m_filterLength)
    , m_sampleRate(other.m_sampleRate)
    , m_channels(other.m_channels)
#ifdef USE_OPUS_AUDIO_PROCESSING
#ifdef USE_WEBRTC_AEC
    , m_opusEchoCanceller(std::move(other.m_opusEchoCanceller))
#endif
#else
    , m_speexEchoState(other.m_speexEchoState)
#endif
{
#ifndef USE_OPUS_AUDIO_PROCESSING
    other.m_speexEchoState = nullptr;
#endif
}

AudioProcessingAdapter::EchoCanceller& AudioProcessingAdapter::EchoCanceller::operator=(EchoCanceller&& other) noexcept {
    if (this != &other) {
#ifndef USE_OPUS_AUDIO_PROCESSING
        if (m_speexEchoState) {
            speex_echo_state_destroy(m_speexEchoState);
        }
#endif
        
        m_frameSize = other.m_frameSize;
        m_filterLength = other.m_filterLength;
        m_sampleRate = other.m_sampleRate;
        m_channels = other.m_channels;

#ifdef USE_OPUS_AUDIO_PROCESSING
#ifdef USE_WEBRTC_AEC
        m_opusEchoCanceller = std::move(other.m_opusEchoCanceller);
#endif
#else
        m_speexEchoState = other.m_speexEchoState;
        other.m_speexEchoState = nullptr;
#endif
    }
    return *this;
}

bool AudioProcessingAdapter::EchoCanceller::initialize() {
#ifdef USE_OPUS_AUDIO_PROCESSING
#ifdef USE_WEBRTC_AEC
    OpusEchoCanceller::Config config;
    config.sampleRate = m_sampleRate;
    config.channels = m_channels;
    config.framesPerBuffer = m_frameSize;
    
    OpusEchoCanceller::Error error;
    m_opusEchoCanceller = OpusEchoCanceller::create(config, &error);
    
    return m_opusEchoCanceller != nullptr;
#else
    return false; // WebRTC AEC not available
#endif
#else
    m_speexEchoState = speex_echo_state_init_mc(
        m_frameSize, m_filterLength, 1, m_channels
    );
    
    if (m_speexEchoState) {
        int sampleRate = static_cast<int>(m_sampleRate);
        speex_echo_ctl(m_speexEchoState, SPEEX_ECHO_SET_SAMPLING_RATE, &sampleRate);
    }
    
    return m_speexEchoState != nullptr;
#endif
}

bool AudioProcessingAdapter::EchoCanceller::processEchoCancellation(
    const std::int16_t* nearEnd,
    const std::int16_t* farEnd,
    std::int16_t* output,
    std::uint32_t frameCount
) {
    if (!isValid()) {
        return false;
    }

#ifdef USE_OPUS_AUDIO_PROCESSING
#ifdef USE_WEBRTC_AEC
    OpusEchoCanceller::Error error = m_opusEchoCanceller->process(
        nearEnd, farEnd, output, frameCount
    );
    return error == OpusEchoCanceller::Error::None;
#else
    // Fallback: just copy input to output
    std::copy(nearEnd, nearEnd + frameCount * m_channels, output);
    return true;
#endif
#else
    speex_echo_cancellation(m_speexEchoState, nearEnd, farEnd, output);
    return true;
#endif
}

bool AudioProcessingAdapter::EchoCanceller::processReverseStream(
    const std::int16_t* farEnd,
    std::uint32_t frameCount
) {
    if (!isValid()) {
        return false;
    }

#ifdef USE_OPUS_AUDIO_PROCESSING
#ifdef USE_WEBRTC_AEC
    OpusEchoCanceller::Error error = m_opusEchoCanceller->processReverseStream(
        farEnd, frameCount
    );
    return error == OpusEchoCanceller::Error::None;
#else
    return true; // No-op if WebRTC AEC not available
#endif
#else
    // SpeexDSP doesn't have a separate reverse stream function
    // This would typically be called as part of the echo cancellation process
    return true;
#endif
}

void AudioProcessingAdapter::EchoCanceller::reset() {
    if (!isValid()) {
        return;
    }

#ifdef USE_OPUS_AUDIO_PROCESSING
#ifdef USE_WEBRTC_AEC
    m_opusEchoCanceller->reset();
#endif
#else
    speex_echo_state_reset(m_speexEchoState);
#endif
}

bool AudioProcessingAdapter::EchoCanceller::isValid() const {
#ifdef USE_OPUS_AUDIO_PROCESSING
#ifdef USE_WEBRTC_AEC
    return m_opusEchoCanceller && m_opusEchoCanceller->isValid();
#else
    return false;
#endif
#else
    return m_speexEchoState != nullptr;
#endif
}

// ================================
// Preprocessor Implementation
// ================================

std::unique_ptr<AudioProcessingAdapter::Preprocessor> AudioProcessingAdapter::Preprocessor::create(
    std::uint32_t frameSize,
    std::uint32_t sampleRate
) {
    auto preprocessor = std::unique_ptr<Preprocessor>(
        new Preprocessor(frameSize, sampleRate)
    );
    
    if (!preprocessor->initialize()) {
        return nullptr;
    }
    
    return preprocessor;
}

AudioProcessingAdapter::Preprocessor::Preprocessor(
    std::uint32_t frameSize,
    std::uint32_t sampleRate
) : m_frameSize(frameSize)
  , m_sampleRate(sampleRate)
#ifdef USE_OPUS_AUDIO_PROCESSING
  , m_opusPreprocessor(nullptr)
#else
  , m_speexPreprocessor()
#endif
  , m_lastSpeechProb(0.0f)
{
}

AudioProcessingAdapter::Preprocessor::~Preprocessor() {
    // Cleanup is handled by smart pointers and RAII
}

AudioProcessingAdapter::Preprocessor::Preprocessor(Preprocessor&& other) noexcept
    : m_frameSize(other.m_frameSize)
    , m_sampleRate(other.m_sampleRate)
#ifdef USE_OPUS_AUDIO_PROCESSING
    , m_opusPreprocessor(std::move(other.m_opusPreprocessor))
    , m_echoCanceller(std::move(other.m_echoCanceller))
#else
    , m_speexPreprocessor(std::move(other.m_speexPreprocessor))
#endif
    , m_lastSpeechProb(other.m_lastSpeechProb)
{
}

AudioProcessingAdapter::Preprocessor& AudioProcessingAdapter::Preprocessor::operator=(Preprocessor&& other) noexcept {
    if (this != &other) {
        m_frameSize = other.m_frameSize;
        m_sampleRate = other.m_sampleRate;
        m_lastSpeechProb = other.m_lastSpeechProb;

#ifdef USE_OPUS_AUDIO_PROCESSING
        m_opusPreprocessor = std::move(other.m_opusPreprocessor);
        m_echoCanceller = std::move(other.m_echoCanceller);
#else
        m_speexPreprocessor = std::move(other.m_speexPreprocessor);
#endif
    }
    return *this;
}

bool AudioProcessingAdapter::Preprocessor::initialize() {
#ifdef USE_OPUS_AUDIO_PROCESSING
    OpusAudioPreprocessor::Config config;
    config.sampleRate = m_sampleRate;
    config.framesPerBuffer = m_frameSize;
    
    OpusAudioPreprocessor::Error error;
    m_opusPreprocessor = OpusAudioPreprocessor::create(config, &error);
    
    return m_opusPreprocessor != nullptr;
#else
    return m_speexPreprocessor.init(m_sampleRate, m_frameSize);
#endif
}

bool AudioProcessingAdapter::Preprocessor::processFrame(std::int16_t* samples, std::uint32_t frameCount) {
    if (!isValid()) {
        return false;
    }

#ifdef USE_OPUS_AUDIO_PROCESSING
    OpusAudioPreprocessor::Error error = m_opusPreprocessor->processFrame(samples, frameCount);
    
    // Update speech probability
    m_lastSpeechProb = m_opusPreprocessor->detectVoiceActivity(samples, frameCount);
    
    return error == OpusAudioPreprocessor::Error::None;
#else
    bool result = m_speexPreprocessor.run(*samples);
    m_lastSpeechProb = static_cast<float>(m_speexPreprocessor.getSpeechProb()) / 100.0f;
    return result;
#endif
}

float AudioProcessingAdapter::Preprocessor::detectVoiceActivity(const std::int16_t* samples, std::uint32_t frameCount) {
    if (!isValid()) {
        return 0.0f;
    }

#ifdef USE_OPUS_AUDIO_PROCESSING
    return m_opusPreprocessor->detectVoiceActivity(samples, frameCount);
#else
    // For SpeexDSP, we need to run the preprocessor to get VAD results
    // This is a limitation of the Speex API
    return m_lastSpeechProb;
#endif
}

void AudioProcessingAdapter::Preprocessor::reset() {
    if (!isValid()) {
        return;
    }

#ifdef USE_OPUS_AUDIO_PROCESSING
    m_opusPreprocessor->reset();
#else
    // SpeexDSP preprocessor doesn't have a reset method
    // Reinitialize if needed
    m_speexPreprocessor.deinit();
    m_speexPreprocessor.init(m_sampleRate, m_frameSize);
#endif
}

bool AudioProcessingAdapter::Preprocessor::setEchoCanceller(std::shared_ptr<EchoCanceller> echoCanceller) {
#ifdef USE_OPUS_AUDIO_PROCESSING
    if (!m_opusPreprocessor) {
        return false;
    }
    
    m_echoCanceller = echoCanceller;
    // Note: Integration with OpusEchoCanceller would require additional implementation
    return true;
#else
    if (!echoCanceller || !echoCanceller->isValid()) {
        return false;
    }
    
    // For SpeexDSP, we would need to get the SpeexEchoState and set it
    // This requires access to the internal state
    return m_speexPreprocessor.setEchoState(echoCanceller->m_speexEchoState);
#endif
}

bool AudioProcessingAdapter::Preprocessor::setNoiseSuppressionEnabled(bool enabled) {
    if (!isValid()) {
        return false;
    }

#ifdef USE_OPUS_AUDIO_PROCESSING
    return m_opusPreprocessor->setNoiseReductionEnabled(enabled) == OpusAudioPreprocessor::Error::None;
#else
    return m_speexPreprocessor.setDenoise(enabled);
#endif
}

bool AudioProcessingAdapter::Preprocessor::setAgcEnabled(bool enabled) {
    if (!isValid()) {
        return false;
    }

#ifdef USE_OPUS_AUDIO_PROCESSING
    return m_opusPreprocessor->setAgcEnabled(enabled) == OpusAudioPreprocessor::Error::None;
#else
    return m_speexPreprocessor.setAGC(enabled);
#endif
}

bool AudioProcessingAdapter::Preprocessor::setVadEnabled(bool enabled) {
    if (!isValid()) {
        return false;
    }

#ifdef USE_OPUS_AUDIO_PROCESSING
    return m_opusPreprocessor->setVadEnabled(enabled) == OpusAudioPreprocessor::Error::None;
#else
    return m_speexPreprocessor.setVAD(enabled);
#endif
}

bool AudioProcessingAdapter::Preprocessor::setAgcTargetLevel(float targetDb) {
    if (!isValid()) {
        return false;
    }

#ifdef USE_OPUS_AUDIO_PROCESSING
    return m_opusPreprocessor->setAgcTargetLevel(targetDb) == OpusAudioPreprocessor::Error::None;
#else
    // Convert dB to Speex target level (0-32768)
    std::int32_t speexTarget = static_cast<std::int32_t>(
        32768.0f * std::pow(10.0f, targetDb / 20.0f)
    );
    speexTarget = std::clamp(speexTarget, 0, 32768);
    return m_speexPreprocessor.setAGCTarget(speexTarget);
#endif
}

bool AudioProcessingAdapter::Preprocessor::setNoiseSuppressionLevel(float level) {
    if (!isValid()) {
        return false;
    }

#ifdef USE_OPUS_AUDIO_PROCESSING
    // Map level to OpusAudioPreprocessor mode
    OpusAudioPreprocessor::NoiseReductionMode mode;
    if (level < 0.25f) {
        mode = OpusAudioPreprocessor::NoiseReductionMode::WebRTCBasic;
    } else if (level < 0.5f) {
        mode = OpusAudioPreprocessor::NoiseReductionMode::WebRTCModerate;
    } else if (level < 0.75f) {
        mode = OpusAudioPreprocessor::NoiseReductionMode::WebRTCHigh;
    } else {
        mode = OpusAudioPreprocessor::NoiseReductionMode::RNNoise;
    }
    return m_opusPreprocessor->setNoiseReductionMode(mode) == OpusAudioPreprocessor::Error::None;
#else
    // Convert level to Speex noise suppression value (-100 to 0 dB)
    std::int32_t speexLevel = static_cast<std::int32_t>(-100.0f + level * 100.0f);
    speexLevel = std::clamp(speexLevel, -100, 0);
    return m_speexPreprocessor.setNoiseSuppress(speexLevel);
#endif
}

float AudioProcessingAdapter::Preprocessor::getSpeechProbability() const {
    return m_lastSpeechProb;
}

bool AudioProcessingAdapter::Preprocessor::isValid() const {
#ifdef USE_OPUS_AUDIO_PROCESSING
    return m_opusPreprocessor && m_opusPreprocessor->isValid();
#else
    // SpeexDSP preprocessor doesn't have an isValid method
    // We assume it's valid if it was initialized successfully
    return true;
#endif
}