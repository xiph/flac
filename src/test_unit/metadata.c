/* test_unit - Simple FLAC unit tester
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

#include "FLAC/metadata.h"
#include <stdio.h>
#include <stdlib.h> /* for malloc() */
#include <string.h> /* for memcmp() */

static FLAC__bool compare_block_data_streaminfo_(const FLAC__StreamMetaData_StreamInfo *block, const FLAC__StreamMetaData_StreamInfo *blockcopy)
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

static FLAC__bool compare_block_data_padding_(const FLAC__StreamMetaData_Padding *block, const FLAC__StreamMetaData_Padding *blockcopy, unsigned block_length)
{
	/* we don't compare the padding guts */
	(void)block, (void)blockcopy, (void)block_length;
	return true;
}

static FLAC__bool compare_block_data_application_(const FLAC__StreamMetaData_Application *block, const FLAC__StreamMetaData_Application *blockcopy, unsigned block_length)
{
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
			printf("FAILED, data mismatch\n");
			return false;
		}
	}
	else {
		if(0 != memcmp(blockcopy->data, block->data, block_length - sizeof(block->id))) {
			printf("FAILED, data mismatch\n");
			return false;
		}
	}
	return true;
}

static FLAC__bool compare_block_data_seektable_(const FLAC__StreamMetaData_SeekTable *block, const FLAC__StreamMetaData_SeekTable *blockcopy)
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

