// Copyright The Mumble Developers. All rights reserved.
// Use of this source code is governed by a BSD-style license
// that can be found in the LICENSE file at the root of the
// Mumble source tree or at <https://www.mumble.info/LICENSE>.

// Stub implementations for Speex functions to allow compilation when using Opus audio processing
// These functions do nothing but return appropriate values to satisfy compilation requirements
// without requiring the full Speex library.

#include "speex/speex_jitter.h"
#include "speex/speex_resampler.h"
#include "speex/speex_preprocess.h"
#include "speex/speex_echo.h"
#include <stdlib.h>
#include <string.h>

// Dummy structures
struct JitterBuffer {
    int dummy;
};

struct SpeexResamplerState_ {
    int dummy;
};

struct SpeexPreprocessState_ {
    int dummy;
};

struct SpeexEchoState_ {
    int dummy;
};

// Jitter buffer stubs
JitterBuffer *jitter_buffer_init(int step) {
    (void)step;
    return (JitterBuffer*)malloc(sizeof(struct JitterBuffer));
}

void jitter_buffer_destroy(JitterBuffer *jitter) {
    free(jitter);
}

void jitter_buffer_put(JitterBuffer *jitter, const JitterBufferPacket *packet) {
    (void)jitter;
    (void)packet;
    // Do nothing
}

int jitter_buffer_get(JitterBuffer *jitter, JitterBufferPacket *packet, int desired_span, int *start_offset) {
    (void)jitter;
    (void)packet;
    (void)desired_span;
    if (start_offset) *start_offset = 0;
    return JITTER_BUFFER_MISSING; // Always return missing to avoid processing
}

int jitter_buffer_get_pointer_timestamp(JitterBuffer *jitter) {
    (void)jitter;
    return 0;
}

int jitter_buffer_ctl(JitterBuffer *jitter, int request, void *ptr) {
    (void)jitter;
    (void)request;
    (void)ptr;
    return 0;
}

void jitter_buffer_tick(JitterBuffer *jitter) {
    (void)jitter;
    // Do nothing
}

void jitter_buffer_update_delay(JitterBuffer *jitter, JitterBufferPacket *packet, int *optimal_delay) {
    (void)jitter;
    (void)packet;
    if (optimal_delay) *optimal_delay = 0;
}

// Resampler stubs
SpeexResamplerState *speex_resampler_init(spx_uint32_t nb_channels, spx_uint32_t in_rate, spx_uint32_t out_rate, int quality, int *err) {
    (void)nb_channels;
    (void)in_rate;
    (void)out_rate;
    (void)quality;
    if (err) *err = RESAMPLER_ERR_SUCCESS;
    return NULL; // Return NULL to indicate no resampling needed
}

void speex_resampler_destroy(SpeexResamplerState *st) {
    free(st);
}

int speex_resampler_process_float(SpeexResamplerState *st, spx_uint32_t channel_index, const float *in, spx_uint32_t *in_len, float *out, spx_uint32_t *out_len) {
    (void)st;
    (void)channel_index;
    
    // Just copy input to output without resampling
    if (in && out && in_len && out_len) {
        spx_uint32_t copy_len = (*in_len < *out_len) ? *in_len : *out_len;
        memcpy(out, in, copy_len * sizeof(float));
        *in_len = copy_len;
        *out_len = copy_len;
    }
    return RESAMPLER_ERR_SUCCESS;
}

int speex_resampler_process_interleaved_float(SpeexResamplerState *st, const float *in, spx_uint32_t *in_len, float *out, spx_uint32_t *out_len) {
    (void)st;
    
    // Just copy input to output without resampling
    if (in && out && in_len && out_len) {
        spx_uint32_t copy_len = (*in_len < *out_len) ? *in_len : *out_len;
        memcpy(out, in, copy_len * sizeof(float));
        *in_len = copy_len;
        *out_len = copy_len;
    }
    return RESAMPLER_ERR_SUCCESS;
}

int speex_resampler_set_rate(SpeexResamplerState *st, spx_uint32_t in_rate, spx_uint32_t out_rate) {
    (void)st;
    (void)in_rate;
    (void)out_rate;
    return RESAMPLER_ERR_SUCCESS;
}

void speex_resampler_get_rate(SpeexResamplerState *st, spx_uint32_t *in_rate, spx_uint32_t *out_rate) {
    (void)st;
    if (in_rate) *in_rate = 48000;
    if (out_rate) *out_rate = 48000;
}

