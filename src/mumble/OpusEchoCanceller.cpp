// Copyright The Mumble Developers. All rights reserved.
// Use of this source code is governed by a BSD-style license
// that can be found in the LICENSE file at the root of the
// Mumble source tree or at <https://www.mumble.info/LICENSE>.

#include "OpusEchoCanceller.h"

#include <algorithm>
#include <cstring>

#ifdef USE_WEBRTC_AEC
// WebRTC headers would be included here
// #include "modules/audio_processing/include/audio_processing.h"
// #include "common_audio/include/audio_util.h"
#endif

std::unique_ptr<OpusEchoCanceller> OpusEchoCanceller::create(
    const Config &config,
    Error *error
) {
    auto echoCanceller = std::unique_ptr<OpusEchoCanceller>(
        new OpusEchoCanceller(config)
    );
    
    Error initError = echoCanceller->initialize();
    if (initError != Error::None) {
        if (error) *error = initError;
        return nullptr;
    }
    
    if (error) *error = Error::None;
    return echoCanceller;
}

OpusEchoCanceller::OpusEchoCanceller(const Config &config)
    : m_config(config)
    , m_audioProcessing(nullptr)
    , m_nearFrame(nullptr)
    , m_farFrame(nullptr)
{
}

OpusEchoCanceller::~OpusEchoCanceller() {
    // Cleanup is handled by smart pointers
}

OpusEchoCanceller::OpusEchoCanceller(OpusEchoCanceller &&other) noexcept
    : m_config(other.m_config)
    , m_audioProcessing(std::move(other.m_audioProcessing))
    , m_nearFrame(std::move(other.m_nearFrame))
    , m_farFrame(std::move(other.m_farFrame))
    , m_tempNearBuffer(std::move(other.m_tempNearBuffer))
    , m_tempFarBuffer(std::move(other.m_tempFarBuffer))
    , m_tempOutputBuffer(std::move(other.m_tempOutputBuffer))
{
}

OpusEchoCanceller &OpusEchoCanceller::operator=(OpusEchoCanceller &&other) noexcept {
    if (this != &other) {
        m_config = other.m_config;
        m_audioProcessing = std::move(other.m_audioProcessing);
        m_nearFrame = std::move(other.m_nearFrame);
        m_farFrame = std::move(other.m_farFrame);
        m_tempNearBuffer = std::move(other.m_tempNearBuffer);
        m_tempFarBuffer = std::move(other.m_tempFarBuffer);
        m_tempOutputBuffer = std::move(other.m_tempOutputBuffer);
    }
    return *this;
}

OpusEchoCanceller::Error OpusEchoCanceller::initialize() {
    // Validate configuration
    if (m_config.sampleRate == 0 || m_config.channels == 0 || m_config.framesPerBuffer == 0) {
        return Error::InvalidParameters;
    }
    
    // Check if sample rate is supported
    if (!isSampleRateSupported(m_config.sampleRate)) {
        return Error::InvalidSampleRate;
    }
    
    if (m_config.channels > 2) {
        return Error::InvalidChannelCount;
    }
    
#ifdef USE_WEBRTC_AEC
    // TODO: Initialize WebRTC AudioProcessing
    // This is a stub implementation - full WebRTC integration requires
    // linking against WebRTC libraries and proper configuration
    
    // For now, we'll create a placeholder that just copies input to output
    std::size_t bufferSize = m_config.framesPerBuffer * m_config.channels;
    m_tempNearBuffer.resize(bufferSize);
    m_tempFarBuffer.resize(bufferSize);
    m_tempOutputBuffer.resize(bufferSize);
    
    return Error::None;
#else
    // WebRTC AEC not available, return error
    return Error::FeatureNotSupported;
#endif
}

OpusEchoCanceller::Error OpusEchoCanceller::process(
    const std::int16_t *nearEnd,
    const std::int16_t *farEnd,
    std::int16_t *output,
    std::size_t frameCount
) {
    if (!isValid()) {
        return Error::ProcessingError;
    }
    
    if (frameCount != m_config.framesPerBuffer) {
        return Error::InvalidFrameSize;
    }
    
#ifdef USE_WEBRTC_AEC
    // TODO: Implement actual WebRTC processing
    // For now, just copy nearEnd to output as a stub
    std::size_t sampleCount = frameCount * m_config.channels;
    std::copy(nearEnd, nearEnd + sampleCount, output);
    
    return Error::None;
#else
    return Error::FeatureNotSupported;
#endif
}

OpusEchoCanceller::Error OpusEchoCanceller::processFloat(
    const float *nearEnd,
    const std::int16_t *farEnd,
    float *output,
    std::size_t frameCount
) {
    if (!isValid()) {
        return Error::ProcessingError;
    }
    
    if (frameCount != m_config.framesPerBuffer) {
        return Error::InvalidFrameSize;
    }
    
#ifdef USE_WEBRTC_AEC
    // TODO: Implement actual WebRTC processing with float samples
    // For now, just copy nearEnd to output as a stub
    std::size_t sampleCount = frameCount * m_config.channels;
    std::copy(nearEnd, nearEnd + sampleCount, output);
    
    return Error::None;
#else
    return Error::FeatureNotSupported;
#endif
}

