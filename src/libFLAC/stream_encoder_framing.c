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

#include <stdio.h>
#include "private/stream_encoder_framing.h"
#include "private/crc.h"
#include "FLAC/assert.h"

#ifdef max
#undef max
#endif
#define max(x,y) ((x)>(y)?(x):(y))

static FLAC__bool subframe_add_entropy_coding_method_(FLAC__BitBuffer *bb, const FLAC__EntropyCodingMethod *method);
static FLAC__bool subframe_add_residual_partitioned_rice_(FLAC__BitBuffer *bb, const FLAC__int32 residual[], const unsigned residual_samples, const unsigned predictor_order, const unsigned rice_parameters[], const unsigned raw_bits[], const unsigned partition_order);

FLAC__bool FLAC__add_metadata_block(const FLAC__StreamMetaData *metadata, FLAC__BitBuffer *bb)
{
	unsigned i;

	if(!FLAC__bitbuffer_write_raw_uint32(bb, metadata->is_last, FLAC__STREAM_METADATA_IS_LAST_LEN))
		return false;

	if(!FLAC__bitbuffer_write_raw_uint32(bb, metadata->type, FLAC__STREAM_METADATA_TYPE_LEN))
		return false;

	FLAC__ASSERT(metadata->length < (1u << FLAC__STREAM_METADATA_LENGTH_LEN));
	if(!FLAC__bitbuffer_write_raw_uint32(bb, metadata->length, FLAC__STREAM_METADATA_LENGTH_LEN))
		return false;

	switch(metadata->type) {
		case FLAC__METADATA_TYPE_STREAMINFO:
			FLAC__ASSERT(metadata->data.stream_info.min_blocksize < (1u << FLAC__STREAM_METADATA_STREAMINFO_MIN_BLOCK_SIZE_LEN));
			if(!FLAC__bitbuffer_write_raw_uint32(bb, metadata->data.stream_info.min_blocksize, FLAC__STREAM_METADATA_STREAMINFO_MIN_BLOCK_SIZE_LEN))
				return false;
			FLAC__ASSERT(metadata->data.stream_info.max_blocksize < (1u << FLAC__STREAM_METADATA_STREAMINFO_MAX_BLOCK_SIZE_LEN));
			if(!FLAC__bitbuffer_write_raw_uint32(bb, metadata->data.stream_info.max_blocksize, FLAC__STREAM_METADATA_STREAMINFO_MAX_BLOCK_SIZE_LEN))
				return false;
			FLAC__ASSERT(metadata->data.stream_info.min_framesize < (1u << FLAC__STREAM_METADATA_STREAMINFO_MIN_FRAME_SIZE_LEN));
			if(!FLAC__bitbuffer_write_raw_uint32(bb, metadata->data.stream_info.min_framesize, FLAC__STREAM_METADATA_STREAMINFO_MIN_FRAME_SIZE_LEN))
				return false;
			FLAC__ASSERT(metadata->data.stream_info.max_framesize < (1u << FLAC__STREAM_METADATA_STREAMINFO_MAX_FRAME_SIZE_LEN));
			if(!FLAC__bitbuffer_write_raw_uint32(bb, metadata->data.stream_info.max_framesize, FLAC__STREAM_METADATA_STREAMINFO_MAX_FRAME_SIZE_LEN))
				return false;
			FLAC__ASSERT(metadata->data.stream_info.sample_rate > 0);
			FLAC__ASSERT(metadata->data.stream_info.sample_rate < (1u << FLAC__STREAM_METADATA_STREAMINFO_SAMPLE_RATE_LEN));
			if(!FLAC__bitbuffer_write_raw_uint32(bb, metadata->data.stream_info.sample_rate, FLAC__STREAM_METADATA_STREAMINFO_SAMPLE_RATE_LEN))
				return false;
			FLAC__ASSERT(metadata->data.stream_info.channels > 0);
			FLAC__ASSERT(metadata->data.stream_info.channels <= (1u << FLAC__STREAM_METADATA_STREAMINFO_CHANNELS_LEN));
			if(!FLAC__bitbuffer_write_raw_uint32(bb, metadata->data.stream_info.channels-1, FLAC__STREAM_METADATA_STREAMINFO_CHANNELS_LEN))
				return false;
			FLAC__ASSERT(metadata->data.stream_info.bits_per_sample > 0);
			FLAC__ASSERT(metadata->data.stream_info.bits_per_sample <= (1u << FLAC__STREAM_METADATA_STREAMINFO_BITS_PER_SAMPLE_LEN));
			if(!FLAC__bitbuffer_write_raw_uint32(bb, metadata->data.stream_info.bits_per_sample-1, FLAC__STREAM_METADATA_STREAMINFO_BITS_PER_SAMPLE_LEN))
				return false;
			if(!FLAC__bitbuffer_write_raw_uint64(bb, metadata->data.stream_info.total_samples, FLAC__STREAM_METADATA_STREAMINFO_TOTAL_SAMPLES_LEN))
				return false;
			for(i = 0; i < 16; i++) {
				if(!FLAC__bitbuffer_write_raw_uint32(bb, metadata->data.stream_info.md5sum[i], 8))
					return false;
			}
			break;
		case FLAC__METADATA_TYPE_PADDING:
			if(!FLAC__bitbuffer_write_zeroes(bb, metadata->length * 8))
				return false;
			break;
		case FLAC__METADATA_TYPE_APPLICATION:
			if(!FLAC__bitbuffer_write_raw_uint32(bb, metadata->data.application.id[0], 8))
				return false;
			if(!FLAC__bitbuffer_write_raw_uint32(bb, metadata->data.application.id[1], 8))
				return false;
			if(!FLAC__bitbuffer_write_raw_uint32(bb, metadata->data.application.id[2], 8))
				return false;
			if(!FLAC__bitbuffer_write_raw_uint32(bb, metadata->data.application.id[3], 8))
				return false;
			for(i = 0; i < metadata->length; i++) {
				if(!FLAC__bitbuffer_write_raw_uint32(bb, metadata->data.application.data[i], 8))
					return false;
			}
			break;
		case FLAC__METADATA_TYPE_SEEKTABLE:
			for(i = 0; i < metadata->data.seek_table.num_points; i++) {
				if(!FLAC__bitbuffer_write_raw_uint64(bb, metadata->data.seek_table.points[i].sample_number, FLAC__STREAM_METADATA_SEEKPOINT_SAMPLE_NUMBER_LEN))
					return false;
				if(!FLAC__bitbuffer_write_raw_uint64(bb, metadata->data.seek_table.points[i].stream_offset, FLAC__STREAM_METADATA_SEEKPOINT_STREAM_OFFSET_LEN))
					return false;
				if(!FLAC__bitbuffer_write_raw_uint32(bb, metadata->data.seek_table.points[i].frame_samples, FLAC__STREAM_METADATA_SEEKPOINT_FRAME_SAMPLES_LEN))
					return false;
			}
			break;
		default:
			FLAC__ASSERT(0);
	}

	return true;
}

