/* libFLAC - Free Lossless Audio Codec library
 * Copyright (C) 2001-2009  Josh Coalson
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
   2. doing some basic bit manipulation (current)
		saves ~3 bits theoretically
		my tests resulted in a ~70% ratio
   3. splitting the "exponent" (8 bit) and "sign+significand" (24 bit) parts of floats into two channels,
	subtracting an (automatically recognised) DC offset from the "exponents" channel
	and	storing the offset in each frame header (i.e. unsigned to signed conversion)
		could be better (haven't tried it yet. hard for me to implement).
		my guess would be ~62% ratio.
*/

#include <stddef.h>
#include "protected/stream_encoder.h"
#include "private/transform_float.h"

/*
float FLAC__transform_i32_to_f32(uint32_t in) {
	// simply doing "return in;", the compiler would instruct the CPU to convert the int
	// like below. this is just a reminder-note on how f32 is represented (IEEE 754 standard).
	union {
		float    f;
		int32_t i;
	} conv = { .i = in }; // evil floating point bit level hacking

	FLAC__bool sign = conv.i < 0;
	int32_t exponent; // 8 bits
	int32_t significand; // 23 bits stored in float & 1 implicit leading bit. total = 24 bits
	int32_t val;

	sign = conv.i < 0; // (conv.i & 0b10000000000000000000000000000000) != 0
	int32_t exponent_bits = (conv.i & 0b01111111100000000000000000000000) >> 23;
	int32_t significand_bits = conv.i & 0b00000000011111111111111111111111;

	switch (exponent_bits) {
		case 0b11111111:
			if (significand_bits != 0) {
				// NaN
			}
			else {
				// infinity
			}
			break;
		case 0b00000000:
			if (significand_bits != 0) {
				// subnormal number
			}
			else {
				exponent = 0;
				significand = 0;
				// zero
			}
			break;
		default:
			significand = significand_bits | 0b00000000100000000000000000000000; // implicit leading bit
			exponent = exponent_bits - 127 - 23;
			// 127 for biased form.
			// 23 of the exponent is already applied to the significand,
			// because it was supposed to be the fraction part.
			if (exponent < -24) {
				// too small
			}
			else if (exponent < 0 && ((0b11111111111111111111111111111111 >> (32 + exponent)) & significand) != 0) {
				// too small
			}
			else if (exponent >= 7) {
				// too large
			}
			else { // exponent is in [-23, 6]
				if (exponent < 0) {
					val = significand >> (-exponent);
				}
				else {
					val = significand << exponent;
				}
				if (sign) {
					val = -val;
				}
			}
	}
}
*/

void FLAC__transform_f32_buffer_to_i32_signal(FLAC__int32 *dest, const FLAC__int32 *src, size_t n)
{
	uint32_t sign;
	uint32_t exponent;
	uint32_t significand;
	for(size_t i = 0; i < n; i++) {
		sign = (src[i] & 0b10000000000000000000000000000000) != 0;
		exponent = (src[i] & 0b01111111100000000000000000000000) >> 23;
		significand = src[i] & 0b00000000011111111111111111111111;
		exponent = ((exponent + 1) % 256) ^ 0b10000000;
		dest[i] = (exponent << 24) | (sign << 23) | significand;
	}
}

bool FLAC__transform_f32_interleaved_buffer_to_i32_signal(FLAC__int32 *dest, const FLAC__int32 *src, uint32_t *i, uint32_t *j, uint32_t *k, uint32_t *channel, struct FLAC__StreamEncoderProtected *protected_, const uint32_t current_sample_number, const uint32_t blocksize, const uint32_t samples, const uint32_t channels, const FLAC__int32 sample_min, const FLAC__int32 sample_max)
{
	uint32_t sign;
	uint32_t exponent;
	uint32_t significand;
	for(*i = current_sample_number; *i <= blocksize && *j < samples; (*i)++, (*j)++) {
		for(*channel = 0; *channel < channels; (*channel)++) {
			if(src[*k] < sample_min || src[*k] > sample_max) {
				protected_->state = FLAC__STREAM_ENCODER_CLIENT_ERROR;
				return false;
			}

			sign = (src[*k] & 0b10000000000000000000000000000000) != 0;
			exponent = (src[*k] & 0b01111111100000000000000000000000) >> 23;
			significand = src[*k] & 0b00000000011111111111111111111111;
			exponent = ((exponent + 1) % 256) ^ 0b10000000;
			dest[*i] = (exponent << 24) | (sign << 23) | significand;

			(*k)++;
		}
	}
	return true;
}

void FLAC__transform_i32_signal_to_f32_buffer(uint32_t *buffer, size_t n)
{
	uint32_t sign;
	uint32_t exponent;
	uint32_t significand;
	for(size_t i = 0; i < n; i++) {
		sign = (buffer[i] & 0b00000000100000000000000000000000) << 8;
		exponent = buffer[i] >> 24; // would not work if buffer was int32_t
		significand = buffer[i] & 0b00000000011111111111111111111111;
		exponent ^= 0b10000000;
		exponent = (exponent == 0) ? 255 : (exponent - 1);
		buffer[i] = (sign) | (exponent << 23) | (significand);
	}
}