/* libFLAC - Free Lossless Audio Coder library
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

#ifndef FLAC__PRIVATE__ENCODER_FRAMING_H
#define FLAC__PRIVATE__ENCODER_FRAMING_H

#include "FLAC/format.h"
#include "bitbuffer.h"

bool FLAC__add_metadata_block(const FLAC__StreamMetaData *metadata, FLAC__BitBuffer *bb);
bool FLAC__frame_add_header(const FLAC__FrameHeader *header, bool streamable_subset, bool is_last_block, FLAC__BitBuffer *bb);
bool FLAC__subframe_add_constant(unsigned bits_per_sample, const FLAC__SubframeHeader *subframe, FLAC__BitBuffer *bb);
bool FLAC__subframe_add_fixed(const int32 residual[], unsigned residual_samples, unsigned bits_per_sample, const FLAC__SubframeHeader *subframe, FLAC__BitBuffer *bb);
bool FLAC__subframe_add_lpc(const int32 residual[], unsigned residual_samples, unsigned bits_per_sample, const FLAC__SubframeHeader *subframe, FLAC__BitBuffer *bb);
bool FLAC__subframe_add_verbatim(const int32 signal[], unsigned samples, unsigned bits_per_sample, FLAC__BitBuffer *bb);

#endif
