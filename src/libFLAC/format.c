/* libFLAC - Free Lossless Audio Codec library
 * Copyright (C) 2000,2001,2002  Josh Coalson
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

#include <stdio.h>
#include <stdlib.h> /* for qsort() */
#include "FLAC/assert.h"
#include "FLAC/format.h"
#include "private/format.h"

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#ifdef min
#undef min
#endif
#define min(a,b) ((a)<(b)?(a):(b))

/* VERSION should come from configure */
const char *FLAC__VERSION_STRING = VERSION;

#if defined _MSC_VER || defined __MINW32__
/* yet one more hack because of MSVC6: */
const char *FLAC__VENDOR_STRING = "reference libFLAC 1.0.4 20020924";
#else
const char *FLAC__VENDOR_STRING = "reference libFLAC " VERSION " 20020924";
#endif

const FLAC__byte FLAC__STREAM_SYNC_STRING[4] = { 'f','L','a','C' };
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

const unsigned FLAC__STREAM_METADATA_APPLICATION_ID_LEN = 32; /* bits */

const unsigned FLAC__STREAM_METADATA_SEEKPOINT_SAMPLE_NUMBER_LEN = 64; /* bits */
const unsigned FLAC__STREAM_METADATA_SEEKPOINT_STREAM_OFFSET_LEN = 64; /* bits */
const unsigned FLAC__STREAM_METADATA_SEEKPOINT_FRAME_SAMPLES_LEN = 16; /* bits */

const FLAC__uint64 FLAC__STREAM_METADATA_SEEKPOINT_PLACEHOLDER = 0xffffffffffffffff;

const unsigned FLAC__STREAM_METADATA_VORBIS_COMMENT_ENTRY_LENGTH_LEN = 32; /* bits */
const unsigned FLAC__STREAM_METADATA_VORBIS_COMMENT_NUM_COMMENTS_LEN = 32; /* bits */

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
const unsigned FLAC__ENTROPY_CODING_METHOD_PARTITIONED_RICE_RAW_LEN = 5; /* bits */

const unsigned FLAC__ENTROPY_CODING_METHOD_PARTITIONED_RICE_ESCAPE_PARAMETER = 15; /* == (1<<FLAC__ENTROPY_CODING_METHOD_PARTITIONED_RICE_PARAMETER_LEN)-1 */

