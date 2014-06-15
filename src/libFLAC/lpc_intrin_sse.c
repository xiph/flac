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

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#ifndef FLAC__INTEGER_ONLY_LIBRARY
#ifndef FLAC__NO_ASM
#if (defined FLAC__CPU_IA32 || defined FLAC__CPU_X86_64) && defined FLAC__HAS_X86INTRIN
#include "private/lpc.h"
#ifdef FLAC__SSE_SUPPORTED

#include "FLAC/assert.h"
#include "FLAC/format.h"

#include <xmmintrin.h> /* SSE */

FLAC__SSE_TARGET("sse")
void FLAC__lpc_compute_autocorrelation_intrin_sse_lag_4(const FLAC__real data[], unsigned data_len, unsigned lag, FLAC__real autoc[])
{
	__m128 xmm0, xmm2, xmm5;

	(void) lag;
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

FLAC__SSE_TARGET("sse")
void FLAC__lpc_compute_autocorrelation_intrin_sse_lag_8(const FLAC__real data[], unsigned data_len, unsigned lag, FLAC__real autoc[])
{
	__m128 xmm0, xmm1, xmm2, xmm3, xmm5, xmm6;

	(void) lag;
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

	while(data_len)
	{
		xmm0 = _mm_load1_ps(data++);

		xmm2 = _mm_shuffle_ps(xmm2, xmm2, _MM_SHUFFLE(2,1,0,3));
		xmm3 = _mm_shuffle_ps(xmm3, xmm3, _MM_SHUFFLE(2,1,0,3));
		xmm3 = _mm_move_ss(xmm3, xmm2);
		xmm2 = _mm_move_ss(xmm2, xmm0);

		xmm1 = xmm0;
		xmm1 = _mm_mul_ps(xmm1, xmm3);
		xmm0 = _mm_mul_ps(xmm0, xmm2);
		xmm6 = _mm_add_ps(xmm6, xmm1);
		xmm5 = _mm_add_ps(xmm5, xmm0);

		data_len--;
	}

	_mm_storeu_ps(autoc,   xmm5);
	_mm_storeu_ps(autoc+4, xmm6);
}

FLAC__SSE_TARGET("sse")
void FLAC__lpc_compute_autocorrelation_intrin_sse_lag_12(const FLAC__real data[], unsigned data_len, unsigned lag, FLAC__real autoc[])
{
	__m128 xmm0, xmm1, xmm2, xmm3, xmm4, xmm5, xmm6, xmm7;

	(void) lag;
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

FLAC__SSE_TARGET("sse")
void FLAC__lpc_compute_autocorrelation_intrin_sse_lag_16(const FLAC__real data[], unsigned data_len, unsigned lag, FLAC__real autoc[])
{
	__m128 xmm0, xmm1, xmm2, xmm3, xmm4, xmm5, xmm6, xmm7, xmm8, xmm9;

	(void) lag;
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

#endif /* FLAC__SSE_SUPPORTED */
#endif /* (FLAC__CPU_IA32 || FLAC__CPU_X86_64) && FLAC__HAS_X86INTRIN */
#endif /* FLAC__NO_ASM */
#endif /* FLAC__INTEGER_ONLY_LIBRARY */
