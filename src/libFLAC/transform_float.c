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

/* Bit manipulation utilities used for conversion between IEEE 754 32-bit float samples
 * and compressible integer samples (up to 63-bit)
 */

/*
To compress float samples, one must understand how they are generated in order to
design an algorithm that is tailored to exploit the specific signal structure.

There are some audio recording equipment coming to the market that can record 32-bit floats.
They utilise two (or rarely more) ADCs working in tandem to create a single audio file.
A low-gain ADC is optimised for loud sections, while the other high-gain ADC is optimised for quieter ones.
The recorder switches between the two on the fly.

With that in mind, we see that although these recordings are floating-point,
their dynamic range does not merely reach the DR of 32-bit floats (between 2^-126 and 2^127),
especially considering that these loud and quiet sections are long enough to be split into
different FLAC frames, each having a normal int-like DR.
Moreover, the majority of the post-processing and audio editing in the music and film industry does not
increase the dynamic range too much. On the contrary, most audio engineers try to decrease the DR.

So basically, we're mostly dealing with a combination of ints,
and a single coefficient, converting them to floats.

Possible design choices are listed below. The further we go, the higher the level of compression:
   1. just leaving the floats as ints
		probably would lead to saving most of them in verbatim frames with no compression
		my tests resulted in an ~80% ratio
   2. doing some basic bit manipulation
		saves ~3 bits theoretically
		my tests resulted in about ~70% ratio
   3. splitting the "exponent" (8 bit) and "sign+significand" (24 bit) parts of floats into two subframes.
		my tests resulted in about ~68% ratio, even with no bit manipulation
   4. splitting + some bit manipulation + subtracting an (automatically recognised) DC offset from the
		"exponents" channel and storing the offset in each frame header (current approach).
		my tests resulted in about ~45% ratio
*/

/*
Binary Positional Notation Transformation (1-subframe mode & DPA mode):
   In binary positional notation, each normalized float sample (+/- 1.mant * 2^(exp - 127)) has:
     - highest active bit position = exp
     - lowest active bit position = exp - 23 + ctz(explicit_mant)
   Across a frame of samples:
     - highest_pos = max(exp_i)
     - lowest_pos = min(exp_i - 23 + ctz(explicit_mant_i))
   The exact minimum integer bit depth needed to losslessly represent all active bits is:
     bps = highest_pos - lowest_pos + 2
   Setting base_exp = lowest_pos, each sample's explicit mantissa is shifted by:
     shift = (exp_i - 23) - lowest_pos >= 0
   into an exact two's complement signed integer of width up to 63 bits without losing any bit.
   When bps <= 63 (and no special NaN/Inf/-0.0f conditions exist), the frame is encoded
   losslessly in Dynamic Positional Alignment (DPA) mode (2 subframes per channel):
     - Delta subframe (8 bps): sample shifts Delta_i = max(0, (exp_i - 23) - lowest_pos)
     - Audio subframe (up to 63 bps): aligned integer x_i with normalized residuals Q_i = r_i >> Delta_i
   The per-channel highest exponent (highest_exp) is stored in the PACKED subframe (in exponent_zero_offsets).

Frame Mode Selection & Fallback Two's Complement Conversion:
   - Mode flag: A 1-bit fallback flag is stored in one of the reserved bits of the frame header
     extension (0 = lossless DPA mode at dynamic bps <= 63, 1 = fallback 2-subframe mode).
   - Fallback mode: When a frame has bps > 63 or contains special float conditions (NaN, Inf,
     subnormals, or -0.0f), it falls back to 2 subframes per channel (exponent + significand):
       * Exponent channel (8 bits): stored with a -128 DC offset ([-128, 127]).
       * Significand channel (24 bits): converted from sign-magnitude to two's complement,
         mapping positive samples to +mantissa, negative samples to -mantissa, and -0.0f
         to -2^23 (-8388608).
*/

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <stddef.h>
#include <stdint.h>
#include "private/macros.h"
#include "private/transform_float.h"
#include "FLAC/assert.h"
#include "private/bitmath.h"
#include "private/md5.h"
#include "share/alloc.h"
#include "share/endswap.h"

#if defined(_MSC_VER)
#  include <intrin.h>
static inline int flac_ctz32(uint32_t v) {
	unsigned long idx = 0;
	if (_BitScanForward(&idx, v))
		return (int)idx;
	return 0;
}
#else
static inline int flac_ctz32(uint32_t v) {
	return v ? (int)__builtin_ctz((unsigned int)v) : 0;
}
#endif


/*
 * Computes base exponent per channel for 1-subframe lossless float encoding (63 bps).
 */
