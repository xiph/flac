/* libFLAC - Free Lossless Audio Codec library
 * Copyright (C) 2001,2002  Josh Coalson
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

#include <stdlib.h>
#include <string.h>

#include "private/metadata.h"

#include "FLAC/assert.h"


/****************************************************************************
 *
 * Local routines
 *
 ***************************************************************************/

static FLAC__bool copy_bytes_(FLAC__byte **to, const FLAC__byte *from, unsigned bytes)
{
	if(bytes > 0 && 0 != from) {
		FLAC__byte *x;
		if(0 == (x = malloc(bytes)))
			return false;
		memcpy(x, from, bytes);
		*to = x;
	}
	else {
		FLAC__ASSERT(0 == from);
		FLAC__ASSERT(bytes == 0);
		*to = 0;
	}
	return true;
}

static FLAC__bool copy_vcentry_(FLAC__StreamMetadata_VorbisComment_Entry *to, const FLAC__StreamMetadata_VorbisComment_Entry *from)
{
	to->length = from->length;
	if(0 == from->entry) {
		FLAC__ASSERT(from->length == 0);
		to->entry = 0;
	}
	else {
		FLAC__byte *x;
		FLAC__ASSERT(from->length > 0);
		if(0 == (x = malloc(from->length)))
			return false;
		memcpy(x, from->entry, from->length);
		to->entry = x;
	}
	return true;
}

static void seektable_calculate_length_(FLAC__StreamMetadata *object)
{
	FLAC__ASSERT(0 != object);
	FLAC__ASSERT(object->type == FLAC__METADATA_TYPE_SEEKTABLE);

	object->length = object->data.seek_table.num_points * FLAC__STREAM_METADATA_SEEKPOINT_LENGTH;
}

static FLAC__StreamMetadata_SeekPoint *seekpoint_array_new_(unsigned num_points)
{
	FLAC__StreamMetadata_SeekPoint *object_array;

	FLAC__ASSERT(num_points > 0);

	object_array = malloc(num_points * sizeof(FLAC__StreamMetadata_SeekPoint));

	if(0 != object_array) {
		unsigned i;
		for(i = 0; i < num_points; i++) {
			object_array[i].sample_number = FLAC__STREAM_METADATA_SEEKPOINT_PLACEHOLDER;
			object_array[i].stream_offset = 0;
			object_array[i].frame_samples = 0;
		}
	}

	return object_array;
}

static void vorbiscomment_calculate_length_(FLAC__StreamMetadata *object)
{
	unsigned i;

	FLAC__ASSERT(object->type == FLAC__METADATA_TYPE_VORBIS_COMMENT);

	object->length = (FLAC__STREAM_METADATA_VORBIS_COMMENT_ENTRY_LENGTH_LEN) / 8;
	object->length += object->data.vorbis_comment.vendor_string.length;
	object->length += (FLAC__STREAM_METADATA_VORBIS_COMMENT_NUM_COMMENTS_LEN) / 8;
	for(i = 0; i < object->data.vorbis_comment.num_comments; i++) {
		object->length += (FLAC__STREAM_METADATA_VORBIS_COMMENT_ENTRY_LENGTH_LEN / 8);
		object->length += object->data.vorbis_comment.comments[i].length;
	}
}

static FLAC__StreamMetadata_VorbisComment_Entry *vorbiscomment_entry_array_new_(unsigned num_comments)
{
	FLAC__ASSERT(num_comments > 0);

	return calloc(num_comments, sizeof(FLAC__StreamMetadata_VorbisComment_Entry));
}

static void vorbiscomment_entry_array_delete_(FLAC__StreamMetadata_VorbisComment_Entry *object_array, unsigned num_comments)
{
	unsigned i;

	FLAC__ASSERT(0 != object_array && num_comments > 0);

	for(i = 0; i < num_comments; i++)
		if(0 != object_array[i].entry)
			free(object_array[i].entry);

	if(0 != object_array)
		free(object_array);
}

