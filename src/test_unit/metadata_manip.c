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
static our_metadata_struct our_metadata_;
static unsigned mc_our_blocknumber_ = 0;

static FLAC__bool die_(const char *msg)
{
	printf("ERROR: %s\n", msg);
	return false;
}

static FLAC__bool die_ss_(const char *msg, FLAC__MetaData_SimpleIterator *siterator)
{
	printf("ERROR: %s\n", msg);
	printf("       status=%s\n", FLAC__MetaData_SimpleIteratorStatusString[FLAC__metadata_simple_iterator_status(siterator)]);
	return false;
}

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

static FLAC__StreamDecoderWriteStatus decoder_write_callback_(const FLAC__FileDecoder *decoder, const FLAC__Frame *frame, const FLAC__int32 *buffer[], void *client_data)
{
	(void)decoder, (void)frame, (void)buffer, (void)client_data;

	return FLAC__STREAM_DECODER_WRITE_CONTINUE;
}

static void decoder_error_callback_(const FLAC__FileDecoder *decoder, FLAC__StreamDecoderErrorStatus status, void *client_data)
{
	decoder_client_struct *dcd = (decoder_client_struct*)client_data;
	(void)decoder;

	dcd->error_occurred = true;
	printf("ERROR: got error callback, status = %s (%u)\n", FLAC__StreamDecoderErrorStatusString[status], (unsigned)status);
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
	encoder_client_struct encoder_client_data;
	FILE *file;
	FLAC__byte buffer[4096];
	FLAC__int32 samples[4096];
	unsigned i, n;

	FLAC__ASSERT(0 != input_filename);

	our_metadata_.num_blocks = 0;

	printf("generating FLAC file for test\n");

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
	FLAC__stream_encoder_set_channels(encoder, 1);
	FLAC__stream_encoder_set_bits_per_sample(encoder, 8);
	FLAC__stream_encoder_set_sample_rate(encoder, 44100);
	FLAC__stream_encoder_set_blocksize(encoder, 576);
	FLAC__stream_encoder_set_max_lpc_order(encoder, 0);
	FLAC__stream_encoder_set_qlp_coeff_precision(encoder, 0);
	FLAC__stream_encoder_set_do_qlp_coeff_prec_search(encoder, false);
	FLAC__stream_encoder_set_do_escape_coding(encoder, false);
	FLAC__stream_encoder_set_do_exhaustive_model_search(encoder, false);
	FLAC__stream_encoder_set_min_residual_partition_order(encoder, 0);
	FLAC__stream_encoder_set_max_residual_partition_order(encoder, 0);
	FLAC__stream_encoder_set_rice_parameter_search_dist(encoder, 0);
	FLAC__stream_encoder_set_total_samples_estimate(encoder, 0);
	FLAC__stream_encoder_set_seek_table(encoder, 0);
	FLAC__stream_encoder_set_padding(encoder, 12345);
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

	mc_our_blocknumber_ = 0;
	decoder_client_data.error_occurred = false;

	printf("testing '%s'... ", filename);

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
	if(!FLAC__file_decoder_process_metadata(decoder)) {
		FLAC__file_decoder_finish(decoder);
		FLAC__file_decoder_delete(decoder);
		return die_("decoding file\n");
	}

	FLAC__file_decoder_finish(decoder);
	FLAC__file_decoder_delete(decoder);

	if(!decoder_client_data.error_occurred)
		printf("PASSED\n");

	return !decoder_client_data.error_occurred;
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
	if(!change_stats_(filename, /*read_only=*/false) || 0 != unlink(filename))
		return die_("removing file");

	return true;
}

static void mc_null_(const FLAC__FileDecoder *decoder, const FLAC__StreamMetaData *metadata, void *client_data)
{
	(void)decoder, (void)metadata, (void)client_data;
}

