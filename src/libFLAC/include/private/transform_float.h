/* libFLAC - Free Lossless Audio Codec library
 * Copyright (C) 2025-2026 Xiph.Org Foundation
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * - Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *
 * - Redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in the
 * documentation and/or other materials provided with the distribution.
 *
 * - Neither the name of the Xiph.org Foundation nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef FLAC__PRIVATE__TRANSFORM_FLOAT_H
#define FLAC__PRIVATE__TRANSFORM_FLOAT_H

#include <stddef.h>
#include "FLAC/assert.h"
#include "FLAC/ordinals.h"
#include "FLAC/format.h"
#include "private/md5.h"

void FLAC__split_f32_buffer_to_subframe_signals(FLAC__int32 *exp_signal, FLAC__int32 *sign_mant_signal, const uint32_t *src, size_t n);
void FLAC__split_f32_interleaved_to_subframe_signals(FLAC__int32 * const integer_signals[], uint32_t channels, uint32_t current_sample_number, const uint32_t *src, size_t n);
void FLAC__compute_channel_base_exp_float(const FLAC__int32 * const split_signals[], uint32_t channels, uint32_t blocksize, uint32_t channel_base_exp[], uint32_t *frame_bps);
void FLAC__combine_subframe_signals_to_f32_buffer(uint32_t *dest, const FLAC__int32 *subframe0, const FLAC__int32 *subframe1, size_t n, FLAC__byte base_exp, FLAC__bool is_fallback);
FLAC__bool FLAC__check_frame_lossless_float(const FLAC__int32 * const split_signals[], uint32_t channels, uint32_t blocksize, uint32_t channel_base_exp[], uint32_t *frame_bps);
void FLAC__transform_frame_fallback_float(FLAC__int32 *signals[], uint32_t channels, uint32_t blocksize);
void FLAC__transform_frame_lossless_1subframe_float(FLAC__int32 *signals[], const uint32_t channel_base_exp[], FLAC__byte exponent_zero_offsets[], uint32_t channels, uint32_t blocksize);
FLAC__bool FLAC__MD5Accumulate_split_float(FLAC__MD5Context *ctx, const FLAC__int32 * const split_signals[], uint32_t channels, uint32_t samples);
FLAC__bool FLAC__MD5Accumulate_float(FLAC__MD5Context *ctx, const uint32_t * const float_signals[], uint32_t channels, uint32_t samples);

#endif
