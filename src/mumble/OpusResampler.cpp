// Copyright The Mumble Developers. All rights reserved.
// Use of this source code is governed by a BSD-style license
// that can be found in the LICENSE file at the root of the
// Mumble source tree or at <https://www.mumble.info/LICENSE>.

#include "OpusResampler.h"

#include <soxr.h>
#include <algorithm>
#include <cassert>
#include <vector>

OpusResampler::OpusResampler(
    std::uint32_t inputSampleRate,
    std::uint32_t outputSampleRate,
    std::uint32_t channels,
    Quality quality
) : m_inputSampleRate(inputSampleRate)
  , m_outputSampleRate(outputSampleRate)
  , m_channels(channels)
  , m_quality(quality)
  , m_soxr(nullptr) {
}

OpusResampler::~OpusResampler() {
    if (m_soxr) {
        soxr_delete(m_soxr);
        m_soxr = nullptr;
    }
}

OpusResampler::OpusResampler(OpusResampler &&other) noexcept
    : m_inputSampleRate(other.m_inputSampleRate)
    , m_outputSampleRate(other.m_outputSampleRate)
    , m_channels(other.m_channels)
    , m_quality(other.m_quality)
    , m_soxr(other.m_soxr) {
    other.m_soxr = nullptr;
}

OpusResampler &OpusResampler::operator=(OpusResampler &&other) noexcept {
    if (this != &other) {
        if (m_soxr) {
            soxr_delete(m_soxr);
        }
        
        m_inputSampleRate = other.m_inputSampleRate;
        m_outputSampleRate = other.m_outputSampleRate;
        m_channels = other.m_channels;
        m_quality = other.m_quality;
        m_soxr = other.m_soxr;
        
        other.m_soxr = nullptr;
    }
    return *this;
}

std::unique_ptr<OpusResampler> OpusResampler::create(
    std::uint32_t inputSampleRate,
    std::uint32_t outputSampleRate,
    std::uint32_t channels,
    Quality quality,
    Error *error
) {
    // Validate parameters
    if (inputSampleRate == 0 || outputSampleRate == 0 || channels == 0 || channels > 8) {
        if (error) *error = Error::InvalidParameters;
        return nullptr;
    }
    
    auto resampler = std::unique_ptr<OpusResampler>(
        new OpusResampler(inputSampleRate, outputSampleRate, channels, quality)
    );
    
    Error initError = resampler->initialize();
    if (initError != Error::None) {
        if (error) *error = initError;
        return nullptr;
    }
    
    if (error) *error = Error::None;
    return resampler;
}

OpusResampler::Error OpusResampler::initialize() {
    soxr_error_t soxrError;
    soxr_io_spec_t ioSpec = soxr_io_spec(SOXR_FLOAT32_I, SOXR_FLOAT32_I);
    soxr_quality_spec_t qualitySpec = soxr_quality_spec(qualityToSoxrSpec(m_quality), 0);
    soxr_runtime_spec_t runtimeSpec = soxr_runtime_spec(1); // Single-threaded
    
    double inputRate = static_cast<double>(m_inputSampleRate);
    double outputRate = static_cast<double>(m_outputSampleRate);
    
    m_soxr = soxr_create(inputRate, outputRate, m_channels, &soxrError, &ioSpec, &qualitySpec, &runtimeSpec);
    
    if (!m_soxr) {
        if (soxrError) {
            // Map soxr errors to our error codes
            return Error::InternalError;
        }
        return Error::OutOfMemory;
    }
    
    return Error::None;
}

OpusResampler::Error OpusResampler::processFloat(
    const float *input,
    std::size_t inputFrames,
    float *output,
    std::size_t outputFrames,
    std::size_t *inputUsed,
    std::size_t *outputGenerated
) {
    if (!m_soxr || m_channels != 1) {
        return Error::InvalidParameters;
    }
    
    size_t inputUsedInternal = 0;
    size_t outputGeneratedInternal = 0;
    
    soxr_error_t error = soxr_process(
        m_soxr,
        input, inputFrames, &inputUsedInternal,
        output, outputFrames, &outputGeneratedInternal
    );
    
    if (inputUsed) *inputUsed = inputUsedInternal;
    if (outputGenerated) *outputGenerated = outputGeneratedInternal;
    
    return error ? Error::ProcessingError : Error::None;
}

OpusResampler::Error OpusResampler::processInterleavedFloat(
    const float *input,
    std::size_t inputFrames,
    float *output,
    std::size_t outputFrames,
    std::size_t *inputUsed,
    std::size_t *outputGenerated
) {
    if (!m_soxr) {
        return Error::InvalidParameters;
    }
    
    size_t inputUsedInternal = 0;
    size_t outputGeneratedInternal = 0;
    
    soxr_error_t error = soxr_process(
        m_soxr,
        input, inputFrames, &inputUsedInternal,
        output, outputFrames, &outputGeneratedInternal
    );
    
    if (inputUsed) *inputUsed = inputUsedInternal;
    if (outputGenerated) *outputGenerated = outputGeneratedInternal;
    
    return error ? Error::ProcessingError : Error::None;
}