FLAC__bool FLAC__frame_add_header(const FLAC__FrameHeader *header, FLAC__bool streamable_subset, FLAC__bool is_last_block, FLAC__BitBuffer *bb)
{
	unsigned u, crc8_start, blocksize_hint, sample_rate_hint;
	FLAC__byte crc8;

	FLAC__ASSERT(bb->bits == 0); /* assert that we're byte-aligned before writing */

	crc8_start = bb->bytes;

	if(!FLAC__bitbuffer_write_raw_uint32(bb, FLAC__FRAME_HEADER_SYNC, FLAC__FRAME_HEADER_SYNC_LEN))
		return false;

	if(!FLAC__bitbuffer_write_raw_uint32(bb, 0, FLAC__FRAME_HEADER_RESERVED_LEN))
		return false;

	FLAC__ASSERT(header->blocksize > 0 && header->blocksize <= FLAC__MAX_BLOCK_SIZE);
	blocksize_hint = 0;
	switch(header->blocksize) {
		case   192: u = 1; break;
		case   576: u = 2; break;
		case  1152: u = 3; break;
		case  2304: u = 4; break;
		case  4608: u = 5; break;
		case   256: u = 8; break;
		case   512: u = 9; break;
		case  1024: u = 10; break;
		case  2048: u = 11; break;
		case  4096: u = 12; break;
		case  8192: u = 13; break;
		case 16384: u = 14; break;
		case 32768: u = 15; break;
		default:
			if(streamable_subset || is_last_block) {
				if(header->blocksize <= 0x100)
					blocksize_hint = u = 6;
				else
					blocksize_hint = u = 7;
			}
			else
				u = 0;
			break;
	}
	if(!FLAC__bitbuffer_write_raw_uint32(bb, u, FLAC__FRAME_HEADER_BLOCK_SIZE_LEN))
		return false;

	FLAC__ASSERT(header->sample_rate > 0 && header->sample_rate < (1u << FLAC__STREAM_METADATA_STREAMINFO_SAMPLE_RATE_LEN));
	sample_rate_hint = 0;
	switch(header->sample_rate) {
		case  8000: u = 4; break;
		case 16000: u = 5; break;
		case 22050: u = 6; break;
		case 24000: u = 7; break;
		case 32000: u = 8; break;
		case 44100: u = 9; break;
		case 48000: u = 10; break;
		case 96000: u = 11; break;
		default:
			if(streamable_subset) {
				if(header->sample_rate % 1000 == 0)
					sample_rate_hint = u = 12;
				else if(header->sample_rate % 10 == 0)
					sample_rate_hint = u = 14;
				else
					sample_rate_hint = u = 13;
			}
			else
				u = 0;
			break;
	}
	if(!FLAC__bitbuffer_write_raw_uint32(bb, u, FLAC__FRAME_HEADER_SAMPLE_RATE_LEN))
		return false;

	FLAC__ASSERT(header->channels > 0 && header->channels <= (1u << FLAC__STREAM_METADATA_STREAMINFO_CHANNELS_LEN) && header->channels <= FLAC__MAX_CHANNELS);
	switch(header->channel_assignment) {
		case FLAC__CHANNEL_ASSIGNMENT_INDEPENDENT:
			u = header->channels - 1;
			break;
		case FLAC__CHANNEL_ASSIGNMENT_LEFT_SIDE:
			FLAC__ASSERT(header->channels == 2);
			u = 8;
			break;
		case FLAC__CHANNEL_ASSIGNMENT_RIGHT_SIDE:
			FLAC__ASSERT(header->channels == 2);
			u = 9;
			break;
		case FLAC__CHANNEL_ASSIGNMENT_MID_SIDE:
			FLAC__ASSERT(header->channels == 2);
			u = 10;
			break;
		default:
			FLAC__ASSERT(0);
	}
	if(!FLAC__bitbuffer_write_raw_uint32(bb, u, FLAC__FRAME_HEADER_CHANNEL_ASSIGNMENT_LEN))
		return false;

	FLAC__ASSERT(header->bits_per_sample > 0 && header->bits_per_sample <= (1u << FLAC__STREAM_METADATA_STREAMINFO_BITS_PER_SAMPLE_LEN));
	switch(header->bits_per_sample) {
		case 8 : u = 1; break;
		case 12: u = 2; break;
		case 16: u = 4; break;
		case 20: u = 5; break;
		case 24: u = 6; break;
		default: u = 0; break;
	}
	if(!FLAC__bitbuffer_write_raw_uint32(bb, u, FLAC__FRAME_HEADER_BITS_PER_SAMPLE_LEN))
		return false;

	if(!FLAC__bitbuffer_write_raw_uint32(bb, 0, FLAC__FRAME_HEADER_ZERO_PAD_LEN))
		return false;

	FLAC__ASSERT(header->number_type == FLAC__FRAME_NUMBER_TYPE_FRAME_NUMBER);
	if(!FLAC__bitbuffer_write_utf8_uint32(bb, header->number.frame_number))
		return false;

	if(blocksize_hint)
		if(!FLAC__bitbuffer_write_raw_uint32(bb, header->blocksize-1, (blocksize_hint==6)? 8:16))
			return false;

	switch(sample_rate_hint) {
		case 12:
			if(!FLAC__bitbuffer_write_raw_uint32(bb, header->sample_rate / 1000, 8))
				return false;
			break;
		case 13:
			if(!FLAC__bitbuffer_write_raw_uint32(bb, header->sample_rate, 16))
				return false;
			break;
		case 14:
			if(!FLAC__bitbuffer_write_raw_uint32(bb, header->sample_rate / 10, 16))
				return false;
			break;
	}

	/* write the CRC */
	FLAC__ASSERT(bb->buffer[crc8_start] == 0xff); /* MAGIC NUMBER for the first byte of the sync code */
	FLAC__ASSERT(bb->bits == 0); /* assert that we're byte-aligned */
	crc8 = FLAC__crc8(bb->buffer+crc8_start, bb->bytes-crc8_start);
	if(!FLAC__bitbuffer_write_raw_uint32(bb, crc8, FLAC__FRAME_HEADER_CRC_LEN))
		return false;

	return true;
}

