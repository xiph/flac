/* test_libOggFLAC - Unit tester for libOggFLAC
 * Copyright (C) 2002,2003,2004,2005,2006  Josh Coalson
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

#if HAVE_CONFIG_H
#  include <config.h>
#endif

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#if defined _MSC_VER || defined __MINGW32__
//@@@ [2G limit] hacks for MSVC6
#define fseeko fseek
#define ftello ftell
#endif
#include "decoders.h"
#include "FLAC/assert.h"
#include "OggFLAC/stream_decoder.h"
#include "share/grabbag.h"
#include "test_libs_common/file_utils_oggflac.h"
#include "test_libs_common/metadata_utils.h"

//@@@@@@ this is almost exactly like test_libFLAC/decoders.c, maybe should consolidate some stuff liek client data and callbacks
typedef enum {
	LAYER_STREAM = 0, /* FLAC__stream_decoder_init_stream() without seeking */
	LAYER_SEEKABLE_STREAM, /* FLAC__stream_decoder_init_stream() with seeking */
	LAYER_FILE, /* FLAC__stream_decoder_init_FILE() */
	LAYER_FILENAME /* FLAC__stream_decoder_init_file() */
} Layer;

static const char * const LayerString[] = {
	"Stream",
	"Seekable Stream",
	"FILE*",
	"Filename"
};

typedef struct {
	Layer layer;
	FILE *file;
	unsigned current_metadata_number;
	FLAC__bool ignore_errors;
	FLAC__bool error_occurred;
} StreamDecoderClientData;

static FLAC__StreamMetadata streaminfo_, padding_, seektable_, application1_, application2_, vorbiscomment_, cuesheet_, picture_, unknown_;
static FLAC__StreamMetadata *expected_metadata_sequence_[9];
static unsigned num_expected_;
static const char *flacfilename_ = "metadata.ogg";
static off_t flacfilesize_;

static FLAC__bool die_(const char *msg)
{
	printf("ERROR: %s\n", msg);
	return false;
}

static FLAC__bool die_s_(const char *msg, const OggFLAC__StreamDecoder *decoder)
{
	OggFLAC__StreamDecoderState state = OggFLAC__stream_decoder_get_state(decoder);

	if(msg)
		printf("FAILED, %s", msg);
	else
		printf("FAILED");

	printf(", state = %u (%s)\n", (unsigned)state, OggFLAC__StreamDecoderStateString[state]);
	if(state == OggFLAC__STREAM_DECODER_FLAC_STREAM_DECODER_ERROR) {
		FLAC__StreamDecoderState state_ = OggFLAC__stream_decoder_get_FLAC_stream_decoder_state(decoder);
		printf("      FLAC stream decoder state = %u (%s)\n", (unsigned)state_, FLAC__StreamDecoderStateString[state_]);
	}

	return false;
}

static void init_metadata_blocks_()
{
	mutils__init_metadata_blocks(&streaminfo_, &padding_, &seektable_, &application1_, &application2_, &vorbiscomment_, &cuesheet_, &picture_, &unknown_);
}

static void free_metadata_blocks_()
{
	mutils__free_metadata_blocks(&streaminfo_, &padding_, &seektable_, &application1_, &application2_, &vorbiscomment_, &cuesheet_, &picture_, &unknown_);
}

static FLAC__bool generate_file_()
{
	printf("\n\ngenerating Ogg FLAC file for decoder tests...\n");

	num_expected_ = 0;
	expected_metadata_sequence_[num_expected_++] = &padding_;
	expected_metadata_sequence_[num_expected_++] = &seektable_;
	expected_metadata_sequence_[num_expected_++] = &application1_;
	expected_metadata_sequence_[num_expected_++] = &application2_;
	expected_metadata_sequence_[num_expected_++] = &vorbiscomment_;
	expected_metadata_sequence_[num_expected_++] = &cuesheet_;
	expected_metadata_sequence_[num_expected_++] = &picture_;
	expected_metadata_sequence_[num_expected_++] = &unknown_;
	/* WATCHOUT: the encoder should move the VORBIS_COMMENT block to the front, right after STREAMINFO */

	if(!file_utils__generate_oggflacfile(flacfilename_, &flacfilesize_, 512 * 1024, &streaminfo_, expected_metadata_sequence_, num_expected_))
		return die_("creating the encoded file");

	return true;
}

static FLAC__StreamDecoderReadStatus stream_decoder_read_callback_(const FLAC__StreamDecoder *decoder, FLAC__byte buffer[], unsigned *bytes, void *client_data)
{
	StreamDecoderClientData *dcd = (StreamDecoderClientData*)client_data;
	const unsigned requested_bytes = *bytes;

	(void)decoder;

	if(0 == dcd) {
		printf("ERROR: client_data in read callback is NULL\n");
		return FLAC__STREAM_DECODER_READ_STATUS_ABORT;
	}

	if(dcd->error_occurred)
		return FLAC__STREAM_DECODER_READ_STATUS_ABORT;

	if(feof(dcd->file)) {
		*bytes = 0;
		return FLAC__STREAM_DECODER_READ_STATUS_END_OF_STREAM;
	}
	else if(requested_bytes > 0) {
		*bytes = fread(buffer, 1, requested_bytes, dcd->file);
		if(*bytes == 0) {
			if(feof(dcd->file))
				return FLAC__STREAM_DECODER_READ_STATUS_END_OF_STREAM;
			else
				return FLAC__STREAM_DECODER_READ_STATUS_ABORT;
		}
		else {
			return FLAC__STREAM_DECODER_READ_STATUS_CONTINUE;
		}
	}
	else
		return FLAC__STREAM_DECODER_READ_STATUS_ABORT; /* abort to avoid a deadlock */
}

