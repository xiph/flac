/* libFLAC - Free Lossless Audio Codec library
 * Copyright (C) 2000,2001  Josh Coalson
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA  02111-1307, USA.
 */

#include <assert.h>
#include <math.h>
#include "private/fixed.h"

#ifndef M_LN2
/* math.h in VC++ doesn't seem to have this (how Microsoft is that?) */
#define M_LN2 0.69314718055994530942
#endif

#ifdef min
#undef min
#endif
#define min(x,y) ((x) < (y)? (x) : (y))

#ifdef local_abs
#undef local_abs
#endif
#define local_abs(x) ((unsigned)((x)<0? -(x) : (x)))

unsigned FLAC__fixed_compute_best_predictor(const int32 data[], unsigned data_len, real residual_bits_per_sample[FLAC__MAX_FIXED_ORDER+1])
{
	int32 last_error_0 = data[-1];
	int32 last_error_1 = data[-1] - data[-2];
	int32 last_error_2 = last_error_1 - (data[-2] - data[-3]);
	int32 last_error_3 = last_error_2 - (data[-2] - 2*data[-3] + data[-4]);
	int32 error_0, error_1, error_2, error_3, error_4;
	uint32 total_error_0 = 0, total_error_1 = 0, total_error_2 = 0, total_error_3 = 0, total_error_4 = 0;
	unsigned i, order;

	for(i = 0; i < data_len; i++) {
		error_0 = data[i]               ; total_error_0 += local_abs(error_0);
		error_1 = error_0 - last_error_0; total_error_1 += local_abs(error_1);
		error_2 = error_1 - last_error_1; total_error_2 += local_abs(error_2);
		error_3 = error_2 - last_error_2; total_error_3 += local_abs(error_3);
		error_4 = error_3 - last_error_3; total_error_4 += local_abs(error_4);

		last_error_0 = error_0;
		last_error_1 = error_1;
		last_error_2 = error_2;
		last_error_3 = error_3;
	}

	if(total_error_0 < min(min(min(total_error_1, total_error_2), total_error_3), total_error_4))
		order = 0;
	else if(total_error_1 < min(min(total_error_2, total_error_3), total_error_4))
		order = 1;
	else if(total_error_2 < min(total_error_3, total_error_4))
		order = 2;
	else if(total_error_3 < total_error_4)
		order = 3;
	else
		order = 4;

	/* Estimate the expected number of bits per residual signal sample. */
	/* 'total_error*' is linearly related to the variance of the residual */
	/* signal, so we use it directly to compute E(|x|) */
#ifdef _MSC_VER
	/* with VC++ you have to spoon feed it the casting */
	residual_bits_per_sample[0] = (real)((data_len > 0) ? log(M_LN2 * (real)(int64)total_error_0  / (real) data_len) / M_LN2 : 0.0);
	residual_bits_per_sample[1] = (real)((data_len > 0) ? log(M_LN2 * (real)(int64)total_error_1  / (real) data_len) / M_LN2 : 0.0);
	residual_bits_per_sample[2] = (real)((data_len > 0) ? log(M_LN2 * (real)(int64)total_error_2  / (real) data_len) / M_LN2 : 0.0);
	residual_bits_per_sample[3] = (real)((data_len > 0) ? log(M_LN2 * (real)(int64)total_error_3  / (real) data_len) / M_LN2 : 0.0);
	residual_bits_per_sample[4] = (real)((data_len > 0) ? log(M_LN2 * (real)(int64)total_error_4  / (real) data_len) / M_LN2 : 0.0);
#else
	residual_bits_per_sample[0] = (real)((data_len > 0) ? log(M_LN2 * (real)total_error_0  / (real) data_len) / M_LN2 : 0.0);
	residual_bits_per_sample[1] = (real)((data_len > 0) ? log(M_LN2 * (real)total_error_1  / (real) data_len) / M_LN2 : 0.0);
	residual_bits_per_sample[2] = (real)((data_len > 0) ? log(M_LN2 * (real)total_error_2  / (real) data_len) / M_LN2 : 0.0);
	residual_bits_per_sample[3] = (real)((data_len > 0) ? log(M_LN2 * (real)total_error_3  / (real) data_len) / M_LN2 : 0.0);
	residual_bits_per_sample[4] = (real)((data_len > 0) ? log(M_LN2 * (real)total_error_4  / (real) data_len) / M_LN2 : 0.0);
#endif

	return order;
}

