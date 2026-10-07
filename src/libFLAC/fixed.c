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

#include <float.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include "share/compat.h"
#include "private/bitmath.h"
#include "private/macros.h"
#include "private/fixed.h"
#include "FLAC/assert.h"

#ifdef local_abs
#undef local_abs
#endif
#define local_abs(x) ((uint32_t)((x)<0? -(x) : (x)))

#ifdef local_abs64
#undef local_abs64
#endif
#define local_abs64(x) ((FLAC__uint64)((x)<0? -(x) : (x)))



#ifdef FLAC__INTEGER_ONLY_LIBRARY
/*
 * ln(x) ~= 1.442695041 * ln(2) * (x - 1) / (x + 1)
 *
 * This function calculates residual bits per sample for fixed predictor
 * from the total error of that predictor.
 */
static FLAC__fixedpoint local__compute_rbps_integerized(FLAC__uint32 err, FLAC__uint32 n)
{
	FLAC__uint32 rbps;
	int fracbits;

	FLAC__ASSERT(sizeof(FLAC__fixedpoint) == 4);

	if(err == 0)
		return 0;

	/*
	 * ln(2) * err / n
	 */
	fracbits = (int)FLAC__bitmath_ilog2(err) - (int)FLAC__bitmath_ilog2(n);
	if(fracbits < 0)
		err <<= -fracbits;
	else
		n <<= fracbits;

	err = (err * (FLAC__uint32)0xb172) / n;
	fracbits += 16;

	/*
	 * log2(err)
	 */
	rbps = (FLAC__bitmath_ilog2(err) << 16);
	fracbits -= 16;
	if(fracbits < 0)
		err <<= -fracbits;
	else
		err >>= fracbits;

	rbps |= (err & 0xffff);

	if(fracbits < 16)
		rbps <<= (16 - fracbits);
	else if(fracbits > 16)
		rbps >>= (fracbits - 16);

	return rbps;
}

static FLAC__fixedpoint local__compute_rbps_wide_integerized(FLAC__uint64 err, FLAC__uint32 n)
{
	FLAC__uint32 rbps;
	int fracbits;

	FLAC__ASSERT(sizeof(FLAC__fixedpoint) == 4);

	if(err == 0)
		return 0;

	/*
	 * ln(2) * err / n
	 */
	fracbits = (int)FLAC__bitmath_ilog2_wide(err) - (int)FLAC__bitmath_ilog2(n);
	if(fracbits < 0)
		err <<= -fracbits;
	else
		n <<= fracbits;

	err = (err * (FLAC__uint64)0xb172) / n;
	fracbits += 16;

	/*
	 * log2(err)
	 */
	rbps = (FLAC__bitmath_ilog2_wide(err) << 16);
	fracbits -= 16;
	if(fracbits < 0)
		err <<= -fracbits;
	else
		err >>= fracbits;

	rbps |= (err & 0xffff);

	if(fracbits < 16)
		rbps <<= (16 - fracbits);
	else if(fracbits > 16)
		rbps >>= (fracbits - 16);

	return rbps;
}
#endif

#ifndef FLAC__INTEGER_ONLY_LIBRARY
uint32_t FLAC__fixed_compute_best_predictor(const FLAC__int32 data[], uint32_t data_len, float residual_bits_per_sample[FLAC__MAX_FIXED_ORDER+1])
#else
uint32_t FLAC__fixed_compute_best_predictor(const FLAC__int32 data[], uint32_t data_len, FLAC__fixedpoint residual_bits_per_sample[FLAC__MAX_FIXED_ORDER+1])
#endif
{
	FLAC__int32 last_error_0 = data[-1];
	FLAC__int32 last_error_1 = data[-1] - data[-2];
	FLAC__int32 last_error_2 = last_error_1 - (data[-2] - data[-3]);
	FLAC__int32 last_error_3 = last_error_2 - (data[-2] - 2*data[-3] + data[-4]);
	FLAC__int32 error, save;
	FLAC__uint32 total_error_0 = 0, total_error_1 = 0, total_error_2 = 0, total_error_3 = 0, total_error_4 = 0;
	uint32_t i, order;

	for(i = 0; i < data_len; i++) {
		error  = data[i];
		total_error_0 += local_abs(error);

		save = error - last_error_0;
		last_error_0 = error;
		error = save;
		total_error_1 += local_abs(error);

		save = error - last_error_1;
		last_error_1 = error;
		error = save;
		total_error_2 += local_abs(error);

		save = error - last_error_2;
		last_error_2 = error;
		error = save;
		total_error_3 += local_abs(error);

		save = error - last_error_3;
		last_error_3 = error;
		error = save;
		total_error_4 += local_abs(error);
	}

	if(total_error_0 < flac_min(flac_min(flac_min(total_error_1, total_error_2), total_error_3), total_error_4))
		order = 0;
	else if(total_error_1 < flac_min(flac_min(total_error_2, total_error_3), total_error_4))
		order = 1;
	else if(total_error_2 < flac_min(total_error_3, total_error_4))
		order = 2;
	else if(total_error_3 < total_error_4)
		order = 3;
	else
		order = 4;

#ifndef FLAC__INTEGER_ONLY_LIBRARY
	residual_bits_per_sample[0] = (float)((total_error_0 > 0) ? log((double)M_LN2 * (double)total_error_0 / (double)data_len) / (double)M_LN2 : 0.0);
	residual_bits_per_sample[1] = (float)((total_error_1 > 0) ? log((double)M_LN2 * (double)total_error_1 / (double)data_len) / (double)M_LN2 : 0.0);
	residual_bits_per_sample[2] = (float)((total_error_2 > 0) ? log((double)M_LN2 * (double)total_error_2 / (double)data_len) / (double)M_LN2 : 0.0);
	residual_bits_per_sample[3] = (float)((total_error_3 > 0) ? log((double)M_LN2 * (double)total_error_3 / (double)data_len) / (double)M_LN2 : 0.0);
	residual_bits_per_sample[4] = (float)((total_error_4 > 0) ? log((double)M_LN2 * (double)total_error_4 / (double)data_len) / (double)M_LN2 : 0.0);
#else
	residual_bits_per_sample[0] = (total_error_0 > 0) ? local__compute_rbps_integerized(total_error_0, data_len) : 0;
	residual_bits_per_sample[1] = (total_error_1 > 0) ? local__compute_rbps_integerized(total_error_1, data_len) : 0;
	residual_bits_per_sample[2] = (total_error_2 > 0) ? local__compute_rbps_integerized(total_error_2, data_len) : 0;
	residual_bits_per_sample[3] = (total_error_3 > 0) ? local__compute_rbps_integerized(total_error_3, data_len) : 0;
	residual_bits_per_sample[4] = (total_error_4 > 0) ? local__compute_rbps_integerized(total_error_4, data_len) : 0;
#endif

	return order;
}

