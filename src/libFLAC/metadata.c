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

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if defined _MSC_VER || defined __MINGW32__
#include <sys/utime.h> /* for utime() */
#include <io.h> /* for chmod() */
#else
#include <sys/types.h> /* some flavors of BSD (like OS X) require this to get time_t */
#include <utime.h> /* for utime() */
#include <unistd.h> /* for chown(), unlink() */
#endif
#include <sys/stat.h> /* for stat(), maybe chmod() */

#include "FLAC/assert.h"
#include "FLAC/metadata.h"
#include "FLAC/file_decoder.h"

#ifdef max
#undef max
#endif
#define max(a,b) ((a)>(b)?(a):(b))
#ifdef min
#undef min
#endif
#define min(a,b) ((a)<(b)?(a):(b))


/****************************************************************************
 *
 * Local function declarations
 *
 ***************************************************************************/

/* this one should probably not go in the public interface: */
static void FLAC__metadata_object_delete_data_(FLAC__StreamMetaData *object);

static void pack_uint32_(FLAC__uint32 val, FLAC__byte *b, unsigned bytes);
static void pack_uint32_little_endian_(FLAC__uint32 val, FLAC__byte *b, unsigned bytes);
static void pack_uint64_(FLAC__uint64 val, FLAC__byte *b, unsigned bytes);
static FLAC__uint32 unpack_uint32_(FLAC__byte *b, unsigned bytes);
static FLAC__uint32 unpack_uint32_little_endian_(FLAC__byte *b, unsigned bytes);
static FLAC__uint64 unpack_uint64_(FLAC__byte *b, unsigned bytes);

static FLAC__bool read_metadata_block_header_(FLAC__MetaData_SimpleIterator *iterator);
static FLAC__bool read_metadata_block_data_(FLAC__MetaData_SimpleIterator *iterator, FLAC__StreamMetaData *block);
static FLAC__MetaData_SimpleIteratorStatus read_metadata_block_data_streaminfo_(FILE *file, FLAC__StreamMetaData_StreamInfo *block);
static FLAC__MetaData_SimpleIteratorStatus read_metadata_block_data_padding_(FILE *file, FLAC__StreamMetaData_Padding *block, unsigned block_length);
static FLAC__MetaData_SimpleIteratorStatus read_metadata_block_data_application_(FILE *file, FLAC__StreamMetaData_Application *block, unsigned block_length);
static FLAC__MetaData_SimpleIteratorStatus read_metadata_block_data_seektable_(FILE *file, FLAC__StreamMetaData_SeekTable *block, unsigned block_length);
static FLAC__MetaData_SimpleIteratorStatus read_metadata_block_data_vorbis_comment_entry_(FILE *file, FLAC__StreamMetaData_VorbisComment_Entry *entry);
static FLAC__MetaData_SimpleIteratorStatus read_metadata_block_data_vorbis_comment_(FILE *file, FLAC__StreamMetaData_VorbisComment *block);

static FLAC__bool write_metadata_block_header_(FILE *file, FLAC__MetaData_SimpleIteratorStatus *status, const FLAC__StreamMetaData *block);
static FLAC__bool write_metadata_block_data_(FILE *file, FLAC__MetaData_SimpleIteratorStatus *status, const FLAC__StreamMetaData *block);
static FLAC__MetaData_SimpleIteratorStatus write_metadata_block_data_streaminfo_(FILE *file, const FLAC__StreamMetaData_StreamInfo *block);
static FLAC__MetaData_SimpleIteratorStatus write_metadata_block_data_padding_(FILE *file, const FLAC__StreamMetaData_Padding *block, unsigned block_length);
static FLAC__MetaData_SimpleIteratorStatus write_metadata_block_data_application_(FILE *file, const FLAC__StreamMetaData_Application *block, unsigned block_length);
static FLAC__MetaData_SimpleIteratorStatus write_metadata_block_data_seektable_(FILE *file, const FLAC__StreamMetaData_SeekTable *block);
static FLAC__MetaData_SimpleIteratorStatus write_metadata_block_data_vorbis_comment_(FILE *file, const FLAC__StreamMetaData_VorbisComment *block);
static FLAC__bool write_metadata_block_stationary_(FLAC__MetaData_SimpleIterator *iterator, const FLAC__StreamMetaData *block);
static FLAC__bool write_metadata_block_stationary_with_padding_(FLAC__MetaData_SimpleIterator *iterator, FLAC__StreamMetaData *block, unsigned padding_length, FLAC__bool padding_is_last);
static FLAC__bool rewrite_whole_file_(FLAC__MetaData_SimpleIterator *iterator, FLAC__StreamMetaData *block, FLAC__bool append);

static FLAC__bool chain_rewrite_chain_(FLAC__MetaData_Chain *chain);
static FLAC__bool chain_rewrite_file_(FLAC__MetaData_Chain *chain, const char *tempfile_path_prefix);

static void simple_iterator_push_(FLAC__MetaData_SimpleIterator *iterator);
static FLAC__bool simple_iterator_pop_(FLAC__MetaData_SimpleIterator *iterator);

/* return 0 if OK, 1 if read error, 2 if not a FLAC file */
static unsigned seek_to_first_metadata_block_(FILE *f);

static FLAC__bool simple_iterator_copy_file_prefix_(FLAC__MetaData_SimpleIterator *iterator, FILE **tempfile, char **tempfilename, FLAC__bool append);
static FLAC__bool simple_iterator_copy_file_postfix_(FLAC__MetaData_SimpleIterator *iterator, FILE **tempfile, char **tempfilename, long fixup_is_last_flag_offset, FLAC__bool backup);

static FLAC__bool copy_n_bytes_from_file_(FILE *file, FILE *tempfile, unsigned bytes/*@@@ 4G limit*/, FLAC__MetaData_SimpleIteratorStatus *status);
static FLAC__bool copy_remaining_bytes_from_file_(FILE *file, FILE *tempfile, FLAC__MetaData_SimpleIteratorStatus *status);

static FLAC__bool open_tempfile_(const char *filename, const char *tempfile_path_prefix, FILE **tempfile, char **tempfilename, FLAC__MetaData_SimpleIteratorStatus *status);
static FLAC__bool transport_tempfile_(const char *filename, FILE **tempfile, char **tempfilename, FLAC__MetaData_SimpleIteratorStatus *status);
static void cleanup_tempfile_(FILE **tempfile, char **tempfilename);

static FLAC__bool get_file_stats_(const char *filename, struct stat *stats);
static void set_file_stats_(const char *filename, struct stat *stats);

static FLAC__MetaData_ChainStatus get_equivalent_status_(FLAC__MetaData_SimpleIteratorStatus status);


/****************************************************************************
 *
 * Level 0 implementation
 *
 ***************************************************************************/

static FLAC__StreamDecoderWriteStatus write_callback_(const FLAC__FileDecoder *decoder, const FLAC__Frame *frame, const FLAC__int32 *buffer[], void *client_data);
static void metadata_callback_(const FLAC__FileDecoder *decoder, const FLAC__StreamMetaData *metadata, void *client_data);
static void error_callback_(const FLAC__FileDecoder *decoder, FLAC__StreamDecoderErrorStatus status, void *client_data);

FLAC__bool FLAC__metadata_get_streaminfo(const char *filename, FLAC__StreamMetaData_StreamInfo *streaminfo)
{
	FLAC__FileDecoder *decoder = FLAC__file_decoder_new();

	if(0 == decoder)
		return false;

	FLAC__file_decoder_set_md5_checking(decoder, false);
	FLAC__file_decoder_set_filename(decoder, filename);
	FLAC__file_decoder_set_write_callback(decoder, write_callback_);
	FLAC__file_decoder_set_metadata_callback(decoder, metadata_callback_);
	FLAC__file_decoder_set_error_callback(decoder, error_callback_);
	FLAC__file_decoder_set_client_data(decoder, &streaminfo);

	if(FLAC__file_decoder_init(decoder) != FLAC__FILE_DECODER_OK)
		return false;

	if(!FLAC__file_decoder_process_metadata(decoder))
		return false;

	if(FLAC__file_decoder_get_state(decoder) != FLAC__FILE_DECODER_UNINITIALIZED)
		FLAC__file_decoder_finish(decoder);

	FLAC__file_decoder_delete(decoder);

	return 0 != streaminfo; /* the metadata_callback_() will set streaminfo to 0 on an error */
}

FLAC__StreamDecoderWriteStatus write_callback_(const FLAC__FileDecoder *decoder, const FLAC__Frame *frame, const FLAC__int32 *buffer[], void *client_data)
{
	(void)decoder, (void)frame, (void)buffer, (void)client_data;

	return FLAC__STREAM_DECODER_WRITE_CONTINUE;
}

void metadata_callback_(const FLAC__FileDecoder *decoder, const FLAC__StreamMetaData *metadata, void *client_data)
{
	FLAC__StreamMetaData_StreamInfo **streaminfo = (FLAC__StreamMetaData_StreamInfo **)client_data;
	(void)decoder;

	if(metadata->type == FLAC__METADATA_TYPE_STREAMINFO && 0 != *streaminfo)
		**streaminfo = metadata->data.stream_info;
}

void error_callback_(const FLAC__FileDecoder *decoder, FLAC__StreamDecoderErrorStatus status, void *client_data)
{
	FLAC__StreamMetaData_StreamInfo **streaminfo = (FLAC__StreamMetaData_StreamInfo **)client_data;
	(void)decoder;

	if(status != FLAC__STREAM_DECODER_ERROR_LOST_SYNC)
		*streaminfo = 0;
}


/****************************************************************************
 *
 * Level 1 implementation
 *
 ***************************************************************************/

#define SIMPLE_ITERATOR_MAX_PUSH_DEPTH (1+4)
/* 1 for initial offset, +4 for our own personal use */

struct FLAC__MetaData_SimpleIterator {
	FILE *file;
	char *filename, *tempfile_path_prefix;
	struct stat stats;
	FLAC__bool has_stats;
	FLAC__bool is_writable;
	FLAC__MetaData_SimpleIteratorStatus status;
	/*@@@ 2G limits here because of the offset type: */
	long offset[SIMPLE_ITERATOR_MAX_PUSH_DEPTH];
	long first_offset; /* this is the offset to the STREAMINFO block */
	unsigned depth;
	/* this is the metadata block header of the current block we are pointing to: */
	FLAC__bool is_last;
	FLAC__MetaDataType type;
	unsigned length;
};

const char *FLAC__MetaData_SimpleIteratorStatusString[] = {
	"FLAC__METADATA_SIMPLE_ITERATOR_STATUS_OK",
	"FLAC__METADATA_SIMPLE_ITERATOR_STATUS_ILLEGAL_INPUT",
	"FLAC__METADATA_SIMPLE_ITERATOR_STATUS_ERROR_OPENING_FILE",
	"FLAC__METADATA_SIMPLE_ITERATOR_STATUS_NOT_A_FLAC_FILE",
	"FLAC__METADATA_SIMPLE_ITERATOR_STATUS_NOT_WRITABLE",
	"FLAC__METADATA_SIMPLE_ITERATOR_STATUS_READ_ERROR",
	"FLAC__METADATA_SIMPLE_ITERATOR_STATUS_SEEK_ERROR",
	"FLAC__METADATA_SIMPLE_ITERATOR_STATUS_WRITE_ERROR",
	"FLAC__METADATA_SIMPLE_ITERATOR_STATUS_RENAME_ERROR",
	"FLAC__METADATA_SIMPLE_ITERATOR_STATUS_UNLINK_ERROR",
	"FLAC__METADATA_SIMPLE_ITERATOR_STATUS_MEMORY_ALLOCATION_ERROR",
	"FLAC__METADATA_SIMPLE_ITERATOR_STATUS_INTERNAL_ERROR"
};


FLAC__MetaData_SimpleIterator *FLAC__metadata_simple_iterator_new()
{
	FLAC__MetaData_SimpleIterator *iterator = malloc(sizeof(FLAC__MetaData_SimpleIterator));

	if(0 != iterator) {
		iterator->file = 0;
		iterator->filename = 0;
		iterator->tempfile_path_prefix = 0;
		iterator->has_stats = false;
		iterator->is_writable = false;
		iterator->status = FLAC__METADATA_SIMPLE_ITERATOR_STATUS_OK;
		iterator->first_offset = iterator->offset[0] = -1;
		iterator->depth = 0;
	}

	return iterator;
}