static FLAC__StreamDecoderSeekStatus stream_decoder_seek_callback_(const FLAC__StreamDecoder *decoder, FLAC__uint64 absolute_byte_offset, void *client_data)
{
	StreamDecoderClientData *dcd = (StreamDecoderClientData*)client_data;

	(void)decoder;

	if(0 == dcd) {
		printf("ERROR: client_data in seek callback is NULL\n");
		return FLAC__STREAM_DECODER_SEEK_STATUS_ERROR;
	}

	if(dcd->error_occurred)
		return FLAC__STREAM_DECODER_SEEK_STATUS_ERROR;

	if(fseeko(dcd->file, (off_t)absolute_byte_offset, SEEK_SET) < 0) {
		dcd->error_occurred = true;
		return FLAC__STREAM_DECODER_SEEK_STATUS_ERROR;
	}

	return FLAC__STREAM_DECODER_SEEK_STATUS_OK;
}

static FLAC__StreamDecoderTellStatus stream_decoder_tell_callback_(const FLAC__StreamDecoder *decoder, FLAC__uint64 *absolute_byte_offset, void *client_data)
{
	StreamDecoderClientData *dcd = (StreamDecoderClientData*)client_data;
	off_t offset;

	(void)decoder;

	if(0 == dcd) {
		printf("ERROR: client_data in tell callback is NULL\n");
		return FLAC__STREAM_DECODER_TELL_STATUS_ERROR;
	}

	if(dcd->error_occurred)
		return FLAC__STREAM_DECODER_TELL_STATUS_ERROR;

	offset = ftello(dcd->file);
	*absolute_byte_offset = (FLAC__uint64)offset;

	if(offset < 0) {
		dcd->error_occurred = true;
		return FLAC__STREAM_DECODER_TELL_STATUS_ERROR;
	}

	return FLAC__STREAM_DECODER_TELL_STATUS_OK;
}

static FLAC__StreamDecoderLengthStatus stream_decoder_length_callback_(const FLAC__StreamDecoder *decoder, FLAC__uint64 *stream_length, void *client_data)
{
	StreamDecoderClientData *dcd = (StreamDecoderClientData*)client_data;

	(void)decoder;

	if(0 == dcd) {
		printf("ERROR: client_data in length callback is NULL\n");
		return FLAC__STREAM_DECODER_LENGTH_STATUS_ERROR;
	}

	if(dcd->error_occurred)
		return FLAC__STREAM_DECODER_LENGTH_STATUS_ERROR;

	*stream_length = (FLAC__uint64)flacfilesize_;
	return FLAC__STREAM_DECODER_LENGTH_STATUS_OK;
}

static FLAC__bool stream_decoder_eof_callback_(const FLAC__StreamDecoder *decoder, void *client_data)
{
	StreamDecoderClientData *dcd = (StreamDecoderClientData*)client_data;

	(void)decoder;

	if(0 == dcd) {
		printf("ERROR: client_data in eof callback is NULL\n");
		return true;
	}

	if(dcd->error_occurred)
		return true;

	return feof(dcd->file);
}

static FLAC__StreamDecoderWriteStatus stream_decoder_write_callback_(const FLAC__StreamDecoder *decoder, const FLAC__Frame *frame, const FLAC__int32 * const buffer[], void *client_data)
{
	StreamDecoderClientData *dcd = (StreamDecoderClientData*)client_data;

	(void)decoder, (void)buffer;

	if(0 == dcd) {
		printf("ERROR: client_data in write callback is NULL\n");
		return FLAC__STREAM_DECODER_WRITE_STATUS_ABORT;
	}

	if(dcd->error_occurred)
		return FLAC__STREAM_DECODER_WRITE_STATUS_ABORT;

	if(
		(frame->header.number_type == FLAC__FRAME_NUMBER_TYPE_FRAME_NUMBER && frame->header.number.frame_number == 0) ||
		(frame->header.number_type == FLAC__FRAME_NUMBER_TYPE_SAMPLE_NUMBER && frame->header.number.sample_number == 0)
	) {
		printf("content... ");
		fflush(stdout);
	}

	return FLAC__STREAM_DECODER_WRITE_STATUS_CONTINUE;
}