void FLAC__compute_channel_base_exp_float(const uint32_t * const raw_signals[], uint32_t channels, uint32_t blocksize, uint32_t channel_highest_exp[], uint32_t channel_lowest_exp[])
{
	uint32_t ch;
	int ch_highest[FLAC__MAX_CHANNELS];
	int ch_lowest[FLAC__MAX_CHANNELS];
	for (ch = 0; ch < channels; ch++) {
		int highest_pos = -1;
		int lowest_pos = 1000;
		uint32_t s;
		for (s = 0; s < blocksize; s++) {
			uint32_t raw = raw_signals[ch][s];
			uint32_t exp = (raw >> 23) & 0xFF;
			uint32_t mant = raw & 0x7FFFFF;
			if (exp != 0 || mant != 0) {
				uint32_t explicit_mant = (1u << 23) | mant;
				int tz = flac_ctz32(explicit_mant);
				int pos_high = (int)exp;
				int pos_low = (int)exp - 23 + tz;
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

/*
 * Recombines 2 integer subframe signals (raw 8-bit exponent and 24-bit two's complement significand)
 * into standard 32-bit IEEE 754 floating-point raw words.
 */
void FLAC__combine_subframe_signals_fallback_to_f32_buffer(uint32_t *dest, const FLAC__int32 *exp_signal, const FLAC__int32 *sign_mant_signal, size_t n)
{
	size_t i;
	for (i = 0; i < n; i++) {
		const uint32_t orig_exp = (uint32_t)(exp_signal[i] + 128) & 0xFF;
		const FLAC__int32 sm = sign_mant_signal[i];
		uint32_t sign, orig_mant;
		if (sm == -8388608) { /* -2^23: preserves sign=1 when mant=0 (e.g., -0.0f, -1.0*2^k, -inf) */
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

/*
 * Recombines 2-way DPA integer subframes (Upper, Lower) into standard 32-bit IEEE 754
 * floating-point raw words.
 */
void FLAC__combine_subframe_signals_dpa_to_f32_buffer(uint32_t *dest, const void *upper_signal, FLAC__bool upper_is_64, const void *lower_signal, FLAC__bool lower_is_64, uint32_t shift_split, size_t n, FLAC__byte highest_exp, uint32_t total_width, uint32_t split_point)
{
	size_t i;
	const uint64_t mask = (shift_split >= 64) ? (~(uint64_t)0) : ((shift_split > 0) ? (((uint64_t)1 << shift_split) - 1) : 0);
	const FLAC__int32 *up32 = upper_is_64 ? NULL : (const FLAC__int32 *)upper_signal;
	const FLAC__int64 *up64 = upper_is_64 ? (const FLAC__int64 *)upper_signal : NULL;
	const FLAC__int32 *low32 = lower_is_64 ? NULL : (const FLAC__int32 *)lower_signal;
	const FLAC__int64 *low64 = lower_is_64 ? (const FLAC__int64 *)lower_signal : NULL;
	const uint32_t eff_width = (total_width > 0) ? total_width : split_point;
	const int base_exp = (int)highest_exp - (int)(eff_width - 2);

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

FLAC__bool FLAC__check_frame_lossless_float(const uint32_t * const raw_signals[], uint32_t channels, uint32_t blocksize, uint32_t channel_highest_exp[], uint32_t channel_lowest_exp[])
{
	uint32_t ch;
	int ch_highest[FLAC__MAX_CHANNELS];
	int ch_lowest[FLAC__MAX_CHANNELS];
	for (ch = 0; ch < channels; ch++) {
		int highest_pos = -1;
		int lowest_pos = 1000;
		uint32_t s;
		for (s = 0; s < blocksize; s++) {
			uint32_t raw = raw_signals[ch][s];
			uint32_t exp = (raw >> 23) & 0xFF;
			uint32_t sign = (raw >> 31) & 1;
			uint32_t mant = raw & 0x7FFFFF;
			if (exp == 0 && mant == 0) {
				if (sign != 0) {
					/* -0.0f cannot be distinguished from +0.0f in two's complement 0 */
					return false;
				}
			} else if (exp == 0 || exp == 255) {
				/* Subnormal numbers, Infinity, and NaN require 2-subframe fallback mode */
				return false;
			} else {
				uint32_t explicit_mant = (1u << 23) | mant;
				int tz = flac_ctz32(explicit_mant);
				int pos_high = (int)exp;
				int pos_low = (int)exp - 23 + tz;
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

/*
 * Transforms signals in-place for Fallback 2-subframe mode:
 *  - signals[2*ch]: 8-bit exponent with -128 DC offset ([-128, 127])
 *  - signals[2*ch+1]: 24-bit significand in two's complement (-0.0f -> -2^23)
 */
void FLAC__transform_frame_fallback_float(FLAC__int32 *dest_signals[], const uint32_t * const raw_signals[], uint32_t channels, uint32_t blocksize)
{
	uint32_t ch;
	for (ch = 0; ch < channels; ch++) {
		uint32_t s;
		for (s = 0; s < blocksize; s++) {
			uint32_t raw = raw_signals[ch][s];
			uint32_t exp = (raw >> 23) & 0xFF;
			uint32_t sign = (raw >> 31) & 1;
			uint32_t mant = raw & 0x7FFFFF;
			FLAC__int32 sm;
			if (sign && mant == 0) {
				sm = -8388608; /* -2^23: preserves sign=1 when mant=0 (e.g., -0.0f, -1.0*2^k, -inf) */
			} else if (sign) {
				sm = -(FLAC__int32)mant;
			} else {
				sm = (FLAC__int32)mant;
			}
			dest_signals[2 * ch][s] = (FLAC__int32)exp - 128;
			dest_signals[2 * ch + 1][s] = sm;
		}
	}
}

/*
 * Transforms signals into 2-way DPA mode:
 *  - dest_signals32[2*ch + 0]: Upper stream x_upper[i] = x[i] >> shift_split
 *  - dest_signals32[2*ch + 1]: Lower stream x_lower[i] (32-bit if shift_split <= 32)
 *  - dest_signals64[2*ch + 1]: Lower stream x_lower[i] (64-bit)
 */
void FLAC__transform_frame_dpa_float(FLAC__int32 *dest_signals32[], FLAC__int64 *dest_signals64[], const uint32_t * const raw_signals[], const uint32_t channel_highest_exp[], const uint32_t channel_lowest_exp[], FLAC__byte exponent_zero_offsets[], FLAC__byte shift_split_out[], uint32_t channel_bps_out[], uint32_t *split_point_out, uint32_t compression_level, uint32_t channels, uint32_t blocksize)
{
	uint32_t ch, s;
	uint32_t channel_bps[FLAC__MAX_CHANNELS];
	uint32_t split_point = 24;
	const char *env_sp;

	for (ch = 0; ch < channels; ch++) {
		FLAC__int64 *lower_out64 = dest_signals64[2 * ch + 1];
		uint32_t highest_exp = channel_highest_exp[ch];
		uint32_t lowest_exp = channel_lowest_exp[ch];
		uint32_t bps = (highest_exp >= lowest_exp) ? (highest_exp - lowest_exp + 2) : 1;
		if (bps > 63)
			bps = 63;

		/* First pass: compute full positionally aligned 64-bit integer into lower_out64 */
		for (s = 0; s < blocksize; s++) {
			uint32_t raw = raw_signals[ch][s];
			uint32_t exp = (raw >> 23) & 0xFF;
			uint32_t sign = (raw >> 31) & 1;
			uint32_t mant = raw & 0x7FFFFF;
			if (exp == 0 && mant == 0) {
				lower_out64[s] = 0;
			} else {
				int shift = ((int)exp - 23) - (int)lowest_exp;
				uint64_t explicit_mant = (1ULL << 23) | (uint64_t)mant;
				uint64_t val = (shift >= 0) ? (explicit_mant << shift) : (explicit_mant >> (-shift));
				FLAC__int64 signed_val = sign ? -(FLAC__int64)val : (FLAC__int64)val;
				lower_out64[s] = signed_val;
			}
		}

		channel_bps[ch] = bps;
		exponent_zero_offsets[ch] = (FLAC__byte)highest_exp;
	}

	/* Split point selection */
	env_sp = getenv("FLAC_DPA_SPLIT_POINT");
	if (env_sp) {
		int v = atoi(env_sp);
		if (v >= 8 && v <= 64)
			split_point = (uint32_t)v;
	} else if (compression_level == 8) {
		static const uint32_t candidates[5] = { 16, 24, 32, 48, 64 };
		uint64_t best_total_bits = (uint64_t)-1;
		uint32_t cand_idx;

		for (cand_idx = 0; cand_idx < 5; cand_idx++) {
			uint32_t cand_sp = candidates[cand_idx];
			uint64_t cand_total_bits = 0;

			for (ch = 0; ch < channels; ch++) {
				uint32_t ch_bps = channel_bps[ch];
				uint32_t shift = (ch_bps > cand_sp) ? (ch_bps - cand_sp) : 0;
				const FLAC__int64 *low64 = dest_signals64[2 * ch + 1];
				uint64_t lower_bits = 0;
				uint64_t sum_abs_r = 0;
				FLAC__int64 prev_up = 0;

				for (s = 0; s < blocksize; s++) {
					FLAC__int64 signed_val = low64[s];
					FLAC__int64 up = (shift > 0) ? (signed_val >> shift) : signed_val;
					uint32_t d;
					uint32_t width;

					if (up == 0 || up == -1) {
						d = 0;
					} else if (up > 0) {
						uint32_t lead_u = FLAC__bitmath_ilog2_wide((FLAC__uint64)up);
						uint32_t lead_full = lead_u + shift;
						d = (lead_full > 23) ? (lead_full - 23) : 0;
					} else {
						uint32_t lead_u = FLAC__bitmath_ilog2_wide((FLAC__uint64)(~up));
						uint32_t lead_full = lead_u + shift;
						d = (lead_full > 23) ? (lead_full - 23) : 0;
					}
					width = (shift > d) ? (shift - d) : 0;
					lower_bits += width;

					if (s > 0) {
						FLAC__int64 diff = up - prev_up;
						sum_abs_r += (diff < 0) ? (uint64_t)(-diff) : (uint64_t)diff;
					}
					prev_up = up;
				}

				if (blocksize > 1 && sum_abs_r > 0) {
					uint64_t mean_abs = sum_abs_r / (blocksize - 1);
					uint32_t k = (mean_abs > 0) ? FLAC__bitmath_ilog2_wide((mean_abs * 7) / 10) : 0;
					uint64_t est_upper_bits = (sum_abs_r >> k) + (uint64_t)(blocksize - 1) * (k + 1);
					cand_total_bits += lower_bits + est_upper_bits;
				} else {
					cand_total_bits += lower_bits + (blocksize * 2);
				}
			}

			if (cand_total_bits < best_total_bits) {
				best_total_bits = cand_total_bits;
				split_point = cand_sp;
			}
		}
	} else {
		split_point = 24;
	}

	*split_point_out = split_point;

	/* Second pass: split into Upper and Lower */
	for (ch = 0; ch < channels; ch++) {
		FLAC__int32 *upper_out32 = dest_signals32[2 * ch];
		FLAC__int64 *upper_out64 = dest_signals64 ? dest_signals64[2 * ch] : NULL;
		FLAC__int32 *lower_out32 = dest_signals32[2 * ch + 1];
		FLAC__int64 *lower_out64 = dest_signals64[2 * ch + 1];
		uint32_t ch_bps = channel_bps[ch];
		uint32_t shift_split = (ch_bps > split_point) ? (ch_bps - split_point) : 0;
		uint64_t mask = (shift_split >= 64) ? (~(uint64_t)0) : ((shift_split > 0) ? (((uint64_t)1 << shift_split) - 1) : 0);

		for (s = 0; s < blocksize; s++) {
			FLAC__int64 signed_val = lower_out64[s];
			if (shift_split == 0) {
				if (upper_out32) upper_out32[s] = (FLAC__int32)signed_val;
				if (upper_out64) upper_out64[s] = signed_val;
				lower_out32[s] = 0;
				lower_out64[s] = 0;
			} else {
				FLAC__int64 up = signed_val >> shift_split;
				uint64_t low_u = (uint64_t)signed_val & mask;
				if (upper_out32) upper_out32[s] = (FLAC__int32)up;
				if (upper_out64) upper_out64[s] = up;
				lower_out32[s] = (FLAC__int32)low_u;
				lower_out64[s] = (FLAC__int64)low_u;
			}
		}
		shift_split_out[ch] = (FLAC__byte)shift_split;
		if (channel_bps_out)
			channel_bps_out[ch] = ch_bps;
	}
}


FLAC__bool FLAC__MD5Accumulate_float(FLAC__MD5Context *ctx, const uint32_t * const float_signals[], uint32_t channels, uint32_t samples)
{
	size_t bytes_needed = 0;
	uint32_t sample, channel;
	FLAC__byte *buf;

	if (channels > 1024)
		return false;

	bytes_needed = (size_t)samples * (size_t)channels * sizeof(uint32_t);
	if (bytes_needed == 0)
		return true;

	buf = safe_malloc_(bytes_needed);
	if (!buf)
		return false;

	{
		uint32_t *dest = (uint32_t *)buf;
		size_t k = 0;
		for (sample = 0; sample < samples; sample++) {
			for (channel = 0; channel < channels; channel++) {
				dest[k++] = H2LE_32(float_signals[channel][sample]);
			}
		}
	}

	FLAC__MD5Update(ctx, buf, (uint32_t)bytes_needed);
	free(buf);
	return true;
}


