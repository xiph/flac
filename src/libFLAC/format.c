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

#include <assert.h>
#include <stdio.h>
#include "FLAC/format.h"

const unsigned FLAC__MAJOR_VERSION = 0;
const unsigned FLAC__MINOR_VERSION = 5;

const byte     FLAC__STREAM_SYNC_STRING[4] = { 'f','L','a','C' };
const unsigned FLAC__STREAM_SYNC = 0x664C6143;
const unsigned FLAC__STREAM_SYNC_LEN = 32; /* bits */;

const unsigned FLAC__STREAM_METADATA_ENCODING_MIN_BLOCK_SIZE_LEN = 16; /* bits */
const unsigned FLAC__STREAM_METADATA_ENCODING_MAX_BLOCK_SIZE_LEN = 16; /* bits */
const unsigned FLAC__STREAM_METADATA_ENCODING_MIN_FRAME_SIZE_LEN = 24; /* bits */
const unsigned FLAC__STREAM_METADATA_ENCODING_MAX_FRAME_SIZE_LEN = 24; /* bits */
const unsigned FLAC__STREAM_METADATA_ENCODING_SAMPLE_RATE_LEN = 20; /* bits */
const unsigned FLAC__STREAM_METADATA_ENCODING_CHANNELS_LEN = 3; /* bits */
const unsigned FLAC__STREAM_METADATA_ENCODING_BITS_PER_SAMPLE_LEN = 5; /* bits */
const unsigned FLAC__STREAM_METADATA_ENCODING_TOTAL_SAMPLES_LEN = 36; /* bits */
const unsigned FLAC__STREAM_METADATA_ENCODING_MD5SUM_LEN = 128; /* bits */
const unsigned FLAC__STREAM_METADATA_ENCODING_LENGTH = 34; /* bytes */

const unsigned FLAC__STREAM_METADATA_IS_LAST_LEN = 1; /* bits */
const unsigned FLAC__STREAM_METADATA_TYPE_LEN = 7; /* bits */
const unsigned FLAC__STREAM_METADATA_LENGTH_LEN = 24; /* bits */

const unsigned FLAC__FRAME_HEADER_SYNC = 0x1fe;
const unsigned FLAC__FRAME_HEADER_SYNC_LEN = 9; /* bits */
const unsigned FLAC__FRAME_HEADER_BLOCK_SIZE_LEN = 3; /* bits */
const unsigned FLAC__FRAME_HEADER_SAMPLE_RATE_LEN = 4; /* bits */
const unsigned FLAC__FRAME_HEADER_CHANNEL_ASSIGNMENT_LEN = 4; /* bits */
const unsigned FLAC__FRAME_HEADER_BITS_PER_SAMPLE_LEN = 3; /* bits */
const unsigned FLAC__FRAME_HEADER_ZERO_PAD_LEN = 1; /* bits */
const unsigned FLAC__FRAME_HEADER_CRC8_LEN = 8; /* bits */

const unsigned FLAC__ENTROPY_CODING_METHOD_TYPE_LEN = 2; /* bits */
const unsigned FLAC__ENTROPY_CODING_METHOD_PARTITIONED_RICE_ORDER_LEN = 4; /* bits */
const unsigned FLAC__ENTROPY_CODING_METHOD_PARTITIONED_RICE_PARAMETER_LEN = 4; /* bits */

const unsigned FLAC__SUBFRAME_HEADER_LPC_QLP_COEFF_PRECISION_LEN = 4; /* bits */
const unsigned FLAC__SUBFRAME_HEADER_LPC_QLP_SHIFT_LEN = 5; /* bits */
const unsigned FLAC__SUBFRAME_HEADER_LPC_RICE_PARAMETER_LEN = 4; /* bits */

const unsigned FLAC__SUBFRAME_HEADER_TYPE_CONSTANT = 0x00;
const unsigned FLAC__SUBFRAME_HEADER_TYPE_VERBATIM = 0x02;
const unsigned FLAC__SUBFRAME_HEADER_TYPE_FIXED = 0x10;
const unsigned FLAC__SUBFRAME_HEADER_TYPE_LPC = 0x40;
const unsigned FLAC__SUBFRAME_HEADER_TYPE_LEN = 8; /* bits */