static void stream_decoder_metadata_callback_(const FLAC__StreamDecoder *decoder, const FLAC__StreamMetadata *metadata, void *client_data)
{
	StreamDecoderClientData *dcd = (StreamDecoderClientData*)client_data;

	(void)decoder;

	if(0 == dcd) {
		printf("ERROR: client_data in metadata callback is NULL\n");
		return;
	}

	if(dcd->error_occurred)
		return;

	printf("%d... ", dcd->current_metadata_number);
	fflush(stdout);

	if(dcd->current_metadata_number >= num_expected_) {
		(void)die_("got more metadata blocks than expected");
		dcd->error_occurred = true;
	}
	else {
		if(!mutils__compare_block(expected_metadata_sequence_[dcd->current_metadata_number], metadata)) {
			(void)die_("metadata block mismatch");
			dcd->error_occurred = true;
		}
	}
	dcd->current_metadata_number++;
}

static void stream_decoder_error_callback_(const FLAC__StreamDecoder *decoder, FLAC__StreamDecoderErrorStatus status, void *client_data)
{
	StreamDecoderClientData *dcd = (StreamDecoderClientData*)client_data;

	(void)decoder;

	if(0 == dcd) {
		printf("ERROR: client_data in error callback is NULL\n");
		return;
	}

	if(!dcd->ignore_errors) {
		printf("ERROR: got error callback: err = %u (%s)\n", (unsigned)status, FLAC__StreamDecoderErrorStatusString[status]);
		dcd->error_occurred = true;
	}
}

static FLAC__bool stream_decoder_test_respond_(OggFLAC__StreamDecoder *decoder, StreamDecoderClientData *dcd)
{
	FLAC__StreamDecoderInitStatus init_status;

	if(!OggFLAC__stream_decoder_set_md5_checking(decoder, true))
		return die_s_("at OggFLAC__stream_decoder_set_md5_checking(), returned false", decoder);

	/* for OggFLAC__stream_encoder_init_FILE(), the OggFLAC__stream_encoder_finish() closes the file so we have to keep re-opening: */
	if(dcd->layer == LAYER_FILE) {
		printf("opening Ogg FLAC file... ");
		dcd->file = fopen(flacfilename_, "rb");
		if(0 == dcd->file) {
			printf("ERROR (%s)\n", strerror(errno));
			return false;
		}
		printf("OK\n");
	}

	printf("testing OggFLAC__stream_decoder_init_stream()... ");
	switch(dcd->layer) {
		case LAYER_STREAM:
			init_status = OggFLAC__stream_decoder_init_stream(decoder, stream_decoder_read_callback_, /*seek_callback=*/0, /*tell_callback=*/0, /*length_callback=*/0, /*eof_callback=*/0, stream_decoder_write_callback_, stream_decoder_metadata_callback_, stream_decoder_error_callback_, dcd);
			break;
		case LAYER_SEEKABLE_STREAM:
			init_status = OggFLAC__stream_decoder_init_stream(decoder, stream_decoder_read_callback_, stream_decoder_seek_callback_, stream_decoder_tell_callback_, stream_decoder_length_callback_, stream_decoder_eof_callback_, stream_decoder_write_callback_, stream_decoder_metadata_callback_, stream_decoder_error_callback_, dcd);
			break;
		case LAYER_FILE:
			init_status = OggFLAC__stream_decoder_init_FILE(decoder, dcd->file, stream_decoder_write_callback_, stream_decoder_metadata_callback_, stream_decoder_error_callback_, dcd);
			break;
		case LAYER_FILENAME:
			init_status = OggFLAC__stream_decoder_init_file(decoder, flacfilename_, stream_decoder_write_callback_, stream_decoder_metadata_callback_, stream_decoder_error_callback_, dcd);
			break;
		default:
			die_("internal error 000");
			return false;
	}
	if(init_status != FLAC__STREAM_DECODER_INIT_STATUS_OK)
		return die_s_(0, decoder);
	printf("OK\n");

	dcd->current_metadata_number = 0;

	if(dcd->layer < LAYER_FILE && fseeko(dcd->file, 0, SEEK_SET) < 0) {
		printf("FAILED rewinding input, errno = %d\n", errno);
		return false;
	}

	printf("testing OggFLAC__stream_decoder_process_until_end_of_stream()... ");
	if(!OggFLAC__stream_decoder_process_until_end_of_stream(decoder))
		return die_s_("returned false", decoder);
	printf("OK\n");

	printf("testing OggFLAC__stream_decoder_finish()... ");
	OggFLAC__stream_decoder_finish(decoder);
	printf("OK\n");

	return true;
}

