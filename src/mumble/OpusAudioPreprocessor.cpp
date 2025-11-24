// Copyright The Mumble Developers. All rights reserved.
// Use of this source code is governed by a BSD-style license
// that can be found in the LICENSE file at the root of the
// Mumble source tree or at <https://www.mumble.info/LICENSE>.

#include "OpusAudioPreprocessor.h"

#include <algorithm>
#include <cmath>
#include <cstring>

#ifdef USE_RNNOISE
extern "C" {
#include "rnnoise.h"
}
#endif

#ifdef USE_WEBRTC_AEC
// WebRTC headers would be included here
// #include "modules/audio_processing/include/audio_processing.h"
// #include "modules/audio_processing/include/gain_control.h"
// #include "modules/audio_processing/include/noise_suppression.h"
// #include "modules/audio_processing/include/voice_detection.h"
#endif

std::unique_ptr<OpusAudioPreprocessor> OpusAudioPreprocessor::create(
    const Config &config,
    Error *error
) {
    auto preprocessor = std::unique_ptr<OpusAudioPreprocessor>(
        new OpusAudioPreprocessor(config)
    );
    
    Error initError = preprocessor->initialize();
    if (initError != Error::None) {
        if (error) *error = initError;
        return nullptr;
    }
    
    if (error) *error = Error::None;
    return preprocessor;
}

OpusAudioPreprocessor::OpusAudioPreprocessor(const Config &config)
    : m_config(config)
    , m_initialized(false)
    , m_audioProcessing(nullptr)
    , m_gainControl(nullptr)
    , m_noiseSuppression(nullptr)
    , m_voiceDetection(nullptr)
#ifdef USE_RNNOISE
    , m_rnnoise(nullptr)
#endif
    , m_hpFilterState(0.0f)
    , m_frameCounter(0)
{
    // Initialize statistics
    m_statistics = {};
}

OpusAudioPreprocessor::~OpusAudioPreprocessor() {
#ifdef USE_RNNOISE
    if (m_rnnoise) {
        rnnoise_destroy(m_rnnoise);
        m_rnnoise = nullptr;
    }
#endif
    // Other cleanup is handled by smart pointers
}

OpusAudioPreprocessor::OpusAudioPreprocessor(OpusAudioPreprocessor &&other) noexcept
    : m_config(other.m_config)
    , m_initialized(other.m_initialized)
    , m_audioProcessing(std::move(other.m_audioProcessing))
    , m_gainControl(std::move(other.m_gainControl))
    , m_noiseSuppression(std::move(other.m_noiseSuppression))
    , m_voiceDetection(std::move(other.m_voiceDetection))
#ifdef USE_RNNOISE
    , m_rnnoise(other.m_rnnoise)
#endif
    , m_echoCanceller(std::move(other.m_echoCanceller))
    , m_tempBuffer(std::move(other.m_tempBuffer))
    , m_processBuffer(std::move(other.m_processBuffer))
    , m_hpFilterState(other.m_hpFilterState)
    , m_statistics(other.m_statistics)
    , m_frameCounter(other.m_frameCounter)
    , m_inputPSD(std::move(other.m_inputPSD))
    , m_noisePSD(std::move(other.m_noisePSD))
{
#ifdef USE_RNNOISE
    other.m_rnnoise = nullptr;
#endif
    other.m_initialized = false;
}

OpusAudioPreprocessor &OpusAudioPreprocessor::operator=(OpusAudioPreprocessor &&other) noexcept {
    if (this != &other) {
#ifdef USE_RNNOISE
        if (m_rnnoise) {
            rnnoise_destroy(m_rnnoise);
        }
#endif
        
        m_config = other.m_config;
        m_initialized = other.m_initialized;
        m_audioProcessing = std::move(other.m_audioProcessing);
        m_gainControl = std::move(other.m_gainControl);
        m_noiseSuppression = std::move(other.m_noiseSuppression);
        m_voiceDetection = std::move(other.m_voiceDetection);
#ifdef USE_RNNOISE
        m_rnnoise = other.m_rnnoise;
        other.m_rnnoise = nullptr;
#endif
        m_echoCanceller = std::move(other.m_echoCanceller);
        m_tempBuffer = std::move(other.m_tempBuffer);
        m_processBuffer = std::move(other.m_processBuffer);
        m_hpFilterState = other.m_hpFilterState;
        m_statistics = other.m_statistics;
        m_frameCounter = other.m_frameCounter;
        m_inputPSD = std::move(other.m_inputPSD);
        m_noisePSD = std::move(other.m_noisePSD);
        
        other.m_initialized = false;
    }
    return *this;
}

