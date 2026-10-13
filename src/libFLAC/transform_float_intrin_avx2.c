/* libFLAC - Free Lossless Audio Codec library
 * Copyright (C) 2000-2009  Josh Coalson
 * Copyright (C) 2011-2025  Xiph.Org Foundation
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

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include "private/cpu.h"

#ifndef FLAC__NO_ASM
#if (defined FLAC__CPU_IA32 || defined FLAC__CPU_X86_64) && FLAC__HAS_X86INTRIN
#include <stdlib.h>
#include "private/transform_float.h"
#include "private/macros.h"
#include "private/bitmath.h"
#ifdef FLAC__AVX2_SUPPORTED

#include <immintrin.h> /* AVX2 */

FLAC__SSE_TARGET("avx2")
static inline int hmax_avx2(__m256i v) {
	__m128i v128 = _mm_max_epi32(_mm256_castsi256_si128(v), _mm256_extracti128_si256(v, 1));
	__m128i m1 = _mm_max_epi32(v128, _mm_shuffle_epi32(v128, _MM_SHUFFLE(1, 0, 3, 2)));
	__m128i m2 = _mm_max_epi32(m1, _mm_shuffle_epi32(m1, _MM_SHUFFLE(2, 3, 0, 1)));
	return _mm_cvtsi128_si32(m2);
}

FLAC__SSE_TARGET("avx2")
static inline int hmin_avx2(__m256i v) {
	__m128i v128 = _mm_min_epi32(_mm256_castsi256_si128(v), _mm256_extracti128_si256(v, 1));
	__m128i m1 = _mm_min_epi32(v128, _mm_shuffle_epi32(v128, _MM_SHUFFLE(1, 0, 3, 2)));
	__m128i m2 = _mm_min_epi32(m1, _mm_shuffle_epi32(m1, _MM_SHUFFLE(2, 3, 0, 1)));
	return _mm_cvtsi128_si32(m2);
}

