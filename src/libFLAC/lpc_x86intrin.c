/* libFLAC - Free Lossless Audio Codec library
 * Copyright (C) 2000-2009  Josh Coalson
 * Copyright (C) 2011-2013  Xiph.Org Foundation
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

#ifndef FLAC__INTEGER_ONLY_LIBRARY
#ifndef FLAC__NO_ASM
#if defined FLAC__CPU_IA32 || defined FLAC__CPU_X86_64
#ifdef FLAC__HAS_X86INTRIN

#if HAVE_CONFIG_H
#  include <config.h>
#endif

#include "FLAC/assert.h"
#include "FLAC/format.h"
#include "private/lpc.h"

#include <emmintrin.h> /* SSE2 */

void FLAC__lpc_compute_autocorrelation_intrin_sse_lag_4(const FLAC__real data[], unsigned data_len, unsigned lag, FLAC__real autoc[])
{
	__m128 xmm0, xmm2, xmm5;

	FLAC__ASSERT(lag > 0);
	FLAC__ASSERT(lag <= 4);
	FLAC__ASSERT(lag <= data_len);
	FLAC__ASSERT(data_len > 0);

	xmm5 = _mm_setzero_ps();

	xmm0 = _mm_load_ss(data++);
	xmm2 = xmm0;
	xmm0 = _mm_shuffle_ps(xmm0, xmm0, 0);

	xmm0 = _mm_mul_ps(xmm0, xmm2);
	xmm5 = _mm_add_ps(xmm5, xmm0);

	data_len--;

	while(data_len)
	{
		xmm0 = _mm_load1_ps(data++);

		xmm2 = _mm_shuffle_ps(xmm2, xmm2, _MM_SHUFFLE(2,1,0,3));
		xmm2 = _mm_move_ss(xmm2, xmm0);
		xmm0 = _mm_mul_ps(xmm0, xmm2);
		xmm5 = _mm_add_ps(xmm5, xmm0);

		data_len--;
	}

	_mm_storeu_ps(autoc, xmm5);
}

void FLAC__lpc_compute_autocorrelation_intrin_sse_lag_8(const FLAC__real data[], unsigned data_len, unsigned lag, FLAC__real autoc[])
{
	__m128 xmm0, xmm1, xmm2, xmm3, xmm5, xmm6;

	FLAC__ASSERT(lag > 0);
	FLAC__ASSERT(lag <= 8);
	FLAC__ASSERT(lag <= data_len);
	FLAC__ASSERT(data_len > 0);

	xmm5 = _mm_setzero_ps();
	xmm6 = _mm_setzero_ps();

	xmm0 = _mm_load_ss(data++);
	xmm2 = xmm0;
	xmm0 = _mm_shuffle_ps(xmm0, xmm0, 0);
	xmm3 = _mm_setzero_ps();

	xmm0 = _mm_mul_ps(xmm0, xmm2);
	xmm5 = _mm_add_ps(xmm5, xmm0);

	data_len--;

	while(data_len) /* see /src/libFLAC/ia32/lpc_asm.nasm */
	{
		xmm0 = _mm_load1_ps(data++);

		xmm2 = _mm_shuffle_ps(xmm2, xmm2, _MM_SHUFFLE(2,1,0,3));
		xmm3 = _mm_shuffle_ps(xmm3, xmm3, _MM_SHUFFLE(2,1,0,3));

		xmm3 = _mm_move_ss(xmm3, xmm2);
		xmm1 = xmm0;
		xmm2 = _mm_move_ss(xmm2, xmm0);

		xmm1 = _mm_mul_ps(xmm1, xmm3);
		xmm0 = _mm_mul_ps(xmm0, xmm2);
		xmm6 = _mm_add_ps(xmm6, xmm1);
		xmm5 = _mm_add_ps(xmm5, xmm0);

		data_len--;
	}

	_mm_storeu_ps(autoc,   xmm5);
	_mm_storeu_ps(autoc+4, xmm6);
}

