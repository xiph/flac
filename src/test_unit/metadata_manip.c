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

#include "metadata_utils.h"
#include "FLAC/assert.h"
#include "FLAC/file_decoder.h"
#include "FLAC/metadata.h"
#include "FLAC/stream_encoder.h"
#include <stdio.h>
#include <stdlib.h> /* for malloc() */
#include <string.h> /* for memcmp() */
#if defined _MSC_VER || defined __MINGW32__
#include <io.h> /* for chmod(), unlink */
#endif
#include <sys/stat.h> /* for stat(), chmod() */
#if defined _WIN32 && !defined __CYGWIN__
#else
#include <unistd.h> /* for unlink() */
#endif

/******************************************************************************
	The general strategy of these tests (for interface levels 1 and 2) is
	to create a dummy FLAC file with a known set of initial metadata
	blocks, then keep a mirror locally of what we expect the metadata to be
	after each operation.  Then testing becomes a simple matter of running
	a FLAC__FileDecoder over the dummy file after each operation, comparing
	the decoded metadata to what's in our local copy.  If there are any
	differences in the metadata,  or the actual audio data is corrupted, we
	will catch it while decoding.
******************************************************************************/

typedef struct {
	FLAC__bool error_occurred;
} decoder_client_struct;

typedef struct {
	FILE *file;
} encoder_client_struct;

typedef struct {
	FLAC__StreamMetaData *blocks[64];
	unsigned num_blocks;
} our_metadata_struct;

static const char *flacfile_ = "metadata.flac";

/* our copy of the metadata in flacfile_ */
static our_metadata_struct our_metadata_;

/* the current block number that corresponds to the position of the iterator we are testing */
static unsigned mc_our_block_number_ = 0;

static FLAC__bool die_(const char *msg)
{
	printf("ERROR: %s\n", msg);
	return false;
}

static FLAC__bool die_c_(const char *msg, FLAC__MetaData_ChainStatus status)
{
	printf("ERROR: %s\n", msg);
	printf("       status=%s\n", FLAC__MetaData_ChainStatusString[status]);
	return false;
}

static FLAC__bool die_ss_(const char *msg, FLAC__MetaData_SimpleIterator *siterator)
{
	printf("ERROR: %s\n", msg);
	printf("       status=%s\n", FLAC__MetaData_SimpleIteratorStatusString[FLAC__metadata_simple_iterator_status(siterator)]);
	return false;
}

/* functions for working with our metadata copy */

static FLAC__bool replace_in_our_metadata_(FLAC__StreamMetaData *block, unsigned position, FLAC__bool copy)
{
	unsigned i;
	FLAC__StreamMetaData *obj = block;
	FLAC__ASSERT(position < our_metadata_.num_blocks);
	if(copy) {
		if(0 == (obj = FLAC__metadata_object_copy(block)))
			return die_("during FLAC__metadata_object_copy()");
	}
	FLAC__metadata_object_delete(our_metadata_.blocks[position]);
	our_metadata_.blocks[position] = obj;

	/* set the is_last flags */
	for(i = 0; i < our_metadata_.num_blocks - 1; i++)
		our_metadata_.blocks[i]->is_last = false;
	our_metadata_.blocks[i]->is_last = true;

	return true;
}

static FLAC__bool insert_to_our_metadata_(FLAC__StreamMetaData *block, unsigned position, FLAC__bool copy)
{
	unsigned i;
	FLAC__StreamMetaData *obj = block;
	if(copy) {
		if(0 == (obj = FLAC__metadata_object_copy(block)))
			return die_("during FLAC__metadata_object_copy()");
	}
	if(position > our_metadata_.num_blocks) {
		position = our_metadata_.num_blocks;
	}
	else {
		for(i = our_metadata_.num_blocks; i > position; i--)
			our_metadata_.blocks[i] = our_metadata_.blocks[i-1];
	}
	our_metadata_.blocks[position] = obj;
	our_metadata_.num_blocks++;

	/* set the is_last flags */
	for(i = 0; i < our_metadata_.num_blocks - 1; i++)
		our_metadata_.blocks[i]->is_last = false;
	our_metadata_.blocks[i]->is_last = true;

	return true;
}

static void delete_from_our_metadata_(unsigned position)
{
	unsigned i;
	FLAC__ASSERT(position < our_metadata_.num_blocks);
	FLAC__metadata_object_delete(our_metadata_.blocks[position]);
	for(i = position; i < our_metadata_.num_blocks - 1; i++)
		our_metadata_.blocks[i] = our_metadata_.blocks[i+1];
	our_metadata_.num_blocks--;

	/* set the is_last flags */
	if(our_metadata_.num_blocks > 0) {
		for(i = 0; i < our_metadata_.num_blocks - 1; i++)
			our_metadata_.blocks[i]->is_last = false;
		our_metadata_.blocks[i]->is_last = true;
	}
}

/* function for comparing our metadata to a FLAC__MetaData_Chain */

static FLAC__bool compare_chain_(FLAC__MetaData_Chain *chain, unsigned current_position, FLAC__StreamMetaData *current_block)
{
	unsigned i;
	FLAC__MetaData_Iterator *iterator;
	FLAC__StreamMetaData *block;
	FLAC__bool next_ok = true;

	FLAC__ASSERT(0 != chain);

	printf("\tcomparing chain... ");
	fflush(stdout);

	if(0 == (iterator = FLAC__metadata_iterator_new()))
		return die_("allocating memory for iterator");

	FLAC__metadata_iterator_init(iterator, chain);

	i = 0;
	do {
		printf("%u... ", i);
		fflush(stdout);

		if(0 == (block = FLAC__metadata_iterator_get_block(iterator))) {
			FLAC__metadata_iterator_delete(iterator);
			return die_("getting block from iterator");
		}

		if(!compare_block_(our_metadata_.blocks[i], block)) {
			FLAC__metadata_iterator_delete(iterator);
			return die_("metadata block mismatch");
		}

		i++;
		next_ok = FLAC__metadata_iterator_next(iterator);
	} while(i < our_metadata_.num_blocks && next_ok);

	FLAC__metadata_iterator_delete(iterator);

	if(next_ok)
		return die_("chain has more blocks than expected");

	if(i < our_metadata_.num_blocks)
		return die_("short block count in chain");

	if(0 != current_block) {
		printf("CURRENT_POSITION... ");
		fflush(stdout);

		if(!compare_block_(our_metadata_.blocks[current_position], current_block))
			return die_("metadata block mismatch");
	}

	printf("PASSED\n");

	return true;
}

/* decoder callbacks for checking the file */

static FLAC__StreamDecoderWriteStatus decoder_write_callback_(const FLAC__FileDecoder *decoder, const FLAC__Frame *frame, const FLAC__int32 *buffer[], void *client_data)
{
	(void)decoder, (void)buffer, (void)client_data;

	if(
		(frame->header.number_type == FLAC__FRAME_NUMBER_TYPE_FRAME_NUMBER && frame->header.number.frame_number == 0) ||
		(frame->header.number_type == FLAC__FRAME_NUMBER_TYPE_SAMPLE_NUMBER && frame->header.number.sample_number == 0)
	) {
		printf("content... ");
		fflush(stdout);
	}

	return FLAC__STREAM_DECODER_WRITE_CONTINUE;
}

static void decoder_error_callback_(const FLAC__FileDecoder *decoder, FLAC__StreamDecoderErrorStatus status, void *client_data)
{
	decoder_client_struct *dcd = (decoder_client_struct*)client_data;
	(void)decoder;

	dcd->error_occurred = true;
	printf("ERROR: got error callback, status = %s (%u)\n", FLAC__StreamDecoderErrorStatusString[status], (unsigned)status);
}

/* this version pays no attention to the metadata */
static void decoder_metadata_callback_null_(const FLAC__FileDecoder *decoder, const FLAC__StreamMetaData *metadata, void *client_data)
{
	(void)decoder, (void)metadata, (void)client_data;

	printf("%d... ", mc_our_block_number_);
	fflush(stdout);

	mc_our_block_number_++;
}

/* this version is used when we want to compare to our metadata copy */
static void decoder_metadata_callback_compare_(const FLAC__FileDecoder *decoder, const FLAC__StreamMetaData *metadata, void *client_data)
{
	decoder_client_struct *dcd = (decoder_client_struct*)client_data;

	(void)decoder;

	/* don't bother checking if we've already hit an error */
	if(dcd->error_occurred)
		return;

	printf("%d... ", mc_our_block_number_);
	fflush(stdout);

	if(mc_our_block_number_ >= our_metadata_.num_blocks) {
		(void)die_("got more metadata blocks than expected");
		dcd->error_occurred = true;
	}
	else {
		if(!compare_block_(our_metadata_.blocks[mc_our_block_number_], metadata)) {
			(void)die_("metadata block mismatch");
			dcd->error_occurred = true;
		}
	}
	mc_our_block_number_++;
}

