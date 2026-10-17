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
 * AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
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
#include "private/bitmath.h"
#ifdef FLAC__SSE2_SUPPORTED

#include <emmintrin.h> /* SSE2 */

FLAC__SSE_TARGET("sse2")
void FLAC__transform_frame_fallback_float_intrin_sse2(FLAC__int32 *dest_signals[], const uint32_t * const raw_signals[], uint32_t channels, uint32_t blocksize)
{
	uint32_t ch, s;
	const __m128i v_0xff = _mm_set1_epi32(0xFF);
	const __m128i v_0x7fffff = _mm_set1_epi32(0x7FFFFF);
	const __m128i v_128 = _mm_set1_epi32(128);
	const __m128i v_neg_8388608 = _mm_set1_epi32(-8388608);
	const __m128i v_zero = _mm_setzero_si128();

	for (ch = 0; ch < channels; ch++) {
		FLAC__int32 *exp_out = dest_signals[2 * ch];
		FLAC__int32 *sign_mant_out = dest_signals[2 * ch + 1];
		const uint32_t *raw_ch = raw_signals[ch];

		for (s = 0; s + 3 < blocksize; s += 4) {
			__m128i v_raw = _mm_loadu_si128((const __m128i*)(const void*)(raw_ch + s));
			__m128i v_exp = _mm_sub_epi32(_mm_and_si128(_mm_srli_epi32(v_raw, 23), v_0xff), v_128);
			__m128i v_mant = _mm_and_si128(v_raw, v_0x7fffff);
			__m128i v_sign_mask = _mm_srai_epi32(v_raw, 31);
			__m128i v_sign_mant = _mm_sub_epi32(_mm_xor_si128(v_mant, v_sign_mask), v_sign_mask);
			__m128i v_is_zero_mant = _mm_cmpeq_epi32(v_mant, v_zero);
			__m128i v_is_neg_zero = _mm_and_si128(v_is_zero_mant, v_sign_mask);
			v_sign_mant = _mm_or_si128(_mm_andnot_si128(v_is_neg_zero, v_sign_mant), _mm_and_si128(v_is_neg_zero, v_neg_8388608));

			_mm_storeu_si128((__m128i*)(void*)(exp_out + s), v_exp);
			_mm_storeu_si128((__m128i*)(void*)(sign_mant_out + s), v_sign_mant);
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

FLAC__SSE_TARGET("sse2")
void FLAC__combine_subframe_signals_fallback_to_f32_buffer_intrin_sse2(uint32_t *dest, const FLAC__int32 *exp_signal, const FLAC__int32 *sign_mant_signal, size_t n)
{
	size_t i;
	const __m128i v_0xff = _mm_set1_epi32(0xFF);
	const __m128i v_0x7fffff = _mm_set1_epi32(0x7FFFFF);
	const __m128i v_sign_bit = _mm_set1_epi32((int)0x80000000);
	const __m128i v_128 = _mm_set1_epi32(128);
	const __m128i v_neg_8388608 = _mm_set1_epi32(-8388608);

	for (i = 0; i + 3 < n; i += 4) {
		__m128i v_exp = _mm_and_si128(_mm_add_epi32(_mm_loadu_si128((const __m128i*)(const void*)(exp_signal + i)), v_128), v_0xff);
		__m128i v_sm = _mm_loadu_si128((const __m128i*)(const void*)(sign_mant_signal + i));
		__m128i v_sign_mask = _mm_srai_epi32(v_sm, 31);
		__m128i v_abs_sm = _mm_sub_epi32(_mm_xor_si128(v_sm, v_sign_mask), v_sign_mask);
		__m128i v_mant = _mm_and_si128(v_abs_sm, v_0x7fffff);
		__m128i v_is_special = _mm_cmpeq_epi32(v_sm, v_neg_8388608);
		__m128i v_sign = _mm_and_si128(v_sign_mask, v_sign_bit);
		__m128i v_dest;
		v_mant = _mm_andnot_si128(v_is_special, v_mant);
		v_dest = _mm_or_si128(_mm_or_si128(v_sign, _mm_slli_epi32(v_exp, 23)), v_mant);

		_mm_storeu_si128((__m128i*)(void*)(dest + i), v_dest);
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

FLAC__SSE_TARGET("sse2")
void FLAC__combine_subframe_signals_dpa_to_f32_buffer_intrin_sse2(uint32_t *dest, const void *upper_signal, FLAC__bool upper_is_64, const void *lower_signal, FLAC__bool lower_is_64, uint32_t lower_bits, size_t n, FLAC__byte highest_exp, uint32_t upper_bits)
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
			int64_t up0 = up32[i], up1 = up32[i+1];
			int64_t low0 = low32[i], low1 = low32[i+1];

			__m128i v_up64 = _mm_set_epi64x(up1, up0);
			__m128i v_low64 = _mm_set_epi64x(low1, low0);
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

			uint32_t r0 = (uint32_t)_mm_cvtsi128_si64(res64);
			uint32_t r1 = (uint32_t)_mm_cvtsi128_si64(_mm_unpackhi_epi64(res64, res64));

			dest[i]   = (up0 == 0) ? (uint32_t)low0 : r0;
			dest[i+1] = (up1 == 0) ? (uint32_t)low1 : r1;
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