static FLAC__StreamMetadata_VorbisComment_Entry *vorbiscomment_entry_array_copy_(const FLAC__StreamMetadata_VorbisComment_Entry *object_array, unsigned num_comments)
{
	FLAC__StreamMetadata_VorbisComment_Entry *return_array;

	FLAC__ASSERT(0 != object_array);
	FLAC__ASSERT(num_comments > 0);

	return_array = vorbiscomment_entry_array_new_(num_comments);

	if(0 != return_array) {
		unsigned i;

		/* Need to do this to set the pointers inside the comments to 0.
		 * In case of an error in the following loop, the object will be
		 * deleted and we don't want the destructor freeing uninitialized
		 * pointers.
		 */
		memset(return_array, 0, num_comments * sizeof(FLAC__StreamMetadata_VorbisComment_Entry));

		for(i = 0; i < num_comments; i++) {
			if(!copy_vcentry_(return_array+i, object_array+i)) {
				vorbiscomment_entry_array_delete_(return_array, num_comments);
				return 0;
			}
		}
	}

	return return_array;
}

static FLAC__bool vorbiscomment_set_entry_(FLAC__StreamMetadata *object, FLAC__StreamMetadata_VorbisComment_Entry *dest, const FLAC__StreamMetadata_VorbisComment_Entry *src, FLAC__bool copy)
{
	FLAC__byte *save;

	FLAC__ASSERT(0 != object);
	FLAC__ASSERT(0 != dest);
	FLAC__ASSERT(0 != src);
	FLAC__ASSERT(object->type == FLAC__METADATA_TYPE_VORBIS_COMMENT);
	FLAC__ASSERT((0 != src->entry && src->length > 0) || (0 == src->entry && src->length == 0 && copy == false));

	save = dest->entry;

	/* do the copy first so that if we fail we leave the object untouched */
	if(copy) {
		if(!copy_vcentry_(dest, src))
			return false;
	}
	else {
		*dest = *src;
	}

	if(0 != save)
		free(save);

	vorbiscomment_calculate_length_(object);
	return true;
}


/****************************************************************************
 *
 * Metadata object routines
 *
 ***************************************************************************/

FLAC_API FLAC__StreamMetadata *FLAC__metadata_object_new(FLAC__MetadataType type)
{
	FLAC__StreamMetadata *object = calloc(1, sizeof(FLAC__StreamMetadata));
	if(0 != object) {
		object->is_last = false;
		object->type = type;
		switch(type) {
			case FLAC__METADATA_TYPE_STREAMINFO:
				object->length = FLAC__STREAM_METADATA_STREAMINFO_LENGTH;
				break;
			case FLAC__METADATA_TYPE_PADDING:
				break;
			case FLAC__METADATA_TYPE_APPLICATION:
				object->length = FLAC__STREAM_METADATA_APPLICATION_ID_LEN / 8;
				break;
			case FLAC__METADATA_TYPE_SEEKTABLE:
				break;
			case FLAC__METADATA_TYPE_VORBIS_COMMENT:
				object->length = (FLAC__STREAM_METADATA_VORBIS_COMMENT_ENTRY_LENGTH_LEN + FLAC__STREAM_METADATA_VORBIS_COMMENT_NUM_COMMENTS_LEN) / 8;
				break;
			default:
				/* double protection: */
				FLAC__ASSERT(0);
				free(object);
				return 0;
		}
	}

	return object;
}

