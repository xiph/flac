/* libFLAC - Free Lossless Audio Codec library
 * Copyright (C) 2000-2009  Josh Coalson
 * Copyright (C) 2011-2016  Xiph.Org Foundation
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

#ifndef FLAC__INTEGER_ONLY_LIBRARY
#ifndef FLAC__NO_ASM
#if defined(FLAC__CPU_PPC64) && defined(FLAC__USE_VSX)

#include "private/cpu.h"
#include "private/lpc.h"
#include "FLAC/assert.h"
#include "FLAC/format.h"

#include <altivec.h>

#ifdef FLAC__HAS_TARGET_POWER8
__attribute__((target("cpu=power8")))
void FLAC__lpc_compute_autocorrelation_intrin_power8_vsx_lag_14(const FLAC__real data[], uint32_t data_len, uint32_t lag, double autoc[])
{
	// This function calculates autocorrelation with POWERPC-specific
	// vector functions up to a lag of 14 (or max LPC order of 13)
	long i;
	long limit = (long)data_len - 14;
	const FLAC__real *base;
	vector double sum0 = { 0.0f, 0.0f};
	vector double sum1 = { 0.0f, 0.0f};
	vector double sum2 = { 0.0f, 0.0f};
	vector double sum3 = { 0.0f, 0.0f};
	vector double sum4 = { 0.0f, 0.0f};
	vector double sum5 = { 0.0f, 0.0f};
	vector double sum6 = { 0.0f, 0.0f};
	vector double sum10 = { 0.0f, 0.0f};
	vector double sum11 = { 0.0f, 0.0f};
	vector double sum12 = { 0.0f, 0.0f};
	vector double sum13 = { 0.0f, 0.0f};
	vector double sum14 = { 0.0f, 0.0f};
	vector double sum15 = { 0.0f, 0.0f};
	vector double sum16 = { 0.0f, 0.0f};
	vector float dtemp;
	vector double d0, d1, d2, d3, d4, d5, d6;
#if WORDS_BIGENDIAN
	vector unsigned long long vperm = { 0x08090A0B0C0D0E0F, 0x1011121314151617 };
	vector unsigned long long vsel = { 0x0000000000000000, 0xFFFFFFFFFFFFFFFF };
#else
	vector unsigned long long vperm = { 0x0F0E0D0C0B0A0908, 0x1716151413121110 };
	vector unsigned long long vsel = { 0xFFFFFFFFFFFFFFFF, 0x0000000000000000 };
#endif

	(void) lag;
	FLAC__ASSERT(lag <= 14);

	base = data;

	// First, check whether it is possible to load
	// 16 elements at once
	if(limit > 2){
		// Convert all floats to doubles
		dtemp = vec_vsx_ld(0, base);
		d0 = vec_doubleh(dtemp);
		d1 = vec_doublel(dtemp);
		dtemp = vec_vsx_ld(16, base);
		d2 = vec_doubleh(dtemp);
		d3 = vec_doublel(dtemp);
		dtemp = vec_vsx_ld(32, base);
		d4 = vec_doubleh(dtemp);
		d5 = vec_doublel(dtemp);
		dtemp = vec_vsx_ld(48, base);
		d6 = vec_doubleh(dtemp);

		base += 14;

		// Loop until nearing data_len
		for (i = 0; i <= (limit-2); i += 2) {
			vector double d, d7;

			// Load next 2 datapoints and convert to double
			// data[i+14] and data[i+15]
			dtemp = vec_vsx_ld(0, base);
			d7 = vec_doubleh(dtemp);
			base += 2;

			// Create vector d with both elements set to the first
			// element of d0, so both elements data[i]
			d = vec_splat(d0, 0);
			sum0 += d0 * d; // Multiply data[i] with data[i] and data[i+1]
			sum1 += d1 * d; // Multiply data[i] with data[i+2] and data[i+3]
			sum2 += d2 * d; // Multiply data[i] with data[i+4] and data[i+5]
			sum3 += d3 * d; // Multiply data[i] with data[i+6] and data[i+7]
			sum4 += d4 * d; // Multiply data[i] with data[i+8] and data[i+9]
			sum5 += d5 * d; // Multiply data[i] with data[i+10] and data[i+11]
			sum6 += d6 * d; // Multiply data[i] with data[i+12] and data[i+13]

			// Set both elements of d to data[i+1]
			d = vec_splat(d0, 1);

			// Set d0 to data[i+14] and data[i+1]
			d0 = vec_sel(d0, d7, vsel);
			sum10 += d0 * d; // Multiply data[i+1] with data[i+14] and data[i+1]
			sum11 += d1 * d; // Multiply data[i+1] with data[i+2] and data[i+3]
			sum12 += d2 * d;
			sum13 += d3 * d;
			sum14 += d4 * d;
			sum15 += d5 * d;
			sum16 += d6 * d; // Multiply data[i+1] with data[i+12] and data[i+13]

			// Shift all loaded values one vector (2 elements) so the next
			// iterations aligns again
			d0 = d1;
			d1 = d2;
			d2 = d3;
			d3 = d4;
			d4 = d5;
			d5 = d6;
			d6 = d7;
		}

		// Because the values in sum10..sum16 do not align with
		// the values in sum0..sum6, these need to be 'left-rotated'
		// before adding them to sum0..sum6
		sum0 += vec_perm(sum10, sum11, (vector unsigned char)vperm);
		sum1 += vec_perm(sum11, sum12, (vector unsigned char)vperm);
		sum2 += vec_perm(sum12, sum13, (vector unsigned char)vperm);
		sum3 += vec_perm(sum13, sum14, (vector unsigned char)vperm);
		sum4 += vec_perm(sum14, sum15, (vector unsigned char)vperm);
		sum5 += vec_perm(sum15, sum16, (vector unsigned char)vperm);
		sum6 += vec_perm(sum16, sum10, (vector unsigned char)vperm);
	}else{
		i = 0;
	}

	// Store result
	vec_vsx_st(sum0, 0, autoc);
	vec_vsx_st(sum1, 16, autoc);
	vec_vsx_st(sum2, 32, autoc);
	vec_vsx_st(sum3, 48, autoc);
	vec_vsx_st(sum4, 64, autoc);
	vec_vsx_st(sum5, 80, autoc);
	vec_vsx_st(sum6, 96, autoc);

	// Process remainder of samples in a non-VSX way
	for (; i < (long)data_len; i++) {
		uint32_t coeff;

		FLAC__real d = data[i];
		for (coeff = 0; coeff < data_len - i; coeff++)
			autoc[coeff] += d * data[i+coeff];
	}
}

__attribute__((target("cpu=power8")))
void FLAC__lpc_compute_autocorrelation_intrin_power8_vsx_lag_12(const FLAC__real data[], uint32_t data_len, uint32_t lag, double autoc[])
{
	// This function calculates autocorrelation with POWERPC-specific
	// vector functions up to a lag of 12 (or max LPC order of 11)
	// For explanation, please see the lag_14 version of this function
	long i;
	long limit = (long)data_len - 12;
	const FLAC__real *base;
	vector double sum0 = { 0.0f, 0.0f};
	vector double sum1 = { 0.0f, 0.0f};
	vector double sum2 = { 0.0f, 0.0f};
	vector double sum3 = { 0.0f, 0.0f};
	vector double sum4 = { 0.0f, 0.0f};
	vector double sum5 = { 0.0f, 0.0f};
	vector double sum10 = { 0.0f, 0.0f};
	vector double sum11 = { 0.0f, 0.0f};
	vector double sum12 = { 0.0f, 0.0f};
	vector double sum13 = { 0.0f, 0.0f};
	vector double sum14 = { 0.0f, 0.0f};
	vector double sum15 = { 0.0f, 0.0f};
	vector float dtemp;
	vector double d0, d1, d2, d3, d4, d5;
#if WORDS_BIGENDIAN
	vector unsigned long long vperm = { 0x08090A0B0C0D0E0F, 0x1011121314151617 };
	vector unsigned long long vsel = { 0x0000000000000000, 0xFFFFFFFFFFFFFFFF };
#else
	vector unsigned long long vperm = { 0x0F0E0D0C0B0A0908, 0x1716151413121110 };
	vector unsigned long long vsel = { 0xFFFFFFFFFFFFFFFF, 0x0000000000000000 };
#endif

	(void) lag;
	FLAC__ASSERT(lag <= 12);

	base = data;
	if(limit > 0){
		dtemp = vec_vsx_ld(0, base);
		d0 = vec_doubleh(dtemp);
		d1 = vec_doublel(dtemp);
		dtemp = vec_vsx_ld(16, base);
		d2 = vec_doubleh(dtemp);
		d3 = vec_doublel(dtemp);
		dtemp = vec_vsx_ld(32, base);
		d4 = vec_doubleh(dtemp);
		d5 = vec_doublel(dtemp);

		base += 12;

		for (i = 0; i <= (limit-2); i += 2) {
			vector double d, d6;

			dtemp = vec_vsx_ld(0, base);
			d6 = vec_doubleh(dtemp);
			base += 2;

			d = vec_splat(d0, 0);
			sum0 += d0 * d;
			sum1 += d1 * d;
			sum2 += d2 * d;
			sum3 += d3 * d;
			sum4 += d4 * d;
			sum5 += d5 * d;

			d = vec_splat(d0, 1);
			d0 = vec_sel(d0, d6, vsel);
			sum10 += d0 * d;
			sum11 += d1 * d;
			sum12 += d2 * d;
			sum13 += d3 * d;
			sum14 += d4 * d;
			sum15 += d5 * d;

			d0 = d1;
			d1 = d2;
			d2 = d3;
			d3 = d4;
			d4 = d5;
			d5 = d6;
		}

		sum0 += vec_perm(sum10, sum11, (vector unsigned char)vperm);
		sum1 += vec_perm(sum11, sum12, (vector unsigned char)vperm);
		sum2 += vec_perm(sum12, sum13, (vector unsigned char)vperm);
		sum3 += vec_perm(sum13, sum14, (vector unsigned char)vperm);
		sum4 += vec_perm(sum14, sum15, (vector unsigned char)vperm);
		sum5 += vec_perm(sum15, sum10, (vector unsigned char)vperm);
	}else{
		i = 0;
	}

	vec_vsx_st(sum0, 0, autoc);
	vec_vsx_st(sum1, 16, autoc);
	vec_vsx_st(sum2, 32, autoc);
	vec_vsx_st(sum3, 48, autoc);
	vec_vsx_st(sum4, 64, autoc);
	vec_vsx_st(sum5, 80, autoc);

	for (; i < (long)data_len; i++) {
		uint32_t coeff;

		FLAC__real d = data[i];
		for (coeff = 0; coeff < data_len - i; coeff++)
			autoc[coeff] += d * data[i+coeff];
	}
}

__attribute__((target("cpu=power8")))
void FLAC__lpc_compute_autocorrelation_intrin_power8_vsx_lag_8(const FLAC__real data[], uint32_t data_len, uint32_t lag, double autoc[])
{
	// This function calculates autocorrelation with POWERPC-specific
	// vector functions up to a lag of 8 (or max LPC order of 7)
	// For explanation, please see the lag_14 version of this function
	long i;
	long limit = (long)data_len - 8;
	const FLAC__real *base;
	vector double sum0 = { 0.0f, 0.0f};
	vector double sum1 = { 0.0f, 0.0f};
	vector double sum2 = { 0.0f, 0.0f};
	vector double sum3 = { 0.0f, 0.0f};
	vector double sum10 = { 0.0f, 0.0f};
	vector double sum11 = { 0.0f, 0.0f};
	vector double sum12 = { 0.0f, 0.0f};
	vector double sum13 = { 0.0f, 0.0f};
	vector float dtemp;
	vector double d0, d1, d2, d3;
#if WORDS_BIGENDIAN
	vector unsigned long long vperm = { 0x08090A0B0C0D0E0F, 0x1011121314151617 };
	vector unsigned long long vsel = { 0x0000000000000000, 0xFFFFFFFFFFFFFFFF };
#else
	vector unsigned long long vperm = { 0x0F0E0D0C0B0A0908, 0x1716151413121110 };
	vector unsigned long long vsel = { 0xFFFFFFFFFFFFFFFF, 0x0000000000000000 };
#endif

	(void) lag;
	FLAC__ASSERT(lag <= 8);

	base = data;
	if(limit > 0){
		dtemp = vec_vsx_ld(0, base);
		d0 = vec_doubleh(dtemp);
		d1 = vec_doublel(dtemp);
		dtemp = vec_vsx_ld(16, base);
		d2 = vec_doubleh(dtemp);
		d3 = vec_doublel(dtemp);

		base += 8;

		for (i = 0; i <= (limit-2); i += 2) {
			vector double d, d4;

			dtemp = vec_vsx_ld(0, base);
			d4 = vec_doubleh(dtemp);
			base += 2;

			d = vec_splat(d0, 0);
			sum0 += d0 * d;
			sum1 += d1 * d;
			sum2 += d2 * d;
			sum3 += d3 * d;

			d = vec_splat(d0, 1);
			d0 = vec_sel(d0, d4, vsel);
			sum10 += d0 * d;
			sum11 += d1 * d;
			sum12 += d2 * d;
			sum13 += d3 * d;

			d0 = d1;
			d1 = d2;
			d2 = d3;
			d3 = d4;
		}

		sum0 += vec_perm(sum10, sum11, (vector unsigned char)vperm);
		sum1 += vec_perm(sum11, sum12, (vector unsigned char)vperm);
		sum2 += vec_perm(sum12, sum13, (vector unsigned char)vperm);
		sum3 += vec_perm(sum13, sum10, (vector unsigned char)vperm);

	}else{
		i = 0;
	}

	vec_vsx_st(sum0, 0, autoc);
	vec_vsx_st(sum1, 16, autoc);
	vec_vsx_st(sum2, 32, autoc);
	vec_vsx_st(sum3, 48, autoc);

	for (; i < (long)data_len; i++) {
		uint32_t coeff;

		FLAC__real d = data[i];
		for (coeff = 0; coeff < data_len - i; coeff++)
			autoc[coeff] += d * data[i+coeff];
	}
}
#endif /* FLAC__HAS_TARGET_POWER8 */