static void mc_ours_(const FLAC__FileDecoder *decoder, const FLAC__StreamMetaData *metadata, void *client_data)
{
	decoder_client_struct *dcd = (decoder_client_struct*)client_data;

	(void)decoder;

	/* don't bother checking if we've already hit an error */
	if(dcd->error_occurred)
		return;

	printf("%d... ", mc_our_blocknumber_);

	if(mc_our_blocknumber_ >= our_metadata_.num_blocks) {
		(void)die_("got more metadata blocks than expected");
		dcd->error_occurred = true;
	}
	else {
		if(!compare_block_(metadata, our_metadata_.blocks[mc_our_blocknumber_])) {
			(void)die_("metadata block mismatch");
			dcd->error_occurred = true;
		}
	}
	mc_our_blocknumber_++;
}

static FLAC__bool test_level_0_(const char *progname)
{
	FLAC__StreamMetaData_StreamInfo streaminfo;

	printf("\n\n++++++ testing level 0 interface\n");

	if(!generate_file_(progname))
		return false;

	if(!test_file_(flacfile_, mc_null_))
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
	FLAC__MetaData_Iterator *iterator;
	FLAC__MetaData_Chain *chain;
	FLAC__StreamMetaData *block, *app, *padding;
	FLAC__byte data[1000];
	unsigned i = 0, our_current_position = 0;

	printf("\n\n++++++ testing level 1 interface\n");

	/************************************************************/

	printf("simple iterator on read-only file\n");

	if(!generate_file_(progname))
		return false;

	if(!change_stats_(flacfile_, /*read_only=*/true))
		return false;

	if(!test_file_(flacfile_, mc_null_))
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
	(void)insert_to_our_metadata_(block, our_current_position, /*copy=*/false);

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
	if(block->length != 12345)
		return die_("bad STREAMINFO length");
	(void)insert_to_our_metadata_(block, our_current_position, /*copy=*/false);

	if(FLAC__metadata_simple_iterator_next(siterator))
		return die_("forward iterator returned true but should have returned false");

	printf("iterate backwards\n");
	if(!FLAC__metadata_simple_iterator_prev(siterator))
		return die_("reverse iterator ended early");
	if(FLAC__metadata_simple_iterator_prev(siterator))
		return die_("reverse iterator returned true but should have returned false");

	printf("testing FLAC__metadata_simple_iterator_set_block() on read-only file...\n");

	if(!FLAC__metadata_simple_iterator_set_block(siterator, (void*)99, false)) {
		printf("PASSED.  FLAC__metadata_simple_iterator_set_block() returned false like it should\n");
		printf("  status=%s\n", FLAC__MetaData_SimpleIteratorStatusString[FLAC__metadata_simple_iterator_status(siterator)]);
	}
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
	memcpy(app->data.application.id, "duh", 4);

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

	printf("[S]P   try to write over STREAMINFO block...\n");
	if(!FLAC__metadata_simple_iterator_set_block(siterator, app, false)) {
		printf("FLAC__metadata_simple_iterator_set_block() returned false like it should\n");
		printf("  status=%s\n", FLAC__MetaData_SimpleIteratorStatusString[FLAC__metadata_simple_iterator_status(siterator)]);
	}
	else
		return die_("FLAC__metadata_simple_iterator_set_block() returned true but shouldn't have");

	printf("[S]P   insert PADDING after, don't expand into padding\n");
	if(!FLAC__metadata_simple_iterator_insert_block_after(siterator, padding, false))
		return die_ss_("FLAC__metadata_simple_iterator_insert_block_after(siterator, app, false)", siterator);
	if(!insert_to_our_metadata_(padding, ++our_current_position, /*copy=*/true))
		return false;

	if(!test_file_(flacfile_, mc_ours_))
		return false;

	
	printf("S[P]P   prev\n");
	if(!FLAC__metadata_simple_iterator_prev(siterator))
		return die_("iterator ended early\n");
	our_current_position--;

	printf("[S]PP   delete (STREAMINFO block), must fail\n");
	if(FLAC__metadata_simple_iterator_delete_block(siterator, false))
		return die_ss_("FLAC__metadata_simple_iterator_delete_block(siterator, false) should have returned false", siterator);

	if(!test_file_(flacfile_, mc_ours_))
		return false;

	printf("[S]PP   next\n");
	if(!FLAC__metadata_simple_iterator_next(siterator))
		return die_("iterator ended early\n");
	our_current_position++;

	printf("S[P]P   delete (middle block), don't replace with padding\n");
	if(!FLAC__metadata_simple_iterator_delete_block(siterator, false))
		return die_ss_("FLAC__metadata_simple_iterator_delete_block(siterator, false)", siterator);
	delete_from_our_metadata_(our_current_position--);

	if(!test_file_(flacfile_, mc_ours_))
		return false;

	printf("[S]P   next\n");
	if(!FLAC__metadata_simple_iterator_next(siterator))
		return die_("iterator ended early\n");
	our_current_position++;

	printf("S[P]   delete (last block), don't replace with padding\n");
	if(!FLAC__metadata_simple_iterator_delete_block(siterator, false))
		return die_ss_("FLAC__metadata_simple_iterator_delete_block(siterator, false)", siterator);
	delete_from_our_metadata_(our_current_position--);

	if(!test_file_(flacfile_, mc_ours_))
		return false;

return true;
	printf("S[P]P   insert APPLICATION after, don't expand into padding\n");
	if(!FLAC__metadata_simple_iterator_insert_block_after(siterator, app, false))
		return die_ss_("FLAC__metadata_simple_iterator_insert_block_after(siterator, app, false)", siterator);
	if(!insert_to_our_metadata_(app, ++our_current_position, /*copy=*/true))
		return false;

	if(!test_file_(flacfile_, mc_ours_))
		return false;

	printf("SP[A]P   insert APPLICATION after, expand into padding of exceeding size\n");
	app->data.application.id[0] = 'e'; /* twiddle the id so that our comparison doesn't miss transposition */
	if(!FLAC__metadata_simple_iterator_insert_block_after(siterator, app, true))
		return die_ss_("FLAC__metadata_simple_iterator_insert_block_after(siterator, app, true)", siterator);
	if(!insert_to_our_metadata_(app, ++our_current_position, /*copy=*/true))
		return false;
	our_metadata_.blocks[our_current_position+1]->length -= 4 + app->length;

	if(!test_file_(flacfile_, mc_ours_))
		return false;

	if(!FLAC__metadata_simple_iterator_next(siterator))
		return die_("iterator ended early\n");

	printf("SPA[A]P   set APPLICATION, expand into padding of exceeding size\n");
	app->data.application.id[0] = 'f'; /* twiddle the id */
	if(!FLAC__metadata_simple_iterator_set_block(siterator, app, true))
		return die_ss_("FLAC__metadata_simple_iterator_set_block(siterator, app, true)", siterator);
	if(!insert_to_our_metadata_(app, ++our_current_position, /*copy=*/true))
		return false;
	our_metadata_.blocks[our_current_position+1]->length -= 4 + app->length;

	if(!test_file_(flacfile_, mc_ours_))
		return false;

	if(!FLAC__metadata_simple_iterator_next(siterator))
		return die_("iterator ended early\n");
	our_current_position++;
	if(FLAC__metadata_simple_iterator_next(siterator))
		return die_("iterator should have ended but didn't\n");

	printf("[S]PAAAP   set STREAMINFO (change sample rate)\n");
	while(FLAC__metadata_simple_iterator_prev(siterator))
		our_current_position--;
	block = FLAC__metadata_simple_iterator_get_block(siterator);
	FLAC__ASSERT(our_current_position == 0);
	block->data.stream_info.sample_rate = 32000;
	if(!replace_in_our_metadata_(block, our_current_position, /*copy=*/true))
		return die_("copying object");
	if(!FLAC__metadata_simple_iterator_set_block(siterator, block, false))
		return die_ss_("FLAC__metadata_simple_iterator_set_block(siterator, block, false)", siterator);
	FLAC__metadata_object_delete(block);

	if(!test_file_(flacfile_, mc_ours_))
		return false;

	printf("SPAA[A]P   set block (grow), don't expand into padding\n");
	while(FLAC__metadata_simple_iterator_next(siterator))
		our_current_position++;
	if(!FLAC__metadata_simple_iterator_prev(siterator))
		return die_("iterator ended early\n");
	our_current_position--;
	app->data.application.id[0] = 'g'; /* twiddle the id */
	if(!FLAC__metadata_object_application_set_data(app, data, sizeof(data), true))
		return die_("setting APPLICATION data");
	if(!replace_in_our_metadata_(app, our_current_position, /*copy=*/true))
		return die_("copying object");
	if(!FLAC__metadata_simple_iterator_set_block(siterator, app, false))
		return die_ss_("FLAC__metadata_simple_iterator_set_block(siterator, app, false)", siterator);

	if(!test_file_(flacfile_, mc_ours_))
		return false;

	printf("SPAA[A]P   set block (shrink), don't fill in with padding\n");
	app->data.application.id[0] = 'h'; /* twiddle the id */
	if(!FLAC__metadata_object_application_set_data(app, data, 12, true))
		return die_("setting APPLICATION data");
	if(!replace_in_our_metadata_(app, our_current_position, /*copy=*/true))
		return die_("copying object");
	if(!FLAC__metadata_simple_iterator_set_block(siterator, app, false))
		return die_ss_("FLAC__metadata_simple_iterator_set_block(siterator, app, false)", siterator);

	if(!test_file_(flacfile_, mc_ours_))
		return false;

	printf("SPAA[A]P   set block (grow), expand into padding\n");
	app->data.application.id[0] = 'i'; /* twiddle the id */
	if(!FLAC__metadata_object_application_set_data(app, data, sizeof(data), true))
		return die_("setting APPLICATION data");
	if(!replace_in_our_metadata_(app, our_current_position, /*copy=*/true))
		return die_("copying object");
	our_metadata_.blocks[our_current_position+1]->length -= (sizeof(data) - 12);
	if(!FLAC__metadata_simple_iterator_set_block(siterator, app, true))
		return die_ss_("FLAC__metadata_simple_iterator_set_block(siterator, app, true)", siterator);
//@@@need to test cases when all follow padding is used up exactly

	if(!test_file_(flacfile_, mc_ours_))
		return false;

	FLAC__metadata_simple_iterator_delete(siterator);

	FLAC__metadata_object_delete(app);

	if(!remove_file_(flacfile_))
		return false;

	return true;
}