static FLAC__StreamEncoderWriteStatus encoder_write_callback_(const FLAC__StreamEncoder *encoder, const FLAC__byte buffer[], unsigned bytes, unsigned samples, unsigned current_frame, void *client_data)
{
	encoder_client_struct *ecd = (encoder_client_struct*)client_data;

	(void)encoder, (void)samples, (void)current_frame;

	if(fwrite(buffer, 1, bytes, ecd->file) != bytes)
		return FLAC__STREAM_ENCODER_WRITE_FATAL_ERROR;
	else
		return FLAC__STREAM_ENCODER_WRITE_OK;
}

static void encoder_metadata_callback_(const FLAC__StreamEncoder *encoder, const FLAC__StreamMetaData *metadata, void *client_data)
{
	(void)encoder, (void)metadata, (void)client_data;
}

static FLAC__bool generate_file_(const char *input_filename)
{
	FLAC__StreamEncoder *encoder;
	FLAC__StreamMetaData streaminfo, padding;
	encoder_client_struct encoder_client_data;
	FILE *file;
	FLAC__byte buffer[4096];
	FLAC__int32 samples[4096];
	unsigned i, n;

	FLAC__ASSERT(0 != input_filename);

	printf("generating FLAC file for test\n");

	while(our_metadata_.num_blocks > 0)
		delete_from_our_metadata_(0);

	streaminfo.is_last = false;
	streaminfo.type = FLAC__METADATA_TYPE_STREAMINFO;
	streaminfo.length = FLAC__STREAM_METADATA_STREAMINFO_LENGTH;
	streaminfo.data.stream_info.min_blocksize = 576;
	streaminfo.data.stream_info.max_blocksize = 576;
	streaminfo.data.stream_info.min_framesize = 0;
	streaminfo.data.stream_info.max_framesize = 0;
	streaminfo.data.stream_info.sample_rate = 44100;
	streaminfo.data.stream_info.channels = 1;
	streaminfo.data.stream_info.bits_per_sample = 8;
	streaminfo.data.stream_info.total_samples = 0;
	memset(streaminfo.data.stream_info.md5sum, 0, 16);

	padding.is_last = true;
	padding.type = FLAC__METADATA_TYPE_PADDING;
	padding.length = 1234;

	if(!insert_to_our_metadata_(&streaminfo, 0, /*copy=*/true) || !insert_to_our_metadata_(&padding, 1, /*copy=*/true))
		return die_("priming our metadata");

	if(0 == (file = fopen(input_filename, "rb")))
		return die_("opening input file");
	if(0 == (encoder_client_data.file = fopen(flacfile_, "wb"))) {
		fclose(file);
		return die_("opening output file");
	}

	encoder = FLAC__stream_encoder_new();
	if(0 == encoder) {
		fclose(file);
		fclose(encoder_client_data.file);
		return die_("creating the encoder instance"); 
	}

	FLAC__stream_encoder_set_streamable_subset(encoder, true);
	FLAC__stream_encoder_set_do_mid_side_stereo(encoder, false);
	FLAC__stream_encoder_set_loose_mid_side_stereo(encoder, false);
	FLAC__stream_encoder_set_channels(encoder, streaminfo.data.stream_info.channels);
	FLAC__stream_encoder_set_bits_per_sample(encoder, streaminfo.data.stream_info.bits_per_sample);
	FLAC__stream_encoder_set_sample_rate(encoder, streaminfo.data.stream_info.sample_rate);
	FLAC__stream_encoder_set_blocksize(encoder, streaminfo.data.stream_info.min_blocksize);
	FLAC__stream_encoder_set_max_lpc_order(encoder, 0);
	FLAC__stream_encoder_set_qlp_coeff_precision(encoder, 0);
	FLAC__stream_encoder_set_do_qlp_coeff_prec_search(encoder, false);
	FLAC__stream_encoder_set_do_escape_coding(encoder, false);
	FLAC__stream_encoder_set_do_exhaustive_model_search(encoder, false);
	FLAC__stream_encoder_set_min_residual_partition_order(encoder, 0);
	FLAC__stream_encoder_set_max_residual_partition_order(encoder, 0);
	FLAC__stream_encoder_set_rice_parameter_search_dist(encoder, 0);
	FLAC__stream_encoder_set_total_samples_estimate(encoder, streaminfo.data.stream_info.total_samples);
	FLAC__stream_encoder_set_seek_table(encoder, 0);
	FLAC__stream_encoder_set_padding(encoder, padding.length);
	FLAC__stream_encoder_set_last_metadata_is_last(encoder, true);
	FLAC__stream_encoder_set_write_callback(encoder, encoder_write_callback_);
	FLAC__stream_encoder_set_metadata_callback(encoder, encoder_metadata_callback_);
	FLAC__stream_encoder_set_client_data(encoder, &encoder_client_data);

	if(FLAC__stream_encoder_init(encoder) != FLAC__STREAM_ENCODER_OK) {
		fclose(file);
		fclose(encoder_client_data.file);
		return die_("initializing encoder");
	}

	while(!feof(file)) {
		n = fread(buffer, 1, sizeof(buffer), file);
		if(n > 0) {
			for(i = 0; i < n; i++)
				samples[i] = (FLAC__int32)((signed char)buffer[i]);

			/* NOTE: some versions of GCC can't figure out const-ness right and will give you an 'incompatible pointer type' warning on arg 2 here: */
			if(!FLAC__stream_encoder_process_interleaved(encoder, samples, n)) {
				fclose(file);
				fclose(encoder_client_data.file);
				return die_("during encoding");
			}
		}
		else if(!feof(file)) {
			fclose(file);
			fclose(encoder_client_data.file);
			return die_("reading input file");
		}
	}

	fclose(file);
	fclose(encoder_client_data.file);

	if(FLAC__stream_encoder_get_state(encoder) == FLAC__STREAM_ENCODER_OK)
		FLAC__stream_encoder_finish(encoder);
	FLAC__stream_encoder_delete(encoder);

	return true;
}

static FLAC__bool test_file_(const char *filename, void (*metadata_callback)(const FLAC__FileDecoder *decoder, const FLAC__StreamMetaData *metadata, void *client_data))
{
	FLAC__FileDecoder *decoder;
	decoder_client_struct decoder_client_data;

	FLAC__ASSERT(0 != filename);
	FLAC__ASSERT(0 != metadata_callback);

	mc_our_block_number_ = 0;
	decoder_client_data.error_occurred = false;

	printf("\ttesting '%s'... ", filename);
	fflush(stdout);

	if(0 == (decoder = FLAC__file_decoder_new()))
		return die_("couldn't allocate memory");

	FLAC__file_decoder_set_md5_checking(decoder, true);
	FLAC__file_decoder_set_filename(decoder, filename);
	FLAC__file_decoder_set_write_callback(decoder, decoder_write_callback_);
	FLAC__file_decoder_set_metadata_callback(decoder, metadata_callback);
	FLAC__file_decoder_set_error_callback(decoder, decoder_error_callback_);
	FLAC__file_decoder_set_client_data(decoder, &decoder_client_data);
	FLAC__file_decoder_set_metadata_respond_all(decoder);
	if(FLAC__file_decoder_init(decoder) != FLAC__FILE_DECODER_OK) {
		FLAC__file_decoder_finish(decoder);
		FLAC__file_decoder_delete(decoder);
		return die_("initializing decoder\n");
	}
	if(!FLAC__file_decoder_process_whole_file(decoder)) {
		FLAC__file_decoder_finish(decoder);
		FLAC__file_decoder_delete(decoder);
		return die_("decoding file\n");
	}

	FLAC__file_decoder_finish(decoder);
	FLAC__file_decoder_delete(decoder);

	if(decoder_client_data.error_occurred)
		return false;

	if(mc_our_block_number_ != our_metadata_.num_blocks)
		return die_("short metadata block count");

	printf("PASSED\n");
	return true;
}

static FLAC__bool change_stats_(const char *filename, FLAC__bool read_only)
{
	struct stat stats;

	if(0 == stat(filename, &stats)) {
		if(read_only) {
			stats.st_mode &= ~S_IWUSR;
			stats.st_mode &= ~S_IWGRP;
			stats.st_mode &= ~S_IWOTH;
		}
		else {
			stats.st_mode |= S_IWUSR;
			stats.st_mode |= S_IWGRP;
			stats.st_mode |= S_IWOTH;
		}
		if(0 != chmod(filename, stats.st_mode))
			return die_("during chmod()");
	}
	else
		return die_("during stat()");

	return true;
}

