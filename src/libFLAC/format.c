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
#include <stdio.h>
#include "FLAC/format.h"

const byte     FLAC__STREAM_SYNC_STRING[4] = { 'f','L','a','C' };
const unsigned FLAC__STREAM_SYNC = 0x664C6143;
const unsigned FLAC__STREAM_SYNC_LEN = 32; /* bits */;

const unsigned FLAC__STREAM_METADATA_STREAMINFO_MIN_BLOCK_SIZE_LEN = 16; /* bits */
const unsigned FLAC__STREAM_METADATA_STREAMINFO_MAX_BLOCK_SIZE_LEN = 16; /* bits */
const unsigned FLAC__STREAM_METADATA_STREAMINFO_MIN_FRAME_SIZE_LEN = 24; /* bits */
const unsigned FLAC__STREAM_METADATA_STREAMINFO_MAX_FRAME_SIZE_LEN = 24; /* bits */
const unsigned FLAC__STREAM_METADATA_STREAMINFO_SAMPLE_RATE_LEN = 20; /* bits */
const unsigned FLAC__STREAM_METADATA_STREAMINFO_CHANNELS_LEN = 3; /* bits */
const unsigned FLAC__STREAM_METADATA_STREAMINFO_BITS_PER_SAMPLE_LEN = 5; /* bits */
const unsigned FLAC__STREAM_METADATA_STREAMINFO_TOTAL_SAMPLES_LEN = 36; /* bits */
const unsigned FLAC__STREAM_METADATA_STREAMINFO_MD5SUM_LEN = 128; /* bits */
const unsigned FLAC__STREAM_METADATA_STREAMINFO_LENGTH = 34; /* bytes */

const unsigned FLAC__STREAM_METADATA_APPLICATION_ID_LEN = 32; /* bits */

const unsigned FLAC__STREAM_METADATA_SEEKPOINT_SAMPLE_NUMBER_LEN = 64; /* bits */
const unsigned FLAC__STREAM_METADATA_SEEKPOINT_STREAM_OFFSET_LEN = 64; /* bits */
const unsigned FLAC__STREAM_METADATA_SEEKPOINT_BLOCK_OFFSET_LEN = 16; /* bits */
const unsigned FLAC__STREAM_METADATA_SEEKPOINT_LEN = 18; /* bytes */

const unsigned FLAC__STREAM_METADATA_IS_LAST_LEN = 1; /* bits */
const unsigned FLAC__STREAM_METADATA_TYPE_LEN = 7; /* bits */
const unsigned FLAC__STREAM_METADATA_LENGTH_LEN = 24; /* bits */

const unsigned FLAC__FRAME_HEADER_SYNC = 0x3ffe;
const unsigned FLAC__FRAME_HEADER_SYNC_LEN = 14; /* bits */
const unsigned FLAC__FRAME_HEADER_RESERVED_LEN = 2; /* bits */
const unsigned FLAC__FRAME_HEADER_BLOCK_SIZE_LEN = 4; /* bits */
const unsigned FLAC__FRAME_HEADER_SAMPLE_RATE_LEN = 4; /* bits */
const unsigned FLAC__FRAME_HEADER_CHANNEL_ASSIGNMENT_LEN = 4; /* bits */
const unsigned FLAC__FRAME_HEADER_BITS_PER_SAMPLE_LEN = 3; /* bits */
const unsigned FLAC__FRAME_HEADER_ZERO_PAD_LEN = 1; /* bits */
const unsigned FLAC__FRAME_HEADER_CRC_LEN = 8; /* bits */

const unsigned FLAC__FRAME_FOOTER_CRC_LEN = 16; /* bits */

const unsigned FLAC__ENTROPY_CODING_METHOD_TYPE_LEN = 2; /* bits */
const unsigned FLAC__ENTROPY_CODING_METHOD_PARTITIONED_RICE_ORDER_LEN = 4; /* bits */
const unsigned FLAC__ENTROPY_CODING_METHOD_PARTITIONED_RICE_PARAMETER_LEN = 4; /* bits */

const char *FLAC__EntropyCodingMethodTypeString[] = {
	"PARTITIONED_RICE"
};

const unsigned FLAC__SUBFRAME_LPC_QLP_COEFF_PRECISION_LEN = 4; /* bits */
const unsigned FLAC__SUBFRAME_LPC_QLP_SHIFT_LEN = 5; /* bits */

const unsigned FLAC__SUBFRAME_ZERO_PAD_LEN = 1; /* bits */
const unsigned FLAC__SUBFRAME_TYPE_LEN = 6; /* bits */
const unsigned FLAC__SUBFRAME_WASTED_BITS_FLAG_LEN = 1; /* bits */

const unsigned FLAC__SUBFRAME_TYPE_CONSTANT_BYTE_ALIGNED_MASK = 0x00;
const unsigned FLAC__SUBFRAME_TYPE_VERBATIM_BYTE_ALIGNED_MASK = 0x02;
const unsigned FLAC__SUBFRAME_TYPE_FIXED_BYTE_ALIGNED_MASK = 0x10;
const unsigned FLAC__SUBFRAME_TYPE_LPC_BYTE_ALIGNED_MASK = 0x40;

const char *FLAC__SubframeTypeString[] = {
	"CONSTANT",
	"VERBATIM",
	"FIXED",
	"LPC"
};

const char *FLAC__ChannelAssignmentString[] = {
	"INDEPENDENT",
	"LEFT_SIDE",
	"RIGHT_SIDE",
	"MID_SIDE"
};

const char *FLAC__MetaDataTypeString[] = {
	"STREAMINFO",
	"PADDING",
	"APPLICATION"
};
