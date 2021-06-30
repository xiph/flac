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
void FLAC__lpc_compute_autocorrelation_intrin_power8_vsx_lag_16(const FLAC__real data[], uint32_t data_len, uint32_t lag, double autoc[])
{
	long i;
	long limit = (long)data_len - 16;
	const FLAC__real *base;
	vector double sum0 = { 0.0f, 0.0f};
	vector double sum1 = { 0.0f, 0.0f};
	vector double sum2 = { 0.0f, 0.0f};
	vector double sum3 = { 0.0f, 0.0f};
	vector double sum4 = { 0.0f, 0.0f};
	vector double sum5 = { 0.0f, 0.0f};
	vector double sum6 = { 0.0f, 0.0f};
	vector double sum7 = { 0.0f, 0.0f};
	vector double sum10 = { 0.0f, 0.0f};
	vector double sum11 = { 0.0f, 0.0f};
	vector double sum12 = { 0.0f, 0.0f};
	vector double sum13 = { 0.0f, 0.0f};
	vector double sum14 = { 0.0f, 0.0f};
	vector double sum15 = { 0.0f, 0.0f};
	vector double sum16 = { 0.0f, 0.0f};
	vector double sum17 = { 0.0f, 0.0f};
	vector double sum20 = { 0.0f, 0.0f};
	vector double sum21 = { 0.0f, 0.0f};
	vector double sum22 = { 0.0f, 0.0f};
	vector double sum23 = { 0.0f, 0.0f};
	vector double sum24 = { 0.0f, 0.0f};
	vector double sum25 = { 0.0f, 0.0f};
	vector double sum26 = { 0.0f, 0.0f};
	vector double sum27 = { 0.0f, 0.0f};
	vector double sum30 = { 0.0f, 0.0f};
	vector double sum31 = { 0.0f, 0.0f};
	vector double sum32 = { 0.0f, 0.0f};
	vector double sum33 = { 0.0f, 0.0f};
	vector double sum34 = { 0.0f, 0.0f};
	vector double sum35 = { 0.0f, 0.0f};
	vector double sum36 = { 0.0f, 0.0f};
	vector double sum37 = { 0.0f, 0.0f};
	vector float d0, d1, d2, d3, d4;
#if WORDS_BIGENDIAN
	vector unsigned int vsel1 = { 0x00000000, 0x00000000, 0x00000000, 0xFFFFFFFF };
	vector unsigned int vsel2 = { 0x00000000, 0x00000000, 0xFFFFFFFF, 0xFFFFFFFF };
	vector unsigned int vsel3 = { 0x00000000, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF };
	vector unsigned long long vperm = { 0x08090A0B0C0D0E0F, 0x1011121314151617 };
#else
	vector unsigned int vsel1 = { 0xFFFFFFFF, 0x00000000, 0x00000000, 0x00000000 };
	vector unsigned int vsel2 = { 0xFFFFFFFF, 0xFFFFFFFF, 0x00000000, 0x00000000 };
	vector unsigned int vsel3 = { 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0x00000000 };
	vector unsigned long long vperm = { 0x0F0E0D0C0B0A0908, 0x1716151413121110 };
#endif

	(void) lag;
	FLAC__ASSERT(lag <= 16);
	FLAC__ASSERT(lag <= data_len);

	base = data;

	d0 = vec_vsx_ld(0, base);
	d1 = vec_vsx_ld(16, base);
	d2 = vec_vsx_ld(32, base);
	d3 = vec_vsx_ld(48, base);

	base += 16;

	for (i = 0; i <= (limit-4); i += 4) {
		vector float d, mult, d0_orig = d0;

		d4 = vec_vsx_ld(0, base);
		base += 4;

		d = vec_splat(d0_orig, 0);
		mult = d0 * d;
		sum0 += vec_doubleh(mult);
		sum1 += vec_doublel(mult);
		mult = d1 * d;
		sum2 += vec_doubleh(mult);
		sum3 += vec_doublel(mult);
		mult = d2 * d;
		sum4 += vec_doubleh(mult);
		sum5 += vec_doublel(mult);
		mult = d3 * d;
		sum6 += vec_doubleh(mult);
		sum7 += vec_doublel(mult);

		d = vec_splat(d0_orig, 1);
		d0 = vec_sel(d0_orig, d4, vsel1);
		mult = d0 * d;
		sum10 += vec_doubleh(mult);
		sum11 += vec_doublel(mult);
		mult = d1 * d;
		sum12 += vec_doubleh(mult);
		sum13 += vec_doublel(mult);
		mult = d2 * d;
		sum14 += vec_doubleh(mult);
		sum15 += vec_doublel(mult);
		mult = d3 * d;
		sum16 += vec_doubleh(mult);
		sum17 += vec_doublel(mult);

		d = vec_splat(d0_orig, 2);
		d0 = vec_sel(d0_orig, d4, vsel2);
		mult = d0 * d;
		sum20 += vec_doubleh(mult);
		sum21 += vec_doublel(mult);
		mult = d1 * d;
		sum22 += vec_doubleh(mult);
		sum23 += vec_doublel(mult);
		mult = d2 * d;
		sum24 += vec_doubleh(mult);
		sum25 += vec_doublel(mult);
		mult = d3 * d;
		sum26 += vec_doubleh(mult);
		sum27 += vec_doublel(mult);

		d = vec_splat(d0_orig, 3);
		d0 = vec_sel(d0_orig, d4, vsel3);
		mult = d0 * d;
		sum30 += vec_doubleh(mult);
		sum31 += vec_doublel(mult);
		mult = d1 * d;
		sum32 += vec_doubleh(mult);
		sum33 += vec_doublel(mult);
		mult = d2 * d;
		sum34 += vec_doubleh(mult);
		sum35 += vec_doublel(mult);
		mult = d3 * d;
		sum36 += vec_doubleh(mult);
		sum37 += vec_doublel(mult);

		d0 = d1;
		d1 = d2;
		d2 = d3;
		d3 = d4;
	}

	sum0 += vec_perm(sum10, sum11, (vector unsigned char)vperm);
	sum1 += vec_perm(sum11, sum12, (vector unsigned char)vperm);
	sum2 += vec_perm(sum12, sum13, (vector unsigned char)vperm);
	sum3 += vec_perm(sum13, sum14, (vector unsigned char)vperm);
	sum4 += vec_perm(sum14, sum15, (vector unsigned char)vperm);
	sum5 += vec_perm(sum15, sum16, (vector unsigned char)vperm);
	sum6 += vec_perm(sum16, sum17, (vector unsigned char)vperm);
	sum7 += vec_perm(sum17, sum10, (vector unsigned char)vperm);

	sum0 += sum21;
	sum1 += sum22;
	sum2 += sum23;
	sum3 += sum24;
	sum4 += sum25;
	sum5 += sum26;
	sum6 += sum27;
	sum7 += sum20;

	sum0 += vec_perm(sum31, sum32, (vector unsigned char)vperm);
	sum1 += vec_perm(sum32, sum33, (vector unsigned char)vperm);
	sum2 += vec_perm(sum33, sum34, (vector unsigned char)vperm);
	sum3 += vec_perm(sum34, sum35, (vector unsigned char)vperm);
	sum4 += vec_perm(sum35, sum36, (vector unsigned char)vperm);
	sum5 += vec_perm(sum36, sum37, (vector unsigned char)vperm);
	sum6 += vec_perm(sum37, sum30, (vector unsigned char)vperm);
	sum7 += vec_perm(sum30, sum31, (vector unsigned char)vperm);


	vec_vsx_st(sum0, 0, autoc);
	vec_vsx_st(sum1, 16, autoc);
	vec_vsx_st(sum2, 32, autoc);
	vec_vsx_st(sum3, 48, autoc);
	vec_vsx_st(sum4, 64, autoc);
	vec_vsx_st(sum5, 80, autoc);
	vec_vsx_st(sum6, 96, autoc);
	vec_vsx_st(sum7, 112, autoc);

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
	vector double sum20 = { 0.0f, 0.0f};
	vector double sum21 = { 0.0f, 0.0f};
	vector double sum22 = { 0.0f, 0.0f};
	vector double sum23 = { 0.0f, 0.0f};
	vector double sum24 = { 0.0f, 0.0f};
	vector double sum25 = { 0.0f, 0.0f};
	vector double sum30 = { 0.0f, 0.0f};
	vector double sum31 = { 0.0f, 0.0f};
	vector double sum32 = { 0.0f, 0.0f};
	vector double sum33 = { 0.0f, 0.0f};
	vector double sum34 = { 0.0f, 0.0f};
	vector double sum35 = { 0.0f, 0.0f};
	vector float d0, d1, d2, d3;
#if WORDS_BIGENDIAN
	vector unsigned int vsel1 = { 0x00000000, 0x00000000, 0x00000000, 0xFFFFFFFF };
	vector unsigned int vsel2 = { 0x00000000, 0x00000000, 0xFFFFFFFF, 0xFFFFFFFF };
	vector unsigned int vsel3 = { 0x00000000, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF };
	vector unsigned long long vperm = { 0x08090A0B0C0D0E0F, 0x1011121314151617 };
#else
	vector unsigned int vsel1 = { 0xFFFFFFFF, 0x00000000, 0x00000000, 0x00000000 };
	vector unsigned int vsel2 = { 0xFFFFFFFF, 0xFFFFFFFF, 0x00000000, 0x00000000 };
	vector unsigned int vsel3 = { 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0x00000000 };
	vector unsigned long long vperm = { 0x0F0E0D0C0B0A0908, 0x1716151413121110 };
#endif

	(void) lag;
	FLAC__ASSERT(lag <= 12);
	FLAC__ASSERT(lag <= data_len);

	base = data;

	d0 = vec_vsx_ld(0, base);
	d1 = vec_vsx_ld(16, base);
	d2 = vec_vsx_ld(32, base);

	base += 12;

	for (i = 0; i <= (limit-3); i += 4) {
		vector float d, mult, d0_orig = d0;

		d3 = vec_vsx_ld(0, base);
		base += 4;

		d = vec_splat(d0_orig, 0);
		mult = d0 * d;
		sum0 += vec_doubleh(mult);
		sum1 += vec_doublel(mult);
		mult = d1 * d;
		sum2 += vec_doubleh(mult);
		sum3 += vec_doublel(mult);
		mult = d2 * d;
		sum4 += vec_doubleh(mult);
		sum5 += vec_doublel(mult);

		d = vec_splat(d0_orig, 1);
		d0 = vec_sel(d0_orig, d3, vsel1);
		mult = d0 * d;
		sum10 += vec_doubleh(mult);
		sum11 += vec_doublel(mult);
		mult = d1 * d;
		sum12 += vec_doubleh(mult);
		sum13 += vec_doublel(mult);
		mult = d2 * d;
		sum14 += vec_doubleh(mult);
		sum15 += vec_doublel(mult);

		d = vec_splat(d0_orig, 2);
		d0 = vec_sel(d0_orig, d3, vsel2);
		mult = d0 * d;
		sum20 += vec_doubleh(mult);
		sum21 += vec_doublel(mult);
		mult = d1 * d;
		sum22 += vec_doubleh(mult);
		sum23 += vec_doublel(mult);
		mult = d2 * d;
		sum24 += vec_doubleh(mult);
		sum25 += vec_doublel(mult);

		d = vec_splat(d0_orig, 3);
		d0 = vec_sel(d0_orig, d3, vsel3);
		mult = d0 * d;
		sum30 += vec_doubleh(mult);
		sum31 += vec_doublel(mult);
		mult = d1 * d;
		sum32 += vec_doubleh(mult);
		sum33 += vec_doublel(mult);
		mult = d2 * d;
		sum34 += vec_doubleh(mult);
		sum35 += vec_doublel(mult);

		d0 = d1;
		d1 = d2;
		d2 = d3;
	}

	sum0 += vec_perm(sum10, sum11, (vector unsigned char)vperm);
	sum1 += vec_perm(sum11, sum12, (vector unsigned char)vperm);
	sum2 += vec_perm(sum12, sum13, (vector unsigned char)vperm);
	sum3 += vec_perm(sum13, sum14, (vector unsigned char)vperm);
	sum4 += vec_perm(sum14, sum15, (vector unsigned char)vperm);
	sum5 += vec_perm(sum15, sum10, (vector unsigned char)vperm);

	sum0 += sum21;
	sum1 += sum22;
	sum2 += sum23;
	sum3 += sum24;
	sum4 += sum25;
	sum5 += sum20;

	sum0 += vec_perm(sum31, sum32, (vector unsigned char)vperm);
	sum1 += vec_perm(sum32, sum33, (vector unsigned char)vperm);
	sum2 += vec_perm(sum33, sum34, (vector unsigned char)vperm);
	sum3 += vec_perm(sum34, sum35, (vector unsigned char)vperm);
	sum4 += vec_perm(sum35, sum30, (vector unsigned char)vperm);
	sum5 += vec_perm(sum30, sum31, (vector unsigned char)vperm);


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
	vector double sum20 = { 0.0f, 0.0f};
	vector double sum21 = { 0.0f, 0.0f};
	vector double sum22 = { 0.0f, 0.0f};
	vector double sum23 = { 0.0f, 0.0f};
	vector double sum30 = { 0.0f, 0.0f};
	vector double sum31 = { 0.0f, 0.0f};
	vector double sum32 = { 0.0f, 0.0f};
	vector double sum33 = { 0.0f, 0.0f};
	vector float d0, d1, d2;
#if WORDS_BIGENDIAN
	vector unsigned int vsel1 = { 0x00000000, 0x00000000, 0x00000000, 0xFFFFFFFF };
	vector unsigned int vsel2 = { 0x00000000, 0x00000000, 0xFFFFFFFF, 0xFFFFFFFF };
	vector unsigned int vsel3 = { 0x00000000, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF };
	vector unsigned long long vperm = { 0x08090A0B0C0D0E0F, 0x1011121314151617 };
#else
	vector unsigned int vsel1 = { 0xFFFFFFFF, 0x00000000, 0x00000000, 0x00000000 };
	vector unsigned int vsel2 = { 0xFFFFFFFF, 0xFFFFFFFF, 0x00000000, 0x00000000 };
	vector unsigned int vsel3 = { 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0x00000000 };
	vector unsigned long long vperm = { 0x0F0E0D0C0B0A0908, 0x1716151413121110 };
#endif

	(void) lag;
	FLAC__ASSERT(lag <= 8);
	FLAC__ASSERT(lag <= data_len);

	base = data;

	d0 = vec_vsx_ld(0, base);
	d1 = vec_vsx_ld(16, base);

	base += 8;

	for (i = 0; i <= (limit-2); i += 4) {
		vector float d, mult, d0_orig = d0;

		d2 = vec_vsx_ld(0, base);
		base += 4;

		d = vec_splat(d0_orig, 0);
		mult = d0 * d;
		sum0 += vec_doubleh(mult);
		sum1 += vec_doublel(mult);
		mult = d1 * d;
		sum2 += vec_doubleh(mult);
		sum3 += vec_doublel(mult);

		d = vec_splat(d0_orig, 1);
		d0 = vec_sel(d0_orig, d2, vsel1);
		mult = d0 * d;
		sum10 += vec_doubleh(mult);
		sum11 += vec_doublel(mult);
		mult = d1 * d;
		sum12 += vec_doubleh(mult);
		sum13 += vec_doublel(mult);

		d = vec_splat(d0_orig, 2);
		d0 = vec_sel(d0_orig, d2, vsel2);
		mult = d0 * d;
		sum20 += vec_doubleh(mult);
		sum21 += vec_doublel(mult);
		mult = d1 * d;
		sum22 += vec_doubleh(mult);
		sum23 += vec_doublel(mult);

		d = vec_splat(d0_orig, 3);
		d0 = vec_sel(d0_orig, d2, vsel3);
		mult = d0 * d;
		sum30 += vec_doubleh(mult);
		sum31 += vec_doublel(mult);
		mult = d1 * d;
		sum32 += vec_doubleh(mult);
		sum33 += vec_doublel(mult);

		d0 = d1;
		d1 = d2;
	}

	sum0 += vec_perm(sum10, sum11, (vector unsigned char)vperm);
	sum1 += vec_perm(sum11, sum12, (vector unsigned char)vperm);
	sum2 += vec_perm(sum12, sum13, (vector unsigned char)vperm);
	sum3 += vec_perm(sum13, sum10, (vector unsigned char)vperm);

	sum0 += sum21;
	sum1 += sum22;
	sum2 += sum23;
	sum3 += sum20;

	sum0 += vec_perm(sum31, sum32, (vector unsigned char)vperm);
	sum1 += vec_perm(sum32, sum33, (vector unsigned char)vperm);
	sum2 += vec_perm(sum33, sum30, (vector unsigned char)vperm);
	sum3 += vec_perm(sum30, sum31, (vector unsigned char)vperm);


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

__attribute__((target("cpu=power8")))
void FLAC__lpc_compute_autocorrelation_intrin_power8_vsx_lag_4(const FLAC__real data[], uint32_t data_len, uint32_t lag, double autoc[])
{
	long i;
	long limit = (long)data_len - 4;
	const FLAC__real *base;
	vector double sum0 = { 0.0f, 0.0f};
	vector double sum1 = { 0.0f, 0.0f};
	vector double sum10 = { 0.0f, 0.0f};
	vector double sum11 = { 0.0f, 0.0f};
	vector double sum20 = { 0.0f, 0.0f};
	vector double sum21 = { 0.0f, 0.0f};
	vector double sum30 = { 0.0f, 0.0f};
	vector double sum31 = { 0.0f, 0.0f};
	vector float d0, d1;
#if WORDS_BIGENDIAN
	vector unsigned int vsel1 = { 0x00000000, 0x00000000, 0x00000000, 0xFFFFFFFF };
	vector unsigned int vsel2 = { 0x00000000, 0x00000000, 0xFFFFFFFF, 0xFFFFFFFF };
	vector unsigned int vsel3 = { 0x00000000, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF };
	vector unsigned long long vperm = { 0x08090A0B0C0D0E0F, 0x1011121314151617 };
#else
	vector unsigned int vsel1 = { 0xFFFFFFFF, 0x00000000, 0x00000000, 0x00000000 };
	vector unsigned int vsel2 = { 0xFFFFFFFF, 0xFFFFFFFF, 0x00000000, 0x00000000 };
	vector unsigned int vsel3 = { 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0x00000000 };
	vector unsigned long long vperm = { 0x0F0E0D0C0B0A0908, 0x1716151413121110 };
#endif

	(void) lag;
	FLAC__ASSERT(lag <= 4);
	FLAC__ASSERT(lag <= data_len);

	base = data;

	d0 = vec_vsx_ld(0, base);

	base += 4;

	for (i = 0; i <= (limit-1); i += 4) {
		vector float d, mult, d0_orig = d0;

		d1 = vec_vsx_ld(0, base);
		base += 4;

		d = vec_splat(d0_orig, 0);
		mult = d0 * d;
		sum0 += vec_doubleh(mult);
		sum1 += vec_doublel(mult);

		d = vec_splat(d0_orig, 1);
		d0 = vec_sel(d0_orig, d1, vsel1);
		mult = d0 * d;
		sum10 += vec_doubleh(mult);
		sum11 += vec_doublel(mult);

		d = vec_splat(d0_orig, 2);
		d0 = vec_sel(d0_orig, d1, vsel2);
		mult = d0 * d;
		sum20 += vec_doubleh(mult);
		sum21 += vec_doublel(mult);

		d = vec_splat(d0_orig, 3);
		d0 = vec_sel(d0_orig, d1, vsel3);
		mult = d0 * d;
		sum30 += vec_doubleh(mult);
		sum31 += vec_doublel(mult);

		d0 = d1;
	}
	sum0 += vec_perm(sum10, sum11, (vector unsigned char)vperm);
	sum1 += vec_perm(sum11, sum10, (vector unsigned char)vperm);

	sum0 += sum21;
	sum1 += sum20;

	sum0 += vec_perm(sum31, sum30, (vector unsigned char)vperm);
	sum1 += vec_perm(sum30, sum31, (vector unsigned char)vperm);


	vec_vsx_st(sum0, 0, autoc);
	vec_vsx_st(sum1, 16, autoc);

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
void FLAC__lpc_compute_autocorrelation_intrin_power9_vsx_lag_16(const FLAC__real data[], uint32_t data_len, uint32_t lag, double autoc[])
{
	long i;
	long limit = (long)data_len - 16;
	const FLAC__real *base;
	vector double sum0 = { 0.0f, 0.0f};
	vector double sum1 = { 0.0f, 0.0f};
	vector double sum2 = { 0.0f, 0.0f};
	vector double sum3 = { 0.0f, 0.0f};
	vector double sum4 = { 0.0f, 0.0f};
	vector double sum5 = { 0.0f, 0.0f};
	vector double sum6 = { 0.0f, 0.0f};
	vector double sum7 = { 0.0f, 0.0f};
	vector double sum10 = { 0.0f, 0.0f};
	vector double sum11 = { 0.0f, 0.0f};
	vector double sum12 = { 0.0f, 0.0f};
	vector double sum13 = { 0.0f, 0.0f};
	vector double sum14 = { 0.0f, 0.0f};
	vector double sum15 = { 0.0f, 0.0f};
	vector double sum16 = { 0.0f, 0.0f};
	vector double sum17 = { 0.0f, 0.0f};
	vector double sum20 = { 0.0f, 0.0f};
	vector double sum21 = { 0.0f, 0.0f};
	vector double sum22 = { 0.0f, 0.0f};
	vector double sum23 = { 0.0f, 0.0f};
	vector double sum24 = { 0.0f, 0.0f};
	vector double sum25 = { 0.0f, 0.0f};
	vector double sum26 = { 0.0f, 0.0f};
	vector double sum27 = { 0.0f, 0.0f};
	vector double sum30 = { 0.0f, 0.0f};
	vector double sum31 = { 0.0f, 0.0f};
	vector double sum32 = { 0.0f, 0.0f};
	vector double sum33 = { 0.0f, 0.0f};
	vector double sum34 = { 0.0f, 0.0f};
	vector double sum35 = { 0.0f, 0.0f};
	vector double sum36 = { 0.0f, 0.0f};
	vector double sum37 = { 0.0f, 0.0f};
	vector float d0, d1, d2, d3, d4;
#if WORDS_BIGENDIAN
	vector unsigned int vsel1 = { 0x00000000, 0x00000000, 0x00000000, 0xFFFFFFFF };
	vector unsigned int vsel2 = { 0x00000000, 0x00000000, 0xFFFFFFFF, 0xFFFFFFFF };
	vector unsigned int vsel3 = { 0x00000000, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF };
	vector unsigned long long vperm = { 0x08090A0B0C0D0E0F, 0x1011121314151617 };
#else
	vector unsigned int vsel1 = { 0xFFFFFFFF, 0x00000000, 0x00000000, 0x00000000 };
	vector unsigned int vsel2 = { 0xFFFFFFFF, 0xFFFFFFFF, 0x00000000, 0x00000000 };
	vector unsigned int vsel3 = { 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0x00000000 };
	vector unsigned long long vperm = { 0x0F0E0D0C0B0A0908, 0x1716151413121110 };
#endif

	(void) lag;
	FLAC__ASSERT(lag <= 16);
	FLAC__ASSERT(lag <= data_len);

	base = data;

	d0 = vec_vsx_ld(0, base);
	d1 = vec_vsx_ld(16, base);
	d2 = vec_vsx_ld(32, base);
	d3 = vec_vsx_ld(48, base);

	base += 16;

	for (i = 0; i <= (limit-4); i += 4) {
		vector float d, mult, d0_orig = d0;

		d4 = vec_vsx_ld(0, base);
		base += 4;

		d = vec_splat(d0_orig, 0);
		mult = d0 * d;
		sum0 += vec_doubleh(mult);
		sum1 += vec_doublel(mult);
		mult = d1 * d;
		sum2 += vec_doubleh(mult);
		sum3 += vec_doublel(mult);
		mult = d2 * d;
		sum4 += vec_doubleh(mult);
		sum5 += vec_doublel(mult);
		mult = d3 * d;
		sum6 += vec_doubleh(mult);
		sum7 += vec_doublel(mult);

		d = vec_splat(d0_orig, 1);
		d0 = vec_sel(d0_orig, d4, vsel1);
		mult = d0 * d;
		sum10 += vec_doubleh(mult);
		sum11 += vec_doublel(mult);
		mult = d1 * d;
		sum12 += vec_doubleh(mult);
		sum13 += vec_doublel(mult);
		mult = d2 * d;
		sum14 += vec_doubleh(mult);
		sum15 += vec_doublel(mult);
		mult = d3 * d;
		sum16 += vec_doubleh(mult);
		sum17 += vec_doublel(mult);

		d = vec_splat(d0_orig, 2);
		d0 = vec_sel(d0_orig, d4, vsel2);
		mult = d0 * d;
		sum20 += vec_doubleh(mult);
		sum21 += vec_doublel(mult);
		mult = d1 * d;
		sum22 += vec_doubleh(mult);
		sum23 += vec_doublel(mult);
		mult = d2 * d;
		sum24 += vec_doubleh(mult);
		sum25 += vec_doublel(mult);
		mult = d3 * d;
		sum26 += vec_doubleh(mult);
		sum27 += vec_doublel(mult);

		d = vec_splat(d0_orig, 3);
		d0 = vec_sel(d0_orig, d4, vsel3);
		mult = d0 * d;
		sum30 += vec_doubleh(mult);
		sum31 += vec_doublel(mult);
		mult = d1 * d;
		sum32 += vec_doubleh(mult);
		sum33 += vec_doublel(mult);
		mult = d2 * d;
		sum34 += vec_doubleh(mult);
		sum35 += vec_doublel(mult);
		mult = d3 * d;
		sum36 += vec_doubleh(mult);
		sum37 += vec_doublel(mult);

		d0 = d1;
		d1 = d2;
		d2 = d3;
		d3 = d4;
	}

	sum0 += vec_perm(sum10, sum11, (vector unsigned char)vperm);
	sum1 += vec_perm(sum11, sum12, (vector unsigned char)vperm);
	sum2 += vec_perm(sum12, sum13, (vector unsigned char)vperm);
	sum3 += vec_perm(sum13, sum14, (vector unsigned char)vperm);
	sum4 += vec_perm(sum14, sum15, (vector unsigned char)vperm);
	sum5 += vec_perm(sum15, sum16, (vector unsigned char)vperm);
	sum6 += vec_perm(sum16, sum17, (vector unsigned char)vperm);
	sum7 += vec_perm(sum17, sum10, (vector unsigned char)vperm);

	sum0 += sum21;
	sum1 += sum22;
	sum2 += sum23;
	sum3 += sum24;
	sum4 += sum25;
	sum5 += sum26;
	sum6 += sum27;
	sum7 += sum20;

	sum0 += vec_perm(sum31, sum32, (vector unsigned char)vperm);
	sum1 += vec_perm(sum32, sum33, (vector unsigned char)vperm);
	sum2 += vec_perm(sum33, sum34, (vector unsigned char)vperm);
	sum3 += vec_perm(sum34, sum35, (vector unsigned char)vperm);
	sum4 += vec_perm(sum35, sum36, (vector unsigned char)vperm);
	sum5 += vec_perm(sum36, sum37, (vector unsigned char)vperm);
	sum6 += vec_perm(sum37, sum30, (vector unsigned char)vperm);
	sum7 += vec_perm(sum30, sum31, (vector unsigned char)vperm);


	vec_vsx_st(sum0, 0, autoc);
	vec_vsx_st(sum1, 16, autoc);
	vec_vsx_st(sum2, 32, autoc);
	vec_vsx_st(sum3, 48, autoc);
	vec_vsx_st(sum4, 64, autoc);
	vec_vsx_st(sum5, 80, autoc);
	vec_vsx_st(sum6, 96, autoc);
	vec_vsx_st(sum7, 112, autoc);

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
	vector double sum20 = { 0.0f, 0.0f};
	vector double sum21 = { 0.0f, 0.0f};
	vector double sum22 = { 0.0f, 0.0f};
	vector double sum23 = { 0.0f, 0.0f};
	vector double sum24 = { 0.0f, 0.0f};
	vector double sum25 = { 0.0f, 0.0f};
	vector double sum30 = { 0.0f, 0.0f};
	vector double sum31 = { 0.0f, 0.0f};
	vector double sum32 = { 0.0f, 0.0f};
	vector double sum33 = { 0.0f, 0.0f};
	vector double sum34 = { 0.0f, 0.0f};
	vector double sum35 = { 0.0f, 0.0f};
	vector float d0, d1, d2, d3;
#if WORDS_BIGENDIAN
	vector unsigned int vsel1 = { 0x00000000, 0x00000000, 0x00000000, 0xFFFFFFFF };
	vector unsigned int vsel2 = { 0x00000000, 0x00000000, 0xFFFFFFFF, 0xFFFFFFFF };
	vector unsigned int vsel3 = { 0x00000000, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF };
	vector unsigned long long vperm = { 0x08090A0B0C0D0E0F, 0x1011121314151617 };
#else
	vector unsigned int vsel1 = { 0xFFFFFFFF, 0x00000000, 0x00000000, 0x00000000 };
	vector unsigned int vsel2 = { 0xFFFFFFFF, 0xFFFFFFFF, 0x00000000, 0x00000000 };
	vector unsigned int vsel3 = { 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0x00000000 };
	vector unsigned long long vperm = { 0x0F0E0D0C0B0A0908, 0x1716151413121110 };
#endif

	(void) lag;
	FLAC__ASSERT(lag <= 12);
	FLAC__ASSERT(lag <= data_len);

	base = data;

	d0 = vec_vsx_ld(0, base);
	d1 = vec_vsx_ld(16, base);
	d2 = vec_vsx_ld(32, base);

	base += 12;

	for (i = 0; i <= (limit-3); i += 4) {
		vector float d, mult, d0_orig = d0;

		d3 = vec_vsx_ld(0, base);
		base += 4;

		d = vec_splat(d0_orig, 0);
		mult = d0 * d;
		sum0 += vec_doubleh(mult);
		sum1 += vec_doublel(mult);
		mult = d1 * d;
		sum2 += vec_doubleh(mult);
		sum3 += vec_doublel(mult);
		mult = d2 * d;
		sum4 += vec_doubleh(mult);
		sum5 += vec_doublel(mult);

		d = vec_splat(d0_orig, 1);
		d0 = vec_sel(d0_orig, d3, vsel1);
		mult = d0 * d;
		sum10 += vec_doubleh(mult);
		sum11 += vec_doublel(mult);
		mult = d1 * d;
		sum12 += vec_doubleh(mult);
		sum13 += vec_doublel(mult);
		mult = d2 * d;
		sum14 += vec_doubleh(mult);
		sum15 += vec_doublel(mult);

		d = vec_splat(d0_orig, 2);
		d0 = vec_sel(d0_orig, d3, vsel2);
		mult = d0 * d;
		sum20 += vec_doubleh(mult);
		sum21 += vec_doublel(mult);
		mult = d1 * d;
		sum22 += vec_doubleh(mult);
		sum23 += vec_doublel(mult);
		mult = d2 * d;
		sum24 += vec_doubleh(mult);
		sum25 += vec_doublel(mult);

		d = vec_splat(d0_orig, 3);
		d0 = vec_sel(d0_orig, d3, vsel3);
		mult = d0 * d;
		sum30 += vec_doubleh(mult);
		sum31 += vec_doublel(mult);
		mult = d1 * d;
		sum32 += vec_doubleh(mult);
		sum33 += vec_doublel(mult);
		mult = d2 * d;
		sum34 += vec_doubleh(mult);
		sum35 += vec_doublel(mult);

		d0 = d1;
		d1 = d2;
		d2 = d3;
	}

	sum0 += vec_perm(sum10, sum11, (vector unsigned char)vperm);
	sum1 += vec_perm(sum11, sum12, (vector unsigned char)vperm);
	sum2 += vec_perm(sum12, sum13, (vector unsigned char)vperm);
	sum3 += vec_perm(sum13, sum14, (vector unsigned char)vperm);
	sum4 += vec_perm(sum14, sum15, (vector unsigned char)vperm);
	sum5 += vec_perm(sum15, sum10, (vector unsigned char)vperm);

	sum0 += sum21;
	sum1 += sum22;
	sum2 += sum23;
	sum3 += sum24;
	sum4 += sum25;
	sum5 += sum20;

	sum0 += vec_perm(sum31, sum32, (vector unsigned char)vperm);
	sum1 += vec_perm(sum32, sum33, (vector unsigned char)vperm);
	sum2 += vec_perm(sum33, sum34, (vector unsigned char)vperm);
	sum3 += vec_perm(sum34, sum35, (vector unsigned char)vperm);
	sum4 += vec_perm(sum35, sum30, (vector unsigned char)vperm);
	sum5 += vec_perm(sum30, sum31, (vector unsigned char)vperm);


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
	vector double sum20 = { 0.0f, 0.0f};
	vector double sum21 = { 0.0f, 0.0f};
	vector double sum22 = { 0.0f, 0.0f};
	vector double sum23 = { 0.0f, 0.0f};
	vector double sum30 = { 0.0f, 0.0f};
	vector double sum31 = { 0.0f, 0.0f};
	vector double sum32 = { 0.0f, 0.0f};
	vector double sum33 = { 0.0f, 0.0f};
	vector float d0, d1, d2;
#if WORDS_BIGENDIAN
	vector unsigned int vsel1 = { 0x00000000, 0x00000000, 0x00000000, 0xFFFFFFFF };
	vector unsigned int vsel2 = { 0x00000000, 0x00000000, 0xFFFFFFFF, 0xFFFFFFFF };
	vector unsigned int vsel3 = { 0x00000000, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF };
	vector unsigned long long vperm = { 0x08090A0B0C0D0E0F, 0x1011121314151617 };
#else
	vector unsigned int vsel1 = { 0xFFFFFFFF, 0x00000000, 0x00000000, 0x00000000 };
	vector unsigned int vsel2 = { 0xFFFFFFFF, 0xFFFFFFFF, 0x00000000, 0x00000000 };
	vector unsigned int vsel3 = { 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0x00000000 };
	vector unsigned long long vperm = { 0x0F0E0D0C0B0A0908, 0x1716151413121110 };
#endif

	(void) lag;
	FLAC__ASSERT(lag <= 8);
	FLAC__ASSERT(lag <= data_len);

	base = data;

	d0 = vec_vsx_ld(0, base);
	d1 = vec_vsx_ld(16, base);

	base += 8;

	for (i = 0; i <= (limit-2); i += 4) {
		vector float d, mult, d0_orig = d0;

		d2 = vec_vsx_ld(0, base);
		base += 4;

		d = vec_splat(d0_orig, 0);
		mult = d0 * d;
		sum0 += vec_doubleh(mult);
		sum1 += vec_doublel(mult);
		mult = d1 * d;
		sum2 += vec_doubleh(mult);
		sum3 += vec_doublel(mult);

		d = vec_splat(d0_orig, 1);
		d0 = vec_sel(d0_orig, d2, vsel1);
		mult = d0 * d;
		sum10 += vec_doubleh(mult);
		sum11 += vec_doublel(mult);
		mult = d1 * d;
		sum12 += vec_doubleh(mult);
		sum13 += vec_doublel(mult);

		d = vec_splat(d0_orig, 2);
		d0 = vec_sel(d0_orig, d2, vsel2);
		mult = d0 * d;
		sum20 += vec_doubleh(mult);
		sum21 += vec_doublel(mult);
		mult = d1 * d;
		sum22 += vec_doubleh(mult);
		sum23 += vec_doublel(mult);

		d = vec_splat(d0_orig, 3);
		d0 = vec_sel(d0_orig, d2, vsel3);
		mult = d0 * d;
		sum30 += vec_doubleh(mult);
		sum31 += vec_doublel(mult);
		mult = d1 * d;
		sum32 += vec_doubleh(mult);
		sum33 += vec_doublel(mult);

		d0 = d1;
		d1 = d2;
	}

	sum0 += vec_perm(sum10, sum11, (vector unsigned char)vperm);
	sum1 += vec_perm(sum11, sum12, (vector unsigned char)vperm);
	sum2 += vec_perm(sum12, sum13, (vector unsigned char)vperm);
	sum3 += vec_perm(sum13, sum10, (vector unsigned char)vperm);

	sum0 += sum21;
	sum1 += sum22;
	sum2 += sum23;
	sum3 += sum20;

	sum0 += vec_perm(sum31, sum32, (vector unsigned char)vperm);
	sum1 += vec_perm(sum32, sum33, (vector unsigned char)vperm);
	sum2 += vec_perm(sum33, sum30, (vector unsigned char)vperm);
	sum3 += vec_perm(sum30, sum31, (vector unsigned char)vperm);


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

__attribute__((target("cpu=power9")))
void FLAC__lpc_compute_autocorrelation_intrin_power9_vsx_lag_4(const FLAC__real data[], uint32_t data_len, uint32_t lag, double autoc[])
{
	long i;
	long limit = (long)data_len - 4;
	const FLAC__real *base;
	vector double sum0 = { 0.0f, 0.0f};
	vector double sum1 = { 0.0f, 0.0f};
	vector double sum10 = { 0.0f, 0.0f};
	vector double sum11 = { 0.0f, 0.0f};
	vector double sum20 = { 0.0f, 0.0f};
	vector double sum21 = { 0.0f, 0.0f};
	vector double sum30 = { 0.0f, 0.0f};
	vector double sum31 = { 0.0f, 0.0f};
	vector float d0, d1;
#if WORDS_BIGENDIAN
	vector unsigned int vsel1 = { 0x00000000, 0x00000000, 0x00000000, 0xFFFFFFFF };
	vector unsigned int vsel2 = { 0x00000000, 0x00000000, 0xFFFFFFFF, 0xFFFFFFFF };
	vector unsigned int vsel3 = { 0x00000000, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF };
	vector unsigned long long vperm = { 0x08090A0B0C0D0E0F, 0x1011121314151617 };
#else
	vector unsigned int vsel1 = { 0xFFFFFFFF, 0x00000000, 0x00000000, 0x00000000 };
	vector unsigned int vsel2 = { 0xFFFFFFFF, 0xFFFFFFFF, 0x00000000, 0x00000000 };
	vector unsigned int vsel3 = { 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0x00000000 };
	vector unsigned long long vperm = { 0x0F0E0D0C0B0A0908, 0x1716151413121110 };
#endif

	(void) lag;
	FLAC__ASSERT(lag <= 4);
	FLAC__ASSERT(lag <= data_len);

	base = data;

	d0 = vec_vsx_ld(0, base);

	base += 4;

	for (i = 0; i <= (limit-1); i += 4) {
		vector float d, mult, d0_orig = d0;

		d1 = vec_vsx_ld(0, base);
		base += 4;

		d = vec_splat(d0_orig, 0);
		mult = d0 * d;
		sum0 += vec_doubleh(mult);
		sum1 += vec_doublel(mult);

		d = vec_splat(d0_orig, 1);
		d0 = vec_sel(d0_orig, d1, vsel1);
		mult = d0 * d;
		sum10 += vec_doubleh(mult);
		sum11 += vec_doublel(mult);

		d = vec_splat(d0_orig, 2);
		d0 = vec_sel(d0_orig, d1, vsel2);
		mult = d0 * d;
		sum20 += vec_doubleh(mult);
		sum21 += vec_doublel(mult);

		d = vec_splat(d0_orig, 3);
		d0 = vec_sel(d0_orig, d1, vsel3);
		mult = d0 * d;
		sum30 += vec_doubleh(mult);
		sum31 += vec_doublel(mult);

		d0 = d1;
	}
	sum0 += vec_perm(sum10, sum11, (vector unsigned char)vperm);
	sum1 += vec_perm(sum11, sum10, (vector unsigned char)vperm);

	sum0 += sum21;
	sum1 += sum20;

	sum0 += vec_perm(sum31, sum30, (vector unsigned char)vperm);
	sum1 += vec_perm(sum30, sum31, (vector unsigned char)vperm);


	vec_vsx_st(sum0, 0, autoc);
	vec_vsx_st(sum1, 16, autoc);

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