#ifdef FLAC__HAS_TARGET_POWER9
__attribute__((target("cpu=power9")))
void FLAC__lpc_compute_autocorrelation_intrin_power9_vsx_lag_14(const FLAC__real data[], uint32_t data_len, uint32_t lag, double autoc[])
{
	// This function calculates autocorrelation with POWERPC-specific
	// vector functions up to a lag of 14 (or max LPC order of 13)
	// For explanation, please see the power8 version of this function
	long i;
	long limit = (long)data_len - 14;
	const FLAC__real *base;
	vector double sum0 = { 0.0f, 0.0f};
	vector double sum1 = { 0.0f, 0.0f};
	vector double sum2 = { 0.0f, 0.0f};
	vector double sum3 = { 0.0f, 0.0f};
	vector double sum4 = { 0.0f, 0.0f};
	vector double sum5 = { 0.0f, 0.0f};
	vector double sum6 = { 0.0f, 0.0f};
	vector double sum10 = { 0.0f, 0.0f};
	vector double sum11 = { 0.0f, 0.0f};
	vector double sum12 = { 0.0f, 0.0f};
	vector double sum13 = { 0.0f, 0.0f};
	vector double sum14 = { 0.0f, 0.0f};
	vector double sum15 = { 0.0f, 0.0f};
	vector double sum16 = { 0.0f, 0.0f};
	vector float dtemp;
	vector double d0, d1, d2, d3, d4, d5, d6;
#if WORDS_BIGENDIAN
	vector unsigned long long vperm = { 0x08090A0B0C0D0E0F, 0x1011121314151617 };
	vector unsigned long long vsel = { 0x0000000000000000, 0xFFFFFFFFFFFFFFFF };
#else
	vector unsigned long long vperm = { 0x0F0E0D0C0B0A0908, 0x1716151413121110 };
	vector unsigned long long vsel = { 0xFFFFFFFFFFFFFFFF, 0x0000000000000000 };
#endif

	(void) lag;
	FLAC__ASSERT(lag <= 14);

	base = data;
	if(limit > 2){
		dtemp = vec_vsx_ld(0, base);
		d0 = vec_doubleh(dtemp);
		d1 = vec_doublel(dtemp);
		dtemp = vec_vsx_ld(16, base);
		d2 = vec_doubleh(dtemp);
		d3 = vec_doublel(dtemp);
		dtemp = vec_vsx_ld(32, base);
		d4 = vec_doubleh(dtemp);
		d5 = vec_doublel(dtemp);
		dtemp = vec_vsx_ld(48, base);
		d6 = vec_doubleh(dtemp);

		base += 14;

		for (i = 0; i <= (limit-2); i += 2) {
			vector double d, d7;

			dtemp = vec_vsx_ld(0, base);
			d7 = vec_doubleh(dtemp);
			base += 2;

			d = vec_splat(d0, 0);
			sum0 += d0 * d;
			sum1 += d1 * d;
			sum2 += d2 * d;
			sum3 += d3 * d;
			sum4 += d4 * d;
			sum5 += d5 * d;
			sum6 += d6 * d;

			d = vec_splat(d0, 1);
			d0 = vec_sel(d0, d7, vsel);
			sum10 += d0 * d;
			sum11 += d1 * d;
			sum12 += d2 * d;
			sum13 += d3 * d;
			sum14 += d4 * d;
			sum15 += d5 * d;
			sum16 += d6 * d;

			d0 = d1;
			d1 = d2;
			d2 = d3;
			d3 = d4;
			d4 = d5;
			d5 = d6;
			d6 = d7;
		}

		sum0 += vec_perm(sum10, sum11, (vector unsigned char)vperm);
		sum1 += vec_perm(sum11, sum12, (vector unsigned char)vperm);
		sum2 += vec_perm(sum12, sum13, (vector unsigned char)vperm);
		sum3 += vec_perm(sum13, sum14, (vector unsigned char)vperm);
		sum4 += vec_perm(sum14, sum15, (vector unsigned char)vperm);
		sum5 += vec_perm(sum15, sum16, (vector unsigned char)vperm);
		sum6 += vec_perm(sum16, sum10, (vector unsigned char)vperm);
	}else{
		i = 0;
	}

	vec_vsx_st(sum0, 0, autoc);
	vec_vsx_st(sum1, 16, autoc);
	vec_vsx_st(sum2, 32, autoc);
	vec_vsx_st(sum3, 48, autoc);
	vec_vsx_st(sum4, 64, autoc);
	vec_vsx_st(sum5, 80, autoc);
	vec_vsx_st(sum6, 96, autoc);

	for (; i < (long)data_len; i++) {
		uint32_t coeff;

		FLAC__real d = data[i];
		for (coeff = 0; coeff < data_len - i; coeff++)
			autoc[coeff] += d * data[i+coeff];
	}
}