FLAC_API FLAC__StreamMetadata *FLAC__metadata_object_clone(const FLAC__StreamMetadata *object)
{
	FLAC__StreamMetadata *to;

	FLAC__ASSERT(0 != object);

	if(0 != (to = FLAC__metadata_object_new(object->type))) {
		to->is_last = object->is_last;
		to->type = object->type;
		to->length = object->length;
		switch(to->type) {
			case FLAC__METADATA_TYPE_STREAMINFO:
				memcpy(&to->data.stream_info, &object->data.stream_info, sizeof(FLAC__StreamMetadata_StreamInfo));
				break;
			case FLAC__METADATA_TYPE_PADDING:
				break;
			case FLAC__METADATA_TYPE_APPLICATION:
				memcpy(&to->data.application.id, &object->data.application.id, FLAC__STREAM_METADATA_APPLICATION_ID_LEN / 8);
				if(!copy_bytes_(&to->data.application.data, object->data.application.data, object->length - FLAC__STREAM_METADATA_APPLICATION_ID_LEN / 8)) {
					FLAC__metadata_object_delete(to);
					return 0;
				}
				break;
			case FLAC__METADATA_TYPE_SEEKTABLE:
				to->data.seek_table.num_points = object->data.seek_table.num_points;
				if(!copy_bytes_((FLAC__byte**)&to->data.seek_table.points, (FLAC__byte*)object->data.seek_table.points, object->data.seek_table.num_points * sizeof(FLAC__StreamMetadata_SeekPoint))) {
					FLAC__metadata_object_delete(to);
					return 0;
				}
				break;
			case FLAC__METADATA_TYPE_VORBIS_COMMENT:
				if(!copy_vcentry_(&to->data.vorbis_comment.vendor_string, &object->data.vorbis_comment.vendor_string)) {
					FLAC__metadata_object_delete(to);
					return 0;
				}
				if(object->data.vorbis_comment.num_comments == 0) {
					FLAC__ASSERT(0 == object->data.vorbis_comment.comments);
					to->data.vorbis_comment.comments = 0;
				}
				else {
					FLAC__ASSERT(0 != object->data.vorbis_comment.comments);
					to->data.vorbis_comment.comments = vorbiscomment_entry_array_copy_(object->data.vorbis_comment.comments, object->data.vorbis_comment.num_comments);
					if(0 == to->data.vorbis_comment.comments) {
						FLAC__metadata_object_delete(to);
						return 0;
					}
				}
				to->data.vorbis_comment.num_comments = object->data.vorbis_comment.num_comments;
				break;
			default:
				/* double protection: */
				FLAC__ASSERT(0);
				free(to);
				return 0;
		}
	}

	return to;
}

void FLAC__metadata_object_delete_data(FLAC__StreamMetadata *object)
{
	FLAC__ASSERT(0 != object);

	switch(object->type) {
		case FLAC__METADATA_TYPE_STREAMINFO:
		case FLAC__METADATA_TYPE_PADDING:
			break;
		case FLAC__METADATA_TYPE_APPLICATION:
			if(0 != object->data.application.data)
				free(object->data.application.data);
			break;
		case FLAC__METADATA_TYPE_SEEKTABLE:
			if(0 != object->data.seek_table.points)
				free(object->data.seek_table.points);
			break;
		case FLAC__METADATA_TYPE_VORBIS_COMMENT:
			if(0 != object->data.vorbis_comment.vendor_string.entry)
				free(object->data.vorbis_comment.vendor_string.entry);
			if(0 != object->data.vorbis_comment.comments) {
				FLAC__ASSERT(object->data.vorbis_comment.num_comments > 0);
				vorbiscomment_entry_array_delete_(object->data.vorbis_comment.comments, object->data.vorbis_comment.num_comments);
			}
			break;
		default:
			FLAC__ASSERT(0);
	}
}

FLAC_API void FLAC__metadata_object_delete(FLAC__StreamMetadata *object)
{
	FLAC__metadata_object_delete_data(object);
	free(object);
}

static FLAC__bool compare_block_data_streaminfo_(const FLAC__StreamMetadata_StreamInfo *block1, const FLAC__StreamMetadata_StreamInfo *block2)
{
	if(block1->min_blocksize != block2->min_blocksize)
		return false;
	if(block1->max_blocksize != block2->max_blocksize)
		return false;
	if(block1->min_framesize != block2->min_framesize)
		return false;
	if(block1->max_framesize != block2->max_framesize)
		return false;
	if(block1->sample_rate != block2->sample_rate)
		return false;
	if(block1->channels != block2->channels)
		return false;
	if(block1->bits_per_sample != block2->bits_per_sample)
		return false;
	if(block1->total_samples != block2->total_samples)
		return false;
	if(0 != memcmp(block1->md5sum, block2->md5sum, 16))
		return false;
	return true;
}

static FLAC__bool compare_block_data_application_(const FLAC__StreamMetadata_Application *block1, const FLAC__StreamMetadata_Application *block2, unsigned block_length)
{
	FLAC__ASSERT(0 != block1);
	FLAC__ASSERT(0 != block2);
	FLAC__ASSERT(block_length >= sizeof(block1->id));

	if(0 != memcmp(block1->id, block2->id, sizeof(block1->id)))
		return false;
	if(0 != block1->data && 0 != block2->data)
		return 0 == memcmp(block1->data, block2->data, block_length - sizeof(block1->id));
	else
		return block1->data == block2->data;
}

