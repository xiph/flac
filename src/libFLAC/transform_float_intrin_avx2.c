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
void FLAC__transform_frame_dpa_float_intrin_avx2(FLAC__int32 *dest_signals32[], FLAC__int64 *dest_signals64[], const uint32_t * const raw_signals[], const uint32_t channel_highest_exp[], const uint32_t channel_lowest_exp[], FLAC__byte exponent_zero_offsets[], FLAC__byte lower_bits_out[], uint32_t *upper_bits_out, uint32_t compression_level, uint32_t channels, uint32_t blocksize)
{
	uint32_t ch, s;
	uint32_t upper_bits = 24;
	const char *env_up = getenv("FLAC_DPA_UPPER_BITS");
	if (!env_up) env_up = getenv("FLAC_DPA_SPLIT_POINT");

	if (compression_level >= 8 || env_up) {
		FLAC__transform_frame_dpa_float(dest_signals32, dest_signals64, raw_signals, channel_highest_exp, channel_lowest_exp, exponent_zero_offsets, lower_bits_out, upper_bits_out, compression_level, channels, blocksize);
		return;
	}

	for (ch = 0; ch < channels; ch++) {
		uint32_t highest_exp = channel_highest_exp[ch];
		exponent_zero_offsets[ch] = (FLAC__byte)highest_exp;
	}

	*upper_bits_out = upper_bits;

	for (ch = 0; ch < channels; ch++) {
		FLAC__int32 *upper_out32 = dest_signals32[2 * ch];
		FLAC__int64 *upper_out64 = dest_signals64 ? dest_signals64[2 * ch] : NULL;
		FLAC__int32 *lower_out32 = dest_signals32[2 * ch + 1];
		FLAC__int64 *lower_out64 = dest_signals64 ? dest_signals64[2 * ch + 1] : NULL;
		uint32_t highest_exp = channel_highest_exp[ch];
		uint32_t lowest_exp = channel_lowest_exp[ch];
		uint32_t ch_bps = (highest_exp >= lowest_exp) ? (highest_exp - lowest_exp + 2) : 1;
		uint32_t lower_bits = (ch_bps > upper_bits) ? (ch_bps - upper_bits) : 0;
		const uint32_t *raw_ch = raw_signals[ch];

		if (lower_bits > 23) lower_bits = 23;

		s = 0;
		if (lower_bits == 0) {
			const __m256i v_highest_exp = _mm256_set1_epi32(highest_exp);
			const __m256i v_one = _mm256_set1_epi32(1);
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
				__m256i v_is_special = _mm256_or_si256(_mm256_cmpeq_epi32(v_exp, v_zero), _mm256_cmpeq_epi32(v_exp, v_exp_mask));
				__m256i v_raw_mask = _mm256_or_si256(v_is_zero, v_is_special);

				__m256i v_delta = _mm256_sub_epi32(v_highest_exp, v_exp);
				__m256i v_shift_down = _mm256_add_epi32(v_one, v_delta);
				__m256i v_val = _mm256_srlv_epi32(v_mant, v_shift_down);
				__m256i v_recovered = _mm256_sllv_epi32(v_val, v_shift_down);
				__m256i v_lost = _mm256_andnot_si256(_mm256_cmpeq_epi32(v_recovered, v_mant), _mm256_set1_epi32(-1));
				__m256i v_raw_all = _mm256_or_si256(v_raw_mask, _mm256_or_si256(_mm256_cmpeq_epi32(v_val, v_zero), v_lost));
				__m256i v_upper = _mm256_sub_epi32(_mm256_xor_si256(v_val, v_sign_mask), v_sign_mask);
				__m256i v_lower = _mm256_blendv_epi8(v_zero, v_raw, v_raw_all);
				v_upper = _mm256_andnot_si256(v_raw_all, v_upper);

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

				if (exp == 0 || exp == 255) {
					if (upper_out32) upper_out32[s] = 0;
					if (upper_out64) upper_out64[s] = 0;
					if (lower_out32) lower_out32[s] = (int32_t)r;
					if (lower_out64) lower_out64[s] = (FLAC__int64)(uint64_t)r;
				} else {
					uint32_t explicit_mant = (1u << 23) | mant;
					uint32_t delta = highest_exp - exp;
					uint32_t shift_down = 1 + delta;
					uint32_t val = (shift_down < 32) ? (explicit_mant >> shift_down) : 0;
					int bits_lost = (shift_down >= 32) || ((val << shift_down) != explicit_mant);
					if (val == 0 || bits_lost) {
						if (upper_out32) upper_out32[s] = 0;
						if (upper_out64) upper_out64[s] = 0;
						if (lower_out32) lower_out32[s] = (int32_t)r;
						if (lower_out64) lower_out64[s] = (FLAC__int64)(uint64_t)r;
					} else {
						int32_t signed_val = sign ? -(int32_t)val : (int32_t)val;
						if (upper_out32) upper_out32[s] = signed_val;
						if (upper_out64) upper_out64[s] = (FLAC__int64)signed_val;
						if (lower_out32) lower_out32[s] = 0;
						if (lower_out64) lower_out64[s] = 0;
					}
				}
			}
		} else {
			uint32_t mask_val = (lower_bits > 0) ? ((1u << lower_bits) - 1) : 0;
			__m256i v_mask = _mm256_set1_epi32(mask_val);
			__m256i v_highest_exp = _mm256_set1_epi32(highest_exp);
			__m256i v_H = _mm256_set1_epi32(lower_bits);
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
				__m256i v_is_special = _mm256_or_si256(_mm256_cmpeq_epi32(v_exp, v_zero), _mm256_cmpeq_epi32(v_exp, v_exp_mask));
				__m256i v_has_borrow = _mm256_cmpgt_epi32(v_low_pos, v_zero);
				__m256i v_borrow_int = _mm256_and_si256(v_has_borrow, v_one);

				__m256i v_up_neg = _mm256_sub_epi32(_mm256_sub_epi32(v_zero, v_up_pos), v_borrow_int);
				__m256i v_low_neg = _mm256_and_si256(_mm256_sub_epi32(v_zero, v_low_pos), v_mask);

				__m256i v_recovered = _mm256_sllv_epi32(v_low_right, v_shift_right);
				__m256i v_lost = _mm256_and_si256(v_gt, _mm256_andnot_si256(_mm256_cmpeq_epi32(v_recovered, v_mant), _mm256_set1_epi32(-1)));
				__m256i v_upper = _mm256_blendv_epi8(v_up_pos, v_up_neg, v_sign_mask);
				__m256i v_lower = _mm256_blendv_epi8(v_low_pos, v_low_neg, v_sign_mask);
				__m256i v_is_small = _mm256_or_si256(_mm256_or_si256(_mm256_or_si256(_mm256_cmpeq_epi32(v_up_pos, v_zero), v_is_zero), v_is_special), v_lost);

			v_upper = _mm256_andnot_si256(v_is_small, v_upper);
			v_lower = _mm256_blendv_epi8(v_lower, v_raw, v_is_small);

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
					if (lower_out32) lower_out32[s] = (int32_t)r;
					if (lower_out64) lower_out64[s] = (FLAC__int64)(uint64_t)r;
				} else {
					uint32_t explicit_mant = (1u << 23) | mant;
					uint32_t delta = highest_exp - exp;
					uint32_t shift_down = 1 + delta;
					uint32_t up_pos = (shift_down < 32) ? (explicit_mant >> shift_down) : 0;
					int bits_lost = (shift_down > lower_bits) && (shift_down - lower_bits >= 32 || ((explicit_mant >> (shift_down - lower_bits)) << (shift_down - lower_bits)) != explicit_mant);
					if (up_pos == 0 || bits_lost) {
						if (upper_out32) upper_out32[s] = 0;
						if (upper_out64) upper_out64[s] = 0;
						if (lower_out32) lower_out32[s] = (int32_t)r;
						if (lower_out64) lower_out64[s] = (FLAC__int64)(uint64_t)r;
					} else {
						uint32_t shift_up = lower_bits - shift_down;
						uint32_t low_pos = (lower_bits >= shift_down) ? ((explicit_mant << shift_up) & mask_val) : ((explicit_mant >> (shift_down - lower_bits)) & mask_val);
					int borrow = (lower_bits > 0 && low_pos != 0);
					int32_t up = sign ? (-(int32_t)up_pos - borrow) : (int32_t)up_pos;
					uint32_t low = sign ? ((-(int32_t)low_pos) & mask_val) : low_pos;

						if (upper_out32) upper_out32[s] = up;
						if (upper_out64) upper_out64[s] = (FLAC__int64)up;
						if (lower_out32) lower_out32[s] = (int32_t)low;
						if (lower_out64) lower_out64[s] = (FLAC__int64)low;
					}
				}
			}
		}

		lower_bits_out[ch] = (FLAC__byte)lower_bits;
	}
}