OpusEchoCanceller::Error OpusEchoCanceller::processReverseStream(
    const std::int16_t *farEnd,
    std::size_t frameCount
) {
    if (!isValid()) {
        return Error::ProcessingError;
    }
    
#ifdef USE_WEBRTC_AEC
    // TODO: Implement WebRTC reverse stream processing
    // This would feed the far-end (speaker) signal to the AEC
    return Error::None;
#else
    return Error::FeatureNotSupported;
#endif
}

OpusEchoCanceller::Error OpusEchoCanceller::processReverseStreamFloat(
    const float *farEnd,
    std::size_t frameCount
) {
    if (!isValid()) {
        return Error::ProcessingError;
    }
    
#ifdef USE_WEBRTC_AEC
    // TODO: Implement WebRTC reverse stream processing with float samples
    return Error::None;
#else
    return Error::FeatureNotSupported;
#endif
}

void OpusEchoCanceller::reset() {
#ifdef USE_WEBRTC_AEC
    // TODO: Reset WebRTC AudioProcessing state
#endif
}

OpusEchoCanceller::Error OpusEchoCanceller::updateConfig(const Config &config) {
    m_config = config;
    return initialize(); // Re-initialize with new config
}

OpusEchoCanceller::Error OpusEchoCanceller::getStatistics(Statistics &stats) const {
    if (!isValid()) {
        return Error::ProcessingError;
    }
    
    // TODO: Get actual statistics from WebRTC
    // For now, return placeholder values
    stats.echoReturnLoss = 0.0f;
    stats.echoReturnLossEnhancement = 0.0f;
    stats.divergentFilterFraction = 0.0f;
    stats.delayMs = 0;
    stats.voiceActivityProbability = 0.0f;
    stats.noiseLevelDb = -60.0f;
    
    return Error::None;
}

OpusEchoCanceller::Error OpusEchoCanceller::setEchoSuppressionEnabled(bool enabled) {
    // TODO: Configure WebRTC echo suppression
    return Error::None;
}

OpusEchoCanceller::Error OpusEchoCanceller::setEchoSuppressionLevel(AecMode mode) {
    // TODO: Set WebRTC AEC aggressiveness
    return Error::None;
}

OpusEchoCanceller::Error OpusEchoCanceller::setNoiseSuppressionEnabled(bool enabled) {
    // TODO: Configure WebRTC noise suppression
    return Error::None;
}

OpusEchoCanceller::Error OpusEchoCanceller::setNoiseSuppressionLevel(NoiseSuppressionLevel level) {
    // TODO: Set WebRTC noise suppression level
    return Error::None;
}

OpusEchoCanceller::Error OpusEchoCanceller::setAgcEnabled(bool enabled) {
    // TODO: Configure WebRTC AGC
    return Error::None;
}

OpusEchoCanceller::Error OpusEchoCanceller::setAgcMode(AgcMode mode) {
    // TODO: Set WebRTC AGC mode
    return Error::None;
}

OpusEchoCanceller::Error OpusEchoCanceller::setAgcTargetLevel(float targetLevelDb) {
    // TODO: Set WebRTC AGC target level
    return Error::None;
}

const char *OpusEchoCanceller::getErrorDescription(Error error) {
    switch (error) {
        case Error::None:
            return "No error";
        case Error::InvalidParameters:
            return "Invalid parameters provided";
        case Error::InitializationFailed:
            return "Initialization failed";
        case Error::ProcessingError:
            return "Processing error";
        case Error::InvalidSampleRate:
            return "Invalid sample rate";
        case Error::InvalidChannelCount:
            return "Invalid channel count";
        case Error::InvalidFrameSize:
            return "Invalid frame size";
        default:
            return "Unknown error";
    }
}

std::uint32_t OpusEchoCanceller::getRecommendedBufferSize(std::uint32_t sampleRate) {
    // Return 10ms buffer size (typical for real-time processing)
    return sampleRate / 100;
}

bool OpusEchoCanceller::isSampleRateSupported(std::uint32_t sampleRate) {
    // WebRTC supports these sample rates
    switch (sampleRate) {
        case 8000:
        case 16000:
        case 32000:
        case 48000:
            return true;
        default:
            return false;
    }
}

void OpusEchoCanceller::convertInt16ToFloat(
    const std::int16_t *input,
    float *output,
    std::size_t sampleCount
) {
    for (std::size_t i = 0; i < sampleCount; ++i) {
        output[i] = static_cast<float>(input[i]) / 32768.0f;
    }
}

void OpusEchoCanceller::convertFloatToInt16(
    const float *input,
    std::int16_t *output,
    std::size_t sampleCount
) {
    for (std::size_t i = 0; i < sampleCount; ++i) {
        float sample = std::clamp(input[i], -1.0f, 1.0f);
        output[i] = static_cast<std::int16_t>(sample * 32767.0f);
    }
}

OpusEchoCanceller::Error OpusEchoCanceller::applyConfig() {
#ifdef USE_WEBRTC_AEC
    // TODO: Apply configuration to WebRTC AudioProcessing
    return Error::None;
#else
    return Error::FeatureNotSupported;
#endif
}