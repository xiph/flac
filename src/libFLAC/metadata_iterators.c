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

#if !defined _MSC_VER && !defined __MINGW32__
/*@@@ don't know how to resolve errno without using LIBC.LIB; must use MSVCRT.LIB only for plugins */
#include <errno.h>
#endif
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

#include "private/metadata.h"

#include "FLAC/assert.h"
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

static void pack_uint32_(FLAC__uint32 val, FLAC__byte *b, unsigned bytes);
static void pack_uint32_little_endian_(FLAC__uint32 val, FLAC__byte *b, unsigned bytes);
static void pack_uint64_(FLAC__uint64 val, FLAC__byte *b, unsigned bytes);
static FLAC__uint32 unpack_uint32_(FLAC__byte *b, unsigned bytes);
static FLAC__uint32 unpack_uint32_little_endian_(FLAC__byte *b, unsigned bytes);
static FLAC__uint64 unpack_uint64_(FLAC__byte *b, unsigned bytes);

static FLAC__bool read_metadata_block_header_(FLAC__Metadata_SimpleIterator *iterator);
static FLAC__bool read_metadata_block_data_(FLAC__Metadata_SimpleIterator *iterator, FLAC__StreamMetadata *block);
static FLAC__Metadata_SimpleIteratorStatus read_metadata_block_data_streaminfo_(FILE *file, FLAC__StreamMetadata_StreamInfo *block);
static FLAC__Metadata_SimpleIteratorStatus read_metadata_block_data_padding_(FILE *file, FLAC__StreamMetadata_Padding *block, unsigned block_length);
static FLAC__Metadata_SimpleIteratorStatus read_metadata_block_data_application_(FILE *file, FLAC__StreamMetadata_Application *block, unsigned block_length);
static FLAC__Metadata_SimpleIteratorStatus read_metadata_block_data_seektable_(FILE *file, FLAC__StreamMetadata_SeekTable *block, unsigned block_length);
static FLAC__Metadata_SimpleIteratorStatus read_metadata_block_data_vorbis_comment_entry_(FILE *file, FLAC__StreamMetadata_VorbisComment_Entry *entry);
static FLAC__Metadata_SimpleIteratorStatus read_metadata_block_data_vorbis_comment_(FILE *file, FLAC__StreamMetadata_VorbisComment *block);

static FLAC__bool write_metadata_block_header_(FILE *file, FLAC__Metadata_SimpleIteratorStatus *status, const FLAC__StreamMetadata *block);
static FLAC__bool write_metadata_block_data_(FILE *file, FLAC__Metadata_SimpleIteratorStatus *status, const FLAC__StreamMetadata *block);
static FLAC__Metadata_SimpleIteratorStatus write_metadata_block_data_streaminfo_(FILE *file, const FLAC__StreamMetadata_StreamInfo *block);
static FLAC__Metadata_SimpleIteratorStatus write_metadata_block_data_padding_(FILE *file, const FLAC__StreamMetadata_Padding *block, unsigned block_length);
static FLAC__Metadata_SimpleIteratorStatus write_metadata_block_data_application_(FILE *file, const FLAC__StreamMetadata_Application *block, unsigned block_length);
static FLAC__Metadata_SimpleIteratorStatus write_metadata_block_data_seektable_(FILE *file, const FLAC__StreamMetadata_SeekTable *block);
static FLAC__Metadata_SimpleIteratorStatus write_metadata_block_data_vorbis_comment_(FILE *file, const FLAC__StreamMetadata_VorbisComment *block);
static FLAC__bool write_metadata_block_stationary_(FLAC__Metadata_SimpleIterator *iterator, const FLAC__StreamMetadata *block);
static FLAC__bool write_metadata_block_stationary_with_padding_(FLAC__Metadata_SimpleIterator *iterator, FLAC__StreamMetadata *block, unsigned padding_length, FLAC__bool padding_is_last);
static FLAC__bool rewrite_whole_file_(FLAC__Metadata_SimpleIterator *iterator, FLAC__StreamMetadata *block, FLAC__bool append);

static FLAC__bool chain_rewrite_chain_(FLAC__Metadata_Chain *chain);
static FLAC__bool chain_rewrite_file_(FLAC__Metadata_Chain *chain, const char *tempfile_path_prefix);

static void simple_iterator_push_(FLAC__Metadata_SimpleIterator *iterator);
static FLAC__bool simple_iterator_pop_(FLAC__Metadata_SimpleIterator *iterator);

/* return 0 if OK, 1 if read error, 2 if not a FLAC file */
static unsigned seek_to_first_metadata_block_(FILE *f);

static FLAC__bool simple_iterator_copy_file_prefix_(FLAC__Metadata_SimpleIterator *iterator, FILE **tempfile, char **tempfilename, FLAC__bool append);
static FLAC__bool simple_iterator_copy_file_postfix_(FLAC__Metadata_SimpleIterator *iterator, FILE **tempfile, char **tempfilename, int fixup_is_last_code, long fixup_is_last_flag_offset, FLAC__bool backup);

static FLAC__bool copy_n_bytes_from_file_(FILE *file, FILE *tempfile, unsigned bytes/*@@@ 4G limit*/, FLAC__Metadata_SimpleIteratorStatus *status);
static FLAC__bool copy_remaining_bytes_from_file_(FILE *file, FILE *tempfile, FLAC__Metadata_SimpleIteratorStatus *status);

static FLAC__bool open_tempfile_(const char *filename, const char *tempfile_path_prefix, FILE **tempfile, char **tempfilename, FLAC__Metadata_SimpleIteratorStatus *status);
static FLAC__bool transport_tempfile_(const char *filename, FILE **tempfile, char **tempfilename, FLAC__Metadata_SimpleIteratorStatus *status);
static void cleanup_tempfile_(FILE **tempfile, char **tempfilename);

static FLAC__bool get_file_stats_(const char *filename, struct stat *stats);
static void set_file_stats_(const char *filename, struct stat *stats);

static FLAC__Metadata_ChainStatus get_equivalent_status_(FLAC__Metadata_SimpleIteratorStatus status);


/****************************************************************************
 *
 * Level 0 implementation
 *
 ***************************************************************************/

static FLAC__StreamDecoderWriteStatus write_callback_(const FLAC__FileDecoder *decoder, const FLAC__Frame *frame, const FLAC__int32 * const buffer[], void *client_data);
static void metadata_callback_(const FLAC__FileDecoder *decoder, const FLAC__StreamMetadata *metadata, void *client_data);
static void error_callback_(const FLAC__FileDecoder *decoder, FLAC__StreamDecoderErrorStatus status, void *client_data);

typedef struct {
	FLAC__bool got_error;
	FLAC__bool got_streaminfo;
	FLAC__StreamMetadata *streaminfo;
} level0_client_data;

FLAC_API FLAC__bool FLAC__metadata_get_streaminfo(const char *filename, FLAC__StreamMetadata *streaminfo)
{
	level0_client_data cd;
	FLAC__FileDecoder *decoder;

	FLAC__ASSERT(0 != filename);
	FLAC__ASSERT(0 != streaminfo);

	decoder = FLAC__file_decoder_new();

	if(0 == decoder)
		return false;

	cd.got_error = false;
	cd.got_streaminfo = false;
	cd.streaminfo = streaminfo;

	FLAC__file_decoder_set_md5_checking(decoder, false);
	FLAC__file_decoder_set_filename(decoder, filename);
	FLAC__file_decoder_set_metadata_ignore_all(decoder);
	FLAC__file_decoder_set_metadata_respond(decoder, FLAC__METADATA_TYPE_STREAMINFO);
	FLAC__file_decoder_set_write_callback(decoder, write_callback_);
	FLAC__file_decoder_set_metadata_callback(decoder, metadata_callback_);
	FLAC__file_decoder_set_error_callback(decoder, error_callback_);
	FLAC__file_decoder_set_client_data(decoder, &cd);

	if(FLAC__file_decoder_init(decoder) != FLAC__FILE_DECODER_OK || cd.got_error) {
		FLAC__file_decoder_finish(decoder);
		FLAC__file_decoder_delete(decoder);
		return false;
	}

	/* the first thing decoded must be the STREAMINFO block: */
	if(!FLAC__file_decoder_process_until_end_of_metadata(decoder) || cd.got_error) {
		FLAC__file_decoder_finish(decoder);
		FLAC__file_decoder_delete(decoder);
		return false;
	}

	FLAC__file_decoder_finish(decoder);
	FLAC__file_decoder_delete(decoder);

	return !cd.got_error && cd.got_streaminfo;
}