FLAC__bool FLAC__subframe_add_constant(const FLAC__Subframe_Constant *subframe, unsigned subframe_bps, unsigned wasted_bits, FLAC__BitBuffer *bb)
{
	FLAC__bool ok;

	ok =
		FLAC__bitbuffer_write_raw_uint32(bb, FLAC__SUBFRAME_TYPE_CONSTANT_BYTE_ALIGNED_MASK | (wasted_bits? 1:0), FLAC__SUBFRAME_ZERO_PAD_LEN + FLAC__SUBFRAME_TYPE_LEN + FLAC__SUBFRAME_WASTED_BITS_FLAG_LEN) &&
		(wasted_bits? FLAC__bitbuffer_write_unary_unsigned(bb, wasted_bits-1) : true) &&
		FLAC__bitbuffer_write_raw_int32(bb, subframe->value, subframe_bps)
	;

	return ok;
}

FLAC__bool FLAC__subframe_add_fixed(const FLAC__Subframe_Fixed *subframe, unsigned residual_samples, unsigned subframe_bps, unsigned wasted_bits, FLAC__BitBuffer *bb)
{
	unsigned i;

	if(!FLAC__bitbuffer_write_raw_uint32(bb, FLAC__SUBFRAME_TYPE_FIXED_BYTE_ALIGNED_MASK | (subframe->order<<1) | (wasted_bits? 1:0), FLAC__SUBFRAME_ZERO_PAD_LEN + FLAC__SUBFRAME_TYPE_LEN + FLAC__SUBFRAME_WASTED_BITS_FLAG_LEN))
		return false;
	if(wasted_bits)
		if(!FLAC__bitbuffer_write_unary_unsigned(bb, wasted_bits-1))
			return false;

	for(i = 0; i < subframe->order; i++)
		if(!FLAC__bitbuffer_write_raw_int32(bb, subframe->warmup[i], subframe_bps))
			return false;

	if(!subframe_add_entropy_coding_method_(bb, &subframe->entropy_coding_method))
		return false;
	switch(subframe->entropy_coding_method.type) {
		case FLAC__ENTROPY_CODING_METHOD_PARTITIONED_RICE:
			if(!subframe_add_residual_partitioned_rice_(bb, subframe->residual, residual_samples, subframe->order, subframe->entropy_coding_method.data.partitioned_rice.parameters, subframe->entropy_coding_method.data.partitioned_rice.raw_bits, subframe->entropy_coding_method.data.partitioned_rice.order))
				return false;
			break;
		default:
			FLAC__ASSERT(0);
	}

	return true;
}

