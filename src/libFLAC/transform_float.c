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
 * and compressible 32-bit integer samples
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
		"exponents" channel and	storing the offset in each frame header (current approach).
		my tests resulted in about ~45% ratio
*/

/*
Dynamic Bit-Depth Binary Positional Notation Transformation (1-subframe mode):
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
   into an exact two's complement signed integer of width bps without losing any bit.
   When bps <= 32 (and no special NaN/Inf/-0.0f conditions exist), the frame is encoded
   losslessly in 1 subframe per channel at the dynamically computed bps.
   The per-channel base exponent (base_exp) is stored in the frame header (in exponent_zero_offsets).

Frame Mode Selection & Fallback Two's Complement Conversion:
   - Mode flag: A 1-bit fallback flag is stored in one of the reserved bits of the frame header
     extension (0 = lossless 1-subframe mode with dynamic bps, 1 = fallback 2-subframe mode).
   - Fallback mode: When a frame has bps > 32 or contains special float conditions (NaN, Inf,
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
#include "private/transform_float.h"
#include "private/md5.h"
#include "share/alloc.h"

/*
 * Splits 32-bit IEEE 754 floating-point samples into two integer subframe signals:
 *  - Exponent (8 bits: raw exponent in bits 23-30)
 *  - Sign + Significand (24 bits: sign at bit 23, significand fraction at bits 0-22)
 */
void FLAC__split_f32_buffer_to_subframe_signals(FLAC__int32 *exp_signal, FLAC__int32 *sign_mant_signal, const uint32_t *src, size_t n)
{
	size_t i;
	for (i = 0; i < n; i++) {
		const uint32_t x = src[i];
		const uint32_t exp = (x >> 23) & 0xFF;
		const uint32_t sign = (x >> 31) & 1;
		const uint32_t mant = x & 0x7FFFFF;
		const uint32_t sign_mant = (sign << 23) | mant;
		exp_signal[i] = (FLAC__int32)exp;
		sign_mant_signal[i] = (FLAC__int32)(sign_mant << 8) >> 8;
	}
}

/*
 * Splits interleaved 32-bit IEEE 754 floating-point samples into two integer subframe signals per channel:
 *  - Exponent (8 bits: raw exponent in bits 23-30) -> integer_signals[2 * ch]
 *  - Sign + Significand (24 bits: sign at bit 23, significand fraction at bits 0-22) -> integer_signals[2 * ch + 1]
 */
void FLAC__split_f32_interleaved_to_subframe_signals(FLAC__int32 * const integer_signals[], uint32_t channels, uint32_t current_sample_number, const uint32_t *src, size_t n)
{
	size_t s, k = 0;
	uint32_t ch;
	for (s = 0; s < n; s++) {
		for (ch = 0; ch < channels; ch++) {
			const uint32_t x = src[k++];
			const uint32_t exp = (x >> 23) & 0xFF;
			const uint32_t sign = (x >> 31) & 1;
			const uint32_t mant = x & 0x7FFFFF;
			const uint32_t sign_mant = (sign << 23) | mant;
			integer_signals[2 * ch][current_sample_number + s] = (FLAC__int32)exp;
			integer_signals[2 * ch + 1][current_sample_number + s] = (FLAC__int32)(sign_mant << 8) >> 8;
		}
	}
}

/*
 * Computes base exponent and required bit depth per channel for 1-subframe lossless float encoding.
 */
void FLAC__compute_channel_base_exp_float(const FLAC__int32 * const split_signals[], uint32_t channels, uint32_t blocksize, uint32_t channel_base_exp[], uint32_t *frame_bps)
{
	uint32_t ch;
	uint32_t max_frame_bps = 0;
	for (ch = 0; ch < channels; ch++) {
		int highest_pos = -1;
		int lowest_pos = 1000;
		uint32_t s;
		for (s = 0; s < blocksize; s++) {
			uint32_t exp = (uint32_t)split_signals[2 * ch][s] & 0xFF;
			uint32_t sign_mant = (uint32_t)split_signals[2 * ch + 1][s] & 0xFFFFFF;
			if (exp != 0 || (sign_mant & 0x7FFFFF) != 0) {
				uint32_t explicit_mant = (1u << 23) | (sign_mant & 0x7FFFFF);
				int tz = __builtin_ctz(explicit_mant);
				int pos_high = (int)exp;
				int pos_low = (int)exp - 23 + tz;
				if (pos_high > highest_pos)
					highest_pos = pos_high;
				if (pos_low < lowest_pos)
					lowest_pos = pos_low;
			}
		}
		if (highest_pos == -1) {
			channel_base_exp[ch] = 127;
		} else {
			channel_base_exp[ch] = (uint32_t)lowest_pos;
			{
				uint32_t ch_bps = (uint32_t)(highest_pos - lowest_pos + 2);
				if (ch_bps > max_frame_bps)
					max_frame_bps = ch_bps;
			}
		}
	}
	if (max_frame_bps < 4)
		max_frame_bps = 4;
	if (frame_bps != NULL)
		*frame_bps = max_frame_bps;
}