FLAC__StreamDecoderWriteStatus write_callback_(const FLAC__FileDecoder *decoder, const FLAC__Frame *frame, const FLAC__int32 * const buffer[], void *client_data)
{
	(void)decoder, (void)frame, (void)buffer, (void)client_data;

	return FLAC__STREAM_DECODER_WRITE_STATUS_CONTINUE;
}

void metadata_callback_(const FLAC__FileDecoder *decoder, const FLAC__StreamMetadata *metadata, void *client_data)
{
	level0_client_data *cd = (level0_client_data *)client_data;
	(void)decoder;

	if(metadata->type == FLAC__METADATA_TYPE_STREAMINFO && 0 != cd->streaminfo) {
		*(cd->streaminfo) = *metadata;
		cd->got_streaminfo = true;
	}
}

void error_callback_(const FLAC__FileDecoder *decoder, FLAC__StreamDecoderErrorStatus status, void *client_data)
{
	level0_client_data *cd = (level0_client_data *)client_data;
	(void)decoder;

	if(status != FLAC__STREAM_DECODER_ERROR_STATUS_LOST_SYNC)
		cd->got_error = true;
}


/****************************************************************************
 *
 * Level 1 implementation
 *
 ***************************************************************************/

#define SIMPLE_ITERATOR_MAX_PUSH_DEPTH (1+4)
/* 1 for initial offset, +4 for our own personal use */

struct FLAC__Metadata_SimpleIterator {
	FILE *file;
	char *filename, *tempfile_path_prefix;
	struct stat stats;
	FLAC__bool has_stats;
	FLAC__bool is_writable;
	FLAC__Metadata_SimpleIteratorStatus status;
	/*@@@ 2G limits here because of the offset type: */
	long offset[SIMPLE_ITERATOR_MAX_PUSH_DEPTH];
	long first_offset; /* this is the offset to the STREAMINFO block */
	unsigned depth;
	/* this is the metadata block header of the current block we are pointing to: */
	FLAC__bool is_last;
	FLAC__MetadataType type;
	unsigned length;
};

FLAC_API const char * const FLAC__Metadata_SimpleIteratorStatusString[] = {
	"FLAC__METADATA_SIMPLE_ITERATOR_STATUS_OK",
	"FLAC__METADATA_SIMPLE_ITERATOR_STATUS_ILLEGAL_INPUT",
	"FLAC__METADATA_SIMPLE_ITERATOR_STATUS_ERROR_OPENING_FILE",
	"FLAC__METADATA_SIMPLE_ITERATOR_STATUS_NOT_A_FLAC_FILE",
	"FLAC__METADATA_SIMPLE_ITERATOR_STATUS_NOT_WRITABLE",
	"FLAC__METADATA_SIMPLE_ITERATOR_STATUS_BAD_METADATA",
	"FLAC__METADATA_SIMPLE_ITERATOR_STATUS_READ_ERROR",
	"FLAC__METADATA_SIMPLE_ITERATOR_STATUS_SEEK_ERROR",
	"FLAC__METADATA_SIMPLE_ITERATOR_STATUS_WRITE_ERROR",
	"FLAC__METADATA_SIMPLE_ITERATOR_STATUS_RENAME_ERROR",
	"FLAC__METADATA_SIMPLE_ITERATOR_STATUS_UNLINK_ERROR",
	"FLAC__METADATA_SIMPLE_ITERATOR_STATUS_MEMORY_ALLOCATION_ERROR",
	"FLAC__METADATA_SIMPLE_ITERATOR_STATUS_INTERNAL_ERROR"
};