#ifndef FLAC__INTEGER_ONLY_LIBRARY
uint32_t FLAC__fixed_compute_best_predictor_wide(const FLAC__int32 data[], uint32_t data_len, float residual_bits_per_sample[FLAC__MAX_FIXED_ORDER+1])
#else
uint32_t FLAC__fixed_compute_best_predictor_wide(const FLAC__int32 data[], uint32_t data_len, FLAC__fixedpoint residual_bits_per_sample[FLAC__MAX_FIXED_ORDER+1])
#endif
{
	FLAC__int32 last_error_0 = data[-1];
	FLAC__int32 last_error_1 = data[-1] - data[-2];
	FLAC__int32 last_error_2 = last_error_1 - (data[-2] - data[-3]);
	FLAC__int32 last_error_3 = last_error_2 - (data[-2] - 2*data[-3] + data[-4]);
	FLAC__int32 error, save;
	FLAC__uint64 total_error_0 = 0, total_error_1 = 0, total_error_2 = 0, total_error_3 = 0, total_error_4 = 0;
	uint32_t i, order;

	for(i = 0; i < data_len; i++) {
		error  = data[i];
		total_error_0 += local_abs(error);

		save = error - last_error_0;
		last_error_0 = error;
		error = save;
		total_error_1 += local_abs(error);

		save = error - last_error_1;
		last_error_1 = error;
		error = save;
		total_error_2 += local_abs(error);

		save = error - last_error_2;
		last_error_2 = error;
		error = save;
		total_error_3 += local_abs(error);

		save = error - last_error_3;
		last_error_3 = error;
		error = save;
		total_error_4 += local_abs(error);
	}

	if(total_error_0 < flac_min(flac_min(flac_min(total_error_1, total_error_2), total_error_3), total_error_4))
		order = 0;
	else if(total_error_1 < flac_min(flac_min(total_error_2, total_error_3), total_error_4))
		order = 1;
	else if(total_error_2 < flac_min(total_error_3, total_error_4))
		order = 2;
	else if(total_error_3 < total_error_4)
		order = 3;
	else
		order = 4;

#ifndef FLAC__INTEGER_ONLY_LIBRARY
	residual_bits_per_sample[0] = (float)((total_error_0 > 0) ? log((double)M_LN2 * (double)total_error_0 / (double)data_len) / (double)M_LN2 : 0.0);
	residual_bits_per_sample[1] = (float)((total_error_1 > 0) ? log((double)M_LN2 * (double)total_error_1 / (double)data_len) / (double)M_LN2 : 0.0);
	residual_bits_per_sample[2] = (float)((total_error_2 > 0) ? log((double)M_LN2 * (double)total_error_2 / (double)data_len) / (double)M_LN2 : 0.0);
	residual_bits_per_sample[3] = (float)((total_error_3 > 0) ? log((double)M_LN2 * (double)total_error_3 / (double)data_len) / (double)M_LN2 : 0.0);
	residual_bits_per_sample[4] = (float)((total_error_4 > 0) ? log((double)M_LN2 * (double)total_error_4 / (double)data_len) / (double)M_LN2 : 0.0);
#else
	residual_bits_per_sample[0] = (total_error_0 > 0) ? local__compute_rbps_wide_integerized(total_error_0, data_len) : 0;
	residual_bits_per_sample[1] = (total_error_1 > 0) ? local__compute_rbps_wide_integerized(total_error_1, data_len) : 0;
	residual_bits_per_sample[2] = (total_error_2 > 0) ? local__compute_rbps_wide_integerized(total_error_2, data_len) : 0;
	residual_bits_per_sample[3] = (total_error_3 > 0) ? local__compute_rbps_wide_integerized(total_error_3, data_len) : 0;
	residual_bits_per_sample[4] = (total_error_4 > 0) ? local__compute_rbps_wide_integerized(total_error_4, data_len) : 0;
#endif

	return order;
}