static FLAC__bool compare_block_data_seektable_(const FLAC__StreamMetadata_SeekTable *block1, const FLAC__StreamMetadata_SeekTable *block2)
{
	unsigned i;

	FLAC__ASSERT(0 != block1);
	FLAC__ASSERT(0 != block2);

	if(block1->num_points != block2->num_points)
		return false;

	if(0 != block1->points && 0 != block2->points) {
		for(i = 0; i < block1->num_points; i++) {
			if(block1->points[i].sample_number != block2->points[i].sample_number)
				return false;
			if(block1->points[i].stream_offset != block2->points[i].stream_offset)
				return false;
			if(block1->points[i].frame_samples != block2->points[i].frame_samples)
				return false;
		}
		return true;
	}
	else
		return block1->points == block2->points;
}

static FLAC__bool compare_block_data_vorbiscomment_(const FLAC__StreamMetadata_VorbisComment *block1, const FLAC__StreamMetadata_VorbisComment *block2)
{
	unsigned i;

	if(block1->vendor_string.length != block2->vendor_string.length)
		return false;

	if(0 != block1->vendor_string.entry && 0 != block2->vendor_string.entry) {
		if(0 != memcmp(block1->vendor_string.entry, block2->vendor_string.entry, block1->vendor_string.length))
			return false;
	}
	else if(block1->vendor_string.entry != block2->vendor_string.entry)
		return false;

	if(block1->num_comments != block2->num_comments)
		return false;

	for(i = 0; i < block1->num_comments; i++) {
		if(0 != block1->comments[i].entry && 0 != block2->comments[i].entry) {
			if(0 != memcmp(block1->comments[i].entry, block2->comments[i].entry, block1->comments[i].length))
				return false;
		}
		else if(block1->comments[i].entry != block2->comments[i].entry)
			return false;
	}
	return true;
}

FLAC_API FLAC__bool FLAC__metadata_object_is_equal(const FLAC__StreamMetadata *block1, const FLAC__StreamMetadata *block2)
{
	FLAC__ASSERT(0 != block1);
	FLAC__ASSERT(0 != block2);

	if(block1->type != block2->type) {
		return false;
	}
	if(block1->is_last != block2->is_last) {
		return false;
	}
	if(block1->length != block2->length) {
		return false;
	}
	switch(block1->type) {
		case FLAC__METADATA_TYPE_STREAMINFO:
			return compare_block_data_streaminfo_(&block1->data.stream_info, &block2->data.stream_info);
		case FLAC__METADATA_TYPE_PADDING:
			return true; /* we don't compare the padding guts */
		case FLAC__METADATA_TYPE_APPLICATION:
			return compare_block_data_application_(&block1->data.application, &block2->data.application, block1->length);
		case FLAC__METADATA_TYPE_SEEKTABLE:
			return compare_block_data_seektable_(&block1->data.seek_table, &block2->data.seek_table);
		case FLAC__METADATA_TYPE_VORBIS_COMMENT:
			return compare_block_data_vorbiscomment_(&block1->data.vorbis_comment, &block2->data.vorbis_comment);
		default:
			FLAC__ASSERT(0);
			return false;
	}
}

FLAC_API FLAC__bool FLAC__metadata_object_application_set_data(FLAC__StreamMetadata *object, FLAC__byte *data, unsigned length, FLAC__bool copy)
{
	FLAC__byte *save;

	FLAC__ASSERT(0 != object);
	FLAC__ASSERT(object->type == FLAC__METADATA_TYPE_APPLICATION);
	FLAC__ASSERT((0 != data && length > 0) || (0 == data && length == 0 && copy == false));

	save = object->data.application.data;

	/* do the copy first so that if we fail we leave the object untouched */
	if(copy) {
		if(!copy_bytes_(&object->data.application.data, data, length))
			return false;
	}
	else {
		object->data.application.data = data;
	}

	if(0 != save)
		free(save);

	object->length = FLAC__STREAM_METADATA_APPLICATION_ID_LEN / 8 + length;
	return true;
}