const char * const FLAC__EntropyCodingMethodTypeString[] = {
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

const char * const FLAC__SubframeTypeString[] = {
	"CONSTANT",
	"VERBATIM",
	"FIXED",
	"LPC"
};

const char * const FLAC__ChannelAssignmentString[] = {
	"INDEPENDENT",
	"LEFT_SIDE",
	"RIGHT_SIDE",
	"MID_SIDE"
};

const char * const FLAC__FrameNumberTypeString[] = {
	"FRAME_NUMBER_TYPE_FRAME_NUMBER",
	"FRAME_NUMBER_TYPE_SAMPLE_NUMBER"
};

const char * const FLAC__MetadataTypeString[] = {
	"STREAMINFO",
	"PADDING",
	"APPLICATION",
	"SEEKTABLE",
	"VORBIS_COMMENT"
};

FLAC__bool FLAC__format_sample_rate_is_valid(unsigned sample_rate)
{
	if(
		sample_rate == 0 ||
		sample_rate > FLAC__MAX_SAMPLE_RATE ||
		(
			sample_rate >= (1u << 16) &&
			!(sample_rate % 1000 == 0 || sample_rate % 10 == 0)
		)
	) {
		return false;
	}
	else
		return true;
}

FLAC__bool FLAC__format_seektable_is_legal(const FLAC__StreamMetadata_SeekTable *seek_table)
{
	unsigned i;
	FLAC__uint64 prev_sample_number = 0;
	FLAC__bool got_prev = false;

	FLAC__ASSERT(0 != seek_table);

	for(i = 0; i < seek_table->num_points; i++) {
		if(got_prev) {
			if(
				seek_table->points[i].sample_number != FLAC__STREAM_METADATA_SEEKPOINT_PLACEHOLDER &&
				seek_table->points[i].sample_number <= prev_sample_number
			)
				return false;
		}
		prev_sample_number = seek_table->points[i].sample_number;
		got_prev = true;
	}

	return true;
}

/* used as the sort predicate for qsort() */
static int seekpoint_compare_(const FLAC__StreamMetadata_SeekPoint *l, const FLAC__StreamMetadata_SeekPoint *r)
{
	/* we don't just 'return l->sample_number - r->sample_number' since the result (FLAC__int64) might overflow an 'int' */
	if(l->sample_number == r->sample_number)
		return 0;
	else if(l->sample_number < r->sample_number)
		return -1;
	else
		return 1;
}

unsigned FLAC__format_seektable_sort(FLAC__StreamMetadata_SeekTable *seek_table)
{
	unsigned i, j;
	FLAC__bool first;

	FLAC__ASSERT(0 != seek_table);

	/* sort the seekpoints */
	qsort(seek_table->points, seek_table->num_points, sizeof(FLAC__StreamMetadata_SeekPoint), (int (*)(const void *, const void *))seekpoint_compare_);

	/* uniquify the seekpoints */
	first = true;
	for(i = j = 0; i < seek_table->num_points; i++) {
		if(seek_table->points[i].sample_number != FLAC__STREAM_METADATA_SEEKPOINT_PLACEHOLDER) {
			if(!first) {
				if(seek_table->points[i].sample_number == seek_table->points[j-1].sample_number)
					continue;
			}
		}
		first = false;
		seek_table->points[j++] = seek_table->points[i];
	}

	for(i = j; i < seek_table->num_points; i++) {
		seek_table->points[i].sample_number = FLAC__STREAM_METADATA_SEEKPOINT_PLACEHOLDER;
		seek_table->points[i].stream_offset = 0;
		seek_table->points[i].frame_samples = 0;
	}

	return j;
}

unsigned FLAC__format_get_max_rice_partition_order(unsigned blocksize, unsigned predictor_order)
{
	return
		FLAC__format_get_max_rice_partition_order_from_blocksize_limited_max_and_predictor_order(
			FLAC__format_get_max_rice_partition_order_from_blocksize(blocksize),
			blocksize,
			predictor_order
		);
}

unsigned FLAC__format_get_max_rice_partition_order_from_blocksize(unsigned blocksize)
{
	unsigned max_rice_partition_order = 0;
	while(!(blocksize & 1)) {
		max_rice_partition_order++;
		blocksize >>= 1;
	}
	return min(FLAC__MAX_RICE_PARTITION_ORDER, max_rice_partition_order);
}

unsigned FLAC__format_get_max_rice_partition_order_from_blocksize_limited_max_and_predictor_order(unsigned limit, unsigned blocksize, unsigned predictor_order)
{
	unsigned max_rice_partition_order = limit;

	while(max_rice_partition_order > 0 && (blocksize >> max_rice_partition_order) <= predictor_order)
		max_rice_partition_order--;

	FLAC__ASSERT(
		(max_rice_partition_order == 0 && blocksize >= predictor_order) ||
		(max_rice_partition_order > 0 && blocksize >> max_rice_partition_order > predictor_order)
	);

	return max_rice_partition_order;
}

void FLAC__format_entropy_coding_method_partitioned_rice_contents_init(FLAC__EntropyCodingMethod_PartitionedRiceContents *object)
{
	FLAC__ASSERT(0 != object);

	object->parameters = 0;
	object->raw_bits = 0;
	object->capacity_by_order = 0;
}

void FLAC__format_entropy_coding_method_partitioned_rice_contents_clear(FLAC__EntropyCodingMethod_PartitionedRiceContents *object)
{
	FLAC__ASSERT(0 != object);

	if(0 != object->parameters)
		free(object->parameters);
	if(0 != object->raw_bits)
		free(object->raw_bits);
	FLAC__format_entropy_coding_method_partitioned_rice_contents_init(object);
}

FLAC__bool FLAC__format_entropy_coding_method_partitioned_rice_contents_ensure_size(FLAC__EntropyCodingMethod_PartitionedRiceContents *object, unsigned max_partition_order)
{
	FLAC__ASSERT(0 != object);

	FLAC__ASSERT(object->capacity_by_order > 0 || (0 == object->parameters && 0 == object->raw_bits));

	if(object->capacity_by_order < max_partition_order) {
		if(0 == (object->parameters = realloc(object->parameters, sizeof(unsigned)*(1 << max_partition_order))))
			return false;
		if(0 == (object->raw_bits = realloc(object->raw_bits, sizeof(unsigned)*(1 << max_partition_order))))
			return false;
		object->capacity_by_order = max_partition_order;
	}

	return true;
}
