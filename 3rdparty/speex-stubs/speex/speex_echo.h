// Copyright The Mumble Developers. All rights reserved.
// Use of this source code is governed by a BSD-style license
// that can be found in the LICENSE file at the root of the
// Mumble source tree or at <https://www.mumble.info/LICENSE>.

// Stub header for speex_echo.h to allow compilation when using Opus audio processing
// This file provides minimal type definitions and function stubs to satisfy compilation
// requirements without requiring the full Speex library.

#ifndef SPEEX_ECHO_H
#define SPEEX_ECHO_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// Stub type definitions
typedef struct SpeexEchoState_ SpeexEchoState;
typedef int16_t spx_int16_t;
typedef int32_t spx_int32_t;
typedef uint32_t spx_uint32_t;

// Control requests
#define SPEEX_ECHO_GET_FRAME_SIZE 3
#define SPEEX_ECHO_SET_SAMPLING_RATE 24
#define SPEEX_ECHO_GET_SAMPLING_RATE 25
#define SPEEX_ECHO_GET_IMPULSE_RESPONSE_SIZE 27
#define SPEEX_ECHO_GET_IMPULSE_RESPONSE 29

// Stub function declarations - these will not actually work but satisfy compilation
SpeexEchoState *speex_echo_state_init(int frame_size, int filter_length);
SpeexEchoState *speex_echo_state_init_mc(int frame_size, int filter_length, int nb_mic, int nb_speakers);
void speex_echo_state_destroy(SpeexEchoState *st);
void speex_echo_cancellation(SpeexEchoState *st, const spx_int16_t *rec, const spx_int16_t *play, spx_int16_t *out);
void speex_echo_cancel(SpeexEchoState *st, const spx_int16_t *rec, const spx_int16_t *play, spx_int16_t *out, spx_int32_t *Yout);
void speex_echo_capture(SpeexEchoState *st, const spx_int16_t *rec, spx_int16_t *out);
void speex_echo_playback(SpeexEchoState *st, const spx_int16_t *play);
int speex_echo_ctl(SpeexEchoState *st, int request, void *ptr);



#ifdef __cplusplus
}
#endif

#endif /* SPEEX_ECHO_H */