FLAC__SSE_TARGET("avx2")
void FLAC__compute_channel_base_exp_float_intrin_avx2(const uint32_t * const raw_signals[], uint32_t channels, uint32_t blocksize, uint32_t channel_highest_exp[], uint32_t channel_lowest_exp[])
{
	uint32_t ch;
	int ch_highest[FLAC__MAX_CHANNELS];
	int ch_lowest[FLAC__MAX_CHANNELS];

	const __m256i v_0xff = _mm256_set1_epi32(0xFF);
	const __m256i v_0x7fffff = _mm256_set1_epi32(0x7FFFFF);
	const __m256i v_0x7fffffff = _mm256_set1_epi32(0x7FFFFFFF);
	const __m256i v_implicit_bit = _mm256_set1_epi32(1 << 23);
	const __m256i v_zero = _mm256_setzero_si256();
	const __m256i v_127 = _mm256_set1_epi32(127);
	const __m256i v_23 = _mm256_set1_epi32(23);
	const __m256i v_neg1 = _mm256_set1_epi32(-1);
	const __m256i v_1000 = _mm256_set1_epi32(1000);

	for (ch = 0; ch < channels; ch++) {
		int highest_pos = -1;
		int lowest_pos = 1000;
		uint32_t s = 0;
		const uint32_t *raw_ch = raw_signals[ch];

		__m256i v_max = _mm256_set1_epi32(-1);
		__m256i v_min = _mm256_set1_epi32(1000);

		for (; s + 7 < blocksize; s += 8) {
			__m256i v_raw = _mm256_loadu_si256((const __m256i*)(const void*)(raw_ch + s));
			__m256i v_exp = _mm256_and_si256(_mm256_srli_epi32(v_raw, 23), v_0xff);
			__m256i v_mant = _mm256_and_si256(v_raw, v_0x7fffff);
			__m256i v_is_zero = _mm256_cmpeq_epi32(_mm256_and_si256(v_raw, v_0x7fffffff), v_zero);
			__m256i v_explicit_mant = _mm256_or_si256(v_mant, v_implicit_bit);
			__m256i v_lsb = _mm256_and_si256(v_explicit_mant, _mm256_sub_epi32(v_zero, v_explicit_mant));
			__m256 v_flt = _mm256_cvtepi32_ps(v_lsb);
			__m256i v_tz = _mm256_sub_epi32(_mm256_srli_epi32(_mm256_castps_si256(v_flt), 23), v_127);
			__m256i v_pos_high = v_exp;
			__m256i v_pos_low = _mm256_add_epi32(_mm256_sub_epi32(v_exp, v_23), v_tz);

			v_pos_high = _mm256_blendv_epi8(v_pos_high, v_neg1, v_is_zero);
			v_pos_low  = _mm256_blendv_epi8(v_pos_low,  v_1000, v_is_zero);

			v_max = _mm256_max_epi32(v_max, v_pos_high);
			v_min = _mm256_min_epi32(v_min, v_pos_low);
		}

		highest_pos = hmax_avx2(v_max);
		lowest_pos = hmin_avx2(v_min);

		for (; s < blocksize; s++) {
			uint32_t raw = raw_ch[s];
			uint32_t exp = (raw >> 23) & 0xFF;
			uint32_t mant = raw & 0x7FFFFF;
			if ((raw & 0x7FFFFFFF) != 0) {
				uint32_t explicit_mant = (1u << 23) | mant;
				uint32_t lsb = explicit_mant & (0u - explicit_mant);
				union { float f; uint32_t u; } u_flt;
				int tz;
				int pos_high;
				int pos_low;

				u_flt.f = (float)lsb;
				tz = (int)((u_flt.u >> 23) & 0xFF) - 127;
				pos_high = (int)exp;
				pos_low = (int)exp - 23 + tz;
				if (pos_high > highest_pos)
					highest_pos = pos_high;
				if (pos_low < lowest_pos)
					lowest_pos = pos_low;
			}
		}

		ch_highest[ch] = highest_pos;
		ch_lowest[ch] = lowest_pos;
		if (highest_pos == -1) {
			channel_highest_exp[ch] = 127;
			channel_lowest_exp[ch] = 127;
		} else {
			channel_highest_exp[ch] = (uint32_t)highest_pos;
			channel_lowest_exp[ch] = (uint32_t)lowest_pos;
		}
	}

	if (channels == 2 && ch_highest[0] != -1 && ch_highest[1] != -1) {
		int max_h = flac_max(ch_highest[0], ch_highest[1]);
		int min_l = flac_min(ch_lowest[0], ch_lowest[1]);
		if (max_h <= 254) {
			uint32_t combined_bps = (uint32_t)(max_h - min_l + 2);
#if FLAC__HAS_INT128
			if (combined_bps <= 63) {
				channel_highest_exp[0] = channel_highest_exp[1] = (uint32_t)max_h;
				channel_lowest_exp[0] = channel_lowest_exp[1] = (uint32_t)min_l;
			}
#else
			if (combined_bps <= 32) {
				channel_highest_exp[0] = channel_highest_exp[1] = (uint32_t)max_h;
				channel_lowest_exp[0] = channel_lowest_exp[1] = (uint32_t)min_l;
			}
#endif
		}
	}
}

