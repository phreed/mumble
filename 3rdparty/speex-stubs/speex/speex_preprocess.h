// Copyright The Mumble Developers. All rights reserved.
// Use of this source code is governed by a BSD-style license
// that can be found in the LICENSE file at the root of the
// Mumble source tree or at <https://www.mumble.info/LICENSE>.

// Stub header for speex_preprocess.h to allow compilation when using Opus audio processing
// This file provides minimal type definitions and function stubs to satisfy compilation
// requirements without requiring the full Speex library.

#ifndef SPEEX_PREPROCESS_H
#define SPEEX_PREPROCESS_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// Stub type definitions
typedef struct SpeexPreprocessState_ SpeexPreprocessState;
typedef struct SpeexEchoState_ SpeexEchoState;

// Control requests
#define SPEEX_PREPROCESS_SET_DENOISE 0
#define SPEEX_PREPROCESS_GET_DENOISE 1
#define SPEEX_PREPROCESS_SET_AGC 2
#define SPEEX_PREPROCESS_GET_AGC 3
#define SPEEX_PREPROCESS_SET_VAD 4
#define SPEEX_PREPROCESS_GET_VAD 5
#define SPEEX_PREPROCESS_SET_AGC_LEVEL 6
#define SPEEX_PREPROCESS_GET_AGC_LEVEL 7
#define SPEEX_PREPROCESS_SET_DEREVERB 8
#define SPEEX_PREPROCESS_GET_DEREVERB 9
#define SPEEX_PREPROCESS_SET_DEREVERB_LEVEL 10
#define SPEEX_PREPROCESS_GET_DEREVERB_LEVEL 11
#define SPEEX_PREPROCESS_SET_DEREVERB_DECAY 12
#define SPEEX_PREPROCESS_GET_DEREVERB_DECAY 13
#define SPEEX_PREPROCESS_SET_PROB_START 14
#define SPEEX_PREPROCESS_GET_PROB_START 15
#define SPEEX_PREPROCESS_SET_PROB_CONTINUE 16
#define SPEEX_PREPROCESS_GET_PROB_CONTINUE 17
#define SPEEX_PREPROCESS_SET_NOISE_SUPPRESS 18
#define SPEEX_PREPROCESS_GET_NOISE_SUPPRESS 19
#define SPEEX_PREPROCESS_SET_ECHO_SUPPRESS 20
#define SPEEX_PREPROCESS_GET_ECHO_SUPPRESS 21
#define SPEEX_PREPROCESS_SET_ECHO_SUPPRESS_ACTIVE 22
#define SPEEX_PREPROCESS_GET_ECHO_SUPPRESS_ACTIVE 23
#define SPEEX_PREPROCESS_SET_ECHO_STATE 24
#define SPEEX_PREPROCESS_GET_ECHO_STATE 25
#define SPEEX_PREPROCESS_SET_AGC_INCREMENT 26
#define SPEEX_PREPROCESS_GET_AGC_INCREMENT 27
#define SPEEX_PREPROCESS_SET_AGC_DECREMENT 28
#define SPEEX_PREPROCESS_GET_AGC_DECREMENT 29
#define SPEEX_PREPROCESS_SET_AGC_MAX_GAIN 30
#define SPEEX_PREPROCESS_GET_AGC_MAX_GAIN 31

// Stub function declarations - these will not actually work but satisfy compilation
SpeexPreprocessState *speex_preprocess_state_init(int frame_size, int sampling_rate);
void speex_preprocess_state_destroy(SpeexPreprocessState *st);
int speex_preprocess_run(SpeexPreprocessState *st, int16_t *x);
int speex_preprocess_ctl(SpeexPreprocessState *st, int request, void *ptr);
void speex_preprocess_estimate_update(SpeexPreprocessState *st, int16_t *x);
int speex_preprocess(SpeexPreprocessState *st, int16_t *x, int32_t *echo);

#ifdef __cplusplus
}
#endif

#endif /* SPEEX_PREPROCESS_H */