static void simple_iterator_free_guts_(FLAC__MetaData_SimpleIterator *iterator)
{
	FLAC__ASSERT(0 != iterator);

	if(0 != iterator->file) {
		fclose(iterator->file);
		iterator->file = 0;
		if(iterator->has_stats)
			set_file_stats_(iterator->filename, &iterator->stats);
	}
	if(0 != iterator->filename) {
		free(iterator->filename);
		iterator->filename = 0;
	}
	if(0 != iterator->tempfile_path_prefix) {
		free(iterator->tempfile_path_prefix);
		iterator->tempfile_path_prefix = 0;
	}
}

void FLAC__metadata_simple_iterator_delete(FLAC__MetaData_SimpleIterator *iterator)
{
	FLAC__ASSERT(0 != iterator);

	simple_iterator_free_guts_(iterator);
	free(iterator);
}

FLAC__MetaData_SimpleIteratorStatus FLAC__metadata_simple_iterator_status(FLAC__MetaData_SimpleIterator *iterator)
{
	FLAC__MetaData_SimpleIteratorStatus status;

	FLAC__ASSERT(0 != iterator);

	status = iterator->status;
	iterator->status = FLAC__METADATA_SIMPLE_ITERATOR_STATUS_OK;
	return status;
}

static FLAC__bool simple_iterator_prime_input_(FLAC__MetaData_SimpleIterator *iterator)
{
	unsigned ret;

	FLAC__ASSERT(0 != iterator);

	iterator->is_writable = false;

	if(0 == (iterator->file = fopen(iterator->filename, "r+b"))) {
		if(errno == EACCES) {
			if(0 == (iterator->file = fopen(iterator->filename, "rb"))) {
				iterator->status = FLAC__METADATA_SIMPLE_ITERATOR_STATUS_ERROR_OPENING_FILE;
				return false;
			}
		}
		else {
			iterator->status = FLAC__METADATA_SIMPLE_ITERATOR_STATUS_ERROR_OPENING_FILE;
			return false;
		}
	}
	else {
		iterator->is_writable = true;
	}

	ret = seek_to_first_metadata_block_(iterator->file);
	switch(ret) {
		case 0:
			iterator->depth = 0;
			iterator->first_offset = iterator->offset[iterator->depth] = ftell(iterator->file);
			return read_metadata_block_header_(iterator);
		case 1:
			iterator->status = FLAC__METADATA_SIMPLE_ITERATOR_STATUS_READ_ERROR;
			return false;
		case 2:
			iterator->status = FLAC__METADATA_SIMPLE_ITERATOR_STATUS_NOT_A_FLAC_FILE;
			return false;
		default:
			FLAC__ASSERT(0);
			return false;
	}
}

#if 0
@@@ If we decide to finish implementing this, put this comment back in metadata.h
/*
 * The 'tempfile_path_prefix' allows you to specify a directory where
 * tempfiles should go.  Remember that if your metadata edits cause the
 * FLAC file to grow, the entire file will have to be rewritten.  If
 * 'tempfile_path_prefix' is NULL, the temp file will be written in the
 * same directory as the original FLAC file.  This makes replacing the
 * original with the tempfile fast but requires extra space in the same
 * partition for the tempfile.  If space is a problem, you can pass a
 * directory name belonging to a different partition in
 * 'tempfile_path_prefix'.  Note that you should use the forward slash
 * '/' as the directory separator.  A trailing slash is not needed; it
 * will be added automatically.
 */
FLAC__bool FLAC__metadata_simple_iterator_init(FLAC__MetaData_SimpleIterator *iterator, const char *filename, FLAC__bool preserve_file_stats, const char *tempfile_path_prefix);
#endif

FLAC__bool FLAC__metadata_simple_iterator_init(FLAC__MetaData_SimpleIterator *iterator, const char *filename, FLAC__bool preserve_file_stats)
{
	const char *tempfile_path_prefix = 0; /*@@@ search for comments near 'rename(' for what it will take to finish implementing this */

	FLAC__ASSERT(0 != iterator);
	FLAC__ASSERT(0 != filename);

	simple_iterator_free_guts_(iterator);

	if(preserve_file_stats)
		iterator->has_stats = get_file_stats_(filename, &iterator->stats);

	if(0 == (iterator->filename = strdup(filename))) {
		iterator->status = FLAC__METADATA_SIMPLE_ITERATOR_STATUS_MEMORY_ALLOCATION_ERROR;
		return false;
	}
	if(0 != tempfile_path_prefix && 0 == (iterator->tempfile_path_prefix = strdup(tempfile_path_prefix))) {
		iterator->status = FLAC__METADATA_SIMPLE_ITERATOR_STATUS_MEMORY_ALLOCATION_ERROR;
		return false;
	}

	return simple_iterator_prime_input_(iterator);
}

FLAC__bool FLAC__metadata_simple_iterator_is_writable(const FLAC__MetaData_SimpleIterator *iterator)
{
	FLAC__ASSERT(0 != iterator);
	FLAC__ASSERT(0 != iterator->file);

	return iterator->is_writable;
}

FLAC__bool FLAC__metadata_simple_iterator_next(FLAC__MetaData_SimpleIterator *iterator)
{
	FLAC__ASSERT(0 != iterator);
	FLAC__ASSERT(0 != iterator->file);

	if(iterator->is_last)
		return false;

	if(0 != fseek(iterator->file, iterator->length, SEEK_CUR)) {
		iterator->status = FLAC__METADATA_SIMPLE_ITERATOR_STATUS_SEEK_ERROR;
		return false;
	}

	iterator->offset[iterator->depth] = ftell(iterator->file);

	return read_metadata_block_header_(iterator);
}

FLAC__bool FLAC__metadata_simple_iterator_prev(FLAC__MetaData_SimpleIterator *iterator)
{
	long this_offset;

	FLAC__ASSERT(0 != iterator);
	FLAC__ASSERT(0 != iterator->file);

	if(iterator->offset[iterator->depth] == iterator->first_offset)
		return false;

	if(0 != fseek(iterator->file, iterator->first_offset, SEEK_SET)) {
		iterator->status = FLAC__METADATA_SIMPLE_ITERATOR_STATUS_SEEK_ERROR;
		return false;
	}
	this_offset = iterator->first_offset;
	if(!read_metadata_block_header_(iterator))
		return false;

	/* we ignore any error from ftell() and catch it in fseek() */
	while(ftell(iterator->file) + (long)iterator->length < iterator->offset[iterator->depth]) {
		if(0 != fseek(iterator->file, iterator->length, SEEK_CUR)) {
			iterator->status = FLAC__METADATA_SIMPLE_ITERATOR_STATUS_SEEK_ERROR;
			return false;
		}
		this_offset = ftell(iterator->file);
		if(!read_metadata_block_header_(iterator))
			return false;
	}

	iterator->offset[iterator->depth] = this_offset;

	return true;
}

FLAC__MetaDataType FLAC__metadata_simple_iterator_get_block_type(const FLAC__MetaData_SimpleIterator *iterator)
{
	FLAC__ASSERT(0 != iterator);
	FLAC__ASSERT(0 != iterator->file);

	return iterator->type;
}

FLAC__StreamMetaData *FLAC__metadata_simple_iterator_get_block(FLAC__MetaData_SimpleIterator *iterator)
{
	FLAC__StreamMetaData *block = FLAC__metadata_object_new(iterator->type);

	FLAC__ASSERT(0 != iterator);
	FLAC__ASSERT(0 != iterator->file);

	if(0 != block) {
		block->is_last = iterator->is_last;
		block->length = iterator->length;

		if(!read_metadata_block_data_(iterator, block)) {
			FLAC__metadata_object_delete(block);
			return 0;
		}

		/* back up to the beginning of the block data to stay consistent */
		if(0 != fseek(iterator->file, iterator->offset[iterator->depth] + 4, SEEK_SET)) { /*@@@ 4 = MAGIC NUMBER for metadata block header bytes */
			iterator->status = FLAC__METADATA_SIMPLE_ITERATOR_STATUS_SEEK_ERROR;
			FLAC__metadata_object_delete(block);
			return 0;
		}
	}
	else
		iterator->status = FLAC__METADATA_SIMPLE_ITERATOR_STATUS_MEMORY_ALLOCATION_ERROR;

	return block;
}

FLAC__bool FLAC__metadata_simple_iterator_set_block(FLAC__MetaData_SimpleIterator *iterator, FLAC__StreamMetaData *block, FLAC__bool use_padding)
{
	FLAC__ASSERT(0 != iterator);
	FLAC__ASSERT(0 != iterator->file);

	if(!iterator->is_writable) {
		iterator->status = FLAC__METADATA_SIMPLE_ITERATOR_STATUS_NOT_WRITABLE;
		return false;
	}

	if(iterator->type == FLAC__METADATA_TYPE_STREAMINFO || block->type == FLAC__METADATA_TYPE_STREAMINFO) {
		if(iterator->type != block->type) {
			iterator->status = FLAC__METADATA_SIMPLE_ITERATOR_STATUS_ILLEGAL_INPUT;
			return false;
		}
	}

	block->is_last = iterator->is_last;
	//@@@@FLAC__metadata_object_calculate_length(block);

	if(iterator->length == block->length)
		return write_metadata_block_stationary_(iterator, block);
	else if(iterator->length > block->length) {
		if(use_padding && iterator->length >= 4 + block->length) /*@@@ 4 = MAGIC NUMBER for metadata block header bytes */
			return write_metadata_block_stationary_with_padding_(iterator, block, iterator->length - block->length - 4, block->is_last);
		else
			return rewrite_whole_file_(iterator, block, /*append=*/false);
	}
	else /* iterator->length < block->length */ {
		unsigned padding_leftover = 0;
		FLAC__bool padding_is_last = false;
		if(use_padding) {
			/* first see if we can even use padding */
			if(iterator->is_last) {
				use_padding = false;
			}
			else {
				unsigned existing_block_length = iterator->length;
				simple_iterator_push_(iterator);
				if(!FLAC__metadata_simple_iterator_next(iterator)) {
					(void)simple_iterator_pop_(iterator);
					return false;
				}
				if(iterator->type != FLAC__METADATA_TYPE_PADDING) {
					use_padding = false;
				}
				else {
					/*@@@ MAGIC NUMBER 4 = metadata block header length, appears 3 times here: */
					const unsigned available_size = existing_block_length + 4 + iterator->length;
					if(available_size == block->length) {
						padding_leftover = 0;
						block->is_last = iterator->is_last;
					}
					else if(available_size - 4 < block->length)
						use_padding = false;
					else {
						padding_leftover = available_size - 4 - block->length;
						padding_is_last = iterator->is_last;
						block->is_last = false;
					}
				}
				if(!simple_iterator_pop_(iterator))
					return false;
			}
		}
		if(use_padding) {
			if(padding_leftover == 0)
				return write_metadata_block_stationary_(iterator, block);
			else
				return write_metadata_block_stationary_with_padding_(iterator, block, padding_leftover, padding_is_last);
		}
		else {
			return rewrite_whole_file_(iterator, block, /*append=*/false);
		}
	}
}

FLAC__bool FLAC__metadata_simple_iterator_insert_block_after(FLAC__MetaData_SimpleIterator *iterator, FLAC__StreamMetaData *block, FLAC__bool use_padding)
{
	unsigned padding_leftover = 0;
	FLAC__bool padding_is_last = false;

	FLAC__ASSERT(0 != iterator);
	FLAC__ASSERT(0 != iterator->file);

	if(!iterator->is_writable)
		return false;

	if(block->type == FLAC__METADATA_TYPE_STREAMINFO) {
		iterator->status = FLAC__METADATA_SIMPLE_ITERATOR_STATUS_ILLEGAL_INPUT;
		return false;
	}

	if(use_padding) {
		/* first see if we can even use padding */
		if(iterator->is_last) {
			use_padding = false;
		}
		else {
			simple_iterator_push_(iterator);
			if(!FLAC__metadata_simple_iterator_next(iterator)) {
				(void)simple_iterator_pop_(iterator);
				return false;
			}
			if(iterator->type != FLAC__METADATA_TYPE_PADDING) {
				use_padding = false;
			}
			else {
				/*@@@ MAGIC NUMBER 4 = metadata block header length, appears 2 times here: */
				if(iterator->length == block->length) {
					padding_leftover = 0;
					block->is_last = iterator->is_last;
				}
				else if(iterator->length < 4 + block->length)
					use_padding = false;
				else {
					padding_leftover = iterator->length - 4 - block->length;
					padding_is_last = iterator->is_last;
					block->is_last = false;
				}
			}
			if(!simple_iterator_pop_(iterator))
				return false;
		}
	}
	if(use_padding) {
		/* move to the next block, which is suitable padding */
		if(!FLAC__metadata_simple_iterator_next(iterator))
			return false;
		if(padding_leftover == 0)
			return write_metadata_block_stationary_(iterator, block);
		else
			return write_metadata_block_stationary_with_padding_(iterator, block, padding_leftover, padding_is_last);
	}
	else {
		return rewrite_whole_file_(iterator, block, /*append=*/true);
	}
}