FLAC__SSE_TARGET("avx2")
FLAC__bool FLAC__check_frame_lossless_float_intrin_avx2(const uint32_t * const raw_signals[], uint32_t channels, uint32_t blocksize, uint32_t channel_highest_exp[], uint32_t channel_lowest_exp[])
{
	uint32_t ch;
	int ch_highest[FLAC__MAX_CHANNELS];
	int ch_lowest[FLAC__MAX_CHANNELS];

	const __m256i v_0xff = _mm256_set1_epi32(0xFF);
	const __m256i v_0x7fffff = _mm256_set1_epi32(0x7FFFFF);
	const __m256i v_0x7fffffff = _mm256_set1_epi32(0x7FFFFFFF);
	const __m256i v_neg_zero = _mm256_set1_epi32((int)0x80000000);
	const __m256i v_implicit_bit = _mm256_set1_epi32(1 << 23);
	const __m256i v_zero = _mm256_setzero_si256();
	const __m256i v_127 = _mm256_set1_epi32(127);
	const __m256i v_23 = _mm256_set1_epi32(23);
	const __m256i v_neg1 = _mm256_set1_epi32(-1);
	const __m256i v_1000 = _mm256_set1_epi32(1000);

	for (ch = 0; ch < channels; ch++) {
		int highest_pos = -1;
		int lowest_pos = 1000;
		uint32_t s = 0;
		const uint32_t *raw_ch = raw_signals[ch];

		__m256i v_max = _mm256_set1_epi32(-1);
		__m256i v_min = _mm256_set1_epi32(1000);
		__m256i v_bad = _mm256_setzero_si256();

		for (; s + 7 < blocksize; s += 8) {
			__m256i v_raw = _mm256_loadu_si256((const __m256i*)(const void*)(raw_ch + s));
			__m256i v_exp = _mm256_and_si256(_mm256_srli_epi32(v_raw, 23), v_0xff);
			__m256i v_mant = _mm256_and_si256(v_raw, v_0x7fffff);
			__m256i v_is_zero = _mm256_cmpeq_epi32(_mm256_and_si256(v_raw, v_0x7fffffff), v_zero);

			__m256i v_is_neg_zero = _mm256_cmpeq_epi32(v_raw, v_neg_zero);
			__m256i v_is_subnormal = _mm256_andnot_si256(v_is_zero, _mm256_cmpeq_epi32(v_exp, v_zero));
			__m256i v_is_inf_nan = _mm256_cmpeq_epi32(v_exp, v_0xff);

			__m256i v_explicit_mant = _mm256_or_si256(v_mant, v_implicit_bit);
			__m256i v_lsb = _mm256_and_si256(v_explicit_mant, _mm256_sub_epi32(v_zero, v_explicit_mant));
			__m256 v_flt = _mm256_cvtepi32_ps(v_lsb);
			__m256i v_tz = _mm256_sub_epi32(_mm256_srli_epi32(_mm256_castps_si256(v_flt), 23), v_127);
			__m256i v_pos_high = v_exp;
			__m256i v_pos_low = _mm256_add_epi32(_mm256_sub_epi32(v_exp, v_23), v_tz);

			v_bad = _mm256_or_si256(v_bad, _mm256_or_si256(v_is_neg_zero, _mm256_or_si256(v_is_subnormal, v_is_inf_nan)));

			v_pos_high = _mm256_blendv_epi8(v_pos_high, v_neg1, v_is_zero);
			v_pos_low  = _mm256_blendv_epi8(v_pos_low,  v_1000, v_is_zero);

			v_max = _mm256_max_epi32(v_max, v_pos_high);
			v_min = _mm256_min_epi32(v_min, v_pos_low);
		}

		if (!_mm256_testz_si256(v_bad, v_bad))
			return false;

		highest_pos = hmax_avx2(v_max);
		lowest_pos = hmin_avx2(v_min);

		for (; s < blocksize; s++) {
			uint32_t raw = raw_ch[s];
			uint32_t exp = (raw >> 23) & 0xFF;
			uint32_t sign = (raw >> 31) & 1;
			uint32_t mant = raw & 0x7FFFFF;
			if (exp == 0 && mant == 0) {
				if (sign != 0)
					return false;
			} else if (exp == 0 || exp == 255) {
				return false;
			} else {
				uint32_t explicit_mant = (1u << 23) | mant;
				uint32_t lsb = explicit_mant & (0u - explicit_mant);
				union { float f; uint32_t u; } u_flt;
				int tz;
				int pos_high;
				int pos_low;

				u_flt.f = (float)lsb;
				tz = (int)((u_flt.u >> 23) & 0xFF) - 127;
				pos_high = (int)exp;
				pos_low = (int)exp - 23 + tz;
				if (pos_high > highest_pos)
					highest_pos = pos_high;
				if (pos_low < lowest_pos)
					lowest_pos = pos_low;
			}
		}

		ch_highest[ch] = highest_pos;
		ch_lowest[ch] = lowest_pos;
		if (highest_pos == -1) {
			channel_highest_exp[ch] = 127;
			channel_lowest_exp[ch] = 127;
		} else {
			uint32_t ch_bps;
			if (highest_pos > 254)
				return false;
			ch_bps = (uint32_t)(highest_pos - lowest_pos + 2);
#if FLAC__HAS_INT128
			if (ch_bps > 63)
				return false;
#else
			if (ch_bps > 32)
				return false;
#endif
			channel_highest_exp[ch] = (uint32_t)highest_pos;
			channel_lowest_exp[ch] = (uint32_t)lowest_pos;
		}
	}

	if (channels == 2 && ch_highest[0] != -1 && ch_highest[1] != -1) {
		int max_h = flac_max(ch_highest[0], ch_highest[1]);
		int min_l = flac_min(ch_lowest[0], ch_lowest[1]);
		if (max_h <= 254) {
			uint32_t combined_bps = (uint32_t)(max_h - min_l + 2);
#if FLAC__HAS_INT128
			if (combined_bps <= 63) {
				channel_highest_exp[0] = channel_highest_exp[1] = (uint32_t)max_h;
				channel_lowest_exp[0] = channel_lowest_exp[1] = (uint32_t)min_l;
			}
#else
			if (combined_bps <= 32) {
				channel_highest_exp[0] = channel_highest_exp[1] = (uint32_t)max_h;
				channel_lowest_exp[0] = channel_lowest_exp[1] = (uint32_t)min_l;
			}
#endif
		}
	}

	return true;
}