unsigned FLAC__fixed_compute_best_predictor_slow(const int32 data[], unsigned data_len, real residual_bits_per_sample[FLAC__MAX_FIXED_ORDER+1])
{
	int32 last_error_0 = data[-1];
	int32 last_error_1 = data[-1] - data[-2];
	int32 last_error_2 = last_error_1 - (data[-2] - data[-3]);
	int32 last_error_3 = last_error_2 - (data[-2] - 2*data[-3] + data[-4]);
	int32 error_0, error_1, error_2, error_3, error_4;
	/* total_error_* are 64-bits to avoid overflow when encoding
	 * erratic signals when the bits-per-sample and blocksize are
	 * large.
	 */
	uint64 total_error_0 = 0, total_error_1 = 0, total_error_2 = 0, total_error_3 = 0, total_error_4 = 0;
	unsigned i, order;

	for(i = 0; i < data_len; i++) {
		error_0 = data[i]               ; total_error_0 += local_abs(error_0);
		error_1 = error_0 - last_error_0; total_error_1 += local_abs(error_1);
		error_2 = error_1 - last_error_1; total_error_2 += local_abs(error_2);
		error_3 = error_2 - last_error_2; total_error_3 += local_abs(error_3);
		error_4 = error_3 - last_error_3; total_error_4 += local_abs(error_4);

		last_error_0 = error_0;
		last_error_1 = error_1;
		last_error_2 = error_2;
		last_error_3 = error_3;
	}

	if(total_error_0 < min(min(min(total_error_1, total_error_2), total_error_3), total_error_4))
		order = 0;
	else if(total_error_1 < min(min(total_error_2, total_error_3), total_error_4))
		order = 1;
	else if(total_error_2 < min(total_error_3, total_error_4))
		order = 2;
	else if(total_error_3 < total_error_4)
		order = 3;
	else
		order = 4;

	/* Estimate the expected number of bits per residual signal sample. */
	/* 'total_error*' is linearly related to the variance of the residual */
	/* signal, so we use it directly to compute E(|x|) */
#ifdef _MSC_VER
	/* with VC++ you have to spoon feed it the casting */
	residual_bits_per_sample[0] = (real)((data_len > 0) ? log(M_LN2 * (real)(int64)total_error_0  / (real) data_len) / M_LN2 : 0.0);
	residual_bits_per_sample[1] = (real)((data_len > 0) ? log(M_LN2 * (real)(int64)total_error_1  / (real) data_len) / M_LN2 : 0.0);
	residual_bits_per_sample[2] = (real)((data_len > 0) ? log(M_LN2 * (real)(int64)total_error_2  / (real) data_len) / M_LN2 : 0.0);
	residual_bits_per_sample[3] = (real)((data_len > 0) ? log(M_LN2 * (real)(int64)total_error_3  / (real) data_len) / M_LN2 : 0.0);
	residual_bits_per_sample[4] = (real)((data_len > 0) ? log(M_LN2 * (real)(int64)total_error_4  / (real) data_len) / M_LN2 : 0.0);
#else
	residual_bits_per_sample[0] = (real)((data_len > 0) ? log(M_LN2 * (real)total_error_0  / (real) data_len) / M_LN2 : 0.0);
	residual_bits_per_sample[1] = (real)((data_len > 0) ? log(M_LN2 * (real)total_error_1  / (real) data_len) / M_LN2 : 0.0);
	residual_bits_per_sample[2] = (real)((data_len > 0) ? log(M_LN2 * (real)total_error_2  / (real) data_len) / M_LN2 : 0.0);
	residual_bits_per_sample[3] = (real)((data_len > 0) ? log(M_LN2 * (real)total_error_3  / (real) data_len) / M_LN2 : 0.0);
	residual_bits_per_sample[4] = (real)((data_len > 0) ? log(M_LN2 * (real)total_error_4  / (real) data_len) / M_LN2 : 0.0);
#endif

	return order;
}

