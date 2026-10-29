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
#include "private/transform_float.h"
#include "private/macros.h"
#include "private/bitmath.h"
#ifdef FLAC__SSE4_1_SUPPORTED

#include <smmintrin.h> /* SSE4.1 */

FLAC__SSE_TARGET("sse4.1")
static inline int hmax_sse41(__m128i v) {
	__m128i m1 = _mm_max_epi32(v, _mm_shuffle_epi32(v, _MM_SHUFFLE(1, 0, 3, 2)));
	__m128i m2 = _mm_max_epi32(m1, _mm_shuffle_epi32(m1, _MM_SHUFFLE(2, 3, 0, 1)));
	return _mm_cvtsi128_si32(m2);
}

FLAC__SSE_TARGET("sse4.1")
static inline int hmin_sse41(__m128i v) {
	__m128i m1 = _mm_min_epi32(v, _mm_shuffle_epi32(v, _MM_SHUFFLE(1, 0, 3, 2)));
	__m128i m2 = _mm_min_epi32(m1, _mm_shuffle_epi32(m1, _MM_SHUFFLE(2, 3, 0, 1)));
	return _mm_cvtsi128_si32(m2);
}

FLAC__SSE_TARGET("sse4.1")
void FLAC__compute_channel_base_exp_float_intrin_sse41(const uint32_t * const raw_signals[], uint32_t channels, uint32_t blocksize, uint32_t channel_highest_exp[], uint32_t channel_lowest_exp[])
{
	uint32_t ch;
	int ch_highest[FLAC__MAX_CHANNELS];
	int ch_lowest[FLAC__MAX_CHANNELS];

	const __m128i v_0xff = _mm_set1_epi32(0xFF);
	const __m128i v_0x7fffff = _mm_set1_epi32(0x7FFFFF);
	const __m128i v_0x7fffffff = _mm_set1_epi32(0x7FFFFFFF);
	const __m128i v_implicit_bit = _mm_set1_epi32(1 << 23);
	const __m128i v_zero = _mm_setzero_si128();
	const __m128i v_127 = _mm_set1_epi32(127);
	const __m128i v_23 = _mm_set1_epi32(23);
	const __m128i v_neg1 = _mm_set1_epi32(-1);
	const __m128i v_1000 = _mm_set1_epi32(1000);

	for (ch = 0; ch < channels; ch++) {
		int highest_pos = -1;
		int lowest_pos = 1000;
		uint32_t s = 0;
		const uint32_t *raw_ch = raw_signals[ch];

		__m128i v_max = _mm_set1_epi32(-1);
		__m128i v_min = _mm_set1_epi32(1000);

		for (; s + 3 < blocksize; s += 4) {
			__m128i v_raw = _mm_loadu_si128((const __m128i*)(const void*)(raw_ch + s));
			__m128i v_exp = _mm_and_si128(_mm_srli_epi32(v_raw, 23), v_0xff);
			__m128i v_mant = _mm_and_si128(v_raw, v_0x7fffff);
			__m128i v_is_zero = _mm_cmpeq_epi32(_mm_and_si128(v_raw, v_0x7fffffff), v_zero);
			__m128i v_explicit_mant = _mm_or_si128(v_mant, v_implicit_bit);
			__m128i v_lsb = _mm_and_si128(v_explicit_mant, _mm_sub_epi32(v_zero, v_explicit_mant));
			__m128 v_flt = _mm_cvtepi32_ps(v_lsb);
			__m128i v_tz = _mm_sub_epi32(_mm_srli_epi32(_mm_castps_si128(v_flt), 23), v_127);
			__m128i v_pos_high = v_exp;
			__m128i v_pos_low = _mm_add_epi32(_mm_sub_epi32(v_exp, v_23), v_tz);

			v_pos_high = _mm_blendv_epi8(v_pos_high, v_neg1, v_is_zero);
			v_pos_low  = _mm_blendv_epi8(v_pos_low,  v_1000, v_is_zero);

			v_max = _mm_max_epi32(v_max, v_pos_high);
			v_min = _mm_min_epi32(v_min, v_pos_low);
		}

		highest_pos = hmax_sse41(v_max);
		lowest_pos = hmin_sse41(v_min);

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

FLAC__SSE_TARGET("sse4.1")
void FLAC__combine_subframe_signals_dpa_to_f32_buffer_intrin_sse41(uint32_t *dest, const void *upper_signal, FLAC__bool upper_is_64, const void *lower_signal, FLAC__bool lower_is_64, uint32_t lower_bits, size_t n, FLAC__byte highest_exp, uint32_t upper_bits)
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
		const __m128i v_mask64 = _mm_set1_epi64x((int64_t)mask);
		const int64_t exp_offset = (int64_t)(base_exp - 1023);
		const __m128i v_exp_offset = _mm_set1_epi64x(exp_offset);
		const __m128i v_mant_mask = _mm_set1_epi64x(0x7FFFFF);

		for (; i + 1 < n; i += 2) {
			__m128i up_2 = _mm_loadl_epi64((const __m128i*)(up32 + i));
			__m128i low_2 = _mm_loadl_epi64((const __m128i*)(low32 + i));

			__m128i v_up64 = _mm_cvtepi32_epi64(up_2);
			__m128i v_low64 = _mm_cvtepu32_epi64(low_2);
			__m128i low_u = _mm_and_si128(v_low64, v_mask64);

			__m128i full_x = _mm_or_si128(_mm_slli_epi64(v_up64, lower_bits), low_u);
			int64_t f0 = _mm_cvtsi128_si64(full_x);
			int64_t f1 = _mm_cvtsi128_si64(_mm_unpackhi_epi64(full_x, full_x));

			int64_t u0 = (f0 < 0) ? -f0 : f0;
			int64_t u1 = (f1 < 0) ? -f1 : f1;

			__m128d v_dbl = _mm_set_pd((double)u1, (double)u0);
			__m128i d_bits = _mm_castpd_si128(v_dbl);

			__m128i exp_raw = _mm_srli_epi64(d_bits, 52);
			__m128i orig_exp = _mm_add_epi64(exp_raw, v_exp_offset);
			__m128i exp_shifted = _mm_slli_epi64(orig_exp, 23);
			__m128i mant_shifted = _mm_and_si128(_mm_srli_epi64(d_bits, 29), v_mant_mask);
			uint64_t s0 = (f0 < 0) ? 0x80000000u : 0;
			uint64_t s1 = (f1 < 0) ? 0x80000000u : 0;
			__m128i sign_shifted = _mm_set_epi64x(s1, s0);

			__m128i res64 = _mm_or_si128(_mm_or_si128(sign_shifted, exp_shifted), mant_shifted);

			__m128 shuf = _mm_shuffle_ps((__m128)res64, (__m128)res64, _MM_SHUFFLE(2, 0, 2, 0));
			__m128i res32 = (__m128i)shuf;

			__m128i is_up_zero = _mm_cmpeq_epi32(up_2, _mm_setzero_si128());
			__m128i final32 = _mm_blendv_epi8(res32, low_2, is_up_zero);

			_mm_storel_epi64((__m128i*)(dest + i), final32);
		}
	}

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
}

#endif
#endif
#endif