FLAC__SSE_TARGET("avx2")
void FLAC__transform_frame_fallback_float_intrin_avx2(FLAC__int32 *dest_signals[], const uint32_t * const raw_signals[], uint32_t channels, uint32_t blocksize)
{
	uint32_t ch, s;
	const __m256i v_0xff = _mm256_set1_epi32(0xFF);
	const __m256i v_0x7fffff = _mm256_set1_epi32(0x7FFFFF);
	const __m256i v_128 = _mm256_set1_epi32(128);
	const __m256i v_neg_8388608 = _mm256_set1_epi32(-8388608);
	const __m256i v_zero = _mm256_setzero_si256();

	for (ch = 0; ch < channels; ch++) {
		FLAC__int32 *exp_out = dest_signals[2 * ch];
		FLAC__int32 *sign_mant_out = dest_signals[2 * ch + 1];
		const uint32_t *raw_ch = raw_signals[ch];

		for (s = 0; s + 7 < blocksize; s += 8) {
			__m256i v_raw = _mm256_loadu_si256((const __m256i*)(const void*)(raw_ch + s));
			__m256i v_exp = _mm256_sub_epi32(_mm256_and_si256(_mm256_srli_epi32(v_raw, 23), v_0xff), v_128);
			__m256i v_mant = _mm256_and_si256(v_raw, v_0x7fffff);
			__m256i v_sign_mask = _mm256_srai_epi32(v_raw, 31);
			__m256i v_sign_mant = _mm256_sub_epi32(_mm256_xor_si256(v_mant, v_sign_mask), v_sign_mask);
			__m256i v_is_zero_mant = _mm256_cmpeq_epi32(v_mant, v_zero);
			__m256i v_is_neg_zero = _mm256_and_si256(v_is_zero_mant, v_sign_mask);
		v_sign_mant = _mm256_blendv_epi8(v_sign_mant, v_neg_8388608, v_is_neg_zero);

			_mm256_storeu_si256((__m256i*)(void*)(exp_out + s), v_exp);
			_mm256_storeu_si256((__m256i*)(void*)(sign_mant_out + s), v_sign_mant);
		}

		for (; s < blocksize; s++) {
			uint32_t raw = raw_ch[s];
			uint32_t exp = (raw >> 23) & 0xFF;
			uint32_t sign = (raw >> 31) & 1;
			uint32_t mant = raw & 0x7FFFFF;
			FLAC__int32 sm;
			if (sign && mant == 0) {
				sm = -8388608;
			} else if (sign) {
				sm = -(FLAC__int32)mant;
			} else {
				sm = (FLAC__int32)mant;
			}
			exp_out[s] = (FLAC__int32)exp - 128;
			sign_mant_out[s] = sm;
		}
	}
}