/*
 * Deletes the block at the current position.  This will cause the
 * entire FLAC file to be rewritten, unless 'use_padding' is true,
 * in which case the block will be replaced by an equal-sized PADDING
 * block.
 *
 * You may not delete the STREAMINFO block
 */
FLAC__bool FLAC__metadata_simple_iterator_delete_block(FLAC__MetaData_SimpleIterator *iterator, FLAC__bool use_padding)
{
	if(iterator->type == FLAC__METADATA_TYPE_STREAMINFO) {
		iterator->status = FLAC__METADATA_SIMPLE_ITERATOR_STATUS_ILLEGAL_INPUT;
		return false;
	}

	if(use_padding) {
		FLAC__StreamMetaData *padding = FLAC__metadata_object_new(FLAC__METADATA_TYPE_PADDING);
		if(0 == padding) {
			iterator->status = FLAC__METADATA_SIMPLE_ITERATOR_STATUS_MEMORY_ALLOCATION_ERROR;
			return false;
		}
		padding->length = iterator->length;
		if(!FLAC__metadata_simple_iterator_set_block(iterator, padding, false)) {
			FLAC__metadata_object_delete(padding);
			return false;
		}
		FLAC__metadata_object_delete(padding);
		return true;
	}
	else {
		return rewrite_whole_file_(iterator, 0, /*append=*/false);
	}
}



/****************************************************************************
 *
 * Level 2 implementation
 *
 ***************************************************************************/


typedef struct FLAC__MetaData_Node {
	FLAC__StreamMetaData *data;
	struct FLAC__MetaData_Node *prev, *next;
} FLAC__MetaData_Node;

struct FLAC__MetaData_Chain {
	char *filename;
	FLAC__MetaData_Node *head;
	FLAC__MetaData_Node *tail;
	unsigned nodes;
	FLAC__MetaData_ChainStatus status;
	long first_offset, last_offset; /*@@@ 2G limit */
	/*
	 * This is the length of the chain initially read from the FLAC file.
	 * it is used to compare against the current_length to decide whether
	 * or not the whole file has to be rewritten.
	 */
	unsigned initial_length; /*@@@ 4G limit */
	unsigned current_length; /*@@@ 4G limit */
};

struct FLAC__MetaData_Iterator {
	FLAC__MetaData_Chain *chain;
	FLAC__MetaData_Node *current;
};

const char *FLAC__MetaData_ChainStatusString[] = {
	"FLAC__METADATA_CHAIN_STATUS_OK",
	"FLAC__METADATA_CHAIN_STATUS_ILLEGAL_INPUT",
	"FLAC__METADATA_CHAIN_STATUS_ERROR_OPENING_FILE",
	"FLAC__METADATA_CHAIN_STATUS_NOT_A_FLAC_FILE",
	"FLAC__METADATA_CHAIN_STATUS_NOT_WRITABLE",
	"FLAC__METADATA_CHAIN_STATUS_READ_ERROR",
	"FLAC__METADATA_CHAIN_STATUS_SEEK_ERROR",
	"FLAC__METADATA_CHAIN_STATUS_WRITE_ERROR",
	"FLAC__METADATA_CHAIN_STATUS_RENAME_ERROR",
	"FLAC__METADATA_CHAIN_STATUS_UNLINK_ERROR",
	"FLAC__METADATA_CHAIN_STATUS_MEMORY_ALLOCATION_ERROR",
	"FLAC__METADATA_CHAIN_STATUS_INTERNAL_ERROR"
};


static FLAC__MetaData_Node *node_new_()
{
	FLAC__MetaData_Node *node = (FLAC__MetaData_Node*)malloc(sizeof(FLAC__MetaData_Node));
	if(0 != node)
		memset(node, 0, sizeof(FLAC__MetaData_Node));
	return node;
}

static void node_delete_(FLAC__MetaData_Node *node)
{
	FLAC__ASSERT(0 != node);
	if(0 != node->data)
		FLAC__metadata_object_delete(node->data);
	free(node);
}

static void chain_append_node_(FLAC__MetaData_Chain *chain, FLAC__MetaData_Node *node)
{
	FLAC__ASSERT(0 != chain);
	FLAC__ASSERT(0 != node);

	node->data->is_last = true;
	if(0 != chain->tail)
		chain->tail->data->is_last = false;

	if(0 == chain->head)
		chain->head = node;
	else {
		chain->tail->next = node;
		node->prev = chain->tail;
	}
	chain->tail = node;
	chain->nodes++;
	chain->current_length += (4 + node->data->length); /*@@@ MAGIC NUMBER 4 = metadata block header bytes */
}

static void chain_remove_node_(FLAC__MetaData_Chain *chain, FLAC__MetaData_Node *node)
{
	FLAC__ASSERT(0 != chain);
	FLAC__ASSERT(0 != node);

	if(node == chain->head)
		chain->head = node->next;
	else
		node->prev->next = node->next;

	if(node == chain->tail)
		chain->tail = node->prev;
	else
		node->next->prev = node->prev;

	chain->nodes--;
	chain->current_length -= (4 + node->data->length); /*@@@ MAGIC NUMBER 4 = metadata block header bytes */
}

static void chain_delete_node_(FLAC__MetaData_Chain *chain, FLAC__MetaData_Node *node)
{
	chain_remove_node_(chain, node);
	node_delete_(node);
}

static void iterator_insert_node_(FLAC__MetaData_Iterator *iterator, FLAC__MetaData_Node *node)
{
	FLAC__ASSERT(0 != node);
	FLAC__ASSERT(0 != iterator);
	FLAC__ASSERT(0 != iterator->current);
	FLAC__ASSERT(0 != iterator->chain);
	FLAC__ASSERT(0 != iterator->chain->head);
	FLAC__ASSERT(0 != iterator->chain->tail);

	node->prev = iterator->current->prev;
	if(0 == node->prev)
		iterator->chain->head = node;
	else
		node->prev->next = node;

	node->next = iterator->current;
	iterator->current->prev = node;

	iterator->chain->nodes++;
	iterator->chain->current_length += (4 + node->data->length); /*@@@ MAGIC NUMBER 4 = metadata block header bytes */
}

static void iterator_insert_node_after_(FLAC__MetaData_Iterator *iterator, FLAC__MetaData_Node *node)
{
	FLAC__ASSERT(0 != node);
	FLAC__ASSERT(0 != iterator);
	FLAC__ASSERT(0 != iterator->current);
	FLAC__ASSERT(0 != iterator->chain);
	FLAC__ASSERT(0 != iterator->chain->head);
	FLAC__ASSERT(0 != iterator->chain->tail);

	iterator->current->data->is_last = false;

	node->prev = iterator->current;
	iterator->current->next = node;

	node->next = iterator->current->next;
	if(0 == node->next)
		iterator->chain->tail = node;
	else
		node->next->prev = node;

	iterator->chain->tail->data->is_last = true;

	iterator->chain->nodes++;
	iterator->chain->current_length += (4 + node->data->length); /*@@@ MAGIC NUMBER 4 = metadata block header bytes */
}

FLAC__MetaData_Chain *FLAC__metadata_chain_new()
{
	FLAC__MetaData_Chain *chain = malloc(sizeof(FLAC__MetaData_Chain));

	if(0 != chain) {
		chain->filename = 0;
		chain->head = chain->tail = 0;
		chain->nodes = 0;
		chain->status = FLAC__METADATA_CHAIN_STATUS_OK;
		chain->initial_length = chain->current_length = 0;
	}

	return chain;
}

void FLAC__metadata_chain_delete(FLAC__MetaData_Chain *chain)
{
	FLAC__MetaData_Node *node, *next;

	FLAC__ASSERT(0 != chain);

	for(node = chain->head; node; ) {
		next = node->next;
		node_delete_(node);
		node = next;
	}

	if(0 != chain->filename)
		free(chain->filename);

	free(chain);
}

FLAC__MetaData_ChainStatus FLAC__metadata_chain_status(FLAC__MetaData_Chain *chain)
{
	FLAC__MetaData_ChainStatus status;

	FLAC__ASSERT(0 != chain);

	status = chain->status;
	chain->status = FLAC__METADATA_CHAIN_STATUS_OK;
	return status;
}

FLAC__bool FLAC__metadata_chain_read(FLAC__MetaData_Chain *chain, const char *filename)
{
	FLAC__MetaData_SimpleIterator *iterator;
	FLAC__MetaData_Node *node;

	FLAC__ASSERT(0 != chain);
	FLAC__ASSERT(0 != filename);

	if(0 == (chain->filename = strdup(filename))) {
		chain->status = FLAC__METADATA_CHAIN_STATUS_MEMORY_ALLOCATION_ERROR;
		return false;
	}

	if(0 == (iterator = FLAC__metadata_simple_iterator_new())) {
		chain->status = FLAC__METADATA_CHAIN_STATUS_MEMORY_ALLOCATION_ERROR;
		return false;
	}

	if(!FLAC__metadata_simple_iterator_init(iterator, filename, /*preserve_file_stats=*/false)) {
		chain->status = get_equivalent_status_(iterator->status);
		return false;
	}

	do {
		node = node_new_();
		if(0 == node) {
			chain->status = FLAC__METADATA_CHAIN_STATUS_MEMORY_ALLOCATION_ERROR;
			return false;
		}
		node->data = FLAC__metadata_simple_iterator_get_block(iterator);
		if(0 == node->data) {
			node_delete_(node);
			chain->status = get_equivalent_status_(iterator->status);
			return false;
		}
		chain_append_node_(chain, node);
	} while(FLAC__metadata_simple_iterator_next(iterator));

	if(!iterator->is_last || iterator->status != FLAC__METADATA_SIMPLE_ITERATOR_STATUS_OK) {
		chain->status = get_equivalent_status_(iterator->status);
		return false;
	}

	chain->last_offset = ftell(iterator->file);
	FLAC__metadata_simple_iterator_delete(iterator);

	chain->initial_length = chain->current_length;
	return true;
}

/*
 * Write all metadata out to the FLAC file.  This function tries to be as
 * efficient as possible; how the metadata is actually written is shown by
 * the following:
 *
 * If the current chain is the same size as the existing metadata, the new
 * data is written in place.
 *
 * If the current chain is longer than the existing metadata, the entire
 * FLAC file must be rewritten.
 *
 * If the current chain is shorted than the existing metadata, and
 * use_padding is true, a PADDING block is added to the end of the new
 * data to make it the same size as the existing data (if possible, see
 * the note to FLAC__metadata_simple_iterator_set_block() about the four
 * byte limit) and the new data is written in place.  If use_padding is
 * false, the entire FLAC file is rewritten.
 */
FLAC__bool FLAC__metadata_chain_write(FLAC__MetaData_Chain *chain, FLAC__bool use_padding, FLAC__bool preserve_file_stats)
{
	struct stat stats;
	const char *tempfile_path_prefix = 0;

	if(use_padding && chain->current_length + 4 <= chain->initial_length) {
		FLAC__StreamMetaData *padding;
		FLAC__MetaData_Node *node;
		if(0 == (padding = FLAC__metadata_object_new(FLAC__METADATA_TYPE_PADDING))) {
			chain->status = FLAC__METADATA_CHAIN_STATUS_MEMORY_ALLOCATION_ERROR;
			return false;
		}
		padding->length = chain->initial_length - (4 + chain->current_length);
		if(0 == (node = node_new_())) {
			FLAC__metadata_object_delete(padding);
			chain->status = FLAC__METADATA_CHAIN_STATUS_MEMORY_ALLOCATION_ERROR;
			return false;
		}
		node->data = padding;
		chain_append_node_(chain, node);
	}

	if(preserve_file_stats)
		get_file_stats_(chain->filename, &stats);

	if(chain->current_length == chain->initial_length) {
		if(!chain_rewrite_chain_(chain))
			return false;
	}
	else {
		if(!chain_rewrite_file_(chain, tempfile_path_prefix))
			return false;
	}

	if(preserve_file_stats)
		set_file_stats_(chain->filename, &stats);

	chain->initial_length = chain->current_length;
	return true;
}

