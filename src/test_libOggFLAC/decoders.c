/* test_libOggFLAC - Unit tester for libOggFLAC
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

#include "decoders.h"
#include "file_utils.h"
#include "metadata_utils.h"
#include "FLAC/assert.h"
#include "OggFLAC/stream_decoder.h"
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

static FLAC__StreamMetadata streaminfo_, padding_, seektable_, application1_, application2_, vorbiscomment_;
static FLAC__StreamMetadata *expected_metadata_sequence_[6];
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

static void *malloc_or_die_(size_t size)
{
	void *x = malloc(size);
	if(0 == x) {
		fprintf(stderr, "ERROR: out of memory allocating %u bytes\n", (unsigned)size);
		exit(1);
	}
	return x;
}

static void init_metadata_blocks_()
{
	/*
		most of the actual numbers and data in the blocks don't matter,
		we just want to make sure the decoder parses them correctly

		remember, the metadata interface gets tested after the decoders,
		so we do all the metadata manipulation here without it.
	*/

	/* min/max_framesize and md5sum don't get written at first, so we have to leave them 0 */
	streaminfo_.is_last = false;
	streaminfo_.type = FLAC__METADATA_TYPE_STREAMINFO;
	streaminfo_.length = FLAC__STREAM_METADATA_STREAMINFO_LENGTH;
	streaminfo_.data.stream_info.min_blocksize = 576;
	streaminfo_.data.stream_info.max_blocksize = 576;
	streaminfo_.data.stream_info.min_framesize = 0;
	streaminfo_.data.stream_info.max_framesize = 0;
	streaminfo_.data.stream_info.sample_rate = 44100;
	streaminfo_.data.stream_info.channels = 1;
	streaminfo_.data.stream_info.bits_per_sample = 8;
	streaminfo_.data.stream_info.total_samples = 0;
	memset(streaminfo_.data.stream_info.md5sum, 0, 16);

	padding_.is_last = false;
	padding_.type = FLAC__METADATA_TYPE_PADDING;
	padding_.length = 1234;

	seektable_.is_last = false;
	seektable_.type = FLAC__METADATA_TYPE_SEEKTABLE;
	seektable_.data.seek_table.num_points = 2;
	seektable_.length = seektable_.data.seek_table.num_points * FLAC__STREAM_METADATA_SEEKPOINT_LENGTH;
	seektable_.data.seek_table.points = malloc_or_die_(seektable_.data.seek_table.num_points * sizeof(FLAC__StreamMetadata_SeekPoint));
	seektable_.data.seek_table.points[0].sample_number = 0;
	seektable_.data.seek_table.points[0].stream_offset = 0;
	seektable_.data.seek_table.points[0].frame_samples = streaminfo_.data.stream_info.min_blocksize;
	seektable_.data.seek_table.points[1].sample_number = FLAC__STREAM_METADATA_SEEKPOINT_PLACEHOLDER;
	seektable_.data.seek_table.points[1].stream_offset = 1000;
	seektable_.data.seek_table.points[1].frame_samples = streaminfo_.data.stream_info.min_blocksize;

	application1_.is_last = false;
	application1_.type = FLAC__METADATA_TYPE_APPLICATION;
	application1_.length = 8;
	memcpy(application1_.data.application.id, "\xfe\xdc\xba\x98", 4);
	application1_.data.application.data = malloc_or_die_(4);
	memcpy(application1_.data.application.data, "\xf0\xe1\xd2\xc3", 4);

	application2_.is_last = false;
	application2_.type = FLAC__METADATA_TYPE_APPLICATION;
	application2_.length = 4;
	memcpy(application2_.data.application.id, "\x76\x54\x32\x10", 4);
	application2_.data.application.data = 0;

	{
		const unsigned vendor_string_length = (unsigned)strlen(FLAC__VENDOR_STRING);
		vorbiscomment_.is_last = true;
		vorbiscomment_.type = FLAC__METADATA_TYPE_VORBIS_COMMENT;
		vorbiscomment_.length = (4 + vendor_string_length) + 4 + (4 + 5) + (4 + 0);
		vorbiscomment_.data.vorbis_comment.vendor_string.length = vendor_string_length;
		vorbiscomment_.data.vorbis_comment.vendor_string.entry = malloc_or_die_(vendor_string_length);
		memcpy(vorbiscomment_.data.vorbis_comment.vendor_string.entry, FLAC__VENDOR_STRING, vendor_string_length);
			vorbiscomment_.data.vorbis_comment.num_comments = 2;
		vorbiscomment_.data.vorbis_comment.comments = malloc_or_die_(vorbiscomment_.data.vorbis_comment.num_comments * sizeof(FLAC__StreamMetadata_VorbisComment_Entry));
		vorbiscomment_.data.vorbis_comment.comments[0].length = 5;
		vorbiscomment_.data.vorbis_comment.comments[0].entry = malloc_or_die_(5);
		memcpy(vorbiscomment_.data.vorbis_comment.comments[0].entry, "ab=cd", 5);
		vorbiscomment_.data.vorbis_comment.comments[1].length = 0;
		vorbiscomment_.data.vorbis_comment.comments[1].entry = 0;
	}
}