void FLAC__fixed_compute_residual(const int32 data[], unsigned data_len, unsigned order, int32 residual[])
{
	unsigned i;

#if 0
	switch(order) {
		case 0:
			for(i = 0; i < data_len; i++) {
				residual[i] = data[i];
			}
			break;
		case 1:
			for(i = 0; i < data_len; i++) {
				residual[i] = data[i] - data[i-1];
			}
			break;
		case 2:
			for(i = 0; i < data_len; i++) {
				/* == data[i] - 2*data[i-1] + data[i-2] */
				residual[i] = data[i] - (data[i-1] << 1) + data[i-2];
			}
			break;
		case 3:
			for(i = 0; i < data_len; i++) {
				/* == data[i] - 3*data[i-1] + 3*data[i-2] - data[i-3] */
				residual[i] = data[i] - (((data[i-1]-data[i-2])<<1) + (data[i-1]-data[i-2])) - data[i-3];
			}
			break;
		case 4:
			for(i = 0; i < data_len; i++) {
				/* == data[i] - 4*data[i-1] + 6*data[i-2] - 4*data[i-3] + data[i-4] */
				residual[i] = data[i] - ((data[i-1]+data[i-3])<<2) + ((data[i-2]<<2) + (data[i-2]<<1)) + data[i-4];
			}
			break;
		default:
			assert(0);
	}
#else
	switch(order) {
		case 0:
			for(i = 0; i < data_len; i++)
				residual[i] = data[i];
			break;
		case 1:
			for(i = 0; i < data_len; i++)
				residual[i] = data[i] - data[i-1];
			break;
		case 2:
			for(i = 0; i < data_len; i++)
				residual[i] = data[i] - 2*data[i-1] + data[i-2];
			break;
		case 3:
			for(i = 0; i < data_len; i++)
				residual[i] = data[i] - 3*data[i-1] + 3*data[i-2] - data[i-3];
			break;
		case 4:
			for(i = 0; i < data_len; i++)
				residual[i] = data[i] - 4*data[i-1] + 6*data[i-2] - 4*data[i-3] + data[i-4];
			break;
		default:
			assert(0);
	}
#endif
}

void FLAC__fixed_restore_signal(const int32 residual[], unsigned data_len, unsigned order, int32 data[])
{
	unsigned i;

	switch(order) {
		case 0:
			for(i = 0; i < data_len; i++) {
				data[i] = residual[i];
			}
			break;
		case 1:
			for(i = 0; i < data_len; i++) {
				data[i] = residual[i] + data[i-1];
			}
			break;
		case 2:
			for(i = 0; i < data_len; i++) {
				/* == residual[i] + 2*data[i-1] - data[i-2] */
				data[i] = residual[i] + (data[i-1]<<1) - data[i-2];
			}
			break;
		case 3:
			for(i = 0; i < data_len; i++) {
				/* residual[i] + 3*data[i-1] - 3*data[i-2]) + data[i-3] */
				data[i] = residual[i] + (((data[i-1]-data[i-2])<<1) + (data[i-1]-data[i-2])) + data[i-3];
			}
			break;
		case 4:
			for(i = 0; i < data_len; i++) {
				/* == residual[i] + 4*data[i-1] - 6*data[i-2] + 4*data[i-3] - data[i-4] */
				data[i] = residual[i] + ((data[i-1]+data[i-3])<<2) - ((data[i-2]<<2) + (data[i-2]<<1)) - data[i-4];
			}
			break;
		default:
			assert(0);
	}
}