void FLAC__metadata_chain_merge_padding(FLAC__MetaData_Chain *chain)
{
	FLAC__MetaData_Node *node;

	FLAC__ASSERT(0 != chain);

	for(node = chain->head; node; ) {
		if(node->data->type == FLAC__METADATA_TYPE_PADDING && 0 != node->next && node->next->data->type == FLAC__METADATA_TYPE_PADDING) {
			node->data->length += (4 + node->next->data->length); /*@@@ MAGIC NUMBER 4 = metadata data block header bytes */
			chain_delete_node_(chain, node->next);
		}
		else {
			node = node->next;
		}
	}
	if(0 != chain->tail)
		chain->tail->data->is_last = true;
}

void FLAC__metadata_chain_sort_padding(FLAC__MetaData_Chain *chain)
{
	FLAC__MetaData_Node *node, *save;
	unsigned i;

	FLAC__ASSERT(0 != chain);

	/*
	 * Don't try and be too smart... this simple algo is good enough for
	 * the small number of nodes that we deal with.
	 */
	for(i = 0, node = chain->head; i < chain->nodes; i++) {
		node->data->is_last = false; /* we'll fix at the end */
		if(node->data->type == FLAC__METADATA_TYPE_PADDING) {
			save = node->next;
			chain_remove_node_(chain, node);
			chain_append_node_(chain, node);
			node = save;
		}
		else {
			node = node->next;
		}
	}
	if(0 != chain->tail)
		chain->tail->data->is_last = true;

	FLAC__metadata_chain_merge_padding(chain);
}


FLAC__MetaData_Iterator *FLAC__metadata_iterator_new()
{
	FLAC__MetaData_Iterator *iterator = malloc(sizeof(FLAC__MetaData_Iterator));

	if(0 != iterator) {
		iterator->current = 0;
		iterator->chain = 0;
	}

	return iterator;
}

void FLAC__metadata_iterator_delete(FLAC__MetaData_Iterator *iterator)
{
	FLAC__ASSERT(0 != iterator);

	free(iterator);
}

/*
 * Initialize the iterator to point to the first metadata block in the
 * given chain.
 */
void FLAC__metadata_iterator_init(FLAC__MetaData_Iterator *iterator, FLAC__MetaData_Chain *chain)
{
	FLAC__ASSERT(0 != iterator);
	FLAC__ASSERT(0 != chain);
	FLAC__ASSERT(0 != chain->head);

	iterator->chain = chain;
	iterator->current = chain->head;
}

FLAC__bool FLAC__metadata_iterator_next(FLAC__MetaData_Iterator *iterator)
{
	FLAC__ASSERT(0 != iterator);

	if(0 == iterator->current || 0 == iterator->current->next)
		return false;

	iterator->current = iterator->current->next;
	return true;
}

FLAC__bool FLAC__metadata_iterator_prev(FLAC__MetaData_Iterator *iterator)
{
	FLAC__ASSERT(0 != iterator);

	if(0 == iterator->current || 0 == iterator->current->prev)
		return false;

	iterator->current = iterator->current->prev;
	return true;
}

FLAC__MetaDataType FLAC__metadata_iterator_get_block_type(const FLAC__MetaData_Iterator *iterator)
{
	FLAC__ASSERT(0 != iterator);
	FLAC__ASSERT(0 != iterator->current);

	return iterator->current->data->type;
}

FLAC__StreamMetaData *FLAC__metadata_iterator_get_block(FLAC__MetaData_Iterator *iterator)
{
	FLAC__ASSERT(0 != iterator);
	FLAC__ASSERT(0 != iterator->current);

	return iterator->current->data;
}

FLAC__bool FLAC__metadata_iterator_delete_block(FLAC__MetaData_Iterator *iterator, FLAC__bool replace_with_padding)
{
	FLAC__MetaData_Node *save;

	FLAC__ASSERT(0 != iterator);
	FLAC__ASSERT(0 != iterator->current);

	if(0 == iterator->current->prev) {
		FLAC__ASSERT(iterator->current->data->type == FLAC__METADATA_TYPE_STREAMINFO);
		return false;
	}

	save = iterator->current->prev;

	if(replace_with_padding) {
		FLAC__metadata_object_delete_data_(iterator->current->data);
		iterator->current->data->type = FLAC__METADATA_TYPE_PADDING;
	}
	else {
		chain_delete_node_(iterator->chain, iterator->current);
	}

	iterator->current = save;
	return true;
}

FLAC__bool FLAC__metadata_iterator_insert_block_before(FLAC__MetaData_Iterator *iterator, FLAC__StreamMetaData *block)
{
	FLAC__MetaData_Node *node;

	FLAC__ASSERT(0 != iterator);
	FLAC__ASSERT(0 != iterator->current);

	if(block->type == FLAC__METADATA_TYPE_STREAMINFO)
		return false;

	if(0 == iterator->current->prev) {
		FLAC__ASSERT(iterator->current->data->type == FLAC__METADATA_TYPE_STREAMINFO);
		return false;
	}

	if(0 == (node = node_new_()))
		return false;

	node->data = block;
	iterator_insert_node_(iterator, node);
	iterator->current = node;
	return true;
}

FLAC__bool FLAC__metadata_iterator_insert_block_after(FLAC__MetaData_Iterator *iterator, FLAC__StreamMetaData *block)
{
	FLAC__MetaData_Node *node;

	FLAC__ASSERT(0 != iterator);
	FLAC__ASSERT(0 != iterator->current);

	if(block->type == FLAC__METADATA_TYPE_STREAMINFO)
		return false;

	if(0 == (node = node_new_()))
		return false;

	node->data = block;
	iterator_insert_node_after_(iterator, node);
	iterator->current = node;
	return true;
}


/****************************************************************************
 *
 * Metadata object routines
 *
 ***************************************************************************/

FLAC__StreamMetaData *FLAC__metadata_object_new(FLAC__MetaDataType type)
{
	FLAC__StreamMetaData *object = malloc(sizeof(FLAC__StreamMetaData));
	if(0 != object) {
		memset(object, 0, sizeof(FLAC__StreamMetaData));
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
				FLAC__ASSERT(0);
		}
	}

	return object;
}

static FLAC__bool copy_bytes_(FLAC__byte **to, const FLAC__byte *from, unsigned bytes)
{
	if(0 == (*to = malloc(bytes)))
		return false;
	memcpy(*to, from, bytes);
	return true;
}

static FLAC__bool copy_vcentry_(FLAC__StreamMetaData_VorbisComment_Entry *to, const FLAC__StreamMetaData_VorbisComment_Entry *from)
{
	to->length = from->length;
	if(0 == from->entry) {
		FLAC__ASSERT(0 == from->length);
		to->entry = 0;
	}
	else {
		if(0 == (to->entry = malloc(from->length)))
			return false;
		memcpy(to->entry, from->entry, from->length);
	}
	return true;
}

FLAC__StreamMetaData *FLAC__metadata_object_copy(const FLAC__StreamMetaData *object)
{
	FLAC__StreamMetaData *to;
	unsigned i;

	FLAC__ASSERT(0 != object);

	if(0 != (to = FLAC__metadata_object_new(object->type))) {
		to->is_last = object->is_last;
		to->type = object->type;
		to->length = object->length;
		switch(to->type) {
			case FLAC__METADATA_TYPE_STREAMINFO:
				memcpy(&to->data.stream_info, &object->data.stream_info, sizeof(FLAC__StreamMetaData_StreamInfo));
				break;
			case FLAC__METADATA_TYPE_PADDING:
				break;
			case FLAC__METADATA_TYPE_APPLICATION:
				memcpy(&to->data.application.id, &object->data.application.id, FLAC__STREAM_METADATA_APPLICATION_ID_LEN / 8);
				if(0 == object->data.application.data)
					to->data.application.data = 0;
				else {
					if(!copy_bytes_(&to->data.application.data, object->data.application.data, object->length - FLAC__STREAM_METADATA_APPLICATION_ID_LEN / 8)) {
						FLAC__metadata_object_delete(to);
						return 0;
					}
				}
				break;
			case FLAC__METADATA_TYPE_SEEKTABLE:
				to->data.seek_table.num_points = object->data.seek_table.num_points;
				if(0 == object->data.seek_table.points)
					to->data.seek_table.points = 0;
				else {
					if(!copy_bytes_((FLAC__byte**)&to->data.seek_table.points, (FLAC__byte*)object->data.seek_table.points, object->data.seek_table.num_points * sizeof(FLAC__StreamMetaData_SeekPoint))) {
						FLAC__metadata_object_delete(to);
						return 0;
					}
				}
				break;
			case FLAC__METADATA_TYPE_VORBIS_COMMENT:
				if(!copy_vcentry_(&to->data.vorbis_comment.vendor_string, &object->data.vorbis_comment.vendor_string)) {
					FLAC__metadata_object_delete(to);
					return 0;
				}
				to->data.vorbis_comment.num_comments = object->data.vorbis_comment.num_comments;
				if(0 == (to->data.vorbis_comment.comments = malloc(object->data.vorbis_comment.num_comments * sizeof(FLAC__StreamMetaData_VorbisComment_Entry*)))) {
					FLAC__metadata_object_delete(to);
					return 0;
				}
				/* Need to do this to set the pointers inside the comments to 0.
				 * In case of an error in the following loop, the object will be
				 * deleted and we don't want the destructor freeing uninitialized
				 * pointers.
				 */
				memset(to->data.vorbis_comment.comments, 0, object->data.vorbis_comment.num_comments * sizeof(FLAC__StreamMetaData_VorbisComment_Entry*));
				for(i = 0; i < object->data.vorbis_comment.num_comments; i++) {
					if(!copy_vcentry_(to->data.vorbis_comment.comments+i, object->data.vorbis_comment.comments+i)) {
						FLAC__metadata_object_delete(to);
						return 0;
					}
				}
				break;
			default:
				FLAC__ASSERT(0);
		}
	}

	return to;
}

void FLAC__metadata_object_delete_data_(FLAC__StreamMetaData *object)
{
	unsigned i;

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
			for(i = 0; i < object->data.vorbis_comment.num_comments; i++) {
				if(0 != object->data.vorbis_comment.comments[i].entry)
					free(object->data.vorbis_comment.comments[i].entry);
			}
			break;
		default:
			FLAC__ASSERT(0);
	}
}

void FLAC__metadata_object_delete(FLAC__StreamMetaData *object)
{
	FLAC__metadata_object_delete_data_(object);
	free(object);
}

#if 0
@@@@ should no longer be needed
void FLAC__metadata_object_calculate_length(FLAC__StreamMetaData *object)
{
	unsigned i;

	FLAC__ASSERT(0 != object);

	switch(type) {
		case FLAC__METADATA_TYPE_STREAMINFO:
		case FLAC__METADATA_TYPE_PADDING:
			/* already calculated on object creation */
			break;
		case FLAC__METADATA_TYPE_APPLICATION:
		case FLAC__METADATA_TYPE_SEEKTABLE:
			/* already calculated on object creation or _set */
			break;
		case FLAC__METADATA_TYPE_VORBIS_COMMENT:
			object->length = (FLAC__STREAM_METADATA_VORBIS_COMMENT_ENTRY_LENGTH_LEN) / 8;
			object->length += object->data.vorbis_comment.vendor_string.length;
			object->length += (FLAC__STREAM_METADATA_VORBIS_COMMENT_NUM_COMMENTS_LEN) / 8;
			for(i = 0; i < object->data.vorbis_comment.num_comments; i++) {
				object->length += (FLAC__STREAM_METADATA_VORBIS_COMMENT_ENTRY_LENGTH_LEN / 8);
				object->length += object->data.vorbis_comment.comments[i].length;
			}
			break;
		default:
			FLAC__ASSERT(0);
	}
}
#endif