static FLAC__bool test_stream_decoder(Layer layer)
{
	OggFLAC__StreamDecoder *decoder;
	OggFLAC__StreamDecoderState state;
	FLAC__StreamDecoderState fstate;
	StreamDecoderClientData decoder_client_data;
	FLAC__bool expect;

	decoder_client_data.layer = layer;

	printf("\n+++ libOggFLAC unit test: OggFLAC__StreamDecoder (layer: %s)\n\n", LayerString[layer]);

	printf("testing OggFLAC__stream_decoder_new()... ");
	decoder = OggFLAC__stream_decoder_new();
	if(0 == decoder) {
		printf("FAILED, returned NULL\n");
		return false;
	}
	printf("OK\n");

	printf("testing OggFLAC__stream_decoder_delete()... ");
	OggFLAC__stream_decoder_delete(decoder);
	printf("OK\n");

	printf("testing OggFLAC__stream_decoder_new()... ");
	decoder = OggFLAC__stream_decoder_new();
	if(0 == decoder) {
		printf("FAILED, returned NULL\n");
		return false;
	}
	printf("OK\n");

	switch(layer) {
		case LAYER_STREAM:
		case LAYER_SEEKABLE_STREAM:
			printf("testing OggFLAC__stream_decoder_init_stream()... ");
			if(OggFLAC__stream_decoder_init_stream(decoder, 0, 0, 0, 0, 0, 0, 0, 0, 0) != FLAC__STREAM_DECODER_INIT_STATUS_INVALID_CALLBACKS)
				return die_s_(0, decoder);
			break;
		case LAYER_FILE:
			printf("testing OggFLAC__stream_decoder_init_FILE()... ");
			if(OggFLAC__stream_decoder_init_FILE(decoder, stdin, 0, 0, 0, 0) != FLAC__STREAM_DECODER_INIT_STATUS_INVALID_CALLBACKS)
				return die_s_(0, decoder);
			break;
		case LAYER_FILENAME:
			printf("testing OggFLAC__stream_decoder_init_file()... ");
			if(OggFLAC__stream_decoder_init_file(decoder, flacfilename_, 0, 0, 0, 0) != FLAC__STREAM_DECODER_INIT_STATUS_INVALID_CALLBACKS)
				return die_s_(0, decoder);
			break;
		default:
			die_("internal error 003");
			return false;
	}
	printf("OK\n");

	printf("testing OggFLAC__stream_decoder_delete()... ");
	OggFLAC__stream_decoder_delete(decoder);
	printf("OK\n");

	num_expected_ = 0;
	expected_metadata_sequence_[num_expected_++] = &streaminfo_;

	printf("testing OggFLAC__stream_decoder_new()... ");
	decoder = OggFLAC__stream_decoder_new();
	if(0 == decoder) {
		printf("FAILED, returned NULL\n");
		return false;
	}
	printf("OK\n");

	printf("testing OggFLAC__stream_decoder_set_serial_number()... ");
	if(!OggFLAC__stream_decoder_set_serial_number(decoder, file_utils__serial_number))
		return die_s_("returned false", decoder);
	printf("OK\n");

	printf("testing OggFLAC__stream_decoder_set_md5_checking()... ");
	if(!OggFLAC__stream_decoder_set_md5_checking(decoder, true))
		return die_s_("returned false", decoder);
	printf("OK\n");

	if(layer < LAYER_FILENAME) {
		printf("opening Ogg FLAC file... ");
		decoder_client_data.file = fopen(flacfilename_, "rb");
		if(0 == decoder_client_data.file) {
			printf("ERROR (%s)\n", strerror(errno));
			return false;
		}
		printf("OK\n");
	}

	switch(layer) {
		case LAYER_STREAM:
			printf("testing OggFLAC__stream_decoder_init_stream()... ");
			if(OggFLAC__stream_decoder_init_stream(decoder, stream_decoder_read_callback_, /*seek_callback=*/0, /*tell_callback=*/0, /*length_callback=*/0, /*eof_callback=*/0, stream_decoder_write_callback_, stream_decoder_metadata_callback_, stream_decoder_error_callback_, &decoder_client_data) != FLAC__STREAM_DECODER_INIT_STATUS_OK)
				return die_s_(0, decoder);
			break;
		case LAYER_SEEKABLE_STREAM:
			printf("testing OggFLAC__stream_decoder_init_stream()... ");
			if(OggFLAC__stream_decoder_init_stream(decoder, stream_decoder_read_callback_, stream_decoder_seek_callback_, stream_decoder_tell_callback_, stream_decoder_length_callback_, stream_decoder_eof_callback_, stream_decoder_write_callback_, stream_decoder_metadata_callback_, stream_decoder_error_callback_, &decoder_client_data) != FLAC__STREAM_DECODER_INIT_STATUS_OK)
				return die_s_(0, decoder);
			break;
		case LAYER_FILE:
			printf("testing OggFLAC__stream_decoder_init_FILE()... ");
			if(OggFLAC__stream_decoder_init_FILE(decoder, decoder_client_data.file, stream_decoder_write_callback_, stream_decoder_metadata_callback_, stream_decoder_error_callback_, &decoder_client_data) != FLAC__STREAM_DECODER_INIT_STATUS_OK)
				return die_s_(0, decoder);
			break;
		case LAYER_FILENAME:
			printf("testing OggFLAC__stream_decoder_init_file()... ");
			if(OggFLAC__stream_decoder_init_file(decoder, flacfilename_, stream_decoder_write_callback_, stream_decoder_metadata_callback_, stream_decoder_error_callback_, &decoder_client_data) != FLAC__STREAM_DECODER_INIT_STATUS_OK)
				return die_s_(0, decoder);
			break;
		default:
			die_("internal error 009");
			return false;
	}
	printf("OK\n");

	printf("testing OggFLAC__stream_decoder_get_state()... ");
	state = OggFLAC__stream_decoder_get_state(decoder);
	printf("returned state = %u (%s)... OK\n", state, OggFLAC__StreamDecoderStateString[state]);

	printf("testing OggFLAC__stream_decoder_get_FLAC_stream_decoder_state()... ");
	fstate = OggFLAC__stream_decoder_get_FLAC_stream_decoder_state(decoder);
	printf("returned state = %u (%s)... OK\n", fstate, FLAC__StreamDecoderStateString[fstate]);

	decoder_client_data.current_metadata_number = 0;
	decoder_client_data.ignore_errors = false;
	decoder_client_data.error_occurred = false;

	printf("testing OggFLAC__stream_decoder_get_md5_checking()... ");
	if(!OggFLAC__stream_decoder_get_md5_checking(decoder)) {
		printf("FAILED, returned false, expected true\n");
		return false;
	}
	printf("OK\n");

	printf("testing OggFLAC__stream_decoder_process_until_end_of_metadata()... ");
	if(!OggFLAC__stream_decoder_process_until_end_of_metadata(decoder))
		return die_s_("returned false", decoder);
	printf("OK\n");

	printf("testing OggFLAC__stream_decoder_process_single()... ");
	if(!OggFLAC__stream_decoder_process_single(decoder))
		return die_s_("returned false", decoder);
	printf("OK\n");

	printf("testing OggFLAC__stream_decoder_skip_single_frame()... ");
	if(!OggFLAC__stream_decoder_skip_single_frame(decoder))
		return die_s_("returned false", decoder);
	printf("OK\n");

	if(layer < LAYER_FILE) {
		printf("testing OggFLAC__stream_decoder_flush()... ");
		if(!OggFLAC__stream_decoder_flush(decoder))
			return die_s_("returned false", decoder);
		printf("OK\n");

		decoder_client_data.ignore_errors = true;
		printf("testing OggFLAC__stream_decoder_process_single()... ");
		if(!OggFLAC__stream_decoder_process_single(decoder))
			return die_s_("returned false", decoder);
		printf("OK\n");
		decoder_client_data.ignore_errors = false;
	}

	expect = (layer != LAYER_STREAM);
	printf("testing OggFLAC__stream_decoder_seek_absolute()... ");
	if(OggFLAC__stream_decoder_seek_absolute(decoder, 0) != expect)
		return die_s_(expect? "returned false" : "returned true", decoder);
	printf("OK\n");

	printf("testing OggFLAC__stream_decoder_process_until_end_of_stream()... ");
	if(!OggFLAC__stream_decoder_process_until_end_of_stream(decoder))
		return die_s_("returned false", decoder);
	printf("OK\n");

	expect = (layer != LAYER_STREAM);
	printf("testing OggFLAC__stream_decoder_seek_absolute()... ");
	if(OggFLAC__stream_decoder_seek_absolute(decoder, 0) != expect)
		return die_s_(expect? "returned false" : "returned true", decoder);
	printf("OK\n");

	printf("testing OggFLAC__stream_decoder_get_channels()... ");
	{
		unsigned channels = OggFLAC__stream_decoder_get_channels(decoder);
		if(channels != streaminfo_.data.stream_info.channels) {
			printf("FAILED, returned %u, expected %u\n", channels, streaminfo_.data.stream_info.channels);
			return false;
		}
	}
	printf("OK\n");

	printf("testing OggFLAC__stream_decoder_get_bits_per_sample()... ");
	{
		unsigned bits_per_sample = OggFLAC__stream_decoder_get_bits_per_sample(decoder);
		if(bits_per_sample != streaminfo_.data.stream_info.bits_per_sample) {
			printf("FAILED, returned %u, expected %u\n", bits_per_sample, streaminfo_.data.stream_info.bits_per_sample);
			return false;
		}
	}
	printf("OK\n");

	printf("testing OggFLAC__stream_decoder_get_sample_rate()... ");
	{
		unsigned sample_rate = OggFLAC__stream_decoder_get_sample_rate(decoder);
		if(sample_rate != streaminfo_.data.stream_info.sample_rate) {
			printf("FAILED, returned %u, expected %u\n", sample_rate, streaminfo_.data.stream_info.sample_rate);
			return false;
		}
	}
	printf("OK\n");

	printf("testing OggFLAC__stream_decoder_get_blocksize()... ");
	{
		unsigned blocksize = OggFLAC__stream_decoder_get_blocksize(decoder);
		/* value could be anything since we're at the last block, so accept any reasonable answer */
		printf("returned %u... %s\n", blocksize, blocksize>0? "OK" : "FAILED");
		if(blocksize == 0)
			return false;
	}

	printf("testing OggFLAC__stream_decoder_get_channel_assignment()... ");
	{
		FLAC__ChannelAssignment ca = OggFLAC__stream_decoder_get_channel_assignment(decoder);
		printf("returned %u (%s)... OK\n", (unsigned)ca, FLAC__ChannelAssignmentString[ca]);
	}

	if(layer < LAYER_FILE) {
		printf("testing OggFLAC__stream_decoder_reset()... ");
		if(!OggFLAC__stream_decoder_reset(decoder)) {
			state = OggFLAC__stream_decoder_get_state(decoder);
			printf("FAILED, returned false, state = %u (%s)\n", state, OggFLAC__StreamDecoderStateString[state]);
			return false;
		}
		printf("OK\n");

		if(layer == LAYER_STREAM) {
			/* after a reset() we have to rewind the input ourselves */
			printf("rewinding input... ");
			if(fseeko(decoder_client_data.file, 0, SEEK_SET) < 0) {
				printf("FAILED, errno = %d\n", errno);
				return false;
			}
			printf("OK\n");
		}

		decoder_client_data.current_metadata_number = 0;

		printf("testing OggFLAC__stream_decoder_process_until_end_of_stream()... ");
		if(!OggFLAC__stream_decoder_process_until_end_of_stream(decoder))
			return die_s_("returned false", decoder);
		printf("OK\n");
	}

	printf("testing OggFLAC__stream_decoder_finish()... ");
	(void) OggFLAC__stream_decoder_finish(decoder);
	printf("OK\n");

	/*
	 * respond all
	 */

	printf("testing OggFLAC__stream_decoder_set_metadata_respond_all()... ");
	if(!OggFLAC__stream_decoder_set_metadata_respond_all(decoder))
		return die_s_("returned false", decoder);
	printf("OK\n");

	num_expected_ = 0;
	expected_metadata_sequence_[num_expected_++] = &streaminfo_;
	expected_metadata_sequence_[num_expected_++] = &vorbiscomment_;
	expected_metadata_sequence_[num_expected_++] = &padding_;
	expected_metadata_sequence_[num_expected_++] = &seektable_;
	expected_metadata_sequence_[num_expected_++] = &application1_;
	expected_metadata_sequence_[num_expected_++] = &application2_;
	expected_metadata_sequence_[num_expected_++] = &cuesheet_;
	expected_metadata_sequence_[num_expected_++] = &picture_;
	expected_metadata_sequence_[num_expected_++] = &unknown_;

	if(!stream_decoder_test_respond_(decoder, &decoder_client_data))
		return false;

	/*
	 * ignore all
	 */

	printf("testing OggFLAC__stream_decoder_set_metadata_ignore_all()... ");
	if(!OggFLAC__stream_decoder_set_metadata_ignore_all(decoder))
		return die_s_("returned false", decoder);
	printf("OK\n");

	num_expected_ = 0;

	if(!stream_decoder_test_respond_(decoder, &decoder_client_data))
		return false;

	/*
	 * respond all, ignore VORBIS_COMMENT
	 */

	printf("testing OggFLAC__stream_decoder_set_metadata_respond_all()... ");
	if(!OggFLAC__stream_decoder_set_metadata_respond_all(decoder))
		return die_s_("returned false", decoder);
	printf("OK\n");

	printf("testing OggFLAC__stream_decoder_set_metadata_ignore(VORBIS_COMMENT)... ");
	if(!OggFLAC__stream_decoder_set_metadata_ignore(decoder, FLAC__METADATA_TYPE_VORBIS_COMMENT))
		return die_s_("returned false", decoder);
	printf("OK\n");

	num_expected_ = 0;
	expected_metadata_sequence_[num_expected_++] = &streaminfo_;
	expected_metadata_sequence_[num_expected_++] = &padding_;
	expected_metadata_sequence_[num_expected_++] = &seektable_;
	expected_metadata_sequence_[num_expected_++] = &application1_;
	expected_metadata_sequence_[num_expected_++] = &application2_;
	expected_metadata_sequence_[num_expected_++] = &cuesheet_;
	expected_metadata_sequence_[num_expected_++] = &picture_;
	expected_metadata_sequence_[num_expected_++] = &unknown_;

	if(!stream_decoder_test_respond_(decoder, &decoder_client_data))
		return false;

	/*
	 * respond all, ignore APPLICATION
	 */

	printf("testing OggFLAC__stream_decoder_set_metadata_respond_all()... ");
	if(!OggFLAC__stream_decoder_set_metadata_respond_all(decoder))
		return die_s_("returned false", decoder);
	printf("OK\n");

	printf("testing OggFLAC__stream_decoder_set_metadata_ignore(APPLICATION)... ");
	if(!OggFLAC__stream_decoder_set_metadata_ignore(decoder, FLAC__METADATA_TYPE_APPLICATION))
		return die_s_("returned false", decoder);
	printf("OK\n");

	num_expected_ = 0;
	expected_metadata_sequence_[num_expected_++] = &streaminfo_;
	expected_metadata_sequence_[num_expected_++] = &vorbiscomment_;
	expected_metadata_sequence_[num_expected_++] = &padding_;
	expected_metadata_sequence_[num_expected_++] = &seektable_;
	expected_metadata_sequence_[num_expected_++] = &cuesheet_;
	expected_metadata_sequence_[num_expected_++] = &picture_;
	expected_metadata_sequence_[num_expected_++] = &unknown_;

	if(!stream_decoder_test_respond_(decoder, &decoder_client_data))
		return false;

	/*
	 * respond all, ignore APPLICATION id of app#1
	 */

	printf("testing OggFLAC__stream_decoder_set_metadata_respond_all()... ");
	if(!OggFLAC__stream_decoder_set_metadata_respond_all(decoder))
		return die_s_("returned false", decoder);
	printf("OK\n");

	printf("testing OggFLAC__stream_decoder_set_metadata_ignore_application(of app block #1)... ");
	if(!OggFLAC__stream_decoder_set_metadata_ignore_application(decoder, application1_.data.application.id))
		return die_s_("returned false", decoder);
	printf("OK\n");

	num_expected_ = 0;
	expected_metadata_sequence_[num_expected_++] = &streaminfo_;
	expected_metadata_sequence_[num_expected_++] = &vorbiscomment_;
	expected_metadata_sequence_[num_expected_++] = &padding_;
	expected_metadata_sequence_[num_expected_++] = &seektable_;
	expected_metadata_sequence_[num_expected_++] = &application2_;
	expected_metadata_sequence_[num_expected_++] = &cuesheet_;
	expected_metadata_sequence_[num_expected_++] = &picture_;
	expected_metadata_sequence_[num_expected_++] = &unknown_;

	if(!stream_decoder_test_respond_(decoder, &decoder_client_data))
		return false;

	/*
	 * respond all, ignore APPLICATION id of app#1 & app#2
	 */

	printf("testing OggFLAC__stream_decoder_set_metadata_respond_all()... ");
	if(!OggFLAC__stream_decoder_set_metadata_respond_all(decoder))
		return die_s_("returned false", decoder);
	printf("OK\n");

	printf("testing OggFLAC__stream_decoder_set_metadata_ignore_application(of app block #1)... ");
	if(!OggFLAC__stream_decoder_set_metadata_ignore_application(decoder, application1_.data.application.id))
		return die_s_("returned false", decoder);
	printf("OK\n");

	printf("testing OggFLAC__stream_decoder_set_metadata_ignore_application(of app block #2)... ");
	if(!OggFLAC__stream_decoder_set_metadata_ignore_application(decoder, application2_.data.application.id))
		return die_s_("returned false", decoder);
	printf("OK\n");

	num_expected_ = 0;
	expected_metadata_sequence_[num_expected_++] = &streaminfo_;
	expected_metadata_sequence_[num_expected_++] = &vorbiscomment_;
	expected_metadata_sequence_[num_expected_++] = &padding_;
	expected_metadata_sequence_[num_expected_++] = &seektable_;
	expected_metadata_sequence_[num_expected_++] = &cuesheet_;
	expected_metadata_sequence_[num_expected_++] = &picture_;
	expected_metadata_sequence_[num_expected_++] = &unknown_;

	if(!stream_decoder_test_respond_(decoder, &decoder_client_data))
		return false;

	/*
	 * ignore all, respond VORBIS_COMMENT
	 */

	printf("testing OggFLAC__stream_decoder_set_metadata_ignore_all()... ");
	if(!OggFLAC__stream_decoder_set_metadata_ignore_all(decoder))
		return die_s_("returned false", decoder);
	printf("OK\n");

	printf("testing OggFLAC__stream_decoder_set_metadata_respond(VORBIS_COMMENT)... ");
	if(!OggFLAC__stream_decoder_set_metadata_respond(decoder, FLAC__METADATA_TYPE_VORBIS_COMMENT))
		return die_s_("returned false", decoder);
	printf("OK\n");

	num_expected_ = 0;
	expected_metadata_sequence_[num_expected_++] = &vorbiscomment_;

	if(!stream_decoder_test_respond_(decoder, &decoder_client_data))
		return false;

	/*
	 * ignore all, respond APPLICATION
	 */

	printf("testing OggFLAC__stream_decoder_set_metadata_ignore_all()... ");
	if(!OggFLAC__stream_decoder_set_metadata_ignore_all(decoder))
		return die_s_("returned false", decoder);
	printf("OK\n");

	printf("testing OggFLAC__stream_decoder_set_metadata_respond(APPLICATION)... ");
	if(!OggFLAC__stream_decoder_set_metadata_respond(decoder, FLAC__METADATA_TYPE_APPLICATION))
		return die_s_("returned false", decoder);
	printf("OK\n");

	num_expected_ = 0;
	expected_metadata_sequence_[num_expected_++] = &application1_;
	expected_metadata_sequence_[num_expected_++] = &application2_;

	if(!stream_decoder_test_respond_(decoder, &decoder_client_data))
		return false;

	/*
	 * ignore all, respond APPLICATION id of app#1
	 */

	printf("testing OggFLAC__stream_decoder_set_metadata_ignore_all()... ");
	if(!OggFLAC__stream_decoder_set_metadata_ignore_all(decoder))
		return die_s_("returned false", decoder);
	printf("OK\n");

	printf("testing OggFLAC__stream_decoder_set_metadata_respond_application(of app block #1)... ");
	if(!OggFLAC__stream_decoder_set_metadata_respond_application(decoder, application1_.data.application.id))
		return die_s_("returned false", decoder);
	printf("OK\n");

	num_expected_ = 0;
	expected_metadata_sequence_[num_expected_++] = &application1_;

	if(!stream_decoder_test_respond_(decoder, &decoder_client_data))
		return false;

	/*
	 * ignore all, respond APPLICATION id of app#1 & app#2
	 */

	printf("testing OggFLAC__stream_decoder_set_metadata_ignore_all()... ");
	if(!OggFLAC__stream_decoder_set_metadata_ignore_all(decoder))
		return die_s_("returned false", decoder);
	printf("OK\n");

	printf("testing OggFLAC__stream_decoder_set_metadata_respond_application(of app block #1)... ");
	if(!OggFLAC__stream_decoder_set_metadata_respond_application(decoder, application1_.data.application.id))
		return die_s_("returned false", decoder);
	printf("OK\n");

	printf("testing OggFLAC__stream_decoder_set_metadata_respond_application(of app block #2)... ");
	if(!OggFLAC__stream_decoder_set_metadata_respond_application(decoder, application2_.data.application.id))
		return die_s_("returned false", decoder);
	printf("OK\n");

	num_expected_ = 0;
	expected_metadata_sequence_[num_expected_++] = &application1_;
	expected_metadata_sequence_[num_expected_++] = &application2_;

	if(!stream_decoder_test_respond_(decoder, &decoder_client_data))
		return false;

	/*
	 * respond all, ignore APPLICATION, respond APPLICATION id of app#1
	 */

	printf("testing OggFLAC__stream_decoder_set_metadata_respond_all()... ");
	if(!OggFLAC__stream_decoder_set_metadata_respond_all(decoder))
		return die_s_("returned false", decoder);
	printf("OK\n");

	printf("testing OggFLAC__stream_decoder_set_metadata_ignore(APPLICATION)... ");
	if(!OggFLAC__stream_decoder_set_metadata_ignore(decoder, FLAC__METADATA_TYPE_APPLICATION))
		return die_s_("returned false", decoder);
	printf("OK\n");

	printf("testing OggFLAC__stream_decoder_set_metadata_respond_application(of app block #1)... ");
	if(!OggFLAC__stream_decoder_set_metadata_respond_application(decoder, application1_.data.application.id))
		return die_s_("returned false", decoder);
	printf("OK\n");

	num_expected_ = 0;
	expected_metadata_sequence_[num_expected_++] = &streaminfo_;
	expected_metadata_sequence_[num_expected_++] = &vorbiscomment_;
	expected_metadata_sequence_[num_expected_++] = &padding_;
	expected_metadata_sequence_[num_expected_++] = &seektable_;
	expected_metadata_sequence_[num_expected_++] = &application1_;
	expected_metadata_sequence_[num_expected_++] = &cuesheet_;
	expected_metadata_sequence_[num_expected_++] = &picture_;
	expected_metadata_sequence_[num_expected_++] = &unknown_;

	if(!stream_decoder_test_respond_(decoder, &decoder_client_data))
		return false;

	/*
	 * ignore all, respond APPLICATION, ignore APPLICATION id of app#1
	 */

	printf("testing OggFLAC__stream_decoder_set_metadata_ignore_all()... ");
	if(!OggFLAC__stream_decoder_set_metadata_ignore_all(decoder))
		return die_s_("returned false", decoder);
	printf("OK\n");

	printf("testing OggFLAC__stream_decoder_set_metadata_respond(APPLICATION)... ");
	if(!OggFLAC__stream_decoder_set_metadata_respond(decoder, FLAC__METADATA_TYPE_APPLICATION))
		return die_s_("returned false", decoder);
	printf("OK\n");

	printf("testing OggFLAC__stream_decoder_set_metadata_ignore_application(of app block #1)... ");
	if(!OggFLAC__stream_decoder_set_metadata_ignore_application(decoder, application1_.data.application.id))
		return die_s_("returned false", decoder);
	printf("OK\n");

	num_expected_ = 0;
	expected_metadata_sequence_[num_expected_++] = &application2_;

	if(!stream_decoder_test_respond_(decoder, &decoder_client_data))
		return false;

	if(layer < LAYER_FILE) /* for LAYER_FILE, FLAC__stream_decoder_finish() closes the file */
		fclose(decoder_client_data.file);

	printf("testing OggFLAC__stream_decoder_delete()... ");
	OggFLAC__stream_decoder_delete(decoder);
	printf("OK\n");

	printf("\nPASSED!\n");

	return true;
}

FLAC__bool test_decoders()
{
	init_metadata_blocks_();
	if(!generate_file_())
		return false;

	if(!test_stream_decoder(LAYER_STREAM))
		return false;

	if(!test_stream_decoder(LAYER_SEEKABLE_STREAM))
		return false;

	if(!test_stream_decoder(LAYER_FILE))
		return false;

	if(!test_stream_decoder(LAYER_FILENAME))
		return false;

	(void) grabbag__file_remove_file(flacfilename_);
	free_metadata_blocks_();

	return true;
}