OpusAudioPreprocessor::Error OpusAudioPreprocessor::initialize() {
    // Validate configuration
    if (m_config.sampleRate == 0 || m_config.framesPerBuffer == 0) {
        return Error::InvalidParameters;
    }
    
    // Check supported sample rates
    if (m_config.sampleRate != 8000 && m_config.sampleRate != 16000 && 
        m_config.sampleRate != 32000 && m_config.sampleRate != 48000) {
        return Error::InvalidSampleRate;
    }
    
    if (m_config.channels == 0 || m_config.channels > 2) {
        return Error::InvalidParameters;
    }
    
    // Initialize buffers
    std::size_t bufferSize = m_config.framesPerBuffer * m_config.channels;
    m_tempBuffer.resize(bufferSize);
    m_processBuffer.resize(bufferSize);
    
    // Initialize WebRTC components
    Error webRtcError = initializeWebRTC();
    if (webRtcError != Error::None) {
        return webRtcError;
    }
    
#ifdef USE_RNNOISE
    // Initialize RNNoise if enabled
    if (m_config.noiseReduction == NoiseReductionMode::RNNoise ||
        m_config.noiseReduction == NoiseReductionMode::Hybrid) {
        Error rnnoiseError = initializeRNNoise();
        if (rnnoiseError != Error::None) {
            return rnnoiseError;
        }
    }
#endif
    
    m_initialized = true;
    return Error::None;
}

OpusAudioPreprocessor::Error OpusAudioPreprocessor::initializeWebRTC() {
#ifdef USE_WEBRTC_AEC
    // TODO: Initialize WebRTC AudioProcessing
    // This is a stub implementation
    return Error::None;
#else
    // WebRTC not available, but that's okay for basic processing
    return Error::None;
#endif
}

#ifdef USE_RNNOISE
OpusAudioPreprocessor::Error OpusAudioPreprocessor::initializeRNNoise() {
    if (m_config.sampleRate != 48000) {
        // RNNoise requires 48kHz
        return Error::FeatureNotSupported;
    }
    
    if (m_config.framesPerBuffer != 480) {
        // RNNoise requires 480 samples (10ms at 48kHz)
        return Error::InvalidFrameSize;
    }
    
    m_rnnoise = rnnoise_create(nullptr);
    if (!m_rnnoise) {
        return Error::InitializationFailed;
    }
    
    return Error::None;
}

OpusAudioPreprocessor::Error OpusAudioPreprocessor::processRNNoise(float *samples, std::size_t frameCount) {
    if (!m_rnnoise || frameCount != 480) {
        return Error::ProcessingError;
    }
    
    // RNNoise processes 480 samples at a time
    rnnoise_process_frame(m_rnnoise, samples, samples);
    
    return Error::None;
}
#endif

OpusAudioPreprocessor::Error OpusAudioPreprocessor::processFrame(std::int16_t *samples, std::size_t frameCount) {
    if (!m_initialized) {
        return Error::ProcessingError;
    }
    
    if (frameCount != m_config.framesPerBuffer) {
        return Error::InvalidFrameSize;
    }
    
    std::size_t sampleCount = frameCount * m_config.channels;
    
    // Convert to float for processing
    convertInt16ToFloat(samples, m_tempBuffer.data(), sampleCount);
    
    // Process as float
    Error result = processFrameFloat(m_tempBuffer.data(), frameCount);
    if (result != Error::None) {
        return result;
    }
    
    // Convert back to int16
    convertFloatToInt16(m_tempBuffer.data(), samples, sampleCount);
    
    return Error::None;
}