/*@@@@ Allow setting pointer to 0 to free, or let length be 0 also.   fix everywhere */
FLAC__bool FLAC__metadata_object_application_set_data(FLAC__StreamMetaData *object, FLAC__byte *data, unsigned length, FLAC__bool copy)
{
	FLAC__ASSERT(object->type == FLAC__METADATA_TYPE_APPLICATION);

	if(0 != object->data.application.data)
		free(object->data.application.data);

	if(copy) {
		if(!copy_bytes_(&object->data.application.data, data, length))
			return false;
	}
	else {
		object->data.application.data = data;
	}
	object->length = FLAC__STREAM_METADATA_APPLICATION_ID_LEN / 8 + length;
	return true;
}

FLAC__StreamMetaData_SeekPoint *FLAC__metadata_object_seekpoint_array_new(unsigned num_points)
{
	FLAC__StreamMetaData_SeekPoint *object_array = malloc(num_points * sizeof(FLAC__StreamMetaData_SeekPoint));

	if(0 != object_array)
		memset(object_array, 0, num_points * sizeof(FLAC__StreamMetaData_SeekPoint));

	return object_array;
}

FLAC__StreamMetaData_SeekPoint *FLAC__metadata_object_seekpoint_array_copy(const FLAC__StreamMetaData_SeekPoint *object_array, unsigned num_points)
{
	FLAC__StreamMetaData_SeekPoint *return_array;

	FLAC__ASSERT(0 != object_array);

	return_array = FLAC__metadata_object_seekpoint_array_new(num_points);

	if(0 != return_array)
		memcpy(return_array, object_array, num_points * sizeof(FLAC__StreamMetaData_SeekPoint));

	return return_array;
}

void FLAC__metadata_object_seekpoint_array_delete(FLAC__StreamMetaData_SeekPoint *object_array)
{
	FLAC__ASSERT(0 != object_array);

	free(object_array);
}

FLAC__bool FLAC__metadata_object_seekpoint_array_resize(FLAC__StreamMetaData_SeekPoint **object_array, unsigned old_num_points, unsigned new_num_points)
{
	FLAC__ASSERT(0 != object_array);

	if(0 == *object_array) {
		FLAC__ASSERT(old_num_points == 0);
		return 0 != (*object_array = FLAC__metadata_object_seekpoint_array_new(new_num_points));
	}
	else {
		const unsigned old_size = old_num_points * sizeof(FLAC__StreamMetaData_SeekPoint);
		const unsigned new_size = new_num_points * sizeof(FLAC__StreamMetaData_SeekPoint);

		if(0 == (*object_array = realloc(*object_array, new_size)))
			return false;

		if(new_size > old_size)
			memset(*object_array + old_num_points, 0, new_size - old_size);

		return true;
	}
}

FLAC__bool FLAC__metadata_object_seektable_set_points(FLAC__StreamMetaData *object, FLAC__StreamMetaData_SeekPoint *points, unsigned num_points, FLAC__bool copy)
{
	FLAC__ASSERT(object->type == FLAC__METADATA_TYPE_SEEKTABLE);

	object->data.seek_table.num_points = num_points;

	if(0 != object->data.seek_table.points)
		FLAC__metadata_object_seekpoint_array_delete(object->data.seek_table.points);

	if(copy) {
		if(0 == (object->data.seek_table.points = FLAC__metadata_object_seekpoint_array_copy(points, num_points)))
			return false;
	}
	else {
		object->data.seek_table.points = points;
	}
	object->length = num_points * FLAC__STREAM_METADATA_SEEKPOINT_LENGTH;
	return true;
}

FLAC__StreamMetaData_VorbisComment_Entry *FLAC__metadata_object_vorbiscomment_entry_array_new(unsigned num_comments)
{
	FLAC__StreamMetaData_VorbisComment_Entry *object_array = malloc(num_comments * sizeof(FLAC__StreamMetaData_VorbisComment_Entry));

	if(0 != object_array)
		memset(object_array, 0, num_comments * sizeof(FLAC__StreamMetaData_VorbisComment_Entry));

	return object_array;
}

FLAC__StreamMetaData_VorbisComment_Entry *FLAC__metadata_object_vorbiscomment_entry_array_copy(const FLAC__StreamMetaData_VorbisComment_Entry *object_array, unsigned num_comments)
{
	FLAC__StreamMetaData_VorbisComment_Entry *return_array;

	FLAC__ASSERT(0 != object_array);

	return_array = FLAC__metadata_object_vorbiscomment_entry_array_new(num_comments);

	if(0 != return_array) {
		unsigned i;
		for(i = 0; i < num_comments; i++) {
			return_array[i].length = object_array[i].length;
			if(!copy_bytes_(&(return_array[i].entry), object_array[i].entry, object_array[i].length)) {
				FLAC__metadata_object_vorbiscomment_entry_array_delete(return_array, num_comments);
				return 0;
			}
		}
	}

	return return_array;
}

void FLAC__metadata_object_vorbiscomment_entry_array_delete(FLAC__StreamMetaData_VorbisComment_Entry *object_array, unsigned num_comments)
{
	unsigned i;

	FLAC__ASSERT(0 != object_array);

	for(i = 0; i < num_comments; i++)
		if(0 != object_array[i].entry)
			free(object_array[i].entry);

	free(object_array);
}

FLAC__bool FLAC__metadata_object_vorbiscomment_entry_array_resize(FLAC__StreamMetaData_VorbisComment_Entry **object_array, unsigned old_num_comments, unsigned new_num_comments)
{
	FLAC__ASSERT(0 != object_array);

	if(0 == *object_array) {
		FLAC__ASSERT(old_num_comments == 0);
		return 0 != (*object_array = FLAC__metadata_object_vorbiscomment_entry_array_new(new_num_comments));
	}
	else {
		const unsigned old_size = old_num_comments * sizeof(FLAC__StreamMetaData_VorbisComment_Entry);
		const unsigned new_size = new_num_comments * sizeof(FLAC__StreamMetaData_VorbisComment_Entry);

		/* if shrinking, free the truncated entries */
		if(new_num_comments < old_num_comments) {
			unsigned i;
			for(i = new_num_comments; i < old_num_comments; i++)
				free((*object_array)[i].entry);
		}

		if(0 == (*object_array = realloc(*object_array, new_size)))
			return false;

		/* if growing, zero all the length/pointers of new elements */
		if(new_size > old_size)
			memset(*object_array + old_num_comments, 0, new_size - old_size);

		return true;
	}
}

FLAC__bool FLAC__metadata_object_vorbiscomment_set_vendor_string(FLAC__StreamMetaData *object, FLAC__byte *entry, unsigned length, FLAC__bool copy)
{
	FLAC__ASSERT(object->type == FLAC__METADATA_TYPE_VORBIS_COMMENT);

	if(0 != object->data.vorbis_comment.vendor_string.entry)
		free(object->data.vorbis_comment.vendor_string.entry);

	object->length -= object->data.vorbis_comment.vendor_string.length;

	if(copy) {
		if(!copy_bytes_(&object->data.vorbis_comment.vendor_string.entry, entry, length)) {
			object->data.vorbis_comment.vendor_string.length = 0;
			return false;
		}
	}
	else {
		object->data.vorbis_comment.vendor_string.entry = entry;
	}

	object->data.vorbis_comment.vendor_string.length = length;
	object->length += object->data.vorbis_comment.vendor_string.length;

	return true;
}

FLAC__bool FLAC__metadata_object_vorbiscomment_set_comments(FLAC__StreamMetaData *object, FLAC__StreamMetaData_VorbisComment_Entry *comments, unsigned num_comments, FLAC__bool copy)
{
	unsigned i;

	FLAC__ASSERT(object->type == FLAC__METADATA_TYPE_VORBIS_COMMENT);

	object->data.vorbis_comment.num_comments = num_comments;

	if(0 != object->data.vorbis_comment.comments)
		FLAC__metadata_object_vorbiscomment_entry_array_delete(object->data.vorbis_comment.comments, num_comments);

	if(copy) {
		if(0 == (object->data.vorbis_comment.comments = FLAC__metadata_object_vorbiscomment_entry_array_copy(comments, num_comments)))
			return false;
	}
	else {
		object->data.vorbis_comment.comments = comments;
	}
	object->length = num_comments * FLAC__STREAM_METADATA_SEEKPOINT_LENGTH;

	/* calculate the new length */
	object->length = (FLAC__STREAM_METADATA_VORBIS_COMMENT_ENTRY_LENGTH_LEN) / 8;
	object->length += object->data.vorbis_comment.vendor_string.length;
	object->length += (FLAC__STREAM_METADATA_VORBIS_COMMENT_NUM_COMMENTS_LEN) / 8;
	for(i = 0; i < object->data.vorbis_comment.num_comments; i++) {
		object->length += (FLAC__STREAM_METADATA_VORBIS_COMMENT_ENTRY_LENGTH_LEN / 8);
		object->length += object->data.vorbis_comment.comments[i].length;
	}

	return true;
}


/****************************************************************************
 *
 * Local function definitions
 *
 ***************************************************************************/

void pack_uint32_(FLAC__uint32 val, FLAC__byte *b, unsigned bytes)
{
	unsigned i;

	b += bytes;

	for(i = 0; i < bytes; i++) {
		*(--b) = val & 0xff;
		val >>= 8;
	}
}

void pack_uint32_little_endian_(FLAC__uint32 val, FLAC__byte *b, unsigned bytes)
{
	unsigned i;

	for(i = 0; i < bytes; i++) {
		*(b++) = val & 0xff;
		val >>= 8;
	}
}

void pack_uint64_(FLAC__uint64 val, FLAC__byte *b, unsigned bytes)
{
	unsigned i;

	b += bytes;

	for(i = 0; i < bytes; i++) {
		*(--b) = val & 0xff;
		val >>= 8;
	}
}

FLAC__uint32 unpack_uint32_(FLAC__byte *b, unsigned bytes)
{
	FLAC__uint32 ret = 0;
	unsigned i;

	for(i = 0; i < bytes; i++)
		ret = (ret << 8) | (FLAC__uint32)(*b++);

	return ret;
}

FLAC__uint32 unpack_uint32_little_endian_(FLAC__byte *b, unsigned bytes)
{
	FLAC__uint32 ret = 0;
	unsigned i;

	b += bytes;

	for(i = 0; i < bytes; i++)
		ret = (ret << 8) | (FLAC__uint32)(*--b);

	return ret;
}

FLAC__uint64 unpack_uint64_(FLAC__byte *b, unsigned bytes)
{
	FLAC__uint64 ret = 0;
	unsigned i;

	for(i = 0; i < bytes; i++)
		ret = (ret << 8) | (FLAC__uint64)(*b++);

	return ret;
}

FLAC__bool read_metadata_block_header_(FLAC__MetaData_SimpleIterator *iterator)
{
	FLAC__byte raw_header[4]; /*@@@ 4 = MAGIC NUMBER for metadata block header bytes */

	FLAC__ASSERT(0 != iterator);
	FLAC__ASSERT(0 != iterator->file);

	if(fread(raw_header, 1, 4, iterator->file) != 4) { /*@@@ 4 = MAGIC NUMBER for metadata block header bytes */
		iterator->status = FLAC__METADATA_SIMPLE_ITERATOR_STATUS_READ_ERROR;
		return false;
	}

	iterator->is_last = raw_header[0] & 0x80? true : false;
	iterator->type = (FLAC__MetaDataType)(raw_header[0] & 0x7f);
	iterator->length = unpack_uint32_(raw_header + 1, 3);

	return true;
}

FLAC__bool read_metadata_block_data_(FLAC__MetaData_SimpleIterator *iterator, FLAC__StreamMetaData *block)
{
	FLAC__ASSERT(0 != iterator);
	FLAC__ASSERT(0 != iterator->file);

	switch(block->type) {
		case FLAC__METADATA_TYPE_STREAMINFO:
			iterator->status = read_metadata_block_data_streaminfo_(iterator->file, &block->data.stream_info);
			break;
		case FLAC__METADATA_TYPE_PADDING:
			iterator->status = read_metadata_block_data_padding_(iterator->file, &block->data.padding, block->length);
			break;
		case FLAC__METADATA_TYPE_APPLICATION:
			iterator->status = read_metadata_block_data_application_(iterator->file, &block->data.application, block->length);
			break;
		case FLAC__METADATA_TYPE_SEEKTABLE:
			iterator->status = read_metadata_block_data_seektable_(iterator->file, &block->data.seek_table, block->length);
			break;
		case FLAC__METADATA_TYPE_VORBIS_COMMENT:
			iterator->status = read_metadata_block_data_vorbis_comment_(iterator->file, &block->data.vorbis_comment);
			break;
		default:
			FLAC__ASSERT(0);
			iterator->status = FLAC__METADATA_SIMPLE_ITERATOR_STATUS_INTERNAL_ERROR;
	}

	return (iterator->status == FLAC__METADATA_SIMPLE_ITERATOR_STATUS_OK);
}

