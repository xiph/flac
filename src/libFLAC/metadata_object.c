/* libFLAC - Free Lossless Audio Codec library
 * Copyright (C) 2001,2002,2003,2004  Josh Coalson
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * - Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *
 * - Redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in the
 * documentation and/or other materials provided with the distribution.
 *
 * - Neither the name of the Xiph.org Foundation nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
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
		if(0 == (x = (FLAC__byte*)malloc(bytes)))
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
		if(0 == (x = (FLAC__byte*)malloc(from->length)))
			return false;
		memcpy(x, from->entry, from->length);
		to->entry = x;
	}
	return true;
}

static FLAC__bool copy_track_(FLAC__StreamMetadata_CueSheet_Track *to, const FLAC__StreamMetadata_CueSheet_Track *from)
{
	memcpy(to, from, sizeof(FLAC__StreamMetadata_CueSheet_Track));
	if(0 == from->indices) {
		FLAC__ASSERT(from->num_indices == 0);
	}
	else {
		FLAC__StreamMetadata_CueSheet_Index *x;
		FLAC__ASSERT(from->num_indices > 0);
		if(0 == (x = (FLAC__StreamMetadata_CueSheet_Index*)malloc(from->num_indices * sizeof(FLAC__StreamMetadata_CueSheet_Index))))
			return false;
		memcpy(x, from->indices, from->num_indices * sizeof(FLAC__StreamMetadata_CueSheet_Index));
		to->indices = x;
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

	object_array = (FLAC__StreamMetadata_SeekPoint*)malloc(num_points * sizeof(FLAC__StreamMetadata_SeekPoint));

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

	return (FLAC__StreamMetadata_VorbisComment_Entry*)calloc(num_comments, sizeof(FLAC__StreamMetadata_VorbisComment_Entry));
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

static void cuesheet_calculate_length_(FLAC__StreamMetadata *object)
{
	unsigned i;

	FLAC__ASSERT(object->type == FLAC__METADATA_TYPE_CUESHEET);

	object->length = (
		FLAC__STREAM_METADATA_CUESHEET_MEDIA_CATALOG_NUMBER_LEN +
		FLAC__STREAM_METADATA_CUESHEET_LEAD_IN_LEN +
		FLAC__STREAM_METADATA_CUESHEET_IS_CD_LEN +
		FLAC__STREAM_METADATA_CUESHEET_RESERVED_LEN +
		FLAC__STREAM_METADATA_CUESHEET_NUM_TRACKS_LEN
	) / 8;

	object->length += object->data.cue_sheet.num_tracks * (
		FLAC__STREAM_METADATA_CUESHEET_TRACK_OFFSET_LEN +
		FLAC__STREAM_METADATA_CUESHEET_TRACK_NUMBER_LEN +
		FLAC__STREAM_METADATA_CUESHEET_TRACK_ISRC_LEN +
		FLAC__STREAM_METADATA_CUESHEET_TRACK_TYPE_LEN +
		FLAC__STREAM_METADATA_CUESHEET_TRACK_PRE_EMPHASIS_LEN +
		FLAC__STREAM_METADATA_CUESHEET_TRACK_RESERVED_LEN +
		FLAC__STREAM_METADATA_CUESHEET_TRACK_NUM_INDICES_LEN
	) / 8;

	for(i = 0; i < object->data.cue_sheet.num_tracks; i++) {
		object->length += object->data.cue_sheet.tracks[i].num_indices * (
			FLAC__STREAM_METADATA_CUESHEET_INDEX_OFFSET_LEN +
			FLAC__STREAM_METADATA_CUESHEET_INDEX_NUMBER_LEN +
			FLAC__STREAM_METADATA_CUESHEET_INDEX_RESERVED_LEN
		) / 8;
	}
}

static FLAC__StreamMetadata_CueSheet_Index *cuesheet_track_index_array_new_(unsigned num_indices)
{
	FLAC__ASSERT(num_indices > 0);

	return (FLAC__StreamMetadata_CueSheet_Index*)calloc(num_indices, sizeof(FLAC__StreamMetadata_CueSheet_Index));
}

static FLAC__StreamMetadata_CueSheet_Track *cuesheet_track_array_new_(unsigned num_tracks)
{
	FLAC__ASSERT(num_tracks > 0);

	return (FLAC__StreamMetadata_CueSheet_Track*)calloc(num_tracks, sizeof(FLAC__StreamMetadata_CueSheet_Track));
}

static void cuesheet_track_array_delete_(FLAC__StreamMetadata_CueSheet_Track *object_array, unsigned num_tracks)
{
	unsigned i;

	FLAC__ASSERT(0 != object_array && num_tracks > 0);

	for(i = 0; i < num_tracks; i++) {
		if(0 != object_array[i].indices) {
			FLAC__ASSERT(object_array[i].num_indices > 0);
			free(object_array[i].indices);
		}
	}

	if(0 != object_array)
		free(object_array);
}

static FLAC__StreamMetadata_CueSheet_Track *cuesheet_track_array_copy_(const FLAC__StreamMetadata_CueSheet_Track *object_array, unsigned num_tracks)
{
	FLAC__StreamMetadata_CueSheet_Track *return_array;

	FLAC__ASSERT(0 != object_array);
	FLAC__ASSERT(num_tracks > 0);

	return_array = cuesheet_track_array_new_(num_tracks);

	if(0 != return_array) {
		unsigned i;

		for(i = 0; i < num_tracks; i++) {
			if(!copy_track_(return_array+i, object_array+i)) {
				cuesheet_track_array_delete_(return_array, num_tracks);
				return 0;
			}
		}
	}

	return return_array;
}

static FLAC__bool cuesheet_set_track_(FLAC__StreamMetadata *object, FLAC__StreamMetadata_CueSheet_Track *dest, const FLAC__StreamMetadata_CueSheet_Track *src, FLAC__bool copy)
{
	FLAC__StreamMetadata_CueSheet_Index *save;

	FLAC__ASSERT(0 != object);
	FLAC__ASSERT(0 != dest);
	FLAC__ASSERT(0 != src);
	FLAC__ASSERT(object->type == FLAC__METADATA_TYPE_CUESHEET);
	FLAC__ASSERT((0 != src->indices && src->num_indices > 0) || (0 == src->indices && src->num_indices == 0));

	save = dest->indices;

	/* do the copy first so that if we fail we leave the object untouched */
	if(copy) {
		if(!copy_track_(dest, src))
			return false;
	}
	else {
		*dest = *src;
	}

	if(0 != save)
		free(save);

	cuesheet_calculate_length_(object);
	return true;
}