#ifndef FLAC__INTEGER_ONLY_LIBRARY
#define CHECK_ORDER_IS_VALID(n) \
	if(order_##n##_is_valid) { \
		residual_bits_per_sample[n] = (float)((total_error_##n > 0) ? log((double)M_LN2 * (double)total_error_##n / (double)data_len) / (double)M_LN2 : 0.0); \
		if(total_error_##n < smallest_error) { \
			order = n; \
			smallest_error = total_error_##n; \
		} \
	} \
	else { \
		residual_bits_per_sample[n] = FLT_MAX; \
	}
#else
#define CHECK_ORDER_IS_VALID(n) \
	if(order_##n##_is_valid) { \
		residual_bits_per_sample[n] = (total_error_##n > 0) ? local__compute_rbps_wide_integerized(total_error_##n, data_len) : 0; \
		if(total_error_##n < smallest_error) { \
			order = n; \
			smallest_error = total_error_##n; \
		} \
	} \
	else { \
		residual_bits_per_sample[n] = 34 * FLAC__FP_ONE; \
	}
#endif

#ifndef FLAC__INTEGER_ONLY_LIBRARY
uint32_t FLAC__fixed_compute_best_predictor_limit_residual(const FLAC__int32 data[], uint32_t data_len, float residual_bits_per_sample[FLAC__MAX_FIXED_ORDER+1])
#else
uint32_t FLAC__fixed_compute_best_predictor_limit_residual(const FLAC__int32 data[], uint32_t data_len, FLAC__fixedpoint residual_bits_per_sample[FLAC__MAX_FIXED_ORDER+1])
#endif
{
	FLAC__uint64 total_error_0 = 0, total_error_1 = 0, total_error_2 = 0, total_error_3 = 0, total_error_4 = 0, smallest_error = UINT64_MAX;
	FLAC__uint64 error_0, error_1, error_2, error_3, error_4;
	FLAC__bool order_0_is_valid = true, order_1_is_valid = true, order_2_is_valid = true, order_3_is_valid = true, order_4_is_valid = true;
	uint32_t order = 0;
	int i;

	for(i = -4; i < (int)data_len; i++) {
		error_0 = local_abs64((FLAC__int64)data[i]);
		error_1 = (i > -4) ? local_abs64((FLAC__int64)data[i] - (FLAC__int64)data[i-1]) : 0 ;
		error_2 = (i > -3) ? local_abs64((FLAC__int64)data[i] - 2 * (FLAC__int64)data[i-1] + (FLAC__int64)data[i-2]) : 0;
		error_3 = (i > -2) ? local_abs64((FLAC__int64)data[i] - 3 * (FLAC__int64)data[i-1] + 3 * (FLAC__int64)data[i-2] - (FLAC__int64)data[i-3]) : 0;
		error_4 = (i > -1) ? local_abs64((FLAC__int64)data[i] - 4 * (FLAC__int64)data[i-1] + 6 * (FLAC__int64)data[i-2] - 4 * (FLAC__int64)data[i-3] + (FLAC__int64)data[i-4]) : 0;

		total_error_0 += error_0;
		total_error_1 += error_1;
		total_error_2 += error_2;
		total_error_3 += error_3;
		total_error_4 += error_4;

		if(error_0 > INT32_MAX)
			order_0_is_valid = false;
		if(error_1 > INT32_MAX)
			order_1_is_valid = false;
		if(error_2 > INT32_MAX)
			order_2_is_valid = false;
		if(error_3 > INT32_MAX)
			order_3_is_valid = false;
		if(error_4 > INT32_MAX)
			order_4_is_valid = false;
	}

	CHECK_ORDER_IS_VALID(0);
	CHECK_ORDER_IS_VALID(1);
	CHECK_ORDER_IS_VALID(2);
	CHECK_ORDER_IS_VALID(3);
	CHECK_ORDER_IS_VALID(4);

	return order;
}

#ifndef FLAC__INTEGER_ONLY_LIBRARY
uint32_t FLAC__fixed_compute_best_predictor_limit_residual_64bit(const FLAC__int64 data[], uint32_t data_len, float residual_bits_per_sample[FLAC__MAX_FIXED_ORDER+1])
#else
uint32_t FLAC__fixed_compute_best_predictor_limit_residual_64bit(const FLAC__int64 data[], uint32_t data_len, FLAC__fixedpoint residual_bits_per_sample[FLAC__MAX_FIXED_ORDER+1])
#endif
{
	FLAC__uint64 total_error_0 = 0, total_error_1 = 0, total_error_2 = 0, total_error_3 = 0, total_error_4 = 0, smallest_error = UINT64_MAX;
	FLAC__uint64 error_0, error_1, error_2, error_3, error_4;
	FLAC__bool order_0_is_valid = true, order_1_is_valid = true, order_2_is_valid = true, order_3_is_valid = true, order_4_is_valid = true;
	uint32_t order = 0;
	int i;

#if FLAC__HAS_INT128
	for(i = -4; i < (int)data_len; i++) {
		FLAC__int128 d_0 = (FLAC__int128)data[i];
		FLAC__int128 d_1 = (i > -4) ? (FLAC__int128)data[i-1] : 0;
		FLAC__int128 d_2 = (i > -3) ? (FLAC__int128)data[i-2] : 0;
		FLAC__int128 d_3 = (i > -2) ? (FLAC__int128)data[i-3] : 0;
		FLAC__int128 d_4 = (i > -1) ? (FLAC__int128)data[i-4] : 0;

		FLAC__int128 err_0 = d_0;
		FLAC__int128 err_1 = (i > -4) ? (d_0 - d_1) : 0;
		FLAC__int128 err_2 = (i > -3) ? (d_0 - 2 * d_1 + d_2) : 0;
		FLAC__int128 err_3 = (i > -2) ? (d_0 - 3 * d_1 + 3 * d_2 - d_3) : 0;
		FLAC__int128 err_4 = (i > -1) ? (d_0 - 4 * d_1 + 6 * d_2 - 4 * d_3 + d_4) : 0;

		error_0 = (err_0 < 0) ? (FLAC__uint64)(-err_0) : (FLAC__uint64)err_0;
		error_1 = (err_1 < 0) ? (FLAC__uint64)(-err_1) : (FLAC__uint64)err_1;
		error_2 = (err_2 < 0) ? (FLAC__uint64)(-err_2) : (FLAC__uint64)err_2;
		error_3 = (err_3 < 0) ? (FLAC__uint64)(-err_3) : (FLAC__uint64)err_3;
		error_4 = (err_4 < 0) ? (FLAC__uint64)(-err_4) : (FLAC__uint64)err_4;

		total_error_0 += error_0;
		total_error_1 += error_1;
		total_error_2 += error_2;
		total_error_3 += error_3;
		total_error_4 += error_4;

		if(error_0 > INT32_MAX || err_0 > INT32_MAX || err_0 < -((FLAC__int128)INT32_MAX))
			order_0_is_valid = false;
		if(error_1 > INT32_MAX || err_1 > INT32_MAX || err_1 < -((FLAC__int128)INT32_MAX))
			order_1_is_valid = false;
		if(error_2 > INT32_MAX || err_2 > INT32_MAX || err_2 < -((FLAC__int128)INT32_MAX))
			order_2_is_valid = false;
		if(error_3 > INT32_MAX || err_3 > INT32_MAX || err_3 < -((FLAC__int128)INT32_MAX))
			order_3_is_valid = false;
		if(error_4 > INT32_MAX || err_4 > INT32_MAX || err_4 < -((FLAC__int128)INT32_MAX))
			order_4_is_valid = false;
	}
#else
	for(i = -4; i < (int)data_len; i++) {
		FLAC__int64 d_0 = data[i];
		FLAC__int64 d_1 = (i > -4) ? data[i-1] : 0;
		FLAC__int64 d_2 = (i > -3) ? data[i-2] : 0;
		FLAC__int64 d_3 = (i > -2) ? data[i-3] : 0;
		FLAC__int64 d_4 = (i > -1) ? data[i-4] : 0;

		FLAC__int64 err_0 = d_0;
		FLAC__int64 err_1 = (i > -4) ? (d_0 - d_1) : 0;
		FLAC__int64 err_2 = (i > -3) ? (d_0 - 2 * d_1 + d_2) : 0;
		FLAC__int64 err_3 = (i > -2) ? (d_0 - 3 * d_1 + 3 * d_2 - d_3) : 0;
		FLAC__int64 err_4 = (i > -1) ? (d_0 - 4 * d_1 + 6 * d_2 - 4 * d_3 + d_4) : 0;

		error_0 = (err_0 < 0) ? (FLAC__uint64)(-err_0) : (FLAC__uint64)err_0;
		error_1 = (err_1 < 0) ? (FLAC__uint64)(-err_1) : (FLAC__uint64)err_1;
		error_2 = (err_2 < 0) ? (FLAC__uint64)(-err_2) : (FLAC__uint64)err_2;
		error_3 = (err_3 < 0) ? (FLAC__uint64)(-err_3) : (FLAC__uint64)err_3;
		error_4 = (err_4 < 0) ? (FLAC__uint64)(-err_4) : (FLAC__uint64)err_4;

		total_error_0 += error_0;
		total_error_1 += error_1;
		total_error_2 += error_2;
		total_error_3 += error_3;
		total_error_4 += error_4;

		if(error_0 > INT32_MAX || err_0 > INT32_MAX || err_0 < -((FLAC__int64)INT32_MAX))
			order_0_is_valid = false;
		if(error_1 > INT32_MAX || err_1 > INT32_MAX || err_1 < -((FLAC__int64)INT32_MAX))
			order_1_is_valid = false;
		if(error_2 > INT32_MAX || err_2 > INT32_MAX || err_2 < -((FLAC__int64)INT32_MAX))
			order_2_is_valid = false;
		if(error_3 > INT32_MAX || err_3 > INT32_MAX || err_3 < -((FLAC__int64)INT32_MAX))
			order_3_is_valid = false;
		if(error_4 > INT32_MAX || err_4 > INT32_MAX || err_4 < -((FLAC__int64)INT32_MAX))
			order_4_is_valid = false;
	}
#endif

	CHECK_ORDER_IS_VALID(0);
	CHECK_ORDER_IS_VALID(1);
	CHECK_ORDER_IS_VALID(2);
	CHECK_ORDER_IS_VALID(3);
	CHECK_ORDER_IS_VALID(4);

	return order;
}

void FLAC__fixed_compute_residual(const FLAC__int32 data[], uint32_t data_len, uint32_t order, FLAC__int32 residual[])
{
	const int idata_len = (int)data_len;
	int i;

	switch(order) {
		case 0:
			for(i = 0; i < idata_len; i++)
				residual[i] = data[i];
			break;
		case 1:
			for(i = 0; i < idata_len; i++)
				residual[i] = data[i] - data[i-1];
			break;
		case 2:
			for(i = 0; i < idata_len; i++)
				residual[i] = data[i] - 2*data[i-1] + data[i-2];
			break;
		case 3:
			for(i = 0; i < idata_len; i++)
				residual[i] = data[i] - 3*data[i-1] + 3*data[i-2] - data[i-3];
			break;
		case 4:
			for(i = 0; i < idata_len; i++)
				residual[i] = data[i] - 4*data[i-1] + 6*data[i-2] - 4*data[i-3] + data[i-4];
			break;
		default:
			FLAC__ASSERT(0);
	}
}

void FLAC__fixed_compute_residual_wide(const FLAC__int32 data[], uint32_t data_len, uint32_t order, FLAC__int32 residual[])
{
	const int idata_len = (int)data_len;
	int i;

	switch(order) {
		case 0:
			for(i = 0; i < idata_len; i++)
				residual[i] = data[i];
			break;
		case 1:
			for(i = 0; i < idata_len; i++)
				residual[i] = (FLAC__int32)((FLAC__int64)data[i] - (FLAC__int64)data[i-1]);
			break;
		case 2:
			for(i = 0; i < idata_len; i++)
				residual[i] = (FLAC__int32)((FLAC__int64)data[i] - 2*(FLAC__int64)data[i-1] + (FLAC__int64)data[i-2]);
			break;
		case 3:
			for(i = 0; i < idata_len; i++)
				residual[i] = (FLAC__int32)((FLAC__int64)data[i] - 3*(FLAC__int64)data[i-1] + 3*(FLAC__int64)data[i-2] - (FLAC__int64)data[i-3]);
			break;
		case 4:
			for(i = 0; i < idata_len; i++)
				residual[i] = (FLAC__int32)((FLAC__int64)data[i] - 4*(FLAC__int64)data[i-1] + 6*(FLAC__int64)data[i-2] - 4*(FLAC__int64)data[i-3] + (FLAC__int64)data[i-4]);
			break;
		default:
			FLAC__ASSERT(0);
	}
}

void FLAC__fixed_compute_residual_wide_64bit(const FLAC__int64 data[], uint32_t data_len, uint32_t order, FLAC__int32 residual[])
{
	const int idata_len = (int)data_len;
	int i;

	switch(order) {
		case 0:
			for(i = 0; i < idata_len; i++)
				residual[i] = (FLAC__int32)data[i];
			break;
		case 1:
			for(i = 0; i < idata_len; i++)
				residual[i] = (FLAC__int32)(data[i] - data[i-1]);
			break;
		case 2:
			for(i = 0; i < idata_len; i++)
#if FLAC__HAS_INT128
				residual[i] = (FLAC__int32)((FLAC__int128)data[i] - 2*(FLAC__int128)data[i-1] + (FLAC__int128)data[i-2]);
#else
				residual[i] = (FLAC__int32)((FLAC__int64)data[i] - 2*(FLAC__int64)data[i-1] + (FLAC__int64)data[i-2]);
#endif
			break;
		case 3:
			for(i = 0; i < idata_len; i++)
#if FLAC__HAS_INT128
				residual[i] = (FLAC__int32)((FLAC__int128)data[i] - 3*(FLAC__int128)data[i-1] + 3*(FLAC__int128)data[i-2] - (FLAC__int128)data[i-3]);
#else
				residual[i] = (FLAC__int32)((FLAC__int64)data[i] - 3*(FLAC__int64)data[i-1] + 3*(FLAC__int64)data[i-2] - (FLAC__int64)data[i-3]);
#endif
			break;
		case 4:
			for(i = 0; i < idata_len; i++)
#if FLAC__HAS_INT128
				residual[i] = (FLAC__int32)((FLAC__int128)data[i] - 4*(FLAC__int128)data[i-1] + 6*(FLAC__int128)data[i-2] - 4*(FLAC__int128)data[i-3] + (FLAC__int128)data[i-4]);
#else
				residual[i] = (FLAC__int32)((FLAC__int64)data[i] - 4*(FLAC__int64)data[i-1] + 6*(FLAC__int64)data[i-2] - 4*(FLAC__int64)data[i-3] + (FLAC__int64)data[i-4]);
#endif
			break;
		default:
			FLAC__ASSERT(0);
	}
}

void FLAC__fixed_restore_signal(const FLAC__int32 residual[], uint32_t data_len, uint32_t order, FLAC__int32 data[])
{
	int i, idata_len = (int)data_len;

	switch(order) {
		case 0:
			memcpy(data, residual, sizeof(FLAC__int32)*idata_len);
			break;
		case 1:
			for(i = 0; i < idata_len; i++)
				data[i] = residual[i] + data[i-1];
			break;
		case 2:
			for(i = 0; i < idata_len; i++)
				data[i] = residual[i] + 2*data[i-1] - data[i-2];
			break;
		case 3:
			for(i = 0; i < idata_len; i++)
				data[i] = residual[i] + 3*data[i-1] - 3*data[i-2] + data[i-3];
			break;
		case 4:
			for(i = 0; i < idata_len; i++)
				data[i] = residual[i] + 4*data[i-1] - 6*data[i-2] + 4*data[i-3] - data[i-4];
			break;
		default:
			FLAC__ASSERT(0);
	}
}

void FLAC__fixed_restore_signal_wide(const FLAC__int32 residual[], uint32_t data_len, uint32_t order, FLAC__int32 data[])
{
	int i, idata_len = (int)data_len;

	switch(order) {
		case 0:
			memcpy(data, residual, sizeof(FLAC__int32)*idata_len);
			break;
		case 1:
			for(i = 0; i < idata_len; i++)
				data[i] = residual[i] + data[i-1];
			break;
		case 2:
			for(i = 0; i < idata_len; i++)
				data[i] = (FLAC__int32)((FLAC__int64)residual[i] + 2*(FLAC__int64)data[i-1] - (FLAC__int64)data[i-2]);
			break;
		case 3:
			for(i = 0; i < idata_len; i++)
				data[i] = (FLAC__int32)((FLAC__int64)residual[i] + 3*(FLAC__int64)data[i-1] - 3*(FLAC__int64)data[i-2] + (FLAC__int64)data[i-3]);
			break;
		case 4:
			for(i = 0; i < idata_len; i++)
				data[i] = (FLAC__int32)((FLAC__int64)residual[i] + 4*(FLAC__int64)data[i-1] - 6*(FLAC__int64)data[i-2] + 4*(FLAC__int64)data[i-3] - (FLAC__int64)data[i-4]);
			break;
		default:
			FLAC__ASSERT(0);
	}
}

#if defined __GNUC__
/* As function below uses integer overflow deliberately, disable
 * sanitizing for this. Function is only used on data that has passed
 * verification by fixed_compute_best_predictor, so there is no
 * potential for security problems */
__attribute__((no_sanitize("signed-integer-overflow")))
#endif
void FLAC__fixed_restore_signal_wide_64bit(const FLAC__int32 residual[], uint32_t data_len, uint32_t order, FLAC__int64 data[])
{
	int i, idata_len = (int)data_len;

	switch(order) {
		case 0:
			for(i = 0; i < idata_len; i++)
				data[i] = residual[i];
			break;
		case 1:
			for(i = 0; i < idata_len; i++)
				data[i] = (FLAC__int64)residual[i] + data[i-1];
			break;
		case 2:
			for(i = 0; i < idata_len; i++)
#if FLAC__HAS_INT128
				data[i] = (FLAC__int64)((FLAC__int128)residual[i] + 2*(FLAC__int128)data[i-1] - (FLAC__int128)data[i-2]);
#else
				data[i] = (FLAC__int64)((FLAC__uint64)residual[i] + 2*(FLAC__uint64)data[i-1] - (FLAC__uint64)data[i-2]);
#endif
			break;
		case 3:
			for(i = 0; i < idata_len; i++)
#if FLAC__HAS_INT128
				data[i] = (FLAC__int64)((FLAC__int128)residual[i] + 3*(FLAC__int128)data[i-1] - 3*(FLAC__int128)data[i-2] + (FLAC__int128)data[i-3]);
#else
				data[i] = (FLAC__int64)((FLAC__uint64)residual[i] + 3*(FLAC__uint64)data[i-1] - 3*(FLAC__uint64)data[i-2] + (FLAC__uint64)data[i-3]);
#endif
			break;
		case 4:
			for(i = 0; i < idata_len; i++)
#if FLAC__HAS_INT128
				data[i] = (FLAC__int64)((FLAC__int128)residual[i] + 4*(FLAC__int128)data[i-1] - 6*(FLAC__int128)data[i-2] + 4*(FLAC__int128)data[i-3] - (FLAC__int128)data[i-4]);
#else
				data[i] = (FLAC__int64)((FLAC__uint64)residual[i] + 4*(FLAC__uint64)data[i-1] - 6*(FLAC__uint64)data[i-2] + 4*(FLAC__uint64)data[i-3] - (FLAC__uint64)data[i-4]);
#endif
			break;
		default:
			FLAC__ASSERT(0);
	}
}

#ifndef FLAC__INTEGER_ONLY_LIBRARY
uint32_t FLAC__fixed_compute_best_predictor_limit_residual_64bit_res64(const FLAC__int64 data[], uint32_t data_len, uint32_t subframe_bps, float residual_bits_per_sample[FLAC__MAX_FIXED_ORDER+1])
#else
uint32_t FLAC__fixed_compute_best_predictor_limit_residual_64bit_res64(const FLAC__int64 data[], uint32_t data_len, uint32_t subframe_bps, FLAC__fixedpoint residual_bits_per_sample[FLAC__MAX_FIXED_ORDER+1])
#endif
{
	FLAC__uint64 total_error_0 = 0, total_error_1 = 0, total_error_2 = 0, total_error_3 = 0, total_error_4 = 0, smallest_error = UINT64_MAX;
	FLAC__uint64 error_0, error_1, error_2, error_3, error_4;
	FLAC__bool order_0_is_valid = true, order_1_is_valid = true, order_2_is_valid = true, order_3_is_valid = true, order_4_is_valid = true;
	uint32_t order = 0;
	int i;
	(void)subframe_bps;

#if FLAC__HAS_INT128
	for(i = -4; i < (int)data_len; i++) {
		FLAC__int128 d_0 = (FLAC__int128)data[i];
		FLAC__int128 d_1 = (i > -4) ? (FLAC__int128)data[i-1] : 0;
		FLAC__int128 d_2 = (i > -3) ? (FLAC__int128)data[i-2] : 0;
		FLAC__int128 d_3 = (i > -2) ? (FLAC__int128)data[i-3] : 0;
		FLAC__int128 d_4 = (i > -1) ? (FLAC__int128)data[i-4] : 0;

		FLAC__int128 err_0 = d_0;
		FLAC__int128 err_1 = (i > -4) ? (d_0 - d_1) : 0;
		FLAC__int128 err_2 = (i > -3) ? (d_0 - 2 * d_1 + d_2) : 0;
		FLAC__int128 err_3 = (i > -2) ? (d_0 - 3 * d_1 + 3 * d_2 - d_3) : 0;
		FLAC__int128 err_4 = (i > -1) ? (d_0 - 4 * d_1 + 6 * d_2 - 4 * d_3 + d_4) : 0;

		error_0 = (err_0 < 0) ? (FLAC__uint64)(-err_0) : (FLAC__uint64)err_0;
		error_1 = (err_1 < 0) ? (FLAC__uint64)(-err_1) : (FLAC__uint64)err_1;
		error_2 = (err_2 < 0) ? (FLAC__uint64)(-err_2) : (FLAC__uint64)err_2;
		error_3 = (err_3 < 0) ? (FLAC__uint64)(-err_3) : (FLAC__uint64)err_3;
		error_4 = (err_4 < 0) ? (FLAC__uint64)(-err_4) : (FLAC__uint64)err_4;

		total_error_0 += error_0;
		total_error_1 += error_1;
		total_error_2 += error_2;
		total_error_3 += error_3;
		total_error_4 += error_4;

		if(err_0 > INT64_MAX || err_0 < INT64_MIN) order_0_is_valid = false;
		if(err_1 > INT64_MAX || err_1 < INT64_MIN) order_1_is_valid = false;
		if(err_2 > INT64_MAX || err_2 < INT64_MIN) order_2_is_valid = false;
		if(err_3 > INT64_MAX || err_3 < INT64_MIN) order_3_is_valid = false;
		if(err_4 > INT64_MAX || err_4 < INT64_MIN) order_4_is_valid = false;
	}
#else
	for(i = -4; i < (int)data_len; i++) {
		FLAC__int64 d_0 = data[i];
		FLAC__int64 d_1 = (i > -4) ? data[i-1] : 0;
		FLAC__int64 d_2 = (i > -3) ? data[i-2] : 0;
		FLAC__int64 d_3 = (i > -2) ? data[i-3] : 0;
		FLAC__int64 d_4 = (i > -1) ? data[i-4] : 0;

		FLAC__int64 err_0 = d_0;
		FLAC__int64 err_1 = (i > -4) ? (d_0 - d_1) : 0;
		FLAC__int64 err_2 = (i > -3) ? (d_0 - 2 * d_1 + d_2) : 0;
		FLAC__int64 err_3 = (i > -2) ? (d_0 - 3 * d_1 + 3 * d_2 - d_3) : 0;
		FLAC__int64 err_4 = (i > -1) ? (d_0 - 4 * d_1 + 6 * d_2 - 4 * d_3 + d_4) : 0;

		error_0 = (err_0 < 0) ? (FLAC__uint64)(-err_0) : (FLAC__uint64)err_0;
		error_1 = (err_1 < 0) ? (FLAC__uint64)(-err_1) : (FLAC__uint64)err_1;
		error_2 = (err_2 < 0) ? (FLAC__uint64)(-err_2) : (FLAC__uint64)err_2;
		error_3 = (err_3 < 0) ? (FLAC__uint64)(-err_3) : (FLAC__uint64)err_3;
		error_4 = (err_4 < 0) ? (FLAC__uint64)(-err_4) : (FLAC__uint64)err_4;

		total_error_0 += error_0;
		total_error_1 += error_1;
		total_error_2 += error_2;
		total_error_3 += error_3;
		total_error_4 += error_4;
	}
#endif

	CHECK_ORDER_IS_VALID(0);
	CHECK_ORDER_IS_VALID(1);
	CHECK_ORDER_IS_VALID(2);
	CHECK_ORDER_IS_VALID(3);
	CHECK_ORDER_IS_VALID(4);

	return order;
}

#if FLAC__HAS_INT128
#ifndef FLAC__INTEGER_ONLY_LIBRARY
uint32_t FLAC__fixed_compute_best_predictor_limit_residual_128bit(const FLAC__int128 data[], uint32_t data_len, uint32_t subframe_bps, float residual_bits_per_sample[FLAC__MAX_FIXED_ORDER+1])
#else
uint32_t FLAC__fixed_compute_best_predictor_limit_residual_128bit(const FLAC__int128 data[], uint32_t data_len, uint32_t subframe_bps, FLAC__fixedpoint residual_bits_per_sample[FLAC__MAX_FIXED_ORDER+1])
#endif
{
	FLAC__uint128 total_error_0 = 0, total_error_1 = 0, total_error_2 = 0, total_error_3 = 0, total_error_4 = 0, smallest_error = ((FLAC__uint128)-1);
	FLAC__uint128 error_0, error_1, error_2, error_3, error_4;
	FLAC__bool order_0_is_valid = true, order_1_is_valid = true, order_2_is_valid = true, order_3_is_valid = true, order_4_is_valid = true;
	uint32_t order = 0;
	int i;
	(void)subframe_bps;

	for(i = -4; i < (int)data_len; i++) {
		FLAC__int128 d_0 = data[i];
		FLAC__int128 d_1 = (i > -4) ? data[i-1] : 0;
		FLAC__int128 d_2 = (i > -3) ? data[i-2] : 0;
		FLAC__int128 d_3 = (i > -2) ? data[i-3] : 0;
		FLAC__int128 d_4 = (i > -1) ? data[i-4] : 0;

		FLAC__int128 err_0 = d_0;
		FLAC__int128 err_1 = (i > -4) ? (d_0 - d_1) : 0;
		FLAC__int128 err_2 = (i > -3) ? (d_0 - 2 * d_1 + d_2) : 0;
		FLAC__int128 err_3 = (i > -2) ? (d_0 - 3 * d_1 + 3 * d_2 - d_3) : 0;
		FLAC__int128 err_4 = (i > -1) ? (d_0 - 4 * d_1 + 6 * d_2 - 4 * d_3 + d_4) : 0;

		error_0 = (err_0 < 0) ? (FLAC__uint128)(-err_0) : (FLAC__uint128)err_0;
		error_1 = (err_1 < 0) ? (FLAC__uint128)(-err_1) : (FLAC__uint128)err_1;
		error_2 = (err_2 < 0) ? (FLAC__uint128)(-err_2) : (FLAC__uint128)err_2;
		error_3 = (err_3 < 0) ? (FLAC__uint128)(-err_3) : (FLAC__uint128)err_3;
		error_4 = (err_4 < 0) ? (FLAC__uint128)(-err_4) : (FLAC__uint128)err_4;

		total_error_0 += error_0;
		total_error_1 += error_1;
		total_error_2 += error_2;
		total_error_3 += error_3;
		total_error_4 += error_4;
	}

	CHECK_ORDER_IS_VALID(0);
	CHECK_ORDER_IS_VALID(1);
	CHECK_ORDER_IS_VALID(2);
	CHECK_ORDER_IS_VALID(3);
	CHECK_ORDER_IS_VALID(4);

	return order;
}
#endif

void FLAC__fixed_compute_residual_64bit(const FLAC__int64 data[], uint32_t data_len, uint32_t order, FLAC__int64 residual[])
{
	int i;
	switch(order) {
		case 0:
			for(i = 0; i < (int)data_len; i++)
				residual[i] = data[i];
			break;
		case 1:
			for(i = 0; i < (int)data_len; i++)
				residual[i] = data[i] - data[i-1];
			break;
		case 2:
			for(i = 0; i < (int)data_len; i++)
				residual[i] = data[i] - 2 * data[i-1] + data[i-2];
			break;
		case 3:
			for(i = 0; i < (int)data_len; i++)
				residual[i] = data[i] - 3 * data[i-1] + 3 * data[i-2] - data[i-3];
			break;
		case 4:
			for(i = 0; i < (int)data_len; i++)
				residual[i] = data[i] - 4 * data[i-1] + 6 * data[i-2] - 4 * data[i-3] + data[i-4];
			break;
		default:
			FLAC__ASSERT(0);
	}
}

#if FLAC__HAS_INT128
void FLAC__fixed_compute_residual_128bit(const FLAC__int128 data[], uint32_t data_len, uint32_t order, FLAC__int128 residual[])
{
	int i;
	switch(order) {
		case 0:
			for(i = 0; i < (int)data_len; i++)
				residual[i] = data[i];
			break;
		case 1:
			for(i = 0; i < (int)data_len; i++)
				residual[i] = data[i] - data[i-1];
			break;
		case 2:
			for(i = 0; i < (int)data_len; i++)
				residual[i] = data[i] - 2 * data[i-1] + data[i-2];
			break;
		case 3:
			for(i = 0; i < (int)data_len; i++)
				residual[i] = data[i] - 3 * data[i-1] + 3 * data[i-2] - data[i-3];
			break;
		case 4:
			for(i = 0; i < (int)data_len; i++)
				residual[i] = data[i] - 4 * data[i-1] + 6 * data[i-2] - 4 * data[i-3] + data[i-4];
			break;
		default:
			FLAC__ASSERT(0);
	}
}
#endif

void FLAC__fixed_restore_signal_64bit(const FLAC__int64 residual[], uint32_t data_len, uint32_t order, FLAC__int64 data[])
{
	int i, idata_len = (int)data_len;
	switch(order) {
		case 0:
			for(i = 0; i < idata_len; i++)
				data[i] = residual[i];
			break;
		case 1:
			for(i = 0; i < idata_len; i++)
				data[i] = residual[i] + data[i-1];
			break;
		case 2:
			for(i = 0; i < idata_len; i++)
#if FLAC__HAS_INT128
				data[i] = (FLAC__int64)((FLAC__int128)residual[i] + 2*(FLAC__int128)data[i-1] - (FLAC__int128)data[i-2]);
#else
				data[i] = (FLAC__int64)((FLAC__uint64)residual[i] + 2*(FLAC__uint64)data[i-1] - (FLAC__uint64)data[i-2]);
#endif
			break;
		case 3:
			for(i = 0; i < idata_len; i++)
#if FLAC__HAS_INT128
				data[i] = (FLAC__int64)((FLAC__int128)residual[i] + 3*(FLAC__int128)data[i-1] - 3*(FLAC__int128)data[i-2] + (FLAC__int128)data[i-3]);
#else
				data[i] = (FLAC__int64)((FLAC__uint64)residual[i] + 3*(FLAC__uint64)data[i-1] - 3*(FLAC__uint64)data[i-2] + (FLAC__uint64)data[i-3]);
#endif
			break;
		case 4:
			for(i = 0; i < idata_len; i++)
#if FLAC__HAS_INT128
				data[i] = (FLAC__int64)((FLAC__int128)residual[i] + 4*(FLAC__int128)data[i-1] - 6*(FLAC__int128)data[i-2] + 4*(FLAC__int128)data[i-3] - (FLAC__int128)data[i-4]);
#else
				data[i] = (FLAC__int64)((FLAC__uint64)residual[i] + 4*(FLAC__uint64)data[i-1] - 6*(FLAC__uint64)data[i-2] + 4*(FLAC__uint64)data[i-3] - (FLAC__uint64)data[i-4]);
#endif
			break;
		default:
			FLAC__ASSERT(0);
	}
}

#if FLAC__HAS_INT128
void FLAC__fixed_restore_signal_wide_128bit(const FLAC__int128 residual[], uint32_t data_len, uint32_t order, FLAC__int128 data[])
{
	int i, idata_len = (int)data_len;
	switch(order) {
		case 0:
			for(i = 0; i < idata_len; i++)
				data[i] = residual[i];
			break;
		case 1:
			for(i = 0; i < idata_len; i++)
				data[i] = residual[i] + data[i-1];
			break;
		case 2:
			for(i = 0; i < idata_len; i++)
				data[i] = residual[i] + 2*data[i-1] - data[i-2];
			break;
		case 3:
			for(i = 0; i < idata_len; i++)
				data[i] = residual[i] + 3*data[i-1] - 3*data[i-2] + data[i-3];
			break;
		case 4:
			for(i = 0; i < idata_len; i++)
				data[i] = residual[i] + 4*data[i-1] - 6*data[i-2] + 4*data[i-3] - data[i-4];
			break;
		default:
			FLAC__ASSERT(0);
	}
}
#endif