void FLAC__lpc_compute_autocorrelation_intrin_sse_lag_12(const FLAC__real data[], unsigned data_len, unsigned lag, FLAC__real autoc[])
{
	__m128 xmm0, xmm1, xmm2, xmm3, xmm4, xmm5, xmm6, xmm7;

	FLAC__ASSERT(lag > 0);
	FLAC__ASSERT(lag <= 12);
	FLAC__ASSERT(lag <= data_len);
	FLAC__ASSERT(data_len > 0);

	xmm5 = _mm_setzero_ps();
	xmm6 = _mm_setzero_ps();
	xmm7 = _mm_setzero_ps();

	xmm0 = _mm_load_ss(data++);
	xmm2 = xmm0;
	xmm0 = _mm_shuffle_ps(xmm0, xmm0, 0);
	xmm3 = _mm_setzero_ps();
	xmm4 = _mm_setzero_ps();

	xmm0 = _mm_mul_ps(xmm0, xmm2);
	xmm5 = _mm_add_ps(xmm5, xmm0);

	data_len--;

	while(data_len)
	{
		xmm0 = _mm_load1_ps(data++);

		xmm2 = _mm_shuffle_ps(xmm2, xmm2, _MM_SHUFFLE(2,1,0,3));
		xmm3 = _mm_shuffle_ps(xmm3, xmm3, _MM_SHUFFLE(2,1,0,3));
		xmm4 = _mm_shuffle_ps(xmm4, xmm4, _MM_SHUFFLE(2,1,0,3));
		xmm4 = _mm_move_ss(xmm4, xmm3);
		xmm3 = _mm_move_ss(xmm3, xmm2);
		xmm2 = _mm_move_ss(xmm2, xmm0);

		xmm1 = xmm0;
		xmm1 = _mm_mul_ps(xmm1, xmm2);
		xmm5 = _mm_add_ps(xmm5, xmm1);
		xmm1 = xmm0;
		xmm1 = _mm_mul_ps(xmm1, xmm3);
		xmm6 = _mm_add_ps(xmm6, xmm1);
		xmm0 = _mm_mul_ps(xmm0, xmm4);
		xmm7 = _mm_add_ps(xmm7, xmm0);

		data_len--;
	}

	_mm_storeu_ps(autoc,   xmm5);
	_mm_storeu_ps(autoc+4, xmm6);
	_mm_storeu_ps(autoc+8, xmm7);
}

void FLAC__lpc_compute_autocorrelation_intrin_sse_lag_16(const FLAC__real data[], unsigned data_len, unsigned lag, FLAC__real autoc[])
{
	__m128 xmm0, xmm1, xmm2, xmm3, xmm4, xmm5, xmm6, xmm7, xmm8, xmm9;

	FLAC__ASSERT(lag > 0);
	FLAC__ASSERT(lag <= 16);
	FLAC__ASSERT(lag <= data_len);
	FLAC__ASSERT(data_len > 0);

	xmm6 = _mm_setzero_ps();
	xmm7 = _mm_setzero_ps();
	xmm8 = _mm_setzero_ps();
	xmm9 = _mm_setzero_ps();

	xmm0 = _mm_load_ss(data++);
	xmm2 = xmm0;
	xmm0 = _mm_shuffle_ps(xmm0, xmm0, 0);
	xmm3 = _mm_setzero_ps();
	xmm4 = _mm_setzero_ps();
	xmm5 = _mm_setzero_ps();

	xmm0 = _mm_mul_ps(xmm0, xmm2);
	xmm6 = _mm_add_ps(xmm6, xmm0);

	data_len--;

	while(data_len)
	{
		xmm0 = _mm_load1_ps(data++);

		/* shift xmm5:xmm4:xmm3:xmm2 left by one float */
		xmm5 = _mm_shuffle_ps(xmm5, xmm5, _MM_SHUFFLE(2,1,0,3));
		xmm4 = _mm_shuffle_ps(xmm4, xmm4, _MM_SHUFFLE(2,1,0,3));
		xmm3 = _mm_shuffle_ps(xmm3, xmm3, _MM_SHUFFLE(2,1,0,3));
		xmm2 = _mm_shuffle_ps(xmm2, xmm2, _MM_SHUFFLE(2,1,0,3));
		xmm5 = _mm_move_ss(xmm5, xmm4);
		xmm4 = _mm_move_ss(xmm4, xmm3);
		xmm3 = _mm_move_ss(xmm3, xmm2);
		xmm2 = _mm_move_ss(xmm2, xmm0);

		/* xmm9|xmm8|xmm7|xmm6 += xmm0|xmm0|xmm0|xmm0 * xmm5|xmm4|xmm3|xmm2 */
		xmm1 = xmm0;
		xmm1 = _mm_mul_ps(xmm1, xmm5);
		xmm9 = _mm_add_ps(xmm9, xmm1);
		xmm1 = xmm0;
		xmm1 = _mm_mul_ps(xmm1, xmm4);
		xmm8 = _mm_add_ps(xmm8, xmm1);
		xmm1 = xmm0;
		xmm1 = _mm_mul_ps(xmm1, xmm3);
		xmm7 = _mm_add_ps(xmm7, xmm1);
		xmm0 = _mm_mul_ps(xmm0, xmm2);
		xmm6 = _mm_add_ps(xmm6, xmm0);

		data_len--;
	}

	_mm_storeu_ps(autoc,   xmm6);
	_mm_storeu_ps(autoc+4, xmm7);
	_mm_storeu_ps(autoc+8, xmm8);
	_mm_storeu_ps(autoc+12,xmm9);
}

