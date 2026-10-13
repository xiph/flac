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
#include <stdint.h>
#include "FLAC/ordinals.h"
#include "FLAC/format.h"
#include "private/md5.h"
#include "private/cpu.h"

void FLAC__compute_channel_base_exp_float(const uint32_t * const raw_signals[], uint32_t channels, uint32_t blocksize, uint32_t channel_highest_exp[], uint32_t channel_lowest_exp[]);
void FLAC__combine_subframe_signals_fallback_to_f32_buffer(uint32_t *dest, const FLAC__int32 *exp_signal, const FLAC__int32 *sign_mant_signal, size_t n);
FLAC__bool FLAC__check_frame_lossless_float(const uint32_t * const raw_signals[], uint32_t channels, uint32_t blocksize, uint32_t channel_highest_exp[], uint32_t channel_lowest_exp[]);
void FLAC__transform_frame_fallback_float(FLAC__int32 *dest_signals[], const uint32_t * const raw_signals[], uint32_t channels, uint32_t blocksize);
void FLAC__transform_frame_dpa_float(FLAC__int32 *dest_signals32[], FLAC__int64 *dest_signals64[], const uint32_t * const raw_signals[], const uint32_t channel_highest_exp[], const uint32_t channel_lowest_exp[], FLAC__byte exponent_zero_offsets[], FLAC__byte shift_split[], uint32_t channel_bps_out[], uint32_t *split_point_out, uint32_t compression_level, uint32_t channels, uint32_t blocksize);
void FLAC__combine_subframe_signals_dpa_to_f32_buffer(uint32_t *dest, const void *upper_signal, FLAC__bool upper_is_64, const void *lower_signal, FLAC__bool lower_is_64, uint32_t shift_split, size_t n, FLAC__byte highest_exp, uint32_t total_width, uint32_t split_point);
FLAC__bool FLAC__MD5Accumulate_float(FLAC__MD5Context *ctx, const uint32_t * const float_signals[], uint32_t channels, uint32_t samples);


#if FLAC__HAS_X86INTRIN

#ifdef FLAC__SSE2_SUPPORTED
void FLAC__transform_frame_fallback_float_intrin_sse2(FLAC__int32 *dest_signals[], const uint32_t * const raw_signals[], uint32_t channels, uint32_t blocksize);
void FLAC__combine_subframe_signals_fallback_to_f32_buffer_intrin_sse2(uint32_t *dest, const FLAC__int32 *exp_signal, const FLAC__int32 *sign_mant_signal, size_t n);
#endif

#ifdef FLAC__SSE4_1_SUPPORTED
void FLAC__compute_channel_base_exp_float_intrin_sse41(const uint32_t * const raw_signals[], uint32_t channels, uint32_t blocksize, uint32_t channel_highest_exp[], uint32_t channel_lowest_exp[]);
FLAC__bool FLAC__check_frame_lossless_float_intrin_sse41(const uint32_t * const raw_signals[], uint32_t channels, uint32_t blocksize, uint32_t channel_highest_exp[], uint32_t channel_lowest_exp[]);
#endif

#ifdef FLAC__AVX2_SUPPORTED
void FLAC__compute_channel_base_exp_float_intrin_avx2(const uint32_t * const raw_signals[], uint32_t channels, uint32_t blocksize, uint32_t channel_highest_exp[], uint32_t channel_lowest_exp[]);
FLAC__bool FLAC__check_frame_lossless_float_intrin_avx2(const uint32_t * const raw_signals[], uint32_t channels, uint32_t blocksize, uint32_t channel_highest_exp[], uint32_t channel_lowest_exp[]);
void FLAC__transform_frame_dpa_float_intrin_avx2(FLAC__int32 *dest_signals32[], FLAC__int64 *dest_signals64[], const uint32_t * const raw_signals[], const uint32_t channel_highest_exp[], const uint32_t channel_lowest_exp[], FLAC__byte exponent_zero_offsets[], FLAC__byte shift_split[], uint32_t channel_bps_out[], uint32_t *split_point_out, uint32_t compression_level, uint32_t channels, uint32_t blocksize);
void FLAC__combine_subframe_signals_dpa_to_f32_buffer_intrin_avx2(uint32_t *dest, const void *upper_signal, FLAC__bool upper_is_64, const void *lower_signal, FLAC__bool lower_is_64, uint32_t shift_split, size_t n, FLAC__byte highest_exp, uint32_t total_width, uint32_t split_point);
void FLAC__transform_frame_fallback_float_intrin_avx2(FLAC__int32 *dest_signals[], const uint32_t * const raw_signals[], uint32_t channels, uint32_t blocksize);
void FLAC__combine_subframe_signals_fallback_to_f32_buffer_intrin_avx2(uint32_t *dest, const FLAC__int32 *exp_signal, const FLAC__int32 *sign_mant_signal, size_t n);
#endif

#endif

#endif
