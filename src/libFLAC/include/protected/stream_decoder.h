/* libFLAC - Free Lossless Audio Codec library
 * Copyright (C) 2000,2001,2002,2003  Josh Coalson
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

#ifndef FLAC__PROTECTED__STREAM_DECODER_H
#define FLAC__PROTECTED__STREAM_DECODER_H

#include "FLAC/stream_decoder.h"

typedef struct FLAC__StreamDecoderProtected {
	FLAC__StreamDecoderState state;
	unsigned channels;
	FLAC__ChannelAssignment channel_assignment;
	unsigned bits_per_sample;
	unsigned sample_rate; /* in Hz */
	unsigned blocksize; /* in samples (per channel) */
} FLAC__StreamDecoderProtected;

/*
 * return the number of input bytes consumed
 */
unsigned FLAC__stream_decoder_get_input_bytes_unconsumed(const FLAC__StreamDecoder *decoder);

#endif