FLAC__bool FLAC__subframe_add_lpc(const FLAC__Subframe_LPC *subframe, unsigned residual_samples, unsigned subframe_bps, unsigned wasted_bits, FLAC__BitBuffer *bb)
{
	unsigned i;

	if(!FLAC__bitbuffer_write_raw_uint32(bb, FLAC__SUBFRAME_TYPE_LPC_BYTE_ALIGNED_MASK | ((subframe->order-1)<<1) | (wasted_bits? 1:0), FLAC__SUBFRAME_ZERO_PAD_LEN + FLAC__SUBFRAME_TYPE_LEN + FLAC__SUBFRAME_WASTED_BITS_FLAG_LEN))
		return false;
	if(wasted_bits)
		if(!FLAC__bitbuffer_write_unary_unsigned(bb, wasted_bits-1))
			return false;

	for(i = 0; i < subframe->order; i++)
		if(!FLAC__bitbuffer_write_raw_int32(bb, subframe->warmup[i], subframe_bps))
			return false;

	if(!FLAC__bitbuffer_write_raw_uint32(bb, subframe->qlp_coeff_precision-1, FLAC__SUBFRAME_LPC_QLP_COEFF_PRECISION_LEN))
		return false;
	if(!FLAC__bitbuffer_write_raw_int32(bb, subframe->quantization_level, FLAC__SUBFRAME_LPC_QLP_SHIFT_LEN))
		return false;
	for(i = 0; i < subframe->order; i++)
		if(!FLAC__bitbuffer_write_raw_int32(bb, subframe->qlp_coeff[i], subframe->qlp_coeff_precision))
			return false;

	if(!subframe_add_entropy_coding_method_(bb, &subframe->entropy_coding_method))
		return false;
	switch(subframe->entropy_coding_method.type) {
		case FLAC__ENTROPY_CODING_METHOD_PARTITIONED_RICE:
			if(!subframe_add_residual_partitioned_rice_(bb, subframe->residual, residual_samples, subframe->order, subframe->entropy_coding_method.data.partitioned_rice.parameters, subframe->entropy_coding_method.data.partitioned_rice.raw_bits, subframe->entropy_coding_method.data.partitioned_rice.order))
				return false;
			break;
		default:
			FLAC__ASSERT(0);
	}

	return true;
}

