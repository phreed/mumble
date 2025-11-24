// Copyright The Mumble Developers. All rights reserved.
// Use of this source code is governed by a BSD-style license
// that can be found in the LICENSE file at the root of the
// Mumble source tree or at <https://www.mumble.info/LICENSE>.

// Stub header for speex_resampler.h to allow compilation when using Opus audio processing
// This file provides minimal type definitions and function stubs to satisfy compilation
// requirements without requiring the full Speex library.

#ifndef SPEEX_RESAMPLER_H
#define SPEEX_RESAMPLER_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// Stub type definitions
typedef struct SpeexResamplerState_ SpeexResamplerState;
typedef uint32_t spx_uint32_t;
typedef int32_t spx_int32_t;

// Error codes
#define RESAMPLER_ERR_SUCCESS 0
#define RESAMPLER_ERR_ALLOC_FAILED 1
#define RESAMPLER_ERR_BAD_STATE 2
#define RESAMPLER_ERR_INVALID_ARG 3
#define RESAMPLER_ERR_PTR_OVERLAP 4

// Stub function declarations - these will not actually work but satisfy compilation
SpeexResamplerState *speex_resampler_init(spx_uint32_t nb_channels, spx_uint32_t in_rate, spx_uint32_t out_rate, int quality, int *err);
void speex_resampler_destroy(SpeexResamplerState *st);
int speex_resampler_process_float(SpeexResamplerState *st, spx_uint32_t channel_index, const float *in, spx_uint32_t *in_len, float *out, spx_uint32_t *out_len);
int speex_resampler_process_interleaved_float(SpeexResamplerState *st, const float *in, spx_uint32_t *in_len, float *out, spx_uint32_t *out_len);
int speex_resampler_set_rate(SpeexResamplerState *st, spx_uint32_t in_rate, spx_uint32_t out_rate);
void speex_resampler_get_rate(SpeexResamplerState *st, spx_uint32_t *in_rate, spx_uint32_t *out_rate);
int speex_resampler_set_quality(SpeexResamplerState *st, int quality);
void speex_resampler_get_quality(SpeexResamplerState *st, int *quality);
int speex_resampler_skip_zeros(SpeexResamplerState *st);
int speex_resampler_reset_mem(SpeexResamplerState *st);
const char *speex_resampler_strerror(int err);

#ifdef __cplusplus
}
#endif

#endif /* SPEEX_RESAMPLER_H */