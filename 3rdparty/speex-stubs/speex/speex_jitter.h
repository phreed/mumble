// Copyright The Mumble Developers. All rights reserved.
// Use of this source code is governed by a BSD-style license
// that can be found in the LICENSE file at the root of the
// Mumble source tree or at <https://www.mumble.info/LICENSE>.

// Stub header for speex_jitter.h to allow compilation when using Opus audio processing
// This file provides minimal type definitions and function stubs to satisfy compilation
// requirements without requiring the full Speex library.

#ifndef SPEEX_JITTER_H
#define SPEEX_JITTER_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// Stub type definitions
typedef struct JitterBuffer JitterBuffer;

typedef struct {
    char *data;
    uint32_t len;
    uint32_t timestamp;
    uint32_t span;
} JitterBufferPacket;

// Return values
#define JITTER_BUFFER_OK 0
#define JITTER_BUFFER_MISSING 1
#define JITTER_BUFFER_INSERTION 2

// Control requests
#define JITTER_BUFFER_SET_MARGIN 0
#define JITTER_BUFFER_GET_MARGIN 1
#define JITTER_BUFFER_SET_DESTROY_CALLBACK 2
#define JITTER_BUFFER_GET_AVAILABLE_COUNT 3

// Stub function declarations - these will not actually work but satisfy compilation
JitterBuffer *jitter_buffer_init(int step);
void jitter_buffer_destroy(JitterBuffer *jitter);
void jitter_buffer_put(JitterBuffer *jitter, const JitterBufferPacket *packet);
int jitter_buffer_get(JitterBuffer *jitter, JitterBufferPacket *packet, int desired_span, int *start_offset);
int jitter_buffer_get_pointer_timestamp(JitterBuffer *jitter);
int jitter_buffer_ctl(JitterBuffer *jitter, int request, void *ptr);
void jitter_buffer_tick(JitterBuffer *jitter);
void jitter_buffer_update_delay(JitterBuffer *jitter, JitterBufferPacket *packet, int *optimal_delay);

#ifdef __cplusplus
}
#endif

#endif /* SPEEX_JITTER_H */