/****************************************************************************
 *
 * Metadata object routines
 *
 ***************************************************************************/

FLAC_API FLAC__StreamMetadata *FLAC__metadata_object_new(FLAC__MetadataType type)
{
	FLAC__StreamMetadata *object = (FLAC__StreamMetadata*)calloc(1, sizeof(FLAC__StreamMetadata));
	if(0 != object) {
		object->is_last = false;
		object->type = type;
		switch(type) {
			case FLAC__METADATA_TYPE_STREAMINFO:
				object->length = FLAC__STREAM_METADATA_STREAMINFO_LENGTH;
				break;
			case FLAC__METADATA_TYPE_PADDING:
				/* calloc() took care of this for us:
				object->length = 0;
				*/
				break;
			case FLAC__METADATA_TYPE_APPLICATION:
				object->length = FLAC__STREAM_METADATA_APPLICATION_ID_LEN / 8;
				/* calloc() took care of this for us:
				object->data.application.data = 0;
				*/
				break;
			case FLAC__METADATA_TYPE_SEEKTABLE:
				/* calloc() took care of this for us:
				object->length = 0;
				object->data.seek_table.num_points = 0;
				object->data.seek_table.points = 0;
				*/
				break;
			case FLAC__METADATA_TYPE_VORBIS_COMMENT:
				{
					object->data.vorbis_comment.vendor_string.length = (unsigned)strlen(FLAC__VENDOR_STRING);
					if(!copy_bytes_(&object->data.vorbis_comment.vendor_string.entry, (const FLAC__byte*)FLAC__VENDOR_STRING, object->data.vorbis_comment.vendor_string.length)) {
						free(object);
						return 0;
					}
					vorbiscomment_calculate_length_(object);
				}
				break;
			case FLAC__METADATA_TYPE_CUESHEET:
				cuesheet_calculate_length_(object);
				break;
			default:
				/* calloc() took care of this for us:
				object->length = 0;
				object->data.unknown.data = 0;
				*/
				break;
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
				if(0 != to->data.vorbis_comment.vendor_string.entry) {
					free(to->data.vorbis_comment.vendor_string.entry);
					to->data.vorbis_comment.vendor_string.entry = 0;
				}
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
			case FLAC__METADATA_TYPE_CUESHEET:
				memcpy(&to->data.cue_sheet, &object->data.cue_sheet, sizeof(FLAC__StreamMetadata_CueSheet));
				if(object->data.cue_sheet.num_tracks == 0) {
					FLAC__ASSERT(0 == object->data.cue_sheet.tracks);
				}
				else {
					FLAC__ASSERT(0 != object->data.cue_sheet.tracks);
					to->data.cue_sheet.tracks = cuesheet_track_array_copy_(object->data.cue_sheet.tracks, object->data.cue_sheet.num_tracks);
					if(0 == to->data.cue_sheet.tracks) {
						FLAC__metadata_object_delete(to);
						return 0;
					}
				}
				break;
			default:
				if(!copy_bytes_(&to->data.unknown.data, object->data.unknown.data, object->length)) {
					FLAC__metadata_object_delete(to);
					return 0;
				}
				break;
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
			if(0 != object->data.application.data) {
				free(object->data.application.data);
				object->data.application.data = 0;
			}
			break;
		case FLAC__METADATA_TYPE_SEEKTABLE:
			if(0 != object->data.seek_table.points) {
				free(object->data.seek_table.points);
				object->data.seek_table.points = 0;
			}
			break;
		case FLAC__METADATA_TYPE_VORBIS_COMMENT:
			if(0 != object->data.vorbis_comment.vendor_string.entry) {
				free(object->data.vorbis_comment.vendor_string.entry);
				object->data.vorbis_comment.vendor_string.entry = 0;
			}
			if(0 != object->data.vorbis_comment.comments) {
				FLAC__ASSERT(object->data.vorbis_comment.num_comments > 0);
				vorbiscomment_entry_array_delete_(object->data.vorbis_comment.comments, object->data.vorbis_comment.num_comments);
			}
			break;
		case FLAC__METADATA_TYPE_CUESHEET:
			if(0 != object->data.cue_sheet.tracks) {
				FLAC__ASSERT(object->data.cue_sheet.num_tracks > 0);
				cuesheet_track_array_delete_(object->data.cue_sheet.tracks, object->data.cue_sheet.num_tracks);
			}
			break;
		default:
			if(0 != object->data.unknown.data) {
				free(object->data.unknown.data);
				object->data.unknown.data = 0;
			}
			break;
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

static FLAC__bool compare_block_data_cuesheet_(const FLAC__StreamMetadata_CueSheet *block1, const FLAC__StreamMetadata_CueSheet *block2)
{
	unsigned i, j;

	if(0 != strcmp(block1->media_catalog_number, block2->media_catalog_number))
		return false;

	if(block1->lead_in != block2->lead_in)
		return false;

	if(block1->is_cd != block2->is_cd)
		return false;

	if(block1->num_tracks != block2->num_tracks)
		return false;

	if(0 != block1->tracks && 0 != block2->tracks) {
		FLAC__ASSERT(block1->num_tracks > 0);
		for(i = 0; i < block1->num_tracks; i++) {
			if(block1->tracks[i].offset != block2->tracks[i].offset)
				return false;
			if(block1->tracks[i].number != block2->tracks[i].number)
				return false;
			if(0 != memcmp(block1->tracks[i].isrc, block2->tracks[i].isrc, sizeof(block1->tracks[i].isrc)))
				return false;
			if(block1->tracks[i].type != block2->tracks[i].type)
				return false;
			if(block1->tracks[i].pre_emphasis != block2->tracks[i].pre_emphasis)
				return false;
			if(block1->tracks[i].num_indices != block2->tracks[i].num_indices)
				return false;
			if(0 != block1->tracks[i].indices && 0 != block2->tracks[i].indices) {
				FLAC__ASSERT(block1->tracks[i].num_indices > 0);
				for(j = 0; j < block1->tracks[i].num_indices; j++) {
					if(block1->tracks[i].indices[j].offset != block2->tracks[i].indices[j].offset)
						return false;
					if(block1->tracks[i].indices[j].number != block2->tracks[i].indices[j].number)
						return false;
				}
			}
			else if(block1->tracks[i].indices != block2->tracks[i].indices)
				return false;
		}
	}
	else if(block1->tracks != block2->tracks)
		return false;
	return true;
}

static FLAC__bool compare_block_data_unknown_(const FLAC__StreamMetadata_Unknown *block1, const FLAC__StreamMetadata_Unknown *block2, unsigned block_length)
{
	FLAC__ASSERT(0 != block1);
	FLAC__ASSERT(0 != block2);

	if(0 != block1->data && 0 != block2->data)
		return 0 == memcmp(block1->data, block2->data, block_length);
	else
		return block1->data == block2->data;
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
		case FLAC__METADATA_TYPE_CUESHEET:
			return compare_block_data_cuesheet_(&block1->data.cue_sheet, &block2->data.cue_sheet);
		default:
			return compare_block_data_unknown_(&block1->data.unknown, &block2->data.unknown, block1->length);
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
		else if(0 == (object->data.seek_table.points = (FLAC__StreamMetadata_SeekPoint*)realloc(object->data.seek_table.points, new_size)))
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
	FLAC__ASSERT(point_num < object->data.seek_table.num_points);

	object->data.seek_table.points[point_num] = point;
}

FLAC_API FLAC__bool FLAC__metadata_object_seektable_insert_point(FLAC__StreamMetadata *object, unsigned point_num, FLAC__StreamMetadata_SeekPoint point)
{
	int i;

	FLAC__ASSERT(0 != object);
	FLAC__ASSERT(object->type == FLAC__METADATA_TYPE_SEEKTABLE);
	FLAC__ASSERT(point_num <= object->data.seek_table.num_points);

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
	FLAC__ASSERT(point_num < object->data.seek_table.num_points);

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
		else if(0 == (object->data.vorbis_comment.comments = (FLAC__StreamMetadata_VorbisComment_Entry*)realloc(object->data.vorbis_comment.comments, new_size)))
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
	FLAC__ASSERT(0 != object);
	FLAC__ASSERT(comment_num < object->data.vorbis_comment.num_comments);

	return vorbiscomment_set_entry_(object, &object->data.vorbis_comment.comments[comment_num], &entry, copy);
}

FLAC_API FLAC__bool FLAC__metadata_object_vorbiscomment_insert_comment(FLAC__StreamMetadata *object, unsigned comment_num, FLAC__StreamMetadata_VorbisComment_Entry entry, FLAC__bool copy)
{
	FLAC__StreamMetadata_VorbisComment *vc;

	FLAC__ASSERT(0 != object);
	FLAC__ASSERT(object->type == FLAC__METADATA_TYPE_VORBIS_COMMENT);
	FLAC__ASSERT(comment_num <= object->data.vorbis_comment.num_comments);

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
	FLAC__ASSERT(comment_num < object->data.vorbis_comment.num_comments);

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

FLAC_API FLAC__bool FLAC__metadata_object_vorbiscomment_entry_matches(const FLAC__StreamMetadata_VorbisComment_Entry *entry, const char *field_name, unsigned field_name_length)
{
	const FLAC__byte *eq = (FLAC__byte*)memchr(entry->entry, '=', entry->length);
#if defined _MSC_VER || defined __MINGW32__
#define FLAC__STRNCASECMP strnicmp
#else
#define FLAC__STRNCASECMP strncasecmp
#endif
	return (0 != eq && (unsigned)(eq-entry->entry) == field_name_length && 0 == FLAC__STRNCASECMP(field_name, (const char *)entry->entry, field_name_length));
#undef FLAC__STRNCASECMP
}

FLAC_API int FLAC__metadata_object_vorbiscomment_find_entry_from(const FLAC__StreamMetadata *object, unsigned offset, const char *field_name)
{
	const unsigned field_name_length = strlen(field_name);
	unsigned i;

	FLAC__ASSERT(0 != object);
	FLAC__ASSERT(object->type == FLAC__METADATA_TYPE_VORBIS_COMMENT);

	for(i = offset; i < object->data.vorbis_comment.num_comments; i++) {
		if(FLAC__metadata_object_vorbiscomment_entry_matches(object->data.vorbis_comment.comments + i, field_name, field_name_length))
			return (int)i;
	}

	return -1;
}

FLAC_API int FLAC__metadata_object_vorbiscomment_remove_entry_matching(FLAC__StreamMetadata *object, const char *field_name)
{
	const unsigned field_name_length = strlen(field_name);
	unsigned i;

	FLAC__ASSERT(0 != object);
	FLAC__ASSERT(object->type == FLAC__METADATA_TYPE_VORBIS_COMMENT);

	for(i = 0; i < object->data.vorbis_comment.num_comments; i++) {
		if(FLAC__metadata_object_vorbiscomment_entry_matches(object->data.vorbis_comment.comments + i, field_name, field_name_length)) {
			if(!FLAC__metadata_object_vorbiscomment_delete_comment(object, i))
				return -1;
			else
				return 1;
		}
	}

	return 0;
}

FLAC_API int FLAC__metadata_object_vorbiscomment_remove_entries_matching(FLAC__StreamMetadata *object, const char *field_name)
{
	FLAC__bool ok = true;
	unsigned matching = 0;
	const unsigned field_name_length = strlen(field_name);
	int i;

	FLAC__ASSERT(0 != object);
	FLAC__ASSERT(object->type == FLAC__METADATA_TYPE_VORBIS_COMMENT);

	/* must delete from end to start otherwise it will interfere with our iteration */
	for(i = (int)object->data.vorbis_comment.num_comments - 1; ok && i >= 0; i--) {
		if(FLAC__metadata_object_vorbiscomment_entry_matches(object->data.vorbis_comment.comments + i, field_name, field_name_length)) {
			matching++;
			ok &= FLAC__metadata_object_vorbiscomment_delete_comment(object, (unsigned)i);
		}
	}

	return ok? (int)matching : -1;
}

FLAC_API FLAC__StreamMetadata_CueSheet_Track *FLAC__metadata_object_cuesheet_track_new()
{
	return (FLAC__StreamMetadata_CueSheet_Track*)calloc(1, sizeof(FLAC__StreamMetadata_CueSheet_Track));
}

FLAC_API FLAC__StreamMetadata_CueSheet_Track *FLAC__metadata_object_cuesheet_track_clone(const FLAC__StreamMetadata_CueSheet_Track *object)
{
	FLAC__StreamMetadata_CueSheet_Track *to;

	FLAC__ASSERT(0 != object);

	if(0 != (to = FLAC__metadata_object_cuesheet_track_new())) {
		if(!copy_track_(to, object)) {
			FLAC__metadata_object_cuesheet_track_delete(to);
			return 0;
		}
	}

	return to;
}

void FLAC__metadata_object_cuesheet_track_delete_data(FLAC__StreamMetadata_CueSheet_Track *object)
{
	FLAC__ASSERT(0 != object);

	if(0 != object->indices) {
		FLAC__ASSERT(object->num_indices > 0);
		free(object->indices);
	}
}

FLAC_API void FLAC__metadata_object_cuesheet_track_delete(FLAC__StreamMetadata_CueSheet_Track *object)
{
	FLAC__metadata_object_cuesheet_track_delete_data(object);
	free(object);
}

FLAC_API FLAC__bool FLAC__metadata_object_cuesheet_track_resize_indices(FLAC__StreamMetadata *object, unsigned track_num, unsigned new_num_indices)
{
	FLAC__StreamMetadata_CueSheet_Track *track;
	FLAC__ASSERT(0 != object);
	FLAC__ASSERT(object->type == FLAC__METADATA_TYPE_CUESHEET);
	FLAC__ASSERT(track_num < object->data.cue_sheet.num_tracks);

	track = &object->data.cue_sheet.tracks[track_num];

	if(0 == track->indices) {
		FLAC__ASSERT(track->num_indices == 0);
		if(0 == new_num_indices)
			return true;
		else if(0 == (track->indices = cuesheet_track_index_array_new_(new_num_indices)))
			return false;
	}
	else {
		const unsigned old_size = track->num_indices * sizeof(FLAC__StreamMetadata_CueSheet_Index);
		const unsigned new_size = new_num_indices * sizeof(FLAC__StreamMetadata_CueSheet_Index);

		FLAC__ASSERT(track->num_indices > 0);

		if(new_size == 0) {
			free(track->indices);
			track->indices = 0;
		}
		else if(0 == (track->indices = (FLAC__StreamMetadata_CueSheet_Index*)realloc(track->indices, new_size)))
			return false;

		/* if growing, zero all the lengths/pointers of new elements */
		if(new_size > old_size)
			memset(track->indices + track->num_indices, 0, new_size - old_size);
	}

	track->num_indices = new_num_indices;

	cuesheet_calculate_length_(object);
	return true;
}

FLAC_API FLAC__bool FLAC__metadata_object_cuesheet_track_insert_index(FLAC__StreamMetadata *object, unsigned track_num, unsigned index_num, FLAC__StreamMetadata_CueSheet_Index index)
{
	FLAC__StreamMetadata_CueSheet_Track *track;

	FLAC__ASSERT(0 != object);
	FLAC__ASSERT(object->type == FLAC__METADATA_TYPE_CUESHEET);
	FLAC__ASSERT(track_num < object->data.cue_sheet.num_tracks);
	FLAC__ASSERT(index_num <= object->data.cue_sheet.tracks[track_num].num_indices);

	track = &object->data.cue_sheet.tracks[track_num];

	if(!FLAC__metadata_object_cuesheet_track_resize_indices(object, track_num, track->num_indices+1))
		return false;

	/* move all indices >= index_num forward one space */
	memmove(&track->indices[index_num+1], &track->indices[index_num], sizeof(FLAC__StreamMetadata_CueSheet_Index)*(track->num_indices-1-index_num));

	track->indices[index_num] = index;
	cuesheet_calculate_length_(object);
	return true;
}

FLAC_API FLAC__bool FLAC__metadata_object_cuesheet_track_insert_blank_index(FLAC__StreamMetadata *object, unsigned track_num, unsigned index_num)
{
	FLAC__StreamMetadata_CueSheet_Index index;
	memset(&index, 0, sizeof(index));
	return FLAC__metadata_object_cuesheet_track_insert_index(object, track_num, index_num, index);
}

FLAC_API FLAC__bool FLAC__metadata_object_cuesheet_track_delete_index(FLAC__StreamMetadata *object, unsigned track_num, unsigned index_num)
{
	FLAC__StreamMetadata_CueSheet_Track *track;

	FLAC__ASSERT(0 != object);
	FLAC__ASSERT(object->type == FLAC__METADATA_TYPE_CUESHEET);
	FLAC__ASSERT(track_num < object->data.cue_sheet.num_tracks);
	FLAC__ASSERT(index_num < object->data.cue_sheet.tracks[track_num].num_indices);

	track = &object->data.cue_sheet.tracks[track_num];

	/* move all indices > index_num backward one space */
	memmove(&track->indices[index_num], &track->indices[index_num+1], sizeof(FLAC__StreamMetadata_CueSheet_Index)*(track->num_indices-index_num-1));

	FLAC__metadata_object_cuesheet_track_resize_indices(object, track_num, track->num_indices-1);
	cuesheet_calculate_length_(object);
	return true;
}

FLAC_API FLAC__bool FLAC__metadata_object_cuesheet_resize_tracks(FLAC__StreamMetadata *object, unsigned new_num_tracks)
{
	FLAC__ASSERT(0 != object);
	FLAC__ASSERT(object->type == FLAC__METADATA_TYPE_CUESHEET);

	if(0 == object->data.cue_sheet.tracks) {
		FLAC__ASSERT(object->data.cue_sheet.num_tracks == 0);
		if(0 == new_num_tracks)
			return true;
		else if(0 == (object->data.cue_sheet.tracks = cuesheet_track_array_new_(new_num_tracks)))
			return false;
	}
	else {
		const unsigned old_size = object->data.cue_sheet.num_tracks * sizeof(FLAC__StreamMetadata_CueSheet_Track);
		const unsigned new_size = new_num_tracks * sizeof(FLAC__StreamMetadata_CueSheet_Track);

		FLAC__ASSERT(object->data.cue_sheet.num_tracks > 0);

		/* if shrinking, free the truncated entries */
		if(new_num_tracks < object->data.cue_sheet.num_tracks) {
			unsigned i;
			for(i = new_num_tracks; i < object->data.cue_sheet.num_tracks; i++)
				if(0 != object->data.cue_sheet.tracks[i].indices)
					free(object->data.cue_sheet.tracks[i].indices);
		}

		if(new_size == 0) {
			free(object->data.cue_sheet.tracks);
			object->data.cue_sheet.tracks = 0;
		}
		else if(0 == (object->data.cue_sheet.tracks = (FLAC__StreamMetadata_CueSheet_Track*)realloc(object->data.cue_sheet.tracks, new_size)))
			return false;

		/* if growing, zero all the lengths/pointers of new elements */
		if(new_size > old_size)
			memset(object->data.cue_sheet.tracks + object->data.cue_sheet.num_tracks, 0, new_size - old_size);
	}

	object->data.cue_sheet.num_tracks = new_num_tracks;

	cuesheet_calculate_length_(object);
	return true;
}

FLAC_API FLAC__bool FLAC__metadata_object_cuesheet_set_track(FLAC__StreamMetadata *object, unsigned track_num, FLAC__StreamMetadata_CueSheet_Track *track, FLAC__bool copy)
{
	FLAC__ASSERT(0 != object);
	FLAC__ASSERT(track_num < object->data.cue_sheet.num_tracks);

	return cuesheet_set_track_(object, object->data.cue_sheet.tracks + track_num, track, copy);
}

FLAC_API FLAC__bool FLAC__metadata_object_cuesheet_insert_track(FLAC__StreamMetadata *object, unsigned track_num, FLAC__StreamMetadata_CueSheet_Track *track, FLAC__bool copy)
{
	FLAC__StreamMetadata_CueSheet *cs;

	FLAC__ASSERT(0 != object);
	FLAC__ASSERT(object->type == FLAC__METADATA_TYPE_CUESHEET);
	FLAC__ASSERT(track_num <= object->data.cue_sheet.num_tracks);

	cs = &object->data.cue_sheet;

	if(!FLAC__metadata_object_cuesheet_resize_tracks(object, cs->num_tracks+1))
		return false;

	/* move all tracks >= track_num forward one space */
	memmove(&cs->tracks[track_num+1], &cs->tracks[track_num], sizeof(FLAC__StreamMetadata_CueSheet_Track)*(cs->num_tracks-1-track_num));
	cs->tracks[track_num].num_indices = 0;
	cs->tracks[track_num].indices = 0;

	return FLAC__metadata_object_cuesheet_set_track(object, track_num, track, copy);
}

FLAC_API FLAC__bool FLAC__metadata_object_cuesheet_insert_blank_track(FLAC__StreamMetadata *object, unsigned track_num)
{
	FLAC__StreamMetadata_CueSheet_Track track;
	memset(&track, 0, sizeof(track));
	return FLAC__metadata_object_cuesheet_insert_track(object, track_num, &track, /*copy=*/false);
}

FLAC_API FLAC__bool FLAC__metadata_object_cuesheet_delete_track(FLAC__StreamMetadata *object, unsigned track_num)
{
	FLAC__StreamMetadata_CueSheet *cs;

	FLAC__ASSERT(0 != object);
	FLAC__ASSERT(object->type == FLAC__METADATA_TYPE_CUESHEET);
	FLAC__ASSERT(track_num < object->data.cue_sheet.num_tracks);

	cs = &object->data.cue_sheet;

	/* free the track at track_num */
	if(0 != cs->tracks[track_num].indices)
		free(cs->tracks[track_num].indices);

	/* move all tracks > track_num backward one space */
	memmove(&cs->tracks[track_num], &cs->tracks[track_num+1], sizeof(FLAC__StreamMetadata_CueSheet_Track)*(cs->num_tracks-track_num-1));
	cs->tracks[cs->num_tracks-1].num_indices = 0;
	cs->tracks[cs->num_tracks-1].indices = 0;

	return FLAC__metadata_object_cuesheet_resize_tracks(object, cs->num_tracks-1);
}

FLAC_API FLAC__bool FLAC__metadata_object_cuesheet_is_legal(const FLAC__StreamMetadata *object, FLAC__bool check_cd_da_subset, const char **violation)
{
	FLAC__ASSERT(0 != object);
	FLAC__ASSERT(object->type == FLAC__METADATA_TYPE_CUESHEET);

	return FLAC__format_cuesheet_is_legal(&object->data.cue_sheet, check_cd_da_subset, violation);
}