OpusAudioPreprocessor::Error OpusAudioPreprocessor::processFrameFloat(float *samples, std::size_t frameCount) {
    if (!m_initialized) {
        return Error::ProcessingError;
    }
    
    if (frameCount != m_config.framesPerBuffer) {
        return Error::InvalidFrameSize;
    }
    
    std::size_t sampleCount = frameCount * m_config.channels;
    
    // Store input for statistics
    std::copy(samples, samples + sampleCount, m_processBuffer.data());
    
    // Apply high-pass filter
    if (m_config.enableHighPassFilter) {
        applyHighPassFilter(samples, frameCount);
    }
    
    // Apply noise reduction
    if (m_config.noiseReduction != NoiseReductionMode::Disabled) {
#ifdef USE_RNNOISE
        if (m_config.noiseReduction == NoiseReductionMode::RNNoise ||
            m_config.noiseReduction == NoiseReductionMode::Hybrid) {
            Error rnnoiseResult = processRNNoise(samples, frameCount);
            if (rnnoiseResult != Error::None) {
                return rnnoiseResult;
            }
        }
#endif
        
        // Apply WebRTC noise suppression
        processWebRTC(samples, frameCount);
    }
    
    // Update statistics
    updateStatistics(m_processBuffer.data(), samples, frameCount);
    
    m_frameCounter++;
    return Error::None;
}

float OpusAudioPreprocessor::detectVoiceActivity(const std::int16_t *samples, std::size_t frameCount) {
    if (!m_initialized) {
        return -1.0f;
    }
    
    // Convert to float and process
    std::size_t sampleCount = frameCount * m_config.channels;
    if (sampleCount > m_tempBuffer.size()) {
        return -1.0f;
    }
    
    convertInt16ToFloat(samples, m_tempBuffer.data(), sampleCount);
    return detectVoiceActivityFloat(m_tempBuffer.data(), frameCount);
}

float OpusAudioPreprocessor::detectVoiceActivityFloat(const float *samples, std::size_t frameCount) {
    if (!m_initialized) {
        return -1.0f;
    }
    
    // Simple energy-based VAD as fallback
    float energy = 0.0f;
    std::size_t sampleCount = frameCount * m_config.channels;
    
    for (std::size_t i = 0; i < sampleCount; ++i) {
        energy += samples[i] * samples[i];
    }
    
    energy = std::sqrt(energy / sampleCount);
    
    // Convert to probability (this is a simple heuristic)
    float probability = std::clamp(energy * 10.0f, 0.0f, 1.0f);
    
    return probability;
}

void OpusAudioPreprocessor::reset() {
    if (!m_initialized) {
        return;
    }
    
#ifdef USE_RNNOISE
    if (m_rnnoise) {
        rnnoise_destroy(m_rnnoise);
        m_rnnoise = rnnoise_create(nullptr);
    }
#endif
    
    // Reset filter state
    m_hpFilterState = 0.0f;
    
    // Reset statistics
    m_statistics = {};
    m_frameCounter = 0;
}

OpusAudioPreprocessor::Error OpusAudioPreprocessor::updateConfig(const Config &config) {
    m_config = config;
    return initialize(); // Re-initialize with new config
}

OpusAudioPreprocessor::Error OpusAudioPreprocessor::getStatistics(Statistics &stats) const {
    if (!m_initialized) {
        return Error::ProcessingError;
    }
    
    stats = m_statistics;
    return Error::None;
}

OpusAudioPreprocessor::PsdData OpusAudioPreprocessor::getInputPSD() const {
    // TODO: Implement PSD calculation
    return m_inputPSD;
}

OpusAudioPreprocessor::PsdData OpusAudioPreprocessor::getNoisePSD() const {
    // TODO: Implement noise PSD calculation
    return m_noisePSD;
}

OpusAudioPreprocessor::Error OpusAudioPreprocessor::setEchoCanceller(std::shared_ptr<OpusEchoCanceller> echoCanceller) {
    m_echoCanceller = echoCanceller;
    return Error::None;
}

OpusAudioPreprocessor::Error OpusAudioPreprocessor::setNoiseReductionEnabled(bool enabled) {
    if (enabled) {
        m_config.noiseReduction = NoiseReductionMode::RNNoise;
    } else {
        m_config.noiseReduction = NoiseReductionMode::Disabled;
    }
    return Error::None;
}

OpusAudioPreprocessor::Error OpusAudioPreprocessor::setNoiseReductionMode(NoiseReductionMode mode) {
    m_config.noiseReduction = mode;
    return Error::None;
}

OpusAudioPreprocessor::Error OpusAudioPreprocessor::setAgcEnabled(bool enabled) {
    m_config.agcMode = enabled ? AgcMode::AdaptiveDigital : AgcMode::Disabled;
    return Error::None;
}

OpusAudioPreprocessor::Error OpusAudioPreprocessor::setAgcTargetLevel(float targetLevelDb) {
    m_config.agcTargetLevel = targetLevelDb;
    return Error::None;
}