static FLAC__bool remove_file_(const char *filename)
{
	while(our_metadata_.num_blocks > 0)
		delete_from_our_metadata_(0);

	if(!change_stats_(filename, /*read_only=*/false) || 0 != unlink(filename))
		return die_("removing file");

	return true;
}

static FLAC__bool test_level_0_(const char *progname)
{
	FLAC__StreamMetaData_StreamInfo streaminfo;

	printf("\n\n++++++ testing level 0 interface\n");

	if(!generate_file_(progname))
		return false;

	if(!test_file_(flacfile_, decoder_metadata_callback_null_))
		return false;

	if(!FLAC__metadata_get_streaminfo(flacfile_, &streaminfo))
		return die_("during FLAC__metadata_get_streaminfo()");

	/* check to see if some basic data matches (c.f. generate_file_()) */
	if(streaminfo.channels != 1)
		return die_("mismatch in streaminfo.channels");
	if(streaminfo.bits_per_sample != 8)
		return die_("mismatch in streaminfo.bits_per_sample");
	if(streaminfo.sample_rate != 44100)
		return die_("mismatch in streaminfo.sample_rate");
	if(streaminfo.min_blocksize != 576)
		return die_("mismatch in streaminfo.min_blocksize");
	if(streaminfo.max_blocksize != 576)
		return die_("mismatch in streaminfo.max_blocksize");

	if(!remove_file_(flacfile_))
		return false;

	return true;
}