FLAC__SSE_TARGET("avx2")
void FLAC__combine_subframe_signals_dpa_to_f32_buffer_intrin_avx2(uint32_t *dest, const void *upper_signal, FLAC__bool upper_is_64, const void *lower_signal, FLAC__bool lower_is_64, uint32_t lower_bits, size_t n, FLAC__byte highest_exp, uint32_t upper_bits)
{
	size_t i = 0;
	const uint64_t mask = (lower_bits > 0) ? (((uint64_t)1 << lower_bits) - 1) : 0;
	const FLAC__int32 *up32 = upper_is_64 ? NULL : (const FLAC__int32 *)upper_signal;
	const FLAC__int64 *up64 = upper_is_64 ? (const FLAC__int64 *)upper_signal : NULL;
	const FLAC__int32 *low32 = lower_is_64 ? NULL : (const FLAC__int32 *)lower_signal;
	const FLAC__int64 *low64 = lower_is_64 ? (const FLAC__int64 *)lower_signal : NULL;
	const uint32_t eff_width = upper_bits + lower_bits;
	const int base_exp = (int)highest_exp - (int)(eff_width - 2);

	if (!upper_is_64 && !lower_is_64) {
		const __m256i v_mask64 = _mm256_set1_epi64x((int64_t)mask);
		const int64_t exp_offset = (int64_t)(base_exp - 1023);
		const __m256i v_exp_offset = _mm256_set1_epi64x(exp_offset);
		const __m256i v_mant_mask = _mm256_set1_epi64x(0x7FFFFF);
		const __m256i v_sign_mask_u = _mm256_set1_epi64x((int64_t)0x80000000u);

		for (; i + 7 < n; i += 8) {
			__m128i up128_0, low128_0, res_lo0, res_hi0, res32_0, is_up_zero_0, final32_0;
			__m256i up64_0, low64_0, low_u_0, full_x_0, is_neg_0, neg_full_0, uval_0;
			int64_t u_arr0[4];
			__m256d v_dbl_0;
			__m256i d_bits_0, exp_raw_0, orig_exp_0, exp_shifted_0, mant_shifted_0, sign_shifted_0, res64_0;

			__m128i up128_1, low128_1, res_lo1, res_hi1, res32_1, is_up_zero_1, final32_1;
			__m256i up64_1, low64_1, low_u_1, full_x_1, is_neg_1, neg_full_1, uval_1;
			int64_t u_arr1[4];
			__m256d v_dbl_1;
			__m256i d_bits_1, exp_raw_1, orig_exp_1, exp_shifted_1, mant_shifted_1, sign_shifted_1, res64_1;

			up128_0 = _mm_loadu_si128((const __m128i*)(up32 + i));
			low128_0 = _mm_loadu_si128((const __m128i*)(low32 + i));

			up64_0 = _mm256_cvtepi32_epi64(up128_0);
			low64_0 = _mm256_cvtepi32_epi64(low128_0);
			low_u_0 = _mm256_and_si256(low64_0, v_mask64);
			full_x_0 = _mm256_or_si256(_mm256_slli_epi64(up64_0, lower_bits), low_u_0);
			is_neg_0 = _mm256_cmpgt_epi64(_mm256_setzero_si256(), full_x_0);
			neg_full_0 = _mm256_sub_epi64(_mm256_setzero_si256(), full_x_0);
			uval_0 = _mm256_blendv_epi8(full_x_0, neg_full_0, is_neg_0);

			_mm256_storeu_si256((__m256i*)u_arr0, uval_0);
			v_dbl_0 = _mm256_set_pd((double)u_arr0[3], (double)u_arr0[2], (double)u_arr0[1], (double)u_arr0[0]);
			d_bits_0 = _mm256_castpd_si256(v_dbl_0);

			exp_raw_0 = _mm256_srli_epi64(d_bits_0, 52);
			orig_exp_0 = _mm256_add_epi64(exp_raw_0, v_exp_offset);
			exp_shifted_0 = _mm256_slli_epi64(orig_exp_0, 23);
			mant_shifted_0 = _mm256_and_si256(_mm256_srli_epi64(d_bits_0, 29), v_mant_mask);
			sign_shifted_0 = _mm256_and_si256(_mm256_srli_epi64(is_neg_0, 32), v_sign_mask_u);
			res64_0 = _mm256_or_si256(_mm256_or_si256(sign_shifted_0, exp_shifted_0), mant_shifted_0);

			res_lo0 = _mm256_castsi256_si128(res64_0);
			res_hi0 = _mm256_extracti128_si256(res64_0, 1);
			res32_0 = (__m128i)_mm_shuffle_ps((__m128)res_lo0, (__m128)res_hi0, _MM_SHUFFLE(2, 0, 2, 0));
			is_up_zero_0 = _mm_cmpeq_epi32(up128_0, _mm_setzero_si128());
			final32_0 = _mm_blendv_epi8(res32_0, low128_0, is_up_zero_0);
			_mm_storeu_si128((__m128i*)(dest + i), final32_0);

			/* Second 4 samples */
			up128_1 = _mm_loadu_si128((const __m128i*)(up32 + i + 4));
			low128_1 = _mm_loadu_si128((const __m128i*)(low32 + i + 4));

			up64_1 = _mm256_cvtepi32_epi64(up128_1);
			low64_1 = _mm256_cvtepi32_epi64(low128_1);
			low_u_1 = _mm256_and_si256(low64_1, v_mask64);
			full_x_1 = _mm256_or_si256(_mm256_slli_epi64(up64_1, lower_bits), low_u_1);
			is_neg_1 = _mm256_cmpgt_epi64(_mm256_setzero_si256(), full_x_1);
			neg_full_1 = _mm256_sub_epi64(_mm256_setzero_si256(), full_x_1);
			uval_1 = _mm256_blendv_epi8(full_x_1, neg_full_1, is_neg_1);

			_mm256_storeu_si256((__m256i*)u_arr1, uval_1);
			v_dbl_1 = _mm256_set_pd((double)u_arr1[3], (double)u_arr1[2], (double)u_arr1[1], (double)u_arr1[0]);
			d_bits_1 = _mm256_castpd_si256(v_dbl_1);

			exp_raw_1 = _mm256_srli_epi64(d_bits_1, 52);
			orig_exp_1 = _mm256_add_epi64(exp_raw_1, v_exp_offset);
			exp_shifted_1 = _mm256_slli_epi64(orig_exp_1, 23);
			mant_shifted_1 = _mm256_and_si256(_mm256_srli_epi64(d_bits_1, 29), v_mant_mask);
			sign_shifted_1 = _mm256_and_si256(_mm256_srli_epi64(is_neg_1, 32), v_sign_mask_u);
			res64_1 = _mm256_or_si256(_mm256_or_si256(sign_shifted_1, exp_shifted_1), mant_shifted_1);

			res_lo1 = _mm256_castsi256_si128(res64_1);
			res_hi1 = _mm256_extracti128_si256(res64_1, 1);
			res32_1 = (__m128i)_mm_shuffle_ps((__m128)res_lo1, (__m128)res_hi1, _MM_SHUFFLE(2, 0, 2, 0));
			is_up_zero_1 = _mm_cmpeq_epi32(up128_1, _mm_setzero_si128());
			final32_1 = _mm_blendv_epi8(res32_1, low128_1, is_up_zero_1);
			_mm_storeu_si128((__m128i*)(dest + i + 4), final32_1);
		}

		for (; i + 3 < n; i += 4) {
			__m128i up128, low128, res_lo, res_hi, res32, is_up_zero, final32;
			__m256i up64_t, low64_t, low_u_t, full_x_t, is_neg_t, neg_full_t, uval_t;
		int64_t u_arr[4];
			__m256d v_dbl;
			__m256i d_bits, exp_raw, orig_exp, exp_shifted, mant_shifted, sign_shifted, res64;

			up128 = _mm_loadu_si128((const __m128i*)(up32 + i));
			low128 = _mm_loadu_si128((const __m128i*)(low32 + i));

			up64_t = _mm256_cvtepi32_epi64(up128);
			low64_t = _mm256_cvtepi32_epi64(low128);
			low_u_t = _mm256_and_si256(low64_t, v_mask64);
			full_x_t = _mm256_or_si256(_mm256_slli_epi64(up64_t, lower_bits), low_u_t);
			is_neg_t = _mm256_cmpgt_epi64(_mm256_setzero_si256(), full_x_t);
			neg_full_t = _mm256_sub_epi64(_mm256_setzero_si256(), full_x_t);
			uval_t = _mm256_blendv_epi8(full_x_t, neg_full_t, is_neg_t);

			_mm256_storeu_si256((__m256i*)u_arr, uval_t);
			v_dbl = _mm256_set_pd((double)u_arr[3], (double)u_arr[2], (double)u_arr[1], (double)u_arr[0]);
			d_bits = _mm256_castpd_si256(v_dbl);

			exp_raw = _mm256_srli_epi64(d_bits, 52);
			orig_exp = _mm256_add_epi64(exp_raw, v_exp_offset);
			exp_shifted = _mm256_slli_epi64(orig_exp, 23);
			mant_shifted = _mm256_and_si256(_mm256_srli_epi64(d_bits, 29), v_mant_mask);
			sign_shifted = _mm256_and_si256(_mm256_srli_epi64(is_neg_t, 32), v_sign_mask_u);
			res64 = _mm256_or_si256(_mm256_or_si256(sign_shifted, exp_shifted), mant_shifted);

			res_lo = _mm256_castsi256_si128(res64);
			res_hi = _mm256_extracti128_si256(res64, 1);
			res32 = (__m128i)_mm_shuffle_ps((__m128)res_lo, (__m128)res_hi, _MM_SHUFFLE(2, 0, 2, 0));
			is_up_zero = _mm_cmpeq_epi32(up128, _mm_setzero_si128());
			final32 = _mm_blendv_epi8(res32, low128, is_up_zero);
			_mm_storeu_si128((__m128i*)(dest + i), final32);
		}
	}

	/* Scalar fallback for leftovers or 64-bit signals */
	for (; i < n; i++) {
		FLAC__int64 up = upper_is_64 ? up64[i] : (FLAC__int64)up32[i];
		FLAC__int64 low_val = lower_is_64 ? low64[i] : (FLAC__int64)low32[i];
		uint64_t low_u = (uint64_t)low_val & mask;
		FLAC__int64 full_x;

		if (up == 0) {
			dest[i] = (uint32_t)low_val;
		} else {
			uint32_t sign;
			uint64_t uval;
			int leading_bit_pos, shift;
			uint32_t orig_exp, explicit_mant, orig_mant;
			full_x = (FLAC__int64)(((uint64_t)up << lower_bits) | low_u);
			sign = (full_x < 0) ? 1 : 0;
			uval = (full_x < 0) ? (uint64_t)(-full_x) : (uint64_t)full_x;
			leading_bit_pos = (int)FLAC__bitmath_ilog2_wide(uval);
			shift = leading_bit_pos - 23;
			orig_exp = (uint32_t)(base_exp + leading_bit_pos);
			explicit_mant = (shift >= 0) ? (uint32_t)(uval >> shift) : (uint32_t)(uval << (-shift));
			orig_mant = explicit_mant & 0x7FFFFF;
			dest[i] = (sign << 31) | (orig_exp << 23) | orig_mant;
		}
	}
	_mm256_zeroupper();
}
#endif
#endif
#endif
