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
#include "FLAC/file_decoder.h"
#include "FLAC/metadata.h"
#include "FLAC/stream_encoder.h"
#include <stdio.h>
#include <stdlib.h> /* for malloc() */
#include <string.h> /* for memcmp() */

typedef struct {
	FLAC__bool error_occurred;
} decoder_client_struct;

typedef struct {
	FILE *file;
} encoder_client_struct;

static FLAC__bool die(const char *msg)
{
	printf("ERROR: %s\n", msg);
	return false;
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

static FLAC__bool generate_file_(const char *filename)
{
	FLAC__StreamEncoder *encoder;
	encoder_client_struct encoder_client_data;

	FLAC__ASSERT(0 != filename);

	encoder = FLAC__stream_encoder_new();
	if(0 == encoder)
		return die("creating the encoder instance");

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

	if(FLAC__stream_encoder_init(encoder) != FLAC__STREAM_ENCODER_OK)
		return die("initializing encoder");

	return true;
}

static FLAC__bool test_file_(const char *filename, void (*metadata_callback)(const FLAC__FileDecoder *decoder, const FLAC__StreamMetaData *metadata, void *client_data))
{
	FLAC__FileDecoder *decoder;
	decoder_client_struct decoder_client_data;

	FLAC__ASSERT(0 != filename);
	FLAC__ASSERT(0 != metadata_callback);

	decoder_client_data.error_occurred = false;

	if(0 == (decoder = FLAC__file_decoder_new()))
		return die("couldn't allocate memory");

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
		return die("initializing decoder\n");
	}
	if(!FLAC__file_decoder_process_metadata(decoder)) {
		FLAC__file_decoder_finish(decoder);
		FLAC__file_decoder_delete(decoder);
		return die("decoding file\n");
	}

	FLAC__file_decoder_finish(decoder);
	FLAC__file_decoder_delete(decoder);

	return !decoder_client_data.error_occurred;
}

int test_metadata_file_manipulation()
{
	printf("\n+++ unit test: metadata manipulation\n\n");

	return 0;
}