static FLAC__bool test_level_1_(const char *progname)
{
	FLAC__MetaData_SimpleIterator *siterator;
	FLAC__StreamMetaData *block, *app, *padding;
	FLAC__byte data[1000];
	unsigned our_current_position = 0;

	printf("\n\n++++++ testing level 1 interface\n");

	/************************************************************/

	printf("simple iterator on read-only file\n");

	if(!generate_file_(progname))
		return false;

	if(!change_stats_(flacfile_, /*read_only=*/true))
		return false;

	if(!test_file_(flacfile_, decoder_metadata_callback_null_))
		return false;

	if(0 == (siterator = FLAC__metadata_simple_iterator_new()))
		return die_("FLAC__metadata_simple_iterator_new()");

	if(!FLAC__metadata_simple_iterator_init(siterator, flacfile_, false))
		return die_("ERROR: FLAC__metadata_simple_iterator_init()\n");

	printf("is writable = %u\n", (unsigned)FLAC__metadata_simple_iterator_is_writable(siterator));
	if(FLAC__metadata_simple_iterator_is_writable(siterator))
		return die_("iterator claims file is writable when it should not be\n");

	printf("iterate forwards\n");

	if(FLAC__metadata_simple_iterator_get_block_type(siterator) != FLAC__METADATA_TYPE_STREAMINFO)
		return die_("expected STREAMINFO type from FLAC__metadata_simple_iterator_get_block_type()");
	if(0 == (block = FLAC__metadata_simple_iterator_get_block(siterator)))
		return die_("getting block 0");
	if(block->type != FLAC__METADATA_TYPE_STREAMINFO)
		return die_("expected STREAMINFO type");
	if(block->is_last)
		return die_("expected is_last to be false");
	if(block->length != FLAC__STREAM_METADATA_STREAMINFO_LENGTH)
		return die_("bad STREAMINFO length");
	/* check to see if some basic data matches (c.f. generate_file_()) */
	if(block->data.stream_info.channels != 1)
		return die_("mismatch in channels");
	if(block->data.stream_info.bits_per_sample != 8)
		return die_("mismatch in bits_per_sample");
	if(block->data.stream_info.sample_rate != 44100)
		return die_("mismatch in sample_rate");
	if(block->data.stream_info.min_blocksize != 576)
		return die_("mismatch in min_blocksize");
	if(block->data.stream_info.max_blocksize != 576)
		return die_("mismatch in max_blocksize");

	if(!FLAC__metadata_simple_iterator_next(siterator))
		return die_("forward iterator ended early");
	our_current_position++;

	if(FLAC__metadata_simple_iterator_get_block_type(siterator) != FLAC__METADATA_TYPE_PADDING)
		return die_("expected PADDING type from FLAC__metadata_simple_iterator_get_block_type()");
	if(0 == (block = FLAC__metadata_simple_iterator_get_block(siterator)))
		return die_("getting block 1");
	if(block->type != FLAC__METADATA_TYPE_PADDING)
		return die_("expected PADDING type");
	if(!block->is_last)
		return die_("expected is_last to be true");
	/* check to see if some basic data matches (c.f. generate_file_()) */
	if(block->length != 1234)
		return die_("bad STREAMINFO length");

	if(FLAC__metadata_simple_iterator_next(siterator))
		return die_("forward iterator returned true but should have returned false");

	printf("iterate backwards\n");
	if(!FLAC__metadata_simple_iterator_prev(siterator))
		return die_("reverse iterator ended early");
	if(FLAC__metadata_simple_iterator_prev(siterator))
		return die_("reverse iterator returned true but should have returned false");

	printf("testing FLAC__metadata_simple_iterator_set_block() on read-only file...\n");

	if(!FLAC__metadata_simple_iterator_set_block(siterator, (void*)99, false))
		printf("PASSED.  FLAC__metadata_simple_iterator_set_block() returned false like it should\n");
	else
		return die_("FLAC__metadata_simple_iterator_set_block() returned true but shouldn't have");

	FLAC__metadata_simple_iterator_delete(siterator);

	/************************************************************/

	printf("simple iterator on writable file\n");

	if(!change_stats_(flacfile_, /*read-only=*/false))
		return false;

	printf("creating APPLICATION block\n");

	if(0 == (app = FLAC__metadata_object_new(FLAC__METADATA_TYPE_APPLICATION)))
		return die_("FLAC__metadata_object_new(FLAC__METADATA_TYPE_APPLICATION)");
	memcpy(app->data.application.id, "duh", (FLAC__STREAM_METADATA_APPLICATION_ID_LEN/8));

	printf("creating PADDING block\n");

	if(0 == (padding = FLAC__metadata_object_new(FLAC__METADATA_TYPE_PADDING)))
		return die_("FLAC__metadata_object_new(FLAC__METADATA_TYPE_PADDING)");
	padding->length = 20;

	if(0 == (siterator = FLAC__metadata_simple_iterator_new()))
		return die_("FLAC__metadata_simple_iterator_new()");

	if(!FLAC__metadata_simple_iterator_init(siterator, flacfile_, /*preserve_file_stats=*/false))
		return die_("ERROR: FLAC__metadata_simple_iterator_init()\n");
	our_current_position = 0;

	printf("is writable = %u\n", (unsigned)FLAC__metadata_simple_iterator_is_writable(siterator));

	printf("[S]P\ttry to write over STREAMINFO block...\n");
	if(!FLAC__metadata_simple_iterator_set_block(siterator, app, false))
		printf("FLAC__metadata_simple_iterator_set_block() returned false like it should\n");
	else
		return die_("FLAC__metadata_simple_iterator_set_block() returned true but shouldn't have");

	printf("[S]P\tnext\n");
	if(!FLAC__metadata_simple_iterator_next(siterator))
		return die_("iterator ended early\n");
	our_current_position++;

	printf("S[P]\tinsert PADDING after, don't expand into padding\n");
	padding->length = 25;
	if(!FLAC__metadata_simple_iterator_insert_block_after(siterator, padding, false))
		return die_ss_("FLAC__metadata_simple_iterator_insert_block_after(siterator, padding, false)", siterator);
	if(!insert_to_our_metadata_(padding, ++our_current_position, /*copy=*/true))
		return false;

	printf("SP[P]\tprev\n");
	if(!FLAC__metadata_simple_iterator_prev(siterator))
		return die_("iterator ended early\n");
	our_current_position--;

	printf("S[P]P\tprev\n");
	if(!FLAC__metadata_simple_iterator_prev(siterator))
		return die_("iterator ended early\n");
	our_current_position--;

	printf("[S]PP\tinsert PADDING after, don't expand into padding\n");
	padding->length = 30;
	if(!FLAC__metadata_simple_iterator_insert_block_after(siterator, padding, false))
		return die_ss_("FLAC__metadata_simple_iterator_insert_block_after(siterator, padding, false)", siterator);
	if(!insert_to_our_metadata_(padding, ++our_current_position, /*copy=*/true))
		return false;

	if(!test_file_(flacfile_, decoder_metadata_callback_compare_))
		return false;
	
	printf("S[P]PP\tprev\n");
	if(!FLAC__metadata_simple_iterator_prev(siterator))
		return die_("iterator ended early\n");
	our_current_position--;

	printf("[S]PPP\tdelete (STREAMINFO block), must fail\n");
	if(FLAC__metadata_simple_iterator_delete_block(siterator, false))
		return die_ss_("FLAC__metadata_simple_iterator_delete_block(siterator, false) should have returned false", siterator);

	if(!test_file_(flacfile_, decoder_metadata_callback_compare_))
		return false;

	printf("[S]PPP\tnext\n");
	if(!FLAC__metadata_simple_iterator_next(siterator))
		return die_("iterator ended early\n");
	our_current_position++;

	printf("S[P]PP\tdelete (middle block), replace with padding\n");
	if(!FLAC__metadata_simple_iterator_delete_block(siterator, true))
		return die_ss_("FLAC__metadata_simple_iterator_delete_block(siterator, true)", siterator);
	our_current_position--;

	printf("[S]PPP\tnext\n");
	if(!FLAC__metadata_simple_iterator_next(siterator))
		return die_("iterator ended early\n");
	our_current_position++;

	printf("S[P]PP\tdelete (middle block), don't replace with padding\n");
	if(!FLAC__metadata_simple_iterator_delete_block(siterator, false))
		return die_ss_("FLAC__metadata_simple_iterator_delete_block(siterator, false)", siterator);
	delete_from_our_metadata_(our_current_position--);

	if(!test_file_(flacfile_, decoder_metadata_callback_compare_))
		return false;

	printf("[S]PP\tnext\n");
	if(!FLAC__metadata_simple_iterator_next(siterator))
		return die_("iterator ended early\n");
	our_current_position++;

	printf("S[P]P\tnext\n");
	if(!FLAC__metadata_simple_iterator_next(siterator))
		return die_("iterator ended early\n");
	our_current_position++;

	printf("SP[P]\tdelete (last block), replace with padding\n");
	if(!FLAC__metadata_simple_iterator_delete_block(siterator, true))
		return die_ss_("FLAC__metadata_simple_iterator_delete_block(siterator, false)", siterator);
	our_current_position--;

	if(!test_file_(flacfile_, decoder_metadata_callback_compare_))
		return false;

	printf("S[P]P\tnext\n");
	if(!FLAC__metadata_simple_iterator_next(siterator))
		return die_("iterator ended early\n");
	our_current_position++;

	printf("SP[P]\tdelete (last block), don't replace with padding\n");
	if(!FLAC__metadata_simple_iterator_delete_block(siterator, false))
		return die_ss_("FLAC__metadata_simple_iterator_delete_block(siterator, false)", siterator);
	delete_from_our_metadata_(our_current_position--);

	if(!test_file_(flacfile_, decoder_metadata_callback_compare_))
		return false;

	printf("S[P]\tprev\n");
	if(!FLAC__metadata_simple_iterator_prev(siterator))
		return die_("iterator ended early\n");
	our_current_position--;

	printf("[S]P\tset STREAMINFO (change sample rate)\n");
	FLAC__ASSERT(our_current_position == 0);
	block = FLAC__metadata_simple_iterator_get_block(siterator);
	block->data.stream_info.sample_rate = 32000;
	if(!replace_in_our_metadata_(block, our_current_position, /*copy=*/true))
		return die_("copying object");
	if(!FLAC__metadata_simple_iterator_set_block(siterator, block, false))
		return die_ss_("FLAC__metadata_simple_iterator_set_block(siterator, block, false)", siterator);
	FLAC__metadata_object_delete(block);

	if(!test_file_(flacfile_, decoder_metadata_callback_compare_))
		return false;

	printf("[S]P\tinsert APPLICATION after, expand into padding of exceeding size\n");
	app->data.application.id[0] = 'e'; /* twiddle the id so that our comparison doesn't miss transposition */
	if(!FLAC__metadata_simple_iterator_insert_block_after(siterator, app, true))
		return die_ss_("FLAC__metadata_simple_iterator_insert_block_after(siterator, app, true)", siterator);
	if(!insert_to_our_metadata_(app, ++our_current_position, /*copy=*/true))
		return false;
	our_metadata_.blocks[our_current_position+1]->length -= (FLAC__STREAM_METADATA_APPLICATION_ID_LEN/8) + app->length;

	if(!test_file_(flacfile_, decoder_metadata_callback_compare_))
		return false;

	printf("S[A]P\tnext\n");
	if(!FLAC__metadata_simple_iterator_next(siterator))
		return die_("iterator ended early\n");
	our_current_position++;

	printf("SA[P]\tset APPLICATION, expand into padding of exceeding size\n");
	app->data.application.id[0] = 'f'; /* twiddle the id */
	if(!FLAC__metadata_simple_iterator_set_block(siterator, app, true))
		return die_ss_("FLAC__metadata_simple_iterator_set_block(siterator, app, true)", siterator);
	if(!insert_to_our_metadata_(app, our_current_position, /*copy=*/true))
		return false;
	our_metadata_.blocks[our_current_position+1]->length -= (FLAC__STREAM_METADATA_APPLICATION_ID_LEN/8) + app->length;

	if(!test_file_(flacfile_, decoder_metadata_callback_compare_))
		return false;

	printf("SA[A]P\tset APPLICATION (grow), don't expand into padding\n");
	app->data.application.id[0] = 'g'; /* twiddle the id */
	if(!FLAC__metadata_object_application_set_data(app, data, sizeof(data), true))
		return die_("setting APPLICATION data");
	if(!replace_in_our_metadata_(app, our_current_position, /*copy=*/true))
		return die_("copying object");
	if(!FLAC__metadata_simple_iterator_set_block(siterator, app, false))
		return die_ss_("FLAC__metadata_simple_iterator_set_block(siterator, app, false)", siterator);

	if(!test_file_(flacfile_, decoder_metadata_callback_compare_))
		return false;

	printf("SA[A]P\tset APPLICATION (shrink), don't fill in with padding\n");
	app->data.application.id[0] = 'h'; /* twiddle the id */
	if(!FLAC__metadata_object_application_set_data(app, data, 12, true))
		return die_("setting APPLICATION data");
	if(!replace_in_our_metadata_(app, our_current_position, /*copy=*/true))
		return die_("copying object");
	if(!FLAC__metadata_simple_iterator_set_block(siterator, app, false))
		return die_ss_("FLAC__metadata_simple_iterator_set_block(siterator, app, false)", siterator);

	if(!test_file_(flacfile_, decoder_metadata_callback_compare_))
		return false;

	printf("SA[A]P\tset APPLICATION (grow), expand into padding of exceeding size\n");
	app->data.application.id[0] = 'i'; /* twiddle the id */
	if(!FLAC__metadata_object_application_set_data(app, data, sizeof(data), true))
		return die_("setting APPLICATION data");
	if(!replace_in_our_metadata_(app, our_current_position, /*copy=*/true))
		return die_("copying object");
	our_metadata_.blocks[our_current_position+1]->length -= (sizeof(data) - 12);
	if(!FLAC__metadata_simple_iterator_set_block(siterator, app, true))
		return die_ss_("FLAC__metadata_simple_iterator_set_block(siterator, app, true)", siterator);

	if(!test_file_(flacfile_, decoder_metadata_callback_compare_))
		return false;

	printf("SA[A]P\tset APPLICATION (shrink), fill in with padding\n");
	app->data.application.id[0] = 'j'; /* twiddle the id */
	if(!FLAC__metadata_object_application_set_data(app, data, 23, true))
		return die_("setting APPLICATION data");
	if(!replace_in_our_metadata_(app, our_current_position, /*copy=*/true))
		return die_("copying object");
	if(!insert_to_our_metadata_(padding, our_current_position+1, /*copy=*/true))
		return die_("copying object");
	our_metadata_.blocks[our_current_position+1]->length = sizeof(data) - 23 - 4;
	if(!FLAC__metadata_simple_iterator_set_block(siterator, app, true))
		return die_ss_("FLAC__metadata_simple_iterator_set_block(siterator, app, true)", siterator);

	if(!test_file_(flacfile_, decoder_metadata_callback_compare_))
		return false;

	printf("SA[A]PP\tnext\n");
	if(!FLAC__metadata_simple_iterator_next(siterator))
		return die_("iterator ended early\n");
	our_current_position++;

	printf("SAA[P]P\tnext\n");
	if(!FLAC__metadata_simple_iterator_next(siterator))
		return die_("iterator ended early\n");
	our_current_position++;

	printf("SAAP[P]\tset PADDING (shrink), don't fill in with padding\n");
	padding->length = 5;
	if(!replace_in_our_metadata_(padding, our_current_position, /*copy=*/true))
		return die_("copying object");
	if(!FLAC__metadata_simple_iterator_set_block(siterator, padding, false))
		return die_ss_("FLAC__metadata_simple_iterator_set_block(siterator, padding, false)", siterator);

	if(!test_file_(flacfile_, decoder_metadata_callback_compare_))
		return false;

	printf("SAAP[P]\tset APPLICATION (grow)\n");
	app->data.application.id[0] = 'k'; /* twiddle the id */
	if(!replace_in_our_metadata_(app, our_current_position, /*copy=*/true))
		return die_("copying object");
	if(!FLAC__metadata_simple_iterator_set_block(siterator, app, false))
		return die_ss_("FLAC__metadata_simple_iterator_set_block(siterator, app, false)", siterator);

	if(!test_file_(flacfile_, decoder_metadata_callback_compare_))
		return false;

	printf("SAAP[A]\tset PADDING (equal)\n");
	padding->length = 27;
	if(!replace_in_our_metadata_(padding, our_current_position, /*copy=*/true))
		return die_("copying object");
	if(!FLAC__metadata_simple_iterator_set_block(siterator, padding, false))
		return die_ss_("FLAC__metadata_simple_iterator_set_block(siterator, padding, false)", siterator);

	if(!test_file_(flacfile_, decoder_metadata_callback_compare_))
		return false;

	printf("SAAP[P]\tprev\n");
	if(!FLAC__metadata_simple_iterator_prev(siterator))
		return die_("iterator ended early\n");
	our_current_position--;

	printf("SAA[P]P\tdelete (middle block), don't replace with padding\n");
	if(!FLAC__metadata_simple_iterator_delete_block(siterator, false))
		return die_ss_("FLAC__metadata_simple_iterator_delete_block(siterator, false)", siterator);
	delete_from_our_metadata_(our_current_position--);

	if(!test_file_(flacfile_, decoder_metadata_callback_compare_))
		return false;

	printf("SA[A]P\tdelete (middle block), don't replace with padding\n");
	if(!FLAC__metadata_simple_iterator_delete_block(siterator, false))
		return die_ss_("FLAC__metadata_simple_iterator_delete_block(siterator, false)", siterator);
	delete_from_our_metadata_(our_current_position--);

	if(!test_file_(flacfile_, decoder_metadata_callback_compare_))
		return false;

	printf("S[A]P\tnext\n");
	if(!FLAC__metadata_simple_iterator_next(siterator))
		return die_("iterator ended early\n");
	our_current_position++;

	printf("SA[P]\tinsert PADDING after\n");
	padding->length = 5;
	if(!FLAC__metadata_simple_iterator_insert_block_after(siterator, padding, false))
		return die_ss_("FLAC__metadata_simple_iterator_insert_block_after(siterator, padding, false)", siterator);
	if(!insert_to_our_metadata_(padding, ++our_current_position, /*copy=*/true))
		return false;

	if(!test_file_(flacfile_, decoder_metadata_callback_compare_))
		return false;

	printf("SAP[P]\tprev\n");
	if(!FLAC__metadata_simple_iterator_prev(siterator))
		return die_("iterator ended early\n");
	our_current_position--;

	printf("SA[P]P\tprev\n");
	if(!FLAC__metadata_simple_iterator_prev(siterator))
		return die_("iterator ended early\n");
	our_current_position--;

	printf("S[A]PP\tset APPLICATION (grow), try to expand into padding which is too small\n");
	if(!FLAC__metadata_object_application_set_data(app, data, 32, true))
		return die_("setting APPLICATION data");
	if(!replace_in_our_metadata_(app, our_current_position, /*copy=*/true))
		return die_("copying object");
	if(!FLAC__metadata_simple_iterator_set_block(siterator, app, true))
		return die_ss_("FLAC__metadata_simple_iterator_set_block(siterator, app, true)", siterator);

	if(!test_file_(flacfile_, decoder_metadata_callback_compare_))
		return false;

	printf("S[A]PP\tset APPLICATION (grow), try to expand into padding which is 'close' but still too small\n");
	if(!FLAC__metadata_object_application_set_data(app, data, 60, true))
		return die_("setting APPLICATION data");
	if(!replace_in_our_metadata_(app, our_current_position, /*copy=*/true))
		return die_("copying object");
	if(!FLAC__metadata_simple_iterator_set_block(siterator, app, true))
		return die_ss_("FLAC__metadata_simple_iterator_set_block(siterator, app, true)", siterator);

	if(!test_file_(flacfile_, decoder_metadata_callback_compare_))
		return false;

	printf("S[A]PP\tset APPLICATION (grow), expand into padding which will leave 0-length pad\n");
	if(!FLAC__metadata_object_application_set_data(app, data, 87, true))
		return die_("setting APPLICATION data");
	if(!replace_in_our_metadata_(app, our_current_position, /*copy=*/true))
		return die_("copying object");
	our_metadata_.blocks[our_current_position+1]->length = 0;
	if(!FLAC__metadata_simple_iterator_set_block(siterator, app, true))
		return die_ss_("FLAC__metadata_simple_iterator_set_block(siterator, app, true)", siterator);

	if(!test_file_(flacfile_, decoder_metadata_callback_compare_))
		return false;

	printf("S[A]PP\tset APPLICATION (grow), expand into padding which is exactly consumed\n");
	if(!FLAC__metadata_object_application_set_data(app, data, 91, true))
		return die_("setting APPLICATION data");
	if(!replace_in_our_metadata_(app, our_current_position, /*copy=*/true))
		return die_("copying object");
	delete_from_our_metadata_(our_current_position+1);
	if(!FLAC__metadata_simple_iterator_set_block(siterator, app, true))
		return die_ss_("FLAC__metadata_simple_iterator_set_block(siterator, app, true)", siterator);

	if(!test_file_(flacfile_, decoder_metadata_callback_compare_))
		return false;

	printf("S[A]P\tset APPLICATION (grow), expand into padding which is exactly consumed\n");
	if(!FLAC__metadata_object_application_set_data(app, data, 100, true))
		return die_("setting APPLICATION data");
	if(!replace_in_our_metadata_(app, our_current_position, /*copy=*/true))
		return die_("copying object");
	delete_from_our_metadata_(our_current_position+1);
	our_metadata_.blocks[our_current_position]->is_last = true;
	if(!FLAC__metadata_simple_iterator_set_block(siterator, app, true))
		return die_ss_("FLAC__metadata_simple_iterator_set_block(siterator, app, true)", siterator);

	if(!test_file_(flacfile_, decoder_metadata_callback_compare_))
		return false;

	printf("S[A]\tset PADDING (equal size)\n");
	padding->length = app->length;
	if(!replace_in_our_metadata_(padding, our_current_position, /*copy=*/true))
		return die_("copying object");
	if(!FLAC__metadata_simple_iterator_set_block(siterator, padding, true))
		return die_ss_("FLAC__metadata_simple_iterator_set_block(siterator, padding, true)", siterator);

	if(!test_file_(flacfile_, decoder_metadata_callback_compare_))
		return false;

	printf("S[P]\tinsert PADDING after\n");
	if(!FLAC__metadata_simple_iterator_insert_block_after(siterator, padding, false))
		return die_ss_("FLAC__metadata_simple_iterator_insert_block_after(siterator, padding, false)", siterator);
	if(!insert_to_our_metadata_(padding, ++our_current_position, /*copy=*/true))
		return false;

	if(!test_file_(flacfile_, decoder_metadata_callback_compare_))
		return false;

	printf("SP[P]\tinsert PADDING after\n");
	padding->length = 5;
	if(!FLAC__metadata_simple_iterator_insert_block_after(siterator, padding, false))
		return die_ss_("FLAC__metadata_simple_iterator_insert_block_after(siterator, padding, false)", siterator);
	if(!insert_to_our_metadata_(padding, ++our_current_position, /*copy=*/true))
		return false;

	if(!test_file_(flacfile_, decoder_metadata_callback_compare_))
		return false;

	printf("SPP[P]\tprev\n");
	if(!FLAC__metadata_simple_iterator_prev(siterator))
		return die_("iterator ended early\n");
	our_current_position--;

	printf("SP[P]P\tprev\n");
	if(!FLAC__metadata_simple_iterator_prev(siterator))
		return die_("iterator ended early\n");
	our_current_position--;

	printf("S[P]PP\tprev\n");
	if(!FLAC__metadata_simple_iterator_prev(siterator))
		return die_("iterator ended early\n");
	our_current_position--;

	printf("[S]PPP\tinsert APPLICATION after, try to expand into padding which is too small\n");
	if(!FLAC__metadata_object_application_set_data(app, data, 101, true))
		return die_("setting APPLICATION data");
	if(!insert_to_our_metadata_(app, ++our_current_position, /*copy=*/true))
		return die_("copying object");
	if(!FLAC__metadata_simple_iterator_insert_block_after(siterator, app, true))
		return die_ss_("FLAC__metadata_simple_iterator_insert_block_after(siterator, app, true)", siterator);

	if(!test_file_(flacfile_, decoder_metadata_callback_compare_))
		return false;

	printf("S[A]PPP\tdelete (middle block), don't replace with padding\n");
	if(!FLAC__metadata_simple_iterator_delete_block(siterator, false))
		return die_ss_("FLAC__metadata_simple_iterator_delete_block(siterator, false)", siterator);
	delete_from_our_metadata_(our_current_position--);

	if(!test_file_(flacfile_, decoder_metadata_callback_compare_))
		return false;

	printf("[S]PPP\tinsert APPLICATION after, try to expand into padding which is 'close' but still too small\n");
	if(!FLAC__metadata_object_application_set_data(app, data, 97, true))
		return die_("setting APPLICATION data");
	if(!insert_to_our_metadata_(app, ++our_current_position, /*copy=*/true))
		return die_("copying object");
	if(!FLAC__metadata_simple_iterator_insert_block_after(siterator, app, true))
		return die_ss_("FLAC__metadata_simple_iterator_insert_block_after(siterator, app, true)", siterator);

	if(!test_file_(flacfile_, decoder_metadata_callback_compare_))
		return false;

	printf("S[A]PPP\tdelete (middle block), don't replace with padding\n");
	if(!FLAC__metadata_simple_iterator_delete_block(siterator, false))
		return die_ss_("FLAC__metadata_simple_iterator_delete_block(siterator, false)", siterator);
	delete_from_our_metadata_(our_current_position--);

	if(!test_file_(flacfile_, decoder_metadata_callback_compare_))
		return false;

	printf("[S]PPP\tinsert APPLICATION after, expand into padding which is exactly consumed\n");
	if(!FLAC__metadata_object_application_set_data(app, data, 100, true))
		return die_("setting APPLICATION data");
	if(!insert_to_our_metadata_(app, ++our_current_position, /*copy=*/true))
		return die_("copying object");
	delete_from_our_metadata_(our_current_position+1);
	if(!FLAC__metadata_simple_iterator_insert_block_after(siterator, app, true))
		return die_ss_("FLAC__metadata_simple_iterator_insert_block_after(siterator, app, true)", siterator);

	if(!test_file_(flacfile_, decoder_metadata_callback_compare_))
		return false;

	printf("S[A]PP\tdelete (middle block), don't replace with padding\n");
	if(!FLAC__metadata_simple_iterator_delete_block(siterator, false))
		return die_ss_("FLAC__metadata_simple_iterator_delete_block(siterator, false)", siterator);
	delete_from_our_metadata_(our_current_position--);

	if(!test_file_(flacfile_, decoder_metadata_callback_compare_))
		return false;

	printf("[S]PP\tinsert APPLICATION after, expand into padding which will leave 0-length pad\n");
	if(!FLAC__metadata_object_application_set_data(app, data, 96, true))
		return die_("setting APPLICATION data");
	if(!insert_to_our_metadata_(app, ++our_current_position, /*copy=*/true))
		return die_("copying object");
	our_metadata_.blocks[our_current_position+1]->length = 0;
	if(!FLAC__metadata_simple_iterator_insert_block_after(siterator, app, true))
		return die_ss_("FLAC__metadata_simple_iterator_insert_block_after(siterator, app, true)", siterator);

	if(!test_file_(flacfile_, decoder_metadata_callback_compare_))
		return false;

	printf("S[A]PP\tdelete (middle block), don't replace with padding\n");
	if(!FLAC__metadata_simple_iterator_delete_block(siterator, false))
		return die_ss_("FLAC__metadata_simple_iterator_delete_block(siterator, false)", siterator);
	delete_from_our_metadata_(our_current_position--);

	if(!test_file_(flacfile_, decoder_metadata_callback_compare_))
		return false;

	printf("[S]PP\tnext\n");
	if(!FLAC__metadata_simple_iterator_next(siterator))
		return die_("iterator ended early\n");
	our_current_position++;

	printf("S[P]P\tdelete (middle block), don't replace with padding\n");
	if(!FLAC__metadata_simple_iterator_delete_block(siterator, false))
		return die_ss_("FLAC__metadata_simple_iterator_delete_block(siterator, false)", siterator);
	delete_from_our_metadata_(our_current_position--);

	if(!test_file_(flacfile_, decoder_metadata_callback_compare_))
		return false;

	printf("[S]P\tinsert APPLICATION after, expand into padding which is exactly consumed\n");
	if(!FLAC__metadata_object_application_set_data(app, data, 1, true))
		return die_("setting APPLICATION data");
	if(!insert_to_our_metadata_(app, ++our_current_position, /*copy=*/true))
		return die_("copying object");
	delete_from_our_metadata_(our_current_position+1);
	if(!FLAC__metadata_simple_iterator_insert_block_after(siterator, app, true))
		return die_ss_("FLAC__metadata_simple_iterator_insert_block_after(siterator, app, true)", siterator);

	if(!test_file_(flacfile_, decoder_metadata_callback_compare_))
		return false;

	printf("delete simple iterator\n");

	FLAC__metadata_simple_iterator_delete(siterator);

	FLAC__metadata_object_delete(app);
	FLAC__metadata_object_delete(padding);

	if(!remove_file_(flacfile_))
		return false;

	return true;
}

