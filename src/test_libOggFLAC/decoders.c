/* test_libOggFLAC - Unit tester for libOggFLAC
 * Copyright (C) 2002,2003,2004,2005  Josh Coalson
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

#include "decoders.h"
#include "file_utils.h"
#include "metadata_utils.h"
#include "FLAC/assert.h"
#include "OggFLAC/file_decoder.h"
#include "OggFLAC/seekable_stream_decoder.h"
#include "OggFLAC/stream_decoder.h"
#include "share/grabbag.h"
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
	FILE *file;
	unsigned current_metadata_number;
	FLAC__bool ignore_errors;
	FLAC__bool error_occurred;
} stream_decoder_client_data_struct;

typedef stream_decoder_client_data_struct seekable_stream_decoder_client_data_struct;
typedef stream_decoder_client_data_struct file_decoder_client_data_struct;

static FLAC__StreamMetadata streaminfo_, padding_, seektable_, application1_, application2_, vorbiscomment_, cuesheet_, unknown_;
static FLAC__StreamMetadata *expected_metadata_sequence_[8];
static unsigned num_expected_;
static const char *oggflacfilename_ = "metadata.ogg";
static unsigned oggflacfilesize_;

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

static FLAC__bool die_ss_(const char *msg, const OggFLAC__SeekableStreamDecoder *decoder)
{
	OggFLAC__SeekableStreamDecoderState state = OggFLAC__seekable_stream_decoder_get_state(decoder);

	if(msg)
		printf("FAILED, %s", msg);
	else
		printf("FAILED");

	printf(", state = %u (%s)\n", (unsigned)state, OggFLAC__SeekableStreamDecoderStateString[state]);
	if(state == OggFLAC__SEEKABLE_STREAM_DECODER_STREAM_DECODER_ERROR) {
		OggFLAC__StreamDecoderState state_ = OggFLAC__seekable_stream_decoder_get_stream_decoder_state(decoder);
		printf("      stream decoder state = %u (%s)\n", (unsigned)state_, OggFLAC__StreamDecoderStateString[state_]);
		if(state_ == OggFLAC__STREAM_DECODER_FLAC_STREAM_DECODER_ERROR) {
			FLAC__StreamDecoderState state__ = OggFLAC__seekable_stream_decoder_get_FLAC_stream_decoder_state(decoder);
			printf("      FLAC stream decoder state = %u (%s)\n", (unsigned)state__, FLAC__StreamDecoderStateString[state__]);
		}
	}

	return false;
}

static FLAC__bool die_f_(const char *msg, const OggFLAC__FileDecoder *decoder)
{
	OggFLAC__FileDecoderState state = OggFLAC__file_decoder_get_state(decoder);

	if(msg)
		printf("FAILED, %s", msg);
	else
		printf("FAILED");

	printf(", state = %u (%s)\n", (unsigned)state, OggFLAC__SeekableStreamDecoderStateString[state]);
	if(state == OggFLAC__FILE_DECODER_SEEKABLE_STREAM_DECODER_ERROR) {
		OggFLAC__SeekableStreamDecoderState state_ = OggFLAC__file_decoder_get_seekable_stream_decoder_state(decoder);
		printf("      seekable stream decoder state = %u (%s)\n", (unsigned)state_, OggFLAC__SeekableStreamDecoderStateString[state_]);
		if(state_ == OggFLAC__SEEKABLE_STREAM_DECODER_STREAM_DECODER_ERROR) {
			OggFLAC__StreamDecoderState state__ = OggFLAC__file_decoder_get_stream_decoder_state(decoder);
			printf("      stream decoder state = %u (%s)\n", (unsigned)state__, OggFLAC__StreamDecoderStateString[state__]);
			if(state__ == OggFLAC__STREAM_DECODER_FLAC_STREAM_DECODER_ERROR) {
				FLAC__StreamDecoderState state___ = OggFLAC__file_decoder_get_FLAC_stream_decoder_state(decoder);
				printf("      FLAC stream decoder state = %u (%s)\n", (unsigned)state___, FLAC__StreamDecoderStateString[state___]);
			}
		}
	}

	return false;
}

static void init_metadata_blocks_()
{
	mutils__init_metadata_blocks(&streaminfo_, &padding_, &seektable_, &application1_, &application2_, &vorbiscomment_, &cuesheet_, &unknown_);
}

static void free_metadata_blocks_()
{
	mutils__free_metadata_blocks(&streaminfo_, &padding_, &seektable_, &application1_, &application2_, &vorbiscomment_, &cuesheet_, &unknown_);
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
	expected_metadata_sequence_[num_expected_++] = &unknown_;
	/* WATCHOUT: the encoder should move the VORBIS_COMMENT block to the front, right after STREAMINFO */

	if(!file_utils__generate_oggflacfile(oggflacfilename_, &oggflacfilesize_, 512 * 1024, &streaminfo_, expected_metadata_sequence_, num_expected_))
		return die_("creating the encoded file");

	return true;
}