FLAC_API FLAC__bool FLAC__metadata_object_seektable_resize_points(FLAC__StreamMetadata *object, unsigned new_num_points)
{
	FLAC__ASSERT(0 != object);
	FLAC__ASSERT(object->type == FLAC__METADATA_TYPE_SEEKTABLE);

	if(0 == object->data.seek_table.points) {
		FLAC__ASSERT(object->data.seek_table.num_points == 0);
		if(0 == new_num_points)
			return true;
		else if(0 == (object->data.seek_table.points = seekpoint_array_new_(new_num_points)))
			return false;
	}
	else {
		const unsigned old_size = object->data.seek_table.num_points * sizeof(FLAC__StreamMetadata_SeekPoint);
		const unsigned new_size = new_num_points * sizeof(FLAC__StreamMetadata_SeekPoint);

		FLAC__ASSERT(object->data.seek_table.num_points > 0);

		if(new_size == 0) {
			free(object->data.seek_table.points);
			object->data.seek_table.points = 0;
		}
		else if(0 == (object->data.seek_table.points = realloc(object->data.seek_table.points, new_size)))
			return false;

		/* if growing, set new elements to placeholders */
		if(new_size > old_size) {
			unsigned i;
			for(i = object->data.seek_table.num_points; i < new_num_points; i++) {
				object->data.seek_table.points[i].sample_number = FLAC__STREAM_METADATA_SEEKPOINT_PLACEHOLDER;
				object->data.seek_table.points[i].stream_offset = 0;
				object->data.seek_table.points[i].frame_samples = 0;
			}
		}
	}

	object->data.seek_table.num_points = new_num_points;

	seektable_calculate_length_(object);
	return true;
}

FLAC_API void FLAC__metadata_object_seektable_set_point(FLAC__StreamMetadata *object, unsigned point_num, FLAC__StreamMetadata_SeekPoint point)
{
	FLAC__ASSERT(0 != object);
	FLAC__ASSERT(object->type == FLAC__METADATA_TYPE_SEEKTABLE);
	FLAC__ASSERT(object->data.seek_table.num_points > point_num);

	object->data.seek_table.points[point_num] = point;
}

FLAC_API FLAC__bool FLAC__metadata_object_seektable_insert_point(FLAC__StreamMetadata *object, unsigned point_num, FLAC__StreamMetadata_SeekPoint point)
{
	int i;

	FLAC__ASSERT(0 != object);
	FLAC__ASSERT(object->type == FLAC__METADATA_TYPE_SEEKTABLE);
	FLAC__ASSERT(object->data.seek_table.num_points >= point_num);

	if(!FLAC__metadata_object_seektable_resize_points(object, object->data.seek_table.num_points+1))
		return false;

	/* move all points >= point_num forward one space */
	for(i = (int)object->data.seek_table.num_points-1; i > (int)point_num; i--)
		object->data.seek_table.points[i] = object->data.seek_table.points[i-1];

	FLAC__metadata_object_seektable_set_point(object, point_num, point);
	seektable_calculate_length_(object);
	return true;
}

FLAC_API FLAC__bool FLAC__metadata_object_seektable_delete_point(FLAC__StreamMetadata *object, unsigned point_num)
{
	unsigned i;

	FLAC__ASSERT(0 != object);
	FLAC__ASSERT(object->type == FLAC__METADATA_TYPE_SEEKTABLE);
	FLAC__ASSERT(object->data.seek_table.num_points > point_num);

	/* move all points > point_num backward one space */
	for(i = point_num; i < object->data.seek_table.num_points-1; i++)
		object->data.seek_table.points[i] = object->data.seek_table.points[i+1];

	return FLAC__metadata_object_seektable_resize_points(object, object->data.seek_table.num_points-1);
}

FLAC_API FLAC__bool FLAC__metadata_object_seektable_is_legal(const FLAC__StreamMetadata *object)
{
	FLAC__ASSERT(0 != object);
	FLAC__ASSERT(object->type == FLAC__METADATA_TYPE_SEEKTABLE);

	return FLAC__format_seektable_is_legal(&object->data.seek_table);
}

