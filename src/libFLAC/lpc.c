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
#include <stdio.h>
#include "FLAC/format.h"
#include "private/lpc.h"

#ifndef M_LN2
/* math.h in VC++ doesn't seem to have this (how Microsoft is that?) */
#define M_LN2 0.69314718055994530942
#endif

void FLAC__lpc_compute_autocorrelation(const real data[], unsigned data_len, unsigned lag, real autoc[])
{
	real d;
	unsigned i;

	assert(lag > 0);
	assert(lag <= data_len);

	while(lag--) {
		for(i = lag, d = 0.0; i < data_len; i++)
			d += data[i] * data[i - lag];
		autoc[lag] = d;
	}
}

void FLAC__lpc_compute_lp_coefficients(const real autoc[], unsigned max_order, real lp_coeff[][FLAC__MAX_LPC_ORDER], real error[])
{
	unsigned i, j;
	real r, err, ref[FLAC__MAX_LPC_ORDER], lpc[FLAC__MAX_LPC_ORDER];

	assert(0 < max_order);
	assert(max_order <= FLAC__MAX_LPC_ORDER);
	assert(autoc[0] != 0.0);

	err = autoc[0];

	for(i = 0; i < max_order; i++) {
		/* Sum up this iteration's reflection coefficient. */
		r =- autoc[i+1];
		for(j = 0; j < i; j++)
			r -= lpc[j] * autoc[i-j];
		ref[i] = (r/=err);

		/* Update LPC coefficients and total error. */
		lpc[i]=r;
		for(j = 0; j < (i>>1); j++) {
			real tmp = lpc[j];
			lpc[j] += r * lpc[i-1-j];
			lpc[i-1-j] += r * tmp;
		}
		if(i & 1)
			lpc[j] += lpc[j] * r;

		err *= (1.0 - r * r);

		/* save this order */
		for(j = 0; j <= i; j++)
			lp_coeff[i][j] = -lpc[j]; /* N.B. why do we have to negate here? */
		error[i] = err;
	}
}

int FLAC__lpc_quantize_coefficients(const real lp_coeff[], unsigned order, unsigned precision, unsigned bits_per_sample, int32 qlp_coeff[], int *shift)
{
	unsigned i;
	real d, cmax = -1e99;

	assert(bits_per_sample > 0);
	assert(bits_per_sample <= sizeof(int32)*8);
	assert(precision > 0);
	assert(precision >= FLAC__MIN_QLP_COEFF_PRECISION);
	assert(precision + bits_per_sample < sizeof(int32)*8);
#ifdef NDEBUG
	(void)bits_per_sample; /* silence compiler warning about unused parameter */
#endif

	/* drop one bit for the sign; from here on out we consider only |lp_coeff[i]| */
	precision--;

	for(i = 0; i < order; i++) {
		if(lp_coeff[i] == 0.0)
			continue;
		d = fabs(lp_coeff[i]);
		if(d > cmax)
			cmax = d;
	}
	if(cmax < 0) {
		/* => coeffients are all 0, which means our constant-detect didn't work */
		return 2;
	}
	else {
		const int maxshift = (int)precision - floor(log(cmax) / M_LN2) - 1;
		const int max_shiftlimit = (1 << (FLAC__SUBFRAME_LPC_QLP_SHIFT_LEN-1)) - 1;
		const int min_shiftlimit = -max_shiftlimit - 1;

		*shift = maxshift;

		if(*shift < min_shiftlimit || *shift > max_shiftlimit) {
			return 1;
		}
	}

	if(*shift != 0) { /* just to avoid wasting time... */
		for(i = 0; i < order; i++)
			qlp_coeff[i] = (int32)floor(lp_coeff[i] * (real)(1 << *shift));
	}
	return 0;
}