static FLAC__bool compare_block_data_vorbiscomment_(const FLAC__StreamMetaData_VorbisComment *block, const FLAC__StreamMetaData_VorbisComment *blockcopy)
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
	if(0 != memcmp(blockcopy->vendor_string.entry, block->vendor_string.entry, block->vendor_string.length)) {
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

static FLAC__bool compare_block_(const FLAC__StreamMetaData *block, const FLAC__StreamMetaData *blockcopy)
{
	if(blockcopy->type != block->type) {
		printf("FAILED, type mismatch, expected %s, got %s\n", FLAC__MetaDataTypeString[block->type], FLAC__MetaDataTypeString[blockcopy->type]);
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

static FLAC__bool compare_seekpoint_array_(const FLAC__StreamMetaData_SeekPoint *seekpoint_array, const FLAC__StreamMetaData_SeekPoint *seekpoint_array_copy, unsigned num_points)
{
	if(0 != memcmp(seekpoint_array, seekpoint_array_copy, num_points * sizeof(FLAC__StreamMetaData_SeekPoint))) {
		printf("FAILED, seekpoint mismatch\n");
		return false;
	}
	return true;
}

static FLAC__bool compare_vorbiscomment_entry_array_(const FLAC__StreamMetaData_VorbisComment_Entry *vorbiscomment_entry_array, const FLAC__StreamMetaData_VorbisComment_Entry *vorbiscomment_entry_array_copy, unsigned num_comments)
{
	unsigned i;

	for(i = 0; i < num_comments; i++) {
		if(vorbiscomment_entry_array[i].length != vorbiscomment_entry_array_copy[i].length) {
			printf("FAILED, entry[%u].length mismatch\n", i);
			return false;
		}
		if(0 == vorbiscomment_entry_array[i].entry || 0 == vorbiscomment_entry_array_copy[i].entry) {
			if(vorbiscomment_entry_array[i].entry != vorbiscomment_entry_array_copy[i].entry) {
				printf("FAILED, comments[%u].entry mismatch\n", i);
				return false;
			}
		}
		else {
			if(0 != memcmp(vorbiscomment_entry_array[i].entry, vorbiscomment_entry_array_copy[i].entry, vorbiscomment_entry_array[i].length)) {
				printf("FAILED, entry[%u].entry mismatch\n", i);
				return false;
			}
		}
	}
	return true;
}

static FLAC__byte *make_dummydata_(FLAC__byte *dummydata, unsigned len)
{
	FLAC__byte *ret;

	if(0 == (ret = malloc(len))) {
		printf("FAILED, malloc error\n");
		exit(1);
	}
	else
		memcpy(ret, dummydata, len);

	return ret;
}

int test_metadata()
{
	FLAC__StreamMetaData *block, *blockcopy;
	FLAC__StreamMetaData_SeekPoint *seekpoint_array, *seekpoint_array_copy;
	FLAC__StreamMetaData_VorbisComment_Entry *vorbiscomment_entry_array, *vorbiscomment_entry_array_copy;
	unsigned expected_length;
	static FLAC__byte dummydata[4] = { 'a', 'b', 'c', 'd' };

	printf("\n+++ unit test: metadata\n\n");


	printf("testing STREAMINFO\n");

	printf("testing FLAC__metadata_object_new()... ");
	block = FLAC__metadata_object_new(FLAC__METADATA_TYPE_STREAMINFO);
	if(0 == block) {
		printf("FAILED, returned NULL\n");
		return 1;
	}
	expected_length = FLAC__STREAM_METADATA_STREAMINFO_LENGTH;
	if(block->length != expected_length) {
		printf("FAILED, bad length, expected %u, got %u\n", expected_length, block->length);
		return 1;
    }
	printf("OK\n");

	printf("testing FLAC__metadata_object_copy()... ");
	blockcopy = FLAC__metadata_object_copy(block);
	if(0 == blockcopy) {
		printf("FAILED, returned NULL\n");
		return 1;
	}
	if(!compare_block_(block, blockcopy))
		return 1;
	printf("OK\n");

	printf("testing FLAC__metadata_object_delete()... ");
	FLAC__metadata_object_delete(blockcopy);
	FLAC__metadata_object_delete(block);
	printf("OK\n");


	printf("testing PADDING\n");

	printf("testing FLAC__metadata_object_new()... ");
	block = FLAC__metadata_object_new(FLAC__METADATA_TYPE_PADDING);
	if(0 == block) {
		printf("FAILED, returned NULL\n");
		return 1;
	}
	expected_length = 0;
	if(block->length != expected_length) {
		printf("FAILED, bad length, expected %u, got %u\n", expected_length, block->length);
		return 1;
    }
	printf("OK\n");

	printf("testing FLAC__metadata_object_copy()... ");
	blockcopy = FLAC__metadata_object_copy(block);
	if(0 == blockcopy) {
		printf("FAILED, returned NULL\n");
		return 1;
	}
	if(!compare_block_(block, blockcopy))
		return 1;
	printf("OK\n");

	printf("testing FLAC__metadata_object_delete()... ");
	FLAC__metadata_object_delete(blockcopy);
	FLAC__metadata_object_delete(block);
	printf("OK\n");


	printf("testing APPLICATION\n");

	printf("testing FLAC__metadata_object_new()... ");
	block = FLAC__metadata_object_new(FLAC__METADATA_TYPE_APPLICATION);
	if(0 == block) {
		printf("FAILED, returned NULL\n");
		return 1;
	}
	expected_length = FLAC__STREAM_METADATA_APPLICATION_ID_LEN / 8;
	if(block->length != expected_length) {
		printf("FAILED, bad length, expected %u, got %u\n", expected_length, block->length);
		return 1;
    }
	printf("OK\n");

	printf("testing FLAC__metadata_object_copy()... ");
	blockcopy = FLAC__metadata_object_copy(block);
	if(0 == blockcopy) {
		printf("FAILED, returned NULL\n");
		return 1;
	}
	if(!compare_block_(block, blockcopy))
		return 1;
	printf("OK\n");

	printf("testing FLAC__metadata_object_delete()... ");
	FLAC__metadata_object_delete(blockcopy);
	printf("OK\n");

	printf("testing FLAC__metadata_object_application_set_data(copy)... ");
	if(!FLAC__metadata_object_application_set_data(block, dummydata, sizeof(dummydata), true/*copy*/)) {
		printf("FAILED, returned false\n");
		return 1;
	}
	expected_length = (FLAC__STREAM_METADATA_APPLICATION_ID_LEN / 8) + sizeof(dummydata);
	if(block->length != expected_length) {
		printf("FAILED, bad length, expected %u, got %u\n", expected_length, block->length);
		return 1;
	}
	if(0 != memcmp(block->data.application.data, dummydata, sizeof(dummydata))) {
		printf("FAILED, data mismatch\n");
		return 1;
	}
	printf("OK\n");

	printf("testing FLAC__metadata_object_copy()... ");
	blockcopy = FLAC__metadata_object_copy(block);
	if(0 == blockcopy) {
		printf("FAILED, returned NULL\n");
		return 1;
	}
	if(!compare_block_(block, blockcopy))
		return 1;
	printf("OK\n");

	printf("testing FLAC__metadata_object_delete()... ");
	FLAC__metadata_object_delete(blockcopy);
	printf("OK\n");

	printf("testing FLAC__metadata_object_application_set_data(own)... ");
	if(!FLAC__metadata_object_application_set_data(block, make_dummydata_(dummydata, sizeof(dummydata)), sizeof(dummydata), false/*own*/)) {
		printf("FAILED, returned false\n");
		return 1;
	}
	expected_length = (FLAC__STREAM_METADATA_APPLICATION_ID_LEN / 8) + sizeof(dummydata);
	if(block->length != expected_length) {
		printf("FAILED, bad length, expected %u, got %u\n", expected_length, block->length);
		return 1;
	}
	if(0 != memcmp(block->data.application.data, dummydata, sizeof(dummydata))) {
		printf("FAILED, data mismatch\n");
		return 1;
	}
	printf("OK\n");

	printf("testing FLAC__metadata_object_copy()... ");
	blockcopy = FLAC__metadata_object_copy(block);
	if(0 == blockcopy) {
		printf("FAILED, returned NULL\n");
		return 1;
	}
	if(!compare_block_(block, blockcopy))
		return 1;
	printf("OK\n");

	printf("testing FLAC__metadata_object_delete()... ");
	FLAC__metadata_object_delete(blockcopy);
	FLAC__metadata_object_delete(block);
	printf("OK\n");


	printf("testing SEEKTABLE\n");

	printf("testing FLAC__metadata_object_new()... ");
	block = FLAC__metadata_object_new(FLAC__METADATA_TYPE_SEEKTABLE);
	if(0 == block) {
		printf("FAILED, returned NULL\n");
		return 1;
	}
	expected_length = 0;
	if(block->length != expected_length) {
		printf("FAILED, bad length, expected %u, got %u\n", expected_length, block->length);
		return 1;
    }
	printf("OK\n");

	printf("testing FLAC__metadata_object_copy()... ");
	blockcopy = FLAC__metadata_object_copy(block);
	if(0 == blockcopy) {
		printf("FAILED, returned NULL\n");
		return 1;
	}
	if(!compare_block_(block, blockcopy))
		return 1;
	printf("OK\n");

	printf("testing FLAC__metadata_object_delete()... ");
	FLAC__metadata_object_delete(blockcopy);
	printf("OK\n");

	printf("testing FLAC__metadata_object_seekpoint_array_new()... ");
	seekpoint_array = FLAC__metadata_object_seekpoint_array_new(1u);
	if(0 == seekpoint_array) {
		printf("FAILED, returned NULL\n");
		return 1;
	}
	printf("OK\n");

	seekpoint_array[0].sample_number = 1;
	seekpoint_array[0].stream_offset = 2;
	seekpoint_array[0].frame_samples = 3;

	printf("testing FLAC__metadata_object_seekpoint_array_copy()... ");
	seekpoint_array_copy = FLAC__metadata_object_seekpoint_array_copy(seekpoint_array, 1u);
	if(0 == seekpoint_array_copy) {
		printf("FAILED, returned NULL\n");
		return 1;
	}
	if(!compare_seekpoint_array_(seekpoint_array, seekpoint_array_copy, 1u))
		return 1;
	printf("OK\n");

	printf("testing FLAC__metadata_object_seekpoint_array_resize(grow)...");
	if(!FLAC__metadata_object_seekpoint_array_resize(&seekpoint_array_copy, 1u, 10u)) {
		printf("FAILED, returned false\n");
		return 1;
	}
	if(!compare_seekpoint_array_(seekpoint_array, seekpoint_array_copy, 1u))
		return 1;
	printf("OK\n");

	printf("testing FLAC__metadata_object_seekpoint_array_resize(shrink)...");
	if(!FLAC__metadata_object_seekpoint_array_resize(&seekpoint_array_copy, 10u, 1u)) {
		printf("FAILED, returned false\n");
		return 1;
	}
	if(!compare_seekpoint_array_(seekpoint_array, seekpoint_array_copy, 1u))
		return 1;
	printf("OK\n");

	printf("testing FLAC__metadata_object_seekpoint_array_delete()... ");
	FLAC__metadata_object_seekpoint_array_delete(seekpoint_array_copy);
	printf("OK\n");

	printf("testing FLAC__metadata_object_seektable_set_points(copy)... ");
	if(!FLAC__metadata_object_seektable_set_points(block, seekpoint_array, 1u, true/*copy*/)) {
		printf("FAILED, returned false\n");
		return 1;
	}
	expected_length = 1u * FLAC__STREAM_METADATA_SEEKPOINT_LENGTH;
	if(block->length != expected_length) {
		printf("FAILED, bad length, expected %u, got %u\n", expected_length, block->length);
		return 1;
	}
	if(block->data.seek_table.num_points != 1u) {
		printf("FAILED, bad num_points, expected %u, got %u\n", 1u, block->data.seek_table.num_points);
		return 1;
	}
	if(!compare_seekpoint_array_(block->data.seek_table.points, seekpoint_array, 1u))
		return 1;
	printf("OK\n");

	printf("testing FLAC__metadata_object_copy()... ");
	blockcopy = FLAC__metadata_object_copy(block);
	if(0 == blockcopy) {
		printf("FAILED, returned NULL\n");
		return 1;
	}
	if(!compare_block_(block, blockcopy))
		return 1;
	printf("OK\n");

	printf("testing FLAC__metadata_object_delete()... ");
	FLAC__metadata_object_delete(blockcopy);
	printf("OK\n");

	printf("testing FLAC__metadata_object_seektable_set_points(own)... ");
	if(!FLAC__metadata_object_seektable_set_points(block, seekpoint_array, 1u, false/*own*/)) {
		printf("FAILED, returned false\n");
		return 1;
	}
	expected_length = 1u * FLAC__STREAM_METADATA_SEEKPOINT_LENGTH;
	if(block->length != expected_length) {
		printf("FAILED, bad length, expected %u, got %u\n", expected_length, block->length);
		return 1;
	}
	printf("OK\n");

	printf("testing FLAC__metadata_object_copy()... ");
	blockcopy = FLAC__metadata_object_copy(block);
	if(0 == blockcopy) {
		printf("FAILED, returned NULL\n");
		return 1;
	}
	if(!compare_block_(block, blockcopy))
		return 1;
	printf("OK\n");

	printf("testing FLAC__metadata_object_delete()... ");
	FLAC__metadata_object_delete(blockcopy);
	FLAC__metadata_object_delete(block);
	printf("OK\n");


	printf("testing VORBIS_COMMENT\n");

	printf("testing FLAC__metadata_object_new()... ");
	block = FLAC__metadata_object_new(FLAC__METADATA_TYPE_VORBIS_COMMENT);
	if(0 == block) {
		printf("FAILED, returned NULL\n");
		return 1;
	}
	expected_length = (FLAC__STREAM_METADATA_VORBIS_COMMENT_ENTRY_LENGTH_LEN + FLAC__STREAM_METADATA_VORBIS_COMMENT_NUM_COMMENTS_LEN) / 8;
	if(block->length != expected_length) {
		printf("FAILED, bad length, expected %u, got %u\n", expected_length, block->length);
		return 1;
    }
	printf("OK\n");

	printf("testing FLAC__metadata_object_copy()... ");
	blockcopy = FLAC__metadata_object_copy(block);
	if(0 == blockcopy) {
		printf("FAILED, returned NULL\n");
		return 1;
	}
	if(!compare_block_(block, blockcopy))
		return 1;
	printf("OK\n");

	printf("testing FLAC__metadata_object_delete()... ");
	FLAC__metadata_object_delete(blockcopy);
	printf("OK\n");

	printf("testing FLAC__metadata_object_vorbiscomment_set_vendor_string(copy)\n");
	if(!FLAC__metadata_object_vorbiscomment_set_vendor_string(block, dummydata, sizeof(dummydata), true/*copy*/)) {
		printf("FAILED, returned false\n");
		return 1;
	}
	expected_length = ((FLAC__STREAM_METADATA_VORBIS_COMMENT_ENTRY_LENGTH_LEN / 8) + sizeof(dummydata)) + (FLAC__STREAM_METADATA_VORBIS_COMMENT_NUM_COMMENTS_LEN / 8) + block->data.vorbis_comment.num_comments * ((FLAC__STREAM_METADATA_VORBIS_COMMENT_ENTRY_LENGTH_LEN / 8) + sizeof(dummydata));
	if(block->length != expected_length) {
		printf("FAILED, bad length, expected %u, got %u\n", expected_length, block->length);
		return 1;
	}

	printf("testing FLAC__metadata_object_vorbiscomment_set_vendor_string(own)\n");
	if(!FLAC__metadata_object_vorbiscomment_set_vendor_string(block, make_dummydata_(dummydata, sizeof(dummydata)), sizeof(dummydata), false/*own*/)) {
		printf("FAILED, returned false\n");
		return 1;
	}
	expected_length = ((FLAC__STREAM_METADATA_VORBIS_COMMENT_ENTRY_LENGTH_LEN / 8) + sizeof(dummydata)) + (FLAC__STREAM_METADATA_VORBIS_COMMENT_NUM_COMMENTS_LEN / 8) + block->data.vorbis_comment.num_comments * ((FLAC__STREAM_METADATA_VORBIS_COMMENT_ENTRY_LENGTH_LEN / 8) + sizeof(dummydata));
	if(block->length != expected_length) {
		printf("FAILED, bad length, expected %u, got %u\n", expected_length, block->length);
		return 1;
	}

	printf("testing FLAC__metadata_object_vorbiscomment_entry_array_new()... ");
	vorbiscomment_entry_array = FLAC__metadata_object_vorbiscomment_entry_array_new(1u);
	if(0 == vorbiscomment_entry_array) {
		printf("FAILED, returned NULL\n");
		return 1;
	}
	printf("OK\n");

	vorbiscomment_entry_array[0].length = sizeof(dummydata);
	vorbiscomment_entry_array[0].entry = make_dummydata_(dummydata, sizeof(dummydata));

	printf("testing FLAC__metadata_object_vorbiscomment_entry_array_copy()... ");
	vorbiscomment_entry_array_copy = FLAC__metadata_object_vorbiscomment_entry_array_copy(vorbiscomment_entry_array, 1u);
	if(0 == vorbiscomment_entry_array_copy) {
		printf("FAILED, returned NULL\n");
		return 1;
	}
	if(!compare_vorbiscomment_entry_array_(vorbiscomment_entry_array, vorbiscomment_entry_array_copy, 1u))
		return 1;
	printf("OK\n");

	printf("testing FLAC__metadata_object_vorbiscomment_entry_array_resize(grow)...");
	if(!FLAC__metadata_object_vorbiscomment_entry_array_resize(&vorbiscomment_entry_array_copy, 1u, 10u)) {
		printf("FAILED, returned false\n");
		return 1;
	}
	if(!compare_vorbiscomment_entry_array_(vorbiscomment_entry_array, vorbiscomment_entry_array_copy, 1u))
		return 1;
	printf("OK\n");

	printf("testing FLAC__metadata_object_vorbiscomment_entry_array_resize(shrink)...");
	if(!FLAC__metadata_object_vorbiscomment_entry_array_resize(&vorbiscomment_entry_array_copy, 10u, 1u)) {
		printf("FAILED, returned false\n");
		return 1;
	}
	if(!compare_vorbiscomment_entry_array_(vorbiscomment_entry_array, vorbiscomment_entry_array_copy, 1u))
		return 1;
	printf("OK\n");

	printf("testing FLAC__metadata_object_vorbiscomment_entry_array_delete()... ");
	FLAC__metadata_object_vorbiscomment_entry_array_delete(vorbiscomment_entry_array_copy, 1u);
	printf("OK\n");

	printf("testing FLAC__metadata_object_vorbiscomment_set_comments(copy)... ");
	if(!FLAC__metadata_object_vorbiscomment_set_comments(block, vorbiscomment_entry_array, 1u, true/*copy*/)) {
		printf("FAILED, returned false\n");
		return 1;
	}
	expected_length = ((FLAC__STREAM_METADATA_VORBIS_COMMENT_ENTRY_LENGTH_LEN / 8) + sizeof(dummydata)) + (FLAC__STREAM_METADATA_VORBIS_COMMENT_NUM_COMMENTS_LEN / 8) + block->data.vorbis_comment.num_comments * ((FLAC__STREAM_METADATA_VORBIS_COMMENT_ENTRY_LENGTH_LEN / 8) + sizeof(dummydata));
	if(block->length != expected_length) {
		printf("FAILED, bad length, expected %u, got %u\n", expected_length, block->length);
		return 1;
	}
	if(block->data.vorbis_comment.num_comments != 1u) {
		printf("FAILED, bad num_comments, expected %u, got %u\n", 1u, block->data.vorbis_comment.num_comments);
		return 1;
	}
	if(!compare_vorbiscomment_entry_array_(block->data.vorbis_comment.comments, vorbiscomment_entry_array, 1u))
		return 1;
	printf("OK\n");

	printf("testing FLAC__metadata_object_copy()... ");
	blockcopy = FLAC__metadata_object_copy(block);
	if(0 == blockcopy) {
		printf("FAILED, returned NULL\n");
		return 1;
	}
	if(!compare_block_(block, blockcopy))
		return 1;
	printf("OK\n");

	printf("testing FLAC__metadata_object_delete()... ");
	FLAC__metadata_object_delete(blockcopy);
	printf("OK\n");

	printf("testing FLAC__metadata_object_vorbiscomment_set_comments(own)... ");
	if(!FLAC__metadata_object_vorbiscomment_set_comments(block, vorbiscomment_entry_array, 1u, false/*own*/)) {
		printf("FAILED, returned false\n");
		return 1;
	}
	expected_length = ((FLAC__STREAM_METADATA_VORBIS_COMMENT_ENTRY_LENGTH_LEN / 8) + sizeof(dummydata)) + (FLAC__STREAM_METADATA_VORBIS_COMMENT_NUM_COMMENTS_LEN / 8) + block->data.vorbis_comment.num_comments * ((FLAC__STREAM_METADATA_VORBIS_COMMENT_ENTRY_LENGTH_LEN / 8) + sizeof(dummydata));
	if(block->length != expected_length) {
		printf("FAILED, bad length, expected %u, got %u\n", expected_length, block->length);
		return 1;
	}
	printf("OK\n");

	printf("testing FLAC__metadata_object_copy()... ");
	blockcopy = FLAC__metadata_object_copy(block);
	if(0 == blockcopy) {
		printf("FAILED, returned NULL\n");
		return 1;
	}
	if(!compare_block_(block, blockcopy))
		return 1;
	printf("OK\n");

	printf("testing FLAC__metadata_object_delete()... ");
	FLAC__metadata_object_delete(blockcopy);
	FLAC__metadata_object_delete(block);
	printf("OK\n");


	printf("\nPASSED!\n");
	return 0;
}