FLAC_API FLAC__bool FLAC__metadata_object_seektable_template_append_placeholders(FLAC__StreamMetadata *object, unsigned num)
{
	FLAC__ASSERT(0 != object);
	FLAC__ASSERT(object->type == FLAC__METADATA_TYPE_SEEKTABLE);

	if(num > 0)
		/* WATCHOUT: we rely on the fact that growing the array adds PLACEHOLDERS at the end */
		return FLAC__metadata_object_seektable_resize_points(object, object->data.seek_table.num_points + num);
	else
		return true;
}

FLAC_API FLAC__bool FLAC__metadata_object_seektable_template_append_point(FLAC__StreamMetadata *object, FLAC__uint64 sample_number)
{
	FLAC__StreamMetadata_SeekTable *seek_table;

	FLAC__ASSERT(0 != object);
	FLAC__ASSERT(object->type == FLAC__METADATA_TYPE_SEEKTABLE);

	seek_table = &object->data.seek_table;

	if(!FLAC__metadata_object_seektable_resize_points(object, seek_table->num_points + 1))
		return false;

	seek_table->points[seek_table->num_points - 1].sample_number = sample_number;
	seek_table->points[seek_table->num_points - 1].stream_offset = 0;
	seek_table->points[seek_table->num_points - 1].frame_samples = 0;

	return true;
}

FLAC_API FLAC__bool FLAC__metadata_object_seektable_template_append_points(FLAC__StreamMetadata *object, FLAC__uint64 sample_numbers[], unsigned num)
{
	FLAC__ASSERT(0 != object);
	FLAC__ASSERT(object->type == FLAC__METADATA_TYPE_SEEKTABLE);
	FLAC__ASSERT(0 != sample_numbers || num == 0);

	if(num > 0) {
		FLAC__StreamMetadata_SeekTable *seek_table = &object->data.seek_table;
		unsigned i, j;

		i = seek_table->num_points;

		if(!FLAC__metadata_object_seektable_resize_points(object, seek_table->num_points + num))
			return false;

		for(j = 0; j < num; i++, j++) {
			seek_table->points[i].sample_number = sample_numbers[j];
			seek_table->points[i].stream_offset = 0;
			seek_table->points[i].frame_samples = 0;
		}
	}

	return true;
}

FLAC_API FLAC__bool FLAC__metadata_object_seektable_template_append_spaced_points(FLAC__StreamMetadata *object, unsigned num, FLAC__uint64 total_samples)
{
	FLAC__ASSERT(0 != object);
	FLAC__ASSERT(object->type == FLAC__METADATA_TYPE_SEEKTABLE);
	FLAC__ASSERT(total_samples > 0);

	if(num > 0) {
		FLAC__StreamMetadata_SeekTable *seek_table = &object->data.seek_table;
		unsigned i, j;

		i = seek_table->num_points;

		if(!FLAC__metadata_object_seektable_resize_points(object, seek_table->num_points + num))
			return false;

		for(j = 0; j < num; i++, j++) {
			seek_table->points[i].sample_number = total_samples * (FLAC__uint64)j / (FLAC__uint64)num;
			seek_table->points[i].stream_offset = 0;
			seek_table->points[i].frame_samples = 0;
		}
	}

	return true;
}

FLAC_API FLAC__bool FLAC__metadata_object_seektable_template_sort(FLAC__StreamMetadata *object, FLAC__bool compact)
{
	unsigned unique;

	FLAC__ASSERT(0 != object);
	FLAC__ASSERT(object->type == FLAC__METADATA_TYPE_SEEKTABLE);

	unique = FLAC__format_seektable_sort(&object->data.seek_table);

	return !compact || FLAC__metadata_object_seektable_resize_points(object, unique);
}

FLAC_API FLAC__bool FLAC__metadata_object_vorbiscomment_set_vendor_string(FLAC__StreamMetadata *object, FLAC__StreamMetadata_VorbisComment_Entry entry, FLAC__bool copy)
{
	return vorbiscomment_set_entry_(object, &object->data.vorbis_comment.vendor_string, &entry, copy);
}

