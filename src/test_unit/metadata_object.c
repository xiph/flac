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

#include "FLAC/assert.h"
#include "FLAC/metadata.h"
#include "metadata_utils.h"
#include <stdio.h>
#include <stdlib.h> /* for malloc() */
#include <string.h> /* for memcmp() */

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

static FLAC__bool compare_seekpoint_array_(const FLAC__StreamMetaData_SeekPoint *from, const FLAC__StreamMetaData_SeekPoint *to, unsigned n)
{
	unsigned i;

	FLAC__ASSERT(0 != from);
	FLAC__ASSERT(0 != to);

	for(i = 0; i < n; i++) {
		if(from[i].sample_number != to[i].sample_number) {
			printf("FAILED, point[%u].sample_number mismatch, expected %llu, got %llu\n", i, to[i].sample_number, from[i].sample_number);
			return false;
		}
		if(from[i].stream_offset != to[i].stream_offset) {
			printf("FAILED, point[%u].stream_offset mismatch, expected %llu, got %llu\n", i, to[i].stream_offset, from[i].stream_offset);
			return false;
		}
		if(from[i].frame_samples != to[i].frame_samples) {
			printf("FAILED, point[%u].frame_samples mismatch, expected %u, got %u\n", i, to[i].frame_samples, from[i].frame_samples);
			return false;
		}
	}

	return true;
}

int test_metadata_object()
{
	FLAC__StreamMetaData *block, *blockcopy;
	FLAC__StreamMetaData_SeekPoint seekpoint_array[3];
	FLAC__StreamMetaData_VorbisComment_Entry *vorbiscomment_entry_array, *vorbiscomment_entry_array_copy;
	unsigned expected_length, seekpoints;
	static FLAC__byte dummydata[4] = { 'a', 'b', 'c', 'd' };

	printf("\n+++ unit test: metadata objects (libFLAC)\n\n");


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

	seekpoint_array[0].sample_number = FLAC__STREAM_METADATA_SEEKPOINT_PLACEHOLDER;
	seekpoint_array[0].stream_offset = 0;
	seekpoint_array[0].frame_samples = 0;
	seekpoint_array[1].sample_number = FLAC__STREAM_METADATA_SEEKPOINT_PLACEHOLDER;
	seekpoint_array[1].stream_offset = 0;
	seekpoint_array[1].frame_samples = 0;

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

	seekpoints = 2;
	printf("testing FLAC__metadata_object_seektable_resize_points(grow to %u)...", seekpoints);
	if(!FLAC__metadata_object_seektable_resize_points(block, seekpoints)) {
		printf("FAILED, returned false\n");
		return 1;
	}
	expected_length = seekpoints * FLAC__STREAM_METADATA_SEEKPOINT_LENGTH;
	if(block->length != expected_length) {
		printf("FAILED, bad length, expected %u, got %u\n", expected_length, block->length);
		return 1;
    }
	if(block->data.seek_table.num_points != seekpoints) {
		printf("FAILED, expected %u points, got %u\n", seekpoints, block->data.seek_table.num_points);
		return 1;
	}
	if(!compare_seekpoint_array_(block->data.seek_table.points, seekpoint_array, seekpoints))
		return 1;
	printf("OK\n");

	seekpoints = 1;
	printf("testing FLAC__metadata_object_seektable_resize_points(shrink to %u)...", seekpoints);
	if(!FLAC__metadata_object_seektable_resize_points(block, seekpoints)) {
		printf("FAILED, returned false\n");
		return 1;
	}
	expected_length = seekpoints * FLAC__STREAM_METADATA_SEEKPOINT_LENGTH;
	if(block->length != expected_length) {
		printf("FAILED, bad length, expected %u, got %u\n", expected_length, block->length);
		return 1;
    }
	if(block->data.seek_table.num_points != seekpoints) {
		printf("FAILED, expected %u point, got %u\n", seekpoints, block->data.seek_table.num_points);
		return 1;
	}
	if(!compare_seekpoint_array_(block->data.seek_table.points, seekpoint_array, seekpoints))
		return 1;
	printf("OK\n");

	seekpoints = 0;
	printf("testing FLAC__metadata_object_seektable_resize_points(shrink to %u)...", seekpoints);
	if(!FLAC__metadata_object_seektable_resize_points(block, seekpoints)) {
		printf("FAILED, returned false\n");
		return 1;
	}
	expected_length = seekpoints * FLAC__STREAM_METADATA_SEEKPOINT_LENGTH;
	if(block->length != expected_length) {
		printf("FAILED, bad length, expected %u, got %u\n", expected_length, block->length);
		return 1;
    }
	if(block->data.seek_table.num_points != seekpoints) {
		printf("FAILED, expected %u points, got %u\n", seekpoints, block->data.seek_table.num_points);
		return 1;
	}
	if(0 != block->data.seek_table.points) {
		printf("FAILED, 'points' pointer is not null\n");
		return 1;
	}
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

#if 0
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
#endif
	FLAC__metadata_object_delete(block);
	printf("OK\n");


	return 0;
}

#if 0
int test_metadata_object_pp()
{
	FLAC::Metadata::Prototype *block, *blockcopy;
	FLAC__StreamMetaData_SeekPoint *seekpoint_array, *seekpoint_array_copy;
	FLAC__StreamMetaData_VorbisComment_Entry *vorbiscomment_entry_array, *vorbiscomment_entry_array_copy;
	unsigned expected_length;
	static FLAC__byte dummydata[4] = { 'a', 'b', 'c', 'd' };

	printf("\n+++ unit test: metadata objects (libFLAC++)\n\n");


	printf("testing STREAMINFO\n");

	printf("testing FLAC::Metadata::StreamInfo::StreamInfo()... ");
	block = new FLAC::Metadata::StreamInfo();
	if(0 == block) {
		printf("FAILED, new returned NULL\n");
		return 1;
	}
	if(!block->is_valid()) {
		printf("FAILED, !block->is_valid()\n");
		return 1;
	}
	if(block->is_valid() != *block) {
		printf("FAILED, FLAC::Metadata::Prototype::operator bool() is broken\n");
		return 1;
	}
	expected_length = FLAC__STREAM_METADATA_STREAMINFO_LENGTH;
	if(block->length() != expected_length) {
		printf("FAILED, bad length, expected %u, got %u\n", expected_length, block->length());
		return 1;
    }
	printf("OK\n");

	printf("testing FLAC__metadata_object_copy()... ");
	blockcopy = new FLAC::Metadata::StreamInfo(block);
	if(0 == blockcopy) {
		printf("FAILED, new returned NULL\n");
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


	return 0;
}
#endif