FLAC__SSE_TARGET("avx2")
void FLAC__combine_subframe_signals_fallback_to_f32_buffer_intrin_avx2(uint32_t *dest, const FLAC__int32 *exp_signal, const FLAC__int32 *sign_mant_signal, size_t n)
{
	size_t i = 0;
	const __m256i v_0xff = _mm256_set1_epi32(0xFF);
	const __m256i v_0x7fffff = _mm256_set1_epi32(0x7FFFFF);
	const __m256i v_sign_bit = _mm256_set1_epi32((int)0x80000000);
	const __m256i v_128 = _mm256_set1_epi32(128);
	const __m256i v_neg_8388608 = _mm256_set1_epi32(-8388608);

	for (; i + 7 < n; i += 8) {
		__m256i v_exp = _mm256_and_si256(_mm256_add_epi32(_mm256_loadu_si256((const __m256i*)(const void*)(exp_signal + i)), v_128), v_0xff);
		__m256i v_sm = _mm256_loadu_si256((const __m256i*)(const void*)(sign_mant_signal + i));
		__m256i v_sign_mask = _mm256_srai_epi32(v_sm, 31);
		__m256i v_abs_sm = _mm256_sub_epi32(_mm256_xor_si256(v_sm, v_sign_mask), v_sign_mask);
		__m256i v_mant = _mm256_and_si256(v_abs_sm, v_0x7fffff);
		__m256i v_is_special = _mm256_cmpeq_epi32(v_sm, v_neg_8388608);
		__m256i v_sign = _mm256_and_si256(v_sign_mask, v_sign_bit);
		__m256i v_dest;
		v_mant = _mm256_andnot_si256(v_is_special, v_mant);
		v_dest = _mm256_or_si256(_mm256_or_si256(v_sign, _mm256_slli_epi32(v_exp, 23)), v_mant);

		_mm256_storeu_si256((__m256i*)(void*)(dest + i), v_dest);
	}

	for (; i < n; i++) {
		const uint32_t orig_exp = (uint32_t)(exp_signal[i] + 128) & 0xFF;
		const FLAC__int32 sm = sign_mant_signal[i];
		uint32_t sign, orig_mant;
		if (sm == -8388608) {
			sign = 1;
			orig_mant = 0;
		} else if (sm < 0) {
			sign = 1;
			orig_mant = (uint32_t)(-sm) & 0x7FFFFF;
		} else {
			sign = 0;
			orig_mant = (uint32_t)sm & 0x7FFFFF;
		}
		dest[i] = (sign << 31) | (orig_exp << 23) | orig_mant;
	}
}