OpusAudioPreprocessor::Error OpusAudioPreprocessor::setVadEnabled(bool enabled) {
    m_config.vadMode = enabled ? VadMode::Normal : VadMode::Disabled;
    return Error::None;
}

bool OpusAudioPreprocessor::isValid() const {
    return m_initialized;
}

const char *OpusAudioPreprocessor::getErrorDescription(Error error) {
    switch (error) {
        case Error::None:
            return "No error";
        case Error::InvalidParameters:
            return "Invalid parameters";
        case Error::InitializationFailed:
            return "Initialization failed";
        case Error::ProcessingError:
            return "Processing error";
        case Error::InvalidSampleRate:
            return "Invalid sample rate";
        case Error::InvalidFrameSize:
            return "Invalid frame size";
        case Error::FeatureNotSupported:
            return "Feature not supported";
        default:
            return "Unknown error";
    }
}

bool OpusAudioPreprocessor::isRNNoiseAvailable() {
#ifdef USE_RNNOISE
    return true;
#else
    return false;
#endif
}

OpusAudioPreprocessor::Error OpusAudioPreprocessor::processWebRTC(float *samples, std::size_t frameCount) {
#ifdef USE_WEBRTC_AEC
    // TODO: Implement WebRTC processing
    return Error::None;
#else
    // No WebRTC available, no error
    return Error::None;
#endif
}

void OpusAudioPreprocessor::applyHighPassFilter(float *samples, std::size_t frameCount) {
    if (m_config.channels != 1) {
        return; // Simple filter only supports mono for now
    }
    
    // Simple single-pole high-pass filter
    float alpha = std::exp(-2.0f * M_PI * m_config.highPassCutoff / m_config.sampleRate);
    
    for (std::size_t i = 0; i < frameCount; ++i) {
        float input = samples[i];
        float output = alpha * (m_hpFilterState + input - samples[i]);
        m_hpFilterState = output;
        samples[i] = output;
    }
}

void OpusAudioPreprocessor::updateStatistics(
    const float *inputSamples,
    const float *outputSamples,
    std::size_t frameCount
) {
    std::size_t sampleCount = frameCount * m_config.channels;
    
    // Update input and output levels
    m_statistics.inputLevel = calculateRMSLevel(inputSamples, frameCount);
    m_statistics.outputLevel = calculateRMSLevel(outputSamples, frameCount);
    
    // Simple gain calculation
    if (m_statistics.inputLevel > -96.0f) {
        m_statistics.gainApplied = m_statistics.outputLevel - m_statistics.inputLevel;
    } else {
        m_statistics.gainApplied = 0.0f;
    }
    
    // Update frame counters
    m_statistics.framesProcessed++;
    
    // Simple speech detection based on energy
    float speechThreshold = -30.0f; // dBFS
    if (m_statistics.inputLevel > speechThreshold) {
        m_statistics.speechFrames++;
        m_statistics.speechProbability = 0.9f;
    } else {
        m_statistics.speechProbability = 0.1f;
    }
    
    // Estimate noise level (simple approach)
    if (m_statistics.speechProbability < 0.5f) {
        m_statistics.noiseLevel = 0.9f * m_statistics.noiseLevel + 0.1f * m_statistics.inputLevel;
    }
}

float OpusAudioPreprocessor::calculateRMSLevel(const float *samples, std::size_t frameCount) const {
    if (frameCount == 0) {
        return -96.0f;
    }
    
    float sum = 0.0f;
    std::size_t sampleCount = frameCount * m_config.channels;
    
    for (std::size_t i = 0; i < sampleCount; ++i) {
        sum += samples[i] * samples[i];
    }
    
    float rms = std::sqrt(sum / sampleCount);
    if (rms < 1e-10f) {
        return -96.0f;
    }
    
    return 20.0f * std::log10(rms);
}

void OpusAudioPreprocessor::convertInt16ToFloat(
    const std::int16_t *input,
    float *output,
    std::size_t sampleCount
) {
    for (std::size_t i = 0; i < sampleCount; ++i) {
        output[i] = static_cast<float>(input[i]) / 32768.0f;
    }
}

void OpusAudioPreprocessor::convertFloatToInt16(
    const float *input,
    std::int16_t *output,
    std::size_t sampleCount
) {
    for (std::size_t i = 0; i < sampleCount; ++i) {
        float sample = std::clamp(input[i], -1.0f, 1.0f);
        output[i] = static_cast<std::int16_t>(sample * 32767.0f);
    }
}