FLAC__bool FLAC__subframe_add_verbatim(const FLAC__Subframe_Verbatim *subframe, unsigned samples, unsigned subframe_bps, unsigned wasted_bits, FLAC__BitBuffer *bb)
{
	unsigned i;
	const FLAC__int32 *signal = subframe->data;

	if(!FLAC__bitbuffer_write_raw_uint32(bb, FLAC__SUBFRAME_TYPE_VERBATIM_BYTE_ALIGNED_MASK | (wasted_bits? 1:0), FLAC__SUBFRAME_ZERO_PAD_LEN + FLAC__SUBFRAME_TYPE_LEN + FLAC__SUBFRAME_WASTED_BITS_FLAG_LEN))
		return false;
	if(wasted_bits)
		if(!FLAC__bitbuffer_write_unary_unsigned(bb, wasted_bits-1))
			return false;

	for(i = 0; i < samples; i++)
		if(!FLAC__bitbuffer_write_raw_int32(bb, signal[i], subframe_bps))
			return false;

	return true;
}

FLAC__bool subframe_add_entropy_coding_method_(FLAC__BitBuffer *bb, const FLAC__EntropyCodingMethod *method)
{
	if(!FLAC__bitbuffer_write_raw_uint32(bb, method->type, FLAC__ENTROPY_CODING_METHOD_TYPE_LEN))
		return false;
	switch(method->type) {
		case FLAC__ENTROPY_CODING_METHOD_PARTITIONED_RICE:
			if(!FLAC__bitbuffer_write_raw_uint32(bb, method->data.partitioned_rice.order, FLAC__ENTROPY_CODING_METHOD_PARTITIONED_RICE_ORDER_LEN))
				return false;
			break;
		default:
			FLAC__ASSERT(0);
	}
	return true;
}

FLAC__bool subframe_add_residual_partitioned_rice_(FLAC__BitBuffer *bb, const FLAC__int32 residual[], const unsigned residual_samples, const unsigned predictor_order, const unsigned rice_parameters[], const unsigned raw_bits[], const unsigned partition_order)
{
	if(partition_order == 0) {
		unsigned i;

		if(!FLAC__bitbuffer_write_raw_uint32(bb, rice_parameters[0], FLAC__ENTROPY_CODING_METHOD_PARTITIONED_RICE_PARAMETER_LEN))
			return false;
		if(rice_parameters[0] < FLAC__ENTROPY_CODING_METHOD_PARTITIONED_RICE_ESCAPE_PARAMETER) {
			for(i = 0; i < residual_samples; i++) {
#ifdef FLAC__SYMMETRIC_RICE
				if(!FLAC__bitbuffer_write_symmetric_rice_signed(bb, residual[i], rice_parameters[0]))
					return false;
#else
				if(!FLAC__bitbuffer_write_rice_signed(bb, residual[i], rice_parameters[0]))
					return false;
#endif
			}
		}
		else {
			if(!FLAC__bitbuffer_write_raw_uint32(bb, raw_bits[0], FLAC__ENTROPY_CODING_METHOD_PARTITIONED_RICE_RAW_LEN))
				return false;
			for(i = 0; i < residual_samples; i++) {
				if(!FLAC__bitbuffer_write_raw_int32(bb, residual[i], raw_bits[0]))
					return false;
			}
		}
		return true;
	}
	else {
		unsigned i, j, k = 0, k_last = 0;
		unsigned partition_samples;
		const unsigned default_partition_samples = (residual_samples+predictor_order) >> partition_order;
		for(i = 0; i < (1u<<partition_order); i++) {
			if(!FLAC__bitbuffer_write_raw_uint32(bb, rice_parameters[i], FLAC__ENTROPY_CODING_METHOD_PARTITIONED_RICE_PARAMETER_LEN))
				return false;
			partition_samples = default_partition_samples;
			if(i == 0)
				partition_samples -= predictor_order;
			k += partition_samples;
			if(rice_parameters[i] < FLAC__ENTROPY_CODING_METHOD_PARTITIONED_RICE_ESCAPE_PARAMETER) {
				for(j = k_last; j < k; j++) {
#ifdef FLAC__SYMMETRIC_RICE
					if(!FLAC__bitbuffer_write_symmetric_rice_signed(bb, residual[j], rice_parameters[i]))
						return false;
#else
					if(!FLAC__bitbuffer_write_rice_signed(bb, residual[j], rice_parameters[i]))
						return false;
#endif
				}
			}
			else {
				if(!FLAC__bitbuffer_write_raw_uint32(bb, raw_bits[i], FLAC__ENTROPY_CODING_METHOD_PARTITIONED_RICE_RAW_LEN))
					return false;
				for(j = k_last; j < k; j++) {
					if(!FLAC__bitbuffer_write_raw_int32(bb, residual[j], raw_bits[i]))
						return false;
				}
			}
			k_last = k;
		}
		return true;
	}
}