static void free_metadata_blocks_()
{
	free(seektable_.data.seek_table.points);
	free(application1_.data.application.data);
	free(vorbiscomment_.data.vorbis_comment.vendor_string.entry);
	free(vorbiscomment_.data.vorbis_comment.comments[0].entry);
	free(vorbiscomment_.data.vorbis_comment.comments);
}

static FLAC__bool generate_file_()
{
	printf("\n\ngenerating Ogg FLAC file for decoder tests...\n");

	expected_metadata_sequence_[0] = &padding_;
	expected_metadata_sequence_[1] = &seektable_;
	expected_metadata_sequence_[2] = &application1_;
	expected_metadata_sequence_[3] = &application2_;
	expected_metadata_sequence_[4] = &vorbiscomment_;
	num_expected_ = 5;

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

	if(feof(dcd->file))
		return FLAC__STREAM_DECODER_READ_STATUS_END_OF_STREAM;
	else if(*bytes > 0) {
		unsigned bytes_read = fread(buffer, 1, *bytes, dcd->file);
		if(bytes_read == 0) {
			if(feof(dcd->file))
				return FLAC__STREAM_DECODER_READ_STATUS_END_OF_STREAM;
			else
				return FLAC__STREAM_DECODER_READ_STATUS_CONTINUE;
		}
		else {
			*bytes = bytes_read;
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
		if(!compare_block_(expected_metadata_sequence_[dcd->current_metadata_number], metadata)) {
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
	expected_metadata_sequence_[num_expected_++] = &padding_;
	expected_metadata_sequence_[num_expected_++] = &seektable_;
	expected_metadata_sequence_[num_expected_++] = &application1_;
	expected_metadata_sequence_[num_expected_++] = &application2_;
	expected_metadata_sequence_[num_expected_++] = &vorbiscomment_;

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
	expected_metadata_sequence_[num_expected_++] = &padding_;
	expected_metadata_sequence_[num_expected_++] = &seektable_;
	expected_metadata_sequence_[num_expected_++] = &vorbiscomment_;

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
	expected_metadata_sequence_[num_expected_++] = &padding_;
	expected_metadata_sequence_[num_expected_++] = &seektable_;
	expected_metadata_sequence_[num_expected_++] = &application2_;
	expected_metadata_sequence_[num_expected_++] = &vorbiscomment_;

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
	expected_metadata_sequence_[num_expected_++] = &padding_;
	expected_metadata_sequence_[num_expected_++] = &seektable_;
	expected_metadata_sequence_[num_expected_++] = &vorbiscomment_;

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
	expected_metadata_sequence_[num_expected_++] = &padding_;
	expected_metadata_sequence_[num_expected_++] = &seektable_;
	expected_metadata_sequence_[num_expected_++] = &application1_;
	expected_metadata_sequence_[num_expected_++] = &vorbiscomment_;

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
	expected_metadata_sequence_[num_expected_++] = &padding_;
	expected_metadata_sequence_[num_expected_++] = &seektable_;
	expected_metadata_sequence_[num_expected_++] = &application1_;
	expected_metadata_sequence_[num_expected_++] = &application2_;
	expected_metadata_sequence_[num_expected_++] = &vorbiscomment_;

	printf("testing OggFLAC__stream_decoder_delete()... ");
	OggFLAC__stream_decoder_delete(decoder);
	printf("OK\n");

	fclose(decoder_client_data.file);

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

	(void) file_utils__remove_file(oggflacfilename_);
	free_metadata_blocks_();

	return true;
}