static FLAC__bool test_level_2_(const char *progname)
{
	printf("\n\n++++++ testing level 2 interface\n");

	return true;
}

int test_metadata_file_manipulation(const char *progname)
{
	printf("\n+++ unit test: metadata manipulation\n\n");

	if(!test_level_0_(progname))
		return 1;

	if(!test_level_1_(progname))
		return 1;

	if(!test_level_2_(progname))
		return 1;

	return 0;
}
#if 0
static void printb(const FLAC__StreamMetaData *block)
{
	printf("\ttype=%s (%u)\n", FLAC__MetaDataTypeString[block->type], (unsigned)block->type);
	printf("\tis_last=%s (%u)\n", block->is_last? "true":"false", (unsigned)block->is_last);
	printf("\tlength=%u\n", block->length);
}

static void printsi(FLAC__MetaData_SimpleIterator *siterator)
{
	FLAC__StreamMetaData *block = FLAC__metadata_simple_iterator_get_block(siterator);
	printb(block);
	FLAC__metadata_object_delete(block);
}

int test_metadata()
{

	return 0;
}
#endif
#if 0

/***********************************************************************
 * level 1
 * ---------------------------------------------------------------------
 * The general usage of this interface is:
 *
 * Create an iterator using FLAC__metadata_simple_iterator_new()
 * Attach it to a file using FLAC__metadata_simple_iterator_init() and check
 *    the exit code.  Call FLAC__metadata_simple_iterator_is_writable() to
 *    see if the file is writable, or read-only access is allowed.
 * Use _next() and _prev() to move around the blocks.  This is does not
 *    read the actual blocks themselves.  _next() is relatively fast.
 *    _prev() is slower since it needs to search forward from the front
 *    of the file.
 * Use _get_block_type() or _get_block() to access the actual data.  The
 *    returned object is yours to modify and free.
 * Use _set_block() to write a modified block back.  You must have write
 *    permission to the original file.  Make sure to read the whole
 *    comment to _set_block() below.
 * Use _insert_block_after() to add new blocks.  Use the object creation
 *    functions from the end of this header file to generate new objects.
 * Use _delete_block() to remove the block currently referred to by the
 *    iterator, or replace it with padding.
 * Destroy the iterator with FLAC__metadata_simple_iterator_delete()
 *    when finished.
 *
 * NOTE: The FLAC file remains open the whole time between _init() and
 *       _delete(), so make sure you are not altering the file during
 *       this time.
 *
 * NOTE: Do not modify the is_last, length, or type fields of returned
 *       FLAC__MetaDataType objects.  These are managed automatically.
 *
 * NOTE: If any of the modification functions (_set_block, _delete_block,
 *       _insert_block_after, etc) return false, you should delete the
 *       iterator as it may no longer be valid.
 */