__attribute__((target("cpu=power9")))
void FLAC__lpc_compute_autocorrelation_intrin_power9_vsx_lag_12(const FLAC__real data[], uint32_t data_len, uint32_t lag, double autoc[])
{
	// This function calculates autocorrelation with POWERPC-specific
	// vector functions up to a lag of 12 (or max LPC order of 11)
	// For explanation, please see the power9, lag_14 version of this function
	long i;
	long limit = (long)data_len - 12;
	const FLAC__real *base;
	vector double sum0 = { 0.0f, 0.0f};
	vector double sum1 = { 0.0f, 0.0f};
	vector double sum2 = { 0.0f, 0.0f};
	vector double sum3 = { 0.0f, 0.0f};
	vector double sum4 = { 0.0f, 0.0f};
	vector double sum5 = { 0.0f, 0.0f};
	vector double sum10 = { 0.0f, 0.0f};
	vector double sum11 = { 0.0f, 0.0f};
	vector double sum12 = { 0.0f, 0.0f};
	vector double sum13 = { 0.0f, 0.0f};
	vector double sum14 = { 0.0f, 0.0f};
	vector double sum15 = { 0.0f, 0.0f};
	vector float dtemp;
	vector double d0, d1, d2, d3, d4, d5;
#if WORDS_BIGENDIAN
	vector unsigned long long vperm = { 0x08090A0B0C0D0E0F, 0x1011121314151617 };
	vector unsigned long long vsel = { 0x0000000000000000, 0xFFFFFFFFFFFFFFFF };
#else
	vector unsigned long long vperm = { 0x0F0E0D0C0B0A0908, 0x1716151413121110 };
	vector unsigned long long vsel = { 0xFFFFFFFFFFFFFFFF, 0x0000000000000000 };
#endif

	(void) lag;
	FLAC__ASSERT(lag <= 12);

	base = data;
	if(limit > 0){
		dtemp = vec_vsx_ld(0, base);
		d0 = vec_doubleh(dtemp);
		d1 = vec_doublel(dtemp);
		dtemp = vec_vsx_ld(16, base);
		d2 = vec_doubleh(dtemp);
		d3 = vec_doublel(dtemp);
		dtemp = vec_vsx_ld(32, base);
		d4 = vec_doubleh(dtemp);
		d5 = vec_doublel(dtemp);

		base += 12;

		for (i = 0; i <= (limit-2); i += 2) {
			vector double d, d6;

			dtemp = vec_vsx_ld(0, base);
			d6 = vec_doubleh(dtemp);
			base += 2;

			d = vec_splat(d0, 0);
			sum0 += d0 * d;
			sum1 += d1 * d;
			sum2 += d2 * d;
			sum3 += d3 * d;
			sum4 += d4 * d;
			sum5 += d5 * d;

			d = vec_splat(d0, 1);
			d0 = vec_sel(d0, d6, vsel);
			sum10 += d0 * d;
			sum11 += d1 * d;
			sum12 += d2 * d;
			sum13 += d3 * d;
			sum14 += d4 * d;
			sum15 += d5 * d;

			d0 = d1;
			d1 = d2;
			d2 = d3;
			d3 = d4;
			d4 = d5;
			d5 = d6;
		}

		sum0 += vec_perm(sum10, sum11, (vector unsigned char)vperm);
		sum1 += vec_perm(sum11, sum12, (vector unsigned char)vperm);
		sum2 += vec_perm(sum12, sum13, (vector unsigned char)vperm);
		sum3 += vec_perm(sum13, sum14, (vector unsigned char)vperm);
		sum4 += vec_perm(sum14, sum15, (vector unsigned char)vperm);
		sum5 += vec_perm(sum15, sum10, (vector unsigned char)vperm);
	}else{
		i = 0;
	}

	vec_vsx_st(sum0, 0, autoc);
	vec_vsx_st(sum1, 16, autoc);
	vec_vsx_st(sum2, 32, autoc);
	vec_vsx_st(sum3, 48, autoc);
	vec_vsx_st(sum4, 64, autoc);
	vec_vsx_st(sum5, 80, autoc);

	for (; i < (long)data_len; i++) {
		uint32_t coeff;

		FLAC__real d = data[i];
		for (coeff = 0; coeff < data_len - i; coeff++)
			autoc[coeff] += d * data[i+coeff];
	}
}