void FLAC__lpc_compute_residual_from_qlp_coefficients_16_intrin_sse2(const FLAC__int32 *data, unsigned data_len, const FLAC__int32 qlp_coeff[], unsigned order, int lp_quantization, FLAC__int32 residual[])
{
	int i;
	FLAC__int32 sum;

	FLAC__ASSERT(order > 0);
	FLAC__ASSERT(order <= 32);
	FLAC__ASSERT(data_len > 0);

	if(order <= 12) {
		FLAC__int32 curr;
		if(order > 8) { /* order == 9, 10, 11, 12 */   /* can be modified to work with order <= 15 but the subset limit is 12 */
			__m128i xmm0, xmm1, xmm2, xmm3, xmm4, xmm5, xmm6, xmm7;
			xmm0 = _mm_loadu_si128((const __m128i*)(qlp_coeff+0));
			xmm6 = _mm_loadu_si128((const __m128i*)(qlp_coeff+4));
			xmm1 = _mm_loadu_si128((const __m128i*)(qlp_coeff+8)); /* read 0 to 3 uninitialized coeffs... */
			switch(order)                                          /* ...and zero them out */
			{
			case 9:
				xmm1 = _mm_slli_si128(xmm1, 12); xmm1 = _mm_srli_si128(xmm1, 12);
				break;
			case 10:
				xmm1 = _mm_slli_si128(xmm1, 8); xmm1 = _mm_srli_si128(xmm1, 8);
				break;
			case 11:
				xmm1 = _mm_slli_si128(xmm1, 4); xmm1 = _mm_srli_si128(xmm1, 4);
				break;
			}
			xmm2 = _mm_setzero_si128();
			xmm0 = _mm_packs_epi32(xmm0, xmm6);
			xmm1 = _mm_packs_epi32(xmm1, xmm2);

			xmm4 = _mm_loadu_si128((const __m128i*)(data-12));
			xmm5 = _mm_loadu_si128((const __m128i*)(data-8));
			xmm3 = _mm_loadu_si128((const __m128i*)(data-4));
			xmm4 = _mm_shuffle_epi32(xmm4, _MM_SHUFFLE(0,1,2,3));
			xmm5 = _mm_shuffle_epi32(xmm5, _MM_SHUFFLE(0,1,2,3));
			xmm3 = _mm_shuffle_epi32(xmm3, _MM_SHUFFLE(0,1,2,3));
			xmm4 = _mm_packs_epi32(xmm4, xmm2);
			xmm3 = _mm_packs_epi32(xmm3, xmm5);

			xmm7 = _mm_slli_si128(xmm1, 2);
			xmm7 = _mm_or_si128(xmm7, _mm_srli_si128(xmm0, 14));
			xmm2 = _mm_slli_si128(xmm0, 2);

			/* xmm0, xmm1: qlp_coeff
			   xmm2, xmm7: qlp_coeff << 16bit
			   xmm3, xmm4: data */

			xmm5 = xmm4;
			xmm5 = _mm_madd_epi16(xmm5, xmm1);
			xmm6 = xmm3;
			xmm6 = _mm_madd_epi16(xmm6, xmm0);
			xmm6 = _mm_add_epi32(xmm6, xmm5);
			xmm6 = _mm_add_epi32(xmm6, _mm_srli_si128(xmm6, 8));
			xmm6 = _mm_add_epi32(xmm6, _mm_srli_si128(xmm6, 4));

			curr = *data++;
			*residual++ = curr - (_mm_cvtsi128_si32(xmm6) >> lp_quantization);

			data_len--;

			if(data_len & 1)
			{
				xmm4 = _mm_slli_si128(xmm4, 2);
				xmm6 = xmm3;
				xmm3 = _mm_slli_si128(xmm3, 2);
				xmm4 = _mm_or_si128(xmm4, _mm_srli_si128(xmm6, 14));
				xmm3 = _mm_insert_epi16(xmm3, curr, 0);

				xmm5 = xmm4;
				xmm5 = _mm_madd_epi16(xmm5, xmm1);
				xmm6 = xmm3;
				xmm6 = _mm_madd_epi16(xmm6, xmm0);
				xmm6 = _mm_add_epi32(xmm6, xmm5);
				xmm6 = _mm_add_epi32(xmm6, _mm_srli_si128(xmm6, 8));
				xmm6 = _mm_add_epi32(xmm6, _mm_srli_si128(xmm6, 4));

				curr = *data++;
				*residual++ = curr - (_mm_cvtsi128_si32(xmm6) >> lp_quantization);

				data_len--;
			}

			while(data_len) { /* data_len is even */
				/* 2 shifts per 2 cycles less + 2x loop unwind, but we need shifted qlp_coeff in xmm2:xmm7 */
				xmm4 = _mm_slli_si128(xmm4, 4);
				xmm6 = xmm3;
				xmm3 = _mm_slli_si128(xmm3, 4);
				xmm4 = _mm_or_si128(xmm4, _mm_srli_si128(xmm6, 12));
				xmm3 = _mm_insert_epi16(xmm3, curr, 1);

				xmm5 = xmm4;
				xmm5 = _mm_madd_epi16(xmm5, xmm7);
				xmm6 = xmm3;
				xmm6 = _mm_madd_epi16(xmm6, xmm2);
				xmm6 = _mm_add_epi32(xmm6, xmm5);
				xmm6 = _mm_add_epi32(xmm6, _mm_srli_si128(xmm6, 8));
				xmm6 = _mm_add_epi32(xmm6, _mm_srli_si128(xmm6, 4));

				curr = *data++;
				*residual++ = curr - (_mm_cvtsi128_si32(xmm6) >> lp_quantization);

				xmm3 = _mm_insert_epi16(xmm3, curr, 0);
				xmm5 = xmm4;
				xmm5 = _mm_madd_epi16(xmm5, xmm1);
				xmm6 = xmm3;
				xmm6 = _mm_madd_epi16(xmm6, xmm0);
				xmm6 = _mm_add_epi32(xmm6, xmm5);
				xmm6 = _mm_add_epi32(xmm6, _mm_srli_si128(xmm6, 8));
				xmm6 = _mm_add_epi32(xmm6, _mm_srli_si128(xmm6, 4));

				curr = *data++;
				*residual++ = curr - (_mm_cvtsi128_si32(xmm6) >> lp_quantization);

				data_len-=2;
			}
		}
		else if(order > 4) { /* order == 5, 6, 7, 8 */
			if(order == 8) {
				__m128i xmm0, xmm1, xmm3, xmm6;
				xmm0 = _mm_loadu_si128((const __m128i*)(qlp_coeff+0));
				xmm1 = _mm_loadu_si128((const __m128i*)(qlp_coeff+4));
				xmm0 = _mm_packs_epi32(xmm0, xmm1);

				xmm1 = _mm_loadu_si128((const __m128i*)(data-8));
				xmm3 = _mm_loadu_si128((const __m128i*)(data-4));
				xmm1 = _mm_shuffle_epi32(xmm1, _MM_SHUFFLE(0,1,2,3));
				xmm3 = _mm_shuffle_epi32(xmm3, _MM_SHUFFLE(0,1,2,3));
				xmm3 = _mm_packs_epi32(xmm3, xmm1);

				/* xmm0: qlp_coeff
				   xmm3: data */

				xmm6 = xmm3;
				xmm6 = _mm_madd_epi16(xmm6, xmm0);
				xmm6 = _mm_add_epi32(xmm6, _mm_srli_si128(xmm6, 8));
				xmm6 = _mm_add_epi32(xmm6, _mm_srli_si128(xmm6, 4));

				curr = *data++;
				*residual++ = curr - (_mm_cvtsi128_si32(xmm6) >> lp_quantization);

				data_len--;

				while(data_len) {
					xmm3 = _mm_slli_si128(xmm3, 2);
					xmm3 = _mm_insert_epi16(xmm3, curr, 0);

					xmm6 = xmm3;
					xmm6 = _mm_madd_epi16(xmm6, xmm0);
					xmm6 = _mm_add_epi32(xmm6, _mm_srli_si128(xmm6, 8));
					xmm6 = _mm_add_epi32(xmm6, _mm_srli_si128(xmm6, 4));

					curr = *data++;
					*residual++ = curr - (_mm_cvtsi128_si32(xmm6) >> lp_quantization);

					data_len--;
				}
			}
			else { /* order == 5, 6, 7 */
				__m128i xmm0, xmm1, xmm2, xmm3, xmm6;
				xmm0 = _mm_loadu_si128((const __m128i*)(qlp_coeff+0));
				xmm1 = _mm_loadu_si128((const __m128i*)(qlp_coeff+4));
				switch(order)
				{
				case 5:
					xmm1 = _mm_slli_si128(xmm1, 12); xmm1 = _mm_srli_si128(xmm1, 12);
					break;
				case 6:
					xmm1 = _mm_slli_si128(xmm1, 8); xmm1 = _mm_srli_si128(xmm1, 8);
					break;
				case 7:
					xmm1 = _mm_slli_si128(xmm1, 4); xmm1 = _mm_srli_si128(xmm1, 4);
					break;
				}
				xmm0 = _mm_packs_epi32(xmm0, xmm1);

				xmm1 = _mm_loadu_si128((const __m128i*)(data-8));
				xmm3 = _mm_loadu_si128((const __m128i*)(data-4));
				xmm1 = _mm_shuffle_epi32(xmm1, _MM_SHUFFLE(0,1,2,3));
				xmm3 = _mm_shuffle_epi32(xmm3, _MM_SHUFFLE(0,1,2,3));
				xmm3 = _mm_packs_epi32(xmm3, xmm1);
				xmm2 = _mm_slli_si128(xmm0, 2);

				/* xmm0: qlp_coeff
				   xmm2: qlp_coeff << 16bit
				   xmm3: data */

				xmm6 = xmm3;
				xmm6 = _mm_madd_epi16(xmm6, xmm0);
				xmm6 = _mm_add_epi32(xmm6, _mm_srli_si128(xmm6, 8));
				xmm6 = _mm_add_epi32(xmm6, _mm_srli_si128(xmm6, 4));

				curr = *data++;
				*residual++ = curr - (_mm_cvtsi128_si32(xmm6) >> lp_quantization);

				data_len--;

				if(data_len & 1)
				{
					xmm3 = _mm_slli_si128(xmm3, 2);
					xmm3 = _mm_insert_epi16(xmm3, curr, 0);

					xmm6 = xmm3;
					xmm6 = _mm_madd_epi16(xmm6, xmm0);
					xmm6 = _mm_add_epi32(xmm6, _mm_srli_si128(xmm6, 8));
					xmm6 = _mm_add_epi32(xmm6, _mm_srli_si128(xmm6, 4));

					curr = *data++;
					*residual++ = curr - (_mm_cvtsi128_si32(xmm6) >> lp_quantization);

					data_len--;
				}

				while(data_len) { /* data_len is even */
					xmm3 = _mm_slli_si128(xmm3, 4);
					xmm3 = _mm_insert_epi16(xmm3, curr, 1);

					xmm6 = xmm3;
					xmm6 = _mm_madd_epi16(xmm6, xmm2);
					xmm6 = _mm_add_epi32(xmm6, _mm_srli_si128(xmm6, 8));
					xmm6 = _mm_add_epi32(xmm6, _mm_srli_si128(xmm6, 4));

					curr = *data++;
					*residual++ = curr - (_mm_cvtsi128_si32(xmm6) >> lp_quantization);

					xmm3 = _mm_insert_epi16(xmm3, curr, 0);
					xmm6 = xmm3;
					xmm6 = _mm_madd_epi16(xmm6, xmm0);
					xmm6 = _mm_add_epi32(xmm6, _mm_srli_si128(xmm6, 8));
					xmm6 = _mm_add_epi32(xmm6, _mm_srli_si128(xmm6, 4));

					curr = *data++;
					*residual++ = curr - (_mm_cvtsi128_si32(xmm6) >> lp_quantization);

					data_len-=2;
				}
			}
		}
		else { /* order == 1, 2, 3, 4 */
			if(order > 2) {
				if(order == 4) {
					for(i = 0; i < (int)data_len; i++) {
						sum = 0;
						sum += qlp_coeff[3] * data[i-4];
						sum += qlp_coeff[2] * data[i-3];
						sum += qlp_coeff[1] * data[i-2];
						sum += qlp_coeff[0] * data[i-1];
						residual[i] = data[i] - (sum >> lp_quantization);
					}
				}
				else { /* order == 3 */
					for(i = 0; i < (int)data_len; i++) {
						sum = 0;
						sum += qlp_coeff[2] * data[i-3];
						sum += qlp_coeff[1] * data[i-2];
						sum += qlp_coeff[0] * data[i-1];
						residual[i] = data[i] - (sum >> lp_quantization);
					}
				}
			}
			else {
				if(order == 2) {
					for(i = 0; i < (int)data_len; i++) {
						sum = 0;
						sum += qlp_coeff[1] * data[i-2];
						sum += qlp_coeff[0] * data[i-1];
						residual[i] = data[i] - (sum >> lp_quantization);
					}
				}
				else { /* order == 1 */
					for(i = 0; i < (int)data_len; i++)
						residual[i] = data[i] - ((qlp_coeff[0] * data[i-1]) >> lp_quantization);
				}
			}
		}
	}
	else { /* order > 12 */
		for(i = 0; i < (int)data_len; i++) {
			sum = 0;
			switch(order) {
				case 32: sum += qlp_coeff[31] * data[i-32];
				case 31: sum += qlp_coeff[30] * data[i-31];
				case 30: sum += qlp_coeff[29] * data[i-30];
				case 29: sum += qlp_coeff[28] * data[i-29];
				case 28: sum += qlp_coeff[27] * data[i-28];
				case 27: sum += qlp_coeff[26] * data[i-27];
				case 26: sum += qlp_coeff[25] * data[i-26];
				case 25: sum += qlp_coeff[24] * data[i-25];
				case 24: sum += qlp_coeff[23] * data[i-24];
				case 23: sum += qlp_coeff[22] * data[i-23];
				case 22: sum += qlp_coeff[21] * data[i-22];
				case 21: sum += qlp_coeff[20] * data[i-21];
				case 20: sum += qlp_coeff[19] * data[i-20];
				case 19: sum += qlp_coeff[18] * data[i-19];
				case 18: sum += qlp_coeff[17] * data[i-18];
				case 17: sum += qlp_coeff[16] * data[i-17];
				case 16: sum += qlp_coeff[15] * data[i-16];
				case 15: sum += qlp_coeff[14] * data[i-15];
				case 14: sum += qlp_coeff[13] * data[i-14];
				case 13: sum += qlp_coeff[12] * data[i-13];
				         sum += qlp_coeff[11] * data[i-12];
				         sum += qlp_coeff[10] * data[i-11];
				         sum += qlp_coeff[ 9] * data[i-10];
				         sum += qlp_coeff[ 8] * data[i- 9];
				         sum += qlp_coeff[ 7] * data[i- 8];
				         sum += qlp_coeff[ 6] * data[i- 7];
				         sum += qlp_coeff[ 5] * data[i- 6];
				         sum += qlp_coeff[ 4] * data[i- 5];
				         sum += qlp_coeff[ 3] * data[i- 4];
				         sum += qlp_coeff[ 2] * data[i- 3];
				         sum += qlp_coeff[ 1] * data[i- 2];
				         sum += qlp_coeff[ 0] * data[i- 1];
			}
			residual[i] = data[i] - (sum >> lp_quantization);
		}
	}
}

#endif /* FLAC__HAS_X86INTRIN */
#endif /* FLAC__CPU_IA32 || FLAC__CPU_X86_64 */
#endif /* FLAC__NO_ASM */
#endif /* FLAC__INTEGER_ONLY_LIBRARY */