void FLAC__lpc_compute_residual_from_qlp_coefficients(const int32 data[], unsigned data_len, const int32 qlp_coeff[], unsigned order, int lp_quantization, int32 residual[])
{
#ifdef FLAC_OVERFLOW_DETECT
	int64 sumo;
#endif
	unsigned i, j;
	int32 sum;
	const int32 *history;

#ifdef FLAC_OVERFLOW_DETECT_VERBOSE
	fprintf(stderr,"FLAC__lpc_compute_residual_from_qlp_coefficients: data_len=%d, order=%u, lpq=%d",data_len,order,lp_quantization);
	for(i=0;i<order;i++)
		fprintf(stderr,", q[%u]=%d",i,qlp_coeff[i]);
	fprintf(stderr,"\n");
#endif
	assert(order > 0);

	for(i = 0; i < data_len; i++) {
#ifdef FLAC_OVERFLOW_DETECT
		sumo = 0;
#endif
		sum = 0;
		history = data;
		for(j = 0; j < order; j++) {
			sum += qlp_coeff[j] * (*(--history));
#ifdef FLAC_OVERFLOW_DETECT
			sumo += (int64)qlp_coeff[j] * (int64)(*history);
			if(sumo > 2147483647ll || sumo < -2147483648ll) {
				fprintf(stderr,"FLAC__lpc_compute_residual_from_qlp_coefficients: OVERFLOW, i=%u, j=%u, c=%d, d=%d, sumo=%lld\n",i,j,qlp_coeff[j],*history,sumo);
			}
#endif
		}
		*(residual++) = *(data++) - (sum >> lp_quantization);
	}

	/* Here's a slower but clearer version:
	for(i = 0; i < data_len; i++) {
		sum = 0;
		for(j = 0; j < order; j++)
			sum += qlp_coeff[j] * data[i-j-1];
		residual[i] = data[i] - (sum >> lp_quantization);
	}
	*/
}

void FLAC__lpc_restore_signal(const int32 residual[], unsigned data_len, const int32 qlp_coeff[], unsigned order, int lp_quantization, int32 data[])
{
#ifdef FLAC_OVERFLOW_DETECT
	int64 sumo;
#endif
	unsigned i, j;
	int32 sum;
	const int32 *history;

#ifdef FLAC_OVERFLOW_DETECT_VERBOSE
	fprintf(stderr,"FLAC__lpc_restore_signal: data_len=%d, order=%u, lpq=%d",data_len,order,lp_quantization);
	for(i=0;i<order;i++)
		fprintf(stderr,", q[%u]=%d",i,qlp_coeff[i]);
	fprintf(stderr,"\n");
#endif
	assert(order > 0);

	for(i = 0; i < data_len; i++) {
#ifdef FLAC_OVERFLOW_DETECT
		sumo = 0;
#endif
		sum = 0;
		history = data;
		for(j = 0; j < order; j++) {
			sum += qlp_coeff[j] * (*(--history));
#ifdef FLAC_OVERFLOW_DETECT
			sumo += (int64)qlp_coeff[j] * (int64)(*history);
			if(sumo > 2147483647ll || sumo < -2147483648ll) {
				fprintf(stderr,"FLAC__lpc_restore_signal: OVERFLOW, i=%u, j=%u, c=%d, d=%d, sumo=%lld\n",i,j,qlp_coeff[j],*history,sumo);
			}
#endif
		}
		*(data++) = *(residual++) + (sum >> lp_quantization);
	}

	/* Here's a slower but clearer version:
	for(i = 0; i < data_len; i++) {
		sum = 0;
		for(j = 0; j < order; j++)
			sum += qlp_coeff[j] * data[i-j-1];
		data[i] = residual[i] + (sum >> lp_quantization);
	}
	*/
}

real FLAC__lpc_compute_expected_bits_per_residual_sample(real lpc_error, unsigned total_samples)
{
	real escale;

	assert(lpc_error >= 0.0); /* the error can never be negative */
	assert(total_samples > 0);

	escale = 0.5 * M_LN2 * M_LN2 / (real)total_samples;

	if(lpc_error > 0.0) {
		real bps = 0.5 * log(escale * lpc_error) / M_LN2;
		if(bps >= 0.0)
			return bps;
		else
			return 0.0;
	}
	else {
		return 0.0;
	}
}

unsigned FLAC__lpc_compute_best_order(const real lpc_error[], unsigned max_order, unsigned total_samples, unsigned bits_per_signal_sample)
{
	unsigned order, best_order;
	real best_bits, tmp_bits;

	assert(max_order > 0);

	best_order = 0;
	best_bits = FLAC__lpc_compute_expected_bits_per_residual_sample(lpc_error[0], total_samples) * (real)total_samples;

	for(order = 1; order < max_order; order++) {
		tmp_bits = FLAC__lpc_compute_expected_bits_per_residual_sample(lpc_error[order], total_samples) * (real)(total_samples - order) + (real)(order * bits_per_signal_sample);
		if(tmp_bits < best_bits) {
			best_order = order;
			best_bits = tmp_bits;
		}
	}

	return best_order+1; /* +1 since index of lpc_error[] is order-1 */
}