FLAC_API FLAC__Metadata_SimpleIterator *FLAC__metadata_simple_iterator_new()
{
	FLAC__Metadata_SimpleIterator *iterator = calloc(1, sizeof(FLAC__Metadata_SimpleIterator));

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

static void simple_iterator_free_guts_(FLAC__Metadata_SimpleIterator *iterator)
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

FLAC_API void FLAC__metadata_simple_iterator_delete(FLAC__Metadata_SimpleIterator *iterator)
{
	FLAC__ASSERT(0 != iterator);

	simple_iterator_free_guts_(iterator);
	free(iterator);
}

FLAC_API FLAC__Metadata_SimpleIteratorStatus FLAC__metadata_simple_iterator_status(FLAC__Metadata_SimpleIterator *iterator)
{
	FLAC__Metadata_SimpleIteratorStatus status;

	FLAC__ASSERT(0 != iterator);

	status = iterator->status;
	iterator->status = FLAC__METADATA_SIMPLE_ITERATOR_STATUS_OK;
	return status;
}

static FLAC__bool simple_iterator_prime_input_(FLAC__Metadata_SimpleIterator *iterator, FLAC__bool read_only)
{
	unsigned ret;

	FLAC__ASSERT(0 != iterator);

	if(read_only || 0 == (iterator->file = fopen(iterator->filename, "r+b"))) {
		iterator->is_writable = false;
#if !defined _MSC_VER && !defined __MINGW32__
/*@@@ don't know how to resolve errno without using LIBC.LIB; must use MSVCRT.LIB only for plugins */
		if(read_only || errno == EACCES) {
#endif
			if(0 == (iterator->file = fopen(iterator->filename, "rb"))) {
				iterator->status = FLAC__METADATA_SIMPLE_ITERATOR_STATUS_ERROR_OPENING_FILE;
				return false;
			}
#if !defined _MSC_VER && !defined __MINGW32__
		}
		else {
			iterator->status = FLAC__METADATA_SIMPLE_ITERATOR_STATUS_ERROR_OPENING_FILE;
			return false;
		}
#endif
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
FLAC__bool FLAC__metadata_simple_iterator_init(FLAC__Metadata_SimpleIterator *iterator, const char *filename, FLAC__bool preserve_file_stats, const char *tempfile_path_prefix);
#endif

FLAC_API FLAC__bool FLAC__metadata_simple_iterator_init(FLAC__Metadata_SimpleIterator *iterator, const char *filename, FLAC__bool read_only, FLAC__bool preserve_file_stats)
{
	const char *tempfile_path_prefix = 0; /*@@@ search for comments near 'rename(...)' for what it will take to finish implementing this */

	FLAC__ASSERT(0 != iterator);
	FLAC__ASSERT(0 != filename);

	simple_iterator_free_guts_(iterator);

	if(!read_only && preserve_file_stats)
		iterator->has_stats = get_file_stats_(filename, &iterator->stats);

	if(0 == (iterator->filename = strdup(filename))) {
		iterator->status = FLAC__METADATA_SIMPLE_ITERATOR_STATUS_MEMORY_ALLOCATION_ERROR;
		return false;
	}
	if(0 != tempfile_path_prefix && 0 == (iterator->tempfile_path_prefix = strdup(tempfile_path_prefix))) {
		iterator->status = FLAC__METADATA_SIMPLE_ITERATOR_STATUS_MEMORY_ALLOCATION_ERROR;
		return false;
	}

	return simple_iterator_prime_input_(iterator, read_only);
}

FLAC_API FLAC__bool FLAC__metadata_simple_iterator_is_writable(const FLAC__Metadata_SimpleIterator *iterator)
{
	FLAC__ASSERT(0 != iterator);
	FLAC__ASSERT(0 != iterator->file);

	return iterator->is_writable;
}

FLAC_API FLAC__bool FLAC__metadata_simple_iterator_next(FLAC__Metadata_SimpleIterator *iterator)
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

FLAC_API FLAC__bool FLAC__metadata_simple_iterator_prev(FLAC__Metadata_SimpleIterator *iterator)
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

FLAC_API FLAC__MetadataType FLAC__metadata_simple_iterator_get_block_type(const FLAC__Metadata_SimpleIterator *iterator)
{
	FLAC__ASSERT(0 != iterator);
	FLAC__ASSERT(0 != iterator->file);

	return iterator->type;
}

FLAC_API FLAC__StreamMetadata *FLAC__metadata_simple_iterator_get_block(FLAC__Metadata_SimpleIterator *iterator)
{
	FLAC__StreamMetadata *block = FLAC__metadata_object_new(iterator->type);

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
		if(0 != fseek(iterator->file, iterator->offset[iterator->depth] + FLAC__STREAM_METADATA_HEADER_LENGTH, SEEK_SET)) {
			iterator->status = FLAC__METADATA_SIMPLE_ITERATOR_STATUS_SEEK_ERROR;
			FLAC__metadata_object_delete(block);
			return 0;
		}
	}
	else
		iterator->status = FLAC__METADATA_SIMPLE_ITERATOR_STATUS_MEMORY_ALLOCATION_ERROR;

	return block;
}

FLAC_API FLAC__bool FLAC__metadata_simple_iterator_set_block(FLAC__Metadata_SimpleIterator *iterator, FLAC__StreamMetadata *block, FLAC__bool use_padding)
{
	FLAC__ASSERT_DECLARATION(long debug_target_offset = iterator->offset[iterator->depth];)
	FLAC__bool ret;

	FLAC__ASSERT(0 != iterator);
	FLAC__ASSERT(0 != iterator->file);
	FLAC__ASSERT(0 != block);

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

	if(iterator->length == block->length)
		return write_metadata_block_stationary_(iterator, block);
	else if(iterator->length > block->length) {
		if(use_padding && iterator->length >= FLAC__STREAM_METADATA_HEADER_LENGTH + block->length) {
			ret =  write_metadata_block_stationary_with_padding_(iterator, block, iterator->length - FLAC__STREAM_METADATA_HEADER_LENGTH - block->length, block->is_last);
			FLAC__ASSERT(iterator->offset[iterator->depth] == debug_target_offset);
			FLAC__ASSERT(ftell(iterator->file) == debug_target_offset + (long)FLAC__STREAM_METADATA_HEADER_LENGTH);
			return ret;
		}
		else {
			ret = rewrite_whole_file_(iterator, block, /*append=*/false);
			FLAC__ASSERT(iterator->offset[iterator->depth] == debug_target_offset);
			FLAC__ASSERT(ftell(iterator->file) == debug_target_offset + (long)FLAC__STREAM_METADATA_HEADER_LENGTH);
			return ret;
		}
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
				const unsigned extra_padding_bytes_required = block->length - iterator->length;
				simple_iterator_push_(iterator);
				if(!FLAC__metadata_simple_iterator_next(iterator)) {
					(void)simple_iterator_pop_(iterator);
					return false;
				}
				if(iterator->type != FLAC__METADATA_TYPE_PADDING) {
					use_padding = false;
				}
				else {
					if(FLAC__STREAM_METADATA_HEADER_LENGTH + iterator->length == extra_padding_bytes_required) {
						padding_leftover = 0;
						block->is_last = iterator->is_last;
					}
					else if(iterator->length < extra_padding_bytes_required)
						use_padding = false;
					else {
						padding_leftover = FLAC__STREAM_METADATA_HEADER_LENGTH + iterator->length - extra_padding_bytes_required;
						padding_is_last = iterator->is_last;
						block->is_last = false;
					}
				}
				if(!simple_iterator_pop_(iterator))
					return false;
			}
		}
		if(use_padding) {
			if(padding_leftover == 0) {
				ret = write_metadata_block_stationary_(iterator, block);
				FLAC__ASSERT(iterator->offset[iterator->depth] == debug_target_offset);
				FLAC__ASSERT(ftell(iterator->file) == debug_target_offset + (long)FLAC__STREAM_METADATA_HEADER_LENGTH);
				return ret;
			}
			else {
				FLAC__ASSERT(padding_leftover >= FLAC__STREAM_METADATA_HEADER_LENGTH);
				ret = write_metadata_block_stationary_with_padding_(iterator, block, padding_leftover - FLAC__STREAM_METADATA_HEADER_LENGTH, padding_is_last);
				FLAC__ASSERT(iterator->offset[iterator->depth] == debug_target_offset);
				FLAC__ASSERT(ftell(iterator->file) == debug_target_offset + (long)FLAC__STREAM_METADATA_HEADER_LENGTH);
				return ret;
			}
		}
		else {
			ret = rewrite_whole_file_(iterator, block, /*append=*/false);
			FLAC__ASSERT(iterator->offset[iterator->depth] == debug_target_offset);
			FLAC__ASSERT(ftell(iterator->file) == debug_target_offset + (long)FLAC__STREAM_METADATA_HEADER_LENGTH);
			return ret;
		}
	}
}

FLAC_API FLAC__bool FLAC__metadata_simple_iterator_insert_block_after(FLAC__Metadata_SimpleIterator *iterator, FLAC__StreamMetadata *block, FLAC__bool use_padding)
{
	unsigned padding_leftover = 0;
	FLAC__bool padding_is_last = false;

	FLAC__ASSERT_DECLARATION(long debug_target_offset = iterator->offset[iterator->depth] + FLAC__STREAM_METADATA_HEADER_LENGTH + iterator->length;)
	FLAC__bool ret;

	FLAC__ASSERT(0 != iterator);
	FLAC__ASSERT(0 != iterator->file);
	FLAC__ASSERT(0 != block);

	if(!iterator->is_writable)
		return false;

	if(block->type == FLAC__METADATA_TYPE_STREAMINFO) {
		iterator->status = FLAC__METADATA_SIMPLE_ITERATOR_STATUS_ILLEGAL_INPUT;
		return false;
	}

	block->is_last = iterator->is_last;

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
				if(iterator->length == block->length) {
					padding_leftover = 0;
					block->is_last = iterator->is_last;
				}
				else if(iterator->length < FLAC__STREAM_METADATA_HEADER_LENGTH + block->length)
					use_padding = false;
				else {
					padding_leftover = iterator->length - block->length;
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
		if(padding_leftover == 0) {
			ret = write_metadata_block_stationary_(iterator, block);
			FLAC__ASSERT(iterator->offset[iterator->depth] == debug_target_offset);
			FLAC__ASSERT(ftell(iterator->file) == debug_target_offset + (long)FLAC__STREAM_METADATA_HEADER_LENGTH);
			return ret;
		}
		else {
			FLAC__ASSERT(padding_leftover >= FLAC__STREAM_METADATA_HEADER_LENGTH);
			ret = write_metadata_block_stationary_with_padding_(iterator, block, padding_leftover - FLAC__STREAM_METADATA_HEADER_LENGTH, padding_is_last);
			FLAC__ASSERT(iterator->offset[iterator->depth] == debug_target_offset);
			FLAC__ASSERT(ftell(iterator->file) == debug_target_offset + (long)FLAC__STREAM_METADATA_HEADER_LENGTH);
			return ret;
		}
	}
	else {
		ret = rewrite_whole_file_(iterator, block, /*append=*/true);
		FLAC__ASSERT(iterator->offset[iterator->depth] == debug_target_offset);
		FLAC__ASSERT(ftell(iterator->file) == debug_target_offset + (long)FLAC__STREAM_METADATA_HEADER_LENGTH);
		return ret;
	}
}