/*
 * Write a block back to the FLAC file.  This function tries to be
 * as efficient as possible; how the block is actually written is
 * shown by the following:
 *
 * Existing block is a STREAMINFO block and the new block is a
 * STREAMINFO block: the new block is written in place.  Make sure
 * you know what you're doing when changing the values of a
 * STREAMINFO block.
 * 
 * Existing block is a STREAMINFO block and the new block is a
 * not a STREAMINFO block: this is an error since the first block
 * must be a STREAMINFO block.  Returns false without altering the
 * file. 
 * 
 * Existing block is not a STREAMINFO block and the new block is a
 * STREAMINFO block: this is an error since there may be only one
 * STREAMINFO block.  Returns false without altering the file. 
 * 
 * Existing block and new block are the same length: the existing
 * block will be replaced by the new block, written in place.
 *
 * Existing block is longer than new block: if use_padding is true,
 * the existing block will be overwritten in place with the new
 * block followed by a PADDING block, if possible, to make the total
 * size the same as the existing block.  Remember that a padding
 * block requires at least four bytes so if the difference in size
 * between the new block and existing block is less than that, the
 * entire file will have to be rewritten, using the new block's
 * exact size.  If use_padding is false, the entire file will be
 * rewritten, replacing the existing block by the new block.
 *
 * Existing block is shorter than new block: if use_padding is true,
 * the function will try and expand the new block into the following
 * PADDING block, if it exists and doing so won't shrink the PADDING
 * block to less than 4 bytes.  If there is no following PADDING
 * block, or it will shrink to less than 4 bytes, or use_padding is
 * false, the entire file is rewritten, replacing the existing block
 * with the new block.  Note that in this case any following PADDING
 * block is preserved as is.
 *
 * After writing the block, the iterator will remain in the same
 * place, i.e. pointing to the new block.
 */
