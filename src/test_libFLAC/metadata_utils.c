/* test_libFLAC - Unit tester for libFLAC
 * Copyright (C) 2002  Josh Coalson
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

/*
 * These are not tests, just utility functions used by the metadata tests
 */

#include "metadata_utils.h"
#include "FLAC/metadata.h"
#include <stdio.h>
#include <stdlib.h> /* for malloc() */
#include <string.h> /* for memcmp() */

FLAC__bool compare_block_data_streaminfo_(const FLAC__StreamMetadata_StreamInfo *block, const FLAC__StreamMetadata_StreamInfo *blockcopy)
{
	if(blockcopy->min_blocksize != block->min_blocksize) {
		printf("FAILED, min_blocksize mismatch, expected %u, got %u\n", block->min_blocksize, blockcopy->min_blocksize);
		return false;
	}
	if(blockcopy->max_blocksize != block->max_blocksize) {
		printf("FAILED, max_blocksize mismatch, expected %u, got %u\n", block->max_blocksize, blockcopy->max_blocksize);
		return false;
	}
	if(blockcopy->min_framesize != block->min_framesize) {
		printf("FAILED, min_framesize mismatch, expected %u, got %u\n", block->min_framesize, blockcopy->min_framesize);
		return false;
	}
	if(blockcopy->max_framesize != block->max_framesize) {
		printf("FAILED, max_framesize mismatch, expected %u, got %u\n", block->max_framesize, blockcopy->max_framesize);
		return false;
	}
	if(blockcopy->sample_rate != block->sample_rate) {
		printf("FAILED, sample_rate mismatch, expected %u, got %u\n", block->sample_rate, blockcopy->sample_rate);
		return false;
	}
	if(blockcopy->channels != block->channels) {
		printf("FAILED, channels mismatch, expected %u, got %u\n", block->channels, blockcopy->channels);
		return false;
	}
	if(blockcopy->bits_per_sample != block->bits_per_sample) {
		printf("FAILED, bits_per_sample mismatch, expected %u, got %u\n", block->bits_per_sample, blockcopy->bits_per_sample);
		return false;
	}
	if(blockcopy->total_samples != block->total_samples) {
		printf("FAILED, total_samples mismatch, expected %llu, got %llu\n", block->total_samples, blockcopy->total_samples);
		return false;
	}
	if(0 != memcmp(blockcopy->md5sum, block->md5sum, sizeof(block->md5sum))) {
		printf("FAILED, md5sum mismatch, expected %02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X, got %02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X\n",
			(unsigned)block->md5sum[0],
			(unsigned)block->md5sum[1],
			(unsigned)block->md5sum[2],
			(unsigned)block->md5sum[3],
			(unsigned)block->md5sum[4],
			(unsigned)block->md5sum[5],
			(unsigned)block->md5sum[6],
			(unsigned)block->md5sum[7],
			(unsigned)block->md5sum[8],
			(unsigned)block->md5sum[9],
			(unsigned)block->md5sum[10],
			(unsigned)block->md5sum[11],
			(unsigned)block->md5sum[12],
			(unsigned)block->md5sum[13],
			(unsigned)block->md5sum[14],
			(unsigned)block->md5sum[15],
			(unsigned)blockcopy->md5sum[0],
			(unsigned)blockcopy->md5sum[1],
			(unsigned)blockcopy->md5sum[2],
			(unsigned)blockcopy->md5sum[3],
			(unsigned)blockcopy->md5sum[4],
			(unsigned)blockcopy->md5sum[5],
			(unsigned)blockcopy->md5sum[6],
			(unsigned)blockcopy->md5sum[7],
			(unsigned)blockcopy->md5sum[8],
			(unsigned)blockcopy->md5sum[9],
			(unsigned)blockcopy->md5sum[10],
			(unsigned)blockcopy->md5sum[11],
			(unsigned)blockcopy->md5sum[12],
			(unsigned)blockcopy->md5sum[13],
			(unsigned)blockcopy->md5sum[14],
			(unsigned)blockcopy->md5sum[15]
		);
		return false;
	}
	return true;
}

FLAC__bool compare_block_data_padding_(const FLAC__StreamMetadata_Padding *block, const FLAC__StreamMetadata_Padding *blockcopy, unsigned block_length)
{
	/* we don't compare the padding guts */
	(void)block, (void)blockcopy, (void)block_length;
	return true;
}

FLAC__bool compare_block_data_application_(const FLAC__StreamMetadata_Application *block, const FLAC__StreamMetadata_Application *blockcopy, unsigned block_length)
{
	if(block_length < sizeof(block->id)) {
		printf("FAILED, bad block length = %u\n", block_length);
		return false;
	}
	if(0 != memcmp(blockcopy->id, block->id, sizeof(block->id))) {
		printf("FAILED, id mismatch, expected %02X%02X%02X%02X, got %02X%02X%02X%02X\n",
			(unsigned)block->id[0],
			(unsigned)block->id[1],
			(unsigned)block->id[2],
			(unsigned)block->id[3],
			(unsigned)blockcopy->id[0],
			(unsigned)blockcopy->id[1],
			(unsigned)blockcopy->id[2],
			(unsigned)blockcopy->id[3]
		);
		return false;
	}
	if(0 == block->data || 0 == blockcopy->data) {
		if(block->data != blockcopy->data) {
			printf("FAILED, data mismatch (%s's data pointer is null)\n", 0==block->data?"original":"copy");
			return false;
		}
		else if(block_length - sizeof(block->id) > 0) {
			printf("FAILED, data pointer is null but block length is not 0\n");
			return false;
		}
	}
	else {
		if(block_length - sizeof(block->id) == 0) {
			printf("FAILED, data pointer is not null but block length is 0\n");
			return false;
		}
		else if(0 != memcmp(blockcopy->data, block->data, block_length - sizeof(block->id))) {
			printf("FAILED, data mismatch\n");
			return false;
		}
	}
	return true;
}