/*
 * Recombines integer subframe signal(s) into standard 32-bit IEEE 754 floating-point raw words:
 *  - When is_fallback == true: 2 subframes per channel (raw 8-bit exponent and 24-bit two's complement significand)
 *  - When is_fallback == false: 1 subframe per channel (preprocessed two's complement integer reconstructed via base_exp = lowest_pos)
 */
void FLAC__combine_subframe_signals_to_f32_buffer(uint32_t *dest, const FLAC__int32 *subframe0, const FLAC__int32 *subframe1, size_t n, FLAC__byte base_exp, FLAC__bool is_fallback)
{
	size_t i;
	if (is_fallback) {
		const FLAC__int32 *exp_signal = subframe0;
		const FLAC__int32 *sign_mant_signal = subframe1;
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
	} else {
		/* Single dynamic-bps preprocessed significand+sign subframe */
		const FLAC__int32 *sig_signal = subframe0;
		for (i = 0; i < n; i++) {
			FLAC__int32 val = sig_signal[i];
			if (val == 0) {
				dest[i] = 0;
			} else {
				uint32_t sign = (val < 0) ? 1 : 0;
				uint32_t uval = (val < 0) ? (uint32_t)(-val) : (uint32_t)val;
				int leading_bit_pos = 31 - __builtin_clz(uval);
				int shift = leading_bit_pos - 23;
				uint32_t orig_exp = (uint32_t)base_exp + (uint32_t)leading_bit_pos;
				uint32_t explicit_mant = (shift >= 0) ? (uval >> shift) : (uval << (-shift));
				uint32_t orig_mant = explicit_mant & 0x7FFFFF;
				dest[i] = (sign << 31) | (orig_exp << 23) | orig_mant;
			}
		}
	}
}

/*
 * Analyzes split float subframes across all channels to verify if the entire frame can be
 * losslessly represented using 1 subframe per channel (dynamic integer bit depth <= 32).
 */
FLAC__bool FLAC__check_frame_lossless_float(const FLAC__int32 * const split_signals[], uint32_t channels, uint32_t blocksize, uint32_t channel_base_exp[], uint32_t *frame_bps)
{
	uint32_t ch;
	uint32_t max_frame_bps = 0;
	for (ch = 0; ch < channels; ch++) {
		int highest_pos = -1;
		int lowest_pos = 1000;
		uint32_t s;
		for (s = 0; s < blocksize; s++) {
			uint32_t exp = (uint32_t)split_signals[2 * ch][s] & 0xFF;
			uint32_t sign_mant = (uint32_t)split_signals[2 * ch + 1][s] & 0xFFFFFF;
			if (exp == 0 && (sign_mant & 0x7FFFFF) == 0) {
				if ((sign_mant >> 23) != 0) {
					/* -0.0f cannot be distinguished from +0.0f in two's complement 0 */
					return false;
				}
			} else if (exp == 0 || exp == 255) {
				/* Subnormal numbers, Infinity, and NaN require 2-subframe fallback mode */
				return false;
			} else {
				uint32_t explicit_mant = (1u << 23) | (sign_mant & 0x7FFFFF);
				int tz = __builtin_ctz(explicit_mant);
				int pos_high = (int)exp;
				int pos_low = (int)exp - 23 + tz;
				if (pos_high > highest_pos)
					highest_pos = pos_high;
				if (pos_low < lowest_pos)
					lowest_pos = pos_low;
			}
		}
		if (highest_pos == -1) {
			channel_base_exp[ch] = 127;
		} else {
			uint32_t ch_bps;
			if (lowest_pos < 0 || highest_pos > 254)
				return false;
			ch_bps = (uint32_t)(highest_pos - lowest_pos + 2);
			if (ch_bps > 32)
				return false;
			channel_base_exp[ch] = (uint32_t)lowest_pos;
			if (ch_bps > max_frame_bps)
				max_frame_bps = ch_bps;
		}
	}
	if (max_frame_bps < 4)
		max_frame_bps = 4;
	if (frame_bps != NULL)
		*frame_bps = max_frame_bps;
	return true;
}

/*
 * Transforms signals in-place for Fallback 2-subframe mode:
 *  - signals[2*ch]: 8-bit exponent with -128 DC offset ([-128, 127])
 *  - signals[2*ch+1]: 24-bit significand in two's complement (-0.0f -> -2^23)
 */
void FLAC__transform_frame_fallback_float(FLAC__int32 *signals[], uint32_t channels, uint32_t blocksize)
{
	uint32_t ch;
	for (ch = 0; ch < channels; ch++) {
		uint32_t s;
		for (s = 0; s < blocksize; s++) {
			uint32_t exp = (uint32_t)signals[2 * ch][s] & 0xFF;
			uint32_t sign_mant = (uint32_t)signals[2 * ch + 1][s] & 0xFFFFFF;
			uint32_t sign = (sign_mant >> 23) & 1;
			uint32_t mant = sign_mant & 0x7FFFFF;
			FLAC__int32 sm;
			if (sign && mant == 0) {
				sm = -8388608; /* -2^23: preserves sign=1 when mant=0 (e.g., -0.0f, -1.0*2^k, -inf) */
			} else if (sign) {
				sm = -(FLAC__int32)mant;
			} else {
				sm = (FLAC__int32)mant;
			}
			signals[2 * ch][s] = (FLAC__int32)exp - 128;
			signals[2 * ch + 1][s] = sm;
		}
	}
}

