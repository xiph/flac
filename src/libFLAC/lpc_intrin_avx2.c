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

#ifndef FLAC__INTEGER_ONLY_LIBRARY
#ifndef FLAC__NO_ASM
#if (defined FLAC__CPU_IA32 || defined FLAC__CPU_X86_64) && FLAC__HAS_X86INTRIN
#include "private/lpc.h"
#ifdef FLAC__AVX2_SUPPORTED

#include "FLAC/assert.h"
#include "FLAC/format.h"

#include <immintrin.h> /* AVX2 */

FLAC__SSE_TARGET("avx2")
void FLAC__lpc_compute_residual_from_qlp_coefficients_16_intrin_avx2(const FLAC__int32 *data, uint32_t data_len, const FLAC__int32 qlp_coeff[], uint32_t order, int lp_quantization, FLAC__int32 residual[])
{
	int i;
	FLAC__int32 sum;
	const __m128i cnt = _mm_cvtsi32_si128(lp_quantization);

	FLAC__ASSERT(order > 0);
	FLAC__ASSERT(order <= 32);

	if(order <= 12) {
		if(order > 8) {
			if(order > 10) {
				if(order == 12) {
					__m256i q0, q1, q2, q3, q4, q5, q6, q7, q8, q9, q10, q11;
					q0  = _mm256_set1_epi32(0xffff & qlp_coeff[0 ]);
					q1  = _mm256_set1_epi32(0xffff & qlp_coeff[1 ]);
					q2  = _mm256_set1_epi32(0xffff & qlp_coeff[2 ]);
					q3  = _mm256_set1_epi32(0xffff & qlp_coeff[3 ]);
					q4  = _mm256_set1_epi32(0xffff & qlp_coeff[4 ]);
					q5  = _mm256_set1_epi32(0xffff & qlp_coeff[5 ]);
					q6  = _mm256_set1_epi32(0xffff & qlp_coeff[6 ]);
					q7  = _mm256_set1_epi32(0xffff & qlp_coeff[7 ]);
					q8  = _mm256_set1_epi32(0xffff & qlp_coeff[8 ]);
					q9  = _mm256_set1_epi32(0xffff & qlp_coeff[9 ]);
					q10 = _mm256_set1_epi32(0xffff & qlp_coeff[10]);
					q11 = _mm256_set1_epi32(0xffff & qlp_coeff[11]);

					for(i = 0; i < (int)data_len-7; i+=8) {
						__m256i summ, mull;
						summ = _mm256_madd_epi16(q11, _mm256_loadu_si256((const __m256i*)(const void*)(data+i-12)));
						mull = _mm256_madd_epi16(q10, _mm256_loadu_si256((const __m256i*)(const void*)(data+i-11))); summ = _mm256_add_epi32(summ, mull);
						mull = _mm256_madd_epi16(q9,  _mm256_loadu_si256((const __m256i*)(const void*)(data+i-10))); summ = _mm256_add_epi32(summ, mull);
						mull = _mm256_madd_epi16(q8,  _mm256_loadu_si256((const __m256i*)(const void*)(data+i-9 ))); summ = _mm256_add_epi32(summ, mull);
						mull = _mm256_madd_epi16(q7,  _mm256_loadu_si256((const __m256i*)(const void*)(data+i-8 ))); summ = _mm256_add_epi32(summ, mull);
						mull = _mm256_madd_epi16(q6,  _mm256_loadu_si256((const __m256i*)(const void*)(data+i-7 ))); summ = _mm256_add_epi32(summ, mull);
						mull = _mm256_madd_epi16(q5,  _mm256_loadu_si256((const __m256i*)(const void*)(data+i-6 ))); summ = _mm256_add_epi32(summ, mull);
						mull = _mm256_madd_epi16(q4,  _mm256_loadu_si256((const __m256i*)(const void*)(data+i-5 ))); summ = _mm256_add_epi32(summ, mull);
						mull = _mm256_madd_epi16(q3,  _mm256_loadu_si256((const __m256i*)(const void*)(data+i-4 ))); summ = _mm256_add_epi32(summ, mull);
						mull = _mm256_madd_epi16(q2,  _mm256_loadu_si256((const __m256i*)(const void*)(data+i-3 ))); summ = _mm256_add_epi32(summ, mull);
						mull = _mm256_madd_epi16(q1,  _mm256_loadu_si256((const __m256i*)(const void*)(data+i-2 ))); summ = _mm256_add_epi32(summ, mull);
						mull = _mm256_madd_epi16(q0,  _mm256_loadu_si256((const __m256i*)(const void*)(data+i-1 ))); summ = _mm256_add_epi32(summ, mull);
						summ = _mm256_sra_epi32(summ, cnt);
						_mm256_storeu_si256((__m256i*)(void*)(residual+i), _mm256_sub_epi32(_mm256_loadu_si256((const __m256i*)(const void*)(data+i)), summ));
					}
				}
				else { /* order == 11 */
					__m256i q0, q1, q2, q3, q4, q5, q6, q7, q8, q9, q10;
					q0  = _mm256_set1_epi32(0xffff & qlp_coeff[0 ]);
					q1  = _mm256_set1_epi32(0xffff & qlp_coeff[1 ]);
					q2  = _mm256_set1_epi32(0xffff & qlp_coeff[2 ]);
					q3  = _mm256_set1_epi32(0xffff & qlp_coeff[3 ]);
					q4  = _mm256_set1_epi32(0xffff & qlp_coeff[4 ]);
					q5  = _mm256_set1_epi32(0xffff & qlp_coeff[5 ]);
					q6  = _mm256_set1_epi32(0xffff & qlp_coeff[6 ]);
					q7  = _mm256_set1_epi32(0xffff & qlp_coeff[7 ]);
					q8  = _mm256_set1_epi32(0xffff & qlp_coeff[8 ]);
					q9  = _mm256_set1_epi32(0xffff & qlp_coeff[9 ]);
					q10 = _mm256_set1_epi32(0xffff & qlp_coeff[10]);

					for(i = 0; i < (int)data_len-7; i+=8) {
						__m256i summ, mull;
						summ = _mm256_madd_epi16(q10, _mm256_loadu_si256((const __m256i*)(const void*)(data+i-11)));
						mull = _mm256_madd_epi16(q9,  _mm256_loadu_si256((const __m256i*)(const void*)(data+i-10))); summ = _mm256_add_epi32(summ, mull);
						mull = _mm256_madd_epi16(q8,  _mm256_loadu_si256((const __m256i*)(const void*)(data+i-9 ))); summ = _mm256_add_epi32(summ, mull);
						mull = _mm256_madd_epi16(q7,  _mm256_loadu_si256((const __m256i*)(const void*)(data+i-8 ))); summ = _mm256_add_epi32(summ, mull);
						mull = _mm256_madd_epi16(q6,  _mm256_loadu_si256((const __m256i*)(const void*)(data+i-7 ))); summ = _mm256_add_epi32(summ, mull);
						mull = _mm256_madd_epi16(q5,  _mm256_loadu_si256((const __m256i*)(const void*)(data+i-6 ))); summ = _mm256_add_epi32(summ, mull);
						mull = _mm256_madd_epi16(q4,  _mm256_loadu_si256((const __m256i*)(const void*)(data+i-5 ))); summ = _mm256_add_epi32(summ, mull);
						mull = _mm256_madd_epi16(q3,  _mm256_loadu_si256((const __m256i*)(const void*)(data+i-4 ))); summ = _mm256_add_epi32(summ, mull);
						mull = _mm256_madd_epi16(q2,  _mm256_loadu_si256((const __m256i*)(const void*)(data+i-3 ))); summ = _mm256_add_epi32(summ, mull);
						mull = _mm256_madd_epi16(q1,  _mm256_loadu_si256((const __m256i*)(const void*)(data+i-2 ))); summ = _mm256_add_epi32(summ, mull);
						mull = _mm256_madd_epi16(q0,  _mm256_loadu_si256((const __m256i*)(const void*)(data+i-1 ))); summ = _mm256_add_epi32(summ, mull);
						summ = _mm256_sra_epi32(summ, cnt);
						_mm256_storeu_si256((__m256i*)(void*)(residual+i), _mm256_sub_epi32(_mm256_loadu_si256((const __m256i*)(const void*)(data+i)), summ));
					}
				}
			}
			else {
				if(order == 10) {
					__m256i q0, q1, q2, q3, q4, q5, q6, q7, q8, q9;
					q0  = _mm256_set1_epi32(0xffff & qlp_coeff[0 ]);
					q1  = _mm256_set1_epi32(0xffff & qlp_coeff[1 ]);
					q2  = _mm256_set1_epi32(0xffff & qlp_coeff[2 ]);
					q3  = _mm256_set1_epi32(0xffff & qlp_coeff[3 ]);
					q4  = _mm256_set1_epi32(0xffff & qlp_coeff[4 ]);
					q5  = _mm256_set1_epi32(0xffff & qlp_coeff[5 ]);
					q6  = _mm256_set1_epi32(0xffff & qlp_coeff[6 ]);
					q7  = _mm256_set1_epi32(0xffff & qlp_coeff[7 ]);
					q8  = _mm256_set1_epi32(0xffff & qlp_coeff[8 ]);
					q9  = _mm256_set1_epi32(0xffff & qlp_coeff[9 ]);

					for(i = 0; i < (int)data_len-7; i+=8) {
						__m256i summ, mull;
						summ = _mm256_madd_epi16(q9,  _mm256_loadu_si256((const __m256i*)(const void*)(data+i-10)));
						mull = _mm256_madd_epi16(q8,  _mm256_loadu_si256((const __m256i*)(const void*)(data+i-9 ))); summ = _mm256_add_epi32(summ, mull);
						mull = _mm256_madd_epi16(q7,  _mm256_loadu_si256((const __m256i*)(const void*)(data+i-8 ))); summ = _mm256_add_epi32(summ, mull);
						mull = _mm256_madd_epi16(q6,  _mm256_loadu_si256((const __m256i*)(const void*)(data+i-7 ))); summ = _mm256_add_epi32(summ, mull);
						mull = _mm256_madd_epi16(q5,  _mm256_loadu_si256((const __m256i*)(const void*)(data+i-6 ))); summ = _mm256_add_epi32(summ, mull);
						mull = _mm256_madd_epi16(q4,  _mm256_loadu_si256((const __m256i*)(const void*)(data+i-5 ))); summ = _mm256_add_epi32(summ, mull);
						mull = _mm256_madd_epi16(q3,  _mm256_loadu_si256((const __m256i*)(const void*)(data+i-4 ))); summ = _mm256_add_epi32(summ, mull);
						mull = _mm256_madd_epi16(q2,  _mm256_loadu_si256((const __m256i*)(const void*)(data+i-3 ))); summ = _mm256_add_epi32(summ, mull);
						mull = _mm256_madd_epi16(q1,  _mm256_loadu_si256((const __m256i*)(const void*)(data+i-2 ))); summ = _mm256_add_epi32(summ, mull);
						mull = _mm256_madd_epi16(q0,  _mm256_loadu_si256((const __m256i*)(const void*)(data+i-1 ))); summ = _mm256_add_epi32(summ, mull);
						summ = _mm256_sra_epi32(summ, cnt);
						_mm256_storeu_si256((__m256i*)(void*)(residual+i), _mm256_sub_epi32(_mm256_loadu_si256((const __m256i*)(const void*)(data+i)), summ));
					}
				}
				else { /* order == 9 */
					__m256i q0, q1, q2, q3, q4, q5, q6, q7, q8;
					q0  = _mm256_set1_epi32(0xffff & qlp_coeff[0 ]);
					q1  = _mm256_set1_epi32(0xffff & qlp_coeff[1 ]);
					q2  = _mm256_set1_epi32(0xffff & qlp_coeff[2 ]);
					q3  = _mm256_set1_epi32(0xffff & qlp_coeff[3 ]);
					q4  = _mm256_set1_epi32(0xffff & qlp_coeff[4 ]);
					q5  = _mm256_set1_epi32(0xffff & qlp_coeff[5 ]);
					q6  = _mm256_set1_epi32(0xffff & qlp_coeff[6 ]);
					q7  = _mm256_set1_epi32(0xffff & qlp_coeff[7 ]);
					q8  = _mm256_set1_epi32(0xffff & qlp_coeff[8 ]);

					for(i = 0; i < (int)data_len-7; i+=8) {
						__m256i summ, mull;
						summ = _mm256_madd_epi16(q8,  _mm256_loadu_si256((const __m256i*)(const void*)(data+i-9 )));
						mull = _mm256_madd_epi16(q7,  _mm256_loadu_si256((const __m256i*)(const void*)(data+i-8 ))); summ = _mm256_add_epi32(summ, mull);
						mull = _mm256_madd_epi16(q6,  _mm256_loadu_si256((const __m256i*)(const void*)(data+i-7 ))); summ = _mm256_add_epi32(summ, mull);
						mull = _mm256_madd_epi16(q5,  _mm256_loadu_si256((const __m256i*)(const void*)(data+i-6 ))); summ = _mm256_add_epi32(summ, mull);
						mull = _mm256_madd_epi16(q4,  _mm256_loadu_si256((const __m256i*)(const void*)(data+i-5 ))); summ = _mm256_add_epi32(summ, mull);
						mull = _mm256_madd_epi16(q3,  _mm256_loadu_si256((const __m256i*)(const void*)(data+i-4 ))); summ = _mm256_add_epi32(summ, mull);
						mull = _mm256_madd_epi16(q2,  _mm256_loadu_si256((const __m256i*)(const void*)(data+i-3 ))); summ = _mm256_add_epi32(summ, mull);
						mull = _mm256_madd_epi16(q1,  _mm256_loadu_si256((const __m256i*)(const void*)(data+i-2 ))); summ = _mm256_add_epi32(summ, mull);
						mull = _mm256_madd_epi16(q0,  _mm256_loadu_si256((const __m256i*)(const void*)(data+i-1 ))); summ = _mm256_add_epi32(summ, mull);
						summ = _mm256_sra_epi32(summ, cnt);
						_mm256_storeu_si256((__m256i*)(void*)(residual+i), _mm256_sub_epi32(_mm256_loadu_si256((const __m256i*)(const void*)(data+i)), summ));
					}
				}
			}
		}
		else if(order > 4) {
			if(order > 6) {
				if(order == 8) {
					__m256i q0, q1, q2, q3, q4, q5, q6, q7;
					q0  = _mm256_set1_epi32(0xffff & qlp_coeff[0 ]);
					q1  = _mm256_set1_epi32(0xffff & qlp_coeff[1 ]);
					q2  = _mm256_set1_epi32(0xffff & qlp_coeff[2 ]);
					q3  = _mm256_set1_epi32(0xffff & qlp_coeff[3 ]);
					q4  = _mm256_set1_epi32(0xffff & qlp_coeff[4 ]);
					q5  = _mm256_set1_epi32(0xffff & qlp_coeff[5 ]);
					q6  = _mm256_set1_epi32(0xffff & qlp_coeff[6 ]);
					q7  = _mm256_set1_epi32(0xffff & qlp_coeff[7 ]);

					for(i = 0; i < (int)data_len-7; i+=8) {
						__m256i summ, mull;
						summ = _mm256_madd_epi16(q7,  _mm256_loadu_si256((const __m256i*)(const void*)(data+i-8 )));
						mull = _mm256_madd_epi16(q6,  _mm256_loadu_si256((const __m256i*)(const void*)(data+i-7 ))); summ = _mm256_add_epi32(summ, mull);
						mull = _mm256_madd_epi16(q5,  _mm256_loadu_si256((const __m256i*)(const void*)(data+i-6 ))); summ = _mm256_add_epi32(summ, mull);
						mull = _mm256_madd_epi16(q4,  _mm256_loadu_si256((const __m256i*)(const void*)(data+i-5 ))); summ = _mm256_add_epi32(summ, mull);
						mull = _mm256_madd_epi16(q3,  _mm256_loadu_si256((const __m256i*)(const void*)(data+i-4 ))); summ = _mm256_add_epi32(summ, mull);
						mull = _mm256_madd_epi16(q2,  _mm256_loadu_si256((const __m256i*)(const void*)(data+i-3 ))); summ = _mm256_add_epi32(summ, mull);
						mull = _mm256_madd_epi16(q1,  _mm256_loadu_si256((const __m256i*)(const void*)(data+i-2 ))); summ = _mm256_add_epi32(summ, mull);
						mull = _mm256_madd_epi16(q0,  _mm256_loadu_si256((const __m256i*)(const void*)(data+i-1 ))); summ = _mm256_add_epi32(summ, mull);
						summ = _mm256_sra_epi32(summ, cnt);
						_mm256_storeu_si256((__m256i*)(void*)(residual+i), _mm256_sub_epi32(_mm256_loadu_si256((const __m256i*)(const void*)(data+i)), summ));
					}
				}
				else { /* order == 7 */
					__m256i q0, q1, q2, q3, q4, q5, q6;
					q0  = _mm256_set1_epi32(0xffff & qlp_coeff[0 ]);
					q1  = _mm256_set1_epi32(0xffff & qlp_coeff[1 ]);
					q2  = _mm256_set1_epi32(0xffff & qlp_coeff[2 ]);
					q3  = _mm256_set1_epi32(0xffff & qlp_coeff[3 ]);
					q4  = _mm256_set1_epi32(0xffff & qlp_coeff[4 ]);
					q5  = _mm256_set1_epi32(0xffff & qlp_coeff[5 ]);
					q6  = _mm256_set1_epi32(0xffff & qlp_coeff[6 ]);

					for(i = 0; i < (int)data_len-7; i+=8) {
						__m256i summ, mull;
						summ = _mm256_madd_epi16(q6,  _mm256_loadu_si256((const __m256i*)(const void*)(data+i-7 )));
						mull = _mm256_madd_epi16(q5,  _mm256_loadu_si256((const __m256i*)(const void*)(data+i-6 ))); summ = _mm256_add_epi32(summ, mull);
						mull = _mm256_madd_epi16(q4,  _mm256_loadu_si256((const __m256i*)(const void*)(data+i-5 ))); summ = _mm256_add_epi32(summ, mull);
						mull = _mm256_madd_epi16(q3,  _mm256_loadu_si256((const __m256i*)(const void*)(data+i-4 ))); summ = _mm256_add_epi32(summ, mull);
						mull = _mm256_madd_epi16(q2,  _mm256_loadu_si256((const __m256i*)(const void*)(data+i-3 ))); summ = _mm256_add_epi32(summ, mull);
						mull = _mm256_madd_epi16(q1,  _mm256_loadu_si256((const __m256i*)(const void*)(data+i-2 ))); summ = _mm256_add_epi32(summ, mull);
						mull = _mm256_madd_epi16(q0,  _mm256_loadu_si256((const __m256i*)(const void*)(data+i-1 ))); summ = _mm256_add_epi32(summ, mull);
						summ = _mm256_sra_epi32(summ, cnt);
						_mm256_storeu_si256((__m256i*)(void*)(residual+i), _mm256_sub_epi32(_mm256_loadu_si256((const __m256i*)(const void*)(data+i)), summ));
					}
				}
			}
			else {
				if(order == 6) {
					__m256i q0, q1, q2, q3, q4, q5;
					q0  = _mm256_set1_epi32(0xffff & qlp_coeff[0 ]);
					q1  = _mm256_set1_epi32(0xffff & qlp_coeff[1 ]);
					q2  = _mm256_set1_epi32(0xffff & qlp_coeff[2 ]);
					q3  = _mm256_set1_epi32(0xffff & qlp_coeff[3 ]);
					q4  = _mm256_set1_epi32(0xffff & qlp_coeff[4 ]);
					q5  = _mm256_set1_epi32(0xffff & qlp_coeff[5 ]);

					for(i = 0; i < (int)data_len-7; i+=8) {
						__m256i summ, mull;
						summ = _mm256_madd_epi16(q5,  _mm256_loadu_si256((const __m256i*)(const void*)(data+i-6 )));
						mull = _mm256_madd_epi16(q4,  _mm256_loadu_si256((const __m256i*)(const void*)(data+i-5 ))); summ = _mm256_add_epi32(summ, mull);
						mull = _mm256_madd_epi16(q3,  _mm256_loadu_si256((const __m256i*)(const void*)(data+i-4 ))); summ = _mm256_add_epi32(summ, mull);
						mull = _mm256_madd_epi16(q2,  _mm256_loadu_si256((const __m256i*)(const void*)(data+i-3 ))); summ = _mm256_add_epi32(summ, mull);
						mull = _mm256_madd_epi16(q1,  _mm256_loadu_si256((const __m256i*)(const void*)(data+i-2 ))); summ = _mm256_add_epi32(summ, mull);
						mull = _mm256_madd_epi16(q0,  _mm256_loadu_si256((const __m256i*)(const void*)(data+i-1 ))); summ = _mm256_add_epi32(summ, mull);
						summ = _mm256_sra_epi32(summ, cnt);
						_mm256_storeu_si256((__m256i*)(void*)(residual+i), _mm256_sub_epi32(_mm256_loadu_si256((const __m256i*)(const void*)(data+i)), summ));
					}
				}
				else { /* order == 5 */
					__m256i q0, q1, q2, q3, q4;
					q0  = _mm256_set1_epi32(0xffff & qlp_coeff[0 ]);
					q1  = _mm256_set1_epi32(0xffff & qlp_coeff[1 ]);
					q2  = _mm256_set1_epi32(0xffff & qlp_coeff[2 ]);
					q3  = _mm256_set1_epi32(0xffff & qlp_coeff[3 ]);
					q4  = _mm256_set1_epi32(0xffff & qlp_coeff[4 ]);

					for(i = 0; i < (int)data_len-7; i+=8) {
						__m256i summ, mull;
						summ = _mm256_madd_epi16(q4,  _mm256_loadu_si256((const __m256i*)(const void*)(data+i-5 )));
						mull = _mm256_madd_epi16(q3,  _mm256_loadu_si256((const __m256i*)(const void*)(data+i-4 ))); summ = _mm256_add_epi32(summ, mull);
						mull = _mm256_madd_epi16(q2,  _mm256_loadu_si256((const __m256i*)(const void*)(data+i-3 ))); summ = _mm256_add_epi32(summ, mull);
						mull = _mm256_madd_epi16(q1,  _mm256_loadu_si256((const __m256i*)(const void*)(data+i-2 ))); summ = _mm256_add_epi32(summ, mull);
						mull = _mm256_madd_epi16(q0,  _mm256_loadu_si256((const __m256i*)(const void*)(data+i-1 ))); summ = _mm256_add_epi32(summ, mull);
						summ = _mm256_sra_epi32(summ, cnt);
						_mm256_storeu_si256((__m256i*)(void*)(residual+i), _mm256_sub_epi32(_mm256_loadu_si256((const __m256i*)(const void*)(data+i)), summ));
					}
				}
			}
		}
		else {
			if(order > 2) {
				if(order == 4) {
					__m256i q0, q1, q2, q3;
					q0  = _mm256_set1_epi32(0xffff & qlp_coeff[0 ]);
					q1  = _mm256_set1_epi32(0xffff & qlp_coeff[1 ]);
					q2  = _mm256_set1_epi32(0xffff & qlp_coeff[2 ]);
					q3  = _mm256_set1_epi32(0xffff & qlp_coeff[3 ]);

					for(i = 0; i < (int)data_len-7; i+=8) {
						__m256i summ, mull;
						summ = _mm256_madd_epi16(q3,  _mm256_loadu_si256((const __m256i*)(const void*)(data+i-4 )));
						mull = _mm256_madd_epi16(q2,  _mm256_loadu_si256((const __m256i*)(const void*)(data+i-3 ))); summ = _mm256_add_epi32(summ, mull);
						mull = _mm256_madd_epi16(q1,  _mm256_loadu_si256((const __m256i*)(const void*)(data+i-2 ))); summ = _mm256_add_epi32(summ, mull);
						mull = _mm256_madd_epi16(q0,  _mm256_loadu_si256((const __m256i*)(const void*)(data+i-1 ))); summ = _mm256_add_epi32(summ, mull);
						summ = _mm256_sra_epi32(summ, cnt);
						_mm256_storeu_si256((__m256i*)(void*)(residual+i), _mm256_sub_epi32(_mm256_loadu_si256((const __m256i*)(const void*)(data+i)), summ));
					}
				}
				else { /* order == 3 */
					__m256i q0, q1, q2;
					q0  = _mm256_set1_epi32(0xffff & qlp_coeff[0 ]);
					q1  = _mm256_set1_epi32(0xffff & qlp_coeff[1 ]);
					q2  = _mm256_set1_epi32(0xffff & qlp_coeff[2 ]);

					for(i = 0; i < (int)data_len-7; i+=8) {
						__m256i summ, mull;
						summ = _mm256_madd_epi16(q2,  _mm256_loadu_si256((const __m256i*)(const void*)(data+i-3 )));
						mull = _mm256_madd_epi16(q1,  _mm256_loadu_si256((const __m256i*)(const void*)(data+i-2 ))); summ = _mm256_add_epi32(summ, mull);
						mull = _mm256_madd_epi16(q0,  _mm256_loadu_si256((const __m256i*)(const void*)(data+i-1 ))); summ = _mm256_add_epi32(summ, mull);
						summ = _mm256_sra_epi32(summ, cnt);
						_mm256_storeu_si256((__m256i*)(void*)(residual+i), _mm256_sub_epi32(_mm256_loadu_si256((const __m256i*)(const void*)(data+i)), summ));
					}
				}
			}
			else {
				if(order == 2) {
					__m256i q0, q1;
					q0  = _mm256_set1_epi32(0xffff & qlp_coeff[0 ]);
					q1  = _mm256_set1_epi32(0xffff & qlp_coeff[1 ]);

					for(i = 0; i < (int)data_len-7; i+=8) {
						__m256i summ, mull;
						summ = _mm256_madd_epi16(q1,  _mm256_loadu_si256((const __m256i*)(const void*)(data+i-2 )));
						mull = _mm256_madd_epi16(q0,  _mm256_loadu_si256((const __m256i*)(const void*)(data+i-1 ))); summ = _mm256_add_epi32(summ, mull);
						summ = _mm256_sra_epi32(summ, cnt);
						_mm256_storeu_si256((__m256i*)(void*)(residual+i), _mm256_sub_epi32(_mm256_loadu_si256((const __m256i*)(const void*)(data+i)), summ));
					}
				}
				else { /* order == 1 */
					__m256i q0;
					q0  = _mm256_set1_epi32(0xffff & qlp_coeff[0 ]);

					for(i = 0; i < (int)data_len-7; i+=8) {
						__m256i summ;
						summ = _mm256_madd_epi16(q0,  _mm256_loadu_si256((const __m256i*)(const void*)(data+i-1 )));
						summ = _mm256_sra_epi32(summ, cnt);
						_mm256_storeu_si256((__m256i*)(void*)(residual+i), _mm256_sub_epi32(_mm256_loadu_si256((const __m256i*)(const void*)(data+i)), summ));
					}
				}
			}
		}
		for(; i < (int)data_len; i++) {
			sum = 0;
			switch(order) {
				case 12: sum += qlp_coeff[11] * data[i-12]; /* Falls through. */
				case 11: sum += qlp_coeff[10] * data[i-11]; /* Falls through. */
				case 10: sum += qlp_coeff[ 9] * data[i-10]; /* Falls through. */
				case 9:  sum += qlp_coeff[ 8] * data[i- 9]; /* Falls through. */
				case 8:  sum += qlp_coeff[ 7] * data[i- 8]; /* Falls through. */
				case 7:  sum += qlp_coeff[ 6] * data[i- 7]; /* Falls through. */
				case 6:  sum += qlp_coeff[ 5] * data[i- 6]; /* Falls through. */
				case 5:  sum += qlp_coeff[ 4] * data[i- 5]; /* Falls through. */
				case 4:  sum += qlp_coeff[ 3] * data[i- 4]; /* Falls through. */
				case 3:  sum += qlp_coeff[ 2] * data[i- 3]; /* Falls through. */
				case 2:  sum += qlp_coeff[ 1] * data[i- 2]; /* Falls through. */
				case 1:  sum += qlp_coeff[ 0] * data[i- 1];
			}
			residual[i] = data[i] - (sum >> lp_quantization);
		}
	}
	else { /* order > 12 */
		for(i = 0; i < (int)data_len; i++) {
			sum = 0;
			switch(order) {
				case 32: sum += qlp_coeff[31] * data[i-32]; /* Falls through. */
				case 31: sum += qlp_coeff[30] * data[i-31]; /* Falls through. */
				case 30: sum += qlp_coeff[29] * data[i-30]; /* Falls through. */
				case 29: sum += qlp_coeff[28] * data[i-29]; /* Falls through. */
				case 28: sum += qlp_coeff[27] * data[i-28]; /* Falls through. */
				case 27: sum += qlp_coeff[26] * data[i-27]; /* Falls through. */
				case 26: sum += qlp_coeff[25] * data[i-26]; /* Falls through. */
				case 25: sum += qlp_coeff[24] * data[i-25]; /* Falls through. */
				case 24: sum += qlp_coeff[23] * data[i-24]; /* Falls through. */
				case 23: sum += qlp_coeff[22] * data[i-23]; /* Falls through. */
				case 22: sum += qlp_coeff[21] * data[i-22]; /* Falls through. */
				case 21: sum += qlp_coeff[20] * data[i-21]; /* Falls through. */
				case 20: sum += qlp_coeff[19] * data[i-20]; /* Falls through. */
				case 19: sum += qlp_coeff[18] * data[i-19]; /* Falls through. */
				case 18: sum += qlp_coeff[17] * data[i-18]; /* Falls through. */
				case 17: sum += qlp_coeff[16] * data[i-17]; /* Falls through. */
				case 16: sum += qlp_coeff[15] * data[i-16]; /* Falls through. */
				case 15: sum += qlp_coeff[14] * data[i-15]; /* Falls through. */
				case 14: sum += qlp_coeff[13] * data[i-14]; /* Falls through. */
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
	_mm256_zeroupper();
}

FLAC__SSE_TARGET("avx2")
void FLAC__lpc_compute_residual_from_qlp_coefficients_intrin_avx2(const FLAC__int32 *data, uint32_t data_len, const FLAC__int32 qlp_coeff[], uint32_t order, int lp_quantization, FLAC__int32 residual[])
{
	int i;
	FLAC__int32 sum;
	const __m128i cnt = _mm_cvtsi32_si128(lp_quantization);

	FLAC__ASSERT(order > 0);
	FLAC__ASSERT(order <= 32);

	if(order <= 12) {
		if(order > 8) {
			if(order > 10) {
				if(order == 12) {
					__m256i q0, q1, q2, q3, q4, q5, q6, q7, q8, q9, q10, q11;
					q0  = _mm256_set1_epi32(qlp_coeff[0 ]);
					q1  = _mm256_set1_epi32(qlp_coeff[1 ]);
					q2  = _mm256_set1_epi32(qlp_coeff[2 ]);
					q3  = _mm256_set1_epi32(qlp_coeff[3 ]);
					q4  = _mm256_set1_epi32(qlp_coeff[4 ]);
					q5  = _mm256_set1_epi32(qlp_coeff[5 ]);
					q6  = _mm256_set1_epi32(qlp_coeff[6 ]);
					q7  = _mm256_set1_epi32(qlp_coeff[7 ]);
					q8  = _mm256_set1_epi32(qlp_coeff[8 ]);
					q9  = _mm256_set1_epi32(qlp_coeff[9 ]);
					q10 = _mm256_set1_epi32(qlp_coeff[10]);
					q11 = _mm256_set1_epi32(qlp_coeff[11]);

					for(i = 0; i < (int)data_len-7; i+=8) {
						__m256i summ, mull;
						summ = _mm256_mullo_epi32(q11, _mm256_loadu_si256((const __m256i*)(const void*)(data+i-12)));
						mull = _mm256_mullo_epi32(q10, _mm256_loadu_si256((const __m256i*)(const void*)(data+i-11))); summ = _mm256_add_epi32(summ, mull);
						mull = _mm256_mullo_epi32(q9,  _mm256_loadu_si256((const __m256i*)(const void*)(data+i-10))); summ = _mm256_add_epi32(summ, mull);
						mull = _mm256_mullo_epi32(q8,  _mm256_loadu_si256((const __m256i*)(const void*)(data+i-9)));  summ = _mm256_add_epi32(summ, mull);
						mull = _mm256_mullo_epi32(q7,  _mm256_loadu_si256((const __m256i*)(const void*)(data+i-8)));  summ = _mm256_add_epi32(summ, mull);
						mull = _mm256_mullo_epi32(q6,  _mm256_loadu_si256((const __m256i*)(const void*)(data+i-7)));  summ = _mm256_add_epi32(summ, mull);
						mull = _mm256_mullo_epi32(q5,  _mm256_loadu_si256((const __m256i*)(const void*)(data+i-6)));  summ = _mm256_add_epi32(summ, mull);
						mull = _mm256_mullo_epi32(q4,  _mm256_loadu_si256((const __m256i*)(const void*)(data+i-5)));  summ = _mm256_add_epi32(summ, mull);
						mull = _mm256_mullo_epi32(q3,  _mm256_loadu_si256((const __m256i*)(const void*)(data+i-4)));  summ = _mm256_add_epi32(summ, mull);
						mull = _mm256_mullo_epi32(q2,  _mm256_loadu_si256((const __m256i*)(const void*)(data+i-3)));  summ = _mm256_add_epi32(summ, mull);
						mull = _mm256_mullo_epi32(q1,  _mm256_loadu_si256((const __m256i*)(const void*)(data+i-2)));  summ = _mm256_add_epi32(summ, mull);
						mull = _mm256_mullo_epi32(q0,  _mm256_loadu_si256((const __m256i*)(const void*)(data+i-1)));  summ = _mm256_add_epi32(summ, mull);
						summ = _mm256_sra_epi32(summ, cnt);
						_mm256_storeu_si256((__m256i*)(void*)(residual+i), _mm256_sub_epi32(_mm256_loadu_si256((const __m256i*)(const void*)(data+i)), summ));
					}
				}
				else { /* order == 11 */
					__m256i q0, q1, q2, q3, q4, q5, q6, q7, q8, q9, q10;
					q0  = _mm256_set1_epi32(qlp_coeff[0 ]);
					q1  = _mm256_set1_epi32(qlp_coeff[1 ]);
					q2  = _mm256_set1_epi32(qlp_coeff[2 ]);
					q3  = _mm256_set1_epi32(qlp_coeff[3 ]);
					q4  = _mm256_set1_epi32(qlp_coeff[4 ]);
					q5  = _mm256_set1_epi32(qlp_coeff[5 ]);
					q6  = _mm256_set1_epi32(qlp_coeff[6 ]);
					q7  = _mm256_set1_epi32(qlp_coeff[7 ]);
					q8  = _mm256_set1_epi32(qlp_coeff[8 ]);
					q9  = _mm256_set1_epi32(qlp_coeff[9 ]);
					q10 = _mm256_set1_epi32(qlp_coeff[10]);

					for(i = 0; i < (int)data_len-7; i+=8) {
						__m256i summ, mull;
						summ = _mm256_mullo_epi32(q10, _mm256_loadu_si256((const __m256i*)(const void*)(data+i-11)));
						mull = _mm256_mullo_epi32(q9,  _mm256_loadu_si256((const __m256i*)(const void*)(data+i-10))); summ = _mm256_add_epi32(summ, mull);
						mull = _mm256_mullo_epi32(q8,  _mm256_loadu_si256((const __m256i*)(const void*)(data+i-9)));  summ = _mm256_add_epi32(summ, mull);
						mull = _mm256_mullo_epi32(q7,  _mm256_loadu_si256((const __m256i*)(const void*)(data+i-8)));  summ = _mm256_add_epi32(summ, mull);
						mull = _mm256_mullo_epi32(q6,  _mm256_loadu_si256((const __m256i*)(const void*)(data+i-7)));  summ = _mm256_add_epi32(summ, mull);
						mull = _mm256_mullo_epi32(q5,  _mm256_loadu_si256((const __m256i*)(const void*)(data+i-6)));  summ = _mm256_add_epi32(summ, mull);
						mull = _mm256_mullo_epi32(q4,  _mm256_loadu_si256((const __m256i*)(const void*)(data+i-5)));  summ = _mm256_add_epi32(summ, mull);
						mull = _mm256_mullo_epi32(q3,  _mm256_loadu_si256((const __m256i*)(const void*)(data+i-4)));  summ = _mm256_add_epi32(summ, mull);
						mull = _mm256_mullo_epi32(q2,  _mm256_loadu_si256((const __m256i*)(const void*)(data+i-3)));  summ = _mm256_add_epi32(summ, mull);
						mull = _mm256_mullo_epi32(q1,  _mm256_loadu_si256((const __m256i*)(const void*)(data+i-2)));  summ = _mm256_add_epi32(summ, mull);
						mull = _mm256_mullo_epi32(q0,  _mm256_loadu_si256((const __m256i*)(const void*)(data+i-1)));  summ = _mm256_add_epi32(summ, mull);
						summ = _mm256_sra_epi32(summ, cnt);
						_mm256_storeu_si256((__m256i*)(void*)(residual+i), _mm256_sub_epi32(_mm256_loadu_si256((const __m256i*)(const void*)(data+i)), summ));
					}
				}
			}
			else {
				if(order == 10) {
					__m256i q0, q1, q2, q3, q4, q5, q6, q7, q8, q9;
					q0  = _mm256_set1_epi32(qlp_coeff[0 ]);
					q1  = _mm256_set1_epi32(qlp_coeff[1 ]);
					q2  = _mm256_set1_epi32(qlp_coeff[2 ]);
					q3  = _mm256_set1_epi32(qlp_coeff[3 ]);
					q4  = _mm256_set1_epi32(qlp_coeff[4 ]);
					q5  = _mm256_set1_epi32(qlp_coeff[5 ]);
					q6  = _mm256_set1_epi32(qlp_coeff[6 ]);
					q7  = _mm256_set1_epi32(qlp_coeff[7 ]);
					q8  = _mm256_set1_epi32(qlp_coeff[8 ]);
					q9  = _mm256_set1_epi32(qlp_coeff[9 ]);

					for(i = 0; i < (int)data_len-7; i+=8) {
						__m256i summ, mull;
						summ = _mm256_mullo_epi32(q9,  _mm256_loadu_si256((const __m256i*)(const void*)(data+i-10)));
						mull = _mm256_mullo_epi32(q8,  _mm256_loadu_si256((const __m256i*)(const void*)(data+i-9)));  summ = _mm256_add_epi32(summ, mull);
						mull = _mm256_mullo_epi32(q7,  _mm256_loadu_si256((const __m256i*)(const void*)(data+i-8)));  summ = _mm256_add_epi32(summ, mull);
						mull = _mm256_mullo_epi32(q6,  _mm256_loadu_si256((const __m256i*)(const void*)(data+i-7)));  summ = _mm256_add_epi32(summ, mull);
						mull = _mm256_mullo_epi32(q5,  _mm256_loadu_si256((const __m256i*)(const void*)(data+i-6)));  summ = _mm256_add_epi32(summ, mull);
						mull = _mm256_mullo_epi32(q4,  _mm256_loadu_si256((const __m256i*)(const void*)(data+i-5)));  summ = _mm256_add_epi32(summ, mull);
						mull = _mm256_mullo_epi32(q3,  _mm256_loadu_si256((const __m256i*)(const void*)(data+i-4)));  summ = _mm256_add_epi32(summ, mull);
						mull = _mm256_mullo_epi32(q2,  _mm256_loadu_si256((const __m256i*)(const void*)(data+i-3)));  summ = _mm256_add_epi32(summ, mull);
						mull = _mm256_mullo_epi32(q1,  _mm256_loadu_si256((const __m256i*)(const void*)(data+i-2)));  summ = _mm256_add_epi32(summ, mull);
						mull = _mm256_mullo_epi32(q0,  _mm256_loadu_si256((const __m256i*)(const void*)(data+i-1)));  summ = _mm256_add_epi32(summ, mull);
						summ = _mm256_sra_epi32(summ, cnt);
						_mm256_storeu_si256((__m256i*)(void*)(residual+i), _mm256_sub_epi32(_mm256_loadu_si256((const __m256i*)(const void*)(data+i)), summ));
					}
				}
				else { /* order == 9 */
					__m256i q0, q1, q2, q3, q4, q5, q6, q7, q8;
					q0  = _mm256_set1_epi32(qlp_coeff[0 ]);
					q1  = _mm256_set1_epi32(qlp_coeff[1 ]);
					q2  = _mm256_set1_epi32(qlp_coeff[2 ]);
					q3  = _mm256_set1_epi32(qlp_coeff[3 ]);
					q4  = _mm256_set1_epi32(qlp_coeff[4 ]);
					q5  = _mm256_set1_epi32(qlp_coeff[5 ]);
					q6  = _mm256_set1_epi32(qlp_coeff[6 ]);
					q7  = _mm256_set1_epi32(qlp_coeff[7 ]);
					q8  = _mm256_set1_epi32(qlp_coeff[8 ]);

					for(i = 0; i < (int)data_len-7; i+=8) {
						__m256i summ, mull;
						summ = _mm256_mullo_epi32(q8,  _mm256_loadu_si256((const __m256i*)(const void*)(data+i-9)));
						mull = _mm256_mullo_epi32(q7,  _mm256_loadu_si256((const __m256i*)(const void*)(data+i-8)));  summ = _mm256_add_epi32(summ, mull);
						mull = _mm256_mullo_epi32(q6,  _mm256_loadu_si256((const __m256i*)(const void*)(data+i-7)));  summ = _mm256_add_epi32(summ, mull);
						mull = _mm256_mullo_epi32(q5,  _mm256_loadu_si256((const __m256i*)(const void*)(data+i-6)));  summ = _mm256_add_epi32(summ, mull);
						mull = _mm256_mullo_epi32(q4,  _mm256_loadu_si256((const __m256i*)(const void*)(data+i-5)));  summ = _mm256_add_epi32(summ, mull);
						mull = _mm256_mullo_epi32(q3,  _mm256_loadu_si256((const __m256i*)(const void*)(data+i-4)));  summ = _mm256_add_epi32(summ, mull);
						mull = _mm256_mullo_epi32(q2,  _mm256_loadu_si256((const __m256i*)(const void*)(data+i-3)));  summ = _mm256_add_epi32(summ, mull);
						mull = _mm256_mullo_epi32(q1,  _mm256_loadu_si256((const __m256i*)(const void*)(data+i-2)));  summ = _mm256_add_epi32(summ, mull);
						mull = _mm256_mullo_epi32(q0,  _mm256_loadu_si256((const __m256i*)(const void*)(data+i-1)));  summ = _mm256_add_epi32(summ, mull);
						summ = _mm256_sra_epi32(summ, cnt);
						_mm256_storeu_si256((__m256i*)(void*)(residual+i), _mm256_sub_epi32(_mm256_loadu_si256((const __m256i*)(const void*)(data+i)), summ));
					}
				}
			}
		}
		else if(order > 4) {
			if(order > 6) {
				if(order == 8) {
					__m256i q0, q1, q2, q3, q4, q5, q6, q7;
					q0  = _mm256_set1_epi32(qlp_coeff[0 ]);
					q1  = _mm256_set1_epi32(qlp_coeff[1 ]);
					q2  = _mm256_set1_epi32(qlp_coeff[2 ]);
					q3  = _mm256_set1_epi32(qlp_coeff[3 ]);
					q4  = _mm256_set1_epi32(qlp_coeff[4 ]);
					q5  = _mm256_set1_epi32(qlp_coeff[5 ]);
					q6  = _mm256_set1_epi32(qlp_coeff[6 ]);
					q7  = _mm256_set1_epi32(qlp_coeff[7 ]);

					for(i = 0; i < (int)data_len-7; i+=8) {
						__m256i summ, mull;
						summ = _mm256_mullo_epi32(q7,  _mm256_loadu_si256((const __m256i*)(const void*)(data+i-8)));
						mull = _mm256_mullo_epi32(q6,  _mm256_loadu_si256((const __m256i*)(const void*)(data+i-7)));  summ = _mm256_add_epi32(summ, mull);
						mull = _mm256_mullo_epi32(q5,  _mm256_loadu_si256((const __m256i*)(const void*)(data+i-6)));  summ = _mm256_add_epi32(summ, mull);
						mull = _mm256_mullo_epi32(q4,  _mm256_loadu_si256((const __m256i*)(const void*)(data+i-5)));  summ = _mm256_add_epi32(summ, mull);
						mull = _mm256_mullo_epi32(q3,  _mm256_loadu_si256((const __m256i*)(const void*)(data+i-4)));  summ = _mm256_add_epi32(summ, mull);
						mull = _mm256_mullo_epi32(q2,  _mm256_loadu_si256((const __m256i*)(const void*)(data+i-3)));  summ = _mm256_add_epi32(summ, mull);
						mull = _mm256_mullo_epi32(q1,  _mm256_loadu_si256((const __m256i*)(const void*)(data+i-2)));  summ = _mm256_add_epi32(summ, mull);
						mull = _mm256_mullo_epi32(q0,  _mm256_loadu_si256((const __m256i*)(const void*)(data+i-1)));  summ = _mm256_add_epi32(summ, mull);
						summ = _mm256_sra_epi32(summ, cnt);
						_mm256_storeu_si256((__m256i*)(void*)(residual+i), _mm256_sub_epi32(_mm256_loadu_si256((const __m256i*)(const void*)(data+i)), summ));
					}
				}
				else { /* order == 7 */
					__m256i q0, q1, q2, q3, q4, q5, q6;
					q0  = _mm256_set1_epi32(qlp_coeff[0 ]);
					q1  = _mm256_set1_epi32(qlp_coeff[1 ]);
					q2  = _mm256_set1_epi32(qlp_coeff[2 ]);
					q3  = _mm256_set1_epi32(qlp_coeff[3 ]);
					q4  = _mm256_set1_epi32(qlp_coeff[4 ]);
					q5  = _mm256_set1_epi32(qlp_coeff[5 ]);
					q6  = _mm256_set1_epi32(qlp_coeff[6 ]);

					for(i = 0; i < (int)data_len-7; i+=8) {
						__m256i summ, mull;
						summ = _mm256_mullo_epi32(q6,  _mm256_loadu_si256((const __m256i*)(const void*)(data+i-7)));
						mull = _mm256_mullo_epi32(q5,  _mm256_loadu_si256((const __m256i*)(const void*)(data+i-6)));  summ = _mm256_add_epi32(summ, mull);
						mull = _mm256_mullo_epi32(q4,  _mm256_loadu_si256((const __m256i*)(const void*)(data+i-5)));  summ = _mm256_add_epi32(summ, mull);
						mull = _mm256_mullo_epi32(q3,  _mm256_loadu_si256((const __m256i*)(const void*)(data+i-4)));  summ = _mm256_add_epi32(summ, mull);
						mull = _mm256_mullo_epi32(q2,  _mm256_loadu_si256((const __m256i*)(const void*)(data+i-3)));  summ = _mm256_add_epi32(summ, mull);
						mull = _mm256_mullo_epi32(q1,  _mm256_loadu_si256((const __m256i*)(const void*)(data+i-2)));  summ = _mm256_add_epi32(summ, mull);
						mull = _mm256_mullo_epi32(q0,  _mm256_loadu_si256((const __m256i*)(const void*)(data+i-1)));  summ = _mm256_add_epi32(summ, mull);
						summ = _mm256_sra_epi32(summ, cnt);
						_mm256_storeu_si256((__m256i*)(void*)(residual+i), _mm256_sub_epi32(_mm256_loadu_si256((const __m256i*)(const void*)(data+i)), summ));
					}
				}
			}
			else {
				if(order == 6) {
					__m256i q0, q1, q2, q3, q4, q5;
					q0  = _mm256_set1_epi32(qlp_coeff[0 ]);
					q1  = _mm256_set1_epi32(qlp_coeff[1 ]);
					q2  = _mm256_set1_epi32(qlp_coeff[2 ]);
					q3  = _mm256_set1_epi32(qlp_coeff[3 ]);
					q4  = _mm256_set1_epi32(qlp_coeff[4 ]);
					q5  = _mm256_set1_epi32(qlp_coeff[5 ]);

					for(i = 0; i < (int)data_len-7; i+=8) {
						__m256i summ, mull;
						summ = _mm256_mullo_epi32(q5,  _mm256_loadu_si256((const __m256i*)(const void*)(data+i-6)));
						mull = _mm256_mullo_epi32(q4,  _mm256_loadu_si256((const __m256i*)(const void*)(data+i-5)));  summ = _mm256_add_epi32(summ, mull);
						mull = _mm256_mullo_epi32(q3,  _mm256_loadu_si256((const __m256i*)(const void*)(data+i-4)));  summ = _mm256_add_epi32(summ, mull);
						mull = _mm256_mullo_epi32(q2,  _mm256_loadu_si256((const __m256i*)(const void*)(data+i-3)));  summ = _mm256_add_epi32(summ, mull);
						mull = _mm256_mullo_epi32(q1,  _mm256_loadu_si256((const __m256i*)(const void*)(data+i-2)));  summ = _mm256_add_epi32(summ, mull);
						mull = _mm256_mullo_epi32(q0,  _mm256_loadu_si256((const __m256i*)(const void*)(data+i-1)));  summ = _mm256_add_epi32(summ, mull);
						summ = _mm256_sra_epi32(summ, cnt);
						_mm256_storeu_si256((__m256i*)(void*)(residual+i), _mm256_sub_epi32(_mm256_loadu_si256((const __m256i*)(const void*)(data+i)), summ));
					}
				}
				else { /* order == 5 */
					__m256i q0, q1, q2, q3, q4;
					q0  = _mm256_set1_epi32(qlp_coeff[0 ]);
					q1  = _mm256_set1_epi32(qlp_coeff[1 ]);
					q2  = _mm256_set1_epi32(qlp_coeff[2 ]);
					q3  = _mm256_set1_epi32(qlp_coeff[3 ]);
					q4  = _mm256_set1_epi32(qlp_coeff[4 ]);

					for(i = 0; i < (int)data_len-7; i+=8) {
						__m256i summ, mull;
						summ = _mm256_mullo_epi32(q4,  _mm256_loadu_si256((const __m256i*)(const void*)(data+i-5)));
						mull = _mm256_mullo_epi32(q3,  _mm256_loadu_si256((const __m256i*)(const void*)(data+i-4)));  summ = _mm256_add_epi32(summ, mull);
						mull = _mm256_mullo_epi32(q2,  _mm256_loadu_si256((const __m256i*)(const void*)(data+i-3)));  summ = _mm256_add_epi32(summ, mull);
						mull = _mm256_mullo_epi32(q1,  _mm256_loadu_si256((const __m256i*)(const void*)(data+i-2)));  summ = _mm256_add_epi32(summ, mull);
						mull = _mm256_mullo_epi32(q0,  _mm256_loadu_si256((const __m256i*)(const void*)(data+i-1)));  summ = _mm256_add_epi32(summ, mull);
						summ = _mm256_sra_epi32(summ, cnt);
						_mm256_storeu_si256((__m256i*)(void*)(residual+i), _mm256_sub_epi32(_mm256_loadu_si256((const __m256i*)(const void*)(data+i)), summ));
					}
				}
			}
		}
		else {
			if(order > 2) {
				if(order == 4) {
					__m256i q0, q1, q2, q3;
					q0  = _mm256_set1_epi32(qlp_coeff[0 ]);
					q1  = _mm256_set1_epi32(qlp_coeff[1 ]);
					q2  = _mm256_set1_epi32(qlp_coeff[2 ]);
					q3  = _mm256_set1_epi32(qlp_coeff[3 ]);

					for(i = 0; i < (int)data_len-7; i+=8) {
						__m256i summ, mull;
						summ = _mm256_mullo_epi32(q3,  _mm256_loadu_si256((const __m256i*)(const void*)(data+i-4)));
						mull = _mm256_mullo_epi32(q2,  _mm256_loadu_si256((const __m256i*)(const void*)(data+i-3)));  summ = _mm256_add_epi32(summ, mull);
						mull = _mm256_mullo_epi32(q1,  _mm256_loadu_si256((const __m256i*)(const void*)(data+i-2)));  summ = _mm256_add_epi32(summ, mull);
						mull = _mm256_mullo_epi32(q0,  _mm256_loadu_si256((const __m256i*)(const void*)(data+i-1)));  summ = _mm256_add_epi32(summ, mull);
						summ = _mm256_sra_epi32(summ, cnt);
						_mm256_storeu_si256((__m256i*)(void*)(residual+i), _mm256_sub_epi32(_mm256_loadu_si256((const __m256i*)(const void*)(data+i)), summ));
					}
				}
				else { /* order == 3 */
					__m256i q0, q1, q2;
					q0  = _mm256_set1_epi32(qlp_coeff[0 ]);
					q1  = _mm256_set1_epi32(qlp_coeff[1 ]);
					q2  = _mm256_set1_epi32(qlp_coeff[2 ]);

					for(i = 0; i < (int)data_len-7; i+=8) {
						__m256i summ, mull;
						summ = _mm256_mullo_epi32(q2,  _mm256_loadu_si256((const __m256i*)(const void*)(data+i-3)));
						mull = _mm256_mullo_epi32(q1,  _mm256_loadu_si256((const __m256i*)(const void*)(data+i-2)));  summ = _mm256_add_epi32(summ, mull);
						mull = _mm256_mullo_epi32(q0,  _mm256_loadu_si256((const __m256i*)(const void*)(data+i-1)));  summ = _mm256_add_epi32(summ, mull);
						summ = _mm256_sra_epi32(summ, cnt);
						_mm256_storeu_si256((__m256i*)(void*)(residual+i), _mm256_sub_epi32(_mm256_loadu_si256((const __m256i*)(const void*)(data+i)), summ));
					}
				}
			}
			else {
				if(order == 2) {
					__m256i q0, q1;
					q0  = _mm256_set1_epi32(qlp_coeff[0 ]);
					q1  = _mm256_set1_epi32(qlp_coeff[1 ]);

					for(i = 0; i < (int)data_len-7; i+=8) {
						__m256i summ, mull;
						summ = _mm256_mullo_epi32(q1,  _mm256_loadu_si256((const __m256i*)(const void*)(data+i-2)));
						mull = _mm256_mullo_epi32(q0,  _mm256_loadu_si256((const __m256i*)(const void*)(data+i-1)));  summ = _mm256_add_epi32(summ, mull);
						summ = _mm256_sra_epi32(summ, cnt);
						_mm256_storeu_si256((__m256i*)(void*)(residual+i), _mm256_sub_epi32(_mm256_loadu_si256((const __m256i*)(const void*)(data+i)), summ));
					}
				}
				else { /* order == 1 */
					__m256i q0;
					q0  = _mm256_set1_epi32(qlp_coeff[0 ]);

					for(i = 0; i < (int)data_len-7; i+=8) {
						__m256i summ;
						summ = _mm256_mullo_epi32(q0,  _mm256_loadu_si256((const __m256i*)(const void*)(data+i-1)));
						summ = _mm256_sra_epi32(summ, cnt);
						_mm256_storeu_si256((__m256i*)(void*)(residual+i), _mm256_sub_epi32(_mm256_loadu_si256((const __m256i*)(const void*)(data+i)), summ));
					}
				}
			}
		}
		for(; i < (int)data_len; i++) {
			sum = 0;
			switch(order) {
				case 12: sum += qlp_coeff[11] * data[i-12]; /* Falls through. */
				case 11: sum += qlp_coeff[10] * data[i-11]; /* Falls through. */
				case 10: sum += qlp_coeff[ 9] * data[i-10]; /* Falls through. */
				case 9:  sum += qlp_coeff[ 8] * data[i- 9]; /* Falls through. */
				case 8:  sum += qlp_coeff[ 7] * data[i- 8]; /* Falls through. */
				case 7:  sum += qlp_coeff[ 6] * data[i- 7]; /* Falls through. */
				case 6:  sum += qlp_coeff[ 5] * data[i- 6]; /* Falls through. */
				case 5:  sum += qlp_coeff[ 4] * data[i- 5]; /* Falls through. */
				case 4:  sum += qlp_coeff[ 3] * data[i- 4]; /* Falls through. */
				case 3:  sum += qlp_coeff[ 2] * data[i- 3]; /* Falls through. */
				case 2:  sum += qlp_coeff[ 1] * data[i- 2]; /* Falls through. */
				case 1:  sum += qlp_coeff[ 0] * data[i- 1];
			}
			residual[i] = data[i] - (sum >> lp_quantization);
		}
	}
	else { /* order > 12 */
		for(i = 0; i < (int)data_len; i++) {
			sum = 0;
			switch(order) {
				case 32: sum += qlp_coeff[31] * data[i-32]; /* Falls through. */
				case 31: sum += qlp_coeff[30] * data[i-31]; /* Falls through. */
				case 30: sum += qlp_coeff[29] * data[i-30]; /* Falls through. */
				case 29: sum += qlp_coeff[28] * data[i-29]; /* Falls through. */
				case 28: sum += qlp_coeff[27] * data[i-28]; /* Falls through. */
				case 27: sum += qlp_coeff[26] * data[i-27]; /* Falls through. */
				case 26: sum += qlp_coeff[25] * data[i-26]; /* Falls through. */
				case 25: sum += qlp_coeff[24] * data[i-25]; /* Falls through. */
				case 24: sum += qlp_coeff[23] * data[i-24]; /* Falls through. */
				case 23: sum += qlp_coeff[22] * data[i-23]; /* Falls through. */
				case 22: sum += qlp_coeff[21] * data[i-22]; /* Falls through. */
				case 21: sum += qlp_coeff[20] * data[i-21]; /* Falls through. */
				case 20: sum += qlp_coeff[19] * data[i-20]; /* Falls through. */
				case 19: sum += qlp_coeff[18] * data[i-19]; /* Falls through. */
				case 18: sum += qlp_coeff[17] * data[i-18]; /* Falls through. */
				case 17: sum += qlp_coeff[16] * data[i-17]; /* Falls through. */
				case 16: sum += qlp_coeff[15] * data[i-16]; /* Falls through. */
				case 15: sum += qlp_coeff[14] * data[i-15]; /* Falls through. */
				case 14: sum += qlp_coeff[13] * data[i-14]; /* Falls through. */
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
	_mm256_zeroupper();
}

static FLAC__int32 pack_arr[8] = { 0, 2, 4, 6, 1, 3, 5, 7 };

FLAC__SSE_TARGET("avx2")
void FLAC__lpc_compute_residual_from_qlp_coefficients_wide_intrin_avx2(const FLAC__int32 *data, uint32_t data_len, const FLAC__int32 qlp_coeff[], uint32_t order, int lp_quantization, FLAC__int32 residual[])
{
	int i;
	FLAC__int64 sum;
	const __m128i cnt = _mm_cvtsi32_si128(lp_quantization);
	const __m256i pack = _mm256_loadu_si256((const __m256i *)(const void*)pack_arr);

	FLAC__ASSERT(order > 0);
	FLAC__ASSERT(order <= 32);
	FLAC__ASSERT(lp_quantization <= 32); /* there's no _mm256_sra_epi64() so we have to use _mm256_srl_epi64() */

	if(order <= 12) {
		if(order > 8) {
			if(order > 10) {
				if(order == 12) {
					__m256i q0, q1, q2, q3, q4, q5, q6, q7, q8, q9, q10, q11;
					q0  = _mm256_cvtepu32_epi64(_mm_set1_epi32(qlp_coeff[0 ]));
					q1  = _mm256_cvtepu32_epi64(_mm_set1_epi32(qlp_coeff[1 ]));
					q2  = _mm256_cvtepu32_epi64(_mm_set1_epi32(qlp_coeff[2 ]));
					q3  = _mm256_cvtepu32_epi64(_mm_set1_epi32(qlp_coeff[3 ]));
					q4  = _mm256_cvtepu32_epi64(_mm_set1_epi32(qlp_coeff[4 ]));
					q5  = _mm256_cvtepu32_epi64(_mm_set1_epi32(qlp_coeff[5 ]));
					q6  = _mm256_cvtepu32_epi64(_mm_set1_epi32(qlp_coeff[6 ]));
					q7  = _mm256_cvtepu32_epi64(_mm_set1_epi32(qlp_coeff[7 ]));
					q8  = _mm256_cvtepu32_epi64(_mm_set1_epi32(qlp_coeff[8 ]));
					q9  = _mm256_cvtepu32_epi64(_mm_set1_epi32(qlp_coeff[9 ]));
					q10 = _mm256_cvtepu32_epi64(_mm_set1_epi32(qlp_coeff[10]));
					q11 = _mm256_cvtepu32_epi64(_mm_set1_epi32(qlp_coeff[11]));

					for(i = 0; i < (int)data_len-3; i+=4) {
						__m256i summ, mull;
						summ = _mm256_mul_epi32(q11, _mm256_cvtepu32_epi64(_mm_loadu_si128((const __m128i*)(const void*)(data+i-12))));
						mull = _mm256_mul_epi32(q10, _mm256_cvtepu32_epi64(_mm_loadu_si128((const __m128i*)(const void*)(data+i-11)))); summ = _mm256_add_epi64(summ, mull);
						mull = _mm256_mul_epi32(q9,  _mm256_cvtepu32_epi64(_mm_loadu_si128((const __m128i*)(const void*)(data+i-10)))); summ = _mm256_add_epi64(summ, mull);
						mull = _mm256_mul_epi32(q8,  _mm256_cvtepu32_epi64(_mm_loadu_si128((const __m128i*)(const void*)(data+i-9 )))); summ = _mm256_add_epi64(summ, mull);
						mull = _mm256_mul_epi32(q7,  _mm256_cvtepu32_epi64(_mm_loadu_si128((const __m128i*)(const void*)(data+i-8 )))); summ = _mm256_add_epi64(summ, mull);
						mull = _mm256_mul_epi32(q6,  _mm256_cvtepu32_epi64(_mm_loadu_si128((const __m128i*)(const void*)(data+i-7 )))); summ = _mm256_add_epi64(summ, mull);
						mull = _mm256_mul_epi32(q5,  _mm256_cvtepu32_epi64(_mm_loadu_si128((const __m128i*)(const void*)(data+i-6 )))); summ = _mm256_add_epi64(summ, mull);
						mull = _mm256_mul_epi32(q4,  _mm256_cvtepu32_epi64(_mm_loadu_si128((const __m128i*)(const void*)(data+i-5 )))); summ = _mm256_add_epi64(summ, mull);
						mull = _mm256_mul_epi32(q3,  _mm256_cvtepu32_epi64(_mm_loadu_si128((const __m128i*)(const void*)(data+i-4 )))); summ = _mm256_add_epi64(summ, mull);
						mull = _mm256_mul_epi32(q2,  _mm256_cvtepu32_epi64(_mm_loadu_si128((const __m128i*)(const void*)(data+i-3 )))); summ = _mm256_add_epi64(summ, mull);
						mull = _mm256_mul_epi32(q1,  _mm256_cvtepu32_epi64(_mm_loadu_si128((const __m128i*)(const void*)(data+i-2 )))); summ = _mm256_add_epi64(summ, mull);
						mull = _mm256_mul_epi32(q0,  _mm256_cvtepu32_epi64(_mm_loadu_si128((const __m128i*)(const void*)(data+i-1 )))); summ = _mm256_add_epi64(summ, mull);
						summ = _mm256_permutevar8x32_epi32(_mm256_srl_epi64(summ, cnt), pack);
						_mm_storeu_si128((__m128i*)(void*)(residual+i), _mm_sub_epi32(_mm_loadu_si128((const __m128i*)(const void*)(data+i)), _mm256_castsi256_si128(summ)));
					}
				}
				else { /* order == 11 */
					__m256i q0, q1, q2, q3, q4, q5, q6, q7, q8, q9, q10;
					q0  = _mm256_cvtepu32_epi64(_mm_set1_epi32(qlp_coeff[0 ]));
					q1  = _mm256_cvtepu32_epi64(_mm_set1_epi32(qlp_coeff[1 ]));
					q2  = _mm256_cvtepu32_epi64(_mm_set1_epi32(qlp_coeff[2 ]));
					q3  = _mm256_cvtepu32_epi64(_mm_set1_epi32(qlp_coeff[3 ]));
					q4  = _mm256_cvtepu32_epi64(_mm_set1_epi32(qlp_coeff[4 ]));
					q5  = _mm256_cvtepu32_epi64(_mm_set1_epi32(qlp_coeff[5 ]));
					q6  = _mm256_cvtepu32_epi64(_mm_set1_epi32(qlp_coeff[6 ]));
					q7  = _mm256_cvtepu32_epi64(_mm_set1_epi32(qlp_coeff[7 ]));
					q8  = _mm256_cvtepu32_epi64(_mm_set1_epi32(qlp_coeff[8 ]));
					q9  = _mm256_cvtepu32_epi64(_mm_set1_epi32(qlp_coeff[9 ]));
					q10 = _mm256_cvtepu32_epi64(_mm_set1_epi32(qlp_coeff[10]));

					for(i = 0; i < (int)data_len-3; i+=4) {
						__m256i summ, mull;
						summ = _mm256_mul_epi32(q10, _mm256_cvtepu32_epi64(_mm_loadu_si128((const __m128i*)(const void*)(data+i-11))));
						mull = _mm256_mul_epi32(q9,  _mm256_cvtepu32_epi64(_mm_loadu_si128((const __m128i*)(const void*)(data+i-10)))); summ = _mm256_add_epi64(summ, mull);
						mull = _mm256_mul_epi32(q8,  _mm256_cvtepu32_epi64(_mm_loadu_si128((const __m128i*)(const void*)(data+i-9 )))); summ = _mm256_add_epi64(summ, mull);
						mull = _mm256_mul_epi32(q7,  _mm256_cvtepu32_epi64(_mm_loadu_si128((const __m128i*)(const void*)(data+i-8 )))); summ = _mm256_add_epi64(summ, mull);
						mull = _mm256_mul_epi32(q6,  _mm256_cvtepu32_epi64(_mm_loadu_si128((const __m128i*)(const void*)(data+i-7 )))); summ = _mm256_add_epi64(summ, mull);
						mull = _mm256_mul_epi32(q5,  _mm256_cvtepu32_epi64(_mm_loadu_si128((const __m128i*)(const void*)(data+i-6 )))); summ = _mm256_add_epi64(summ, mull);
						mull = _mm256_mul_epi32(q4,  _mm256_cvtepu32_epi64(_mm_loadu_si128((const __m128i*)(const void*)(data+i-5 )))); summ = _mm256_add_epi64(summ, mull);
						mull = _mm256_mul_epi32(q3,  _mm256_cvtepu32_epi64(_mm_loadu_si128((const __m128i*)(const void*)(data+i-4 )))); summ = _mm256_add_epi64(summ, mull);
						mull = _mm256_mul_epi32(q2,  _mm256_cvtepu32_epi64(_mm_loadu_si128((const __m128i*)(const void*)(data+i-3 )))); summ = _mm256_add_epi64(summ, mull);
						mull = _mm256_mul_epi32(q1,  _mm256_cvtepu32_epi64(_mm_loadu_si128((const __m128i*)(const void*)(data+i-2 )))); summ = _mm256_add_epi64(summ, mull);
						mull = _mm256_mul_epi32(q0,  _mm256_cvtepu32_epi64(_mm_loadu_si128((const __m128i*)(const void*)(data+i-1 )))); summ = _mm256_add_epi64(summ, mull);
						summ = _mm256_permutevar8x32_epi32(_mm256_srl_epi64(summ, cnt), pack);
						_mm_storeu_si128((__m128i*)(void*)(residual+i), _mm_sub_epi32(_mm_loadu_si128((const __m128i*)(const void*)(data+i)), _mm256_castsi256_si128(summ)));
					}
				}
			}
			else {
				if(order == 10) {
					__m256i q0, q1, q2, q3, q4, q5, q6, q7, q8, q9;
					q0  = _mm256_cvtepu32_epi64(_mm_set1_epi32(qlp_coeff[0 ]));
					q1  = _mm256_cvtepu32_epi64(_mm_set1_epi32(qlp_coeff[1 ]));
					q2  = _mm256_cvtepu32_epi64(_mm_set1_epi32(qlp_coeff[2 ]));
					q3  = _mm256_cvtepu32_epi64(_mm_set1_epi32(qlp_coeff[3 ]));
					q4  = _mm256_cvtepu32_epi64(_mm_set1_epi32(qlp_coeff[4 ]));
					q5  = _mm256_cvtepu32_epi64(_mm_set1_epi32(qlp_coeff[5 ]));
					q6  = _mm256_cvtepu32_epi64(_mm_set1_epi32(qlp_coeff[6 ]));
					q7  = _mm256_cvtepu32_epi64(_mm_set1_epi32(qlp_coeff[7 ]));
					q8  = _mm256_cvtepu32_epi64(_mm_set1_epi32(qlp_coeff[8 ]));
					q9  = _mm256_cvtepu32_epi64(_mm_set1_epi32(qlp_coeff[9 ]));

					for(i = 0; i < (int)data_len-3; i+=4) {
						__m256i summ, mull;
						summ = _mm256_mul_epi32(q9,  _mm256_cvtepu32_epi64(_mm_loadu_si128((const __m128i*)(const void*)(data+i-10))));
						mull = _mm256_mul_epi32(q8,  _mm256_cvtepu32_epi64(_mm_loadu_si128((const __m128i*)(const void*)(data+i-9 )))); summ = _mm256_add_epi64(summ, mull);
						mull = _mm256_mul_epi32(q7,  _mm256_cvtepu32_epi64(_mm_loadu_si128((const __m128i*)(const void*)(data+i-8 )))); summ = _mm256_add_epi64(summ, mull);
						mull = _mm256_mul_epi32(q6,  _mm256_cvtepu32_epi64(_mm_loadu_si128((const __m128i*)(const void*)(data+i-7 )))); summ = _mm256_add_epi64(summ, mull);
						mull = _mm256_mul_epi32(q5,  _mm256_cvtepu32_epi64(_mm_loadu_si128((const __m128i*)(const void*)(data+i-6 )))); summ = _mm256_add_epi64(summ, mull);
						mull = _mm256_mul_epi32(q4,  _mm256_cvtepu32_epi64(_mm_loadu_si128((const __m128i*)(const void*)(data+i-5 )))); summ = _mm256_add_epi64(summ, mull);
						mull = _mm256_mul_epi32(q3,  _mm256_cvtepu32_epi64(_mm_loadu_si128((const __m128i*)(const void*)(data+i-4 )))); summ = _mm256_add_epi64(summ, mull);
						mull = _mm256_mul_epi32(q2,  _mm256_cvtepu32_epi64(_mm_loadu_si128((const __m128i*)(const void*)(data+i-3 )))); summ = _mm256_add_epi64(summ, mull);
						mull = _mm256_mul_epi32(q1,  _mm256_cvtepu32_epi64(_mm_loadu_si128((const __m128i*)(const void*)(data+i-2 )))); summ = _mm256_add_epi64(summ, mull);
						mull = _mm256_mul_epi32(q0,  _mm256_cvtepu32_epi64(_mm_loadu_si128((const __m128i*)(const void*)(data+i-1 )))); summ = _mm256_add_epi64(summ, mull);
						summ = _mm256_permutevar8x32_epi32(_mm256_srl_epi64(summ, cnt), pack);
						_mm_storeu_si128((__m128i*)(void*)(residual+i), _mm_sub_epi32(_mm_loadu_si128((const __m128i*)(const void*)(data+i)), _mm256_castsi256_si128(summ)));
					}
				}
				else { /* order == 9 */
					__m256i q0, q1, q2, q3, q4, q5, q6, q7, q8;
					q0  = _mm256_cvtepu32_epi64(_mm_set1_epi32(qlp_coeff[0 ]));
					q1  = _mm256_cvtepu32_epi64(_mm_set1_epi32(qlp_coeff[1 ]));
					q2  = _mm256_cvtepu32_epi64(_mm_set1_epi32(qlp_coeff[2 ]));
					q3  = _mm256_cvtepu32_epi64(_mm_set1_epi32(qlp_coeff[3 ]));
					q4  = _mm256_cvtepu32_epi64(_mm_set1_epi32(qlp_coeff[4 ]));
					q5  = _mm256_cvtepu32_epi64(_mm_set1_epi32(qlp_coeff[5 ]));
					q6  = _mm256_cvtepu32_epi64(_mm_set1_epi32(qlp_coeff[6 ]));
					q7  = _mm256_cvtepu32_epi64(_mm_set1_epi32(qlp_coeff[7 ]));
					q8  = _mm256_cvtepu32_epi64(_mm_set1_epi32(qlp_coeff[8 ]));

					for(i = 0; i < (int)data_len-3; i+=4) {
						__m256i summ, mull;
						summ = _mm256_mul_epi32(q8,  _mm256_cvtepu32_epi64(_mm_loadu_si128((const __m128i*)(const void*)(data+i-9 ))));
						mull = _mm256_mul_epi32(q7,  _mm256_cvtepu32_epi64(_mm_loadu_si128((const __m128i*)(const void*)(data+i-8 )))); summ = _mm256_add_epi64(summ, mull);
						mull = _mm256_mul_epi32(q6,  _mm256_cvtepu32_epi64(_mm_loadu_si128((const __m128i*)(const void*)(data+i-7 )))); summ = _mm256_add_epi64(summ, mull);
						mull = _mm256_mul_epi32(q5,  _mm256_cvtepu32_epi64(_mm_loadu_si128((const __m128i*)(const void*)(data+i-6 )))); summ = _mm256_add_epi64(summ, mull);
						mull = _mm256_mul_epi32(q4,  _mm256_cvtepu32_epi64(_mm_loadu_si128((const __m128i*)(const void*)(data+i-5 )))); summ = _mm256_add_epi64(summ, mull);
						mull = _mm256_mul_epi32(q3,  _mm256_cvtepu32_epi64(_mm_loadu_si128((const __m128i*)(const void*)(data+i-4 )))); summ = _mm256_add_epi64(summ, mull);
						mull = _mm256_mul_epi32(q2,  _mm256_cvtepu32_epi64(_mm_loadu_si128((const __m128i*)(const void*)(data+i-3 )))); summ = _mm256_add_epi64(summ, mull);
						mull = _mm256_mul_epi32(q1,  _mm256_cvtepu32_epi64(_mm_loadu_si128((const __m128i*)(const void*)(data+i-2 )))); summ = _mm256_add_epi64(summ, mull);
						mull = _mm256_mul_epi32(q0,  _mm256_cvtepu32_epi64(_mm_loadu_si128((const __m128i*)(const void*)(data+i-1 )))); summ = _mm256_add_epi64(summ, mull);
						summ = _mm256_permutevar8x32_epi32(_mm256_srl_epi64(summ, cnt), pack);
						_mm_storeu_si128((__m128i*)(void*)(residual+i), _mm_sub_epi32(_mm_loadu_si128((const __m128i*)(const void*)(data+i)), _mm256_castsi256_si128(summ)));
					}
				}
			}
		}
		else if(order > 4) {
			if(order > 6) {
				if(order == 8) {
					__m256i q0, q1, q2, q3, q4, q5, q6, q7;
					q0  = _mm256_cvtepu32_epi64(_mm_set1_epi32(qlp_coeff[0 ]));
					q1  = _mm256_cvtepu32_epi64(_mm_set1_epi32(qlp_coeff[1 ]));
					q2  = _mm256_cvtepu32_epi64(_mm_set1_epi32(qlp_coeff[2 ]));
					q3  = _mm256_cvtepu32_epi64(_mm_set1_epi32(qlp_coeff[3 ]));
					q4  = _mm256_cvtepu32_epi64(_mm_set1_epi32(qlp_coeff[4 ]));
					q5  = _mm256_cvtepu32_epi64(_mm_set1_epi32(qlp_coeff[5 ]));
					q6  = _mm256_cvtepu32_epi64(_mm_set1_epi32(qlp_coeff[6 ]));
					q7  = _mm256_cvtepu32_epi64(_mm_set1_epi32(qlp_coeff[7 ]));

					for(i = 0; i < (int)data_len-3; i+=4) {
						__m256i summ, mull;
						summ = _mm256_mul_epi32(q7,  _mm256_cvtepu32_epi64(_mm_loadu_si128((const __m128i*)(const void*)(data+i-8 ))));
						mull = _mm256_mul_epi32(q6,  _mm256_cvtepu32_epi64(_mm_loadu_si128((const __m128i*)(const void*)(data+i-7 )))); summ = _mm256_add_epi64(summ, mull);
						mull = _mm256_mul_epi32(q5,  _mm256_cvtepu32_epi64(_mm_loadu_si128((const __m128i*)(const void*)(data+i-6 )))); summ = _mm256_add_epi64(summ, mull);
						mull = _mm256_mul_epi32(q4,  _mm256_cvtepu32_epi64(_mm_loadu_si128((const __m128i*)(const void*)(data+i-5 )))); summ = _mm256_add_epi64(summ, mull);
						mull = _mm256_mul_epi32(q3,  _mm256_cvtepu32_epi64(_mm_loadu_si128((const __m128i*)(const void*)(data+i-4 )))); summ = _mm256_add_epi64(summ, mull);
						mull = _mm256_mul_epi32(q2,  _mm256_cvtepu32_epi64(_mm_loadu_si128((const __m128i*)(const void*)(data+i-3 )))); summ = _mm256_add_epi64(summ, mull);
						mull = _mm256_mul_epi32(q1,  _mm256_cvtepu32_epi64(_mm_loadu_si128((const __m128i*)(const void*)(data+i-2 )))); summ = _mm256_add_epi64(summ, mull);
						mull = _mm256_mul_epi32(q0,  _mm256_cvtepu32_epi64(_mm_loadu_si128((const __m128i*)(const void*)(data+i-1 )))); summ = _mm256_add_epi64(summ, mull);
						summ = _mm256_permutevar8x32_epi32(_mm256_srl_epi64(summ, cnt), pack);
						_mm_storeu_si128((__m128i*)(void*)(residual+i), _mm_sub_epi32(_mm_loadu_si128((const __m128i*)(const void*)(data+i)), _mm256_castsi256_si128(summ)));
					}
				}
				else { /* order == 7 */
					__m256i q0, q1, q2, q3, q4, q5, q6;
					q0  = _mm256_cvtepu32_epi64(_mm_set1_epi32(qlp_coeff[0 ]));
					q1  = _mm256_cvtepu32_epi64(_mm_set1_epi32(qlp_coeff[1 ]));
					q2  = _mm256_cvtepu32_epi64(_mm_set1_epi32(qlp_coeff[2 ]));
					q3  = _mm256_cvtepu32_epi64(_mm_set1_epi32(qlp_coeff[3 ]));
					q4  = _mm256_cvtepu32_epi64(_mm_set1_epi32(qlp_coeff[4 ]));
					q5  = _mm256_cvtepu32_epi64(_mm_set1_epi32(qlp_coeff[5 ]));
					q6  = _mm256_cvtepu32_epi64(_mm_set1_epi32(qlp_coeff[6 ]));

					for(i = 0; i < (int)data_len-3; i+=4) {
						__m256i summ, mull;
						summ = _mm256_mul_epi32(q6,  _mm256_cvtepu32_epi64(_mm_loadu_si128((const __m128i*)(const void*)(data+i-7 ))));
						mull = _mm256_mul_epi32(q5,  _mm256_cvtepu32_epi64(_mm_loadu_si128((const __m128i*)(const void*)(data+i-6 )))); summ = _mm256_add_epi64(summ, mull);
						mull = _mm256_mul_epi32(q4,  _mm256_cvtepu32_epi64(_mm_loadu_si128((const __m128i*)(const void*)(data+i-5 )))); summ = _mm256_add_epi64(summ, mull);
						mull = _mm256_mul_epi32(q3,  _mm256_cvtepu32_epi64(_mm_loadu_si128((const __m128i*)(const void*)(data+i-4 )))); summ = _mm256_add_epi64(summ, mull);
						mull = _mm256_mul_epi32(q2,  _mm256_cvtepu32_epi64(_mm_loadu_si128((const __m128i*)(const void*)(data+i-3 )))); summ = _mm256_add_epi64(summ, mull);
						mull = _mm256_mul_epi32(q1,  _mm256_cvtepu32_epi64(_mm_loadu_si128((const __m128i*)(const void*)(data+i-2 )))); summ = _mm256_add_epi64(summ, mull);
						mull = _mm256_mul_epi32(q0,  _mm256_cvtepu32_epi64(_mm_loadu_si128((const __m128i*)(const void*)(data+i-1 )))); summ = _mm256_add_epi64(summ, mull);
						summ = _mm256_permutevar8x32_epi32(_mm256_srl_epi64(summ, cnt), pack);
						_mm_storeu_si128((__m128i*)(void*)(residual+i), _mm_sub_epi32(_mm_loadu_si128((const __m128i*)(const void*)(data+i)), _mm256_castsi256_si128(summ)));
					}
				}
			}
			else {
				if(order == 6) {
					__m256i q0, q1, q2, q3, q4, q5;
					q0  = _mm256_cvtepu32_epi64(_mm_set1_epi32(qlp_coeff[0 ]));
					q1  = _mm256_cvtepu32_epi64(_mm_set1_epi32(qlp_coeff[1 ]));
					q2  = _mm256_cvtepu32_epi64(_mm_set1_epi32(qlp_coeff[2 ]));
					q3  = _mm256_cvtepu32_epi64(_mm_set1_epi32(qlp_coeff[3 ]));
					q4  = _mm256_cvtepu32_epi64(_mm_set1_epi32(qlp_coeff[4 ]));
					q5  = _mm256_cvtepu32_epi64(_mm_set1_epi32(qlp_coeff[5 ]));

					for(i = 0; i < (int)data_len-3; i+=4) {
						__m256i summ, mull;
						summ = _mm256_mul_epi32(q5,  _mm256_cvtepu32_epi64(_mm_loadu_si128((const __m128i*)(const void*)(data+i-6 ))));
						mull = _mm256_mul_epi32(q4,  _mm256_cvtepu32_epi64(_mm_loadu_si128((const __m128i*)(const void*)(data+i-5 )))); summ = _mm256_add_epi64(summ, mull);
						mull = _mm256_mul_epi32(q3,  _mm256_cvtepu32_epi64(_mm_loadu_si128((const __m128i*)(const void*)(data+i-4 )))); summ = _mm256_add_epi64(summ, mull);
						mull = _mm256_mul_epi32(q2,  _mm256_cvtepu32_epi64(_mm_loadu_si128((const __m128i*)(const void*)(data+i-3 )))); summ = _mm256_add_epi64(summ, mull);
						mull = _mm256_mul_epi32(q1,  _mm256_cvtepu32_epi64(_mm_loadu_si128((const __m128i*)(const void*)(data+i-2 )))); summ = _mm256_add_epi64(summ, mull);
						mull = _mm256_mul_epi32(q0,  _mm256_cvtepu32_epi64(_mm_loadu_si128((const __m128i*)(const void*)(data+i-1 )))); summ = _mm256_add_epi64(summ, mull);
						summ = _mm256_permutevar8x32_epi32(_mm256_srl_epi64(summ, cnt), pack);
						_mm_storeu_si128((__m128i*)(void*)(residual+i), _mm_sub_epi32(_mm_loadu_si128((const __m128i*)(const void*)(data+i)), _mm256_castsi256_si128(summ)));
					}
				}
				else { /* order == 5 */
					__m256i q0, q1, q2, q3, q4;
					q0  = _mm256_cvtepu32_epi64(_mm_set1_epi32(qlp_coeff[0 ]));
					q1  = _mm256_cvtepu32_epi64(_mm_set1_epi32(qlp_coeff[1 ]));
					q2  = _mm256_cvtepu32_epi64(_mm_set1_epi32(qlp_coeff[2 ]));
					q3  = _mm256_cvtepu32_epi64(_mm_set1_epi32(qlp_coeff[3 ]));
					q4  = _mm256_cvtepu32_epi64(_mm_set1_epi32(qlp_coeff[4 ]));

					for(i = 0; i < (int)data_len-3; i+=4) {
						__m256i summ, mull;
						summ = _mm256_mul_epi32(q4,  _mm256_cvtepu32_epi64(_mm_loadu_si128((const __m128i*)(const void*)(data+i-5 ))));
						mull = _mm256_mul_epi32(q3,  _mm256_cvtepu32_epi64(_mm_loadu_si128((const __m128i*)(const void*)(data+i-4 )))); summ = _mm256_add_epi64(summ, mull);
						mull = _mm256_mul_epi32(q2,  _mm256_cvtepu32_epi64(_mm_loadu_si128((const __m128i*)(const void*)(data+i-3 )))); summ = _mm256_add_epi64(summ, mull);
						mull = _mm256_mul_epi32(q1,  _mm256_cvtepu32_epi64(_mm_loadu_si128((const __m128i*)(const void*)(data+i-2 )))); summ = _mm256_add_epi64(summ, mull);
						mull = _mm256_mul_epi32(q0,  _mm256_cvtepu32_epi64(_mm_loadu_si128((const __m128i*)(const void*)(data+i-1 )))); summ = _mm256_add_epi64(summ, mull);
						summ = _mm256_permutevar8x32_epi32(_mm256_srl_epi64(summ, cnt), pack);
						_mm_storeu_si128((__m128i*)(void*)(residual+i), _mm_sub_epi32(_mm_loadu_si128((const __m128i*)(const void*)(data+i)), _mm256_castsi256_si128(summ)));
					}
				}
			}
		}
		else {
			if(order > 2) {
				if(order == 4) {
					__m256i q0, q1, q2, q3;
					q0  = _mm256_cvtepu32_epi64(_mm_set1_epi32(qlp_coeff[0 ]));
					q1  = _mm256_cvtepu32_epi64(_mm_set1_epi32(qlp_coeff[1 ]));
					q2  = _mm256_cvtepu32_epi64(_mm_set1_epi32(qlp_coeff[2 ]));
					q3  = _mm256_cvtepu32_epi64(_mm_set1_epi32(qlp_coeff[3 ]));

					for(i = 0; i < (int)data_len-3; i+=4) {
						__m256i summ, mull;
						summ = _mm256_mul_epi32(q3,  _mm256_cvtepu32_epi64(_mm_loadu_si128((const __m128i*)(const void*)(data+i-4 ))));
						mull = _mm256_mul_epi32(q2,  _mm256_cvtepu32_epi64(_mm_loadu_si128((const __m128i*)(const void*)(data+i-3 )))); summ = _mm256_add_epi64(summ, mull);
						mull = _mm256_mul_epi32(q1,  _mm256_cvtepu32_epi64(_mm_loadu_si128((const __m128i*)(const void*)(data+i-2 )))); summ = _mm256_add_epi64(summ, mull);
						mull = _mm256_mul_epi32(q0,  _mm256_cvtepu32_epi64(_mm_loadu_si128((const __m128i*)(const void*)(data+i-1 )))); summ = _mm256_add_epi64(summ, mull);
						summ = _mm256_permutevar8x32_epi32(_mm256_srl_epi64(summ, cnt), pack);
						_mm_storeu_si128((__m128i*)(void*)(residual+i), _mm_sub_epi32(_mm_loadu_si128((const __m128i*)(const void*)(data+i)), _mm256_castsi256_si128(summ)));
					}
				}
				else { /* order == 3 */
					__m256i q0, q1, q2;
					q0  = _mm256_cvtepu32_epi64(_mm_set1_epi32(qlp_coeff[0 ]));
					q1  = _mm256_cvtepu32_epi64(_mm_set1_epi32(qlp_coeff[1 ]));
					q2  = _mm256_cvtepu32_epi64(_mm_set1_epi32(qlp_coeff[2 ]));

					for(i = 0; i < (int)data_len-3; i+=4) {
						__m256i summ, mull;
						summ = _mm256_mul_epi32(q2,  _mm256_cvtepu32_epi64(_mm_loadu_si128((const __m128i*)(const void*)(data+i-3 ))));
						mull = _mm256_mul_epi32(q1,  _mm256_cvtepu32_epi64(_mm_loadu_si128((const __m128i*)(const void*)(data+i-2 )))); summ = _mm256_add_epi64(summ, mull);
						mull = _mm256_mul_epi32(q0,  _mm256_cvtepu32_epi64(_mm_loadu_si128((const __m128i*)(const void*)(data+i-1 )))); summ = _mm256_add_epi64(summ, mull);
						summ = _mm256_permutevar8x32_epi32(_mm256_srl_epi64(summ, cnt), pack);
						_mm_storeu_si128((__m128i*)(void*)(residual+i), _mm_sub_epi32(_mm_loadu_si128((const __m128i*)(const void*)(data+i)), _mm256_castsi256_si128(summ)));
					}
				}
			}
			else {
				if(order == 2) {
					__m256i q0, q1;
					q0  = _mm256_cvtepu32_epi64(_mm_set1_epi32(qlp_coeff[0 ]));
					q1  = _mm256_cvtepu32_epi64(_mm_set1_epi32(qlp_coeff[1 ]));

					for(i = 0; i < (int)data_len-3; i+=4) {
						__m256i summ, mull;
						summ = _mm256_mul_epi32(q1,  _mm256_cvtepu32_epi64(_mm_loadu_si128((const __m128i*)(const void*)(data+i-2 ))));
						mull = _mm256_mul_epi32(q0,  _mm256_cvtepu32_epi64(_mm_loadu_si128((const __m128i*)(const void*)(data+i-1 )))); summ = _mm256_add_epi64(summ, mull);
						summ = _mm256_permutevar8x32_epi32(_mm256_srl_epi64(summ, cnt), pack);
						_mm_storeu_si128((__m128i*)(void*)(residual+i), _mm_sub_epi32(_mm_loadu_si128((const __m128i*)(const void*)(data+i)), _mm256_castsi256_si128(summ)));
					}
				}
				else { /* order == 1 */
					__m256i q0;
					q0  = _mm256_cvtepu32_epi64(_mm_set1_epi32(qlp_coeff[0 ]));

					for(i = 0; i < (int)data_len-3; i+=4) {
						__m256i summ;
						summ = _mm256_mul_epi32(q0,  _mm256_cvtepu32_epi64(_mm_loadu_si128((const __m128i*)(const void*)(data+i-1 ))));
						summ = _mm256_permutevar8x32_epi32(_mm256_srl_epi64(summ, cnt), pack);
						_mm_storeu_si128((__m128i*)(void*)(residual+i), _mm_sub_epi32(_mm_loadu_si128((const __m128i*)(const void*)(data+i)), _mm256_castsi256_si128(summ)));
					}
				}
			}
		}
		for(; i < (int)data_len; i++) {
			sum = 0;
			switch(order) {
				case 12: sum += qlp_coeff[11] * (FLAC__int64)data[i-12]; /* Falls through. */
				case 11: sum += qlp_coeff[10] * (FLAC__int64)data[i-11]; /* Falls through. */
				case 10: sum += qlp_coeff[ 9] * (FLAC__int64)data[i-10]; /* Falls through. */
				case 9:  sum += qlp_coeff[ 8] * (FLAC__int64)data[i- 9]; /* Falls through. */
				case 8:  sum += qlp_coeff[ 7] * (FLAC__int64)data[i- 8]; /* Falls through. */
				case 7:  sum += qlp_coeff[ 6] * (FLAC__int64)data[i- 7]; /* Falls through. */
				case 6:  sum += qlp_coeff[ 5] * (FLAC__int64)data[i- 6]; /* Falls through. */
				case 5:  sum += qlp_coeff[ 4] * (FLAC__int64)data[i- 5]; /* Falls through. */
				case 4:  sum += qlp_coeff[ 3] * (FLAC__int64)data[i- 4]; /* Falls through. */
				case 3:  sum += qlp_coeff[ 2] * (FLAC__int64)data[i- 3]; /* Falls through. */
				case 2:  sum += qlp_coeff[ 1] * (FLAC__int64)data[i- 2]; /* Falls through. */
				case 1:  sum += qlp_coeff[ 0] * (FLAC__int64)data[i- 1];
			}
			residual[i] = data[i] - (FLAC__int32)(sum >> lp_quantization);
		}
	}
	else { /* order > 12 */
		for(i = 0; i < (int)data_len; i++) {
			sum = 0;
			switch(order) {
				case 32: sum += qlp_coeff[31] * (FLAC__int64)data[i-32]; /* Falls through. */
				case 31: sum += qlp_coeff[30] * (FLAC__int64)data[i-31]; /* Falls through. */
				case 30: sum += qlp_coeff[29] * (FLAC__int64)data[i-30]; /* Falls through. */
				case 29: sum += qlp_coeff[28] * (FLAC__int64)data[i-29]; /* Falls through. */
				case 28: sum += qlp_coeff[27] * (FLAC__int64)data[i-28]; /* Falls through. */
				case 27: sum += qlp_coeff[26] * (FLAC__int64)data[i-27]; /* Falls through. */
				case 26: sum += qlp_coeff[25] * (FLAC__int64)data[i-26]; /* Falls through. */
				case 25: sum += qlp_coeff[24] * (FLAC__int64)data[i-25]; /* Falls through. */
				case 24: sum += qlp_coeff[23] * (FLAC__int64)data[i-24]; /* Falls through. */
				case 23: sum += qlp_coeff[22] * (FLAC__int64)data[i-23]; /* Falls through. */
				case 22: sum += qlp_coeff[21] * (FLAC__int64)data[i-22]; /* Falls through. */
				case 21: sum += qlp_coeff[20] * (FLAC__int64)data[i-21]; /* Falls through. */
				case 20: sum += qlp_coeff[19] * (FLAC__int64)data[i-20]; /* Falls through. */
				case 19: sum += qlp_coeff[18] * (FLAC__int64)data[i-19]; /* Falls through. */
				case 18: sum += qlp_coeff[17] * (FLAC__int64)data[i-18]; /* Falls through. */
				case 17: sum += qlp_coeff[16] * (FLAC__int64)data[i-17]; /* Falls through. */
				case 16: sum += qlp_coeff[15] * (FLAC__int64)data[i-16]; /* Falls through. */
				case 15: sum += qlp_coeff[14] * (FLAC__int64)data[i-15]; /* Falls through. */
				case 14: sum += qlp_coeff[13] * (FLAC__int64)data[i-14]; /* Falls through. */
				case 13: sum += qlp_coeff[12] * (FLAC__int64)data[i-13];
				         sum += qlp_coeff[11] * (FLAC__int64)data[i-12];
				         sum += qlp_coeff[10] * (FLAC__int64)data[i-11];
				         sum += qlp_coeff[ 9] * (FLAC__int64)data[i-10];
				         sum += qlp_coeff[ 8] * (FLAC__int64)data[i- 9];
				         sum += qlp_coeff[ 7] * (FLAC__int64)data[i- 8];
				         sum += qlp_coeff[ 6] * (FLAC__int64)data[i- 7];
				         sum += qlp_coeff[ 5] * (FLAC__int64)data[i- 6];
				         sum += qlp_coeff[ 4] * (FLAC__int64)data[i- 5];
				         sum += qlp_coeff[ 3] * (FLAC__int64)data[i- 4];
				         sum += qlp_coeff[ 2] * (FLAC__int64)data[i- 3];
				         sum += qlp_coeff[ 1] * (FLAC__int64)data[i- 2];
				         sum += qlp_coeff[ 0] * (FLAC__int64)data[i- 1];
			}
			residual[i] = data[i] - (FLAC__int32)(sum >> lp_quantization);
		}
	}
	_mm256_zeroupper();
}





FLAC__SSE_TARGET("avx2")
static inline __m256i local__mul_i64_x4_by_i32(__m256i x, int32_t q)
{
	__m256i q_vec = _mm256_set1_epi64x((uint32_t)q);
	__m256i x_hi = _mm256_srli_epi64(x, 32);
	__m256i prod_lo = _mm256_mul_epu32(x, q_vec);
	__m256i prod_hi = _mm256_mul_epi32(x_hi, q_vec);
	__m256i prod = _mm256_add_epi64(prod_lo, _mm256_slli_epi64(prod_hi, 32));
	if (q < 0) {
		__m256i x_lo_32 = _mm256_and_si256(x, _mm256_set1_epi64x(0xFFFFFFFFULL));
		prod = _mm256_sub_epi64(prod, _mm256_slli_epi64(x_lo_32, 32));
	}
	return prod;
}

FLAC__SSE_TARGET("avx2")
static inline __m256i local__sra_i64_x4(__m256i x, int shift)
{
	__m256i sign, fill, srl;
	if (shift == 0) return x;
	sign = _mm256_cmpgt_epi64(_mm256_setzero_si256(), x);
	fill = _mm256_sll_epi64(sign, _mm_cvtsi32_si128(64 - shift));
	srl = _mm256_srl_epi64(x, _mm_cvtsi32_si128(shift));
	return _mm256_or_si256(srl, fill);
}

FLAC__SSE_TARGET("avx2")
FLAC__bool FLAC__lpc_compute_residual_from_qlp_coefficients_limit_residual_64bit_res64_intrin_avx2(const FLAC__int64 *data, uint32_t data_len, const FLAC__int32 qlp_coeff[], uint32_t order, int lp_quantization, FLAC__int64 residual[], uint32_t subframe_bps)
{
#if FLAC__HAS_INT128
	int i;
	if (subframe_bps > 43)
		return FLAC__lpc_compute_residual_from_qlp_coefficients_limit_residual_64bit_res64(data, data_len, qlp_coeff, order, lp_quantization, residual, subframe_bps);
	FLAC__ASSERT(order > 0);
	FLAC__ASSERT(order <= 32);
	FLAC__ASSERT(lp_quantization <= 32);

	for(i = 0; i < (int)data_len-3; i+=4) {
		__m256i summ = _mm256_setzero_si256();
		__m256i shifted, res64;
		switch(order) {
			case 32: summ = _mm256_add_epi64(summ, local__mul_i64_x4_by_i32(_mm256_loadu_si256((const __m256i*)(const void*)(data+i-32)), qlp_coeff[31])); /* Falls through. */
			case 31: summ = _mm256_add_epi64(summ, local__mul_i64_x4_by_i32(_mm256_loadu_si256((const __m256i*)(const void*)(data+i-31)), qlp_coeff[30])); /* Falls through. */
			case 30: summ = _mm256_add_epi64(summ, local__mul_i64_x4_by_i32(_mm256_loadu_si256((const __m256i*)(const void*)(data+i-30)), qlp_coeff[29])); /* Falls through. */
			case 29: summ = _mm256_add_epi64(summ, local__mul_i64_x4_by_i32(_mm256_loadu_si256((const __m256i*)(const void*)(data+i-29)), qlp_coeff[28])); /* Falls through. */
			case 28: summ = _mm256_add_epi64(summ, local__mul_i64_x4_by_i32(_mm256_loadu_si256((const __m256i*)(const void*)(data+i-28)), qlp_coeff[27])); /* Falls through. */
			case 27: summ = _mm256_add_epi64(summ, local__mul_i64_x4_by_i32(_mm256_loadu_si256((const __m256i*)(const void*)(data+i-27)), qlp_coeff[26])); /* Falls through. */
			case 26: summ = _mm256_add_epi64(summ, local__mul_i64_x4_by_i32(_mm256_loadu_si256((const __m256i*)(const void*)(data+i-26)), qlp_coeff[25])); /* Falls through. */
			case 25: summ = _mm256_add_epi64(summ, local__mul_i64_x4_by_i32(_mm256_loadu_si256((const __m256i*)(const void*)(data+i-25)), qlp_coeff[24])); /* Falls through. */
			case 24: summ = _mm256_add_epi64(summ, local__mul_i64_x4_by_i32(_mm256_loadu_si256((const __m256i*)(const void*)(data+i-24)), qlp_coeff[23])); /* Falls through. */
			case 23: summ = _mm256_add_epi64(summ, local__mul_i64_x4_by_i32(_mm256_loadu_si256((const __m256i*)(const void*)(data+i-23)), qlp_coeff[22])); /* Falls through. */
			case 22: summ = _mm256_add_epi64(summ, local__mul_i64_x4_by_i32(_mm256_loadu_si256((const __m256i*)(const void*)(data+i-22)), qlp_coeff[21])); /* Falls through. */
			case 21: summ = _mm256_add_epi64(summ, local__mul_i64_x4_by_i32(_mm256_loadu_si256((const __m256i*)(const void*)(data+i-21)), qlp_coeff[20])); /* Falls through. */
			case 20: summ = _mm256_add_epi64(summ, local__mul_i64_x4_by_i32(_mm256_loadu_si256((const __m256i*)(const void*)(data+i-20)), qlp_coeff[19])); /* Falls through. */
			case 19: summ = _mm256_add_epi64(summ, local__mul_i64_x4_by_i32(_mm256_loadu_si256((const __m256i*)(const void*)(data+i-19)), qlp_coeff[18])); /* Falls through. */
			case 18: summ = _mm256_add_epi64(summ, local__mul_i64_x4_by_i32(_mm256_loadu_si256((const __m256i*)(const void*)(data+i-18)), qlp_coeff[17])); /* Falls through. */
			case 17: summ = _mm256_add_epi64(summ, local__mul_i64_x4_by_i32(_mm256_loadu_si256((const __m256i*)(const void*)(data+i-17)), qlp_coeff[16])); /* Falls through. */
			case 16: summ = _mm256_add_epi64(summ, local__mul_i64_x4_by_i32(_mm256_loadu_si256((const __m256i*)(const void*)(data+i-16)), qlp_coeff[15])); /* Falls through. */
			case 15: summ = _mm256_add_epi64(summ, local__mul_i64_x4_by_i32(_mm256_loadu_si256((const __m256i*)(const void*)(data+i-15)), qlp_coeff[14])); /* Falls through. */
			case 14: summ = _mm256_add_epi64(summ, local__mul_i64_x4_by_i32(_mm256_loadu_si256((const __m256i*)(const void*)(data+i-14)), qlp_coeff[13])); /* Falls through. */
			case 13: summ = _mm256_add_epi64(summ, local__mul_i64_x4_by_i32(_mm256_loadu_si256((const __m256i*)(const void*)(data+i-13)), qlp_coeff[12])); /* Falls through. */
			case 12: summ = _mm256_add_epi64(summ, local__mul_i64_x4_by_i32(_mm256_loadu_si256((const __m256i*)(const void*)(data+i-12)), qlp_coeff[11])); /* Falls through. */
			case 11: summ = _mm256_add_epi64(summ, local__mul_i64_x4_by_i32(_mm256_loadu_si256((const __m256i*)(const void*)(data+i-11)), qlp_coeff[10])); /* Falls through. */
			case 10: summ = _mm256_add_epi64(summ, local__mul_i64_x4_by_i32(_mm256_loadu_si256((const __m256i*)(const void*)(data+i-10)), qlp_coeff[ 9])); /* Falls through. */
			case  9: summ = _mm256_add_epi64(summ, local__mul_i64_x4_by_i32(_mm256_loadu_si256((const __m256i*)(const void*)(data+i- 9)), qlp_coeff[ 8])); /* Falls through. */
			case  8: summ = _mm256_add_epi64(summ, local__mul_i64_x4_by_i32(_mm256_loadu_si256((const __m256i*)(const void*)(data+i- 8)), qlp_coeff[ 7])); /* Falls through. */
			case  7: summ = _mm256_add_epi64(summ, local__mul_i64_x4_by_i32(_mm256_loadu_si256((const __m256i*)(const void*)(data+i- 7)), qlp_coeff[ 6])); /* Falls through. */
			case  6: summ = _mm256_add_epi64(summ, local__mul_i64_x4_by_i32(_mm256_loadu_si256((const __m256i*)(const void*)(data+i- 6)), qlp_coeff[ 5])); /* Falls through. */
			case  5: summ = _mm256_add_epi64(summ, local__mul_i64_x4_by_i32(_mm256_loadu_si256((const __m256i*)(const void*)(data+i- 5)), qlp_coeff[ 4])); /* Falls through. */
			case  4: summ = _mm256_add_epi64(summ, local__mul_i64_x4_by_i32(_mm256_loadu_si256((const __m256i*)(const void*)(data+i- 4)), qlp_coeff[ 3])); /* Falls through. */
			case  3: summ = _mm256_add_epi64(summ, local__mul_i64_x4_by_i32(_mm256_loadu_si256((const __m256i*)(const void*)(data+i- 3)), qlp_coeff[ 2])); /* Falls through. */
			case  2: summ = _mm256_add_epi64(summ, local__mul_i64_x4_by_i32(_mm256_loadu_si256((const __m256i*)(const void*)(data+i- 2)), qlp_coeff[ 1])); /* Falls through. */
			case  1: summ = _mm256_add_epi64(summ, local__mul_i64_x4_by_i32(_mm256_loadu_si256((const __m256i*)(const void*)(data+i- 1)), qlp_coeff[ 0]));
		}
		shifted = local__sra_i64_x4(summ, lp_quantization);
		res64 = _mm256_sub_epi64(_mm256_loadu_si256((const __m256i*)(const void*)(data+i)), shifted);
		_mm256_storeu_si256((__m256i*)(void*)(residual+i), res64);
	}
	for(; i < (int)data_len; i++) {
		FLAC__int128 sum = 0;
		switch(order) {
			case 32: sum += (FLAC__int128)qlp_coeff[31] * (FLAC__int128)data[i-32]; /* Falls through. */
			case 31: sum += (FLAC__int128)qlp_coeff[30] * (FLAC__int128)data[i-31]; /* Falls through. */
			case 30: sum += (FLAC__int128)qlp_coeff[29] * (FLAC__int128)data[i-30]; /* Falls through. */
			case 29: sum += (FLAC__int128)qlp_coeff[28] * (FLAC__int128)data[i-29]; /* Falls through. */
			case 28: sum += (FLAC__int128)qlp_coeff[27] * (FLAC__int128)data[i-28]; /* Falls through. */
			case 27: sum += (FLAC__int128)qlp_coeff[26] * (FLAC__int128)data[i-27]; /* Falls through. */
			case 26: sum += (FLAC__int128)qlp_coeff[25] * (FLAC__int128)data[i-26]; /* Falls through. */
			case 25: sum += (FLAC__int128)qlp_coeff[24] * (FLAC__int128)data[i-25]; /* Falls through. */
			case 24: sum += (FLAC__int128)qlp_coeff[23] * (FLAC__int128)data[i-24]; /* Falls through. */
			case 23: sum += (FLAC__int128)qlp_coeff[22] * (FLAC__int128)data[i-23]; /* Falls through. */
			case 22: sum += (FLAC__int128)qlp_coeff[21] * (FLAC__int128)data[i-22]; /* Falls through. */
			case 21: sum += (FLAC__int128)qlp_coeff[20] * (FLAC__int128)data[i-21]; /* Falls through. */
			case 20: sum += (FLAC__int128)qlp_coeff[19] * (FLAC__int128)data[i-20]; /* Falls through. */
			case 19: sum += (FLAC__int128)qlp_coeff[18] * (FLAC__int128)data[i-19]; /* Falls through. */
			case 18: sum += (FLAC__int128)qlp_coeff[17] * (FLAC__int128)data[i-18]; /* Falls through. */
			case 17: sum += (FLAC__int128)qlp_coeff[16] * (FLAC__int128)data[i-17]; /* Falls through. */
			case 16: sum += (FLAC__int128)qlp_coeff[15] * (FLAC__int128)data[i-16]; /* Falls through. */
			case 15: sum += (FLAC__int128)qlp_coeff[14] * (FLAC__int128)data[i-15]; /* Falls through. */
			case 14: sum += (FLAC__int128)qlp_coeff[13] * (FLAC__int128)data[i-14]; /* Falls through. */
			case 13: sum += (FLAC__int128)qlp_coeff[12] * (FLAC__int128)data[i-13]; /* Falls through. */
			case 12: sum += (FLAC__int128)qlp_coeff[11] * (FLAC__int128)data[i-12]; /* Falls through. */
			case 11: sum += (FLAC__int128)qlp_coeff[10] * (FLAC__int128)data[i-11]; /* Falls through. */
			case 10: sum += (FLAC__int128)qlp_coeff[ 9] * (FLAC__int128)data[i-10]; /* Falls through. */
			case  9: sum += (FLAC__int128)qlp_coeff[ 8] * (FLAC__int128)data[i- 9]; /* Falls through. */
			case  8: sum += (FLAC__int128)qlp_coeff[ 7] * (FLAC__int128)data[i- 8]; /* Falls through. */
			case  7: sum += (FLAC__int128)qlp_coeff[ 6] * (FLAC__int128)data[i- 7]; /* Falls through. */
			case  6: sum += (FLAC__int128)qlp_coeff[ 5] * (FLAC__int128)data[i- 6]; /* Falls through. */
			case  5: sum += (FLAC__int128)qlp_coeff[ 4] * (FLAC__int128)data[i- 5]; /* Falls through. */
			case  4: sum += (FLAC__int128)qlp_coeff[ 3] * (FLAC__int128)data[i- 4]; /* Falls through. */
			case  3: sum += (FLAC__int128)qlp_coeff[ 2] * (FLAC__int128)data[i- 3]; /* Falls through. */
			case  2: sum += (FLAC__int128)qlp_coeff[ 1] * (FLAC__int128)data[i- 2]; /* Falls through. */
			case  1: sum += (FLAC__int128)qlp_coeff[ 0] * (FLAC__int128)data[i- 1];
		}
		residual[i] = data[i] - (FLAC__int64)(sum >> lp_quantization);
	}
	_mm256_zeroupper();
	return true;
#else
	(void)data; (void)data_len; (void)qlp_coeff; (void)order; (void)lp_quantization; (void)residual; (void)subframe_bps;
	return false;
#endif
}
#endif /* FLAC__AVX2_SUPPORTED */
#endif /* (FLAC__CPU_IA32 || FLAC__CPU_X86_64) && FLAC__HAS_X86INTRIN */
#endif /* FLAC__NO_ASM */
#endif /* FLAC__INTEGER_ONLY_LIBRARY */