FLAC__MetaData_SimpleIteratorStatus read_metadata_block_data_streaminfo_(FILE *file, FLAC__StreamMetaData_StreamInfo *block)
{
	FLAC__byte buffer[FLAC__STREAM_METADATA_STREAMINFO_LENGTH], *b;

	FLAC__ASSERT(0 != file);

	if(fread(buffer, 1, FLAC__STREAM_METADATA_STREAMINFO_LENGTH, file) != FLAC__STREAM_METADATA_STREAMINFO_LENGTH)
		return FLAC__METADATA_SIMPLE_ITERATOR_STATUS_READ_ERROR;

	b = buffer;

	/* @@@ we are using hardcoded numbers for simplicity but we should
	 * probably eventually write a bit-level unpacker and use the
	 * _STREAMINFO_ constants.
	 */
	block->min_blocksize = unpack_uint32_(b, 2); b += 2;
	block->max_blocksize = unpack_uint32_(b, 2); b += 2;
	block->min_framesize = unpack_uint32_(b, 3); b += 3;
	block->max_framesize = unpack_uint32_(b, 3); b += 3;
	block->sample_rate = (unpack_uint32_(b, 2) << 4) | ((unsigned)(b[2] & 0xf0) >> 4);
	block->channels = (unsigned)((b[2] & 0x0e) >> 1) + 1;
	block->bits_per_sample = ((((unsigned)(b[2] & 0x01)) << 1) | (((unsigned)(b[3] & 0xf0)) >> 4)) + 1;
	block->total_samples = (((FLAC__uint64)(b[3] & 0x0f)) << 32) | unpack_uint64_(b+4, 4);
	memcpy(block->md5sum, b+8, 16);

	return FLAC__METADATA_SIMPLE_ITERATOR_STATUS_OK;
}


FLAC__MetaData_SimpleIteratorStatus read_metadata_block_data_padding_(FILE *file, FLAC__StreamMetaData_Padding *block, unsigned block_length)
{
	FLAC__ASSERT(0 != file);

	(void)block; /* nothing to do; we don't care about reading the padding bytes */

	if(0 != fseek(file, block_length, SEEK_CUR))
		return FLAC__METADATA_SIMPLE_ITERATOR_STATUS_SEEK_ERROR;

	return FLAC__METADATA_SIMPLE_ITERATOR_STATUS_OK;
}

FLAC__MetaData_SimpleIteratorStatus read_metadata_block_data_application_(FILE *file, FLAC__StreamMetaData_Application *block, unsigned block_length)
{
	const unsigned id_bytes = FLAC__STREAM_METADATA_APPLICATION_ID_LEN / 8;

	FLAC__ASSERT(0 != file);

	if(fread(block->id, 1, id_bytes, file) != id_bytes)
		return FLAC__METADATA_SIMPLE_ITERATOR_STATUS_READ_ERROR;

	block_length -= id_bytes;

	if(0 == (block->data = malloc(block_length)))
		return FLAC__METADATA_SIMPLE_ITERATOR_STATUS_MEMORY_ALLOCATION_ERROR;

	if(fread(block->data, 1, block_length, file) != block_length)
		return FLAC__METADATA_SIMPLE_ITERATOR_STATUS_READ_ERROR;

	return FLAC__METADATA_SIMPLE_ITERATOR_STATUS_OK;
}

FLAC__MetaData_SimpleIteratorStatus read_metadata_block_data_seektable_(FILE *file, FLAC__StreamMetaData_SeekTable *block, unsigned block_length)
{
	unsigned i;
	FLAC__byte buffer[FLAC__STREAM_METADATA_SEEKPOINT_LENGTH];

	FLAC__ASSERT(0 != file);
	FLAC__ASSERT(block_length % FLAC__STREAM_METADATA_SEEKPOINT_LENGTH == 0);

	block->num_points = block_length / FLAC__STREAM_METADATA_SEEKPOINT_LENGTH;

	if(0 == (block->points = malloc(block->num_points * sizeof(FLAC__StreamMetaData_SeekPoint))))
		return FLAC__METADATA_SIMPLE_ITERATOR_STATUS_MEMORY_ALLOCATION_ERROR;

	for(i = 0; i < block->num_points; i++) {
		if(fread(buffer, 1, FLAC__STREAM_METADATA_SEEKPOINT_LENGTH, file) != FLAC__STREAM_METADATA_SEEKPOINT_LENGTH)
			return FLAC__METADATA_SIMPLE_ITERATOR_STATUS_READ_ERROR;
		/*@@@ some MAGIC NUMBERs here */
		block->points[i].sample_number = unpack_uint64_(buffer, 8);
		block->points[i].stream_offset = unpack_uint64_(buffer+8, 8);
		block->points[i].frame_samples = unpack_uint32_(buffer+16, 2);
	}

	return FLAC__METADATA_SIMPLE_ITERATOR_STATUS_OK;
}

FLAC__MetaData_SimpleIteratorStatus read_metadata_block_data_vorbis_comment_entry_(FILE *file, FLAC__StreamMetaData_VorbisComment_Entry *entry)
{
	const unsigned entry_length_len = FLAC__STREAM_METADATA_VORBIS_COMMENT_ENTRY_LENGTH_LEN / 8;
	FLAC__byte buffer[FLAC__STREAM_METADATA_VORBIS_COMMENT_ENTRY_LENGTH_LEN / 8];

	FLAC__ASSERT(0 != file);

	if(fread(buffer, 1, entry_length_len, file) != entry_length_len)
		return FLAC__METADATA_SIMPLE_ITERATOR_STATUS_READ_ERROR;
	entry->length = unpack_uint32_little_endian_(buffer, 4);

	if(0 == (entry->entry = malloc(entry->length)))
		return FLAC__METADATA_SIMPLE_ITERATOR_STATUS_MEMORY_ALLOCATION_ERROR;

	if(fread(entry->entry, 1, entry->length, file) != entry->length)
		return FLAC__METADATA_SIMPLE_ITERATOR_STATUS_READ_ERROR;

	return FLAC__METADATA_SIMPLE_ITERATOR_STATUS_OK;
}

FLAC__MetaData_SimpleIteratorStatus read_metadata_block_data_vorbis_comment_(FILE *file, FLAC__StreamMetaData_VorbisComment *block)
{
	unsigned i;
	FLAC__MetaData_SimpleIteratorStatus status;
	const unsigned num_comments_len = FLAC__STREAM_METADATA_VORBIS_COMMENT_NUM_COMMENTS_LEN / 8;
	FLAC__byte buffer[FLAC__STREAM_METADATA_VORBIS_COMMENT_NUM_COMMENTS_LEN / 8];

	FLAC__ASSERT(0 != file);

	if(FLAC__METADATA_SIMPLE_ITERATOR_STATUS_OK != (status = read_metadata_block_data_vorbis_comment_entry_(file, &(block->vendor_string))))
		return status;

	if(fread(buffer, 1, num_comments_len, file) != num_comments_len)
		return FLAC__METADATA_SIMPLE_ITERATOR_STATUS_READ_ERROR;
	block->num_comments = unpack_uint32_little_endian_(buffer, num_comments_len);

	if(0 == (block->comments = malloc(block->num_comments * sizeof(FLAC__StreamMetaData_VorbisComment_Entry))))
		return FLAC__METADATA_SIMPLE_ITERATOR_STATUS_MEMORY_ALLOCATION_ERROR;

	for(i = 0; i < block->num_comments; i++) {
		if(FLAC__METADATA_SIMPLE_ITERATOR_STATUS_OK != (status = read_metadata_block_data_vorbis_comment_entry_(file, block->comments + i)))
			return status;
	}

	return FLAC__METADATA_SIMPLE_ITERATOR_STATUS_OK;
}

FLAC__bool write_metadata_block_header_(FILE *file, FLAC__MetaData_SimpleIteratorStatus *status, const FLAC__StreamMetaData *block)
{
	FLAC__byte buffer[4];

	FLAC__ASSERT(0 != file);
	FLAC__ASSERT(0 != status);
	FLAC__ASSERT(block->length < (1u << FLAC__STREAM_METADATA_LENGTH_LEN));

	buffer[0] = (block->is_last? 0x80 : 0) | (FLAC__byte)block->type;
	pack_uint32_(block->length, buffer + 1, 3);

	if(fwrite(buffer, 1, 4, file) != 4) {
		*status = FLAC__METADATA_SIMPLE_ITERATOR_STATUS_WRITE_ERROR;
		return false;
	}

	return true;
}

FLAC__bool write_metadata_block_data_(FILE *file, FLAC__MetaData_SimpleIteratorStatus *status, const FLAC__StreamMetaData *block)
{
	FLAC__ASSERT(0 != file);
	FLAC__ASSERT(0 != status);

	switch(block->type) {
		case FLAC__METADATA_TYPE_STREAMINFO:
			*status = write_metadata_block_data_streaminfo_(file, &block->data.stream_info);
			break;
		case FLAC__METADATA_TYPE_PADDING:
			*status = write_metadata_block_data_padding_(file, &block->data.padding, block->length);
			break;
		case FLAC__METADATA_TYPE_APPLICATION:
			*status = write_metadata_block_data_application_(file, &block->data.application, block->length);
			break;
		case FLAC__METADATA_TYPE_SEEKTABLE:
			*status = write_metadata_block_data_seektable_(file, &block->data.seek_table);
			break;
		case FLAC__METADATA_TYPE_VORBIS_COMMENT:
			*status = write_metadata_block_data_vorbis_comment_(file, &block->data.vorbis_comment);
			break;
		default:
			FLAC__ASSERT(0);
			*status = FLAC__METADATA_SIMPLE_ITERATOR_STATUS_INTERNAL_ERROR;
	}
	return (*status == FLAC__METADATA_SIMPLE_ITERATOR_STATUS_OK);
}

FLAC__MetaData_SimpleIteratorStatus write_metadata_block_data_streaminfo_(FILE *file, const FLAC__StreamMetaData_StreamInfo *block)
{
	FLAC__byte buffer[FLAC__STREAM_METADATA_STREAMINFO_LENGTH];
	const unsigned channels1 = block->channels - 1;
	const unsigned bps1 = block->bits_per_sample - 1;

	FLAC__ASSERT(0 != file);

	/* @@@ we are using hardcoded numbers for simplicity but we should
	 * probably eventually write a bit-level packer and use the
	 * _STREAMINFO_ constants.
	 */
	pack_uint32_(block->min_blocksize, buffer, 2);
	pack_uint32_(block->max_blocksize, buffer+2, 2);
	pack_uint32_(block->min_framesize, buffer+4, 3);
	pack_uint32_(block->max_framesize, buffer+7, 3);
	buffer[10] = (block->sample_rate >> 12) & 0xff;
	buffer[11] = (block->sample_rate >> 4) & 0xff;
	buffer[12] = ((block->sample_rate & 0x0f) << 4) | (channels1 << 1) | (bps1 >> 4);
	buffer[13] = ((bps1 & 0x0f) << 4) | ((block->total_samples >> 32) & 0x0f);
	pack_uint32_((FLAC__uint32)block->total_samples, buffer+14, 4);
	memcpy(buffer+18, block->md5sum, 16);

	if(fwrite(buffer, 1, FLAC__STREAM_METADATA_STREAMINFO_LENGTH, file) != FLAC__STREAM_METADATA_STREAMINFO_LENGTH)
		return FLAC__METADATA_SIMPLE_ITERATOR_STATUS_WRITE_ERROR;

	return FLAC__METADATA_SIMPLE_ITERATOR_STATUS_OK;
}

FLAC__MetaData_SimpleIteratorStatus write_metadata_block_data_padding_(FILE *file, const FLAC__StreamMetaData_Padding *block, unsigned block_length)
{
	unsigned i, n = block_length;
	FLAC__byte buffer[1024];

	FLAC__ASSERT(0 != file);

	(void)block;

	memset(buffer, 0, 1024);

	for(i = 0; i < n/1024; i++)
		if(fwrite(buffer, 1, 1024, file) != 1024)
			return FLAC__METADATA_SIMPLE_ITERATOR_STATUS_WRITE_ERROR;

	n %= 1024;
	
	if(fwrite(buffer, 1, n, file) != n)
		return FLAC__METADATA_SIMPLE_ITERATOR_STATUS_WRITE_ERROR;

	return FLAC__METADATA_SIMPLE_ITERATOR_STATUS_OK;
}