FLAC__SSE_TARGET("avx2")
void FLAC__transform_frame_dpa_float_intrin_avx2(FLAC__int32 *dest_signals32[], FLAC__int64 *dest_signals64[], const uint32_t * const raw_signals[], const uint32_t channel_highest_exp[], const uint32_t channel_lowest_exp[], FLAC__byte exponent_zero_offsets[], FLAC__byte shift_split_out[], uint32_t channel_bps_out[], uint32_t *split_point_out, uint32_t compression_level, uint32_t channels, uint32_t blocksize)
{
	uint32_t ch, s;
	uint32_t channel_bps[FLAC__MAX_CHANNELS];
	uint32_t split_point = 24;

	if (compression_level >= 8 || getenv("FLAC_DPA_SPLIT_POINT")) {
		FLAC__transform_frame_dpa_float(dest_signals32, dest_signals64, raw_signals, channel_highest_exp, channel_lowest_exp, exponent_zero_offsets, shift_split_out, channel_bps_out, split_point_out, compression_level, channels, blocksize);
		return;
	}

	for (ch = 0; ch < channels; ch++) {
		uint32_t highest_exp = channel_highest_exp[ch];
		uint32_t lowest_exp = channel_lowest_exp[ch];
		uint32_t bps = (highest_exp >= lowest_exp) ? (highest_exp - lowest_exp + 2) : 1;
		if (bps > 63)
			bps = 63;
		channel_bps[ch] = bps;
		exponent_zero_offsets[ch] = (FLAC__byte)highest_exp;
	}

	*split_point_out = split_point;

	for (ch = 0; ch < channels; ch++) {
		FLAC__int32 *upper_out32 = dest_signals32[2 * ch];
		FLAC__int64 *upper_out64 = dest_signals64 ? dest_signals64[2 * ch] : NULL;
		FLAC__int32 *lower_out32 = dest_signals32[2 * ch + 1];
		FLAC__int64 *lower_out64 = dest_signals64 ? dest_signals64[2 * ch + 1] : NULL;
		uint32_t ch_bps = channel_bps[ch];
		uint32_t shift_split = (ch_bps > split_point) ? (ch_bps - split_point) : 0;
		uint32_t highest_exp = channel_highest_exp[ch];
		uint32_t lowest_exp = channel_lowest_exp[ch];
		const uint32_t *raw_ch = raw_signals[ch];

		s = 0;
		if (shift_split == 0) {
			const __m256i v_base_shift = _mm256_set1_epi32(23 + (int)lowest_exp);
			const __m256i v_exp_mask = _mm256_set1_epi32(0xFF);
			const __m256i v_mant_mask = _mm256_set1_epi32(0x7FFFFF);
			const __m256i v_implicit_bit = _mm256_set1_epi32(1 << 23);
			const __m256i v_zero = _mm256_setzero_si256();

			for (; s + 7 < blocksize; s += 8) {
				__m256i v_raw = _mm256_loadu_si256((const __m256i*)(const void*)(raw_ch + s));
				__m256i v_exp = _mm256_and_si256(_mm256_srli_epi32(v_raw, 23), v_exp_mask);
				__m256i v_sign_mask = _mm256_srai_epi32(v_raw, 31);
				__m256i v_mant = _mm256_or_si256(_mm256_and_si256(v_raw, v_mant_mask), v_implicit_bit);
				__m256i v_is_zero = _mm256_cmpeq_epi32(_mm256_and_si256(v_raw, _mm256_set1_epi32(0x7FFFFFFF)), v_zero);

				__m256i v_shift_right = _mm256_sub_epi32(v_base_shift, v_exp);
				__m256i v_val = _mm256_srlv_epi32(v_mant, v_shift_right);
				__m256i v_upper = _mm256_sub_epi32(_mm256_xor_si256(v_val, v_sign_mask), v_sign_mask);
				v_upper = _mm256_andnot_si256(v_is_zero, v_upper);

				if (upper_out32) _mm256_storeu_si256((__m256i*)(void*)(upper_out32 + s), v_upper);
				if (lower_out32) _mm256_storeu_si256((__m256i*)(void*)(lower_out32 + s), v_zero);
				if (upper_out64) {
					_mm256_storeu_si256((__m256i*)(void*)(upper_out64 + s), _mm256_cvtepi32_epi64(_mm256_castsi256_si128(v_upper)));
					_mm256_storeu_si256((__m256i*)(void*)(upper_out64 + s + 4), _mm256_cvtepi32_epi64(_mm256_extracti128_si256(v_upper, 1)));
				}
				if (lower_out64) {
					_mm256_storeu_si256((__m256i*)(void*)(lower_out64 + s), v_zero);
					_mm256_storeu_si256((__m256i*)(void*)(lower_out64 + s + 4), v_zero);
				}
			}

			for (; s < blocksize; s++) {
				uint32_t r = raw_ch[s];
				uint32_t exp = (r >> 23) & 0xFF;
				uint32_t sign = (r >> 31) & 1;
				uint32_t mant = r & 0x7FFFFF;

				if ((r & 0x7FFFFFFF) == 0) {
				if (upper_out32) upper_out32[s] = 0;
				if (upper_out64) upper_out64[s] = 0;
				if (lower_out32) lower_out32[s] = 0;
				if (lower_out64) lower_out64[s] = 0;
				} else {
					int shift = ((int)exp - 23) - (int)lowest_exp;
					uint32_t explicit_mant = (1u << 23) | mant;
					uint32_t val = (shift >= 0) ? (explicit_mant << shift) : (explicit_mant >> (-shift));
					int32_t signed_val = sign ? -(int32_t)val : (int32_t)val;
					if (upper_out32) upper_out32[s] = signed_val;
					if (upper_out64) upper_out64[s] = (FLAC__int64)signed_val;
					if (lower_out32) lower_out32[s] = 0;
					if (lower_out64) lower_out64[s] = 0;
				}
			}
		} else {
			uint32_t mask_val = (shift_split >= 32) ? 0xFFFFFFFFu : ((shift_split > 0) ? ((1u << shift_split) - 1) : 0);
			__m256i v_mask = _mm256_set1_epi32(mask_val);
			__m256i v_highest_exp = _mm256_set1_epi32(highest_exp);
			__m256i v_H = _mm256_set1_epi32(shift_split);
			__m256i v_one = _mm256_set1_epi32(1);
			__m256i v_zero = _mm256_setzero_si256();
			__m256i v_exp_mask = _mm256_set1_epi32(0xFF);
			__m256i v_mant_mask = _mm256_set1_epi32(0x7FFFFF);
			__m256i v_implicit_bit = _mm256_set1_epi32(1 << 23);

			for (; s + 7 < blocksize; s += 8) {
				__m256i v_raw = _mm256_loadu_si256((const __m256i*)(const void*)(raw_ch + s));
				__m256i v_exp = _mm256_and_si256(_mm256_srli_epi32(v_raw, 23), v_exp_mask);
				__m256i v_sign_mask = _mm256_srai_epi32(v_raw, 31);
				__m256i v_mant = _mm256_or_si256(_mm256_and_si256(v_raw, v_mant_mask), v_implicit_bit);

				__m256i v_delta = _mm256_sub_epi32(v_highest_exp, v_exp);
				__m256i v_shift_down = _mm256_add_epi32(v_one, v_delta);
				__m256i v_up_pos = _mm256_srlv_epi32(v_mant, v_shift_down);
				__m256i v_shift_left = _mm256_sub_epi32(v_H, v_shift_down);
				__m256i v_shift_right = _mm256_sub_epi32(v_shift_down, v_H);
				__m256i v_low_left = _mm256_sllv_epi32(v_mant, v_shift_left);
				__m256i v_low_right = _mm256_srlv_epi32(v_mant, v_shift_right);
				__m256i v_gt = _mm256_cmpgt_epi32(v_shift_down, v_H);
				__m256i v_low_pos = _mm256_and_si256(_mm256_blendv_epi8(v_low_left, v_low_right, v_gt), v_mask);

				__m256i v_is_zero = _mm256_cmpeq_epi32(_mm256_and_si256(v_raw, _mm256_set1_epi32(0x7FFFFFFF)), v_zero);
				__m256i v_has_borrow = _mm256_cmpgt_epi32(v_low_pos, v_zero);
				__m256i v_borrow_int = _mm256_and_si256(v_has_borrow, v_one);

				__m256i v_up_neg = _mm256_sub_epi32(_mm256_sub_epi32(v_zero, v_up_pos), v_borrow_int);
				__m256i v_low_neg = _mm256_and_si256(_mm256_sub_epi32(v_zero, v_low_pos), v_mask);

				__m256i v_upper = _mm256_blendv_epi8(v_up_pos, v_up_neg, v_sign_mask);
				__m256i v_lower = _mm256_blendv_epi8(v_low_pos, v_low_neg, v_sign_mask);

				v_upper = _mm256_andnot_si256(v_is_zero, v_upper);
			v_lower = _mm256_andnot_si256(v_is_zero, v_lower);

				if (upper_out32) _mm256_storeu_si256((__m256i*)(void*)(upper_out32 + s), v_upper);
				if (lower_out32) _mm256_storeu_si256((__m256i*)(void*)(lower_out32 + s), v_lower);
				if (upper_out64) {
					_mm256_storeu_si256((__m256i*)(void*)(upper_out64 + s), _mm256_cvtepi32_epi64(_mm256_castsi256_si128(v_upper)));
					_mm256_storeu_si256((__m256i*)(void*)(upper_out64 + s + 4), _mm256_cvtepi32_epi64(_mm256_extracti128_si256(v_upper, 1)));
				}
				if (lower_out64) {
					_mm256_storeu_si256((__m256i*)(void*)(lower_out64 + s), _mm256_cvtepu32_epi64(_mm256_castsi256_si128(v_lower)));
					_mm256_storeu_si256((__m256i*)(void*)(lower_out64 + s + 4), _mm256_cvtepu32_epi64(_mm256_extracti128_si256(v_lower, 1)));
				}
			}

			for (; s < blocksize; s++) {
				uint32_t r = raw_ch[s];
				uint32_t exp = (r >> 23) & 0xFF;
				uint32_t sign = (r >> 31) & 1;
				uint32_t mant = r & 0x7FFFFF;

				if ((r & 0x7FFFFFFF) == 0) {
					if (upper_out32) upper_out32[s] = 0;
					if (upper_out64) upper_out64[s] = 0;
					if (lower_out32) lower_out32[s] = 0;
					if (lower_out64) lower_out64[s] = 0;
				} else {
					uint32_t explicit_mant = (1u << 23) | mant;
					uint32_t delta = highest_exp - exp;
					uint32_t shift_down = 1 + delta;
					uint32_t up_pos = (shift_down < 32) ? (explicit_mant >> shift_down) : 0;
					uint32_t shift_up = shift_split - shift_down;
					uint32_t low_pos = (shift_split >= shift_down) ? ((explicit_mant << shift_up) & mask_val) : ((explicit_mant >> (shift_down - shift_split)) & mask_val);
					int borrow = (shift_split > 0 && low_pos != 0);
					int32_t up = sign ? (-(int32_t)up_pos - borrow) : (int32_t)up_pos;
					uint32_t low = sign ? ((-(int32_t)low_pos) & mask_val) : low_pos;

					if (upper_out32) upper_out32[s] = up;
					if (upper_out64) upper_out64[s] = (FLAC__int64)up;
					if (lower_out32) lower_out32[s] = (int32_t)low;
					if (lower_out64) lower_out64[s] = (FLAC__int64)low;
				}
			}
		}

		shift_split_out[ch] = (FLAC__byte)shift_split;
		if (channel_bps_out)
			channel_bps_out[ch] = ch_bps;
	}
}