FLAC__bool FLAC__metadata_simple_iterator_set_block(FLAC__MetaData_SimpleIterator *iterator, FLAC__StreamMetaData *block, FLAC__bool use_padding);

/*
 * This is similar to FLAC__metadata_simple_iterator_set_block()
 * except that instead of writing over an existing block, it appends
 * a block after the existing block.  'use_padding' is again used to
 * tell the function to try an expand into following padding in an
 * attempt to avoid rewriting the entire file.
 *
 * This function will fail and return false if given a STREAMINFO
 * block.
 *
 * After writing the block, the iterator will be pointing to the
 * new block.
 */
FLAC__bool FLAC__metadata_simple_iterator_insert_block_after(FLAC__MetaData_SimpleIterator *iterator, FLAC__StreamMetaData *block, FLAC__bool use_padding);

/*
 * Deletes the block at the current position.  This will cause the
 * entire FLAC file to be rewritten, unless 'use_padding' is true,
 * in which case the block will be replaced by an equal-sized PADDING
 * block.  The iterator will be left pointing to the block before the
 * one just deleted.
 *
 * You may not delete the STREAMINFO block.
 */
FLAC__bool FLAC__metadata_simple_iterator_delete_block(FLAC__MetaData_SimpleIterator *iterator, FLAC__bool use_padding);


