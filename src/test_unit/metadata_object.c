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

#include "FLAC/assert.h"
#include "FLAC/metadata.h"
#include "metadata_utils.h"
#include <stdio.h>
#include <stdlib.h> /* for malloc() */
#include <string.h> /* for memcmp() */

static FLAC__byte *make_dummydata_(FLAC__byte *dummydata, unsigned len)
{
	FLAC__byte *ret;

	if(0 == (ret = (FLAC__byte*)malloc(len))) {
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

static FLAC__bool check_seektable_(const FLAC__StreamMetaData *block, unsigned num_points, const FLAC__StreamMetaData_SeekPoint *array)
{
	const unsigned expected_length = num_points * FLAC__STREAM_METADATA_SEEKPOINT_LENGTH;

	if(block->length != expected_length) {
		printf("FAILED, bad length, expected %u, got %u\n", expected_length, block->length);
		return false;
    }
	if(block->data.seek_table.num_points != num_points) {
		printf("FAILED, expected %u point, got %u\n", num_points, block->data.seek_table.num_points);
		return false;
	}
	if(0 == array) {
		if(0 != block->data.seek_table.points) {
			printf("FAILED, 'points' pointer is not null\n");
			return false;
		}
	}
	else {
		if(!compare_seekpoint_array_(block->data.seek_table.points, array, num_points))
			return false;
	}
	printf("OK\n");

	return true;
}

static void entry_new_(FLAC__StreamMetaData_VorbisComment_Entry *entry, const char *field)
{
	entry->length = strlen(field);
	entry->entry = (FLAC__byte*)malloc(entry->length);
	FLAC__ASSERT(0 != entry->entry);	
	memcpy(entry->entry, field, entry->length);
}

static void entry_clone_(FLAC__StreamMetaData_VorbisComment_Entry *entry)
{
	FLAC__byte *x = (FLAC__byte*)malloc(entry->length);
	FLAC__ASSERT(0 != x);	
	memcpy(x, entry->entry, entry->length);
	entry->entry = x;
}

static void vc_calc_len_(FLAC__StreamMetaData *block)
{
	const FLAC__StreamMetaData_VorbisComment *vc = &block->data.vorbis_comment;
	unsigned i;

	block->length = FLAC__STREAM_METADATA_VORBIS_COMMENT_ENTRY_LENGTH_LEN / 8;
	block->length += vc->vendor_string.length;
	block->length += FLAC__STREAM_METADATA_VORBIS_COMMENT_NUM_COMMENTS_LEN / 8;
	for(i = 0; i < vc->num_comments; i++) {
		block->length += FLAC__STREAM_METADATA_VORBIS_COMMENT_ENTRY_LENGTH_LEN / 8;
		block->length += vc->comments[i].length;
	}
}

static void vc_resize_(FLAC__StreamMetaData *block, unsigned num)
{
	FLAC__StreamMetaData_VorbisComment *vc = &block->data.vorbis_comment;

	if(vc->num_comments != 0) {
		FLAC__ASSERT(0 != vc->comments);
		if(num < vc->num_comments) {
			unsigned i;
			for(i = num; i < vc->num_comments; i++) {
				if(0 != vc->comments[i].entry)
					free(vc->comments[i].entry);
			}
		}
	}
	if(num == 0) {
		if(0 != vc->comments) {
			free(vc->comments);
			vc->comments = 0;
		}
	}
	else {
		vc->comments = (FLAC__StreamMetaData_VorbisComment_Entry*)realloc(vc->comments, sizeof(FLAC__StreamMetaData_VorbisComment_Entry)*num);
		FLAC__ASSERT(0 != vc->comments);
		if(num > vc->num_comments)
			memset(vc->comments+vc->num_comments, 0, sizeof(FLAC__StreamMetaData_VorbisComment_Entry)*(num-vc->num_comments));
	}

	vc->num_comments = num;
	vc_calc_len_(block);
}

static void vc_set_vs_new_(FLAC__StreamMetaData_VorbisComment_Entry *entry, FLAC__StreamMetaData *block, const char *field)
{
	entry_new_(entry, field);
	block->data.vorbis_comment.vendor_string = *entry;
	vc_calc_len_(block);
}

static void vc_set_new_(FLAC__StreamMetaData_VorbisComment_Entry *entry, FLAC__StreamMetaData *block, unsigned pos, const char *field)
{
	entry_new_(entry, field);
	block->data.vorbis_comment.comments[pos] = *entry;
	vc_calc_len_(block);
}

static void vc_insert_new_(FLAC__StreamMetaData_VorbisComment_Entry *entry, FLAC__StreamMetaData *block, unsigned pos, const char *field)
{
	vc_resize_(block, block->data.vorbis_comment.num_comments+1);
	memmove(&block->data.vorbis_comment.comments[pos+1], &block->data.vorbis_comment.comments[pos], sizeof(FLAC__StreamMetaData_VorbisComment_Entry)*(block->data.vorbis_comment.num_comments-1-pos));
	vc_set_new_(entry, block, pos, field);
	vc_calc_len_(block);
}

static void vc_delete_(FLAC__StreamMetaData *block, unsigned pos)
{
	if(0 != block->data.vorbis_comment.comments[pos].entry)
		free(block->data.vorbis_comment.comments[pos].entry);
	memmove(&block->data.vorbis_comment.comments[pos], &block->data.vorbis_comment.comments[pos+1], sizeof(FLAC__StreamMetaData_VorbisComment_Entry)*(block->data.vorbis_comment.num_comments-pos-1));
	block->data.vorbis_comment.comments[block->data.vorbis_comment.num_comments-1].entry = 0;
	block->data.vorbis_comment.comments[block->data.vorbis_comment.num_comments-1].length = 0;
	vc_resize_(block, block->data.vorbis_comment.num_comments-1);
	vc_calc_len_(block);
}

FLAC__bool test_metadata_object()
{
	FLAC__StreamMetaData *block, *blockcopy, *vorbiscomment;
	FLAC__StreamMetaData_SeekPoint seekpoint_array[4];
	FLAC__StreamMetaData_VorbisComment_Entry entry;
	unsigned i, expected_length, seekpoints;
	static FLAC__byte dummydata[4] = { 'a', 'b', 'c', 'd' };

	printf("\n+++ unit test: metadata objects (libFLAC)\n\n");


	printf("testing STREAMINFO\n");

	printf("testing FLAC__metadata_object_new()... ");
	block = FLAC__metadata_object_new(FLAC__METADATA_TYPE_STREAMINFO);
	if(0 == block) {
		printf("FAILED, returned NULL\n");
		return false;
	}
	expected_length = FLAC__STREAM_METADATA_STREAMINFO_LENGTH;
	if(block->length != expected_length) {
		printf("FAILED, bad length, expected %u, got %u\n", expected_length, block->length);
		return false;
    }
	printf("OK\n");

	printf("testing FLAC__metadata_object_copy()... ");
	blockcopy = FLAC__metadata_object_copy(block);
	if(0 == blockcopy) {
		printf("FAILED, returned NULL\n");
		return false;
	}
	if(!compare_block_(block, blockcopy))
		return false;
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
		return false;
	}
	expected_length = 0;
	if(block->length != expected_length) {
		printf("FAILED, bad length, expected %u, got %u\n", expected_length, block->length);
		return false;
    }
	printf("OK\n");

	printf("testing FLAC__metadata_object_copy()... ");
	blockcopy = FLAC__metadata_object_copy(block);
	if(0 == blockcopy) {
		printf("FAILED, returned NULL\n");
		return false;
	}
	if(!compare_block_(block, blockcopy))
		return false;
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
		return false;
	}
	expected_length = FLAC__STREAM_METADATA_APPLICATION_ID_LEN / 8;
	if(block->length != expected_length) {
		printf("FAILED, bad length, expected %u, got %u\n", expected_length, block->length);
		return false;
    }
	printf("OK\n");

	printf("testing FLAC__metadata_object_copy()... ");
	blockcopy = FLAC__metadata_object_copy(block);
	if(0 == blockcopy) {
		printf("FAILED, returned NULL\n");
		return false;
	}
	if(!compare_block_(block, blockcopy))
		return false;
	printf("OK\n");

	printf("testing FLAC__metadata_object_delete()... ");
	FLAC__metadata_object_delete(blockcopy);
	printf("OK\n");

	printf("testing FLAC__metadata_object_application_set_data(copy)... ");
	if(!FLAC__metadata_object_application_set_data(block, dummydata, sizeof(dummydata), true/*copy*/)) {
		printf("FAILED, returned false\n");
		return false;
	}
	expected_length = (FLAC__STREAM_METADATA_APPLICATION_ID_LEN / 8) + sizeof(dummydata);
	if(block->length != expected_length) {
		printf("FAILED, bad length, expected %u, got %u\n", expected_length, block->length);
		return false;
	}
	if(0 != memcmp(block->data.application.data, dummydata, sizeof(dummydata))) {
		printf("FAILED, data mismatch\n");
		return false;
	}
	printf("OK\n");

	printf("testing FLAC__metadata_object_copy()... ");
	blockcopy = FLAC__metadata_object_copy(block);
	if(0 == blockcopy) {
		printf("FAILED, returned NULL\n");
		return false;
	}
	if(!compare_block_(block, blockcopy))
		return false;
	printf("OK\n");

	printf("testing FLAC__metadata_object_delete()... ");
	FLAC__metadata_object_delete(blockcopy);
	printf("OK\n");

	printf("testing FLAC__metadata_object_application_set_data(own)... ");
	if(!FLAC__metadata_object_application_set_data(block, make_dummydata_(dummydata, sizeof(dummydata)), sizeof(dummydata), false/*own*/)) {
		printf("FAILED, returned false\n");
		return false;
	}
	expected_length = (FLAC__STREAM_METADATA_APPLICATION_ID_LEN / 8) + sizeof(dummydata);
	if(block->length != expected_length) {
		printf("FAILED, bad length, expected %u, got %u\n", expected_length, block->length);
		return false;
	}
	if(0 != memcmp(block->data.application.data, dummydata, sizeof(dummydata))) {
		printf("FAILED, data mismatch\n");
		return false;
	}
	printf("OK\n");

	printf("testing FLAC__metadata_object_copy()... ");
	blockcopy = FLAC__metadata_object_copy(block);
	if(0 == blockcopy) {
		printf("FAILED, returned NULL\n");
		return false;
	}
	if(!compare_block_(block, blockcopy))
		return false;
	printf("OK\n");

	printf("testing FLAC__metadata_object_delete()... ");
	FLAC__metadata_object_delete(blockcopy);
	FLAC__metadata_object_delete(block);
	printf("OK\n");


	printf("testing SEEKTABLE\n");

	for(i = 0; i < 4; i++) {
		seekpoint_array[i].sample_number = FLAC__STREAM_METADATA_SEEKPOINT_PLACEHOLDER;
		seekpoint_array[i].stream_offset = 0;
		seekpoint_array[i].frame_samples = 0;
	}

	seekpoints = 0;
	printf("testing FLAC__metadata_object_new()... ");
	block = FLAC__metadata_object_new(FLAC__METADATA_TYPE_SEEKTABLE);
	if(0 == block) {
		printf("FAILED, returned NULL\n");
		return false;
	}
	if(!check_seektable_(block, seekpoints, 0))
		return false;

	printf("testing FLAC__metadata_object_copy()... ");
	blockcopy = FLAC__metadata_object_copy(block);
	if(0 == blockcopy) {
		printf("FAILED, returned NULL\n");
		return false;
	}
	if(!compare_block_(block, blockcopy))
		return false;
	printf("OK\n");

	printf("testing FLAC__metadata_object_delete()... ");
	FLAC__metadata_object_delete(blockcopy);
	printf("OK\n");

	seekpoints = 2;
	printf("testing FLAC__metadata_object_seektable_resize_points(grow to %u)...", seekpoints);
	if(!FLAC__metadata_object_seektable_resize_points(block, seekpoints)) {
		printf("FAILED, returned false\n");
		return false;
	}
	if(!check_seektable_(block, seekpoints, seekpoint_array))
		return false;

	seekpoints = 1;
	printf("testing FLAC__metadata_object_seektable_resize_points(shrink to %u)...", seekpoints);
	if(!FLAC__metadata_object_seektable_resize_points(block, seekpoints)) {
		printf("FAILED, returned false\n");
		return false;
	}
	if(!check_seektable_(block, seekpoints, seekpoint_array))
		return false;

	printf("testing FLAC__metadata_object_seektable_is_legal()...");
	if(!FLAC__metadata_object_seektable_is_legal(block)) {
		printf("FAILED, returned false\n");
		return false;
	}
	printf("OK\n");

	seekpoints = 0;
	printf("testing FLAC__metadata_object_seektable_resize_points(shrink to %u)...", seekpoints);
	if(!FLAC__metadata_object_seektable_resize_points(block, seekpoints)) {
		printf("FAILED, returned false\n");
		return false;
	}
	if(!check_seektable_(block, seekpoints, 0))
		return false;

	seekpoints++;
	printf("testing FLAC__metadata_object_seektable_insert_point() on empty array...");
	if(!FLAC__metadata_object_seektable_insert_point(block, 0, seekpoint_array[0])) {
		printf("FAILED, returned false\n");
		return false;
	}
	if(!check_seektable_(block, seekpoints, seekpoint_array))
		return false;

	seekpoint_array[0].sample_number = 1;
	seekpoints++;
	printf("testing FLAC__metadata_object_seektable_insert_point() on beginning of non-empty array...");
	if(!FLAC__metadata_object_seektable_insert_point(block, 0, seekpoint_array[0])) {
		printf("FAILED, returned false\n");
		return false;
	}
	if(!check_seektable_(block, seekpoints, seekpoint_array))
		return false;

	seekpoint_array[1].sample_number = 2;
	seekpoints++;
	printf("testing FLAC__metadata_object_seektable_insert_point() on middle of non-empty array...");
	if(!FLAC__metadata_object_seektable_insert_point(block, 1, seekpoint_array[1])) {
		printf("FAILED, returned false\n");
		return false;
	}
	if(!check_seektable_(block, seekpoints, seekpoint_array))
		return false;

	seekpoint_array[3].sample_number = 3;
	seekpoints++;
	printf("testing FLAC__metadata_object_seektable_insert_point() on end of non-empty array...");
	if(!FLAC__metadata_object_seektable_insert_point(block, 3, seekpoint_array[3])) {
		printf("FAILED, returned false\n");
		return false;
	}
	if(!check_seektable_(block, seekpoints, seekpoint_array))
		return false;

	printf("testing FLAC__metadata_object_copy()... ");
	blockcopy = FLAC__metadata_object_copy(block);
	if(0 == blockcopy) {
		printf("FAILED, returned NULL\n");
		return false;
	}
	if(!compare_block_(block, blockcopy))
		return false;
	printf("OK\n");

	printf("testing FLAC__metadata_object_delete()... ");
	FLAC__metadata_object_delete(blockcopy);
	printf("OK\n");

	seekpoint_array[2].sample_number = seekpoint_array[3].sample_number;
	seekpoints--;
	printf("testing FLAC__metadata_object_seektable_delete_point() on middle of array...");
	if(!FLAC__metadata_object_seektable_delete_point(block, 2)) {
		printf("FAILED, returned false\n");
		return false;
	}
	if(!check_seektable_(block, seekpoints, seekpoint_array))
		return false;

	seekpoints--;
	printf("testing FLAC__metadata_object_seektable_delete_point() on end of array...");
	if(!FLAC__metadata_object_seektable_delete_point(block, 2)) {
		printf("FAILED, returned false\n");
		return false;
	}
	if(!check_seektable_(block, seekpoints, seekpoint_array))
		return false;

	seekpoints--;
	printf("testing FLAC__metadata_object_seektable_delete_point() on beginning of array...");
	if(!FLAC__metadata_object_seektable_delete_point(block, 0)) {
		printf("FAILED, returned false\n");
		return false;
	}
	if(!check_seektable_(block, seekpoints, seekpoint_array+1))
		return false;

	printf("testing FLAC__metadata_object_seektable_set_point()...");
	FLAC__metadata_object_seektable_set_point(block, 0, seekpoint_array[0]);
	if(!check_seektable_(block, seekpoints, seekpoint_array))
		return false;



	printf("testing VORBIS_COMMENT\n");

	printf("testing FLAC__metadata_object_new()... ");
	block = FLAC__metadata_object_new(FLAC__METADATA_TYPE_VORBIS_COMMENT);
	if(0 == block) {
		printf("FAILED, returned NULL\n");
		return false;
	}
	expected_length = (FLAC__STREAM_METADATA_VORBIS_COMMENT_ENTRY_LENGTH_LEN + FLAC__STREAM_METADATA_VORBIS_COMMENT_NUM_COMMENTS_LEN) / 8;
	if(block->length != expected_length) {
		printf("FAILED, bad length, expected %u, got %u\n", expected_length, block->length);
		return false;
    }
	printf("OK\n");

	printf("testing FLAC__metadata_object_copy()... ");
	vorbiscomment = FLAC__metadata_object_copy(block);
	if(0 == vorbiscomment) {
		printf("FAILED, returned NULL\n");
		return false;
	}
	if(!compare_block_(vorbiscomment, block))
		return false;
	printf("OK\n");

	vc_resize_(vorbiscomment, 2);
	printf("testing FLAC__metadata_object_vorbiscomment_resize_comments(grow to %u)...", vorbiscomment->data.vorbis_comment.num_comments);
	if(!FLAC__metadata_object_vorbiscomment_resize_comments(block, vorbiscomment->data.vorbis_comment.num_comments)) {
		printf("FAILED, returned false\n");
		return false;
	}
	if(!compare_block_(vorbiscomment, block))
		return false;
	printf("OK\n");

	vc_resize_(vorbiscomment, 1);
	printf("testing FLAC__metadata_object_vorbiscomment_resize_comments(shrink to %u)...", vorbiscomment->data.vorbis_comment.num_comments);
	if(!FLAC__metadata_object_vorbiscomment_resize_comments(block, vorbiscomment->data.vorbis_comment.num_comments)) {
		printf("FAILED, returned false\n");
		return false;
	}
	if(!compare_block_(vorbiscomment, block))
		return false;
	printf("OK\n");

	vc_resize_(vorbiscomment, 0);
	printf("testing FLAC__metadata_object_vorbiscomment_resize_comments(shrink to %u)...", vorbiscomment->data.vorbis_comment.num_comments);
	if(!FLAC__metadata_object_vorbiscomment_resize_comments(block, vorbiscomment->data.vorbis_comment.num_comments)) {
		printf("FAILED, returned false\n");
		return false;
	}
	if(!compare_block_(vorbiscomment, block))
		return false;
	printf("OK\n");

	printf("testing FLAC__metadata_object_vorbiscomment_insert_comment(copy) on empty array...");
	vc_insert_new_(&entry, vorbiscomment, 0, "name1=field1");
	if(!FLAC__metadata_object_vorbiscomment_insert_comment(block, 0, entry, /*copy=*/true)) {
		printf("FAILED, returned false\n");
		return false;
	}
	if(!compare_block_(vorbiscomment, block))
		return false;
	printf("OK\n");

	printf("testing FLAC__metadata_object_vorbiscomment_insert_comment(copy) on beginning of non-empty array...");
	vc_insert_new_(&entry, vorbiscomment, 0, "name2=field2");
	if(!FLAC__metadata_object_vorbiscomment_insert_comment(block, 0, entry, /*copy=*/true)) {
		printf("FAILED, returned false\n");
		return false;
	}
	if(!compare_block_(vorbiscomment, block))
		return false;
	printf("OK\n");

	printf("testing FLAC__metadata_object_vorbiscomment_insert_comment(copy) on middle of non-empty array...");
	vc_insert_new_(&entry, vorbiscomment, 1, "name3=field3");
	if(!FLAC__metadata_object_vorbiscomment_insert_comment(block, 1, entry, /*copy=*/true)) {
		printf("FAILED, returned false\n");
		return false;
	}
	if(!compare_block_(vorbiscomment, block))
		return false;
	printf("OK\n");

	printf("testing FLAC__metadata_object_vorbiscomment_insert_comment(copy) on end of non-empty array...");
	vc_insert_new_(&entry, vorbiscomment, 3, "name4=field4");
	if(!FLAC__metadata_object_vorbiscomment_insert_comment(block, 3, entry, /*copy=*/true)) {
		printf("FAILED, returned false\n");
		return false;
	}
	if(!compare_block_(vorbiscomment, block))
		return false;
	printf("OK\n");

	printf("testing FLAC__metadata_object_copy()... ");
	blockcopy = FLAC__metadata_object_copy(block);
	if(0 == blockcopy) {
		printf("FAILED, returned NULL\n");
		return false;
	}
	if(!compare_block_(block, blockcopy))
		return false;
	printf("OK\n");

	printf("testing FLAC__metadata_object_delete()... ");
	FLAC__metadata_object_delete(blockcopy);
	printf("OK\n");

	printf("testing FLAC__metadata_object_vorbiscomment_delete_comment() on middle of array...");
	vc_delete_(vorbiscomment, 2);
	if(!FLAC__metadata_object_vorbiscomment_delete_comment(block, 2)) {
		printf("FAILED, returned false\n");
		return false;
	}
	if(!compare_block_(vorbiscomment, block))
		return false;
	printf("OK\n");

	printf("testing FLAC__metadata_object_vorbiscomment_delete_comment() on end of array...");
	vc_delete_(vorbiscomment, 2);
	if(!FLAC__metadata_object_vorbiscomment_delete_comment(block, 2)) {
		printf("FAILED, returned false\n");
		return false;
	}
	if(!compare_block_(vorbiscomment, block))
		return false;
	printf("OK\n");

	printf("testing FLAC__metadata_object_vorbiscomment_delete_comment() on beginning of array...");
	vc_delete_(vorbiscomment, 0);
	if(!FLAC__metadata_object_vorbiscomment_delete_comment(block, 0)) {
		printf("FAILED, returned false\n");
		return false;
	}
	if(!compare_block_(vorbiscomment, block))
		return false;
	printf("OK\n");

	printf("testing FLAC__metadata_object_vorbiscomment_set_comment(copy)...");
	vc_set_new_(&entry, vorbiscomment, 0, "name5=field5");
	FLAC__metadata_object_vorbiscomment_set_comment(block, 0, entry, /*copy=*/true);
	if(!compare_block_(vorbiscomment, block))
		return false;
	printf("OK\n");

	printf("testing FLAC__metadata_object_vorbiscomment_set_vendor_string(copy)...");
	vc_set_vs_new_(&entry, vorbiscomment, "name6=field6");
	FLAC__metadata_object_vorbiscomment_set_vendor_string(block, entry, /*copy=*/true);
	if(!compare_block_(vorbiscomment, block))
		return false;
	printf("OK\n");

	printf("testing FLAC__metadata_object_delete()... ");
	FLAC__metadata_object_delete(vorbiscomment);
	FLAC__metadata_object_delete(block);
	printf("OK\n");


	printf("testing FLAC__metadata_object_new()... ");
	block = FLAC__metadata_object_new(FLAC__METADATA_TYPE_VORBIS_COMMENT);
	if(0 == block) {
		printf("FAILED, returned NULL\n");
		return false;
	}
	printf("OK\n");

	printf("testing FLAC__metadata_object_copy()... ");
	vorbiscomment = FLAC__metadata_object_copy(block);
	if(0 == vorbiscomment) {
		printf("FAILED, returned NULL\n");
		return false;
	}
	if(!compare_block_(vorbiscomment, block))
		return false;
	printf("OK\n");

	printf("testing FLAC__metadata_object_vorbiscomment_insert_comment(own) on empty array...");
	vc_insert_new_(&entry, vorbiscomment, 0, "name1=field1");
	entry_clone_(&entry);
	if(!FLAC__metadata_object_vorbiscomment_insert_comment(block, 0, entry, /*copy=*/false)) {
		printf("FAILED, returned false\n");
		return false;
	}
	if(!compare_block_(vorbiscomment, block))
		return false;
	printf("OK\n");

	printf("testing FLAC__metadata_object_vorbiscomment_insert_comment(own) on beginning of non-empty array...");
	vc_insert_new_(&entry, vorbiscomment, 0, "name2=field2");
	entry_clone_(&entry);
	if(!FLAC__metadata_object_vorbiscomment_insert_comment(block, 0, entry, /*copy=*/false)) {
		printf("FAILED, returned false\n");
		return false;
	}
	if(!compare_block_(vorbiscomment, block))
		return false;
	printf("OK\n");

	printf("testing FLAC__metadata_object_vorbiscomment_insert_comment(own) on middle of non-empty array...");
	vc_insert_new_(&entry, vorbiscomment, 1, "name3=field3");
	entry_clone_(&entry);
	if(!FLAC__metadata_object_vorbiscomment_insert_comment(block, 1, entry, /*copy=*/false)) {
		printf("FAILED, returned false\n");
		return false;
	}
	if(!compare_block_(vorbiscomment, block))
		return false;
	printf("OK\n");

	printf("testing FLAC__metadata_object_vorbiscomment_insert_comment(own) on end of non-empty array...");
	vc_insert_new_(&entry, vorbiscomment, 3, "name4=field4");
	entry_clone_(&entry);
	if(!FLAC__metadata_object_vorbiscomment_insert_comment(block, 3, entry, /*copy=*/false)) {
		printf("FAILED, returned false\n");
		return false;
	}
	if(!compare_block_(vorbiscomment, block))
		return false;
	printf("OK\n");

	printf("testing FLAC__metadata_object_vorbiscomment_delete_comment() on middle of array...");
	vc_delete_(vorbiscomment, 2);
	if(!FLAC__metadata_object_vorbiscomment_delete_comment(block, 2)) {
		printf("FAILED, returned false\n");
		return false;
	}
	if(!compare_block_(vorbiscomment, block))
		return false;
	printf("OK\n");

	printf("testing FLAC__metadata_object_vorbiscomment_delete_comment() on end of array...");
	vc_delete_(vorbiscomment, 2);
	if(!FLAC__metadata_object_vorbiscomment_delete_comment(block, 2)) {
		printf("FAILED, returned false\n");
		return false;
	}
	if(!compare_block_(vorbiscomment, block))
		return false;
	printf("OK\n");

	printf("testing FLAC__metadata_object_vorbiscomment_delete_comment() on beginning of array...");
	vc_delete_(vorbiscomment, 0);
	if(!FLAC__metadata_object_vorbiscomment_delete_comment(block, 0)) {
		printf("FAILED, returned false\n");
		return false;
	}
	if(!compare_block_(vorbiscomment, block))
		return false;
	printf("OK\n");

	printf("testing FLAC__metadata_object_vorbiscomment_set_comment(own)...");
	vc_set_new_(&entry, vorbiscomment, 0, "name5=field5");
	entry_clone_(&entry);
	FLAC__metadata_object_vorbiscomment_set_comment(block, 0, entry, /*copy=*/false);
	if(!compare_block_(vorbiscomment, block))
		return false;
	printf("OK\n");

	printf("testing FLAC__metadata_object_vorbiscomment_set_vendor_string(own)...");
	vc_set_vs_new_(&entry, vorbiscomment, "name6=field6");
	entry_clone_(&entry);
	FLAC__metadata_object_vorbiscomment_set_vendor_string(block, entry, /*copy=*/false);
	if(!compare_block_(vorbiscomment, block))
		return false;
	printf("OK\n");

	printf("testing FLAC__metadata_object_delete()... ");
	FLAC__metadata_object_delete(vorbiscomment);
	FLAC__metadata_object_delete(block);
	printf("OK\n");


	return true;
}