OpusResampler::Error OpusResampler::processInt16(
    const std::int16_t *input,
    std::size_t inputFrames,
    std::int16_t *output,
    std::size_t outputFrames,
    std::size_t *inputUsed,
    std::size_t *outputGenerated
) {
    // For int16 processing, we need to create a new resampler with int16 I/O spec
    // or convert to float. For simplicity, we'll convert to float internally.
    
    if (!m_soxr || m_channels != 1) {
        return Error::InvalidParameters;
    }
    
    // Convert input to float
    std::vector<float> floatInput(inputFrames);
    for (std::size_t i = 0; i < inputFrames; ++i) {
        floatInput[i] = static_cast<float>(input[i]) / 32768.0f;
    }
    
    // Process as float
    std::vector<float> floatOutput(outputFrames);
    std::size_t actualInputUsed, actualOutputGenerated;
    
    Error result = processFloat(
        floatInput.data(), inputFrames,
        floatOutput.data(), outputFrames,
        &actualInputUsed, &actualOutputGenerated
    );
    
    if (result != Error::None) {
        return result;
    }
    
    // Convert output back to int16
    for (std::size_t i = 0; i < actualOutputGenerated; ++i) {
        float sample = floatOutput[i] * 32768.0f;
        sample = std::clamp(sample, -32768.0f, 32767.0f);
        output[i] = static_cast<std::int16_t>(sample);
    }
    
    if (inputUsed) *inputUsed = actualInputUsed;
    if (outputGenerated) *outputGenerated = actualOutputGenerated;
    
    return Error::None;
}

OpusResampler::Error OpusResampler::processInterleavedInt16(
    const std::int16_t *input,
    std::size_t inputFrames,
    std::int16_t *output,
    std::size_t outputFrames,
    std::size_t *inputUsed,
    std::size_t *outputGenerated
) {
    if (!m_soxr) {
        return Error::InvalidParameters;
    }
    
    std::size_t inputSamples = inputFrames * m_channels;
    std::size_t outputSamples = outputFrames * m_channels;
    
    // Convert input to float
    std::vector<float> floatInput(inputSamples);
    for (std::size_t i = 0; i < inputSamples; ++i) {
        floatInput[i] = static_cast<float>(input[i]) / 32768.0f;
    }
    
    // Process as float
    std::vector<float> floatOutput(outputSamples);
    std::size_t actualInputUsed, actualOutputGenerated;
    
    Error result = processInterleavedFloat(
        floatInput.data(), inputFrames,
        floatOutput.data(), outputFrames,
        &actualInputUsed, &actualOutputGenerated
    );
    
    if (result != Error::None) {
        return result;
    }
    
    // Convert output back to int16
    std::size_t actualOutputSamples = actualOutputGenerated * m_channels;
    for (std::size_t i = 0; i < actualOutputSamples; ++i) {
        float sample = floatOutput[i] * 32768.0f;
        sample = std::clamp(sample, -32768.0f, 32767.0f);
        output[i] = static_cast<std::int16_t>(sample);
    }
    
    if (inputUsed) *inputUsed = actualInputUsed;
    if (outputGenerated) *outputGenerated = actualOutputGenerated;
    
    return Error::None;
}

void OpusResampler::reset() {
    if (m_soxr) {
        soxr_clear(m_soxr);
    }
}

std::size_t OpusResampler::getExpectedOutputFrames(std::size_t inputFrames) const {
    if (m_inputSampleRate == 0) {
        return 0;
    }
    
    // Calculate expected output frames based on the sample rate ratio
    double ratio = static_cast<double>(m_outputSampleRate) / static_cast<double>(m_inputSampleRate);
    return static_cast<std::size_t>(inputFrames * ratio + 0.5); // Round to nearest
}

std::size_t OpusResampler::getDelay() const {
    if (!m_soxr) {
        return 0;
    }
    
    // Get the delay from libsoxr
    size_t delay = soxr_delay(m_soxr);
    return delay;
}

const char *OpusResampler::getErrorDescription(Error error) {
    switch (error) {
        case Error::None:
            return "No error";
        case Error::InvalidParameters:
            return "Invalid parameters provided";
        case Error::OutOfMemory:
            return "Out of memory";
        case Error::InternalError:
            return "Internal error";
        case Error::InvalidQuality:
            return "Invalid quality setting";
        case Error::ProcessingError:
            return "Processing error";
        default:
            return "Unknown error";
    }
}

unsigned long OpusResampler::qualityToSoxrSpec(Quality quality) {
    switch (quality) {
        case Quality::Quick:
            return SOXR_QQ;         // Quick quality
        case Quality::Low:
            return SOXR_LQ;         // Low quality
        case Quality::Medium:
            return SOXR_MQ;         // Medium quality
        case Quality::High:
            return SOXR_HQ;         // High quality
        case Quality::VeryHigh:
            return SOXR_VHQ;        // Very high quality
        default:
            return SOXR_MQ;         // Default to medium quality
    }
}