/***********************************************************************
 * level 2
 * ---------------------------------------------------------------------
 * The general usage of this interface is:
 *
 * Create a new chain using FLAC__metadata_chain_new().  A chain is a
 *    linked list of metadata blocks.
 * Read all metadata into the the chain from a FLAC file using
 *    FLAC__metadata_chain_read() and check the status.
 * Optionally, consolidate the padding using
 *    FLAC__metadata_chain_merge_padding() or
 *    FLAC__metadata_chain_sort_padding().
 * Create a new iterator using FLAC__metadata_iterator_new()
 * Initialize the iterator to point to the first element in the chain
 *    using FLAC__metadata_iterator_init()
 * Traverse the chain using FLAC__metadata_iterator_next/prev().
 * Get a block for reading or modification using
 *    FLAC__metadata_iterator_get_block().  The pointer to the object
 *    inside the chain is returned, so the block is yours to modify.
 *    Changes will be reflected in the FLAC file when you write the
 *    chain.  You can also add and delete blocks (see functions below).
 * When done, write out the chain using FLAC__metadata_chain_write().
 *    Make sure to read the whole comment to the function below.
 * Delete the chain using FLAC__metadata_chain_delete().
 *
 * NOTE: Even though the FLAC file is not open while the chain is being
 *       manipulated, you must not alter the file externally during
 *       this time.  The chain assumes the FLAC file will not change
 *       between the time of FLAC__metadata_chain_read() and
 *       FLAC__metadata_chain_write().
 *
 * NOTE: Do not modify the is_last, length, or type fields of returned
 *       FLAC__MetaDataType objects.  These are managed automatically.
 */

/*
 * opaque structure definitions
 */
struct FLAC__MetaData_Chain;
typedef struct FLAC__MetaData_Chain FLAC__MetaData_Chain;
struct FLAC__MetaData_Iterator;
typedef struct FLAC__MetaData_Iterator FLAC__MetaData_Iterator;

typedef enum {
	FLAC__METADATA_CHAIN_STATUS_OK = 0,
	FLAC__METADATA_CHAIN_STATUS_ILLEGAL_INPUT,
	FLAC__METADATA_CHAIN_STATUS_ERROR_OPENING_FILE,
	FLAC__METADATA_CHAIN_STATUS_NOT_A_FLAC_FILE,
	FLAC__METADATA_CHAIN_STATUS_NOT_WRITABLE,
	FLAC__METADATA_CHAIN_STATUS_READ_ERROR,
	FLAC__METADATA_CHAIN_STATUS_SEEK_ERROR,
	FLAC__METADATA_CHAIN_STATUS_WRITE_ERROR,
	FLAC__METADATA_CHAIN_STATUS_RENAME_ERROR,
	FLAC__METADATA_CHAIN_STATUS_UNLINK_ERROR,
	FLAC__METADATA_CHAIN_STATUS_MEMORY_ALLOCATION_ERROR,
	FLAC__METADATA_CHAIN_STATUS_INTERNAL_ERROR
} FLAC__MetaData_Chain_Status;

/*********** FLAC__MetaData_Chain ***********/

/*
 * Constructor/destructor
 */
FLAC__MetaData_Chain *FLAC__metadata_chain_new();
void FLAC__metadata_chain_delete(FLAC__MetaData_Chain *chain);

/*
 * Get the current status of the chain.  Call this after a function
 * returns false to get the reason for the error.  Also resets the status
 * to FLAC__METADATA_CHAIN_STATUS_OK
 */
FLAC__MetaData_Chain_Status FLAC__metadata_chain_status(FLAC__MetaData_Chain *chain);

/*
 * Read all metadata into the chain
 */
FLAC__bool FLAC__metadata_chain_read(FLAC__MetaData_Chain *chain, const char *filename);

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
 * If the current chain is shorter than the existing metadata, and
 * use_padding is true, a PADDING block is added to the end of the new
 * data to make it the same size as the existing data (if possible, see
 * the note to FLAC__metadata_simple_iterator_set_block() about the four
 * byte limit) and the new data is written in place.  If use_padding is
 * false, the entire FLAC file is rewritten.
 *
 * If 'preserve_file_stats' is true, the owner and modification time will
 * be preserved even if the FLAC file is written.
 */