static FLAC__bool test_level_2_(const char *progname)
{
	FLAC__MetaData_Iterator *iterator;
	FLAC__MetaData_Chain *chain;
	FLAC__StreamMetaData *block, *app, *padding;
	FLAC__byte data[2000];
	unsigned our_current_position;

	printf("\n\n++++++ testing level 2 interface\n");

	printf("generate read-only file\n");

	if(!generate_file_(progname))
		return false;

	if(!change_stats_(flacfile_, /*read_only=*/true))
		return false;

	printf("create chain\n");

	if(0 == (chain = FLAC__metadata_chain_new()))
		return die_("allocating chain");

	printf("read chain\n");

	if(!FLAC__metadata_chain_read(chain, flacfile_))
		return die_c_("reading chain", FLAC__metadata_chain_status(chain));

	printf("[S]P\ttest initial metadata\n");

	if(!compare_chain_(chain, 0, 0))
		return false;
	if(!test_file_(flacfile_, decoder_metadata_callback_compare_))
		return false;

	printf("switch file to read-write\n");

	if(!change_stats_(flacfile_, /*read-only=*/false))
		return false;

	printf("[S]P\tmodify STREAMINFO, write\n");

	if(0 == (iterator = FLAC__metadata_iterator_new()))
		return die_("allocating memory for iterator");

	our_current_position = 0;

	FLAC__metadata_iterator_init(iterator, chain);

	if(0 == (block = FLAC__metadata_iterator_get_block(iterator)))
		return die_("getting block from iterator");

	FLAC__ASSERT(block->type == FLAC__METADATA_TYPE_STREAMINFO);

	block->data.stream_info.sample_rate = 32000;
	if(!replace_in_our_metadata_(block, our_current_position, /*copy=*/true))
		return die_("copying object");

	if(!FLAC__metadata_chain_write(chain, /*use_padding=*/false, /*preserve_file_stats=*/true))
		return die_c_("during FLAC__metadata_chain_write(chain, false, true)", FLAC__metadata_chain_status(chain));
	if(!compare_chain_(chain, our_current_position, FLAC__metadata_iterator_get_block(iterator)))
		return false;
	if(!test_file_(flacfile_, decoder_metadata_callback_compare_))
		return false;

	printf("[S]P\tnext\n");
	if(!FLAC__metadata_iterator_next(iterator))
		return die_("iterator ended early\n");
	our_current_position++;

	printf("S[P]\treplace PADDING with identical-size APPLICATION\n");
	if(0 == (block = FLAC__metadata_iterator_get_block(iterator)))
		return die_("getting block from iterator");
	if(0 == (app = FLAC__metadata_object_new(FLAC__METADATA_TYPE_APPLICATION)))
		return die_("FLAC__metadata_object_new(FLAC__METADATA_TYPE_APPLICATION)");
	memcpy(app->data.application.id, "duh", (FLAC__STREAM_METADATA_APPLICATION_ID_LEN/8));
	if(!FLAC__metadata_object_application_set_data(app, data, block->length-(FLAC__STREAM_METADATA_APPLICATION_ID_LEN/8), true))
		return die_("setting APPLICATION data");
	FLAC__metadata_object_delete(block);
	if(!replace_in_our_metadata_(app, our_current_position, /*copy=*/false))
		return die_("copying object");
	if(!FLAC__metadata_iterator_set_block(iterator, app))
		return die_c_("FLAC__metadata_iterator_set_block(iterator, app)", FLAC__metadata_chain_status(chain));

	if(!FLAC__metadata_chain_write(chain, /*use_padding=*/false, /*preserve_file_stats=*/false))
		return die_c_("during FLAC__metadata_chain_write(chain, false, true)", FLAC__metadata_chain_status(chain));
	if(!compare_chain_(chain, our_current_position, FLAC__metadata_iterator_get_block(iterator)))
		return false;
	if(!test_file_(flacfile_, decoder_metadata_callback_compare_))
		return false;

	printf("S[A]\tshrink APPLICATION, don't use padding\n");
	if(0 == (app = FLAC__metadata_object_copy(our_metadata_.blocks[our_current_position])))
		return die_("copying object");
	if(!FLAC__metadata_object_application_set_data(app, data, 26, true))
		return die_("setting APPLICATION data");
	if(!replace_in_our_metadata_(app, our_current_position, /*copy=*/true))
		return die_("copying object");
	if(!FLAC__metadata_iterator_set_block(iterator, app))
		return die_c_("FLAC__metadata_iterator_set_block(iterator, app)", FLAC__metadata_chain_status(chain));

	if(!FLAC__metadata_chain_write(chain, /*use_padding=*/false, /*preserve_file_stats=*/false))
		return die_c_("during FLAC__metadata_chain_write(chain, false, true)", FLAC__metadata_chain_status(chain));
	if(!compare_chain_(chain, our_current_position, FLAC__metadata_iterator_get_block(iterator)))
		return false;
	if(!test_file_(flacfile_, decoder_metadata_callback_compare_))
		return false;

	printf("S[A]\tgrow APPLICATION, don't use padding\n");
	if(0 == (app = FLAC__metadata_object_copy(our_metadata_.blocks[our_current_position])))
		return die_("copying object");
	if(!FLAC__metadata_object_application_set_data(app, data, 28, true))
		return die_("setting APPLICATION data");
	if(!replace_in_our_metadata_(app, our_current_position, /*copy=*/true))
		return die_("copying object");
	if(!FLAC__metadata_iterator_set_block(iterator, app))
		return die_c_("FLAC__metadata_iterator_set_block(iterator, app)", FLAC__metadata_chain_status(chain));

	if(!FLAC__metadata_chain_write(chain, /*use_padding=*/false, /*preserve_file_stats=*/false))
		return die_c_("during FLAC__metadata_chain_write(chain, false, true)", FLAC__metadata_chain_status(chain));
	if(!compare_chain_(chain, our_current_position, FLAC__metadata_iterator_get_block(iterator)))
		return false;
	if(!test_file_(flacfile_, decoder_metadata_callback_compare_))
		return false;

	printf("S[A]\tgrow APPLICATION, use padding, but last block is not padding\n");
	if(0 == (app = FLAC__metadata_object_copy(our_metadata_.blocks[our_current_position])))
		return die_("copying object");
	if(!FLAC__metadata_object_application_set_data(app, data, 36, true))
		return die_("setting APPLICATION data");
	if(!replace_in_our_metadata_(app, our_current_position, /*copy=*/true))
		return die_("copying object");
	if(!FLAC__metadata_iterator_set_block(iterator, app))
		return die_c_("FLAC__metadata_iterator_set_block(iterator, app)", FLAC__metadata_chain_status(chain));

	if(!FLAC__metadata_chain_write(chain, /*use_padding=*/false, /*preserve_file_stats=*/false))
		return die_c_("during FLAC__metadata_chain_write(chain, false, true)", FLAC__metadata_chain_status(chain));
	if(!compare_chain_(chain, our_current_position, FLAC__metadata_iterator_get_block(iterator)))
		return false;
	if(!test_file_(flacfile_, decoder_metadata_callback_compare_))
		return false;

	printf("S[A]\tshrink APPLICATION, use padding, last block is not padding, but delta is too small for new PADDING block\n");
	if(0 == (app = FLAC__metadata_object_copy(our_metadata_.blocks[our_current_position])))
		return die_("copying object");
	if(!FLAC__metadata_object_application_set_data(app, data, 33, true))
		return die_("setting APPLICATION data");
	if(!replace_in_our_metadata_(app, our_current_position, /*copy=*/true))
		return die_("copying object");
	if(!FLAC__metadata_iterator_set_block(iterator, app))
		return die_c_("FLAC__metadata_iterator_set_block(iterator, app)", FLAC__metadata_chain_status(chain));

	if(!FLAC__metadata_chain_write(chain, /*use_padding=*/true, /*preserve_file_stats=*/false))
		return die_c_("during FLAC__metadata_chain_write(chain, true, true)", FLAC__metadata_chain_status(chain));
	if(!compare_chain_(chain, our_current_position, FLAC__metadata_iterator_get_block(iterator)))
		return false;
	if(!test_file_(flacfile_, decoder_metadata_callback_compare_))
		return false;

	printf("S[A]\tshrink APPLICATION, use padding, last block is not padding, delta is enough for new PADDING block\n");
	if(0 == (padding = FLAC__metadata_object_new(FLAC__METADATA_TYPE_PADDING)))
		return die_("creating PADDING block");
	if(0 == (app = FLAC__metadata_object_copy(our_metadata_.blocks[our_current_position])))
		return die_("copying object");
	if(!FLAC__metadata_object_application_set_data(app, data, 29, true))
		return die_("setting APPLICATION data");
	if(!replace_in_our_metadata_(app, our_current_position, /*copy=*/true))
		return die_("copying object");
	padding->length = 0;
	if(!insert_to_our_metadata_(padding, our_current_position+1, /*copy=*/false))
		return die_("internal error");
	if(!FLAC__metadata_iterator_set_block(iterator, app))
		return die_c_("FLAC__metadata_iterator_set_block(iterator, app)", FLAC__metadata_chain_status(chain));

	if(!FLAC__metadata_chain_write(chain, /*use_padding=*/true, /*preserve_file_stats=*/false))
		return die_c_("during FLAC__metadata_chain_write(chain, true, true)", FLAC__metadata_chain_status(chain));
	if(!compare_chain_(chain, our_current_position, FLAC__metadata_iterator_get_block(iterator)))
		return false;
	if(!test_file_(flacfile_, decoder_metadata_callback_compare_))
		return false;

	printf("S[A]P\tshrink APPLICATION, use padding, last block is padding\n");
	if(0 == (app = FLAC__metadata_object_copy(our_metadata_.blocks[our_current_position])))
		return die_("copying object");
	if(!FLAC__metadata_object_application_set_data(app, data, 16, true))
		return die_("setting APPLICATION data");
	if(!replace_in_our_metadata_(app, our_current_position, /*copy=*/true))
		return die_("copying object");
	our_metadata_.blocks[our_current_position+1]->length = 13;
	if(!FLAC__metadata_iterator_set_block(iterator, app))
		return die_c_("FLAC__metadata_iterator_set_block(iterator, app)", FLAC__metadata_chain_status(chain));

	if(!FLAC__metadata_chain_write(chain, /*use_padding=*/true, /*preserve_file_stats=*/false))
		return die_c_("during FLAC__metadata_chain_write(chain, true, true)", FLAC__metadata_chain_status(chain));
	if(!compare_chain_(chain, our_current_position, FLAC__metadata_iterator_get_block(iterator)))
		return false;
	if(!test_file_(flacfile_, decoder_metadata_callback_compare_))
		return false;

	printf("S[A]P\tgrow APPLICATION, use padding, last block is padding, but delta is too small\n");
	if(0 == (app = FLAC__metadata_object_copy(our_metadata_.blocks[our_current_position])))
		return die_("copying object");
	if(!FLAC__metadata_object_application_set_data(app, data, 50, true))
		return die_("setting APPLICATION data");
	if(!replace_in_our_metadata_(app, our_current_position, /*copy=*/true))
		return die_("copying object");
	if(!FLAC__metadata_iterator_set_block(iterator, app))
		return die_c_("FLAC__metadata_iterator_set_block(iterator, app)", FLAC__metadata_chain_status(chain));

	if(!FLAC__metadata_chain_write(chain, /*use_padding=*/true, /*preserve_file_stats=*/false))
		return die_c_("during FLAC__metadata_chain_write(chain, true, true)", FLAC__metadata_chain_status(chain));
	if(!compare_chain_(chain, our_current_position, FLAC__metadata_iterator_get_block(iterator)))
		return false;
	if(!test_file_(flacfile_, decoder_metadata_callback_compare_))
		return false;

	printf("S[A]P\tgrow APPLICATION, use padding, last block is padding of exceeding size\n");
	if(0 == (app = FLAC__metadata_object_copy(our_metadata_.blocks[our_current_position])))
		return die_("copying object");
	if(!FLAC__metadata_object_application_set_data(app, data, 56, true))
		return die_("setting APPLICATION data");
	if(!replace_in_our_metadata_(app, our_current_position, /*copy=*/true))
		return die_("copying object");
	our_metadata_.blocks[our_current_position+1]->length -= (56 - 50);
	if(!FLAC__metadata_iterator_set_block(iterator, app))
		return die_c_("FLAC__metadata_iterator_set_block(iterator, app)", FLAC__metadata_chain_status(chain));

	if(!FLAC__metadata_chain_write(chain, /*use_padding=*/true, /*preserve_file_stats=*/false))
		return die_c_("during FLAC__metadata_chain_write(chain, true, true)", FLAC__metadata_chain_status(chain));
	if(!compare_chain_(chain, our_current_position, FLAC__metadata_iterator_get_block(iterator)))
		return false;
	if(!test_file_(flacfile_, decoder_metadata_callback_compare_))
		return false;

	printf("S[A]P\tgrow APPLICATION, use padding, last block is padding of exact size\n");
	if(0 == (app = FLAC__metadata_object_copy(our_metadata_.blocks[our_current_position])))
		return die_("copying object");
	if(!FLAC__metadata_object_application_set_data(app, data, 67, true))
		return die_("setting APPLICATION data");
	if(!replace_in_our_metadata_(app, our_current_position, /*copy=*/true))
		return die_("copying object");
	delete_from_our_metadata_(our_current_position+1);
	if(!FLAC__metadata_iterator_set_block(iterator, app))
		return die_c_("FLAC__metadata_iterator_set_block(iterator, app)", FLAC__metadata_chain_status(chain));

	if(!FLAC__metadata_chain_write(chain, /*use_padding=*/true, /*preserve_file_stats=*/false))
		return die_c_("during FLAC__metadata_chain_write(chain, true, true)", FLAC__metadata_chain_status(chain));
	if(!compare_chain_(chain, our_current_position, FLAC__metadata_iterator_get_block(iterator)))
		return false;
	if(!test_file_(flacfile_, decoder_metadata_callback_compare_))
		return false;

	printf("S[A]\tprev\n");
	if(!FLAC__metadata_iterator_prev(iterator))
		return die_("iterator ended early\n");
	our_current_position--;

	printf("[S]A\tinsert PADDING before STREAMINFO (should fail)\n");
	if(0 == (padding = FLAC__metadata_object_new(FLAC__METADATA_TYPE_PADDING)))
		return die_("creating PADDING block");
	padding->length = 30;
	if(!FLAC__metadata_iterator_insert_block_before(iterator, padding))
		printf("\tFLAC__metadata_iterator_insert_block_before() returned false like it should\n");
	else
		return die_("FLAC__metadata_iterator_insert_block_before() should have returned false");

	printf("[S]A\tinsert PADDING after\n");
	if(!insert_to_our_metadata_(padding, ++our_current_position, /*copy=*/true))
		return die_("copying metadata");
	if(!FLAC__metadata_iterator_insert_block_after(iterator, padding))
		return die_("FLAC__metadata_iterator_insert_block_after(iterator, padding)");

	if(!compare_chain_(chain, our_current_position, FLAC__metadata_iterator_get_block(iterator)))
		return false;

	printf("S[P]A\tinsert PADDING before\n");
	if(0 == (padding = FLAC__metadata_object_copy(our_metadata_.blocks[our_current_position])))
		return die_("creating PADDING block");
	padding->length = 17;
	if(!insert_to_our_metadata_(padding, our_current_position, /*copy=*/true))
		return die_("copying metadata");
	if(!FLAC__metadata_iterator_insert_block_before(iterator, padding))
		return die_("FLAC__metadata_iterator_insert_block_before(iterator, padding)");

	if(!compare_chain_(chain, our_current_position, FLAC__metadata_iterator_get_block(iterator)))
		return false;

	printf("S[P]PA\tinsert PADDING before\n");
	if(0 == (padding = FLAC__metadata_object_copy(our_metadata_.blocks[our_current_position])))
		return die_("creating PADDING block");
	padding->length = 0;
	if(!insert_to_our_metadata_(padding, our_current_position, /*copy=*/true))
		return die_("copying metadata");
	if(!FLAC__metadata_iterator_insert_block_before(iterator, padding))
		return die_("FLAC__metadata_iterator_insert_block_before(iterator, padding)");

	if(!compare_chain_(chain, our_current_position, FLAC__metadata_iterator_get_block(iterator)))
		return false;

	printf("S[P]PPA\tnext\n");
	if(!FLAC__metadata_iterator_next(iterator))
		return die_("iterator ended early\n");
	our_current_position++;

	printf("SP[P]PA\tnext\n");
	if(!FLAC__metadata_iterator_next(iterator))
		return die_("iterator ended early\n");
	our_current_position++;

	printf("SPP[P]A\tnext\n");
	if(!FLAC__metadata_iterator_next(iterator))
		return die_("iterator ended early\n");
	our_current_position++;

	printf("SPPP[A]\tinsert PADDING after\n");
	if(0 == (padding = FLAC__metadata_object_copy(our_metadata_.blocks[1])))
		return die_("creating PADDING block");
	padding->length = 57;
	if(!insert_to_our_metadata_(padding, ++our_current_position, /*copy=*/true))
		return die_("copying metadata");
	if(!FLAC__metadata_iterator_insert_block_after(iterator, padding))
		return die_("FLAC__metadata_iterator_insert_block_after(iterator, padding)");

	if(!compare_chain_(chain, our_current_position, FLAC__metadata_iterator_get_block(iterator)))
		return false;

	printf("SPPPA[P]\tinsert PADDING before\n");
	if(0 == (padding = FLAC__metadata_object_copy(our_metadata_.blocks[1])))
		return die_("creating PADDING block");
	padding->length = 99;
	if(!insert_to_our_metadata_(padding, our_current_position, /*copy=*/true))
		return die_("copying metadata");
	if(!FLAC__metadata_iterator_insert_block_before(iterator, padding))
		return die_("FLAC__metadata_iterator_insert_block_before(iterator, padding)");

	if(!compare_chain_(chain, our_current_position, FLAC__metadata_iterator_get_block(iterator)))
		return false;

	printf("delete iterator\n");
	FLAC__metadata_iterator_delete(iterator);
	our_current_position = 0;

	printf("SPPPAPP\tmerge padding\n");
	FLAC__metadata_chain_merge_padding(chain);
	our_metadata_.blocks[1]->length += (4 + our_metadata_.blocks[2]->length);
	our_metadata_.blocks[1]->length += (4 + our_metadata_.blocks[3]->length);
	our_metadata_.blocks[5]->length += (4 + our_metadata_.blocks[6]->length);
	delete_from_our_metadata_(6);
	delete_from_our_metadata_(3);
	delete_from_our_metadata_(2);

	if(!FLAC__metadata_chain_write(chain, /*use_padding=*/true, /*preserve_file_stats=*/false))
		return die_c_("during FLAC__metadata_chain_write(chain, true, true)", FLAC__metadata_chain_status(chain));
	if(!compare_chain_(chain, 0, 0))
		return false;
	if(!test_file_(flacfile_, decoder_metadata_callback_compare_))
		return false;

	printf("SPAP\tsort padding\n");
	FLAC__metadata_chain_sort_padding(chain);
	our_metadata_.blocks[3]->length += (4 + our_metadata_.blocks[1]->length);
	delete_from_our_metadata_(1);

	if(!FLAC__metadata_chain_write(chain, /*use_padding=*/true, /*preserve_file_stats=*/false))
		return die_c_("during FLAC__metadata_chain_write(chain, true, true)", FLAC__metadata_chain_status(chain));
	if(!compare_chain_(chain, 0, 0))
		return false;
	if(!test_file_(flacfile_, decoder_metadata_callback_compare_))
		return false;

	printf("delete chain\n");

	FLAC__metadata_chain_delete(chain);

	if(!remove_file_(flacfile_))
		return false;

	return true;
}

int test_metadata_file_manipulation(const char *progname)
{
	printf("\n+++ unit test: metadata manipulation\n\n");

	our_metadata_.num_blocks = 0;

	if(!test_level_0_(progname))
		return 1;

	if(!test_level_1_(progname))
		return 1;

	if(!test_level_2_(progname))
		return 1;

	return 0;
}
