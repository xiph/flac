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

#include "decoders.h"
#include "file_utils.h"
#include "metadata_utils.h"
#include "FLAC/assert.h"
#include "FLAC/file_decoder.h"
#include "FLAC/seekable_stream_decoder.h"
#include "FLAC/stream_decoder.h"
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

static FLAC__StreamMetaData streaminfo_, padding_, seektable_, application1_, application2_, vorbiscomment_;
static FLAC__StreamMetaData *expected_metadata_sequence_[6];
static unsigned num_expected_;
static const char *flacfilename_ = "metadata.flac";
static unsigned flacfilesize_;

static FLAC__bool die_(const char *msg)
{
	printf("ERROR: %s\n", msg);
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
	seektable_.data.seek_table.points = malloc_or_die_(seektable_.data.seek_table.num_points * sizeof(FLAC__StreamMetaData_SeekPoint));
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

    vorbiscomment_.is_last = true;
    vorbiscomment_.type = FLAC__METADATA_TYPE_VORBIS_COMMENT;
    vorbiscomment_.length = (4 + 8) + 4 + (4 + 5) + (4 + 0);
	vorbiscomment_.data.vorbis_comment.vendor_string.length = 8;
	vorbiscomment_.data.vorbis_comment.vendor_string.entry = malloc_or_die_(8);
	memcpy(vorbiscomment_.data.vorbis_comment.vendor_string.entry, "flac 1.x", 8);
	vorbiscomment_.data.vorbis_comment.num_comments = 2;
	vorbiscomment_.data.vorbis_comment.comments = malloc_or_die_(vorbiscomment_.data.vorbis_comment.num_comments * sizeof(FLAC__StreamMetaData_VorbisComment_Entry));
	vorbiscomment_.data.vorbis_comment.comments[0].length = 5;
	vorbiscomment_.data.vorbis_comment.comments[0].entry = malloc_or_die_(5);
	memcpy(vorbiscomment_.data.vorbis_comment.comments[0].entry, "ab=cd", 5);
	vorbiscomment_.data.vorbis_comment.comments[1].length = 0;
	vorbiscomment_.data.vorbis_comment.comments[1].entry = 0;
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
	printf("\n\ngenerating FLAC file for decoder tests...\n");

	expected_metadata_sequence_[0] = &padding_;
	expected_metadata_sequence_[1] = &seektable_;
	expected_metadata_sequence_[2] = &application1_;
	expected_metadata_sequence_[3] = &application2_;
	expected_metadata_sequence_[4] = &vorbiscomment_;
	num_expected_ = 5;

	if(!file_utils__generate_flacfile(flacfilename_, &flacfilesize_, 512 * 1024, &streaminfo_, expected_metadata_sequence_, num_expected_))
		return die_("creating the encoded file"); 

	return true;
}

static FLAC__StreamDecoderReadStatus stream_decoder_read_callback_(const FLAC__StreamDecoder *decoder, FLAC__byte buffer[], unsigned *bytes, void *client_data)
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