FLAC__bool FLAC__metadata_chain_write(FLAC__MetaData_Chain *chain, FLAC__bool use_padding, FLAC__bool preserve_file_stats);

/*
 * This function will merge adjacent PADDING blocks into a single block.
 *
 * NOTE: this function does not write to the FLAC file, it only
 * modifies the chain.
 *
 * NOTE: Any iterator on the current chain will become invalid after this
 * call.  You should delete the iterator and get a new one.
 */
void FLAC__metadata_chain_merge_padding(FLAC__MetaData_Chain *chain);

/*
 * This function will move all PADDING blocks to the end on the metadata,
 * then merge them into a single block.
 *
 * NOTE: this function does not write to the FLAC file, it only
 * modifies the chain.
 *
 * NOTE: Any iterator on the current chain will become invalid after this
 * call.  You should delete the iterator and get a new one.
 */
void FLAC__metadata_chain_sort_padding(FLAC__MetaData_Chain *chain);


/*********** FLAC__MetaData_Iterator ***********/

/*
 * Constructor/destructor
 */
FLAC__MetaData_Iterator *FLAC__metadata_iterator_new();
void FLAC__metadata_iterator_delete(FLAC__MetaData_Iterator *iterator);

/*
 * Initialize the iterator to point to the first metadata block in the
 * given chain.
 */
void FLAC__metadata_iterator_init(FLAC__MetaData_Iterator *iterator, FLAC__MetaData_Chain *chain);

/*
 * These move the iterator forwards or backwards, returning false if
 * already at the end.
 */
FLAC__bool FLAC__metadata_iterator_next(FLAC__MetaData_Iterator *iterator);
FLAC__bool FLAC__metadata_iterator_prev(FLAC__MetaData_Iterator *iterator);

/*
 * Get the type of the metadata block at the current position.
 */
FLAC__MetaDataType FLAC__metadata_iterator_get_block_type(const FLAC__MetaData_Iterator *iterator);

/*
 * Get the metadata block at the current position.  You can modify 
 * the block in place but must write the chain before the changes
 * are reflected to the FLAC file.
 *
 * Do not call FLAC__metadata_object_delete() on the returned object;
 * to delete a block use FLAC__metadata_iterator_delete_block().
 */
FLAC__StreamMetaData *FLAC__metadata_iterator_get_block(FLAC__MetaData_Iterator *iterator);

/*
 * Removes the current block from the chain.  If replace_with_padding is
 * true, the block will instead be replaced with a padding block of equal
 * size.  You can not delete the STREAMINFO block.  The iterator will be
 * left pointing to the block before the one just 'deleted', even if
 * 'replace_with_padding' is true.
 */
FLAC__bool FLAC__metadata_iterator_delete_block(FLAC__MetaData_Iterator *iterator, FLAC__bool replace_with_padding);

/*
 * Insert a new block before or after the current block.  You cannot
 * insert a block before the first STREAMINFO block.  You cannot
 * insert a STREAMINFO block as there can be only one, the one that
 * already exists at the head when you read in a chain.  The iterator
 * will be left pointing to the new block.
 */
FLAC__bool FLAC__metadata_iterator_insert_block_before(FLAC__MetaData_Iterator *iterator, FLAC__StreamMetaData *block);
FLAC__bool FLAC__metadata_iterator_insert_block_after(FLAC__MetaData_Iterator *iterator, FLAC__StreamMetaData *block);