FLAC__MetaData_SimpleIteratorStatus write_metadata_block_data_application_(FILE *file, const FLAC__StreamMetaData_Application *block, unsigned block_length)
{
	const unsigned id_bytes = FLAC__STREAM_METADATA_APPLICATION_ID_LEN / 8;

	FLAC__ASSERT(0 != file);

	if(fwrite(block->id, 1, id_bytes, file) != id_bytes)
		return FLAC__METADATA_SIMPLE_ITERATOR_STATUS_WRITE_ERROR;

	block_length -= id_bytes;

	if(fwrite(block->data, 1, block_length, file) != block_length)
		return FLAC__METADATA_SIMPLE_ITERATOR_STATUS_WRITE_ERROR;

	return FLAC__METADATA_SIMPLE_ITERATOR_STATUS_OK;
}

FLAC__MetaData_SimpleIteratorStatus write_metadata_block_data_seektable_(FILE *file, const FLAC__StreamMetaData_SeekTable *block)
{
	unsigned i;
	FLAC__byte buffer[FLAC__STREAM_METADATA_SEEKPOINT_LENGTH];

	FLAC__ASSERT(0 != file);

	for(i = 0; i < block->num_points; i++) {
		/*@@@ some MAGIC NUMBERs here */
		pack_uint64_(block->points[i].sample_number, buffer, 8);
		pack_uint64_(block->points[i].stream_offset, buffer+8, 8);
		pack_uint32_(block->points[i].frame_samples, buffer+16, 2);
		if(fwrite(buffer, 1, FLAC__STREAM_METADATA_SEEKPOINT_LENGTH, file) != FLAC__STREAM_METADATA_SEEKPOINT_LENGTH)
			return FLAC__METADATA_SIMPLE_ITERATOR_STATUS_WRITE_ERROR;
	}

	return FLAC__METADATA_SIMPLE_ITERATOR_STATUS_OK;
}

FLAC__MetaData_SimpleIteratorStatus write_metadata_block_data_vorbis_comment_(FILE *file, const FLAC__StreamMetaData_VorbisComment *block)
{
	unsigned i;
	const unsigned entry_length_len = FLAC__STREAM_METADATA_VORBIS_COMMENT_ENTRY_LENGTH_LEN / 8;
	const unsigned num_comments_len = FLAC__STREAM_METADATA_VORBIS_COMMENT_NUM_COMMENTS_LEN / 8;
	FLAC__byte buffer[max(FLAC__STREAM_METADATA_VORBIS_COMMENT_ENTRY_LENGTH_LEN, FLAC__STREAM_METADATA_VORBIS_COMMENT_NUM_COMMENTS_LEN) / 8];

	FLAC__ASSERT(0 != file);

	pack_uint32_little_endian_(block->vendor_string.length, buffer, entry_length_len);
	if(fwrite(buffer, 1, entry_length_len, file) != entry_length_len)
		return FLAC__METADATA_SIMPLE_ITERATOR_STATUS_WRITE_ERROR;
	if(fwrite(block->vendor_string.entry, 1, block->vendor_string.length, file) != block->vendor_string.length)
		return FLAC__METADATA_SIMPLE_ITERATOR_STATUS_WRITE_ERROR;

	pack_uint32_little_endian_(block->num_comments, buffer, num_comments_len);
	if(fwrite(buffer, 1, num_comments_len, file) != num_comments_len)
		return FLAC__METADATA_SIMPLE_ITERATOR_STATUS_WRITE_ERROR;

	for(i = 0; i < block->num_comments; i++) {
		pack_uint32_little_endian_(block->comments[i].length, buffer, entry_length_len);
		if(fwrite(buffer, 1, entry_length_len, file) != entry_length_len)
			return FLAC__METADATA_SIMPLE_ITERATOR_STATUS_WRITE_ERROR;
		if(fwrite(block->comments[i].entry, 1, block->comments[i].length, file) != block->comments[i].length)
			return FLAC__METADATA_SIMPLE_ITERATOR_STATUS_WRITE_ERROR;
	}

	return FLAC__METADATA_SIMPLE_ITERATOR_STATUS_OK;
}

FLAC__bool write_metadata_block_stationary_(FLAC__MetaData_SimpleIterator *iterator, const FLAC__StreamMetaData *block)
{
	if(0 != fseek(iterator->file, iterator->offset[iterator->depth], SEEK_SET)) {
		iterator->status = FLAC__METADATA_SIMPLE_ITERATOR_STATUS_SEEK_ERROR;
		return false;
	}

	if(!write_metadata_block_header_(iterator->file, &iterator->status, block))
		return false;

	if(!write_metadata_block_data_(iterator->file, &iterator->status, block))
		return false;

	if(0 != fseek(iterator->file, iterator->offset[iterator->depth], SEEK_SET)) {
		iterator->status = FLAC__METADATA_SIMPLE_ITERATOR_STATUS_SEEK_ERROR;
		return false;
	}

	return read_metadata_block_header_(iterator);
}

FLAC__bool write_metadata_block_stationary_with_padding_(FLAC__MetaData_SimpleIterator *iterator, FLAC__StreamMetaData *block, unsigned padding_length, FLAC__bool padding_is_last)
{
	FLAC__StreamMetaData *padding;

	if(0 != fseek(iterator->file, iterator->offset[iterator->depth], SEEK_SET)) {
		iterator->status = FLAC__METADATA_SIMPLE_ITERATOR_STATUS_SEEK_ERROR;
		return false;
	}

	block->is_last = false;

	if(!write_metadata_block_header_(iterator->file, &iterator->status, block))
		return false;

	if(!write_metadata_block_data_(iterator->file, &iterator->status, block))
		return false;

	if(0 == (padding = FLAC__metadata_object_new(FLAC__METADATA_TYPE_PADDING)))
		return FLAC__METADATA_SIMPLE_ITERATOR_STATUS_MEMORY_ALLOCATION_ERROR;

	padding->is_last = padding_is_last;
	padding->length = padding_length;

	if(!write_metadata_block_header_(iterator->file, &iterator->status, padding)) {
		FLAC__metadata_object_delete(padding);
		return false;
	}

	if(!write_metadata_block_data_(iterator->file, &iterator->status, padding)) {
		FLAC__metadata_object_delete(padding);
		return false;
	}

	FLAC__metadata_object_delete(padding);

	if(0 != fseek(iterator->file, iterator->offset[iterator->depth], SEEK_SET)) {
		iterator->status = FLAC__METADATA_SIMPLE_ITERATOR_STATUS_SEEK_ERROR;
		return false;
	}

	return read_metadata_block_header_(iterator);
}

FLAC__bool rewrite_whole_file_(FLAC__MetaData_SimpleIterator *iterator, FLAC__StreamMetaData *block, FLAC__bool append)
{
	FILE *tempfile;
	char *tempfilename;
	long fixup_is_last_flag_offset;

	if(!simple_iterator_copy_file_prefix_(iterator, &tempfile, &tempfilename, append))
		return false;

	if(0 != block) {
		if(!write_metadata_block_header_(tempfile, &iterator->status, block)) {
			cleanup_tempfile_(&tempfile, &tempfilename);
			return false;
		}

		if(!write_metadata_block_data_(tempfile, &iterator->status, block)) {
			cleanup_tempfile_(&tempfile, &tempfilename);
			return false;
		}
	}

	if(append && iterator->is_last)
		fixup_is_last_flag_offset = iterator->offset[iterator->depth]; /*@@@ MAGIC NUMBER; we know the is_last flag is here */
	else
		fixup_is_last_flag_offset = -1;
	if(!simple_iterator_copy_file_postfix_(iterator, &tempfile, &tempfilename, fixup_is_last_flag_offset, block==0))
		return false;

	return true;
}

FLAC__bool chain_rewrite_chain_(FLAC__MetaData_Chain *chain)
{
	FILE *f;
	FLAC__MetaData_Node *node;
	FLAC__MetaData_SimpleIteratorStatus status;

	FLAC__ASSERT(0 != chain);
	FLAC__ASSERT(0 != chain->filename);
	FLAC__ASSERT(0 != chain->head);

	if(0 == (f = fopen(chain->filename, "r+b"))) {
		chain->status = FLAC__METADATA_CHAIN_STATUS_ERROR_OPENING_FILE;
		return false;
	}
	if(0 != fseek(f, chain->first_offset, SEEK_SET)) {
		chain->status = FLAC__METADATA_CHAIN_STATUS_SEEK_ERROR;
		return false;
	}

	for(node = chain->head; node; node = node->next) {
		if(!write_metadata_block_header_(f, &status, node->data)) {
			chain->status = get_equivalent_status_(status);
			return false;
		}
		if(!write_metadata_block_data_(f, &status, node->data)) {
			chain->status = get_equivalent_status_(status);
			return false;
		}
	}

	/*FLAC__ASSERT(fflush(), ftell() == chain->last_offset);*/

	(void)fclose(f);

	return true;
}

FLAC__bool chain_rewrite_file_(FLAC__MetaData_Chain *chain, const char *tempfile_path_prefix)
{
	FILE *f, *tempfile;
	char *tempfilename;
	FLAC__MetaData_SimpleIteratorStatus status;
	const FLAC__MetaData_Node *node;

	FLAC__ASSERT(0 != chain);
	FLAC__ASSERT(0 != chain->filename);
	FLAC__ASSERT(0 != chain->head);

	/* copy the file prefix (data up to first metadata block */
	if(0 == (f = fopen(chain->filename, "rb"))) {
		chain->status = FLAC__METADATA_CHAIN_STATUS_ERROR_OPENING_FILE;
		return false;
	}
	if(!open_tempfile_(chain->filename, tempfile_path_prefix, &tempfile, &tempfilename, &status)) {
		chain->status = get_equivalent_status_(status);
		cleanup_tempfile_(&tempfile, &tempfilename);
		return false;
	}
	if(!copy_n_bytes_from_file_(f, tempfile, chain->first_offset, &status)) {
		chain->status = get_equivalent_status_(status);
		cleanup_tempfile_(&tempfile, &tempfilename);
		return false;
	}

	/* write the metadata */
	for(node = chain->head; node; node = node->next) {
		if(!write_metadata_block_header_(tempfile, &status, node->data)) {
			chain->status = get_equivalent_status_(status);
			return false;
		}
		if(!write_metadata_block_data_(tempfile, &status, node->data)) {
			chain->status = get_equivalent_status_(status);
			return false;
		}
	}
	/*FLAC__ASSERT(fflush(), ftell() == chain->last_offset);*/

	/* copy the file postfix (everything after the metadata) */
	if(0 != fseek(f, chain->last_offset, SEEK_SET)) {
		cleanup_tempfile_(&tempfile, &tempfilename);
		chain->status = FLAC__METADATA_CHAIN_STATUS_SEEK_ERROR;
		return false;
	}
	if(!copy_remaining_bytes_from_file_(f, tempfile, &status)) {
		cleanup_tempfile_(&tempfile, &tempfilename);
		chain->status = get_equivalent_status_(status);
		return false;
	}

	/* move the tempfile on top of the original */
	(void)fclose(f);
	if(!transport_tempfile_(chain->filename, &tempfile, &tempfilename, &status))
		return false;

	return true;
}

void simple_iterator_push_(FLAC__MetaData_SimpleIterator *iterator)
{
	FLAC__ASSERT(iterator->depth+1 < SIMPLE_ITERATOR_MAX_PUSH_DEPTH);
	iterator->offset[iterator->depth+1] = iterator->offset[iterator->depth];
	iterator->depth++;
}

FLAC__bool simple_iterator_pop_(FLAC__MetaData_SimpleIterator *iterator)
{
	FLAC__ASSERT(iterator->depth > 0);
	iterator->depth--;
	if(0 != fseek(iterator->file, iterator->offset[iterator->depth], SEEK_SET)) {
		iterator->status = FLAC__METADATA_SIMPLE_ITERATOR_STATUS_SEEK_ERROR;
		return false;
	}

	return read_metadata_block_header_(iterator);
}