int speex_resampler_set_quality(SpeexResamplerState *st, int quality) {
    (void)st;
    (void)quality;
    return RESAMPLER_ERR_SUCCESS;
}

void speex_resampler_get_quality(SpeexResamplerState *st, int *quality) {
    (void)st;
    if (quality) *quality = 3;
}

int speex_resampler_skip_zeros(SpeexResamplerState *st) {
    (void)st;
    return RESAMPLER_ERR_SUCCESS;
}

int speex_resampler_reset_mem(SpeexResamplerState *st) {
    (void)st;
    return RESAMPLER_ERR_SUCCESS;
}

const char *speex_resampler_strerror(int err) {
    switch (err) {
        case RESAMPLER_ERR_SUCCESS: return "Success";
        case RESAMPLER_ERR_ALLOC_FAILED: return "Memory allocation failed";
        case RESAMPLER_ERR_BAD_STATE: return "Bad resampler state";
        case RESAMPLER_ERR_INVALID_ARG: return "Invalid argument";
        case RESAMPLER_ERR_PTR_OVERLAP: return "Pointer overlap";
        default: return "Unknown error";
    }
}

// Preprocessor stubs
SpeexPreprocessState *speex_preprocess_state_init(int frame_size, int sampling_rate) {
    (void)frame_size;
    (void)sampling_rate;
    return (SpeexPreprocessState*)malloc(sizeof(struct SpeexPreprocessState_));
}

void speex_preprocess_state_destroy(SpeexPreprocessState *st) {
    free(st);
}

int speex_preprocess_run(SpeexPreprocessState *st, int16_t *x) {
    (void)st;
    (void)x;
    return 1; // Always return "voice detected"
}

int speex_preprocess_ctl(SpeexPreprocessState *st, int request, void *ptr) {
    (void)st;
    (void)request;
    (void)ptr;
    return 0;
}

void speex_preprocess_estimate_update(SpeexPreprocessState *st, int16_t *x) {
    (void)st;
    (void)x;
    // Do nothing
}

int speex_preprocess(SpeexPreprocessState *st, int16_t *x, int32_t *echo) {
    (void)st;
    (void)x;
    (void)echo;
    return 1; // Always return "voice detected"
}

// Echo cancellation stubs
SpeexEchoState *speex_echo_state_init(int frame_size, int filter_length) {
    (void)frame_size;
    (void)filter_length;
    return (SpeexEchoState*)malloc(sizeof(struct SpeexEchoState_));
}

SpeexEchoState *speex_echo_state_init_mc(int frame_size, int filter_length, int nb_mic, int nb_speakers) {
    (void)frame_size;
    (void)filter_length;
    (void)nb_mic;
    (void)nb_speakers;
    return (SpeexEchoState*)malloc(sizeof(struct SpeexEchoState_));
}

void speex_echo_state_destroy(SpeexEchoState *st) {
    free(st);
}

void speex_echo_cancellation(SpeexEchoState *st, const int16_t *rec, const int16_t *play, int16_t *out) {
    (void)st;
    (void)play;
    // Just copy input to output without echo cancellation
    if (rec && out) {
        // We don't know the frame size here, so this is a minimal stub
        // In real usage, this would need proper frame size handling
    }
}

void speex_echo_cancel(SpeexEchoState *st, const int16_t *rec, const int16_t *play, int16_t *out, int32_t *Yout) {
    (void)st;
    (void)play;
    (void)Yout;
    // Just copy input to output without echo cancellation
    if (rec && out) {
        // We don't know the frame size here, so this is a minimal stub
    }
}

void speex_echo_capture(SpeexEchoState *st, const int16_t *rec, int16_t *out) {
    (void)st;
    // Just copy input to output
    if (rec && out) {
        // We don't know the frame size here, so this is a minimal stub
    }
}

void speex_echo_playback(SpeexEchoState *st, const int16_t *play) {
    (void)st;
    (void)play;
    // Do nothing
}

int speex_echo_ctl(SpeexEchoState *st, int request, void *ptr) {
    (void)st;
    (void)request;
    if (ptr) {
        // Set some reasonable defaults for common requests
        switch (request) {
            case 3: // SPEEX_ECHO_GET_FRAME_SIZE
                *(int*)ptr = 480;
                break;
            case 25: // SPEEX_ECHO_GET_SAMPLING_RATE
                *(int*)ptr = 48000;
                break;
            case 27: // SPEEX_ECHO_GET_IMPULSE_RESPONSE_SIZE
                *(int*)ptr = 0;
                break;
            default:
                break;
        }
    }
    return 0;
}