static FLAC__StreamDecoderWriteStatus stream_decoder_write_callback_(const FLAC__StreamDecoder *decoder, const FLAC__Frame *frame, const FLAC__int32 *buffer[], void *client_data)
{
	stream_decoder_client_data_struct *dcd = (stream_decoder_client_data_struct*)client_data;

	(void)decoder, (void)frame, (void)buffer;

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

static void stream_decoder_metadata_callback_(const FLAC__StreamDecoder *decoder, const FLAC__StreamMetaData *metadata, void *client_data)
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

static void stream_decoder_error_callback_(const FLAC__StreamDecoder *decoder, FLAC__StreamDecoderErrorStatus status, void *client_data)
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

static FLAC__bool stream_decoder_test_respond_(FLAC__StreamDecoder *decoder, stream_decoder_client_data_struct *dcd)
{
	FLAC__StreamDecoderState state;

	if(!FLAC__stream_decoder_set_read_callback(decoder, stream_decoder_read_callback_)) {
		printf("FAILED at FLAC__stream_decoder_set_read_callback(), returned false\n");
		return false;
	}

	if(!FLAC__stream_decoder_set_write_callback(decoder, stream_decoder_write_callback_)) {
		printf("FAILED at FLAC__stream_decoder_set_write_callback(), returned false\n");
		return false;
	}

	if(!FLAC__stream_decoder_set_metadata_callback(decoder, stream_decoder_metadata_callback_)) {
		printf("FAILED at FLAC__stream_decoder_set_metadata_callback(), returned false\n");
		return false;
	}

	if(!FLAC__stream_decoder_set_error_callback(decoder, stream_decoder_error_callback_)) {
		printf("FAILED at FLAC__stream_decoder_set_error_callback(), returned false\n");
		return false;
	}

	if(!FLAC__stream_decoder_set_client_data(decoder, dcd)) {
		printf("FAILED at FLAC__stream_decoder_set_client_data(), returned false\n");
		return false;
	}

	printf("testing FLAC__stream_decoder_init()... ");
	if((state = FLAC__stream_decoder_init(decoder)) != FLAC__STREAM_DECODER_SEARCH_FOR_METADATA) {
		printf("FAILED, returned state = %u (%s)\n", state, FLAC__StreamDecoderStateString[state]);
		return false;
	}
	printf("OK\n");

	dcd->current_metadata_number = 0;

	printf("rewinding input... ");
	if(fseek(dcd->file, 0, SEEK_SET) < 0) {
		printf("FAILED, errno = %d\n", errno);
		return false;
	}
	printf("OK\n");

	printf("testing FLAC__stream_decoder_process_whole_stream()... ");
	if(!FLAC__stream_decoder_process_whole_stream(decoder)) {
		state = FLAC__stream_decoder_get_state(decoder);
		printf("FAILED, returned false, state = %u (%s)\n", state, FLAC__StreamDecoderStateString[state]);
		return false;
	}
	printf("OK\n");

	printf("testing FLAC__stream_decoder_finish()... ");
	FLAC__stream_decoder_finish(decoder);
	printf("OK\n");

	return true;;
}

FLAC__bool test_stream_decoder()
{
	FLAC__StreamDecoder *decoder;
	FLAC__StreamDecoderState state;
	stream_decoder_client_data_struct decoder_client_data;

	printf("\n+++ unit test: FLAC__StreamDecoder\n\n");

	num_expected_ = 0;
	expected_metadata_sequence_[num_expected_++] = &streaminfo_;

	printf("testing FLAC__stream_decoder_new()... ");
	decoder = FLAC__stream_decoder_new();
	if(0 == decoder) {
		printf("FAILED, returned NULL\n");
		return false;
	}
	printf("OK\n");

	printf("testing FLAC__stream_decoder_set_read_callback()... ");
	if(!FLAC__stream_decoder_set_read_callback(decoder, stream_decoder_read_callback_)) {
		printf("FAILED, returned false\n");
		return false;
	}
	printf("OK\n");

	printf("testing FLAC__stream_decoder_set_write_callback()... ");
	if(!FLAC__stream_decoder_set_write_callback(decoder, stream_decoder_write_callback_)) {
		printf("FAILED, returned false\n");
		return false;
	}
	printf("OK\n");

	printf("testing FLAC__stream_decoder_set_metadata_callback()... ");
	if(!FLAC__stream_decoder_set_metadata_callback(decoder, stream_decoder_metadata_callback_)) {
		printf("FAILED, returned false\n");
		return false;
	}
	printf("OK\n");

	printf("testing FLAC__stream_decoder_set_error_callback()... ");
	if(!FLAC__stream_decoder_set_error_callback(decoder, stream_decoder_error_callback_)) {
		printf("FAILED, returned false\n");
		return false;
	}
	printf("OK\n");

	printf("testing FLAC__stream_decoder_set_client_data()... ");
	if(!FLAC__stream_decoder_set_client_data(decoder, &decoder_client_data)) {
		printf("FAILED, returned false\n");
		return false;
	}
	printf("OK\n");

	printf("testing FLAC__stream_decoder_init()... ");
	if((state = FLAC__stream_decoder_init(decoder)) != FLAC__STREAM_DECODER_SEARCH_FOR_METADATA) {
		printf("FAILED, returned state = %u (%s)\n", state, FLAC__StreamDecoderStateString[state]);
		return false;
	}
	printf("OK\n");

	printf("testing FLAC__stream_decoder_get_state()... ");
	state = FLAC__stream_decoder_get_state(decoder);
	printf("returned state = %u (%s)... OK\n", state, FLAC__StreamDecoderStateString[state]);

	decoder_client_data.current_metadata_number = 0;
	decoder_client_data.ignore_errors = false;
	decoder_client_data.error_occurred = false;

	printf("opening FLAC file... ");
	decoder_client_data.file = fopen(flacfilename_, "rb");
	if(0 == decoder_client_data.file) {
		printf("ERROR\n");
		return false;
	}
	printf("OK\n");

	printf("testing FLAC__stream_decoder_process_metadata()... ");
	if(!FLAC__stream_decoder_process_metadata(decoder)) {
		state = FLAC__stream_decoder_get_state(decoder);
		printf("FAILED, returned false, state = %u (%s)\n", state, FLAC__StreamDecoderStateString[state]);
		return false;
	}
	printf("OK\n");

	printf("testing FLAC__stream_decoder_process_one_frame()... ");
	if(!FLAC__stream_decoder_process_one_frame(decoder)) {
		state = FLAC__stream_decoder_get_state(decoder);
		printf("FAILED, returned false, state = %u (%s)\n", state, FLAC__StreamDecoderStateString[state]);
		return false;
	}
	printf("OK\n");

	printf("testing FLAC__stream_decoder_flush()... ");
	if(!FLAC__stream_decoder_flush(decoder)) {
		state = FLAC__stream_decoder_get_state(decoder);
		printf("FAILED, returned false, state = %u (%s)\n", state, FLAC__StreamDecoderStateString[state]);
		return false;
	}
	printf("OK\n");

	decoder_client_data.ignore_errors = true;
	printf("testing FLAC__stream_decoder_process_one_frame()... ");
	if(!FLAC__stream_decoder_process_one_frame(decoder)) {
		state = FLAC__stream_decoder_get_state(decoder);
		printf("FAILED, returned false, state = %u (%s)\n", state, FLAC__StreamDecoderStateString[state]);
		return false;
	}
	printf("OK\n");
	decoder_client_data.ignore_errors = false;

	printf("testing FLAC__stream_decoder_process_remaining_frames()... ");
	if(!FLAC__stream_decoder_process_remaining_frames(decoder)) {
		state = FLAC__stream_decoder_get_state(decoder);
		printf("FAILED, returned false, state = %u (%s)\n", state, FLAC__StreamDecoderStateString[state]);
		return false;
	}
	printf("OK\n");

	printf("testing FLAC__stream_decoder_get_channels()... ");
	{
		unsigned channels = FLAC__stream_decoder_get_channels(decoder);
		if(channels != streaminfo_.data.stream_info.channels) {
			printf("FAILED, returned %u, expected %u\n", channels, streaminfo_.data.stream_info.channels);
			return false;
		}
	}
	printf("OK\n");

	printf("testing FLAC__stream_decoder_get_bits_per_sample()... ");
	{
		unsigned bits_per_sample = FLAC__stream_decoder_get_bits_per_sample(decoder);
		if(bits_per_sample != streaminfo_.data.stream_info.bits_per_sample) {
			printf("FAILED, returned %u, expected %u\n", bits_per_sample, streaminfo_.data.stream_info.bits_per_sample);
			return false;
		}
	}
	printf("OK\n");

	printf("testing FLAC__stream_decoder_get_sample_rate()... ");
	{
		unsigned sample_rate = FLAC__stream_decoder_get_sample_rate(decoder);
		if(sample_rate != streaminfo_.data.stream_info.sample_rate) {
			printf("FAILED, returned %u, expected %u\n", sample_rate, streaminfo_.data.stream_info.sample_rate);
			return false;
		}
	}
	printf("OK\n");

	printf("testing FLAC__stream_decoder_get_blocksize()... ");
	{
		unsigned blocksize = FLAC__stream_decoder_get_blocksize(decoder);
		/* value could be anything since we're at the last block, so accept any answer */
		printf("returned %u... OK\n", blocksize);
	}

	printf("testing FLAC__stream_decoder_get_channel_assignment()... ");
	{
		FLAC__ChannelAssignment ca = FLAC__stream_decoder_get_channel_assignment(decoder);
		printf("returned %u (%s)... OK\n", (unsigned)ca, FLAC__ChannelAssignmentString[ca]);
	}

	printf("testing FLAC__stream_decoder_reset()... ");
	if(!FLAC__stream_decoder_reset(decoder)) {
		state = FLAC__stream_decoder_get_state(decoder);
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

	printf("testing FLAC__stream_decoder_process_whole_stream()... ");
	if(!FLAC__stream_decoder_process_whole_stream(decoder)) {
		state = FLAC__stream_decoder_get_state(decoder);
		printf("FAILED, returned false, state = %u (%s)\n", state, FLAC__StreamDecoderStateString[state]);
		return false;
	}
	printf("OK\n");

	printf("testing FLAC__stream_decoder_finish()... ");
	FLAC__stream_decoder_finish(decoder);
	printf("OK\n");

	/*
	 * respond all
	 */

	printf("testing FLAC__stream_decoder_set_metadata_respond_all()... ");
	if(!FLAC__stream_decoder_set_metadata_respond_all(decoder)) {
		printf("FAILED, returned false\n");
		return false;
	}
	printf("OK\n");

	num_expected_ = 0;
	expected_metadata_sequence_[num_expected_++] = &streaminfo_;
	expected_metadata_sequence_[num_expected_++] = &padding_;
	expected_metadata_sequence_[num_expected_++] = &seektable_;
	expected_metadata_sequence_[num_expected_++] = &application1_;
	expected_metadata_sequence_[num_expected_++] = &application2_;
	expected_metadata_sequence_[num_expected_++] = &vorbiscomment_;

	if(!stream_decoder_test_respond_(decoder, &decoder_client_data))
		return 1;

	/*
	 * ignore all
	 */

	printf("testing FLAC__stream_decoder_set_metadata_ignore_all()... ");
	if(!FLAC__stream_decoder_set_metadata_ignore_all(decoder)) {
		printf("FAILED, returned false\n");
		return false;
	}
	printf("OK\n");

	num_expected_ = 0;

	if(!stream_decoder_test_respond_(decoder, &decoder_client_data))
		return 1;

	/*
	 * respond all, ignore VORBIS_COMMENT
	 */

	printf("testing FLAC__stream_decoder_set_metadata_respond_all()... ");
	if(!FLAC__stream_decoder_set_metadata_respond_all(decoder)) {
		printf("FAILED, returned false\n");
		return false;
	}
	printf("OK\n");

	printf("testing FLAC__stream_decoder_set_metadata_ignore(VORBIS_COMMENT)... ");
	if(!FLAC__stream_decoder_set_metadata_ignore(decoder, FLAC__METADATA_TYPE_VORBIS_COMMENT)) {
		printf("FAILED, returned false\n");
		return false;
	}
	printf("OK\n");

	num_expected_ = 0;
	expected_metadata_sequence_[num_expected_++] = &streaminfo_;
	expected_metadata_sequence_[num_expected_++] = &padding_;
	expected_metadata_sequence_[num_expected_++] = &seektable_;
	expected_metadata_sequence_[num_expected_++] = &application1_;
	expected_metadata_sequence_[num_expected_++] = &application2_;

	if(!stream_decoder_test_respond_(decoder, &decoder_client_data))
		return 1;

	/*
	 * respond all, ignore APPLICATION
	 */

	printf("testing FLAC__stream_decoder_set_metadata_respond_all()... ");
	if(!FLAC__stream_decoder_set_metadata_respond_all(decoder)) {
		printf("FAILED, returned false\n");
		return false;
	}
	printf("OK\n");

	printf("testing FLAC__stream_decoder_set_metadata_ignore(APPLICATION)... ");
	if(!FLAC__stream_decoder_set_metadata_ignore(decoder, FLAC__METADATA_TYPE_APPLICATION)) {
		printf("FAILED, returned false\n");
		return false;
	}
	printf("OK\n");

	num_expected_ = 0;
	expected_metadata_sequence_[num_expected_++] = &streaminfo_;
	expected_metadata_sequence_[num_expected_++] = &padding_;
	expected_metadata_sequence_[num_expected_++] = &seektable_;
	expected_metadata_sequence_[num_expected_++] = &vorbiscomment_;

	if(!stream_decoder_test_respond_(decoder, &decoder_client_data))
		return 1;

	/*
	 * respond all, ignore APPLICATION id of app#1
	 */

	printf("testing FLAC__stream_decoder_set_metadata_respond_all()... ");
	if(!FLAC__stream_decoder_set_metadata_respond_all(decoder)) {
		printf("FAILED, returned false\n");
		return false;
	}
	printf("OK\n");

	printf("testing FLAC__stream_decoder_set_metadata_ignore_application(of app block #1)... ");
	if(!FLAC__stream_decoder_set_metadata_ignore_application(decoder, application1_.data.application.id)) {
		printf("FAILED, returned false\n");
		return false;
	}
	printf("OK\n");

	num_expected_ = 0;
	expected_metadata_sequence_[num_expected_++] = &streaminfo_;
	expected_metadata_sequence_[num_expected_++] = &padding_;
	expected_metadata_sequence_[num_expected_++] = &seektable_;
	expected_metadata_sequence_[num_expected_++] = &application2_;
	expected_metadata_sequence_[num_expected_++] = &vorbiscomment_;

	if(!stream_decoder_test_respond_(decoder, &decoder_client_data))
		return 1;

	/*
	 * respond all, ignore APPLICATION id of app#1 & app#2
	 */

	printf("testing FLAC__stream_decoder_set_metadata_respond_all()... ");
	if(!FLAC__stream_decoder_set_metadata_respond_all(decoder)) {
		printf("FAILED, returned false\n");
		return false;
	}
	printf("OK\n");

	printf("testing FLAC__stream_decoder_set_metadata_ignore_application(of app block #1)... ");
	if(!FLAC__stream_decoder_set_metadata_ignore_application(decoder, application1_.data.application.id)) {
		printf("FAILED, returned false\n");
		return false;
	}
	printf("OK\n");

	printf("testing FLAC__stream_decoder_set_metadata_ignore_application(of app block #2)... ");
	if(!FLAC__stream_decoder_set_metadata_ignore_application(decoder, application2_.data.application.id)) {
		printf("FAILED, returned false\n");
		return false;
	}
	printf("OK\n");

	num_expected_ = 0;
	expected_metadata_sequence_[num_expected_++] = &streaminfo_;
	expected_metadata_sequence_[num_expected_++] = &padding_;
	expected_metadata_sequence_[num_expected_++] = &seektable_;
	expected_metadata_sequence_[num_expected_++] = &vorbiscomment_;

	if(!stream_decoder_test_respond_(decoder, &decoder_client_data))
		return 1;

	/*
	 * ignore all, respond VORBIS_COMMENT
	 */

	printf("testing FLAC__stream_decoder_set_metadata_ignore_all()... ");
	if(!FLAC__stream_decoder_set_metadata_ignore_all(decoder)) {
		printf("FAILED, returned false\n");
		return false;
	}
	printf("OK\n");

	printf("testing FLAC__stream_decoder_set_metadata_respond(VORBIS_COMMENT)... ");
	if(!FLAC__stream_decoder_set_metadata_respond(decoder, FLAC__METADATA_TYPE_VORBIS_COMMENT)) {
		printf("FAILED, returned false\n");
		return false;
	}
	printf("OK\n");

	num_expected_ = 0;
	expected_metadata_sequence_[num_expected_++] = &vorbiscomment_;

	if(!stream_decoder_test_respond_(decoder, &decoder_client_data))
		return 1;

	/*
	 * ignore all, respond APPLICATION
	 */

	printf("testing FLAC__stream_decoder_set_metadata_ignore_all()... ");
	if(!FLAC__stream_decoder_set_metadata_ignore_all(decoder)) {
		printf("FAILED, returned false\n");
		return false;
	}
	printf("OK\n");

	printf("testing FLAC__stream_decoder_set_metadata_respond(APPLICATION)... ");
	if(!FLAC__stream_decoder_set_metadata_respond(decoder, FLAC__METADATA_TYPE_APPLICATION)) {
		printf("FAILED, returned false\n");
		return false;
	}
	printf("OK\n");

	num_expected_ = 0;
	expected_metadata_sequence_[num_expected_++] = &application1_;
	expected_metadata_sequence_[num_expected_++] = &application2_;

	if(!stream_decoder_test_respond_(decoder, &decoder_client_data))
		return 1;

	/*
	 * ignore all, respond APPLICATION id of app#1
	 */

	printf("testing FLAC__stream_decoder_set_metadata_ignore_all()... ");
	if(!FLAC__stream_decoder_set_metadata_ignore_all(decoder)) {
		printf("FAILED, returned false\n");
		return false;
	}
	printf("OK\n");

	printf("testing FLAC__stream_decoder_set_metadata_respond_application(of app block #1)... ");
	if(!FLAC__stream_decoder_set_metadata_respond_application(decoder, application1_.data.application.id)) {
		printf("FAILED, returned false\n");
		return false;
	}
	printf("OK\n");

	num_expected_ = 0;
	expected_metadata_sequence_[num_expected_++] = &application1_;

	if(!stream_decoder_test_respond_(decoder, &decoder_client_data))
		return 1;

	/*
	 * ignore all, respond APPLICATION id of app#1 & app#2
	 */

	printf("testing FLAC__stream_decoder_set_metadata_ignore_all()... ");
	if(!FLAC__stream_decoder_set_metadata_ignore_all(decoder)) {
		printf("FAILED, returned false\n");
		return false;
	}
	printf("OK\n");

	printf("testing FLAC__stream_decoder_set_metadata_respond_application(of app block #1)... ");
	if(!FLAC__stream_decoder_set_metadata_respond_application(decoder, application1_.data.application.id)) {
		printf("FAILED, returned false\n");
		return false;
	}
	printf("OK\n");

	printf("testing FLAC__stream_decoder_set_metadata_respond_application(of app block #2)... ");
	if(!FLAC__stream_decoder_set_metadata_respond_application(decoder, application2_.data.application.id)) {
		printf("FAILED, returned false\n");
		return false;
	}
	printf("OK\n");

	num_expected_ = 0;
	expected_metadata_sequence_[num_expected_++] = &application1_;
	expected_metadata_sequence_[num_expected_++] = &application2_;

	if(!stream_decoder_test_respond_(decoder, &decoder_client_data))
		return 1;

	/*
	 * respond all, ignore APPLICATION, respond APPLICATION id of app#1
	 */

	printf("testing FLAC__stream_decoder_set_metadata_respond_all()... ");
	if(!FLAC__stream_decoder_set_metadata_respond_all(decoder)) {
		printf("FAILED, returned false\n");
		return false;
	}
	printf("OK\n");

	printf("testing FLAC__stream_decoder_set_metadata_ignore(APPLICATION)... ");
	if(!FLAC__stream_decoder_set_metadata_ignore(decoder, FLAC__METADATA_TYPE_APPLICATION)) {
		printf("FAILED, returned false\n");
		return false;
	}
	printf("OK\n");

	printf("testing FLAC__stream_decoder_set_metadata_respond_application(of app block #1)... ");
	if(!FLAC__stream_decoder_set_metadata_respond_application(decoder, application1_.data.application.id)) {
		printf("FAILED, returned false\n");
		return false;
	}
	printf("OK\n");

	num_expected_ = 0;
	expected_metadata_sequence_[num_expected_++] = &streaminfo_;
	expected_metadata_sequence_[num_expected_++] = &padding_;
	expected_metadata_sequence_[num_expected_++] = &seektable_;
	expected_metadata_sequence_[num_expected_++] = &application1_;
	expected_metadata_sequence_[num_expected_++] = &vorbiscomment_;

	if(!stream_decoder_test_respond_(decoder, &decoder_client_data))
		return 1;

	/*
	 * ignore all, respond APPLICATION, ignore APPLICATION id of app#1
	 */

	printf("testing FLAC__stream_decoder_set_metadata_ignore_all()... ");
	if(!FLAC__stream_decoder_set_metadata_ignore_all(decoder)) {
		printf("FAILED, returned false\n");
		return false;
	}
	printf("OK\n");

	printf("testing FLAC__stream_decoder_set_metadata_respond(APPLICATION)... ");
	if(!FLAC__stream_decoder_set_metadata_respond(decoder, FLAC__METADATA_TYPE_APPLICATION)) {
		printf("FAILED, returned false\n");
		return false;
	}
	printf("OK\n");

	printf("testing FLAC__stream_decoder_set_metadata_ignore_application(of app block #1)... ");
	if(!FLAC__stream_decoder_set_metadata_ignore_application(decoder, application1_.data.application.id)) {
		printf("FAILED, returned false\n");
		return false;
	}
	printf("OK\n");

	num_expected_ = 0;
	expected_metadata_sequence_[num_expected_++] = &application2_;

	if(!stream_decoder_test_respond_(decoder, &decoder_client_data))
		return 1;

	/* done, now leave the sequence the way we found it... */
	num_expected_ = 0;
	expected_metadata_sequence_[num_expected_++] = &streaminfo_;
	expected_metadata_sequence_[num_expected_++] = &padding_;
	expected_metadata_sequence_[num_expected_++] = &seektable_;
	expected_metadata_sequence_[num_expected_++] = &application1_;
	expected_metadata_sequence_[num_expected_++] = &application2_;
	expected_metadata_sequence_[num_expected_++] = &vorbiscomment_;

	printf("testing FLAC__stream_decoder_delete()... ");
	FLAC__stream_decoder_delete(decoder);
	printf("OK\n");

	fclose(decoder_client_data.file);

	printf("\nPASSED!\n");

	return true;
}

static FLAC__SeekableStreamDecoderReadStatus seekable_stream_decoder_read_callback_(const FLAC__SeekableStreamDecoder *decoder, FLAC__byte buffer[], unsigned *bytes, void *client_data)
{
	(void)decoder;
	switch(stream_decoder_read_callback_(0, buffer, bytes, client_data)) {
		case FLAC__STREAM_DECODER_READ_STATUS_CONTINUE:
			return FLAC__SEEKABLE_STREAM_DECODER_READ_STATUS_OK;
		case FLAC__STREAM_DECODER_READ_STATUS_ABORT:
		case FLAC__STREAM_DECODER_READ_STATUS_END_OF_STREAM:
			return FLAC__SEEKABLE_STREAM_DECODER_READ_STATUS_ERROR;
		default:
			FLAC__ASSERT(0);
			return FLAC__SEEKABLE_STREAM_DECODER_READ_STATUS_ERROR;
	}
}

static FLAC__SeekableStreamDecoderSeekStatus seekable_stream_decoder_seek_callback_(const FLAC__SeekableStreamDecoder *decoder, FLAC__uint64 absolute_byte_offset, void *client_data)
{
	seekable_stream_decoder_client_data_struct *dcd = (seekable_stream_decoder_client_data_struct*)client_data;

	(void)decoder;

	if(0 == dcd) {
		printf("ERROR: client_data in seek callback is NULL\n");
		return FLAC__SEEKABLE_STREAM_DECODER_SEEK_STATUS_ERROR;
	}

	if(dcd->error_occurred)
		return FLAC__SEEKABLE_STREAM_DECODER_SEEK_STATUS_ERROR;

	if(fseek(dcd->file, absolute_byte_offset, SEEK_SET) < 0) {
		dcd->error_occurred = true;
		return FLAC__SEEKABLE_STREAM_DECODER_SEEK_STATUS_ERROR;
	}

	return FLAC__SEEKABLE_STREAM_DECODER_SEEK_STATUS_OK;
}

static FLAC__SeekableStreamDecoderTellStatus seekable_stream_decoder_tell_callback_(const FLAC__SeekableStreamDecoder *decoder, FLAC__uint64 *absolute_byte_offset, void *client_data)
{
	seekable_stream_decoder_client_data_struct *dcd = (seekable_stream_decoder_client_data_struct*)client_data;
	long offset;

	(void)decoder;

	if(0 == dcd) {
		printf("ERROR: client_data in tell callback is NULL\n");
		return FLAC__SEEKABLE_STREAM_DECODER_TELL_STATUS_ERROR;
	}

	if(dcd->error_occurred)
		return FLAC__SEEKABLE_STREAM_DECODER_TELL_STATUS_ERROR;

	offset = ftell(dcd->file);
	*absolute_byte_offset = (FLAC__uint64)offset;

	if(offset < 0) {
		dcd->error_occurred = true;
		return FLAC__SEEKABLE_STREAM_DECODER_TELL_STATUS_ERROR;
	}

	return FLAC__SEEKABLE_STREAM_DECODER_TELL_STATUS_OK;
}

static FLAC__SeekableStreamDecoderLengthStatus seekable_stream_decoder_length_callback_(const FLAC__SeekableStreamDecoder *decoder, FLAC__uint64 *stream_length, void *client_data)
{
	seekable_stream_decoder_client_data_struct *dcd = (seekable_stream_decoder_client_data_struct*)client_data;

	(void)decoder;

	if(0 == dcd) {
		printf("ERROR: client_data in length callback is NULL\n");
		return FLAC__SEEKABLE_STREAM_DECODER_LENGTH_STATUS_ERROR;
	}

	if(dcd->error_occurred)
		return FLAC__SEEKABLE_STREAM_DECODER_LENGTH_STATUS_ERROR;

	*stream_length = (FLAC__uint64)flacfilesize_;
	return FLAC__SEEKABLE_STREAM_DECODER_LENGTH_STATUS_OK;
}

static FLAC__bool seekable_stream_decoder_eof_callback_(const FLAC__SeekableStreamDecoder *decoder, void *client_data)
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

static FLAC__StreamDecoderWriteStatus seekable_stream_decoder_write_callback_(const FLAC__SeekableStreamDecoder *decoder, const FLAC__Frame *frame, const FLAC__int32 *buffer[], void *client_data)
{
	(void)decoder;
	return stream_decoder_write_callback_(0, frame, buffer, client_data);
}

static void seekable_stream_decoder_metadata_callback_(const FLAC__SeekableStreamDecoder *decoder, const FLAC__StreamMetaData *metadata, void *client_data)
{
	(void)decoder;
	stream_decoder_metadata_callback_(0, metadata, client_data);
}

static void seekable_stream_decoder_error_callback_(const FLAC__SeekableStreamDecoder *decoder, FLAC__StreamDecoderErrorStatus status, void *client_data)
{
	(void)decoder;
	stream_decoder_error_callback_(0, status, client_data);
}

static FLAC__bool seekable_stream_decoder_test_respond_(FLAC__SeekableStreamDecoder *decoder, seekable_stream_decoder_client_data_struct *dcd)
{
	FLAC__SeekableStreamDecoderState state;

	if(!FLAC__seekable_stream_decoder_set_read_callback(decoder, seekable_stream_decoder_read_callback_)) {
		printf("FAILED at FLAC__seekable_stream_decoder_set_read_callback(), returned false\n");
		return false;
	}

	if(!FLAC__seekable_stream_decoder_set_seek_callback(decoder, seekable_stream_decoder_seek_callback_)) {
		printf("FAILED at FLAC__seekable_stream_decoder_set_seek_callback(), returned false\n");
		return false;
	}

	if(!FLAC__seekable_stream_decoder_set_tell_callback(decoder, seekable_stream_decoder_tell_callback_)) {
		printf("FAILED at FLAC__seekable_stream_decoder_set_tell_callback(), returned false\n");
		return false;
	}

	if(!FLAC__seekable_stream_decoder_set_length_callback(decoder, seekable_stream_decoder_length_callback_)) {
		printf("FAILED at FLAC__seekable_stream_decoder_set_length_callback(), returned false\n");
		return false;
	}

	if(!FLAC__seekable_stream_decoder_set_eof_callback(decoder, seekable_stream_decoder_eof_callback_)) {
		printf("FAILED at FLAC__seekable_stream_decoder_set_eof_callback(), returned false\n");
		return false;
	}

	if(!FLAC__seekable_stream_decoder_set_write_callback(decoder, seekable_stream_decoder_write_callback_)) {
		printf("FAILED at FLAC__seekable_stream_decoder_set_write_callback(), returned false\n");
		return false;
	}

	if(!FLAC__seekable_stream_decoder_set_metadata_callback(decoder, seekable_stream_decoder_metadata_callback_)) {
		printf("FAILED at FLAC__seekable_stream_decoder_set_metadata_callback(), returned false\n");
		return false;
	}

	if(!FLAC__seekable_stream_decoder_set_error_callback(decoder, seekable_stream_decoder_error_callback_)) {
		printf("FAILED at FLAC__seekable_stream_decoder_set_error_callback(), returned false\n");
		return false;
	}

	if(!FLAC__seekable_stream_decoder_set_client_data(decoder, dcd)) {
		printf("FAILED at FLAC__seekable_stream_decoder_set_client_data(), returned false\n");
		return false;
	}

	if(!FLAC__seekable_stream_decoder_set_md5_checking(decoder, true)) {
		printf("FAILED at FLAC__seekable_stream_decoder_set_md5_checking(), returned false\n");
		return false;
	}

	printf("testing FLAC__seekable_stream_decoder_init()... ");
	if((state = FLAC__seekable_stream_decoder_init(decoder)) != FLAC__SEEKABLE_STREAM_DECODER_OK) {
		printf("FAILED, returned state = %u (%s)\n", state, FLAC__SeekableStreamDecoderStateString[state]);
		return false;
	}
	printf("OK\n");

	dcd->current_metadata_number = 0;

	if(fseek(dcd->file, 0, SEEK_SET) < 0) {
		printf("FAILED rewinding input, errno = %d\n", errno);
		return false;
	}

	printf("testing FLAC__seekable_stream_decoder_process_whole_stream()... ");
	if(!FLAC__seekable_stream_decoder_process_whole_stream(decoder)) {
		state = FLAC__seekable_stream_decoder_get_state(decoder);
		printf("FAILED, returned false, state = %u (%s)\n", state, FLAC__SeekableStreamDecoderStateString[state]);
		return false;
	}
	printf("OK\n");

	printf("testing FLAC__seekable_stream_decoder_finish()... ");
	(void) FLAC__seekable_stream_decoder_finish(decoder);
	printf("OK\n");

	return true;;
}

FLAC__bool test_seekable_stream_decoder()
{
	FLAC__SeekableStreamDecoder *decoder;
	FLAC__SeekableStreamDecoderState state;
	seekable_stream_decoder_client_data_struct decoder_client_data;

	printf("\n+++ unit test: FLAC__SeekableStreamDecoder\n\n");

	num_expected_ = 0;
	expected_metadata_sequence_[num_expected_++] = &streaminfo_;

	printf("testing FLAC__seekable_stream_decoder_new()... ");
	decoder = FLAC__seekable_stream_decoder_new();
	if(0 == decoder) {
		printf("FAILED, returned NULL\n");
		return false;
	}
	printf("OK\n");

	printf("testing FLAC__seekable_stream_decoder_set_read_callback()... ");
	if(!FLAC__seekable_stream_decoder_set_read_callback(decoder, seekable_stream_decoder_read_callback_)) {
		printf("FAILED, returned false\n");
		return false;
	}
	printf("OK\n");

	printf("testing FLAC__seekable_stream_decoder_set_seek_callback()... ");
	if(!FLAC__seekable_stream_decoder_set_seek_callback(decoder, seekable_stream_decoder_seek_callback_)) {
		printf("FAILED, returned false\n");
		return false;
	}
	printf("OK\n");

	printf("testing FLAC__seekable_stream_decoder_set_tell_callback()... ");
	if(!FLAC__seekable_stream_decoder_set_tell_callback(decoder, seekable_stream_decoder_tell_callback_)) {
		printf("FAILED, returned false\n");
		return false;
	}
	printf("OK\n");

	printf("testing FLAC__seekable_stream_decoder_set_length_callback()... ");
	if(!FLAC__seekable_stream_decoder_set_length_callback(decoder, seekable_stream_decoder_length_callback_)) {
		printf("FAILED, returned false\n");
		return false;
	}
	printf("OK\n");

	printf("testing FLAC__seekable_stream_decoder_set_eof_callback()... ");
	if(!FLAC__seekable_stream_decoder_set_eof_callback(decoder, seekable_stream_decoder_eof_callback_)) {
		printf("FAILED, returned false\n");
		return false;
	}
	printf("OK\n");

	printf("testing FLAC__seekable_stream_decoder_set_write_callback()... ");
	if(!FLAC__seekable_stream_decoder_set_write_callback(decoder, seekable_stream_decoder_write_callback_)) {
		printf("FAILED, returned false\n");
		return false;
	}
	printf("OK\n");

	printf("testing FLAC__seekable_stream_decoder_set_metadata_callback()... ");
	if(!FLAC__seekable_stream_decoder_set_metadata_callback(decoder, seekable_stream_decoder_metadata_callback_)) {
		printf("FAILED, returned false\n");
		return false;
	}
	printf("OK\n");

	printf("testing FLAC__seekable_stream_decoder_set_error_callback()... ");
	if(!FLAC__seekable_stream_decoder_set_error_callback(decoder, seekable_stream_decoder_error_callback_)) {
		printf("FAILED, returned false\n");
		return false;
	}
	printf("OK\n");

	printf("testing FLAC__seekable_stream_decoder_set_client_data()... ");
	if(!FLAC__seekable_stream_decoder_set_client_data(decoder, &decoder_client_data)) {
		printf("FAILED, returned false\n");
		return false;
	}
	printf("OK\n");

	printf("testing FLAC__seekable_stream_decoder_set_md5_checking()... ");
	if(!FLAC__seekable_stream_decoder_set_md5_checking(decoder, true)) {
		printf("FAILED, returned false\n");
		return false;
	}
	printf("OK\n");

	printf("testing FLAC__seekable_stream_decoder_init()... ");
	if((state = FLAC__seekable_stream_decoder_init(decoder)) != FLAC__SEEKABLE_STREAM_DECODER_OK) {
		printf("FAILED, returned state = %u (%s)\n", state, FLAC__SeekableStreamDecoderStateString[state]);
		return false;
	}
	printf("OK\n");

	printf("testing FLAC__seekable_stream_decoder_get_state()... ");
	state = FLAC__seekable_stream_decoder_get_state(decoder);
	printf("returned state = %u (%s)... OK\n", state, FLAC__SeekableStreamDecoderStateString[state]);

	decoder_client_data.current_metadata_number = 0;
	decoder_client_data.ignore_errors = false;
	decoder_client_data.error_occurred = false;

	printf("opening FLAC file... ");
	decoder_client_data.file = fopen(flacfilename_, "rb");
	if(0 == decoder_client_data.file) {
		printf("ERROR\n");
		return false;
	}
	printf("OK\n");

	printf("testing FLAC__seekable_stream_decoder_get_md5_checking()... ");
	if(!FLAC__seekable_stream_decoder_get_md5_checking(decoder)) {
		printf("FAILED, returned false, expected true\n");
		return false;
	}
	printf("OK\n");

	printf("testing FLAC__seekable_stream_decoder_process_metadata()... ");
	if(!FLAC__seekable_stream_decoder_process_metadata(decoder)) {
		state = FLAC__seekable_stream_decoder_get_state(decoder);
		printf("FAILED, returned false, state = %u (%s)\n", state, FLAC__SeekableStreamDecoderStateString[state]);
		return false;
	}
	printf("OK\n");

	printf("testing FLAC__seekable_stream_decoder_process_one_frame()... ");
	if(!FLAC__seekable_stream_decoder_process_one_frame(decoder)) {
		state = FLAC__seekable_stream_decoder_get_state(decoder);
		printf("FAILED, returned false, state = %u (%s)\n", state, FLAC__SeekableStreamDecoderStateString[state]);
		return false;
	}
	printf("OK\n");

	printf("testing FLAC__seekable_stream_decoder_flush()... ");
	if(!FLAC__seekable_stream_decoder_flush(decoder)) {
		state = FLAC__seekable_stream_decoder_get_state(decoder);
		printf("FAILED, returned false, state = %u (%s)\n", state, FLAC__SeekableStreamDecoderStateString[state]);
		return false;
	}
	printf("OK\n");

	decoder_client_data.ignore_errors = true;
	printf("testing FLAC__seekable_stream_decoder_process_one_frame()... ");
	if(!FLAC__seekable_stream_decoder_process_one_frame(decoder)) {
		state = FLAC__seekable_stream_decoder_get_state(decoder);
		printf("FAILED, returned false, state = %u (%s)\n", state, FLAC__SeekableStreamDecoderStateString[state]);
		return false;
	}
	printf("OK\n");
	decoder_client_data.ignore_errors = false;

	printf("testing FLAC__seekable_stream_decoder_seek_absolute()... ");
	if(!FLAC__seekable_stream_decoder_seek_absolute(decoder, 0)) {
		state = FLAC__seekable_stream_decoder_get_state(decoder);
		printf("FAILED, returned false, state = %u (%s)\n", state, FLAC__SeekableStreamDecoderStateString[state]);
		return false;
	}
	printf("OK\n");

	printf("testing FLAC__seekable_stream_decoder_process_remaining_frames()... ");
	if(!FLAC__seekable_stream_decoder_process_remaining_frames(decoder)) {
		state = FLAC__seekable_stream_decoder_get_state(decoder);
		printf("FAILED, returned false, state = %u (%s)\n", state, FLAC__SeekableStreamDecoderStateString[state]);
		return false;
	}
	printf("OK\n");

	printf("testing FLAC__seekable_stream_decoder_get_channels()... ");
	{
		unsigned channels = FLAC__seekable_stream_decoder_get_channels(decoder);
		if(channels != streaminfo_.data.stream_info.channels) {
			printf("FAILED, returned %u, expected %u\n", channels, streaminfo_.data.stream_info.channels);
			return false;
		}
	}
	printf("OK\n");

	printf("testing FLAC__seekable_stream_decoder_get_bits_per_sample()... ");
	{
		unsigned bits_per_sample = FLAC__seekable_stream_decoder_get_bits_per_sample(decoder);
		if(bits_per_sample != streaminfo_.data.stream_info.bits_per_sample) {
			printf("FAILED, returned %u, expected %u\n", bits_per_sample, streaminfo_.data.stream_info.bits_per_sample);
			return false;
		}
	}
	printf("OK\n");

	printf("testing FLAC__seekable_stream_decoder_get_sample_rate()... ");
	{
		unsigned sample_rate = FLAC__seekable_stream_decoder_get_sample_rate(decoder);
		if(sample_rate != streaminfo_.data.stream_info.sample_rate) {
			printf("FAILED, returned %u, expected %u\n", sample_rate, streaminfo_.data.stream_info.sample_rate);
			return false;
		}
	}
	printf("OK\n");

	printf("testing FLAC__seekable_stream_decoder_get_blocksize()... ");
	{
		unsigned blocksize = FLAC__seekable_stream_decoder_get_blocksize(decoder);
		/* value could be anything since we're at the last block, so accept any answer */
		printf("returned %u... OK\n", blocksize);
	}

	printf("testing FLAC__seekable_stream_decoder_get_channel_assignment()... ");
	{
		FLAC__ChannelAssignment ca = FLAC__seekable_stream_decoder_get_channel_assignment(decoder);
		printf("returned %u (%s)... OK\n", (unsigned)ca, FLAC__ChannelAssignmentString[ca]);
	}

	printf("testing FLAC__seekable_stream_decoder_reset()... ");
	if(!FLAC__seekable_stream_decoder_reset(decoder)) {
		state = FLAC__seekable_stream_decoder_get_state(decoder);
		printf("FAILED, returned false, state = %u (%s)\n", state, FLAC__SeekableStreamDecoderStateString[state]);
		return false;
	}
	printf("OK\n");

	decoder_client_data.current_metadata_number = 0;

	if(fseek(decoder_client_data.file, 0, SEEK_SET) < 0) {
		printf("FAILED rewinding input, errno = %d\n", errno);
		return false;
	}

	printf("testing FLAC__seekable_stream_decoder_process_whole_stream()... ");
	if(!FLAC__seekable_stream_decoder_process_whole_stream(decoder)) {
		state = FLAC__seekable_stream_decoder_get_state(decoder);
		printf("FAILED, returned false, state = %u (%s)\n", state, FLAC__SeekableStreamDecoderStateString[state]);
		return false;
	}
	printf("OK\n");

	printf("testing FLAC__seekable_stream_decoder_finish()... ");
	(void) FLAC__seekable_stream_decoder_finish(decoder);
	printf("OK\n");

	/*
	 * respond all
	 */

	printf("testing FLAC__seekable_stream_decoder_set_metadata_respond_all()... ");
	if(!FLAC__seekable_stream_decoder_set_metadata_respond_all(decoder)) {
		printf("FAILED, returned false\n");
		return false;
	}
	printf("OK\n");

	num_expected_ = 0;
	expected_metadata_sequence_[num_expected_++] = &streaminfo_;
	expected_metadata_sequence_[num_expected_++] = &padding_;
	expected_metadata_sequence_[num_expected_++] = &seektable_;
	expected_metadata_sequence_[num_expected_++] = &application1_;
	expected_metadata_sequence_[num_expected_++] = &application2_;
	expected_metadata_sequence_[num_expected_++] = &vorbiscomment_;

	if(!seekable_stream_decoder_test_respond_(decoder, &decoder_client_data))
		return 1;

	/*
	 * ignore all
	 */

	printf("testing FLAC__seekable_stream_decoder_set_metadata_ignore_all()... ");
	if(!FLAC__seekable_stream_decoder_set_metadata_ignore_all(decoder)) {
		printf("FAILED, returned false\n");
		return false;
	}
	printf("OK\n");

	num_expected_ = 0;

	if(!seekable_stream_decoder_test_respond_(decoder, &decoder_client_data))
		return 1;

	/*
	 * respond all, ignore VORBIS_COMMENT
	 */

	printf("testing FLAC__seekable_stream_decoder_set_metadata_respond_all()... ");
	if(!FLAC__seekable_stream_decoder_set_metadata_respond_all(decoder)) {
		printf("FAILED, returned false\n");
		return false;
	}
	printf("OK\n");

	printf("testing FLAC__seekable_stream_decoder_set_metadata_ignore(VORBIS_COMMENT)... ");
	if(!FLAC__seekable_stream_decoder_set_metadata_ignore(decoder, FLAC__METADATA_TYPE_VORBIS_COMMENT)) {
		printf("FAILED, returned false\n");
		return false;
	}
	printf("OK\n");

	num_expected_ = 0;
	expected_metadata_sequence_[num_expected_++] = &streaminfo_;
	expected_metadata_sequence_[num_expected_++] = &padding_;
	expected_metadata_sequence_[num_expected_++] = &seektable_;
	expected_metadata_sequence_[num_expected_++] = &application1_;
	expected_metadata_sequence_[num_expected_++] = &application2_;

	if(!seekable_stream_decoder_test_respond_(decoder, &decoder_client_data))
		return 1;

	/*
	 * respond all, ignore APPLICATION
	 */

	printf("testing FLAC__seekable_stream_decoder_set_metadata_respond_all()... ");
	if(!FLAC__seekable_stream_decoder_set_metadata_respond_all(decoder)) {
		printf("FAILED, returned false\n");
		return false;
	}
	printf("OK\n");

	printf("testing FLAC__seekable_stream_decoder_set_metadata_ignore(APPLICATION)... ");
	if(!FLAC__seekable_stream_decoder_set_metadata_ignore(decoder, FLAC__METADATA_TYPE_APPLICATION)) {
		printf("FAILED, returned false\n");
		return false;
	}
	printf("OK\n");

	num_expected_ = 0;
	expected_metadata_sequence_[num_expected_++] = &streaminfo_;
	expected_metadata_sequence_[num_expected_++] = &padding_;
	expected_metadata_sequence_[num_expected_++] = &seektable_;
	expected_metadata_sequence_[num_expected_++] = &vorbiscomment_;

	if(!seekable_stream_decoder_test_respond_(decoder, &decoder_client_data))
		return 1;

	/*
	 * respond all, ignore APPLICATION id of app#1
	 */

	printf("testing FLAC__seekable_stream_decoder_set_metadata_respond_all()... ");
	if(!FLAC__seekable_stream_decoder_set_metadata_respond_all(decoder)) {
		printf("FAILED, returned false\n");
		return false;
	}
	printf("OK\n");

	printf("testing FLAC__seekable_stream_decoder_set_metadata_ignore_application(of app block #1)... ");
	if(!FLAC__seekable_stream_decoder_set_metadata_ignore_application(decoder, application1_.data.application.id)) {
		printf("FAILED, returned false\n");
		return false;
	}
	printf("OK\n");

	num_expected_ = 0;
	expected_metadata_sequence_[num_expected_++] = &streaminfo_;
	expected_metadata_sequence_[num_expected_++] = &padding_;
	expected_metadata_sequence_[num_expected_++] = &seektable_;
	expected_metadata_sequence_[num_expected_++] = &application2_;
	expected_metadata_sequence_[num_expected_++] = &vorbiscomment_;

	if(!seekable_stream_decoder_test_respond_(decoder, &decoder_client_data))
		return 1;

	/*
	 * respond all, ignore APPLICATION id of app#1 & app#2
	 */

	printf("testing FLAC__seekable_stream_decoder_set_metadata_respond_all()... ");
	if(!FLAC__seekable_stream_decoder_set_metadata_respond_all(decoder)) {
		printf("FAILED, returned false\n");
		return false;
	}
	printf("OK\n");

	printf("testing FLAC__seekable_stream_decoder_set_metadata_ignore_application(of app block #1)... ");
	if(!FLAC__seekable_stream_decoder_set_metadata_ignore_application(decoder, application1_.data.application.id)) {
		printf("FAILED, returned false\n");
		return false;
	}
	printf("OK\n");

	printf("testing FLAC__seekable_stream_decoder_set_metadata_ignore_application(of app block #2)... ");
	if(!FLAC__seekable_stream_decoder_set_metadata_ignore_application(decoder, application2_.data.application.id)) {
		printf("FAILED, returned false\n");
		return false;
	}
	printf("OK\n");

	num_expected_ = 0;
	expected_metadata_sequence_[num_expected_++] = &streaminfo_;
	expected_metadata_sequence_[num_expected_++] = &padding_;
	expected_metadata_sequence_[num_expected_++] = &seektable_;
	expected_metadata_sequence_[num_expected_++] = &vorbiscomment_;

	if(!seekable_stream_decoder_test_respond_(decoder, &decoder_client_data))
		return 1;

	/*
	 * ignore all, respond VORBIS_COMMENT
	 */

	printf("testing FLAC__seekable_stream_decoder_set_metadata_ignore_all()... ");
	if(!FLAC__seekable_stream_decoder_set_metadata_ignore_all(decoder)) {
		printf("FAILED, returned false\n");
		return false;
	}
	printf("OK\n");

	printf("testing FLAC__seekable_stream_decoder_set_metadata_respond(VORBIS_COMMENT)... ");
	if(!FLAC__seekable_stream_decoder_set_metadata_respond(decoder, FLAC__METADATA_TYPE_VORBIS_COMMENT)) {
		printf("FAILED, returned false\n");
		return false;
	}
	printf("OK\n");

	num_expected_ = 0;
	expected_metadata_sequence_[num_expected_++] = &vorbiscomment_;

	if(!seekable_stream_decoder_test_respond_(decoder, &decoder_client_data))
		return 1;

	/*
	 * ignore all, respond APPLICATION
	 */

	printf("testing FLAC__seekable_stream_decoder_set_metadata_ignore_all()... ");
	if(!FLAC__seekable_stream_decoder_set_metadata_ignore_all(decoder)) {
		printf("FAILED, returned false\n");
		return false;
	}
	printf("OK\n");

	printf("testing FLAC__seekable_stream_decoder_set_metadata_respond(APPLICATION)... ");
	if(!FLAC__seekable_stream_decoder_set_metadata_respond(decoder, FLAC__METADATA_TYPE_APPLICATION)) {
		printf("FAILED, returned false\n");
		return false;
	}
	printf("OK\n");

	num_expected_ = 0;
	expected_metadata_sequence_[num_expected_++] = &application1_;
	expected_metadata_sequence_[num_expected_++] = &application2_;

	if(!seekable_stream_decoder_test_respond_(decoder, &decoder_client_data))
		return 1;

	/*
	 * ignore all, respond APPLICATION id of app#1
	 */

	printf("testing FLAC__seekable_stream_decoder_set_metadata_ignore_all()... ");
	if(!FLAC__seekable_stream_decoder_set_metadata_ignore_all(decoder)) {
		printf("FAILED, returned false\n");
		return false;
	}
	printf("OK\n");

	printf("testing FLAC__seekable_stream_decoder_set_metadata_respond_application(of app block #1)... ");
	if(!FLAC__seekable_stream_decoder_set_metadata_respond_application(decoder, application1_.data.application.id)) {
		printf("FAILED, returned false\n");
		return false;
	}
	printf("OK\n");

	num_expected_ = 0;
	expected_metadata_sequence_[num_expected_++] = &application1_;

	if(!seekable_stream_decoder_test_respond_(decoder, &decoder_client_data))
		return 1;

	/*
	 * ignore all, respond APPLICATION id of app#1 & app#2
	 */

	printf("testing FLAC__seekable_stream_decoder_set_metadata_ignore_all()... ");
	if(!FLAC__seekable_stream_decoder_set_metadata_ignore_all(decoder)) {
		printf("FAILED, returned false\n");
		return false;
	}
	printf("OK\n");

	printf("testing FLAC__seekable_stream_decoder_set_metadata_respond_application(of app block #1)... ");
	if(!FLAC__seekable_stream_decoder_set_metadata_respond_application(decoder, application1_.data.application.id)) {
		printf("FAILED, returned false\n");
		return false;
	}
	printf("OK\n");

	printf("testing FLAC__seekable_stream_decoder_set_metadata_respond_application(of app block #2)... ");
	if(!FLAC__seekable_stream_decoder_set_metadata_respond_application(decoder, application2_.data.application.id)) {
		printf("FAILED, returned false\n");
		return false;
	}
	printf("OK\n");

	num_expected_ = 0;
	expected_metadata_sequence_[num_expected_++] = &application1_;
	expected_metadata_sequence_[num_expected_++] = &application2_;

	if(!seekable_stream_decoder_test_respond_(decoder, &decoder_client_data))
		return 1;

	/*
	 * respond all, ignore APPLICATION, respond APPLICATION id of app#1
	 */

	printf("testing FLAC__seekable_stream_decoder_set_metadata_respond_all()... ");
	if(!FLAC__seekable_stream_decoder_set_metadata_respond_all(decoder)) {
		printf("FAILED, returned false\n");
		return false;
	}
	printf("OK\n");

	printf("testing FLAC__seekable_stream_decoder_set_metadata_ignore(APPLICATION)... ");
	if(!FLAC__seekable_stream_decoder_set_metadata_ignore(decoder, FLAC__METADATA_TYPE_APPLICATION)) {
		printf("FAILED, returned false\n");
		return false;
	}
	printf("OK\n");

	printf("testing FLAC__seekable_stream_decoder_set_metadata_respond_application(of app block #1)... ");
	if(!FLAC__seekable_stream_decoder_set_metadata_respond_application(decoder, application1_.data.application.id)) {
		printf("FAILED, returned false\n");
		return false;
	}
	printf("OK\n");

	num_expected_ = 0;
	expected_metadata_sequence_[num_expected_++] = &streaminfo_;
	expected_metadata_sequence_[num_expected_++] = &padding_;
	expected_metadata_sequence_[num_expected_++] = &seektable_;
	expected_metadata_sequence_[num_expected_++] = &application1_;
	expected_metadata_sequence_[num_expected_++] = &vorbiscomment_;

	if(!seekable_stream_decoder_test_respond_(decoder, &decoder_client_data))
		return 1;

	/*
	 * ignore all, respond APPLICATION, ignore APPLICATION id of app#1
	 */

	printf("testing FLAC__seekable_stream_decoder_set_metadata_ignore_all()... ");
	if(!FLAC__seekable_stream_decoder_set_metadata_ignore_all(decoder)) {
		printf("FAILED, returned false\n");
		return false;
	}
	printf("OK\n");

	printf("testing FLAC__seekable_stream_decoder_set_metadata_respond(APPLICATION)... ");
	if(!FLAC__seekable_stream_decoder_set_metadata_respond(decoder, FLAC__METADATA_TYPE_APPLICATION)) {
		printf("FAILED, returned false\n");
		return false;
	}
	printf("OK\n");

	printf("testing FLAC__seekable_stream_decoder_set_metadata_ignore_application(of app block #1)... ");
	if(!FLAC__seekable_stream_decoder_set_metadata_ignore_application(decoder, application1_.data.application.id)) {
		printf("FAILED, returned false\n");
		return false;
	}
	printf("OK\n");

	num_expected_ = 0;
	expected_metadata_sequence_[num_expected_++] = &application2_;

	if(!seekable_stream_decoder_test_respond_(decoder, &decoder_client_data))
		return 1;

	/* done, now leave the sequence the way we found it... */
	num_expected_ = 0;
	expected_metadata_sequence_[num_expected_++] = &streaminfo_;
	expected_metadata_sequence_[num_expected_++] = &padding_;
	expected_metadata_sequence_[num_expected_++] = &seektable_;
	expected_metadata_sequence_[num_expected_++] = &application1_;
	expected_metadata_sequence_[num_expected_++] = &application2_;
	expected_metadata_sequence_[num_expected_++] = &vorbiscomment_;

	printf("testing FLAC__seekable_stream_decoder_delete()... ");
	FLAC__seekable_stream_decoder_delete(decoder);
	printf("OK\n");

	fclose(decoder_client_data.file);

	printf("\nPASSED!\n");

	return true;
}

static FLAC__StreamDecoderWriteStatus file_decoder_write_callback_(const FLAC__FileDecoder *decoder, const FLAC__Frame *frame, const FLAC__int32 *buffer[], void *client_data)
{
	(void)decoder;
	return stream_decoder_write_callback_(0, frame, buffer, client_data);
}

static void file_decoder_metadata_callback_(const FLAC__FileDecoder *decoder, const FLAC__StreamMetaData *metadata, void *client_data)
{
	(void)decoder;
	stream_decoder_metadata_callback_(0, metadata, client_data);
}

static void file_decoder_error_callback_(const FLAC__FileDecoder *decoder, FLAC__StreamDecoderErrorStatus status, void *client_data)
{
	(void)decoder;
	stream_decoder_error_callback_(0, status, client_data);
}

static FLAC__bool file_decoder_test_respond_(FLAC__FileDecoder *decoder, file_decoder_client_data_struct *dcd)
{
	FLAC__FileDecoderState state;

	if(!FLAC__file_decoder_set_write_callback(decoder, file_decoder_write_callback_)) {
		printf("FAILED at FLAC__file_decoder_set_write_callback(), returned false\n");
		return false;
	}

	if(!FLAC__file_decoder_set_metadata_callback(decoder, file_decoder_metadata_callback_)) {
		printf("FAILED at FLAC__file_decoder_set_metadata_callback(), returned false\n");
		return false;
	}

	if(!FLAC__file_decoder_set_error_callback(decoder, file_decoder_error_callback_)) {
		printf("FAILED at FLAC__file_decoder_set_error_callback(), returned false\n");
		return false;
	}

	if(!FLAC__file_decoder_set_client_data(decoder, dcd)) {
		printf("FAILED at FLAC__file_decoder_set_client_data(), returned false\n");
		return false;
	}

	if(!FLAC__file_decoder_set_filename(decoder, flacfilename_)) {
		printf("FAILED at FLAC__file_decoder_set_filename(), returned false\n");
		return false;
	}

	if(!FLAC__file_decoder_set_md5_checking(decoder, true)) {
		printf("FAILED at FLAC__file_decoder_set_md5_checking(), returned false\n");
		return false;
	}

	printf("testing FLAC__file_decoder_init()... ");
	if((state = FLAC__file_decoder_init(decoder)) != FLAC__FILE_DECODER_OK) {
		printf("FAILED, returned state = %u (%s)\n", state, FLAC__FileDecoderStateString[state]);
		return false;
	}
	printf("OK\n");

	dcd->current_metadata_number = 0;

	printf("testing FLAC__file_decoder_process_whole_file()... ");
	if(!FLAC__file_decoder_process_whole_file(decoder)) {
		state = FLAC__file_decoder_get_state(decoder);
		printf("FAILED, returned false, state = %u (%s)\n", state, FLAC__FileDecoderStateString[state]);
		return false;
	}
	printf("OK\n");

	printf("testing FLAC__file_decoder_finish()... ");
	(void) FLAC__file_decoder_finish(decoder);
	printf("OK\n");

	return true;;
}

FLAC__bool test_file_decoder()
{
	FLAC__FileDecoder *decoder;
	FLAC__FileDecoderState state;
	seekable_stream_decoder_client_data_struct decoder_client_data;

	printf("\n+++ unit test: FLAC__FileDecoder\n\n");

	num_expected_ = 0;
	expected_metadata_sequence_[num_expected_++] = &streaminfo_;

	printf("testing FLAC__file_decoder_new()... ");
	decoder = FLAC__file_decoder_new();
	if(0 == decoder) {
		printf("FAILED, returned NULL\n");
		return false;
	}
	printf("OK\n");

	printf("testing FLAC__file_decoder_set_write_callback()... ");
	if(!FLAC__file_decoder_set_write_callback(decoder, file_decoder_write_callback_)) {
		printf("FAILED, returned false\n");
		return false;
	}
	printf("OK\n");

	printf("testing FLAC__file_decoder_set_metadata_callback()... ");
	if(!FLAC__file_decoder_set_metadata_callback(decoder, file_decoder_metadata_callback_)) {
		printf("FAILED, returned false\n");
		return false;
	}
	printf("OK\n");

	printf("testing FLAC__file_decoder_set_error_callback()... ");
	if(!FLAC__file_decoder_set_error_callback(decoder, file_decoder_error_callback_)) {
		printf("FAILED, returned false\n");
		return false;
	}
	printf("OK\n");

	printf("testing FLAC__file_decoder_set_client_data()... ");
	if(!FLAC__file_decoder_set_client_data(decoder, &decoder_client_data)) {
		printf("FAILED, returned false\n");
		return false;
	}
	printf("OK\n");

	printf("testing FLAC__file_decoder_set_filename()... ");
	if(!FLAC__file_decoder_set_filename(decoder, flacfilename_)) {
		printf("FAILED, returned false\n");
		return false;
	}
	printf("OK\n");

	printf("testing FLAC__file_decoder_set_md5_checking()... ");
	if(!FLAC__file_decoder_set_md5_checking(decoder, true)) {
		printf("FAILED, returned false\n");
		return false;
	}
	printf("OK\n");

	printf("testing FLAC__file_decoder_init()... ");
	if((state = FLAC__file_decoder_init(decoder)) != FLAC__FILE_DECODER_OK) {
		printf("FAILED, returned state = %u (%s)\n", state, FLAC__FileDecoderStateString[state]);
		return false;
	}
	printf("OK\n");

	printf("testing FLAC__file_decoder_get_state()... ");
	state = FLAC__file_decoder_get_state(decoder);
	printf("returned state = %u (%s)... OK\n", state, FLAC__FileDecoderStateString[state]);

	decoder_client_data.current_metadata_number = 0;
	decoder_client_data.ignore_errors = false;
	decoder_client_data.error_occurred = false;

	printf("testing FLAC__file_decoder_get_md5_checking()... ");
	if(!FLAC__file_decoder_get_md5_checking(decoder)) {
		printf("FAILED, returned false, expected true\n");
		return false;
	}
	printf("OK\n");

	printf("testing FLAC__file_decoder_process_metadata()... ");
	if(!FLAC__file_decoder_process_metadata(decoder)) {
		state = FLAC__file_decoder_get_state(decoder);
		printf("FAILED, returned false, state = %u (%s)\n", state, FLAC__FileDecoderStateString[state]);
		return false;
	}
	printf("OK\n");

	printf("testing FLAC__file_decoder_process_one_frame()... ");
	if(!FLAC__file_decoder_process_one_frame(decoder)) {
		state = FLAC__file_decoder_get_state(decoder);
		printf("FAILED, returned false, state = %u (%s)\n", state, FLAC__FileDecoderStateString[state]);
		return false;
	}
	printf("OK\n");

	printf("testing FLAC__file_decoder_seek_absolute()... ");
	if(!FLAC__file_decoder_seek_absolute(decoder, 0)) {
		state = FLAC__file_decoder_get_state(decoder);
		printf("FAILED, returned false, state = %u (%s)\n", state, FLAC__FileDecoderStateString[state]);
		return false;
	}
	printf("OK\n");

	printf("testing FLAC__file_decoder_process_remaining_frames()... ");
	if(!FLAC__file_decoder_process_remaining_frames(decoder)) {
		state = FLAC__file_decoder_get_state(decoder);
		printf("FAILED, returned false, state = %u (%s)\n", state, FLAC__FileDecoderStateString[state]);
		return false;
	}
	printf("OK\n");

	printf("testing FLAC__file_decoder_get_channels()... ");
	{
		unsigned channels = FLAC__file_decoder_get_channels(decoder);
		if(channels != streaminfo_.data.stream_info.channels) {
			printf("FAILED, returned %u, expected %u\n", channels, streaminfo_.data.stream_info.channels);
			return false;
		}
	}
	printf("OK\n");

	printf("testing FLAC__file_decoder_get_bits_per_sample()... ");
	{
		unsigned bits_per_sample = FLAC__file_decoder_get_bits_per_sample(decoder);
		if(bits_per_sample != streaminfo_.data.stream_info.bits_per_sample) {
			printf("FAILED, returned %u, expected %u\n", bits_per_sample, streaminfo_.data.stream_info.bits_per_sample);
			return false;
		}
	}
	printf("OK\n");

	printf("testing FLAC__file_decoder_get_sample_rate()... ");
	{
		unsigned sample_rate = FLAC__file_decoder_get_sample_rate(decoder);
		if(sample_rate != streaminfo_.data.stream_info.sample_rate) {
			printf("FAILED, returned %u, expected %u\n", sample_rate, streaminfo_.data.stream_info.sample_rate);
			return false;
		}
	}
	printf("OK\n");

	printf("testing FLAC__file_decoder_get_blocksize()... ");
	{
		unsigned blocksize = FLAC__file_decoder_get_blocksize(decoder);
		/* value could be anything since we're at the last block, so accept any answer */
		printf("returned %u... OK\n", blocksize);
	}

	printf("testing FLAC__file_decoder_get_channel_assignment()... ");
	{
		FLAC__ChannelAssignment ca = FLAC__file_decoder_get_channel_assignment(decoder);
		printf("returned %u (%s)... OK\n", (unsigned)ca, FLAC__ChannelAssignmentString[ca]);
	}

	printf("testing FLAC__file_decoder_finish()... ");
	(void) FLAC__file_decoder_finish(decoder);
	printf("OK\n");

	/*
	 * respond all
	 */

	printf("testing FLAC__file_decoder_set_metadata_respond_all()... ");
	if(!FLAC__file_decoder_set_metadata_respond_all(decoder)) {
		printf("FAILED, returned false\n");
		return false;
	}
	printf("OK\n");

	num_expected_ = 0;
	expected_metadata_sequence_[num_expected_++] = &streaminfo_;
	expected_metadata_sequence_[num_expected_++] = &padding_;
	expected_metadata_sequence_[num_expected_++] = &seektable_;
	expected_metadata_sequence_[num_expected_++] = &application1_;
	expected_metadata_sequence_[num_expected_++] = &application2_;
	expected_metadata_sequence_[num_expected_++] = &vorbiscomment_;

	if(!file_decoder_test_respond_(decoder, &decoder_client_data))
		return 1;

	/*
	 * ignore all
	 */

	printf("testing FLAC__file_decoder_set_metadata_ignore_all()... ");
	if(!FLAC__file_decoder_set_metadata_ignore_all(decoder)) {
		printf("FAILED, returned false\n");
		return false;
	}
	printf("OK\n");

	num_expected_ = 0;

	if(!file_decoder_test_respond_(decoder, &decoder_client_data))
		return 1;

	/*
	 * respond all, ignore VORBIS_COMMENT
	 */

	printf("testing FLAC__file_decoder_set_metadata_respond_all()... ");
	if(!FLAC__file_decoder_set_metadata_respond_all(decoder)) {
		printf("FAILED, returned false\n");
		return false;
	}
	printf("OK\n");

	printf("testing FLAC__file_decoder_set_metadata_ignore(VORBIS_COMMENT)... ");
	if(!FLAC__file_decoder_set_metadata_ignore(decoder, FLAC__METADATA_TYPE_VORBIS_COMMENT)) {
		printf("FAILED, returned false\n");
		return false;
	}
	printf("OK\n");

	num_expected_ = 0;
	expected_metadata_sequence_[num_expected_++] = &streaminfo_;
	expected_metadata_sequence_[num_expected_++] = &padding_;
	expected_metadata_sequence_[num_expected_++] = &seektable_;
	expected_metadata_sequence_[num_expected_++] = &application1_;
	expected_metadata_sequence_[num_expected_++] = &application2_;

	if(!file_decoder_test_respond_(decoder, &decoder_client_data))
		return 1;

	/*
	 * respond all, ignore APPLICATION
	 */

	printf("testing FLAC__file_decoder_set_metadata_respond_all()... ");
	if(!FLAC__file_decoder_set_metadata_respond_all(decoder)) {
		printf("FAILED, returned false\n");
		return false;
	}
	printf("OK\n");

	printf("testing FLAC__file_decoder_set_metadata_ignore(APPLICATION)... ");
	if(!FLAC__file_decoder_set_metadata_ignore(decoder, FLAC__METADATA_TYPE_APPLICATION)) {
		printf("FAILED, returned false\n");
		return false;
	}
	printf("OK\n");

	num_expected_ = 0;
	expected_metadata_sequence_[num_expected_++] = &streaminfo_;
	expected_metadata_sequence_[num_expected_++] = &padding_;
	expected_metadata_sequence_[num_expected_++] = &seektable_;
	expected_metadata_sequence_[num_expected_++] = &vorbiscomment_;

	if(!file_decoder_test_respond_(decoder, &decoder_client_data))
		return 1;

	/*
	 * respond all, ignore APPLICATION id of app#1
	 */

	printf("testing FLAC__file_decoder_set_metadata_respond_all()... ");
	if(!FLAC__file_decoder_set_metadata_respond_all(decoder)) {
		printf("FAILED, returned false\n");
		return false;
	}
	printf("OK\n");

	printf("testing FLAC__file_decoder_set_metadata_ignore_application(of app block #1)... ");
	if(!FLAC__file_decoder_set_metadata_ignore_application(decoder, application1_.data.application.id)) {
		printf("FAILED, returned false\n");
		return false;
	}
	printf("OK\n");

	num_expected_ = 0;
	expected_metadata_sequence_[num_expected_++] = &streaminfo_;
	expected_metadata_sequence_[num_expected_++] = &padding_;
	expected_metadata_sequence_[num_expected_++] = &seektable_;
	expected_metadata_sequence_[num_expected_++] = &application2_;
	expected_metadata_sequence_[num_expected_++] = &vorbiscomment_;

	if(!file_decoder_test_respond_(decoder, &decoder_client_data))
		return 1;

	/*
	 * respond all, ignore APPLICATION id of app#1 & app#2
	 */

	printf("testing FLAC__file_decoder_set_metadata_respond_all()... ");
	if(!FLAC__file_decoder_set_metadata_respond_all(decoder)) {
		printf("FAILED, returned false\n");
		return false;
	}
	printf("OK\n");

	printf("testing FLAC__file_decoder_set_metadata_ignore_application(of app block #1)... ");
	if(!FLAC__file_decoder_set_metadata_ignore_application(decoder, application1_.data.application.id)) {
		printf("FAILED, returned false\n");
		return false;
	}
	printf("OK\n");

	printf("testing FLAC__file_decoder_set_metadata_ignore_application(of app block #2)... ");
	if(!FLAC__file_decoder_set_metadata_ignore_application(decoder, application2_.data.application.id)) {
		printf("FAILED, returned false\n");
		return false;
	}
	printf("OK\n");

	num_expected_ = 0;
	expected_metadata_sequence_[num_expected_++] = &streaminfo_;
	expected_metadata_sequence_[num_expected_++] = &padding_;
	expected_metadata_sequence_[num_expected_++] = &seektable_;
	expected_metadata_sequence_[num_expected_++] = &vorbiscomment_;

	if(!file_decoder_test_respond_(decoder, &decoder_client_data))
		return 1;

	/*
	 * ignore all, respond VORBIS_COMMENT
	 */

	printf("testing FLAC__file_decoder_set_metadata_ignore_all()... ");
	if(!FLAC__file_decoder_set_metadata_ignore_all(decoder)) {
		printf("FAILED, returned false\n");
		return false;
	}
	printf("OK\n");

	printf("testing FLAC__file_decoder_set_metadata_respond(VORBIS_COMMENT)... ");
	if(!FLAC__file_decoder_set_metadata_respond(decoder, FLAC__METADATA_TYPE_VORBIS_COMMENT)) {
		printf("FAILED, returned false\n");
		return false;
	}
	printf("OK\n");

	num_expected_ = 0;
	expected_metadata_sequence_[num_expected_++] = &vorbiscomment_;

	if(!file_decoder_test_respond_(decoder, &decoder_client_data))
		return 1;

	/*
	 * ignore all, respond APPLICATION
	 */

	printf("testing FLAC__file_decoder_set_metadata_ignore_all()... ");
	if(!FLAC__file_decoder_set_metadata_ignore_all(decoder)) {
		printf("FAILED, returned false\n");
		return false;
	}
	printf("OK\n");

	printf("testing FLAC__file_decoder_set_metadata_respond(APPLICATION)... ");
	if(!FLAC__file_decoder_set_metadata_respond(decoder, FLAC__METADATA_TYPE_APPLICATION)) {
		printf("FAILED, returned false\n");
		return false;
	}
	printf("OK\n");

	num_expected_ = 0;
	expected_metadata_sequence_[num_expected_++] = &application1_;
	expected_metadata_sequence_[num_expected_++] = &application2_;

	if(!file_decoder_test_respond_(decoder, &decoder_client_data))
		return 1;

	/*
	 * ignore all, respond APPLICATION id of app#1
	 */

	printf("testing FLAC__file_decoder_set_metadata_ignore_all()... ");
	if(!FLAC__file_decoder_set_metadata_ignore_all(decoder)) {
		printf("FAILED, returned false\n");
		return false;
	}
	printf("OK\n");

	printf("testing FLAC__file_decoder_set_metadata_respond_application(of app block #1)... ");
	if(!FLAC__file_decoder_set_metadata_respond_application(decoder, application1_.data.application.id)) {
		printf("FAILED, returned false\n");
		return false;
	}
	printf("OK\n");

	num_expected_ = 0;
	expected_metadata_sequence_[num_expected_++] = &application1_;

	if(!file_decoder_test_respond_(decoder, &decoder_client_data))
		return 1;

	/*
	 * ignore all, respond APPLICATION id of app#1 & app#2
	 */

	printf("testing FLAC__file_decoder_set_metadata_ignore_all()... ");
	if(!FLAC__file_decoder_set_metadata_ignore_all(decoder)) {
		printf("FAILED, returned false\n");
		return false;
	}
	printf("OK\n");

	printf("testing FLAC__file_decoder_set_metadata_respond_application(of app block #1)... ");
	if(!FLAC__file_decoder_set_metadata_respond_application(decoder, application1_.data.application.id)) {
		printf("FAILED, returned false\n");
		return false;
	}
	printf("OK\n");

	printf("testing FLAC__file_decoder_set_metadata_respond_application(of app block #2)... ");
	if(!FLAC__file_decoder_set_metadata_respond_application(decoder, application2_.data.application.id)) {
		printf("FAILED, returned false\n");
		return false;
	}
	printf("OK\n");

	num_expected_ = 0;
	expected_metadata_sequence_[num_expected_++] = &application1_;
	expected_metadata_sequence_[num_expected_++] = &application2_;

	if(!file_decoder_test_respond_(decoder, &decoder_client_data))
		return 1;

	/*
	 * respond all, ignore APPLICATION, respond APPLICATION id of app#1
	 */

	printf("testing FLAC__file_decoder_set_metadata_respond_all()... ");
	if(!FLAC__file_decoder_set_metadata_respond_all(decoder)) {
		printf("FAILED, returned false\n");
		return false;
	}
	printf("OK\n");

	printf("testing FLAC__file_decoder_set_metadata_ignore(APPLICATION)... ");
	if(!FLAC__file_decoder_set_metadata_ignore(decoder, FLAC__METADATA_TYPE_APPLICATION)) {
		printf("FAILED, returned false\n");
		return false;
	}
	printf("OK\n");

	printf("testing FLAC__file_decoder_set_metadata_respond_application(of app block #1)... ");
	if(!FLAC__file_decoder_set_metadata_respond_application(decoder, application1_.data.application.id)) {
		printf("FAILED, returned false\n");
		return false;
	}
	printf("OK\n");

	num_expected_ = 0;
	expected_metadata_sequence_[num_expected_++] = &streaminfo_;
	expected_metadata_sequence_[num_expected_++] = &padding_;
	expected_metadata_sequence_[num_expected_++] = &seektable_;
	expected_metadata_sequence_[num_expected_++] = &application1_;
	expected_metadata_sequence_[num_expected_++] = &vorbiscomment_;

	if(!file_decoder_test_respond_(decoder, &decoder_client_data))
		return 1;

	/*
	 * ignore all, respond APPLICATION, ignore APPLICATION id of app#1
	 */

	printf("testing FLAC__file_decoder_set_metadata_ignore_all()... ");
	if(!FLAC__file_decoder_set_metadata_ignore_all(decoder)) {
		printf("FAILED, returned false\n");
		return false;
	}
	printf("OK\n");

	printf("testing FLAC__file_decoder_set_metadata_respond(APPLICATION)... ");
	if(!FLAC__file_decoder_set_metadata_respond(decoder, FLAC__METADATA_TYPE_APPLICATION)) {
		printf("FAILED, returned false\n");
		return false;
	}
	printf("OK\n");

	printf("testing FLAC__file_decoder_set_metadata_ignore_application(of app block #1)... ");
	if(!FLAC__file_decoder_set_metadata_ignore_application(decoder, application1_.data.application.id)) {
		printf("FAILED, returned false\n");
		return false;
	}
	printf("OK\n");

	num_expected_ = 0;
	expected_metadata_sequence_[num_expected_++] = &application2_;

	if(!file_decoder_test_respond_(decoder, &decoder_client_data))
		return 1;

	/* done, now leave the sequence the way we found it... */
	num_expected_ = 0;
	expected_metadata_sequence_[num_expected_++] = &streaminfo_;
	expected_metadata_sequence_[num_expected_++] = &padding_;
	expected_metadata_sequence_[num_expected_++] = &seektable_;
	expected_metadata_sequence_[num_expected_++] = &application1_;
	expected_metadata_sequence_[num_expected_++] = &application2_;
	expected_metadata_sequence_[num_expected_++] = &vorbiscomment_;

	printf("testing FLAC__file_decoder_delete()... ");
	FLAC__file_decoder_delete(decoder);
	printf("OK\n");

	printf("\nPASSED!\n");

	printf("\nPASSED!\n");

	return true;
}

int test_decoders()
{
	init_metadata_blocks_();
	if(!generate_file_())
		return 1;

	if(!test_stream_decoder())
		return 1;

	if(!test_seekable_stream_decoder())
		return 1;

	if(!test_file_decoder())
		return 1;

	(void) file_utils__remove_file(flacfilename_);
	free_metadata_blocks_();

	return 0;
}
#if 0
typedef enum {
    FLAC__FILE_DECODER_OK = 0,
	FLAC__FILE_DECODER_END_OF_FILE,
    FLAC__FILE_DECODER_ERROR_OPENING_FILE,
    FLAC__FILE_DECODER_MEMORY_ALLOCATION_ERROR,
	FLAC__FILE_DECODER_SEEK_ERROR,
	FLAC__FILE_DECODER_SEEKABLE_STREAM_DECODER_ERROR,
    FLAC__FILE_DECODER_ALREADY_INITIALIZED,
    FLAC__FILE_DECODER_INVALID_CALLBACK,
    FLAC__FILE_DECODER_UNINITIALIZED
} FLAC__FileDecoderState;
extern const char *FLAC__FileDecoderStateString[];

/*
 * Methods for decoding the data
 */
FLAC__bool FLAC__file_decoder_process_whole_file(FLAC__FileDecoder *decoder);
FLAC__bool FLAC__file_decoder_process_metadata(FLAC__FileDecoder *decoder);
FLAC__bool FLAC__file_decoder_process_one_frame(FLAC__FileDecoder *decoder);
FLAC__bool FLAC__file_decoder_process_remaining_frames(FLAC__FileDecoder *decoder);

FLAC__bool FLAC__file_decoder_seek_absolute(FLAC__FileDecoder *decoder, FLAC__uint64 sample);
#endif