/******************************************************************************
	The following are methods for manipulating the defined block types.
	Since many are variable length we have to be careful about the memory
	management.  We decree that all pointers to data in the object are
	owned by the object and memory-managed by the object.

	Use the _new and _delete functions to create all instances.  When
	using the _set_ functions to set pointers to data, set 'copy' to true
	to have the function make it's own copy of the data, or to false to
	give the object ownership of your data.  In the latter case your pointer
	must be freeable by free() and will be free()d when the object is
	_delete()d.

	The _new and _copy function will return NULL in the case of a memory
	allocation error, otherwise a new object.  The _set_ functions return
	false in the case of a memory allocation error.

	We don't have the convenience of C++ here, so note that the library
	relies on you to keep the types straight.  In other words, if you pass,
	for example, a FLAC__StreamMetaData* that represents a STREAMINFO block
	to FLAC__metadata_object_application_set_data(), you will get an
	assertion failure.

	There is no need to recalculate the length field on metadata blocks
	you have modified.  They will be calculated automatically before they
	are written back to a file.
******************************************************************************/


/******************************************************************
 * Common to all the types derived from FLAC__StreamMetaData:
 */
FLAC__StreamMetaData *FLAC__metadata_object_new(FLAC__MetaDataType type);
FLAC__StreamMetaData *FLAC__metadata_object_copy(const FLAC__StreamMetaData *object);
void FLAC__metadata_object_delete(FLAC__StreamMetaData *object);

/******************************************************************
 * FLAC__StreamMetaData_Application
 * ----------------------------------------------------------------
 * Note: 'length' is in bytes.
 */
FLAC__bool FLAC__metadata_object_application_set_data(FLAC__StreamMetaData *object, FLAC__byte *data, unsigned length, FLAC__bool copy);

/******************************************************************
 * FLAC__StreamMetaData_SeekPoint
 * ----------------------------------------------------------------
 * Note that we do not manipulate individual seek points as the
 * seek table holds a pointer to an array of seek points.  You can
 * use the _resize function to alter in.  If the size shrinks,
 * elements will truncated; if it grows, new elements will be added
 * to the end.
 */
FLAC__StreamMetaData_SeekPoint *FLAC__metadata_object_seekpoint_array_new(unsigned num_points);
FLAC__StreamMetaData_SeekPoint *FLAC__metadata_object_seekpoint_array_copy(const FLAC__StreamMetaData_SeekPoint *object_array, unsigned num_points);
void FLAC__metadata_object_seekpoint_array_delete(FLAC__StreamMetaData_SeekPoint *object_array);
FLAC__bool FLAC__metadata_object_seekpoint_array_resize(FLAC__StreamMetaData_SeekPoint **object_array, unsigned old_num_points, unsigned new_num_points);

/******************************************************************
 * FLAC__StreamMetaData_SeekTable
 */
FLAC__bool FLAC__metadata_object_seektable_set_points(FLAC__StreamMetaData *object, FLAC__StreamMetaData_SeekPoint *points, unsigned num_points, FLAC__bool copy);

/******************************************************************
 * FLAC__StreamMetaData_VorbisComment_Entry
 * ----------------------------------------------------------------
 * This is similar to FLAC__StreamMetaData_SeekPoint.
 */
FLAC__StreamMetaData_VorbisComment_Entry *FLAC__metadata_object_vorbiscomment_entry_array_new(unsigned num_comments);
FLAC__StreamMetaData_VorbisComment_Entry *FLAC__metadata_object_vorbiscomment_entry_array_copy(const FLAC__StreamMetaData_VorbisComment_Entry *object_array, unsigned num_comments);
void FLAC__metadata_object_vorbiscomment_entry_array_delete(FLAC__StreamMetaData_VorbisComment_Entry *object_array, unsigned num_comments);
FLAC__bool FLAC__metadata_object_vorbiscomment_entry_array_resize(FLAC__StreamMetaData_VorbisComment_Entry **object_array, unsigned old_num_comments, unsigned new_num_comments);

/******************************************************************
 * FLAC__StreamMetaData_VorbisComment
 */
FLAC__bool FLAC__metadata_object_vorbiscomment_set_vendor_string(FLAC__StreamMetaData *object, FLAC__byte *entry, unsigned length, FLAC__bool copy);
FLAC__bool FLAC__metadata_object_vorbiscomment_set_comments(FLAC__StreamMetaData *object, FLAC__StreamMetaData_VorbisComment_Entry *comments, unsigned num_comments, FLAC__bool copy);

#endif
