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

#ifndef FLAC__PRIVATE__FIXED_H
#define FLAC__PRIVATE__FIXED_H

#include "FLAC/format.h"

/*
 *	FLAC__fixed_compute_best_predictor()
 *	--------------------------------------------------------------------
 *	Compute the best fixed predictor and the expected bits-per-sample
 *	of the residual signal for each order.
 *
 *	IN data[0,data_len-1]
 *	IN data_len
 *	OUT residual_bits_per_sample[0,FLAC__MAX_FIXED_ORDER]
 */
unsigned FLAC__fixed_compute_best_predictor(const int32 data[], unsigned data_len, real residual_bits_per_sample[FLAC__MAX_FIXED_ORDER+1]);

/*
 *	FLAC__fixed_compute_residual()
 *	--------------------------------------------------------------------
 *	Compute the residual signal obtained from sutracting the predicted
 *	signal from the original.
 *
 *	IN data[-order,data_len-1]        original signal (NOTE THE INDICES!)
 *	IN data_len                       length of original signal
 *	IN order <= FLAC__MAX_FIXED_ORDER fixed-predictor order
 *	OUT residual[0,data_len-1]        residual signal
 */
void FLAC__fixed_compute_residual(const int32 data[], unsigned data_len, unsigned order, int32 residual[]);

/*
 *	FLAC__fixed_restore_signal()
 *	--------------------------------------------------------------------
 *	Restore the original signal by summing the residual and the
 *	predictor.
 *
 *	IN residual[0,data_len-1]         residual signal
 *	IN data_len                       length of original signal
 *	IN order <= FLAC__MAX_FIXED_ORDER fixed-predictor order
 *	*** IMPORTANT: the caller must pass in the historical samples:
 *	IN  data[-order,-1]               previously-reconstructed historical samples
 *	OUT data[0,data_len-1]            original signal
 */
void FLAC__fixed_restore_signal(const int32 residual[], unsigned data_len, unsigned order, int32 data[]);

#endif
