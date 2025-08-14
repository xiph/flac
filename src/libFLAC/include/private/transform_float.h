
#include "FLAC/assert.h"
#include "FLAC/ordinals.h"

void FLAC__transform_f32_buffer_to_i32_signal(FLAC__int32 *dest, const FLAC__int32 *src, size_t n);

bool FLAC__transform_f32_interleaved_buffer_to_i32_signal(FLAC__int32 *dest, const FLAC__int32 *src, uint32_t *i, uint32_t *j, uint32_t *k, uint32_t *channel, struct FLAC__StreamEncoderProtected *protected_, const uint32_t current_sample_number, const uint32_t blocksize, const uint32_t samples, const uint32_t channels, const FLAC__int32 sample_min, const FLAC__int32 sample_max);

void FLAC__transform_i32_signal_to_f32_buffer(uint32_t *buffer, size_t n);