static FLAC__StreamDecoderReadStatus stream_decoder_read_callback_(const OggFLAC__StreamDecoder *decoder, FLAC__byte buffer[], unsigned *bytes, void *client_data)
{
	stream_decoder_client_data_struct *dcd = (stream_decoder_client_data_struct*)client_data;

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
	else if(*bytes > 0) {
		*bytes = fread(buffer, 1, *bytes, dcd->file);
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

static FLAC__StreamDecoderWriteStatus stream_decoder_write_callback_(const OggFLAC__StreamDecoder *decoder, const FLAC__Frame *frame, const FLAC__int32 * const buffer[], void *client_data)
{
	stream_decoder_client_data_struct *dcd = (stream_decoder_client_data_struct*)client_data;

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

static void stream_decoder_metadata_callback_(const OggFLAC__StreamDecoder *decoder, const FLAC__StreamMetadata *metadata, void *client_data)
{
	stream_decoder_client_data_struct *dcd = (stream_decoder_client_data_struct*)client_data;

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

static void stream_decoder_error_callback_(const OggFLAC__StreamDecoder *decoder, FLAC__StreamDecoderErrorStatus status, void *client_data)
{
	stream_decoder_client_data_struct *dcd = (stream_decoder_client_data_struct*)client_data;

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

static FLAC__bool stream_decoder_test_respond_(OggFLAC__StreamDecoder *decoder, stream_decoder_client_data_struct *dcd)
{
	if(!OggFLAC__stream_decoder_set_read_callback(decoder, stream_decoder_read_callback_))
		return die_s_("at OggFLAC__stream_decoder_set_read_callback(), returned false", decoder);

	if(!OggFLAC__stream_decoder_set_write_callback(decoder, stream_decoder_write_callback_))
		return die_s_("at OggFLAC__stream_decoder_set_write_callback(), returned false", decoder);

	if(!OggFLAC__stream_decoder_set_metadata_callback(decoder, stream_decoder_metadata_callback_))
		return die_s_("at OggFLAC__stream_decoder_set_metadata_callback(), returned false", decoder);

	if(!OggFLAC__stream_decoder_set_error_callback(decoder, stream_decoder_error_callback_))
		return die_s_("at OggFLAC__stream_decoder_set_error_callback(), returned false", decoder);

	if(!OggFLAC__stream_decoder_set_client_data(decoder, dcd))
		return die_s_("at OggFLAC__stream_decoder_set_client_data(), returned false", decoder);

	printf("testing OggFLAC__stream_decoder_init()... ");
	if(OggFLAC__stream_decoder_init(decoder) != OggFLAC__STREAM_DECODER_OK)
		return die_s_(0, decoder);
	printf("OK\n");

	dcd->current_metadata_number = 0;

	if(fseek(dcd->file, 0, SEEK_SET) < 0) {
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

static FLAC__bool test_stream_decoder()
{
	OggFLAC__StreamDecoder *decoder;
	OggFLAC__StreamDecoderState state;
	FLAC__StreamDecoderState fstate;
	stream_decoder_client_data_struct decoder_client_data;

	printf("\n+++ libOggFLAC unit test: OggFLAC__StreamDecoder\n\n");

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

	printf("testing OggFLAC__stream_decoder_init()... ");
	if(OggFLAC__stream_decoder_init(decoder) == OggFLAC__STREAM_DECODER_OK)
		return die_s_(0, decoder);
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

	printf("testing OggFLAC__stream_decoder_set_read_callback()... ");
	if(!OggFLAC__stream_decoder_set_read_callback(decoder, stream_decoder_read_callback_))
		return die_s_("returned false", decoder);
	printf("OK\n");

	printf("testing OggFLAC__stream_decoder_set_write_callback()... ");
	if(!OggFLAC__stream_decoder_set_write_callback(decoder, stream_decoder_write_callback_))
		return die_s_("returned false", decoder);
	printf("OK\n");

	printf("testing OggFLAC__stream_decoder_set_metadata_callback()... ");
	if(!OggFLAC__stream_decoder_set_metadata_callback(decoder, stream_decoder_metadata_callback_))
		return die_s_("returned false", decoder);
	printf("OK\n");

	printf("testing OggFLAC__stream_decoder_set_error_callback()... ");
	if(!OggFLAC__stream_decoder_set_error_callback(decoder, stream_decoder_error_callback_))
		return die_s_("returned false", decoder);
	printf("OK\n");

	printf("testing OggFLAC__stream_decoder_set_client_data()... ");
	if(!OggFLAC__stream_decoder_set_client_data(decoder, &decoder_client_data))
		return die_s_("returned false", decoder);
	printf("OK\n");

	printf("testing OggFLAC__stream_decoder_init()... ");
	if(OggFLAC__stream_decoder_init(decoder) != OggFLAC__STREAM_DECODER_OK)
		return die_s_(0, decoder);
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

	printf("opening Ogg FLAC file... ");
	decoder_client_data.file = fopen(oggflacfilename_, "rb");
	if(0 == decoder_client_data.file) {
		printf("ERROR\n");
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

	printf("testing OggFLAC__stream_decoder_process_until_end_of_stream()... ");
	if(!OggFLAC__stream_decoder_process_until_end_of_stream(decoder))
		return die_s_("returned false", decoder);
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
		/* value could be anything since we're at the last block, so accept any answer */
		printf("returned %u... OK\n", blocksize);
	}

	printf("testing OggFLAC__stream_decoder_get_channel_assignment()... ");
	{
		FLAC__ChannelAssignment ca = OggFLAC__stream_decoder_get_channel_assignment(decoder);
		printf("returned %u (%s)... OK\n", (unsigned)ca, FLAC__ChannelAssignmentString[ca]);
	}

	printf("testing OggFLAC__stream_decoder_reset()... ");
	if(!OggFLAC__stream_decoder_reset(decoder)) {
		state = OggFLAC__stream_decoder_get_state(decoder);
		printf("FAILED, returned false, state = %u (%s)\n", state, FLAC__StreamDecoderStateString[state]);
		return false;
	}
	printf("OK\n");

	decoder_client_data.current_metadata_number = 0;

	printf("rewinding input... ");
	if(fseek(decoder_client_data.file, 0, SEEK_SET) < 0) {
		printf("FAILED, errno = %d\n", errno);
		return false;
	}
	printf("OK\n");

	printf("testing OggFLAC__stream_decoder_process_until_end_of_stream()... ");
	if(!OggFLAC__stream_decoder_process_until_end_of_stream(decoder))
		return die_s_("returned false", decoder);
	printf("OK\n");

	printf("testing OggFLAC__stream_decoder_finish()... ");
	OggFLAC__stream_decoder_finish(decoder);
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

	/* done, now leave the sequence the way we found it... */
	num_expected_ = 0;
	expected_metadata_sequence_[num_expected_++] = &streaminfo_;
	expected_metadata_sequence_[num_expected_++] = &vorbiscomment_;
	expected_metadata_sequence_[num_expected_++] = &padding_;
	expected_metadata_sequence_[num_expected_++] = &seektable_;
	expected_metadata_sequence_[num_expected_++] = &application1_;
	expected_metadata_sequence_[num_expected_++] = &application2_;
	expected_metadata_sequence_[num_expected_++] = &cuesheet_;
	expected_metadata_sequence_[num_expected_++] = &unknown_;

	printf("testing OggFLAC__stream_decoder_delete()... ");
	OggFLAC__stream_decoder_delete(decoder);
	printf("OK\n");

	fclose(decoder_client_data.file);

	printf("\nPASSED!\n");

	return true;
}

static OggFLAC__SeekableStreamDecoderReadStatus seekable_stream_decoder_read_callback_(const OggFLAC__SeekableStreamDecoder *decoder, FLAC__byte buffer[], unsigned *bytes, void *client_data)
{
	(void)decoder;
	switch(stream_decoder_read_callback_(0, buffer, bytes, client_data)) {
		case FLAC__STREAM_DECODER_READ_STATUS_CONTINUE:
		case FLAC__STREAM_DECODER_READ_STATUS_END_OF_STREAM:
			return OggFLAC__SEEKABLE_STREAM_DECODER_READ_STATUS_OK;
		case FLAC__STREAM_DECODER_READ_STATUS_ABORT:
			return OggFLAC__SEEKABLE_STREAM_DECODER_READ_STATUS_ERROR;
		default:
			FLAC__ASSERT(0);
			return OggFLAC__SEEKABLE_STREAM_DECODER_READ_STATUS_ERROR;
	}
}

static OggFLAC__SeekableStreamDecoderSeekStatus seekable_stream_decoder_seek_callback_(const OggFLAC__SeekableStreamDecoder *decoder, FLAC__uint64 absolute_byte_offset, void *client_data)
{
	seekable_stream_decoder_client_data_struct *dcd = (seekable_stream_decoder_client_data_struct*)client_data;

	(void)decoder;

	if(0 == dcd) {
		printf("ERROR: client_data in seek callback is NULL\n");
		return OggFLAC__SEEKABLE_STREAM_DECODER_SEEK_STATUS_ERROR;
	}

	if(dcd->error_occurred)
		return OggFLAC__SEEKABLE_STREAM_DECODER_SEEK_STATUS_ERROR;

	if(fseek(dcd->file, (long)absolute_byte_offset, SEEK_SET) < 0) {
		dcd->error_occurred = true;
		return OggFLAC__SEEKABLE_STREAM_DECODER_SEEK_STATUS_ERROR;
	}

	return OggFLAC__SEEKABLE_STREAM_DECODER_SEEK_STATUS_OK;
}

static OggFLAC__SeekableStreamDecoderTellStatus seekable_stream_decoder_tell_callback_(const OggFLAC__SeekableStreamDecoder *decoder, FLAC__uint64 *absolute_byte_offset, void *client_data)
{
	seekable_stream_decoder_client_data_struct *dcd = (seekable_stream_decoder_client_data_struct*)client_data;
	long offset;

	(void)decoder;

	if(0 == dcd) {
		printf("ERROR: client_data in tell callback is NULL\n");
		return OggFLAC__SEEKABLE_STREAM_DECODER_TELL_STATUS_ERROR;
	}

	if(dcd->error_occurred)
		return OggFLAC__SEEKABLE_STREAM_DECODER_TELL_STATUS_ERROR;

	offset = ftell(dcd->file);
	*absolute_byte_offset = (FLAC__uint64)offset;

	if(offset < 0) {
		dcd->error_occurred = true;
		return OggFLAC__SEEKABLE_STREAM_DECODER_TELL_STATUS_ERROR;
	}

	return OggFLAC__SEEKABLE_STREAM_DECODER_TELL_STATUS_OK;
}

static OggFLAC__SeekableStreamDecoderLengthStatus seekable_stream_decoder_length_callback_(const OggFLAC__SeekableStreamDecoder *decoder, FLAC__uint64 *stream_length, void *client_data)
{
	seekable_stream_decoder_client_data_struct *dcd = (seekable_stream_decoder_client_data_struct*)client_data;

	(void)decoder;

	if(0 == dcd) {
		printf("ERROR: client_data in length callback is NULL\n");
		return OggFLAC__SEEKABLE_STREAM_DECODER_LENGTH_STATUS_ERROR;
	}

	if(dcd->error_occurred)
		return OggFLAC__SEEKABLE_STREAM_DECODER_LENGTH_STATUS_ERROR;

	*stream_length = (FLAC__uint64)oggflacfilesize_;
	return OggFLAC__SEEKABLE_STREAM_DECODER_LENGTH_STATUS_OK;
}

static FLAC__bool seekable_stream_decoder_eof_callback_(const OggFLAC__SeekableStreamDecoder *decoder, void *client_data)
{
	seekable_stream_decoder_client_data_struct *dcd = (seekable_stream_decoder_client_data_struct*)client_data;

	(void)decoder;

	if(0 == dcd) {
		printf("ERROR: client_data in eof callback is NULL\n");
		return true;
	}

	if(dcd->error_occurred)
		return true;

	return feof(dcd->file);
}

static FLAC__StreamDecoderWriteStatus seekable_stream_decoder_write_callback_(const OggFLAC__SeekableStreamDecoder *decoder, const FLAC__Frame *frame, const FLAC__int32 * const buffer[], void *client_data)
{
	(void)decoder;
	return stream_decoder_write_callback_(0, frame, buffer, client_data);
}

static void seekable_stream_decoder_metadata_callback_(const OggFLAC__SeekableStreamDecoder *decoder, const FLAC__StreamMetadata *metadata, void *client_data)
{
	(void)decoder;
	stream_decoder_metadata_callback_(0, metadata, client_data);
}

static void seekable_stream_decoder_error_callback_(const OggFLAC__SeekableStreamDecoder *decoder, FLAC__StreamDecoderErrorStatus status, void *client_data)
{
	(void)decoder;
	stream_decoder_error_callback_(0, status, client_data);
}

static FLAC__bool seekable_stream_decoder_test_respond_(OggFLAC__SeekableStreamDecoder *decoder, seekable_stream_decoder_client_data_struct *dcd)
{
	if(!OggFLAC__seekable_stream_decoder_set_read_callback(decoder, seekable_stream_decoder_read_callback_))
		return die_ss_("at OggFLAC__seekable_stream_decoder_set_read_callback(), returned false", decoder);

	if(!OggFLAC__seekable_stream_decoder_set_seek_callback(decoder, seekable_stream_decoder_seek_callback_))
		return die_ss_("at OggFLAC__seekable_stream_decoder_set_seek_callback(), returned false", decoder);

	if(!OggFLAC__seekable_stream_decoder_set_tell_callback(decoder, seekable_stream_decoder_tell_callback_))
		return die_ss_("at OggFLAC__seekable_stream_decoder_set_tell_callback(), returned false", decoder);

	if(!OggFLAC__seekable_stream_decoder_set_length_callback(decoder, seekable_stream_decoder_length_callback_))
		return die_ss_("at OggFLAC__seekable_stream_decoder_set_length_callback(), returned false", decoder);

	if(!OggFLAC__seekable_stream_decoder_set_eof_callback(decoder, seekable_stream_decoder_eof_callback_))
		return die_ss_("at OggFLAC__seekable_stream_decoder_set_eof_callback(), returned false", decoder);

	if(!OggFLAC__seekable_stream_decoder_set_write_callback(decoder, seekable_stream_decoder_write_callback_))
		return die_ss_("at OggFLAC__seekable_stream_decoder_set_write_callback(), returned false", decoder);

	if(!OggFLAC__seekable_stream_decoder_set_metadata_callback(decoder, seekable_stream_decoder_metadata_callback_))
		return die_ss_("at OggFLAC__seekable_stream_decoder_set_metadata_callback(), returned false", decoder);

	if(!OggFLAC__seekable_stream_decoder_set_error_callback(decoder, seekable_stream_decoder_error_callback_))
		return die_ss_("at OggFLAC__seekable_stream_decoder_set_error_callback(), returned false", decoder);

	if(!OggFLAC__seekable_stream_decoder_set_client_data(decoder, dcd))
		return die_ss_("at OggFLAC__seekable_stream_decoder_set_client_data(), returned false", decoder);

	if(!OggFLAC__seekable_stream_decoder_set_md5_checking(decoder, true))
		return die_ss_("at OggFLAC__seekable_stream_decoder_set_md5_checking(), returned false", decoder);

	printf("testing OggFLAC__seekable_stream_decoder_init()... ");
	if(OggFLAC__seekable_stream_decoder_init(decoder) != OggFLAC__SEEKABLE_STREAM_DECODER_OK)
		return die_ss_(0, decoder);
	printf("OK\n");

	dcd->current_metadata_number = 0;

	if(fseek(dcd->file, 0, SEEK_SET) < 0) {
		printf("FAILED rewinding input, errno = %d\n", errno);
		return false;
	}

	printf("testing OggFLAC__seekable_stream_decoder_process_until_end_of_stream()... ");
	if(!OggFLAC__seekable_stream_decoder_process_until_end_of_stream(decoder))
		return die_ss_("returned false", decoder);
	printf("OK\n");

	printf("testing OggFLAC__seekable_stream_decoder_finish()... ");
	(void) OggFLAC__seekable_stream_decoder_finish(decoder);
	printf("OK\n");

	return true;
}

static FLAC__bool test_seekable_stream_decoder()
{
	OggFLAC__SeekableStreamDecoder *decoder;
	OggFLAC__SeekableStreamDecoderState state;
	OggFLAC__StreamDecoderState sstate;
	FLAC__StreamDecoderState fstate;
	seekable_stream_decoder_client_data_struct decoder_client_data;

	printf("\n+++ libOggFLAC unit test: OggFLAC__SeekableStreamDecoder\n\n");

	printf("testing OggFLAC__seekable_stream_decoder_new()... ");
	decoder = OggFLAC__seekable_stream_decoder_new();
	if(0 == decoder) {
		printf("FAILED, returned NULL\n");
		return false;
	}
	printf("OK\n");

	printf("testing OggFLAC__seekable_stream_decoder_delete()... ");
	OggFLAC__seekable_stream_decoder_delete(decoder);
	printf("OK\n");

	printf("testing OggFLAC__seekable_stream_decoder_new()... ");
	decoder = OggFLAC__seekable_stream_decoder_new();
	if(0 == decoder) {
		printf("FAILED, returned NULL\n");
		return false;
	}
	printf("OK\n");

	printf("testing OggFLAC__seekable_stream_decoder_init()... ");
	if(OggFLAC__seekable_stream_decoder_init(decoder) == OggFLAC__SEEKABLE_STREAM_DECODER_OK)
		return die_ss_(0, decoder);
	printf("OK\n");

	printf("testing OggFLAC__seekable_stream_decoder_delete()... ");
	OggFLAC__seekable_stream_decoder_delete(decoder);
	printf("OK\n");

	num_expected_ = 0;
	expected_metadata_sequence_[num_expected_++] = &streaminfo_;

	printf("testing OggFLAC__seekable_stream_decoder_new()... ");
	decoder = OggFLAC__seekable_stream_decoder_new();
	if(0 == decoder) {
		printf("FAILED, returned NULL\n");
		return false;
	}
	printf("OK\n");

	printf("testing OggFLAC__seekable_stream_decoder_set_serial_number()... ");
	if(!OggFLAC__seekable_stream_decoder_set_serial_number(decoder, file_utils__serial_number))
		return die_ss_("returned false", decoder);
	printf("OK\n");

	printf("testing OggFLAC__seekable_stream_decoder_set_read_callback()... ");
	if(!OggFLAC__seekable_stream_decoder_set_read_callback(decoder, seekable_stream_decoder_read_callback_))
		return die_ss_("returned false", decoder);
	printf("OK\n");

	printf("testing OggFLAC__seekable_stream_decoder_set_seek_callback()... ");
	if(!OggFLAC__seekable_stream_decoder_set_seek_callback(decoder, seekable_stream_decoder_seek_callback_))
		return die_ss_("returned false", decoder);
	printf("OK\n");

	printf("testing OggFLAC__seekable_stream_decoder_set_tell_callback()... ");
	if(!OggFLAC__seekable_stream_decoder_set_tell_callback(decoder, seekable_stream_decoder_tell_callback_))
		return die_ss_("returned false", decoder);
	printf("OK\n");

	printf("testing OggFLAC__seekable_stream_decoder_set_length_callback()... ");
	if(!OggFLAC__seekable_stream_decoder_set_length_callback(decoder, seekable_stream_decoder_length_callback_))
		return die_ss_("returned false", decoder);
	printf("OK\n");

	printf("testing OggFLAC__seekable_stream_decoder_set_eof_callback()... ");
	if(!OggFLAC__seekable_stream_decoder_set_eof_callback(decoder, seekable_stream_decoder_eof_callback_))
		return die_ss_("returned false", decoder);
	printf("OK\n");

	printf("testing OggFLAC__seekable_stream_decoder_set_write_callback()... ");
	if(!OggFLAC__seekable_stream_decoder_set_write_callback(decoder, seekable_stream_decoder_write_callback_))
		return die_ss_("returned false", decoder);
	printf("OK\n");

	printf("testing OggFLAC__seekable_stream_decoder_set_metadata_callback()... ");
	if(!OggFLAC__seekable_stream_decoder_set_metadata_callback(decoder, seekable_stream_decoder_metadata_callback_))
		return die_ss_("returned false", decoder);
	printf("OK\n");

	printf("testing OggFLAC__seekable_stream_decoder_set_error_callback()... ");
	if(!OggFLAC__seekable_stream_decoder_set_error_callback(decoder, seekable_stream_decoder_error_callback_))
		return die_ss_("returned false", decoder);
	printf("OK\n");

	printf("testing OggFLAC__seekable_stream_decoder_set_client_data()... ");
	if(!OggFLAC__seekable_stream_decoder_set_client_data(decoder, &decoder_client_data))
		return die_ss_("returned false", decoder);
	printf("OK\n");

	printf("testing OggFLAC__seekable_stream_decoder_set_md5_checking()... ");
	if(!OggFLAC__seekable_stream_decoder_set_md5_checking(decoder, true))
		return die_ss_("returned false", decoder);
	printf("OK\n");

	printf("testing OggFLAC__seekable_stream_decoder_init()... ");
	if(OggFLAC__seekable_stream_decoder_init(decoder) != OggFLAC__SEEKABLE_STREAM_DECODER_OK)
		return die_ss_(0, decoder);
	printf("OK\n");

	printf("testing OggFLAC__seekable_stream_decoder_get_state()... ");
	state = OggFLAC__seekable_stream_decoder_get_state(decoder);
	printf("returned state = %u (%s)... OK\n", state, OggFLAC__SeekableStreamDecoderStateString[state]);

	printf("testing OggFLAC__seekable_stream_decoder_get_stream_decoder_state()... ");
	sstate = OggFLAC__seekable_stream_decoder_get_stream_decoder_state(decoder);
	printf("returned state = %u (%s)... OK\n", sstate, OggFLAC__StreamDecoderStateString[sstate]);

	printf("testing OggFLAC__seekable_stream_decoder_get_FLAC_stream_decoder_state()... ");
	fstate = OggFLAC__seekable_stream_decoder_get_FLAC_stream_decoder_state(decoder);
	printf("returned state = %u (%s)... OK\n", fstate, FLAC__StreamDecoderStateString[fstate]);

	decoder_client_data.current_metadata_number = 0;
	decoder_client_data.ignore_errors = false;
	decoder_client_data.error_occurred = false;

	printf("opening Ogg FLAC file... ");
	decoder_client_data.file = fopen(oggflacfilename_, "rb");
	if(0 == decoder_client_data.file) {
		printf("ERROR\n");
		return false;
	}
	printf("OK\n");

	printf("testing OggFLAC__seekable_stream_decoder_get_md5_checking()... ");
	if(!OggFLAC__seekable_stream_decoder_get_md5_checking(decoder)) {
		printf("FAILED, returned false, expected true\n");
		return false;
	}
	printf("OK\n");

	printf("testing OggFLAC__seekable_stream_decoder_process_until_end_of_metadata()... ");
	if(!OggFLAC__seekable_stream_decoder_process_until_end_of_metadata(decoder))
		return die_ss_("returned false", decoder);
	printf("OK\n");

	printf("testing OggFLAC__seekable_stream_decoder_process_single()... ");
	if(!OggFLAC__seekable_stream_decoder_process_single(decoder))
		return die_ss_("returned false", decoder);
	printf("OK\n");

	printf("testing OggFLAC__seekable_stream_decoder_flush()... ");
	if(!OggFLAC__seekable_stream_decoder_flush(decoder))
		return die_ss_("returned false", decoder);
	printf("OK\n");

	decoder_client_data.ignore_errors = true;
	printf("testing OggFLAC__seekable_stream_decoder_process_single()... ");
	if(!OggFLAC__seekable_stream_decoder_process_single(decoder))
		return die_ss_("returned false", decoder);
	printf("OK\n");
	decoder_client_data.ignore_errors = false;

	printf("testing OggFLAC__seekable_stream_decoder_seek_absolute()... ");
	if(!OggFLAC__seekable_stream_decoder_seek_absolute(decoder, 0))
		return die_ss_("returned false", decoder);
	printf("OK\n");

	printf("testing OggFLAC__seekable_stream_decoder_process_until_end_of_stream()... ");
	if(!OggFLAC__seekable_stream_decoder_process_until_end_of_stream(decoder))
		return die_ss_("returned false", decoder);
	printf("OK\n");

	printf("testing OggFLAC__seekable_stream_decoder_get_channels()... ");
	{
		unsigned channels = OggFLAC__seekable_stream_decoder_get_channels(decoder);
		if(channels != streaminfo_.data.stream_info.channels) {
			printf("FAILED, returned %u, expected %u\n", channels, streaminfo_.data.stream_info.channels);
			return false;
		}
	}
	printf("OK\n");

	printf("testing OggFLAC__seekable_stream_decoder_get_bits_per_sample()... ");
	{
		unsigned bits_per_sample = OggFLAC__seekable_stream_decoder_get_bits_per_sample(decoder);
		if(bits_per_sample != streaminfo_.data.stream_info.bits_per_sample) {
			printf("FAILED, returned %u, expected %u\n", bits_per_sample, streaminfo_.data.stream_info.bits_per_sample);
			return false;
		}
	}
	printf("OK\n");

	printf("testing OggFLAC__seekable_stream_decoder_get_sample_rate()... ");
	{
		unsigned sample_rate = OggFLAC__seekable_stream_decoder_get_sample_rate(decoder);
		if(sample_rate != streaminfo_.data.stream_info.sample_rate) {
			printf("FAILED, returned %u, expected %u\n", sample_rate, streaminfo_.data.stream_info.sample_rate);
			return false;
		}
	}
	printf("OK\n");

	printf("testing OggFLAC__seekable_stream_decoder_get_blocksize()... ");
	{
		unsigned blocksize = OggFLAC__seekable_stream_decoder_get_blocksize(decoder);
		/* value could be anything since we're at the last block, so accept any answer */
		printf("returned %u... OK\n", blocksize);
	}

	printf("testing OggFLAC__seekable_stream_decoder_get_channel_assignment()... ");
	{
		FLAC__ChannelAssignment ca = OggFLAC__seekable_stream_decoder_get_channel_assignment(decoder);
		printf("returned %u (%s)... OK\n", (unsigned)ca, FLAC__ChannelAssignmentString[ca]);
	}

	printf("testing OggFLAC__seekable_stream_decoder_reset()... ");
	if(!OggFLAC__seekable_stream_decoder_reset(decoder))
		return die_ss_("returned false", decoder);
	printf("OK\n");

	decoder_client_data.current_metadata_number = 0;

	printf("rewinding input... ");
	if(fseek(decoder_client_data.file, 0, SEEK_SET) < 0) {
		printf("FAILED, errno = %d\n", errno);
		return false;
	}
	printf("OK\n");

	printf("testing OggFLAC__seekable_stream_decoder_process_until_end_of_stream()... ");
	if(!OggFLAC__seekable_stream_decoder_process_until_end_of_stream(decoder))
		return die_ss_("returned false", decoder);
	printf("OK\n");

	printf("testing OggFLAC__seekable_stream_decoder_finish()... ");
	(void) OggFLAC__seekable_stream_decoder_finish(decoder);
	printf("OK\n");

	/*
	 * respond all
	 */

	printf("testing OggFLAC__seekable_stream_decoder_set_metadata_respond_all()... ");
	if(!OggFLAC__seekable_stream_decoder_set_metadata_respond_all(decoder))
		return die_ss_("returned false", decoder);
	printf("OK\n");

	num_expected_ = 0;
	expected_metadata_sequence_[num_expected_++] = &streaminfo_;
	expected_metadata_sequence_[num_expected_++] = &vorbiscomment_;
	expected_metadata_sequence_[num_expected_++] = &padding_;
	expected_metadata_sequence_[num_expected_++] = &seektable_;
	expected_metadata_sequence_[num_expected_++] = &application1_;
	expected_metadata_sequence_[num_expected_++] = &application2_;
	expected_metadata_sequence_[num_expected_++] = &cuesheet_;
	expected_metadata_sequence_[num_expected_++] = &unknown_;

	if(!seekable_stream_decoder_test_respond_(decoder, &decoder_client_data))
		return false;

	/*
	 * ignore all
	 */

	printf("testing OggFLAC__seekable_stream_decoder_set_metadata_ignore_all()... ");
	if(!OggFLAC__seekable_stream_decoder_set_metadata_ignore_all(decoder))
		return die_ss_("returned false", decoder);
	printf("OK\n");

	num_expected_ = 0;

	if(!seekable_stream_decoder_test_respond_(decoder, &decoder_client_data))
		return false;

	/*
	 * respond all, ignore VORBIS_COMMENT
	 */

	printf("testing OggFLAC__seekable_stream_decoder_set_metadata_respond_all()... ");
	if(!OggFLAC__seekable_stream_decoder_set_metadata_respond_all(decoder))
		return die_ss_("returned false", decoder);
	printf("OK\n");

	printf("testing OggFLAC__seekable_stream_decoder_set_metadata_ignore(VORBIS_COMMENT)... ");
	if(!OggFLAC__seekable_stream_decoder_set_metadata_ignore(decoder, FLAC__METADATA_TYPE_VORBIS_COMMENT))
		return die_ss_("returned false", decoder);
	printf("OK\n");

	num_expected_ = 0;
	expected_metadata_sequence_[num_expected_++] = &streaminfo_;
	expected_metadata_sequence_[num_expected_++] = &padding_;
	expected_metadata_sequence_[num_expected_++] = &seektable_;
	expected_metadata_sequence_[num_expected_++] = &application1_;
	expected_metadata_sequence_[num_expected_++] = &application2_;
	expected_metadata_sequence_[num_expected_++] = &cuesheet_;
	expected_metadata_sequence_[num_expected_++] = &unknown_;

	if(!seekable_stream_decoder_test_respond_(decoder, &decoder_client_data))
		return false;

	/*
	 * respond all, ignore APPLICATION
	 */

	printf("testing OggFLAC__seekable_stream_decoder_set_metadata_respond_all()... ");
	if(!OggFLAC__seekable_stream_decoder_set_metadata_respond_all(decoder))
		return die_ss_("returned false", decoder);
	printf("OK\n");

	printf("testing OggFLAC__seekable_stream_decoder_set_metadata_ignore(APPLICATION)... ");
	if(!OggFLAC__seekable_stream_decoder_set_metadata_ignore(decoder, FLAC__METADATA_TYPE_APPLICATION))
		return die_ss_("returned false", decoder);
	printf("OK\n");

	num_expected_ = 0;
	expected_metadata_sequence_[num_expected_++] = &streaminfo_;
	expected_metadata_sequence_[num_expected_++] = &vorbiscomment_;
	expected_metadata_sequence_[num_expected_++] = &padding_;
	expected_metadata_sequence_[num_expected_++] = &seektable_;
	expected_metadata_sequence_[num_expected_++] = &cuesheet_;
	expected_metadata_sequence_[num_expected_++] = &unknown_;

	if(!seekable_stream_decoder_test_respond_(decoder, &decoder_client_data))
		return false;

	/*
	 * respond all, ignore APPLICATION id of app#1
	 */

	printf("testing OggFLAC__seekable_stream_decoder_set_metadata_respond_all()... ");
	if(!OggFLAC__seekable_stream_decoder_set_metadata_respond_all(decoder))
		return die_ss_("returned false", decoder);
	printf("OK\n");

	printf("testing OggFLAC__seekable_stream_decoder_set_metadata_ignore_application(of app block #1)... ");
	if(!OggFLAC__seekable_stream_decoder_set_metadata_ignore_application(decoder, application1_.data.application.id))
		return die_ss_("returned false", decoder);
	printf("OK\n");

	num_expected_ = 0;
	expected_metadata_sequence_[num_expected_++] = &streaminfo_;
	expected_metadata_sequence_[num_expected_++] = &vorbiscomment_;
	expected_metadata_sequence_[num_expected_++] = &padding_;
	expected_metadata_sequence_[num_expected_++] = &seektable_;
	expected_metadata_sequence_[num_expected_++] = &application2_;
	expected_metadata_sequence_[num_expected_++] = &cuesheet_;
	expected_metadata_sequence_[num_expected_++] = &unknown_;

	if(!seekable_stream_decoder_test_respond_(decoder, &decoder_client_data))
		return false;

	/*
	 * respond all, ignore APPLICATION id of app#1 & app#2
	 */

	printf("testing OggFLAC__seekable_stream_decoder_set_metadata_respond_all()... ");
	if(!OggFLAC__seekable_stream_decoder_set_metadata_respond_all(decoder))
		return die_ss_("returned false", decoder);
	printf("OK\n");

	printf("testing OggFLAC__seekable_stream_decoder_set_metadata_ignore_application(of app block #1)... ");
	if(!OggFLAC__seekable_stream_decoder_set_metadata_ignore_application(decoder, application1_.data.application.id))
		return die_ss_("returned false", decoder);
	printf("OK\n");

	printf("testing OggFLAC__seekable_stream_decoder_set_metadata_ignore_application(of app block #2)... ");
	if(!OggFLAC__seekable_stream_decoder_set_metadata_ignore_application(decoder, application2_.data.application.id))
		return die_ss_("returned false", decoder);
	printf("OK\n");

	num_expected_ = 0;
	expected_metadata_sequence_[num_expected_++] = &streaminfo_;
	expected_metadata_sequence_[num_expected_++] = &vorbiscomment_;
	expected_metadata_sequence_[num_expected_++] = &padding_;
	expected_metadata_sequence_[num_expected_++] = &seektable_;
	expected_metadata_sequence_[num_expected_++] = &cuesheet_;
	expected_metadata_sequence_[num_expected_++] = &unknown_;

	if(!seekable_stream_decoder_test_respond_(decoder, &decoder_client_data))
		return false;

	/*
	 * ignore all, respond VORBIS_COMMENT
	 */

	printf("testing OggFLAC__seekable_stream_decoder_set_metadata_ignore_all()... ");
	if(!OggFLAC__seekable_stream_decoder_set_metadata_ignore_all(decoder))
		return die_ss_("returned false", decoder);
	printf("OK\n");

	printf("testing OggFLAC__seekable_stream_decoder_set_metadata_respond(VORBIS_COMMENT)... ");
	if(!OggFLAC__seekable_stream_decoder_set_metadata_respond(decoder, FLAC__METADATA_TYPE_VORBIS_COMMENT))
		return die_ss_("returned false", decoder);
	printf("OK\n");

	num_expected_ = 0;
	expected_metadata_sequence_[num_expected_++] = &vorbiscomment_;

	if(!seekable_stream_decoder_test_respond_(decoder, &decoder_client_data))
		return false;

	/*
	 * ignore all, respond APPLICATION
	 */

	printf("testing OggFLAC__seekable_stream_decoder_set_metadata_ignore_all()... ");
	if(!OggFLAC__seekable_stream_decoder_set_metadata_ignore_all(decoder))
		return die_ss_("returned false", decoder);
	printf("OK\n");

	printf("testing OggFLAC__seekable_stream_decoder_set_metadata_respond(APPLICATION)... ");
	if(!OggFLAC__seekable_stream_decoder_set_metadata_respond(decoder, FLAC__METADATA_TYPE_APPLICATION))
		return die_ss_("returned false", decoder);
	printf("OK\n");

	num_expected_ = 0;
	expected_metadata_sequence_[num_expected_++] = &application1_;
	expected_metadata_sequence_[num_expected_++] = &application2_;

	if(!seekable_stream_decoder_test_respond_(decoder, &decoder_client_data))
		return false;

	/*
	 * ignore all, respond APPLICATION id of app#1
	 */

	printf("testing OggFLAC__seekable_stream_decoder_set_metadata_ignore_all()... ");
	if(!OggFLAC__seekable_stream_decoder_set_metadata_ignore_all(decoder))
		return die_ss_("returned false", decoder);
	printf("OK\n");

	printf("testing OggFLAC__seekable_stream_decoder_set_metadata_respond_application(of app block #1)... ");
	if(!OggFLAC__seekable_stream_decoder_set_metadata_respond_application(decoder, application1_.data.application.id))
		return die_ss_("returned false", decoder);
	printf("OK\n");

	num_expected_ = 0;
	expected_metadata_sequence_[num_expected_++] = &application1_;

	if(!seekable_stream_decoder_test_respond_(decoder, &decoder_client_data))
		return false;

	/*
	 * ignore all, respond APPLICATION id of app#1 & app#2
	 */

	printf("testing OggFLAC__seekable_stream_decoder_set_metadata_ignore_all()... ");
	if(!OggFLAC__seekable_stream_decoder_set_metadata_ignore_all(decoder))
		return die_ss_("returned false", decoder);
	printf("OK\n");

	printf("testing OggFLAC__seekable_stream_decoder_set_metadata_respond_application(of app block #1)... ");
	if(!OggFLAC__seekable_stream_decoder_set_metadata_respond_application(decoder, application1_.data.application.id))
		return die_ss_("returned false", decoder);
	printf("OK\n");

	printf("testing OggFLAC__seekable_stream_decoder_set_metadata_respond_application(of app block #2)... ");
	if(!OggFLAC__seekable_stream_decoder_set_metadata_respond_application(decoder, application2_.data.application.id))
		return die_ss_("returned false", decoder);
	printf("OK\n");

	num_expected_ = 0;
	expected_metadata_sequence_[num_expected_++] = &application1_;
	expected_metadata_sequence_[num_expected_++] = &application2_;

	if(!seekable_stream_decoder_test_respond_(decoder, &decoder_client_data))
		return false;

	/*
	 * respond all, ignore APPLICATION, respond APPLICATION id of app#1
	 */

	printf("testing OggFLAC__seekable_stream_decoder_set_metadata_respond_all()... ");
	if(!OggFLAC__seekable_stream_decoder_set_metadata_respond_all(decoder))
		return die_ss_("returned false", decoder);
	printf("OK\n");

	printf("testing OggFLAC__seekable_stream_decoder_set_metadata_ignore(APPLICATION)... ");
	if(!OggFLAC__seekable_stream_decoder_set_metadata_ignore(decoder, FLAC__METADATA_TYPE_APPLICATION))
		return die_ss_("returned false", decoder);
	printf("OK\n");

	printf("testing OggFLAC__seekable_stream_decoder_set_metadata_respond_application(of app block #1)... ");
	if(!OggFLAC__seekable_stream_decoder_set_metadata_respond_application(decoder, application1_.data.application.id))
		return die_ss_("returned false", decoder);
	printf("OK\n");

	num_expected_ = 0;
	expected_metadata_sequence_[num_expected_++] = &streaminfo_;
	expected_metadata_sequence_[num_expected_++] = &vorbiscomment_;
	expected_metadata_sequence_[num_expected_++] = &padding_;
	expected_metadata_sequence_[num_expected_++] = &seektable_;
	expected_metadata_sequence_[num_expected_++] = &application1_;
	expected_metadata_sequence_[num_expected_++] = &cuesheet_;
	expected_metadata_sequence_[num_expected_++] = &unknown_;

	if(!seekable_stream_decoder_test_respond_(decoder, &decoder_client_data))
		return false;

	/*
	 * ignore all, respond APPLICATION, ignore APPLICATION id of app#1
	 */

	printf("testing OggFLAC__seekable_stream_decoder_set_metadata_ignore_all()... ");
	if(!OggFLAC__seekable_stream_decoder_set_metadata_ignore_all(decoder))
		return die_ss_("returned false", decoder);
	printf("OK\n");

	printf("testing OggFLAC__seekable_stream_decoder_set_metadata_respond(APPLICATION)... ");
	if(!OggFLAC__seekable_stream_decoder_set_metadata_respond(decoder, FLAC__METADATA_TYPE_APPLICATION))
		return die_ss_("returned false", decoder);
	printf("OK\n");

	printf("testing OggFLAC__seekable_stream_decoder_set_metadata_ignore_application(of app block #1)... ");
	if(!OggFLAC__seekable_stream_decoder_set_metadata_ignore_application(decoder, application1_.data.application.id))
		return die_ss_("returned false", decoder);
	printf("OK\n");

	num_expected_ = 0;
	expected_metadata_sequence_[num_expected_++] = &application2_;

	if(!seekable_stream_decoder_test_respond_(decoder, &decoder_client_data))
		return false;

	/* done, now leave the sequence the way we found it... */
	num_expected_ = 0;
	expected_metadata_sequence_[num_expected_++] = &streaminfo_;
	expected_metadata_sequence_[num_expected_++] = &vorbiscomment_;
	expected_metadata_sequence_[num_expected_++] = &padding_;
	expected_metadata_sequence_[num_expected_++] = &seektable_;
	expected_metadata_sequence_[num_expected_++] = &application1_;
	expected_metadata_sequence_[num_expected_++] = &application2_;
	expected_metadata_sequence_[num_expected_++] = &cuesheet_;
	expected_metadata_sequence_[num_expected_++] = &unknown_;

	printf("testing OggFLAC__seekable_stream_decoder_delete()... ");
	OggFLAC__seekable_stream_decoder_delete(decoder);
	printf("OK\n");

	fclose(decoder_client_data.file);

	printf("\nPASSED!\n");

	return true;
}

static FLAC__StreamDecoderWriteStatus file_decoder_write_callback_(const OggFLAC__FileDecoder *decoder, const FLAC__Frame *frame, const FLAC__int32 * const buffer[], void *client_data)
{
	(void)decoder;
	return stream_decoder_write_callback_(0, frame, buffer, client_data);
}

static void file_decoder_metadata_callback_(const OggFLAC__FileDecoder *decoder, const FLAC__StreamMetadata *metadata, void *client_data)
{
	(void)decoder;
	stream_decoder_metadata_callback_(0, metadata, client_data);
}

static void file_decoder_error_callback_(const OggFLAC__FileDecoder *decoder, FLAC__StreamDecoderErrorStatus status, void *client_data)
{
	(void)decoder;
	stream_decoder_error_callback_(0, status, client_data);
}

static FLAC__bool file_decoder_test_respond_(OggFLAC__FileDecoder *decoder, file_decoder_client_data_struct *dcd)
{
	if(!OggFLAC__file_decoder_set_write_callback(decoder, file_decoder_write_callback_))
		return die_f_("at OggFLAC__file_decoder_set_write_callback(), returned false", decoder);

	if(!OggFLAC__file_decoder_set_metadata_callback(decoder, file_decoder_metadata_callback_))
		return die_f_("at OggFLAC__file_decoder_set_metadata_callback(), returned false", decoder);

	if(!OggFLAC__file_decoder_set_error_callback(decoder, file_decoder_error_callback_))
		return die_f_("at OggFLAC__file_decoder_set_error_callback(), returned false", decoder);

	if(!OggFLAC__file_decoder_set_client_data(decoder, dcd))
		return die_f_("at OggFLAC__file_decoder_set_client_data(), returned false", decoder);

	if(!OggFLAC__file_decoder_set_filename(decoder, oggflacfilename_))
		return die_f_("at OggFLAC__file_decoder_set_filename(), returned false", decoder);

	if(!OggFLAC__file_decoder_set_md5_checking(decoder, true))
		return die_f_("at OggFLAC__file_decoder_set_md5_checking(), returned false", decoder);

	printf("testing OggFLAC__file_decoder_init()... ");
	if(OggFLAC__file_decoder_init(decoder) != OggFLAC__FILE_DECODER_OK)
		return die_f_(0, decoder);
	printf("OK\n");

	dcd->current_metadata_number = 0;

	printf("testing OggFLAC__file_decoder_process_until_end_of_file()... ");
	if(!OggFLAC__file_decoder_process_until_end_of_file(decoder))
		return die_f_("returned false", decoder);
	printf("OK\n");

	printf("testing OggFLAC__file_decoder_finish()... ");
	(void) OggFLAC__file_decoder_finish(decoder);
	printf("OK\n");

	return true;
}

static FLAC__bool test_file_decoder()
{
	OggFLAC__FileDecoder *decoder;
	OggFLAC__FileDecoderState state;
	OggFLAC__SeekableStreamDecoderState ssstate;
	OggFLAC__StreamDecoderState sstate;
	FLAC__StreamDecoderState fstate;
	seekable_stream_decoder_client_data_struct decoder_client_data;

	printf("\n+++ libOggFLAC unit test: OggFLAC__FileDecoder\n\n");

	printf("testing OggFLAC__file_decoder_new()... ");
	decoder = OggFLAC__file_decoder_new();
	if(0 == decoder) {
		printf("FAILED, returned NULL\n");
		return false;
	}
	printf("OK\n");

	printf("testing OggFLAC__file_decoder_delete()... ");
	OggFLAC__file_decoder_delete(decoder);
	printf("OK\n");

	printf("testing OggFLAC__file_decoder_new()... ");
	decoder = OggFLAC__file_decoder_new();
	if(0 == decoder) {
		printf("FAILED, returned NULL\n");
		return false;
	}
	printf("OK\n");

	printf("testing OggFLAC__file_decoder_init()... ");
	if(OggFLAC__file_decoder_init(decoder) == OggFLAC__FILE_DECODER_OK)
		return die_f_(0, decoder);
	printf("OK\n");

	printf("testing OggFLAC__file_decoder_delete()... ");
	OggFLAC__file_decoder_delete(decoder);
	printf("OK\n");

	num_expected_ = 0;
	expected_metadata_sequence_[num_expected_++] = &streaminfo_;

	printf("testing OggFLAC__file_decoder_new()... ");
	decoder = OggFLAC__file_decoder_new();
	if(0 == decoder) {
		printf("FAILED, returned NULL\n");
		return false;
	}
	printf("OK\n");

	printf("testing OggFLAC__file_decoder_set_serial_number()... ");
	if(!OggFLAC__file_decoder_set_serial_number(decoder, file_utils__serial_number))
		return die_f_("returned false", decoder);
	printf("OK\n");

	printf("testing OggFLAC__file_decoder_set_write_callback()... ");
	if(!OggFLAC__file_decoder_set_write_callback(decoder, file_decoder_write_callback_))
		return die_f_("returned false", decoder);
	printf("OK\n");

	printf("testing OggFLAC__file_decoder_set_metadata_callback()... ");
	if(!OggFLAC__file_decoder_set_metadata_callback(decoder, file_decoder_metadata_callback_))
		return die_f_("returned false", decoder);
	printf("OK\n");

	printf("testing OggFLAC__file_decoder_set_error_callback()... ");
	if(!OggFLAC__file_decoder_set_error_callback(decoder, file_decoder_error_callback_))
		return die_f_("returned false", decoder);
	printf("OK\n");

	printf("testing OggFLAC__file_decoder_set_client_data()... ");
	if(!OggFLAC__file_decoder_set_client_data(decoder, &decoder_client_data))
		return die_f_("returned false", decoder);
	printf("OK\n");

	printf("testing OggFLAC__file_decoder_set_filename()... ");
	if(!OggFLAC__file_decoder_set_filename(decoder, oggflacfilename_))
		return die_f_("returned false", decoder);
	printf("OK\n");

	printf("testing OggFLAC__file_decoder_set_md5_checking()... ");
	if(!OggFLAC__file_decoder_set_md5_checking(decoder, true))
		return die_f_("returned false", decoder);
	printf("OK\n");

	printf("testing OggFLAC__file_decoder_init()... ");
	if(OggFLAC__file_decoder_init(decoder) != OggFLAC__FILE_DECODER_OK)
		return die_f_(0, decoder);
	printf("OK\n");

	printf("testing OggFLAC__file_decoder_get_state()... ");
	state = OggFLAC__file_decoder_get_state(decoder);
	printf("returned state = %u (%s)... OK\n", state, OggFLAC__FileDecoderStateString[state]);

	printf("testing OggFLAC__file_decoder_get_seekable_stream_decoder_state()... ");
	ssstate = OggFLAC__file_decoder_get_seekable_stream_decoder_state(decoder);
	printf("returned state = %u (%s)... OK\n", ssstate, OggFLAC__SeekableStreamDecoderStateString[ssstate]);

	printf("testing OggFLAC__file_decoder_get_stream_decoder_state()... ");
	sstate = OggFLAC__file_decoder_get_stream_decoder_state(decoder);
	printf("returned state = %u (%s)... OK\n", sstate, OggFLAC__StreamDecoderStateString[sstate]);

	printf("testing OggFLAC__file_decoder_get_FLAC_stream_decoder_state()... ");
	fstate = OggFLAC__file_decoder_get_FLAC_stream_decoder_state(decoder);
	printf("returned state = %u (%s)... OK\n", fstate, FLAC__StreamDecoderStateString[fstate]);

	decoder_client_data.current_metadata_number = 0;
	decoder_client_data.ignore_errors = false;
	decoder_client_data.error_occurred = false;

	printf("testing OggFLAC__file_decoder_get_md5_checking()... ");
	if(!OggFLAC__file_decoder_get_md5_checking(decoder)) {
		printf("FAILED, returned false, expected true\n");
		return false;
	}
	printf("OK\n");

	printf("testing OggFLAC__file_decoder_process_until_end_of_metadata()... ");
	if(!OggFLAC__file_decoder_process_until_end_of_metadata(decoder))
		return die_f_("returned false", decoder);
	printf("OK\n");

	printf("testing OggFLAC__file_decoder_process_single()... ");
	if(!OggFLAC__file_decoder_process_single(decoder))
		return die_f_("returned false", decoder);
	printf("OK\n");

	printf("testing OggFLAC__file_decoder_seek_absolute()... ");
	if(!OggFLAC__file_decoder_seek_absolute(decoder, 0))
		return die_f_("returned false", decoder);
	printf("OK\n");

	printf("testing OggFLAC__file_decoder_process_until_end_of_file()... ");
	if(!OggFLAC__file_decoder_process_until_end_of_file(decoder))
		return die_f_("returned false", decoder);
	printf("OK\n");

	printf("testing OggFLAC__file_decoder_get_channels()... ");
	{
		unsigned channels = OggFLAC__file_decoder_get_channels(decoder);
		if(channels != streaminfo_.data.stream_info.channels) {
			printf("FAILED, returned %u, expected %u\n", channels, streaminfo_.data.stream_info.channels);
			return false;
		}
	}
	printf("OK\n");

	printf("testing OggFLAC__file_decoder_get_bits_per_sample()... ");
	{
		unsigned bits_per_sample = OggFLAC__file_decoder_get_bits_per_sample(decoder);
		if(bits_per_sample != streaminfo_.data.stream_info.bits_per_sample) {
			printf("FAILED, returned %u, expected %u\n", bits_per_sample, streaminfo_.data.stream_info.bits_per_sample);
			return false;
		}
	}
	printf("OK\n");

	printf("testing OggFLAC__file_decoder_get_sample_rate()... ");
	{
		unsigned sample_rate = OggFLAC__file_decoder_get_sample_rate(decoder);
		if(sample_rate != streaminfo_.data.stream_info.sample_rate) {
			printf("FAILED, returned %u, expected %u\n", sample_rate, streaminfo_.data.stream_info.sample_rate);
			return false;
		}
	}
	printf("OK\n");

	printf("testing OggFLAC__file_decoder_get_blocksize()... ");
	{
		unsigned blocksize = OggFLAC__file_decoder_get_blocksize(decoder);
		/* value could be anything since we're at the last block, so accept any answer */
		printf("returned %u... OK\n", blocksize);
	}

	printf("testing OggFLAC__file_decoder_get_channel_assignment()... ");
	{
		FLAC__ChannelAssignment ca = OggFLAC__file_decoder_get_channel_assignment(decoder);
		printf("returned %u (%s)... OK\n", (unsigned)ca, FLAC__ChannelAssignmentString[ca]);
	}

	printf("testing OggFLAC__file_decoder_finish()... ");
	(void) OggFLAC__file_decoder_finish(decoder);
	printf("OK\n");

	/*
	 * respond all
	 */

	printf("testing OggFLAC__file_decoder_set_metadata_respond_all()... ");
	if(!OggFLAC__file_decoder_set_metadata_respond_all(decoder))
		return die_f_("returned false", decoder);
	printf("OK\n");

	num_expected_ = 0;
	expected_metadata_sequence_[num_expected_++] = &streaminfo_;
	expected_metadata_sequence_[num_expected_++] = &vorbiscomment_;
	expected_metadata_sequence_[num_expected_++] = &padding_;
	expected_metadata_sequence_[num_expected_++] = &seektable_;
	expected_metadata_sequence_[num_expected_++] = &application1_;
	expected_metadata_sequence_[num_expected_++] = &application2_;
	expected_metadata_sequence_[num_expected_++] = &cuesheet_;
	expected_metadata_sequence_[num_expected_++] = &unknown_;

	if(!file_decoder_test_respond_(decoder, &decoder_client_data))
		return false;

	/*
	 * ignore all
	 */

	printf("testing OggFLAC__file_decoder_set_metadata_ignore_all()... ");
	if(!OggFLAC__file_decoder_set_metadata_ignore_all(decoder))
		return die_f_("returned false", decoder);
	printf("OK\n");

	num_expected_ = 0;

	if(!file_decoder_test_respond_(decoder, &decoder_client_data))
		return false;

	/*
	 * respond all, ignore VORBIS_COMMENT
	 */

	printf("testing OggFLAC__file_decoder_set_metadata_respond_all()... ");
	if(!OggFLAC__file_decoder_set_metadata_respond_all(decoder))
		return die_f_("returned false", decoder);
	printf("OK\n");

	printf("testing OggFLAC__file_decoder_set_metadata_ignore(VORBIS_COMMENT)... ");
	if(!OggFLAC__file_decoder_set_metadata_ignore(decoder, FLAC__METADATA_TYPE_VORBIS_COMMENT))
		return die_f_("returned false", decoder);
	printf("OK\n");

	num_expected_ = 0;
	expected_metadata_sequence_[num_expected_++] = &streaminfo_;
	expected_metadata_sequence_[num_expected_++] = &padding_;
	expected_metadata_sequence_[num_expected_++] = &seektable_;
	expected_metadata_sequence_[num_expected_++] = &application1_;
	expected_metadata_sequence_[num_expected_++] = &application2_;
	expected_metadata_sequence_[num_expected_++] = &cuesheet_;
	expected_metadata_sequence_[num_expected_++] = &unknown_;

	if(!file_decoder_test_respond_(decoder, &decoder_client_data))
		return false;

	/*
	 * respond all, ignore APPLICATION
	 */

	printf("testing OggFLAC__file_decoder_set_metadata_respond_all()... ");
	if(!OggFLAC__file_decoder_set_metadata_respond_all(decoder))
		return die_f_("returned false", decoder);
	printf("OK\n");

	printf("testing OggFLAC__file_decoder_set_metadata_ignore(APPLICATION)... ");
	if(!OggFLAC__file_decoder_set_metadata_ignore(decoder, FLAC__METADATA_TYPE_APPLICATION))
		return die_f_("returned false", decoder);
	printf("OK\n");

	num_expected_ = 0;
	expected_metadata_sequence_[num_expected_++] = &streaminfo_;
	expected_metadata_sequence_[num_expected_++] = &vorbiscomment_;
	expected_metadata_sequence_[num_expected_++] = &padding_;
	expected_metadata_sequence_[num_expected_++] = &seektable_;
	expected_metadata_sequence_[num_expected_++] = &cuesheet_;
	expected_metadata_sequence_[num_expected_++] = &unknown_;

	if(!file_decoder_test_respond_(decoder, &decoder_client_data))
		return false;

	/*
	 * respond all, ignore APPLICATION id of app#1
	 */

	printf("testing OggFLAC__file_decoder_set_metadata_respond_all()... ");
	if(!OggFLAC__file_decoder_set_metadata_respond_all(decoder))
		return die_f_("returned false", decoder);
	printf("OK\n");

	printf("testing OggFLAC__file_decoder_set_metadata_ignore_application(of app block #1)... ");
	if(!OggFLAC__file_decoder_set_metadata_ignore_application(decoder, application1_.data.application.id))
		return die_f_("returned false", decoder);
	printf("OK\n");

	num_expected_ = 0;
	expected_metadata_sequence_[num_expected_++] = &streaminfo_;
	expected_metadata_sequence_[num_expected_++] = &vorbiscomment_;
	expected_metadata_sequence_[num_expected_++] = &padding_;
	expected_metadata_sequence_[num_expected_++] = &seektable_;
	expected_metadata_sequence_[num_expected_++] = &application2_;
	expected_metadata_sequence_[num_expected_++] = &cuesheet_;
	expected_metadata_sequence_[num_expected_++] = &unknown_;

	if(!file_decoder_test_respond_(decoder, &decoder_client_data))
		return false;

	/*
	 * respond all, ignore APPLICATION id of app#1 & app#2
	 */

	printf("testing OggFLAC__file_decoder_set_metadata_respond_all()... ");
	if(!OggFLAC__file_decoder_set_metadata_respond_all(decoder))
		return die_f_("returned false", decoder);
	printf("OK\n");

	printf("testing OggFLAC__file_decoder_set_metadata_ignore_application(of app block #1)... ");
	if(!OggFLAC__file_decoder_set_metadata_ignore_application(decoder, application1_.data.application.id))
		return die_f_("returned false", decoder);
	printf("OK\n");

	printf("testing OggFLAC__file_decoder_set_metadata_ignore_application(of app block #2)... ");
	if(!OggFLAC__file_decoder_set_metadata_ignore_application(decoder, application2_.data.application.id))
		return die_f_("returned false", decoder);
	printf("OK\n");

	num_expected_ = 0;
	expected_metadata_sequence_[num_expected_++] = &streaminfo_;
	expected_metadata_sequence_[num_expected_++] = &vorbiscomment_;
	expected_metadata_sequence_[num_expected_++] = &padding_;
	expected_metadata_sequence_[num_expected_++] = &seektable_;
	expected_metadata_sequence_[num_expected_++] = &cuesheet_;
	expected_metadata_sequence_[num_expected_++] = &unknown_;

	if(!file_decoder_test_respond_(decoder, &decoder_client_data))
		return false;

	/*
	 * ignore all, respond VORBIS_COMMENT
	 */

	printf("testing OggFLAC__file_decoder_set_metadata_ignore_all()... ");
	if(!OggFLAC__file_decoder_set_metadata_ignore_all(decoder))
		return die_f_("returned false", decoder);
	printf("OK\n");

	printf("testing OggFLAC__file_decoder_set_metadata_respond(VORBIS_COMMENT)... ");
	if(!OggFLAC__file_decoder_set_metadata_respond(decoder, FLAC__METADATA_TYPE_VORBIS_COMMENT))
		return die_f_("returned false", decoder);
	printf("OK\n");

	num_expected_ = 0;
	expected_metadata_sequence_[num_expected_++] = &vorbiscomment_;

	if(!file_decoder_test_respond_(decoder, &decoder_client_data))
		return false;

	/*
	 * ignore all, respond APPLICATION
	 */

	printf("testing OggFLAC__file_decoder_set_metadata_ignore_all()... ");
	if(!OggFLAC__file_decoder_set_metadata_ignore_all(decoder))
		return die_f_("returned false", decoder);
	printf("OK\n");

	printf("testing OggFLAC__file_decoder_set_metadata_respond(APPLICATION)... ");
	if(!OggFLAC__file_decoder_set_metadata_respond(decoder, FLAC__METADATA_TYPE_APPLICATION))
		return die_f_("returned false", decoder);
	printf("OK\n");

	num_expected_ = 0;
	expected_metadata_sequence_[num_expected_++] = &application1_;
	expected_metadata_sequence_[num_expected_++] = &application2_;

	if(!file_decoder_test_respond_(decoder, &decoder_client_data))
		return false;

	/*
	 * ignore all, respond APPLICATION id of app#1
	 */

	printf("testing OggFLAC__file_decoder_set_metadata_ignore_all()... ");
	if(!OggFLAC__file_decoder_set_metadata_ignore_all(decoder))
		return die_f_("returned false", decoder);
	printf("OK\n");

	printf("testing OggFLAC__file_decoder_set_metadata_respond_application(of app block #1)... ");
	if(!OggFLAC__file_decoder_set_metadata_respond_application(decoder, application1_.data.application.id))
		return die_f_("returned false", decoder);
	printf("OK\n");

	num_expected_ = 0;
	expected_metadata_sequence_[num_expected_++] = &application1_;

	if(!file_decoder_test_respond_(decoder, &decoder_client_data))
		return false;

	/*
	 * ignore all, respond APPLICATION id of app#1 & app#2
	 */

	printf("testing OggFLAC__file_decoder_set_metadata_ignore_all()... ");
	if(!OggFLAC__file_decoder_set_metadata_ignore_all(decoder))
		return die_f_("returned false", decoder);
	printf("OK\n");

	printf("testing OggFLAC__file_decoder_set_metadata_respond_application(of app block #1)... ");
	if(!OggFLAC__file_decoder_set_metadata_respond_application(decoder, application1_.data.application.id))
		return die_f_("returned false", decoder);
	printf("OK\n");

	printf("testing OggFLAC__file_decoder_set_metadata_respond_application(of app block #2)... ");
	if(!OggFLAC__file_decoder_set_metadata_respond_application(decoder, application2_.data.application.id))
		return die_f_("returned false", decoder);
	printf("OK\n");

	num_expected_ = 0;
	expected_metadata_sequence_[num_expected_++] = &application1_;
	expected_metadata_sequence_[num_expected_++] = &application2_;

	if(!file_decoder_test_respond_(decoder, &decoder_client_data))
		return false;

	/*
	 * respond all, ignore APPLICATION, respond APPLICATION id of app#1
	 */

	printf("testing OggFLAC__file_decoder_set_metadata_respond_all()... ");
	if(!OggFLAC__file_decoder_set_metadata_respond_all(decoder))
		return die_f_("returned false", decoder);
	printf("OK\n");

	printf("testing OggFLAC__file_decoder_set_metadata_ignore(APPLICATION)... ");
	if(!OggFLAC__file_decoder_set_metadata_ignore(decoder, FLAC__METADATA_TYPE_APPLICATION))
		return die_f_("returned false", decoder);
	printf("OK\n");

	printf("testing OggFLAC__file_decoder_set_metadata_respond_application(of app block #1)... ");
	if(!OggFLAC__file_decoder_set_metadata_respond_application(decoder, application1_.data.application.id))
		return die_f_("returned false", decoder);
	printf("OK\n");

	num_expected_ = 0;
	expected_metadata_sequence_[num_expected_++] = &streaminfo_;
	expected_metadata_sequence_[num_expected_++] = &vorbiscomment_;
	expected_metadata_sequence_[num_expected_++] = &padding_;
	expected_metadata_sequence_[num_expected_++] = &seektable_;
	expected_metadata_sequence_[num_expected_++] = &application1_;
	expected_metadata_sequence_[num_expected_++] = &cuesheet_;
	expected_metadata_sequence_[num_expected_++] = &unknown_;

	if(!file_decoder_test_respond_(decoder, &decoder_client_data))
		return false;

	/*
	 * ignore all, respond APPLICATION, ignore APPLICATION id of app#1
	 */

	printf("testing OggFLAC__file_decoder_set_metadata_ignore_all()... ");
	if(!OggFLAC__file_decoder_set_metadata_ignore_all(decoder))
		return die_f_("returned false", decoder);
	printf("OK\n");

	printf("testing OggFLAC__file_decoder_set_metadata_respond(APPLICATION)... ");
	if(!OggFLAC__file_decoder_set_metadata_respond(decoder, FLAC__METADATA_TYPE_APPLICATION))
		return die_f_("returned false", decoder);
	printf("OK\n");

	printf("testing OggFLAC__file_decoder_set_metadata_ignore_application(of app block #1)... ");
	if(!OggFLAC__file_decoder_set_metadata_ignore_application(decoder, application1_.data.application.id))
		return die_f_("returned false", decoder);
	printf("OK\n");

	num_expected_ = 0;
	expected_metadata_sequence_[num_expected_++] = &application2_;

	if(!file_decoder_test_respond_(decoder, &decoder_client_data))
		return false;

	/* done, now leave the sequence the way we found it... */
	num_expected_ = 0;
	expected_metadata_sequence_[num_expected_++] = &streaminfo_;
	expected_metadata_sequence_[num_expected_++] = &vorbiscomment_;
	expected_metadata_sequence_[num_expected_++] = &padding_;
	expected_metadata_sequence_[num_expected_++] = &seektable_;
	expected_metadata_sequence_[num_expected_++] = &application1_;
	expected_metadata_sequence_[num_expected_++] = &application2_;
	expected_metadata_sequence_[num_expected_++] = &cuesheet_;
	expected_metadata_sequence_[num_expected_++] = &unknown_;

	printf("testing OggFLAC__file_decoder_delete()... ");
	OggFLAC__file_decoder_delete(decoder);
	printf("OK\n");

	printf("\nPASSED!\n");

	return true;
}

FLAC__bool test_decoders()
{
	init_metadata_blocks_();
	if(!generate_file_())
		return false;

	if(!test_stream_decoder())
		return false;

	if(!test_seekable_stream_decoder())
		return false;

	if(!test_file_decoder())
		return false;

	(void) grabbag__file_remove_file(oggflacfilename_);
	free_metadata_blocks_();

	return true;
}