/*
 * Transforms signals for Lossless 1-subframe mode:
 *  - converts signals[2*ch] / signals[2*ch+1] into preprocessed dynamic two's complement integer in signals[ch]
 */
void FLAC__transform_frame_lossless_1subframe_float(FLAC__int32 *signals[], const uint32_t channel_base_exp[], FLAC__byte exponent_zero_offsets[], uint32_t channels, uint32_t blocksize)
{
	uint32_t ch;
	for (ch = 0; ch < channels; ch++) {
		uint32_t base_exp = channel_base_exp[ch];
		uint32_t s;
		for (s = 0; s < blocksize; s++) {
			uint32_t exp = (uint32_t)signals[2 * ch][s] & 0xFF;
			uint32_t sign_mant = (uint32_t)signals[2 * ch + 1][s] & 0xFFFFFF;
			if (exp == 0 && (sign_mant & 0x7FFFFF) == 0) {
				signals[ch][s] = 0;
			} else {
				int shift = ((int)exp - 23) - (int)base_exp;
				uint32_t explicit_mant = (1u << 23) | (sign_mant & 0x7FFFFF);
				uint32_t val = (shift >= 0) ? (explicit_mant << shift) : (explicit_mant >> (-shift));
				uint32_t sign = (sign_mant >> 23) & 1;
				FLAC__int32 signed_val = sign ? -(FLAC__int32)val : (FLAC__int32)val;
				signals[ch][s] = signed_val;
			}
		}
		exponent_zero_offsets[ch] = (FLAC__byte)base_exp;
	}
}

/*
 * Accumulates MD5 checksum over the unencoded 32-bit float audio samples reconstructed
 * from the split signals (exp = split_signals[2*ch], sign_mant = split_signals[2*ch+1]):
 * Accumulates 4 bytes per sample (little-endian IEEE 754 float32 bit pattern)
 */
FLAC__bool FLAC__MD5Accumulate_split_float(FLAC__MD5Context *ctx, const FLAC__int32 * const split_signals[], uint32_t channels, uint32_t samples)
{
	size_t bytes_needed = 0;
	uint32_t sample, channel;
	FLAC__byte *buf;

	if (channels > 1024)
		return false;

	bytes_needed = (size_t)samples * (size_t)channels * 4;

	if (bytes_needed == 0)
		return true;

	buf = safe_malloc_(bytes_needed);
	if (!buf)
		return false;

	{
		size_t k = 0;
		for (sample = 0; sample < samples; sample++) {
			for (channel = 0; channel < channels; channel++) {
				const uint32_t exp = (uint32_t)split_signals[2 * channel][sample] & 0xFF;
				const uint32_t sign_mant = (uint32_t)split_signals[2 * channel + 1][sample] & 0xFFFFFF;
				const uint32_t raw_val = (sign_mant & 0x800000 ? 0x80000000 : 0) | (exp << 23) | (sign_mant & 0x7FFFFF);
				buf[k++] = (FLAC__byte)(raw_val);
				buf[k++] = (FLAC__byte)(raw_val >> 8);
				buf[k++] = (FLAC__byte)(raw_val >> 16);
				buf[k++] = (FLAC__byte)(raw_val >> 24);
			}
		}
	}

	FLAC__MD5Update(ctx, buf, (uint32_t)bytes_needed);
	free(buf);
	return true;
}

/*
 * Accumulates MD5 checksum over raw IEEE 754 32-bit float samples (channel-separated buffers):
 * Accumulates 4 bytes per sample (little-endian IEEE 754 float32 bit pattern)
 */
FLAC__bool FLAC__MD5Accumulate_float(FLAC__MD5Context *ctx, const uint32_t * const float_signals[], uint32_t channels, uint32_t samples)
{
	size_t bytes_needed = 0;
	uint32_t sample, channel;
	FLAC__byte *buf;

	if (channels > 1024)
		return false;

	bytes_needed = (size_t)samples * (size_t)channels * 4;

	if (bytes_needed == 0)
		return true;

	buf = safe_malloc_(bytes_needed);
	if (!buf)
		return false;

	{
		size_t k = 0;
		for (sample = 0; sample < samples; sample++) {
			for (channel = 0; channel < channels; channel++) {
				const uint32_t raw_val = float_signals[channel][sample];
				buf[k++] = (FLAC__byte)(raw_val);
				buf[k++] = (FLAC__byte)(raw_val >> 8);
				buf[k++] = (FLAC__byte)(raw_val >> 16);
				buf[k++] = (FLAC__byte)(raw_val >> 24);
			}
		}
	}

	FLAC__MD5Update(ctx, buf, (uint32_t)bytes_needed);
	free(buf);
	return true;
}