FLAC_API FLAC__bool FLAC__metadata_simple_iterator_delete_block(FLAC__Metadata_SimpleIterator *iterator, FLAC__bool use_padding)
{
	FLAC__ASSERT_DECLARATION(long debug_target_offset = iterator->offset[iterator->depth];)
	FLAC__bool ret;

	if(iterator->type == FLAC__METADATA_TYPE_STREAMINFO) {
		iterator->status = FLAC__METADATA_SIMPLE_ITERATOR_STATUS_ILLEGAL_INPUT;
		return false;
	}

	if(use_padding) {
		FLAC__StreamMetadata *padding = FLAC__metadata_object_new(FLAC__METADATA_TYPE_PADDING);
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
		if(!FLAC__metadata_simple_iterator_prev(iterator))
			return false;
		FLAC__ASSERT(iterator->offset[iterator->depth] + (long)FLAC__STREAM_METADATA_HEADER_LENGTH + (long)iterator->length == debug_target_offset);
		FLAC__ASSERT(ftell(iterator->file) + (long)iterator->length == debug_target_offset);
		return true;
	}
	else {
		ret = rewrite_whole_file_(iterator, 0, /*append=*/false);
		FLAC__ASSERT(iterator->offset[iterator->depth] + (long)FLAC__STREAM_METADATA_HEADER_LENGTH + (long)iterator->length == debug_target_offset);
		FLAC__ASSERT(ftell(iterator->file) + (long)iterator->length == debug_target_offset);
		return ret;
	}
}



/****************************************************************************
 *
 * Level 2 implementation
 *
 ***************************************************************************/


typedef struct FLAC__Metadata_Node {
	FLAC__StreamMetadata *data;
	struct FLAC__Metadata_Node *prev, *next;
} FLAC__Metadata_Node;

struct FLAC__Metadata_Chain {
	char *filename;
	FLAC__Metadata_Node *head;
	FLAC__Metadata_Node *tail;
	unsigned nodes;
	FLAC__Metadata_ChainStatus status;
	long first_offset, last_offset; /*@@@ 2G limit */
	/*
	 * This is the length of the chain initially read from the FLAC file.
	 * it is used to compare against the current length to decide whether
	 * or not the whole file has to be rewritten.
	 */
	unsigned initial_length; /*@@@ 4G limit */
};

struct FLAC__Metadata_Iterator {
	FLAC__Metadata_Chain *chain;
	FLAC__Metadata_Node *current;
};

FLAC_API const char * const FLAC__Metadata_ChainStatusString[] = {
	"FLAC__METADATA_CHAIN_STATUS_OK",
	"FLAC__METADATA_CHAIN_STATUS_ILLEGAL_INPUT",
	"FLAC__METADATA_CHAIN_STATUS_ERROR_OPENING_FILE",
	"FLAC__METADATA_CHAIN_STATUS_NOT_A_FLAC_FILE",
	"FLAC__METADATA_CHAIN_STATUS_NOT_WRITABLE",
	"FLAC__METADATA_CHAIN_STATUS_BAD_METADATA",
	"FLAC__METADATA_CHAIN_STATUS_READ_ERROR",
	"FLAC__METADATA_CHAIN_STATUS_SEEK_ERROR",
	"FLAC__METADATA_CHAIN_STATUS_WRITE_ERROR",
	"FLAC__METADATA_CHAIN_STATUS_RENAME_ERROR",
	"FLAC__METADATA_CHAIN_STATUS_UNLINK_ERROR",
	"FLAC__METADATA_CHAIN_STATUS_MEMORY_ALLOCATION_ERROR",
	"FLAC__METADATA_CHAIN_STATUS_INTERNAL_ERROR"
};


static FLAC__Metadata_Node *node_new_()
{
	return (FLAC__Metadata_Node*)calloc(1, sizeof(FLAC__Metadata_Node));
}

static void node_delete_(FLAC__Metadata_Node *node)
{
	FLAC__ASSERT(0 != node);
	if(0 != node->data)
		FLAC__metadata_object_delete(node->data);
	free(node);
}

static void chain_append_node_(FLAC__Metadata_Chain *chain, FLAC__Metadata_Node *node)
{
	FLAC__ASSERT(0 != chain);
	FLAC__ASSERT(0 != node);
	FLAC__ASSERT(0 != node->data);

	node->next = node->prev = 0;
	node->data->is_last = true;
	if(0 != chain->tail)
		chain->tail->data->is_last = false;

	if(0 == chain->head)
		chain->head = node;
	else {
		FLAC__ASSERT(0 != chain->tail);
		chain->tail->next = node;
		node->prev = chain->tail;
	}
	chain->tail = node;
	chain->nodes++;
}

static void chain_remove_node_(FLAC__Metadata_Chain *chain, FLAC__Metadata_Node *node)
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

	if(0 != chain->tail)
		chain->tail->data->is_last = true;

	chain->nodes--;
}

static void chain_delete_node_(FLAC__Metadata_Chain *chain, FLAC__Metadata_Node *node)
{
	chain_remove_node_(chain, node);
	node_delete_(node);
}

static unsigned chain_calculate_length_(FLAC__Metadata_Chain *chain)
{
	const FLAC__Metadata_Node *node;
	unsigned length = 0;
	for(node = chain->head; node; node = node->next)
		length += (FLAC__STREAM_METADATA_HEADER_LENGTH + node->data->length);
	return length;
}

static void iterator_insert_node_(FLAC__Metadata_Iterator *iterator, FLAC__Metadata_Node *node)
{
	FLAC__ASSERT(0 != node);
	FLAC__ASSERT(0 != node->data);
	FLAC__ASSERT(0 != iterator);
	FLAC__ASSERT(0 != iterator->current);
	FLAC__ASSERT(0 != iterator->chain);
	FLAC__ASSERT(0 != iterator->chain->head);
	FLAC__ASSERT(0 != iterator->chain->tail);

	node->data->is_last = false;

	node->prev = iterator->current->prev;
	node->next = iterator->current;

	if(0 == node->prev)
		iterator->chain->head = node;
	else
		node->prev->next = node;

	iterator->current->prev = node;

	iterator->chain->nodes++;
}

static void iterator_insert_node_after_(FLAC__Metadata_Iterator *iterator, FLAC__Metadata_Node *node)
{
	FLAC__ASSERT(0 != node);
	FLAC__ASSERT(0 != node->data);
	FLAC__ASSERT(0 != iterator);
	FLAC__ASSERT(0 != iterator->current);
	FLAC__ASSERT(0 != iterator->chain);
	FLAC__ASSERT(0 != iterator->chain->head);
	FLAC__ASSERT(0 != iterator->chain->tail);

	iterator->current->data->is_last = false;

	node->prev = iterator->current;
	node->next = iterator->current->next;

	if(0 == node->next)
		iterator->chain->tail = node;
	else
		node->next->prev = node;

	node->prev->next = node;

	iterator->chain->tail->data->is_last = true;

	iterator->chain->nodes++;
}