FLAC__bool compare_block_data_seektable_(const FLAC__StreamMetadata_SeekTable *block, const FLAC__StreamMetadata_SeekTable *blockcopy)
{
	unsigned i;
	if(blockcopy->num_points != block->num_points) {
		printf("FAILED, num_points mismatch, expected %u, got %u\n", block->num_points, blockcopy->num_points);
		return false;
	}
	for(i = 0; i < block->num_points; i++) {
		if(blockcopy->points[i].sample_number != block->points[i].sample_number) {
			printf("FAILED, points[%u].sample_number mismatch, expected %llu, got %llu\n", i, block->points[i].sample_number, blockcopy->points[i].sample_number);
			return false;
		}
		if(blockcopy->points[i].stream_offset != block->points[i].stream_offset) {
			printf("FAILED, points[%u].stream_offset mismatch, expected %llu, got %llu\n", i, block->points[i].stream_offset, blockcopy->points[i].stream_offset);
			return false;
		}
		if(blockcopy->points[i].frame_samples != block->points[i].frame_samples) {
			printf("FAILED, points[%u].frame_samples mismatch, expected %u, got %u\n", i, block->points[i].frame_samples, blockcopy->points[i].frame_samples);
			return false;
		}
	}
	return true;
}

FLAC__bool compare_block_data_vorbiscomment_(const FLAC__StreamMetadata_VorbisComment *block, const FLAC__StreamMetadata_VorbisComment *blockcopy)
{
	unsigned i;
	if(blockcopy->vendor_string.length != block->vendor_string.length) {
		printf("FAILED, vendor_string.length mismatch, expected %u, got %u\n", block->vendor_string.length, blockcopy->vendor_string.length);
		return false;
	}
	if(0 == block->vendor_string.entry || 0 == blockcopy->vendor_string.entry) {
		if(block->vendor_string.entry != blockcopy->vendor_string.entry) {
			printf("FAILED, vendor_string.entry mismatch\n");
			return false;
		}
	}
	else if(0 != memcmp(blockcopy->vendor_string.entry, block->vendor_string.entry, block->vendor_string.length)) {
		printf("FAILED, vendor_string.entry mismatch\n");
		return false;
	}
	if(blockcopy->num_comments != block->num_comments) {
		printf("FAILED, num_comments mismatch, expected %u, got %u\n", block->num_comments, blockcopy->num_comments);
		return false;
	}
	for(i = 0; i < block->num_comments; i++) {
		if(blockcopy->comments[i].length != block->comments[i].length) {
			printf("FAILED, comments[%u].length mismatch, expected %u, got %u\n", i, block->comments[i].length, blockcopy->comments[i].length);
			return false;
		}
		if(0 == block->comments[i].entry || 0 == blockcopy->comments[i].entry) {
			if(block->comments[i].entry != blockcopy->comments[i].entry) {
				printf("FAILED, comments[%u].entry mismatch\n", i);
				return false;
			}
		}
		else {
			if(0 != memcmp(blockcopy->comments[i].entry, block->comments[i].entry, block->comments[i].length)) {
				printf("FAILED, comments[%u].entry mismatch\n", i);
				return false;
			}
		}
	}
	return true;
}

FLAC__bool compare_block_(const FLAC__StreamMetadata *block, const FLAC__StreamMetadata *blockcopy)
{
	if(blockcopy->type != block->type) {
		printf("FAILED, type mismatch, expected %s, got %s\n", FLAC__MetadataTypeString[block->type], FLAC__MetadataTypeString[blockcopy->type]);
		return false;
	}
	if(blockcopy->is_last != block->is_last) {
		printf("FAILED, is_last mismatch, expected %u, got %u\n", (unsigned)block->is_last, (unsigned)blockcopy->is_last);
		return false;
	}
	if(blockcopy->length != block->length) {
		printf("FAILED, length mismatch, expected %u, got %u\n", block->length, blockcopy->length);
		return false;
	}
	switch(block->type) {
		case FLAC__METADATA_TYPE_STREAMINFO:
			return compare_block_data_streaminfo_(&block->data.stream_info, &blockcopy->data.stream_info);
		case FLAC__METADATA_TYPE_PADDING:
			return compare_block_data_padding_(&block->data.padding, &blockcopy->data.padding, block->length);
		case FLAC__METADATA_TYPE_APPLICATION:
			return compare_block_data_application_(&block->data.application, &blockcopy->data.application, block->length);
		case FLAC__METADATA_TYPE_SEEKTABLE:
			return compare_block_data_seektable_(&block->data.seek_table, &blockcopy->data.seek_table);
		case FLAC__METADATA_TYPE_VORBIS_COMMENT:
			return compare_block_data_vorbiscomment_(&block->data.vorbis_comment, &blockcopy->data.vorbis_comment);
		default:
			printf("FAILED, invalid block type %u\n", (unsigned)block->type);
			return false;
	}
}