FLAC_API FLAC__bool FLAC__metadata_object_vorbiscomment_resize_comments(FLAC__StreamMetadata *object, unsigned new_num_comments)
{
	FLAC__ASSERT(0 != object);
	FLAC__ASSERT(object->type == FLAC__METADATA_TYPE_VORBIS_COMMENT);

	if(0 == object->data.vorbis_comment.comments) {
		FLAC__ASSERT(object->data.vorbis_comment.num_comments == 0);
		if(0 == new_num_comments)
			return true;
		else if(0 == (object->data.vorbis_comment.comments = vorbiscomment_entry_array_new_(new_num_comments)))
			return false;
	}
	else {
		const unsigned old_size = object->data.vorbis_comment.num_comments * sizeof(FLAC__StreamMetadata_VorbisComment_Entry);
		const unsigned new_size = new_num_comments * sizeof(FLAC__StreamMetadata_VorbisComment_Entry);

		FLAC__ASSERT(object->data.vorbis_comment.num_comments > 0);

		/* if shrinking, free the truncated entries */
		if(new_num_comments < object->data.vorbis_comment.num_comments) {
			unsigned i;
			for(i = new_num_comments; i < object->data.vorbis_comment.num_comments; i++)
				if(0 != object->data.vorbis_comment.comments[i].entry)
					free(object->data.vorbis_comment.comments[i].entry);
		}

		if(new_size == 0) {
			free(object->data.vorbis_comment.comments);
			object->data.vorbis_comment.comments = 0;
		}
		else if(0 == (object->data.vorbis_comment.comments = realloc(object->data.vorbis_comment.comments, new_size)))
			return false;

		/* if growing, zero all the length/pointers of new elements */
		if(new_size > old_size)
			memset(object->data.vorbis_comment.comments + object->data.vorbis_comment.num_comments, 0, new_size - old_size);
	}

	object->data.vorbis_comment.num_comments = new_num_comments;

	vorbiscomment_calculate_length_(object);
	return true;
}

FLAC_API FLAC__bool FLAC__metadata_object_vorbiscomment_set_comment(FLAC__StreamMetadata *object, unsigned comment_num, FLAC__StreamMetadata_VorbisComment_Entry entry, FLAC__bool copy)
{
	return vorbiscomment_set_entry_(object, &object->data.vorbis_comment.comments[comment_num], &entry, copy);
}

FLAC_API FLAC__bool FLAC__metadata_object_vorbiscomment_insert_comment(FLAC__StreamMetadata *object, unsigned comment_num, FLAC__StreamMetadata_VorbisComment_Entry entry, FLAC__bool copy)
{
	FLAC__StreamMetadata_VorbisComment *vc;

	FLAC__ASSERT(0 != object);
	FLAC__ASSERT(object->type == FLAC__METADATA_TYPE_VORBIS_COMMENT);
	FLAC__ASSERT(object->data.vorbis_comment.num_comments >= comment_num);

	vc = &object->data.vorbis_comment;

	if(!FLAC__metadata_object_vorbiscomment_resize_comments(object, vc->num_comments+1))
		return false;

	/* move all comments >= comment_num forward one space */
	memmove(&vc->comments[comment_num+1], &vc->comments[comment_num], sizeof(FLAC__StreamMetadata_VorbisComment_Entry)*(vc->num_comments-1-comment_num));
	vc->comments[comment_num].length = 0;
	vc->comments[comment_num].entry = 0;

	return FLAC__metadata_object_vorbiscomment_set_comment(object, comment_num, entry, copy);
}

FLAC_API FLAC__bool FLAC__metadata_object_vorbiscomment_delete_comment(FLAC__StreamMetadata *object, unsigned comment_num)
{
	FLAC__StreamMetadata_VorbisComment *vc;

	FLAC__ASSERT(0 != object);
	FLAC__ASSERT(object->type == FLAC__METADATA_TYPE_VORBIS_COMMENT);
	FLAC__ASSERT(object->data.vorbis_comment.num_comments > comment_num);

	vc = &object->data.vorbis_comment;

	/* free the comment at comment_num */
	if(0 != vc->comments[comment_num].entry)
		free(vc->comments[comment_num].entry);

	/* move all comments > comment_num backward one space */
	memmove(&vc->comments[comment_num], &vc->comments[comment_num+1], sizeof(FLAC__StreamMetadata_VorbisComment_Entry)*(vc->num_comments-comment_num-1));
	vc->comments[vc->num_comments-1].length = 0;
	vc->comments[vc->num_comments-1].entry = 0;

	return FLAC__metadata_object_vorbiscomment_resize_comments(object, vc->num_comments-1);
}