__attribute__((target("cpu=power9")))
void FLAC__lpc_compute_autocorrelation_intrin_power9_vsx_lag_8(const FLAC__real data[], uint32_t data_len, uint32_t lag, double autoc[])
{
	// This function calculates autocorrelation with POWERPC-specific
	// vector functions up to a lag of 8 (or max LPC order of 7)
	// For explanation, please see the power9, lag_14 version of this function
	long i;
	long limit = (long)data_len - 8;
	const FLAC__real *base;
	vector double sum0 = { 0.0f, 0.0f};
	vector double sum1 = { 0.0f, 0.0f};
	vector double sum2 = { 0.0f, 0.0f};
	vector double sum3 = { 0.0f, 0.0f};
	vector double sum10 = { 0.0f, 0.0f};
	vector double sum11 = { 0.0f, 0.0f};
	vector double sum12 = { 0.0f, 0.0f};
	vector double sum13 = { 0.0f, 0.0f};
	vector float dtemp;
	vector double d0, d1, d2, d3;
#if WORDS_BIGENDIAN
	vector unsigned long long vperm = { 0x08090A0B0C0D0E0F, 0x1011121314151617 };
	vector unsigned long long vsel = { 0x0000000000000000, 0xFFFFFFFFFFFFFFFF };
#else
	vector unsigned long long vperm = { 0x0F0E0D0C0B0A0908, 0x1716151413121110 };
	vector unsigned long long vsel = { 0xFFFFFFFFFFFFFFFF, 0x0000000000000000 };
#endif

	(void) lag;
	FLAC__ASSERT(lag <= 8);

	base = data;
	if(limit > 0){
		dtemp = vec_vsx_ld(0, base);
		d0 = vec_doubleh(dtemp);
		d1 = vec_doublel(dtemp);
		dtemp = vec_vsx_ld(16, base);
		d2 = vec_doubleh(dtemp);
		d3 = vec_doublel(dtemp);

		base += 8;

		for (i = 0; i <= (limit-2); i += 2) {
			vector double d, d4;

			dtemp = vec_vsx_ld(0, base);
			d4 = vec_doubleh(dtemp);
			base += 2;

			d = vec_splat(d0, 0);
			sum0 += d0 * d;
			sum1 += d1 * d;
			sum2 += d2 * d;
			sum3 += d3 * d;

			d = vec_splat(d0, 1);
			d0 = vec_sel(d0, d4, vsel);
			sum10 += d0 * d;
			sum11 += d1 * d;
			sum12 += d2 * d;
			sum13 += d3 * d;

			d0 = d1;
			d1 = d2;
			d2 = d3;
			d3 = d4;
		}

		sum0 += vec_perm(sum10, sum11, (vector unsigned char)vperm);
		sum1 += vec_perm(sum11, sum12, (vector unsigned char)vperm);
		sum2 += vec_perm(sum12, sum13, (vector unsigned char)vperm);
		sum3 += vec_perm(sum13, sum10, (vector unsigned char)vperm);

	}else{
		i = 0;
	}

	vec_vsx_st(sum0, 0, autoc);
	vec_vsx_st(sum1, 16, autoc);
	vec_vsx_st(sum2, 32, autoc);
	vec_vsx_st(sum3, 48, autoc);

	for (; i < (long)data_len; i++) {
		uint32_t coeff;

		FLAC__real d = data[i];
		for (coeff = 0; coeff < data_len - i; coeff++)
			autoc[coeff] += d * data[i+coeff];
	}
}
#endif /* FLAC__HAS_TARGET_POWER9 */

#endif /* FLAC__CPU_PPC64 && FLAC__USE_VSX */
#endif /* FLAC__NO_ASM */
#endif /* FLAC__INTEGER_ONLY_LIBRARY */