unsigned seek_to_first_metadata_block_(FILE *f)
{
	FLAC__byte signature[4]; /*@@@ 4 = MAGIC NUMBER for signature bytes */

	FLAC__ASSERT(0 != f);

	/*@@@@ skip id3v2, change comment about id3v2 in metadata.h*/

	/* search for the fLaC signature */
	if(fread(signature, 1, 4, f) == 4) { /*@@@ 4 = MAGIC NUMBER for signature bytes */
		if(0 == memcmp(FLAC__STREAM_SYNC_STRING, signature, 4)) {
			return 0;
		}
		else {
			return 2;
		}
	}
	else {
		return 1;
	}
}

FLAC__bool simple_iterator_copy_file_prefix_(FLAC__MetaData_SimpleIterator *iterator, FILE **tempfile, char **tempfilename, FLAC__bool append)
{
	const long offset_end = append? iterator->offset[iterator->depth] + 4 + (long)iterator->length : iterator->offset[iterator->depth]; /*@@@ MAGIC NUMBER 4 = metadata block header bytes */

	if(0 != fseek(iterator->file, 0, SEEK_SET)) {
		iterator->status = FLAC__METADATA_SIMPLE_ITERATOR_STATUS_SEEK_ERROR;
		return false;
	}
	if(!open_tempfile_(iterator->filename, iterator->tempfile_path_prefix, tempfile, tempfilename, &iterator->status)) {
		cleanup_tempfile_(tempfile, tempfilename);
		return false;
	}
	if(!copy_n_bytes_from_file_(iterator->file, *tempfile, offset_end, &iterator->status)) {
		cleanup_tempfile_(tempfile, tempfilename);
		return false;
	}

	return true;
}

FLAC__bool simple_iterator_copy_file_postfix_(FLAC__MetaData_SimpleIterator *iterator, FILE **tempfile, char **tempfilename, long fixup_is_last_flag_offset, FLAC__bool backup)
{
	long save_offset = iterator->offset[iterator->depth]; /*@@@ 2G limit */
	FLAC__ASSERT(0 != *tempfile);

	/*@@@ MAGIC NUMBER 4 = metadata header bytes */
	if(0 != fseek(iterator->file, save_offset + 4 + iterator->length, SEEK_SET)) {
		cleanup_tempfile_(tempfile, tempfilename);
		iterator->status = FLAC__METADATA_SIMPLE_ITERATOR_STATUS_SEEK_ERROR;
		return false;
	}
	if(!copy_remaining_bytes_from_file_(iterator->file, *tempfile, &iterator->status)) {
		cleanup_tempfile_(tempfile, tempfilename);
		return false;
	}

	if(fixup_is_last_flag_offset >= 0) {
		/* this means a block was appended to the end so we have to clear the is_last flag of the previous block */
		/*@@@ MAGIC NUMBERs here; we know the is_last flag is the hit bit of the byte at this location */
		FLAC__byte x;
		if(0 != fseek(*tempfile, fixup_is_last_flag_offset, SEEK_SET)) {
			cleanup_tempfile_(tempfile, tempfilename);
			iterator->status = FLAC__METADATA_SIMPLE_ITERATOR_STATUS_SEEK_ERROR;
			return false;
		}
		if(fread(&x, 1, 1, *tempfile) != 1) {
			cleanup_tempfile_(tempfile, tempfilename);
			iterator->status = FLAC__METADATA_SIMPLE_ITERATOR_STATUS_READ_ERROR;
			return false;
		}
		FLAC__ASSERT(x & 0x80);
		x &= 0x7f;
		if(0 != fseek(*tempfile, fixup_is_last_flag_offset, SEEK_SET)) {
			cleanup_tempfile_(tempfile, tempfilename);
			iterator->status = FLAC__METADATA_SIMPLE_ITERATOR_STATUS_SEEK_ERROR;
			return false;
		}
		if(fwrite(&x, 1, 1, *tempfile) != 1) {
			cleanup_tempfile_(tempfile, tempfilename);
			iterator->status = FLAC__METADATA_SIMPLE_ITERATOR_STATUS_WRITE_ERROR;
			return false;
		}
	}

	(void)fclose(iterator->file);

	if(!transport_tempfile_(iterator->filename, tempfile, tempfilename, &iterator->status))
		return false;

	if(iterator->has_stats)
		set_file_stats_(iterator->filename, &iterator->stats);

	if(!simple_iterator_prime_input_(iterator))
		return false;
	if(backup) {
		while(iterator->offset[iterator->depth] + 4 + (long)iterator->length < save_offset)
			if(!FLAC__metadata_simple_iterator_next(iterator))
				return false;
		return true;
	}
	else {
		/* move the iterator to it's original block faster by faking a push, then doing a pop_ */
		FLAC__ASSERT(iterator->depth == 0);
		iterator->offset[0] = save_offset;
		iterator->depth++;
		return simple_iterator_pop_(iterator);
	}
}

FLAC__bool copy_n_bytes_from_file_(FILE *file, FILE *tempfile, unsigned bytes/*@@@ 4G limit*/, FLAC__MetaData_SimpleIteratorStatus *status)
{
	FLAC__byte buffer[8192];
	unsigned n;

	while(bytes > 0) {
		n = min(sizeof(buffer), bytes);
		if(fread(buffer, 1, n, file) != n) {
			*status = FLAC__METADATA_SIMPLE_ITERATOR_STATUS_READ_ERROR;
			return false;
		}
		if(fwrite(buffer, 1, n, tempfile) != n) {
			*status = FLAC__METADATA_SIMPLE_ITERATOR_STATUS_WRITE_ERROR;
			return false;
		}
		bytes -= n;
	}

	return true;
}

FLAC__bool copy_remaining_bytes_from_file_(FILE *file, FILE *tempfile, FLAC__MetaData_SimpleIteratorStatus *status)
{
	FLAC__byte buffer[8192];
	size_t n;

	while(!feof(file)) {
		n = fread(buffer, 1, sizeof(buffer), file);
		if(n == 0 && !feof(file)) {
			*status = FLAC__METADATA_SIMPLE_ITERATOR_STATUS_READ_ERROR;
			return false;
		}
		if(n > 0 && fwrite(buffer, 1, n, tempfile) != n) {
			*status = FLAC__METADATA_SIMPLE_ITERATOR_STATUS_WRITE_ERROR;
			return false;
		}
	}

	return true;
}

FLAC__bool open_tempfile_(const char *filename, const char *tempfile_path_prefix, FILE **tempfile, char **tempfilename, FLAC__MetaData_SimpleIteratorStatus *status)
{
	static const char *tempfile_suffix = ".metadata_edit";
	if(0 == tempfile_path_prefix) {
		if(0 == (*tempfilename = malloc(strlen(filename) + strlen(tempfile_suffix) + 1))) {
			*status = FLAC__METADATA_SIMPLE_ITERATOR_STATUS_MEMORY_ALLOCATION_ERROR;
			return false;
		}
		strcpy(*tempfilename, filename);
		strcat(*tempfilename, tempfile_suffix);
	}
	else {
		const char *p = strrchr(filename, '/');
		if(0 == p)
			p = filename;
		else
			p++;

		if(0 == (*tempfilename = malloc(strlen(tempfile_path_prefix) + 1 + strlen(p) + strlen(tempfile_suffix) + 1))) {
			*status = FLAC__METADATA_SIMPLE_ITERATOR_STATUS_MEMORY_ALLOCATION_ERROR;
			return false;
		}
		strcpy(*tempfilename, tempfile_path_prefix);
		strcat(*tempfilename, "/");
		strcat(*tempfilename, p);
		strcat(*tempfilename, tempfile_suffix);
	}

	if(0 == (*tempfile = fopen(*tempfilename, "wb"))) {
		*status = FLAC__METADATA_SIMPLE_ITERATOR_STATUS_ERROR_OPENING_FILE;
		return false;
	}

	return true;
}

FLAC__bool transport_tempfile_(const char *filename, FILE **tempfile, char **tempfilename, FLAC__MetaData_SimpleIteratorStatus *status)
{
	FLAC__ASSERT(0 != filename);
	FLAC__ASSERT(0 != tempfile);
	FLAC__ASSERT(0 != *tempfile);
	FLAC__ASSERT(0 != tempfilename);
	FLAC__ASSERT(0 != *tempfilename);
	FLAC__ASSERT(0 != status);

	(void)fclose(*tempfile);
	*tempfile = 0;
	/*@@@ to fully support the tempfile_path_prefix we need to update this piece to actually copy across filesystems instead of just rename(): */
	if(0 != rename(*tempfilename, filename)) {
		cleanup_tempfile_(tempfile, tempfilename);
		*status = FLAC__METADATA_SIMPLE_ITERATOR_STATUS_RENAME_ERROR;
		return false;
	}

	cleanup_tempfile_(tempfile, tempfilename);

	return true;
}

void cleanup_tempfile_(FILE **tempfile, char **tempfilename)
{
	if(0 != *tempfile) {
		(void)fclose(*tempfile);
		*tempfile = 0;
	}

	if(0 != *tempfilename) {
		(void)unlink(*tempfilename);
		free(*tempfilename);
		*tempfilename = 0;
	}
}

FLAC__bool get_file_stats_(const char *filename, struct stat *stats)
{
	FLAC__ASSERT(0 != filename);
	FLAC__ASSERT(0 != stats);
	return (0 == stat(filename, stats));
}

void set_file_stats_(const char *filename, struct stat *stats)
{
	struct utimbuf srctime;

	FLAC__ASSERT(0 != filename);
	FLAC__ASSERT(0 != stats);

	srctime.actime = stats->st_atime;
	srctime.modtime = stats->st_mtime;
	(void)chmod(filename, stats->st_mode);
	(void)utime(filename, &srctime);
#if !defined _MSC_VER && !defined __MINGW32__
	(void)chown(filename, stats->st_uid, -1);
	(void)chown(filename, -1, stats->st_gid);
#endif
}

FLAC__MetaData_ChainStatus get_equivalent_status_(FLAC__MetaData_SimpleIteratorStatus status)
{
	switch(status) {
		case FLAC__METADATA_SIMPLE_ITERATOR_STATUS_OK:
			return FLAC__METADATA_CHAIN_STATUS_OK;
		case FLAC__METADATA_SIMPLE_ITERATOR_STATUS_ILLEGAL_INPUT:
			return FLAC__METADATA_CHAIN_STATUS_ILLEGAL_INPUT;
		case FLAC__METADATA_SIMPLE_ITERATOR_STATUS_ERROR_OPENING_FILE:
			return FLAC__METADATA_CHAIN_STATUS_ERROR_OPENING_FILE;
		case FLAC__METADATA_SIMPLE_ITERATOR_STATUS_NOT_A_FLAC_FILE:
			return FLAC__METADATA_CHAIN_STATUS_NOT_A_FLAC_FILE;
		case FLAC__METADATA_SIMPLE_ITERATOR_STATUS_NOT_WRITABLE:
			return FLAC__METADATA_CHAIN_STATUS_NOT_WRITABLE;
		case FLAC__METADATA_SIMPLE_ITERATOR_STATUS_READ_ERROR:
			return FLAC__METADATA_CHAIN_STATUS_READ_ERROR;
		case FLAC__METADATA_SIMPLE_ITERATOR_STATUS_SEEK_ERROR:
			return FLAC__METADATA_CHAIN_STATUS_SEEK_ERROR;
		case FLAC__METADATA_SIMPLE_ITERATOR_STATUS_WRITE_ERROR:
			return FLAC__METADATA_CHAIN_STATUS_WRITE_ERROR;
		case FLAC__METADATA_SIMPLE_ITERATOR_STATUS_RENAME_ERROR:
			return FLAC__METADATA_CHAIN_STATUS_RENAME_ERROR;
		case FLAC__METADATA_SIMPLE_ITERATOR_STATUS_UNLINK_ERROR:
			return FLAC__METADATA_CHAIN_STATUS_UNLINK_ERROR;
		case FLAC__METADATA_SIMPLE_ITERATOR_STATUS_MEMORY_ALLOCATION_ERROR:
			return FLAC__METADATA_CHAIN_STATUS_MEMORY_ALLOCATION_ERROR;
		case FLAC__METADATA_SIMPLE_ITERATOR_STATUS_INTERNAL_ERROR:
		default:
			return FLAC__METADATA_CHAIN_STATUS_INTERNAL_ERROR;
	}
}