FLAC__SSE_TARGET("avx2")
void FLAC__combine_subframe_signals_dpa_to_f32_buffer_intrin_avx2(uint32_t *dest, const void *upper_signal, FLAC__bool upper_is_64, const void *lower_signal, FLAC__bool lower_is_64, uint32_t shift_split, size_t n, FLAC__byte highest_exp, uint32_t total_width, uint32_t split_point)
{
	size_t i;
	const uint64_t mask = (shift_split >= 64) ? (~(uint64_t)0) : ((shift_split > 0) ? (((uint64_t)1 << shift_split) - 1) : 0);
	const FLAC__int32 *up32 = upper_is_64 ? NULL : (const FLAC__int32 *)upper_signal;
	const FLAC__int64 *up64 = upper_is_64 ? (const FLAC__int64 *)upper_signal : NULL;
	const FLAC__int32 *low32 = lower_is_64 ? NULL : (const FLAC__int32 *)lower_signal;
	const FLAC__int64 *low64 = lower_is_64 ? (const FLAC__int64 *)lower_signal : NULL;
	const uint32_t eff_width = (total_width > 0) ? total_width : split_point;
	const int base_exp = (int)highest_exp - (int)(eff_width - 2);

	/* High-speed scalar unrolled loop leveraging bsrq/lzcnt */
	for (i = 0; i < n; i++) {
		FLAC__int64 up = upper_is_64 ? up64[i] : (FLAC__int64)up32[i];
		FLAC__int64 low_val = lower_is_64 ? low64[i] : (FLAC__int64)low32[i];
		uint64_t low_u = (uint64_t)low_val & mask;
		FLAC__int64 full_x;

		if (shift_split == 0)
			full_x = up;
		else
			full_x = (FLAC__int64)(((uint64_t)up << shift_split) | low_u);

		if (full_x == 0) {
			dest[i] = 0;
		} else {
			uint32_t sign = (full_x < 0) ? 1 : 0;
			uint64_t uval = (full_x < 0) ? (uint64_t)(-full_x) : (uint64_t)full_x;
			int leading_bit_pos = (int)FLAC__bitmath_ilog2_wide(uval);
			int shift = leading_bit_pos - 23;
			uint32_t orig_exp = (uint32_t)(base_exp + leading_bit_pos);
			uint32_t explicit_mant = (shift >= 0) ? (uint32_t)(uval >> shift) : (uint32_t)(uval << (-shift));
			uint32_t orig_mant = explicit_mant & 0x7FFFFF;
			dest[i] = (sign << 31) | (orig_exp << 23) | orig_mant;
		}
	}
}

#endif
#endif
#endif