/* return true iff node and node->next are both padding */
static FLAC__bool chain_merge_adjacent_padding_(FLAC__Metadata_Chain *chain, FLAC__Metadata_Node *node)
{
	if(node->data->type == FLAC__METADATA_TYPE_PADDING && 0 != node->next && node->next->data->type == FLAC__METADATA_TYPE_PADDING) {
		const unsigned growth = FLAC__STREAM_METADATA_HEADER_LENGTH + node->next->data->length;
		node->data->length += growth;

		chain_delete_node_(chain, node->next);
		return true;
	}
	else
		return false;
}

FLAC_API FLAC__Metadata_Chain *FLAC__metadata_chain_new()
{
	FLAC__Metadata_Chain *chain = calloc(1, sizeof(FLAC__Metadata_Chain));

	if(0 != chain) {
		chain->filename = 0;
		chain->head = chain->tail = 0;
		chain->nodes = 0;
		chain->status = FLAC__METADATA_CHAIN_STATUS_OK;
		chain->initial_length = 0;
	}

	return chain;
}

FLAC_API void FLAC__metadata_chain_delete(FLAC__Metadata_Chain *chain)
{
	FLAC__Metadata_Node *node, *next;

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

FLAC_API FLAC__Metadata_ChainStatus FLAC__metadata_chain_status(FLAC__Metadata_Chain *chain)
{
	FLAC__Metadata_ChainStatus status;

	FLAC__ASSERT(0 != chain);

	status = chain->status;
	chain->status = FLAC__METADATA_CHAIN_STATUS_OK;
	return status;
}

FLAC_API FLAC__bool FLAC__metadata_chain_read(FLAC__Metadata_Chain *chain, const char *filename)
{
	FLAC__Metadata_SimpleIterator *iterator;
	FLAC__Metadata_Node *node;

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

	if(!FLAC__metadata_simple_iterator_init(iterator, filename, /*read_only=*/true, /*preserve_file_stats=*/false)) {
		chain->status = get_equivalent_status_(iterator->status);
		return false;
	}

	chain->first_offset = iterator->offset[iterator->depth];

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

	chain->last_offset = ftell(iterator->file) + iterator->length;
	FLAC__metadata_simple_iterator_delete(iterator);

	chain->initial_length = chain_calculate_length_(chain);
	return true;
}

FLAC_API FLAC__bool FLAC__metadata_chain_write(FLAC__Metadata_Chain *chain, FLAC__bool use_padding, FLAC__bool preserve_file_stats)
{
	struct stat stats;
	const char *tempfile_path_prefix = 0;
	unsigned current_length;

	FLAC__ASSERT(0 != chain);

	current_length = chain_calculate_length_(chain);

	if(use_padding) {
		if(current_length < chain->initial_length && chain->tail->data->type == FLAC__METADATA_TYPE_PADDING) {
			const unsigned delta = chain->initial_length - current_length;
			chain->tail->data->length += delta;
			current_length += delta;
			FLAC__ASSERT(current_length == chain->initial_length);
		}
		else if(current_length + FLAC__STREAM_METADATA_HEADER_LENGTH <= chain->initial_length) {
			FLAC__StreamMetadata *padding;
			FLAC__Metadata_Node *node;
			if(0 == (padding = FLAC__metadata_object_new(FLAC__METADATA_TYPE_PADDING))) {
				chain->status = FLAC__METADATA_CHAIN_STATUS_MEMORY_ALLOCATION_ERROR;
				return false;
			}
			padding->length = chain->initial_length - (FLAC__STREAM_METADATA_HEADER_LENGTH + current_length);
			if(0 == (node = node_new_())) {
				FLAC__metadata_object_delete(padding);
				chain->status = FLAC__METADATA_CHAIN_STATUS_MEMORY_ALLOCATION_ERROR;
				return false;
			}
			node->data = padding;
			chain_append_node_(chain, node);
			current_length = chain_calculate_length_(chain);
			FLAC__ASSERT(current_length == chain->initial_length);
		}
		else if(current_length > chain->initial_length) {
			const unsigned delta = current_length - chain->initial_length;
			if(chain->tail->data->type == FLAC__METADATA_TYPE_PADDING) {
				if(chain->tail->data->length + FLAC__STREAM_METADATA_HEADER_LENGTH == delta) {
					chain_delete_node_(chain, chain->tail);
					current_length = chain_calculate_length_(chain);
					FLAC__ASSERT(current_length == chain->initial_length);
				}
				else if(chain->tail->data->length >= delta) {
					chain->tail->data->length -= delta;
					current_length -= delta;
					FLAC__ASSERT(current_length == chain->initial_length);
				}
			}
		}
	}

	if(preserve_file_stats)
		get_file_stats_(chain->filename, &stats);

	if(current_length == chain->initial_length) {
		if(!chain_rewrite_chain_(chain))
			return false;
	}
	else {
		if(!chain_rewrite_file_(chain, tempfile_path_prefix))
			return false;
	}

	if(preserve_file_stats)
		set_file_stats_(chain->filename, &stats);

	/* recompute lengths and offsets if necessary */
	if(chain->initial_length != current_length) {
		const FLAC__Metadata_Node *node;
		chain->initial_length = current_length;
		chain->last_offset = chain->first_offset;
		for(node = chain->head; node; node = node->next)
			chain->last_offset += (FLAC__STREAM_METADATA_HEADER_LENGTH + node->data->length);
	}

	return true;
}

FLAC_API void FLAC__metadata_chain_merge_padding(FLAC__Metadata_Chain *chain)
{
	FLAC__Metadata_Node *node;

	FLAC__ASSERT(0 != chain);

	for(node = chain->head; node; ) {
		if(!chain_merge_adjacent_padding_(chain, node))
			node = node->next;
	}
}

FLAC_API void FLAC__metadata_chain_sort_padding(FLAC__Metadata_Chain *chain)
{
	FLAC__Metadata_Node *node, *save;
	unsigned i;

	FLAC__ASSERT(0 != chain);

	/*
	 * Don't try and be too smart... this simple algo is good enough for
	 * the small number of nodes that we deal with.
	 */
	for(i = 0, node = chain->head; i < chain->nodes; i++) {
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

	FLAC__metadata_chain_merge_padding(chain);
}


FLAC_API FLAC__Metadata_Iterator *FLAC__metadata_iterator_new()
{
	FLAC__Metadata_Iterator *iterator = calloc(1, sizeof(FLAC__Metadata_Iterator));

	/* calloc() implies:
		iterator->current = 0;
		iterator->chain = 0;
	*/

	return iterator;
}

FLAC_API void FLAC__metadata_iterator_delete(FLAC__Metadata_Iterator *iterator)
{
	FLAC__ASSERT(0 != iterator);

	free(iterator);
}

FLAC_API void FLAC__metadata_iterator_init(FLAC__Metadata_Iterator *iterator, FLAC__Metadata_Chain *chain)
{
	FLAC__ASSERT(0 != iterator);
	FLAC__ASSERT(0 != chain);
	FLAC__ASSERT(0 != chain->head);

	iterator->chain = chain;
	iterator->current = chain->head;
}

FLAC_API FLAC__bool FLAC__metadata_iterator_next(FLAC__Metadata_Iterator *iterator)
{
	FLAC__ASSERT(0 != iterator);

	if(0 == iterator->current || 0 == iterator->current->next)
		return false;

	iterator->current = iterator->current->next;
	return true;
}

FLAC_API FLAC__bool FLAC__metadata_iterator_prev(FLAC__Metadata_Iterator *iterator)
{
	FLAC__ASSERT(0 != iterator);

	if(0 == iterator->current || 0 == iterator->current->prev)
		return false;

	iterator->current = iterator->current->prev;
	return true;
}

FLAC_API FLAC__MetadataType FLAC__metadata_iterator_get_block_type(const FLAC__Metadata_Iterator *iterator)
{
	FLAC__ASSERT(0 != iterator);
	FLAC__ASSERT(0 != iterator->current);
	FLAC__ASSERT(0 != iterator->current->data);

	return iterator->current->data->type;
}

FLAC_API FLAC__StreamMetadata *FLAC__metadata_iterator_get_block(FLAC__Metadata_Iterator *iterator)
{
	FLAC__ASSERT(0 != iterator);
	FLAC__ASSERT(0 != iterator->current);

	return iterator->current->data;
}

FLAC_API FLAC__bool FLAC__metadata_iterator_set_block(FLAC__Metadata_Iterator *iterator, FLAC__StreamMetadata *block)
{
	FLAC__ASSERT(0 != iterator);
	FLAC__ASSERT(0 != block);
	return FLAC__metadata_iterator_delete_block(iterator, false) && FLAC__metadata_iterator_insert_block_after(iterator, block);
}

FLAC_API FLAC__bool FLAC__metadata_iterator_delete_block(FLAC__Metadata_Iterator *iterator, FLAC__bool replace_with_padding)
{
	FLAC__Metadata_Node *save;

	FLAC__ASSERT(0 != iterator);
	FLAC__ASSERT(0 != iterator->current);

	if(0 == iterator->current->prev) {
		FLAC__ASSERT(iterator->current->data->type == FLAC__METADATA_TYPE_STREAMINFO);
		return false;
	}

	save = iterator->current->prev;

	if(replace_with_padding) {
		FLAC__metadata_object_delete_data(iterator->current->data);
		iterator->current->data->type = FLAC__METADATA_TYPE_PADDING;
	}
	else {
		chain_delete_node_(iterator->chain, iterator->current);
	}

	iterator->current = save;
	return true;
}

FLAC_API FLAC__bool FLAC__metadata_iterator_insert_block_before(FLAC__Metadata_Iterator *iterator, FLAC__StreamMetadata *block)
{
	FLAC__Metadata_Node *node;

	FLAC__ASSERT(0 != iterator);
	FLAC__ASSERT(0 != iterator->current);
	FLAC__ASSERT(0 != block);

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

FLAC_API FLAC__bool FLAC__metadata_iterator_insert_block_after(FLAC__Metadata_Iterator *iterator, FLAC__StreamMetadata *block)
{
	FLAC__Metadata_Node *node;

	FLAC__ASSERT(0 != iterator);
	FLAC__ASSERT(0 != iterator->current);
	FLAC__ASSERT(0 != block);

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
 * Local function definitions
 *
 ***************************************************************************/

void pack_uint32_(FLAC__uint32 val, FLAC__byte *b, unsigned bytes)
{
	unsigned i;

	b += bytes;

	for(i = 0; i < bytes; i++) {
		*(--b) = (FLAC__byte)(val & 0xff);
		val >>= 8;
	}
}

void pack_uint32_little_endian_(FLAC__uint32 val, FLAC__byte *b, unsigned bytes)
{
	unsigned i;

	for(i = 0; i < bytes; i++) {
		*(b++) = (FLAC__byte)(val & 0xff);
		val >>= 8;
	}
}

void pack_uint64_(FLAC__uint64 val, FLAC__byte *b, unsigned bytes)
{
	unsigned i;

	b += bytes;

	for(i = 0; i < bytes; i++) {
		*(--b) = (FLAC__byte)(val & 0xff);
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

FLAC__bool read_metadata_block_header_(FLAC__Metadata_SimpleIterator *iterator)
{
	FLAC__byte raw_header[FLAC__STREAM_METADATA_HEADER_LENGTH];

	FLAC__ASSERT(0 != iterator);
	FLAC__ASSERT(0 != iterator->file);

	if(fread(raw_header, 1, FLAC__STREAM_METADATA_HEADER_LENGTH, iterator->file) != FLAC__STREAM_METADATA_HEADER_LENGTH) {
		iterator->status = FLAC__METADATA_SIMPLE_ITERATOR_STATUS_READ_ERROR;
		return false;
	}

	iterator->is_last = raw_header[0] & 0x80? true : false;
	iterator->type = (FLAC__MetadataType)(raw_header[0] & 0x7f);
	iterator->length = unpack_uint32_(raw_header + 1, 3);

	/* do some checking */
	if(iterator->type > FLAC__METADATA_TYPE_VORBIS_COMMENT) {
		iterator->status = FLAC__METADATA_SIMPLE_ITERATOR_STATUS_BAD_METADATA;
		return false;
	}

	return true;
}

FLAC__bool read_metadata_block_data_(FLAC__Metadata_SimpleIterator *iterator, FLAC__StreamMetadata *block)
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

FLAC__Metadata_SimpleIteratorStatus read_metadata_block_data_streaminfo_(FILE *file, FLAC__StreamMetadata_StreamInfo *block)
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
	block->bits_per_sample = ((((unsigned)(b[2] & 0x01)) << 4) | (((unsigned)(b[3] & 0xf0)) >> 4)) + 1;
	block->total_samples = (((FLAC__uint64)(b[3] & 0x0f)) << 32) | unpack_uint64_(b+4, 4);
	memcpy(block->md5sum, b+8, 16);

	return FLAC__METADATA_SIMPLE_ITERATOR_STATUS_OK;
}


FLAC__Metadata_SimpleIteratorStatus read_metadata_block_data_padding_(FILE *file, FLAC__StreamMetadata_Padding *block, unsigned block_length)
{
	FLAC__ASSERT(0 != file);

	(void)block; /* nothing to do; we don't care about reading the padding bytes */

	if(0 != fseek(file, block_length, SEEK_CUR))
		return FLAC__METADATA_SIMPLE_ITERATOR_STATUS_SEEK_ERROR;

	return FLAC__METADATA_SIMPLE_ITERATOR_STATUS_OK;
}

FLAC__Metadata_SimpleIteratorStatus read_metadata_block_data_application_(FILE *file, FLAC__StreamMetadata_Application *block, unsigned block_length)
{
	const unsigned id_bytes = FLAC__STREAM_METADATA_APPLICATION_ID_LEN / 8;

	FLAC__ASSERT(0 != file);

	if(fread(block->id, 1, id_bytes, file) != id_bytes)
		return FLAC__METADATA_SIMPLE_ITERATOR_STATUS_READ_ERROR;

	block_length -= id_bytes;

	if(block_length == 0) {
		block->data = 0;
	}
	else {
		if(0 == (block->data = malloc(block_length)))
			return FLAC__METADATA_SIMPLE_ITERATOR_STATUS_MEMORY_ALLOCATION_ERROR;

		if(fread(block->data, 1, block_length, file) != block_length)
			return FLAC__METADATA_SIMPLE_ITERATOR_STATUS_READ_ERROR;
	}

	return FLAC__METADATA_SIMPLE_ITERATOR_STATUS_OK;
}

FLAC__Metadata_SimpleIteratorStatus read_metadata_block_data_seektable_(FILE *file, FLAC__StreamMetadata_SeekTable *block, unsigned block_length)
{
	unsigned i;
	FLAC__byte buffer[FLAC__STREAM_METADATA_SEEKPOINT_LENGTH];

	FLAC__ASSERT(0 != file);
	FLAC__ASSERT(block_length % FLAC__STREAM_METADATA_SEEKPOINT_LENGTH == 0);

	block->num_points = block_length / FLAC__STREAM_METADATA_SEEKPOINT_LENGTH;

	if(block->num_points == 0)
		block->points = 0;
	else if(0 == (block->points = malloc(block->num_points * sizeof(FLAC__StreamMetadata_SeekPoint))))
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

FLAC__Metadata_SimpleIteratorStatus read_metadata_block_data_vorbis_comment_entry_(FILE *file, FLAC__StreamMetadata_VorbisComment_Entry *entry)
{
	const unsigned entry_length_len = FLAC__STREAM_METADATA_VORBIS_COMMENT_ENTRY_LENGTH_LEN / 8;
	FLAC__byte buffer[4]; /* magic number is asserted below */

	FLAC__ASSERT(FLAC__STREAM_METADATA_VORBIS_COMMENT_ENTRY_LENGTH_LEN / 8 == 4);
	FLAC__ASSERT(0 != file);

	if(fread(buffer, 1, entry_length_len, file) != entry_length_len)
		return FLAC__METADATA_SIMPLE_ITERATOR_STATUS_READ_ERROR;
	entry->length = unpack_uint32_little_endian_(buffer, entry_length_len);

	if(0 != entry->entry)
		free(entry->entry);

	if(entry->length == 0) {
		entry->entry = 0;
	}
	else {
		if(0 == (entry->entry = malloc(entry->length)))
			return FLAC__METADATA_SIMPLE_ITERATOR_STATUS_MEMORY_ALLOCATION_ERROR;

		if(fread(entry->entry, 1, entry->length, file) != entry->length)
			return FLAC__METADATA_SIMPLE_ITERATOR_STATUS_READ_ERROR;
	}

	return FLAC__METADATA_SIMPLE_ITERATOR_STATUS_OK;
}

FLAC__Metadata_SimpleIteratorStatus read_metadata_block_data_vorbis_comment_(FILE *file, FLAC__StreamMetadata_VorbisComment *block)
{
	unsigned i;
	FLAC__Metadata_SimpleIteratorStatus status;
	const unsigned num_comments_len = FLAC__STREAM_METADATA_VORBIS_COMMENT_NUM_COMMENTS_LEN / 8;
	FLAC__byte buffer[4]; /* magic number is asserted below */

	FLAC__ASSERT(FLAC__STREAM_METADATA_VORBIS_COMMENT_NUM_COMMENTS_LEN / 8 == 4);
	FLAC__ASSERT(0 != file);

	if(FLAC__METADATA_SIMPLE_ITERATOR_STATUS_OK != (status = read_metadata_block_data_vorbis_comment_entry_(file, &(block->vendor_string))))
		return status;

	if(fread(buffer, 1, num_comments_len, file) != num_comments_len)
		return FLAC__METADATA_SIMPLE_ITERATOR_STATUS_READ_ERROR;
	block->num_comments = unpack_uint32_little_endian_(buffer, num_comments_len);

	if(block->num_comments == 0) {
		block->comments = 0;
	}
	else if(0 == (block->comments = calloc(sizeof(FLAC__StreamMetadata_VorbisComment_Entry), block->num_comments)))
		return FLAC__METADATA_SIMPLE_ITERATOR_STATUS_MEMORY_ALLOCATION_ERROR;

	for(i = 0; i < block->num_comments; i++) {
		if(FLAC__METADATA_SIMPLE_ITERATOR_STATUS_OK != (status = read_metadata_block_data_vorbis_comment_entry_(file, block->comments + i)))
			return status;
	}

	return FLAC__METADATA_SIMPLE_ITERATOR_STATUS_OK;
}

FLAC__bool write_metadata_block_header_(FILE *file, FLAC__Metadata_SimpleIteratorStatus *status, const FLAC__StreamMetadata *block)
{
	FLAC__byte buffer[FLAC__STREAM_METADATA_HEADER_LENGTH];

	FLAC__ASSERT(0 != file);
	FLAC__ASSERT(0 != status);
	FLAC__ASSERT(block->length < (1u << FLAC__STREAM_METADATA_LENGTH_LEN));

	buffer[0] = (block->is_last? 0x80 : 0) | (FLAC__byte)block->type;
	pack_uint32_(block->length, buffer + 1, 3);

	if(fwrite(buffer, 1, FLAC__STREAM_METADATA_HEADER_LENGTH, file) != FLAC__STREAM_METADATA_HEADER_LENGTH) {
		*status = FLAC__METADATA_SIMPLE_ITERATOR_STATUS_WRITE_ERROR;
		return false;
	}

	return true;
}

FLAC__bool write_metadata_block_data_(FILE *file, FLAC__Metadata_SimpleIteratorStatus *status, const FLAC__StreamMetadata *block)
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

FLAC__Metadata_SimpleIteratorStatus write_metadata_block_data_streaminfo_(FILE *file, const FLAC__StreamMetadata_StreamInfo *block)
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
	buffer[13] = (FLAC__byte)(((bps1 & 0x0f) << 4) | ((block->total_samples >> 32) & 0x0f));
	pack_uint32_((FLAC__uint32)block->total_samples, buffer+14, 4);
	memcpy(buffer+18, block->md5sum, 16);

	if(fwrite(buffer, 1, FLAC__STREAM_METADATA_STREAMINFO_LENGTH, file) != FLAC__STREAM_METADATA_STREAMINFO_LENGTH)
		return FLAC__METADATA_SIMPLE_ITERATOR_STATUS_WRITE_ERROR;

	return FLAC__METADATA_SIMPLE_ITERATOR_STATUS_OK;
}

FLAC__Metadata_SimpleIteratorStatus write_metadata_block_data_padding_(FILE *file, const FLAC__StreamMetadata_Padding *block, unsigned block_length)
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

FLAC__Metadata_SimpleIteratorStatus write_metadata_block_data_application_(FILE *file, const FLAC__StreamMetadata_Application *block, unsigned block_length)
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

FLAC__Metadata_SimpleIteratorStatus write_metadata_block_data_seektable_(FILE *file, const FLAC__StreamMetadata_SeekTable *block)
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

FLAC__Metadata_SimpleIteratorStatus write_metadata_block_data_vorbis_comment_(FILE *file, const FLAC__StreamMetadata_VorbisComment *block)
{
	unsigned i;
	const unsigned entry_length_len = FLAC__STREAM_METADATA_VORBIS_COMMENT_ENTRY_LENGTH_LEN / 8;
	const unsigned num_comments_len = FLAC__STREAM_METADATA_VORBIS_COMMENT_NUM_COMMENTS_LEN / 8;
	FLAC__byte buffer[4]; /* magic number is asserted below */

	FLAC__ASSERT(max(FLAC__STREAM_METADATA_VORBIS_COMMENT_ENTRY_LENGTH_LEN, FLAC__STREAM_METADATA_VORBIS_COMMENT_NUM_COMMENTS_LEN) / 8 == 4);
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

FLAC__bool write_metadata_block_stationary_(FLAC__Metadata_SimpleIterator *iterator, const FLAC__StreamMetadata *block)
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

FLAC__bool write_metadata_block_stationary_with_padding_(FLAC__Metadata_SimpleIterator *iterator, FLAC__StreamMetadata *block, unsigned padding_length, FLAC__bool padding_is_last)
{
	FLAC__StreamMetadata *padding;

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

FLAC__bool rewrite_whole_file_(FLAC__Metadata_SimpleIterator *iterator, FLAC__StreamMetadata *block, FLAC__bool append)
{
	FILE *tempfile;
	char *tempfilename;
	int fixup_is_last_code = 0; /* 0 => no need to change any is_last flags */
	long fixup_is_last_flag_offset = -1;

	FLAC__ASSERT(0 != block || append == false);

	if(iterator->is_last) {
		if(append) {
			fixup_is_last_code = 1; /* 1 => clear the is_last flag at the following offset */
			fixup_is_last_flag_offset = iterator->offset[iterator->depth];
		}
		else if(0 == block) {
			simple_iterator_push_(iterator);
			if(!FLAC__metadata_simple_iterator_prev(iterator)) {
				(void)simple_iterator_pop_(iterator);
				return false;
			}
			fixup_is_last_code = -1; /* -1 => set the is_last the flag at the following offset */
			fixup_is_last_flag_offset = iterator->offset[iterator->depth];
			if(!simple_iterator_pop_(iterator))
				return false;
		}
	}

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

	if(!simple_iterator_copy_file_postfix_(iterator, &tempfile, &tempfilename, fixup_is_last_code, fixup_is_last_flag_offset, block==0))
		return false;

	if(append)
		return FLAC__metadata_simple_iterator_next(iterator);

	return true;
}

FLAC__bool chain_rewrite_chain_(FLAC__Metadata_Chain *chain)
{
	FILE *f;
	FLAC__Metadata_Node *node;
	FLAC__Metadata_SimpleIteratorStatus status;

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

FLAC__bool chain_rewrite_file_(FLAC__Metadata_Chain *chain, const char *tempfile_path_prefix)
{
	FILE *f, *tempfile;
	char *tempfilename;
	FLAC__Metadata_SimpleIteratorStatus status;
	const FLAC__Metadata_Node *node;

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

void simple_iterator_push_(FLAC__Metadata_SimpleIterator *iterator)
{
	FLAC__ASSERT(iterator->depth+1 < SIMPLE_ITERATOR_MAX_PUSH_DEPTH);
	iterator->offset[iterator->depth+1] = iterator->offset[iterator->depth];
	iterator->depth++;
}

FLAC__bool simple_iterator_pop_(FLAC__Metadata_SimpleIterator *iterator)
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
	FLAC__byte buffer[4];
	size_t n;
	unsigned i;

	FLAC__ASSERT(0 != f);
	FLAC__ASSERT(FLAC__STREAM_SYNC_LENGTH == 4);

	/* skip any id3v2 tag */
#if !defined _MSC_VER && !defined __MINGW32__
/*@@@ don't know how to resolve errno without using LIBC.LIB; must use MSVCRT.LIB only for plugins */
	errno = 0;
#endif
	n = fread(buffer, 1, 4, f);
#if !defined _MSC_VER && !defined __MINGW32__
/*@@@ don't know how to resolve errno without using LIBC.LIB; must use MSVCRT.LIB only for plugins */
	if(errno)
		return 1;
	else
#endif
	if(n != 4)
		return 2;
	else if(0 == memcmp(buffer, "ID3", 3)) {
		unsigned tag_length = 0;

		/* skip to the tag length */
		if(fseek(f, 2, SEEK_CUR) < 0)
			return 1;

		/* read the length */
		for(i = 0; i < 4; i++) {
			if(fread(buffer, 1, 1, f) < 1 || buffer[0] & 0x80)
				return 1;
			tag_length <<= 7;
			tag_length |= (buffer[0] & 0x7f);
		}

		/* skip the rest of the tag */
		if(fseek(f, tag_length, SEEK_CUR) < 0)
			return 1;

		/* read the stream sync code */
#if !defined _MSC_VER && !defined __MINGW32__
/*@@@ don't know how to resolve errno without using LIBC.LIB; must use MSVCRT.LIB only for plugins */
		errno = 0;
#endif
		n = fread(buffer, 1, 4, f);
#if !defined _MSC_VER && !defined __MINGW32__
/*@@@ don't know how to resolve errno without using LIBC.LIB; must use MSVCRT.LIB only for plugins */
		if(errno)
			return 1;
		else
#endif
		if(n != 4)
			return 2;
	}

	/* check for the fLaC signature */
	if(0 == memcmp(FLAC__STREAM_SYNC_STRING, buffer, FLAC__STREAM_SYNC_LENGTH))
		return 0;
	else
		return 2;
}

FLAC__bool simple_iterator_copy_file_prefix_(FLAC__Metadata_SimpleIterator *iterator, FILE **tempfile, char **tempfilename, FLAC__bool append)
{
	const long offset_end = append? iterator->offset[iterator->depth] + (long)FLAC__STREAM_METADATA_HEADER_LENGTH + (long)iterator->length : iterator->offset[iterator->depth];

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

FLAC__bool simple_iterator_copy_file_postfix_(FLAC__Metadata_SimpleIterator *iterator, FILE **tempfile, char **tempfilename, int fixup_is_last_code, long fixup_is_last_flag_offset, FLAC__bool backup)
{
	long save_offset = iterator->offset[iterator->depth]; /*@@@ 2G limit */
	FLAC__ASSERT(0 != *tempfile);

	if(0 != fseek(iterator->file, save_offset + FLAC__STREAM_METADATA_HEADER_LENGTH + iterator->length, SEEK_SET)) {
		cleanup_tempfile_(tempfile, tempfilename);
		iterator->status = FLAC__METADATA_SIMPLE_ITERATOR_STATUS_SEEK_ERROR;
		return false;
	}
	if(!copy_remaining_bytes_from_file_(iterator->file, *tempfile, &iterator->status)) {
		cleanup_tempfile_(tempfile, tempfilename);
		return false;
	}

	if(fixup_is_last_code != 0) {
		/*
		 * if code == 1, it means a block was appended to the end so
		 *   we have to clear the is_last flag of the previous block
		 * if code == -1, it means the last block was deleted so
		 *   we have to set the is_last flag of the previous block
		 */
		/*@@@ MAGIC NUMBERs here; we know the is_last flag is the high bit of the byte at this location */
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
		if(fixup_is_last_code > 0) {
			FLAC__ASSERT(x & 0x80);
			x &= 0x7f;
		}
		else {
			FLAC__ASSERT(!(x & 0x80));
			x |= 0x80;
		}
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

	if(!simple_iterator_prime_input_(iterator, !iterator->is_writable))
		return false;
	if(backup) {
		while(iterator->offset[iterator->depth] + (long)FLAC__STREAM_METADATA_HEADER_LENGTH + (long)iterator->length < save_offset)
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

FLAC__bool copy_n_bytes_from_file_(FILE *file, FILE *tempfile, unsigned bytes/*@@@ 4G limit*/, FLAC__Metadata_SimpleIteratorStatus *status)
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

FLAC__bool copy_remaining_bytes_from_file_(FILE *file, FILE *tempfile, FLAC__Metadata_SimpleIteratorStatus *status)
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

FLAC__bool open_tempfile_(const char *filename, const char *tempfile_path_prefix, FILE **tempfile, char **tempfilename, FLAC__Metadata_SimpleIteratorStatus *status)
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

	if(0 == (*tempfile = fopen(*tempfilename, "w+b"))) {
		*status = FLAC__METADATA_SIMPLE_ITERATOR_STATUS_ERROR_OPENING_FILE;
		return false;
	}

	return true;
}

FLAC__bool transport_tempfile_(const char *filename, FILE **tempfile, char **tempfilename, FLAC__Metadata_SimpleIteratorStatus *status)
{
	FLAC__ASSERT(0 != filename);
	FLAC__ASSERT(0 != tempfile);
	FLAC__ASSERT(0 != *tempfile);
	FLAC__ASSERT(0 != tempfilename);
	FLAC__ASSERT(0 != *tempfilename);
	FLAC__ASSERT(0 != status);

	(void)fclose(*tempfile);
	*tempfile = 0;

#if defined _MSC_VER || defined __MINGW32__
	if(unlink(filename) < 0) {
		cleanup_tempfile_(tempfile, tempfilename);
		*status = FLAC__METADATA_SIMPLE_ITERATOR_STATUS_UNLINK_ERROR;
		return false;
	}
#endif

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

FLAC__Metadata_ChainStatus get_equivalent_status_(FLAC__Metadata_SimpleIteratorStatus status)
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
		case FLAC__METADATA_SIMPLE_ITERATOR_STATUS_BAD_METADATA:
			return FLAC__METADATA_CHAIN_STATUS_BAD_METADATA;
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
