/* test_libFLAC++ - Unit tester for libFLAC++
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
extern "C" {
#include "file_utils.h"
}
#include "FLAC/assert.h"
#include "FLAC/metadata.h" // for ::FLAC__metadata_object_is_equal()
#include "FLAC++/decoder.h"
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static ::FLAC__StreamMetadata streaminfo_, padding_, seektable_, application1_, application2_, vorbiscomment_;
static ::FLAC__StreamMetadata *expected_metadata_sequence_[6];
static unsigned num_expected_;
static const char *flacfilename_ = "metadata.flac";
static unsigned flacfilesize_;

static bool die_(const char *msg)
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
    streaminfo_.type = ::FLAC__METADATA_TYPE_STREAMINFO;
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
    padding_.type = ::FLAC__METADATA_TYPE_PADDING;
    padding_.length = 1234;

    seektable_.is_last = false;
    seektable_.type = ::FLAC__METADATA_TYPE_SEEKTABLE;
	seektable_.data.seek_table.num_points = 2;
    seektable_.length = seektable_.data.seek_table.num_points * FLAC__STREAM_METADATA_SEEKPOINT_LENGTH;
	seektable_.data.seek_table.points = (::FLAC__StreamMetadata_SeekPoint*)malloc_or_die_(seektable_.data.seek_table.num_points * sizeof(::FLAC__StreamMetadata_SeekPoint));
	seektable_.data.seek_table.points[0].sample_number = 0;
	seektable_.data.seek_table.points[0].stream_offset = 0;
	seektable_.data.seek_table.points[0].frame_samples = streaminfo_.data.stream_info.min_blocksize;
	seektable_.data.seek_table.points[1].sample_number = ::FLAC__STREAM_METADATA_SEEKPOINT_PLACEHOLDER;
	seektable_.data.seek_table.points[1].stream_offset = 1000;
	seektable_.data.seek_table.points[1].frame_samples = streaminfo_.data.stream_info.min_blocksize;

    application1_.is_last = false;
    application1_.type = ::FLAC__METADATA_TYPE_APPLICATION;
    application1_.length = 8;
	memcpy(application1_.data.application.id, "\xfe\xdc\xba\x98", 4);
	application1_.data.application.data = (FLAC__byte*)malloc_or_die_(4);
	memcpy(application1_.data.application.data, "\xf0\xe1\xd2\xc3", 4);

    application2_.is_last = false;
    application2_.type = ::FLAC__METADATA_TYPE_APPLICATION;
    application2_.length = 4;
	memcpy(application2_.data.application.id, "\x76\x54\x32\x10", 4);
	application2_.data.application.data = 0;

    vorbiscomment_.is_last = true;
    vorbiscomment_.type = ::FLAC__METADATA_TYPE_VORBIS_COMMENT;
    vorbiscomment_.length = (4 + 8) + 4 + (4 + 5) + (4 + 0);
	vorbiscomment_.data.vorbis_comment.vendor_string.length = 8;
	vorbiscomment_.data.vorbis_comment.vendor_string.entry = (FLAC__byte*)malloc_or_die_(8);
	memcpy(vorbiscomment_.data.vorbis_comment.vendor_string.entry, "flac 1.x", 8);
	vorbiscomment_.data.vorbis_comment.num_comments = 2;
	vorbiscomment_.data.vorbis_comment.comments = (::FLAC__StreamMetadata_VorbisComment_Entry*)malloc_or_die_(vorbiscomment_.data.vorbis_comment.num_comments * sizeof(::FLAC__StreamMetadata_VorbisComment_Entry));
	vorbiscomment_.data.vorbis_comment.comments[0].length = 5;
	vorbiscomment_.data.vorbis_comment.comments[0].entry = (FLAC__byte*)malloc_or_die_(5);
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

static bool generate_file_()
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


class DecoderCommon {
public:
	FILE *file_;
	unsigned current_metadata_number_;
	bool ignore_errors_;
	bool error_occurred_;

	DecoderCommon(): file_(0), current_metadata_number_(0), ignore_errors_(false), error_occurred_(false) { }
	::FLAC__StreamDecoderReadStatus common_read_callback_(FLAC__byte buffer[], unsigned *bytes);
	::FLAC__StreamDecoderWriteStatus common_write_callback_(const ::FLAC__Frame *frame);
	void common_metadata_callback_(const ::FLAC__StreamMetadata *metadata);
	void common_error_callback_(::FLAC__StreamDecoderErrorStatus status);
};

::FLAC__StreamDecoderReadStatus DecoderCommon::common_read_callback_(FLAC__byte buffer[], unsigned *bytes)
{
	if(error_occurred_)
		return ::FLAC__STREAM_DECODER_READ_STATUS_ABORT;

	if(feof(file_))
		return ::FLAC__STREAM_DECODER_READ_STATUS_END_OF_STREAM;
    else if(*bytes > 0) {
        unsigned bytes_read = ::fread(buffer, 1, *bytes, file_);
        if(bytes_read == 0) {
            if(feof(file_))
                return ::FLAC__STREAM_DECODER_READ_STATUS_END_OF_STREAM;
            else
                return ::FLAC__STREAM_DECODER_READ_STATUS_CONTINUE;
        }
        else {
            *bytes = bytes_read;
            return ::FLAC__STREAM_DECODER_READ_STATUS_CONTINUE;
        }
    }
    else
        return ::FLAC__STREAM_DECODER_READ_STATUS_ABORT; /* abort to avoid a deadlock */
}

::FLAC__StreamDecoderWriteStatus DecoderCommon::common_write_callback_(const ::FLAC__Frame *frame)
{
	if(error_occurred_)
		return ::FLAC__STREAM_DECODER_WRITE_STATUS_ABORT;

    if(
        (frame->header.number_type == ::FLAC__FRAME_NUMBER_TYPE_FRAME_NUMBER && frame->header.number.frame_number == 0) ||
        (frame->header.number_type == ::FLAC__FRAME_NUMBER_TYPE_SAMPLE_NUMBER && frame->header.number.sample_number == 0)
    ) {
        printf("content... ");
        fflush(stdout);
    }

	return ::FLAC__STREAM_DECODER_WRITE_STATUS_CONTINUE;
}

void DecoderCommon::common_metadata_callback_(const ::FLAC__StreamMetadata *metadata)
{
	if(error_occurred_)
		return;

	printf("%d... ", current_metadata_number_);
	fflush(stdout);

	if(current_metadata_number_ >= num_expected_) {
		(void)die_("got more metadata blocks than expected");
		error_occurred_ = true;
	}
	else {
		if(!::FLAC__metadata_object_is_equal(expected_metadata_sequence_[current_metadata_number_], metadata)) {
			(void)die_("metadata block mismatch");
			error_occurred_ = true;
		}
	}
	current_metadata_number_++;
}

void DecoderCommon::common_error_callback_(::FLAC__StreamDecoderErrorStatus status)
{
	if(!ignore_errors_) {
		printf("ERROR: got error callback: err = %u (%s)\n", (unsigned)status, ::FLAC__StreamDecoderErrorStatusString[status]);
		error_occurred_ = true;
	}
}

class StreamDecoder : public FLAC::Decoder::Stream, public DecoderCommon {
public:
	StreamDecoder(): FLAC::Decoder::Stream(), DecoderCommon() { }
	~StreamDecoder() { }

	// from FLAC::Decoder::Stream
	::FLAC__StreamDecoderReadStatus read_callback(FLAC__byte buffer[], unsigned *bytes);
	::FLAC__StreamDecoderWriteStatus write_callback(const ::FLAC__Frame *frame, const FLAC__int32 * const buffer[]);
	void metadata_callback(const ::FLAC__StreamMetadata *metadata);
	void error_callback(::FLAC__StreamDecoderErrorStatus status);

	bool die(const char *msg = 0) const;

	bool test_respond();
};

::FLAC__StreamDecoderReadStatus StreamDecoder::read_callback(FLAC__byte buffer[], unsigned *bytes)
{
	return common_read_callback_(buffer, bytes);
}

::FLAC__StreamDecoderWriteStatus StreamDecoder::write_callback(const ::FLAC__Frame *frame, const FLAC__int32 * const buffer[])
{
	(void)buffer;

	return common_write_callback_(frame);
}

void StreamDecoder::metadata_callback(const ::FLAC__StreamMetadata *metadata)
{
	common_metadata_callback_(metadata);
}

void StreamDecoder::error_callback(::FLAC__StreamDecoderErrorStatus status)
{
	common_error_callback_(status);
}

bool StreamDecoder::die(const char *msg) const
{
	State state = get_state();

	if(msg)
		printf("FAILED, %s, state = %u (%s)\n", msg, (unsigned)((::FLAC__StreamDecoderState)state), state.as_cstring());
	else
		printf("FAILED, state = %u (%s)\n", (unsigned)((::FLAC__StreamDecoderState)state), state.as_cstring());

	return false;
}

bool StreamDecoder::test_respond()
{
	printf("testing init()... ");
	if(init() != ::FLAC__STREAM_DECODER_SEARCH_FOR_METADATA)
		return die();
	printf("OK\n");

	current_metadata_number_ = 0;

	if(::fseek(file_, 0, SEEK_SET) < 0) {
		printf("FAILED rewinding input, errno = %d\n", errno);
		return false;
	}

	printf("testing process_until_end_of_stream()... ");
	if(!process_until_end_of_stream()) {
		State state = get_state();
		printf("FAILED, returned false, state = %u (%s)\n", (unsigned)((::FLAC__StreamDecoderState)state), state.as_cstring());
		return false;
	}
	printf("OK\n");

	printf("testing finish()... ");
	finish();
	printf("OK\n");

	return true;
}

static bool test_stream_decoder()
{
	StreamDecoder *decoder;

	printf("\n+++ libFLAC++ unit test: FLAC::Decoder::Stream\n\n");

	//
	// test new -> delete
	//
	printf("allocating decoder instance... ");
	decoder = new StreamDecoder();
	if(0 == decoder) {
		printf("FAILED, new returned NULL\n");
		return false;
	}
	printf("OK\n");

	printf("testing is_valid()... ");
	if(!decoder->is_valid()) {
		printf("FAILED, returned false\n");
		return false;
	}
	printf("OK\n");

	printf("freeing decoder instance... ");
	delete decoder;
	printf("OK\n");

	//
	// test new -> init -> delete
	//
	printf("allocating decoder instance... ");
	decoder = new StreamDecoder();
	if(0 == decoder) {
		printf("FAILED, new returned NULL\n");
		return false;
	}
	printf("OK\n");

	printf("testing is_valid()... ");
	if(!decoder->is_valid()) {
		printf("FAILED, returned false\n");
		return false;
	}
	printf("OK\n");

	printf("testing init()... ");
	if(decoder->init() != ::FLAC__STREAM_DECODER_SEARCH_FOR_METADATA)
		return decoder->die();
	printf("OK\n");

	printf("freeing decoder instance... ");
	delete decoder;
	printf("OK\n");

	//
	// test normal usage
	//
	num_expected_ = 0;
	expected_metadata_sequence_[num_expected_++] = &streaminfo_;

	printf("allocating decoder instance... ");
	decoder = new StreamDecoder();
	if(0 == decoder) {
		printf("FAILED, new returned NULL\n");
		return false;
	}
	printf("OK\n");

	printf("testing is_valid()... ");
	if(!decoder->is_valid()) {
		printf("FAILED, returned false\n");
		return false;
	}
	printf("OK\n");

	printf("testing init()... ");
	if(decoder->init() != ::FLAC__STREAM_DECODER_SEARCH_FOR_METADATA)
		return decoder->die();
	printf("OK\n");

	printf("testing get_state()... ");
	FLAC::Decoder::Stream::State state = decoder->get_state();
	printf("returned state = %u (%s)... OK\n", (unsigned)((::FLAC__StreamDecoderState)state), state.as_cstring());

	decoder->current_metadata_number_ = 0;
	decoder->ignore_errors_ = false;
	decoder->error_occurred_ = false;

	printf("opening FLAC file... ");
	decoder->file_ = ::fopen(flacfilename_, "rb");
	if(0 == decoder->file_) {
		printf("ERROR\n");
		return false;
	}
	printf("OK\n");

	printf("testing process_until_end_of_metadata()... ");
	if(!decoder->process_until_end_of_metadata())
		return decoder->die("returned false");
	printf("OK\n");

	printf("testing process_single()... ");
	if(!decoder->process_single())
		return decoder->die("returned false");
	printf("OK\n");

	printf("testing flush()... ");
	if(!decoder->flush())
		return decoder->die("returned false");
	printf("OK\n");

	decoder->ignore_errors_ = true;
	printf("testing process_single()... ");
	if(!decoder->process_single())
		return decoder->die("returned false");
	printf("OK\n");
	decoder->ignore_errors_ = false;

	printf("testing process_until_end_of_stream()... ");
	if(!decoder->process_until_end_of_stream())
		return decoder->die("returned false");
	printf("OK\n");

	printf("testing get_channels()... ");
	{
		unsigned channels = decoder->get_channels();
		if(channels != streaminfo_.data.stream_info.channels) {
			printf("FAILED, returned %u, expected %u\n", channels, streaminfo_.data.stream_info.channels);
			return false;
		}
	}
	printf("OK\n");

	printf("testing get_bits_per_sample()... ");
	{
		unsigned bits_per_sample = decoder->get_bits_per_sample();
		if(bits_per_sample != streaminfo_.data.stream_info.bits_per_sample) {
			printf("FAILED, returned %u, expected %u\n", bits_per_sample, streaminfo_.data.stream_info.bits_per_sample);
			return false;
		}
	}
	printf("OK\n");

	printf("testing get_sample_rate()... ");
	{
		unsigned sample_rate = decoder->get_sample_rate();
		if(sample_rate != streaminfo_.data.stream_info.sample_rate) {
			printf("FAILED, returned %u, expected %u\n", sample_rate, streaminfo_.data.stream_info.sample_rate);
			return false;
		}
	}
	printf("OK\n");

	printf("testing get_blocksize()... ");
	{
		unsigned blocksize = decoder->get_blocksize();
		/* value could be anything since we're at the last block, so accept any answer */
		printf("returned %u... OK\n", blocksize);
	}

	printf("testing get_channel_assignment()... ");
	{
		::FLAC__ChannelAssignment ca = decoder->get_channel_assignment();
		printf("returned %u (%s)... OK\n", (unsigned)ca, ::FLAC__ChannelAssignmentString[ca]);
	}

	printf("testing reset()... ");
	if(!decoder->reset())
		return decoder->die("returned false");
	printf("OK\n");

	decoder->current_metadata_number_ = 0;

	printf("rewinding input... ");
	if(::fseek(decoder->file_, 0, SEEK_SET) < 0) {
		printf("FAILED, errno = %d\n", errno);
		return false;
	}
	printf("OK\n");

	printf("testing process_until_end_of_stream()... ");
	if(!decoder->process_until_end_of_stream())
		return decoder->die("returned false");
	printf("OK\n");

	printf("testing finish()... ");
	decoder->finish();
	printf("OK\n");

	/*
	 * respond all
	 */

	printf("testing set_metadata_respond_all()... ");
	if(!decoder->set_metadata_respond_all()) {
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

	if(!decoder->test_respond())
		return false;

	/*
	 * ignore all
	 */

	printf("testing set_metadata_ignore_all()... ");
	if(!decoder->set_metadata_ignore_all()) {
		printf("FAILED, returned false\n");
		return false;
	}
	printf("OK\n");

	num_expected_ = 0;

	if(!decoder->test_respond())
		return false;

	/*
	 * respond all, ignore VORBIS_COMMENT
	 */

	printf("testing set_metadata_respond_all()... ");
	if(!decoder->set_metadata_respond_all()) {
		printf("FAILED, returned false\n");
		return false;
	}
	printf("OK\n");

	printf("testing set_metadata_ignore(VORBIS_COMMENT)... ");
	if(!decoder->set_metadata_ignore(FLAC__METADATA_TYPE_VORBIS_COMMENT)) {
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

	if(!decoder->test_respond())
		return false;

	/*
	 * respond all, ignore APPLICATION
	 */

	printf("testing set_metadata_respond_all()... ");
	if(!decoder->set_metadata_respond_all()) {
		printf("FAILED, returned false\n");
		return false;
	}
	printf("OK\n");

	printf("testing set_metadata_ignore(APPLICATION)... ");
	if(!decoder->set_metadata_ignore(FLAC__METADATA_TYPE_APPLICATION)) {
		printf("FAILED, returned false\n");
		return false;
	}
	printf("OK\n");

	num_expected_ = 0;
	expected_metadata_sequence_[num_expected_++] = &streaminfo_;
	expected_metadata_sequence_[num_expected_++] = &padding_;
	expected_metadata_sequence_[num_expected_++] = &seektable_;
	expected_metadata_sequence_[num_expected_++] = &vorbiscomment_;

	if(!decoder->test_respond())
		return false;

	/*
	 * respond all, ignore APPLICATION id of app#1
	 */

	printf("testing set_metadata_respond_all()... ");
	if(!decoder->set_metadata_respond_all()) {
		printf("FAILED, returned false\n");
		return false;
	}
	printf("OK\n");

	printf("testing set_metadata_ignore_application(of app block #1)... ");
	if(!decoder->set_metadata_ignore_application(application1_.data.application.id)) {
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

	if(!decoder->test_respond())
		return false;

	/*
	 * respond all, ignore APPLICATION id of app#1 & app#2
	 */

	printf("testing set_metadata_respond_all()... ");
	if(!decoder->set_metadata_respond_all()) {
		printf("FAILED, returned false\n");
		return false;
	}
	printf("OK\n");

	printf("testing set_metadata_ignore_application(of app block #1)... ");
	if(!decoder->set_metadata_ignore_application(application1_.data.application.id)) {
		printf("FAILED, returned false\n");
		return false;
	}
	printf("OK\n");

	printf("testing set_metadata_ignore_application(of app block #2)... ");
	if(!decoder->set_metadata_ignore_application(application2_.data.application.id)) {
		printf("FAILED, returned false\n");
		return false;
	}
	printf("OK\n");

	num_expected_ = 0;
	expected_metadata_sequence_[num_expected_++] = &streaminfo_;
	expected_metadata_sequence_[num_expected_++] = &padding_;
	expected_metadata_sequence_[num_expected_++] = &seektable_;
	expected_metadata_sequence_[num_expected_++] = &vorbiscomment_;

	if(!decoder->test_respond())
		return false;

	/*
	 * ignore all, respond VORBIS_COMMENT
	 */

	printf("testing set_metadata_ignore_all()... ");
	if(!decoder->set_metadata_ignore_all()) {
		printf("FAILED, returned false\n");
		return false;
	}
	printf("OK\n");

	printf("testing set_metadata_respond(VORBIS_COMMENT)... ");
	if(!decoder->set_metadata_respond(FLAC__METADATA_TYPE_VORBIS_COMMENT)) {
		printf("FAILED, returned false\n");
		return false;
	}
	printf("OK\n");

	num_expected_ = 0;
	expected_metadata_sequence_[num_expected_++] = &vorbiscomment_;

	if(!decoder->test_respond())
		return false;

	/*
	 * ignore all, respond APPLICATION
	 */

	printf("testing set_metadata_ignore_all()... ");
	if(!decoder->set_metadata_ignore_all()) {
		printf("FAILED, returned false\n");
		return false;
	}
	printf("OK\n");

	printf("testing set_metadata_respond(APPLICATION)... ");
	if(!decoder->set_metadata_respond(FLAC__METADATA_TYPE_APPLICATION)) {
		printf("FAILED, returned false\n");
		return false;
	}
	printf("OK\n");

	num_expected_ = 0;
	expected_metadata_sequence_[num_expected_++] = &application1_;
	expected_metadata_sequence_[num_expected_++] = &application2_;

	if(!decoder->test_respond())
		return false;

	/*
	 * ignore all, respond APPLICATION id of app#1
	 */

	printf("testing set_metadata_ignore_all()... ");
	if(!decoder->set_metadata_ignore_all()) {
		printf("FAILED, returned false\n");
		return false;
	}
	printf("OK\n");

	printf("testing set_metadata_respond_application(of app block #1)... ");
	if(!decoder->set_metadata_respond_application(application1_.data.application.id)) {
		printf("FAILED, returned false\n");
		return false;
	}
	printf("OK\n");

	num_expected_ = 0;
	expected_metadata_sequence_[num_expected_++] = &application1_;

	if(!decoder->test_respond())
		return false;

	/*
	 * ignore all, respond APPLICATION id of app#1 & app#2
	 */

	printf("testing set_metadata_ignore_all()... ");
	if(!decoder->set_metadata_ignore_all()) {
		printf("FAILED, returned false\n");
		return false;
	}
	printf("OK\n");

	printf("testing set_metadata_respond_application(of app block #1)... ");
	if(!decoder->set_metadata_respond_application(application1_.data.application.id)) {
		printf("FAILED, returned false\n");
		return false;
	}
	printf("OK\n");

	printf("testing set_metadata_respond_application(of app block #2)... ");
	if(!decoder->set_metadata_respond_application(application2_.data.application.id)) {
		printf("FAILED, returned false\n");
		return false;
	}
	printf("OK\n");

	num_expected_ = 0;
	expected_metadata_sequence_[num_expected_++] = &application1_;
	expected_metadata_sequence_[num_expected_++] = &application2_;

	if(!decoder->test_respond())
		return false;

	/*
	 * respond all, ignore APPLICATION, respond APPLICATION id of app#1
	 */

	printf("testing set_metadata_respond_all()... ");
	if(!decoder->set_metadata_respond_all()) {
		printf("FAILED, returned false\n");
		return false;
	}
	printf("OK\n");

	printf("testing set_metadata_ignore(APPLICATION)... ");
	if(!decoder->set_metadata_ignore(FLAC__METADATA_TYPE_APPLICATION)) {
		printf("FAILED, returned false\n");
		return false;
	}
	printf("OK\n");

	printf("testing set_metadata_respond_application(of app block #1)... ");
	if(!decoder->set_metadata_respond_application(application1_.data.application.id)) {
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

	if(!decoder->test_respond())
		return false;

	/*
	 * ignore all, respond APPLICATION, ignore APPLICATION id of app#1
	 */

	printf("testing set_metadata_ignore_all()... ");
	if(!decoder->set_metadata_ignore_all()) {
		printf("FAILED, returned false\n");
		return false;
	}
	printf("OK\n");

	printf("testing set_metadata_respond(APPLICATION)... ");
	if(!decoder->set_metadata_respond(FLAC__METADATA_TYPE_APPLICATION)) {
		printf("FAILED, returned false\n");
		return false;
	}
	printf("OK\n");

	printf("testing set_metadata_ignore_application(of app block #1)... ");
	if(!decoder->set_metadata_ignore_application(application1_.data.application.id)) {
		printf("FAILED, returned false\n");
		return false;
	}
	printf("OK\n");

	num_expected_ = 0;
	expected_metadata_sequence_[num_expected_++] = &application2_;

	if(!decoder->test_respond())
		return false;

	/* done, now leave the sequence the way we found it... */
	num_expected_ = 0;
	expected_metadata_sequence_[num_expected_++] = &streaminfo_;
	expected_metadata_sequence_[num_expected_++] = &padding_;
	expected_metadata_sequence_[num_expected_++] = &seektable_;
	expected_metadata_sequence_[num_expected_++] = &application1_;
	expected_metadata_sequence_[num_expected_++] = &application2_;
	expected_metadata_sequence_[num_expected_++] = &vorbiscomment_;

	::fclose(decoder->file_);

	printf("freeing decoder instance... ");
	delete decoder;
	printf("OK\n");

	printf("\nPASSED!\n");

	return true;
}

class SeekableStreamDecoder : public FLAC::Decoder::SeekableStream, public DecoderCommon {
public:
	SeekableStreamDecoder(): FLAC::Decoder::SeekableStream(), DecoderCommon() { }
	~SeekableStreamDecoder() { }

	// from FLAC::Decoder::SeekableStream
	::FLAC__SeekableStreamDecoderReadStatus read_callback(FLAC__byte buffer[], unsigned *bytes);
	::FLAC__SeekableStreamDecoderSeekStatus seek_callback(FLAC__uint64 absolute_byte_offset);
	::FLAC__SeekableStreamDecoderTellStatus tell_callback(FLAC__uint64 *absolute_byte_offset);
	::FLAC__SeekableStreamDecoderLengthStatus length_callback(FLAC__uint64 *stream_length);
	bool eof_callback();
	::FLAC__StreamDecoderWriteStatus write_callback(const ::FLAC__Frame *frame, const FLAC__int32 * const buffer[]);
	void metadata_callback(const ::FLAC__StreamMetadata *metadata);
	void error_callback(::FLAC__StreamDecoderErrorStatus status);

	bool die(const char *msg = 0) const;

	bool test_respond();
};

::FLAC__SeekableStreamDecoderReadStatus SeekableStreamDecoder::read_callback(FLAC__byte buffer[], unsigned *bytes)
{
	switch(common_read_callback_(buffer, bytes)) {
		case ::FLAC__STREAM_DECODER_READ_STATUS_CONTINUE:
			return ::FLAC__SEEKABLE_STREAM_DECODER_READ_STATUS_OK;
		case ::FLAC__STREAM_DECODER_READ_STATUS_ABORT:
		case ::FLAC__STREAM_DECODER_READ_STATUS_END_OF_STREAM:
			return ::FLAC__SEEKABLE_STREAM_DECODER_READ_STATUS_ERROR;
		default:
			FLAC__ASSERT(0);
			return ::FLAC__SEEKABLE_STREAM_DECODER_READ_STATUS_ERROR;
	}
}

::FLAC__SeekableStreamDecoderSeekStatus SeekableStreamDecoder::seek_callback(FLAC__uint64 absolute_byte_offset)
{
	if(error_occurred_)
		return ::FLAC__SEEKABLE_STREAM_DECODER_SEEK_STATUS_ERROR;

	if(::fseek(file_, (long)absolute_byte_offset, SEEK_SET) < 0) {
		error_occurred_ = true;
		return ::FLAC__SEEKABLE_STREAM_DECODER_SEEK_STATUS_ERROR;
	}

	return ::FLAC__SEEKABLE_STREAM_DECODER_SEEK_STATUS_OK;
}

::FLAC__SeekableStreamDecoderTellStatus SeekableStreamDecoder::tell_callback(FLAC__uint64 *absolute_byte_offset)
{
	if(error_occurred_)
		return ::FLAC__SEEKABLE_STREAM_DECODER_TELL_STATUS_ERROR;

	long offset = ::ftell(file_);
	*absolute_byte_offset = (FLAC__uint64)offset;

	if(offset < 0) {
		error_occurred_ = true;
		return ::FLAC__SEEKABLE_STREAM_DECODER_TELL_STATUS_ERROR;
	}

	return ::FLAC__SEEKABLE_STREAM_DECODER_TELL_STATUS_OK;
}

::FLAC__SeekableStreamDecoderLengthStatus SeekableStreamDecoder::length_callback(FLAC__uint64 *stream_length)
{
	if(error_occurred_)
		return ::FLAC__SEEKABLE_STREAM_DECODER_LENGTH_STATUS_ERROR;

	*stream_length = (FLAC__uint64)flacfilesize_;
	return ::FLAC__SEEKABLE_STREAM_DECODER_LENGTH_STATUS_OK;
}

bool SeekableStreamDecoder::eof_callback()
{
	if(error_occurred_)
		return true;

	return (bool)feof(file_);
}

::FLAC__StreamDecoderWriteStatus SeekableStreamDecoder::write_callback(const ::FLAC__Frame *frame, const FLAC__int32 * const buffer[])
{
	(void)buffer;

	return common_write_callback_(frame);
}

void SeekableStreamDecoder::metadata_callback(const ::FLAC__StreamMetadata *metadata)
{
	common_metadata_callback_(metadata);
}

void SeekableStreamDecoder::error_callback(::FLAC__StreamDecoderErrorStatus status)
{
	common_error_callback_(status);
}

bool SeekableStreamDecoder::die(const char *msg) const
{
	State state = get_state();

	if(msg)
		printf("FAILED, %s, state = %u (%s)\n", msg, (unsigned)((::FLAC__SeekableStreamDecoderState)state), state.as_cstring());
	else
		printf("FAILED, state = %u (%s)\n", (unsigned)((::FLAC__SeekableStreamDecoderState)state), state.as_cstring());

	return false;
}

bool SeekableStreamDecoder::test_respond()
{
	if(!set_md5_checking(true)) {
		printf("FAILED at set_md5_checking(), returned false\n");
		return false;
	}

	printf("testing init()... ");
	if(init() != ::FLAC__SEEKABLE_STREAM_DECODER_OK)
		return die();
	printf("OK\n");

	current_metadata_number_ = 0;

	if(::fseek(file_, 0, SEEK_SET) < 0) {
		printf("FAILED rewinding input, errno = %d\n", errno);
		return false;
	}

	printf("testing process_until_end_of_stream()... ");
	if(!process_until_end_of_stream()) {
		State state = get_state();
		printf("FAILED, returned false, state = %u (%s)\n", (unsigned)((::FLAC__SeekableStreamDecoderState)state), state.as_cstring());
		return false;
	}
	printf("OK\n");

	printf("testing finish()... ");
	finish();
	printf("OK\n");

	return true;
}

static bool test_seekable_stream_decoder()
{
	SeekableStreamDecoder *decoder;

	printf("\n+++ libFLAC++ unit test: FLAC::Decoder::SeekableStream\n\n");

	//
	// test new -> delete
	//
	printf("allocating decoder instance... ");
	decoder = new SeekableStreamDecoder();
	if(0 == decoder) {
		printf("FAILED, new returned NULL\n");
		return false;
	}
	printf("OK\n");

	printf("testing is_valid()... ");
	if(!decoder->is_valid()) {
		printf("FAILED, returned false\n");
		return false;
	}
	printf("OK\n");

	printf("freeing decoder instance... ");
	delete decoder;
	printf("OK\n");

	//
	// test new -> init -> delete
	//
	printf("allocating decoder instance... ");
	decoder = new SeekableStreamDecoder();
	if(0 == decoder) {
		printf("FAILED, new returned NULL\n");
		return false;
	}
	printf("OK\n");

	printf("testing is_valid()... ");
	if(!decoder->is_valid()) {
		printf("FAILED, returned false\n");
		return false;
	}
	printf("OK\n");

	printf("testing init()... ");
	if(decoder->init() != ::FLAC__SEEKABLE_STREAM_DECODER_OK)
		return decoder->die();
	printf("OK\n");

	printf("freeing decoder instance... ");
	delete decoder;
	printf("OK\n");

	//
	// test normal usage
	//
	num_expected_ = 0;
	expected_metadata_sequence_[num_expected_++] = &streaminfo_;

	printf("allocating decoder instance... ");
	decoder = new SeekableStreamDecoder();
	if(0 == decoder) {
		printf("FAILED, new returned NULL\n");
		return false;
	}
	printf("OK\n");

	printf("testing is_valid()... ");
	if(!decoder->is_valid()) {
		printf("FAILED, returned false\n");
		return false;
	}
	printf("OK\n");

	printf("testing set_md5_checking()... ");
	if(!decoder->set_md5_checking(true)) {
		printf("FAILED, returned false\n");
		return false;
	}
	printf("OK\n");

	printf("testing init()... ");
	if(decoder->init() != ::FLAC__SEEKABLE_STREAM_DECODER_OK)
		return decoder->die();
	printf("OK\n");

	printf("testing get_state()... ");
	FLAC::Decoder::SeekableStream::State state = decoder->get_state();
	printf("returned state = %u (%s)... OK\n", (unsigned)((::FLAC__SeekableStreamDecoderState)state), state.as_cstring());

	printf("testing get_stream_decoder_state()... ");
	FLAC::Decoder::Stream::State state_ = decoder->get_stream_decoder_state();
	printf("returned state = %u (%s)... OK\n", (unsigned)((::FLAC__StreamDecoderState)state_), state_.as_cstring());

	decoder->current_metadata_number_ = 0;
	decoder->ignore_errors_ = false;
	decoder->error_occurred_ = false;

	printf("opening FLAC file... ");
	decoder->file_ = ::fopen(flacfilename_, "rb");
	if(0 == decoder->file_) {
		printf("ERROR\n");
		return false;
	}
	printf("OK\n");

	printf("testing get_md5_checking()... ");
	if(!decoder->get_md5_checking()) {
		printf("FAILED, returned false, expected true\n");
		return false;
	}
	printf("OK\n");

	printf("testing process_until_end_of_metadata()... ");
	if(!decoder->process_until_end_of_metadata())
		return decoder->die("returned false");
	printf("OK\n");

	printf("testing process_single()... ");
	if(!decoder->process_single())
		return decoder->die("returned false");
	printf("OK\n");

	printf("testing flush()... ");
	if(!decoder->flush())
		return decoder->die("returned false");
	printf("OK\n");

	decoder->ignore_errors_ = true;
	printf("testing process_single()... ");
	if(!decoder->process_single())
		return decoder->die("returned false");
	printf("OK\n");
	decoder->ignore_errors_ = false;

	printf("testing seek_absolute()... ");
	if(!decoder->seek_absolute(0))
		return decoder->die("returned false");
	printf("OK\n");

	printf("testing process_until_end_of_stream()... ");
	if(!decoder->process_until_end_of_stream())
		return decoder->die("returned false");
	printf("OK\n");

	printf("testing get_channels()... ");
	{
		unsigned channels = decoder->get_channels();
		if(channels != streaminfo_.data.stream_info.channels) {
			printf("FAILED, returned %u, expected %u\n", channels, streaminfo_.data.stream_info.channels);
			return false;
		}
	}
	printf("OK\n");

	printf("testing get_bits_per_sample()... ");
	{
		unsigned bits_per_sample = decoder->get_bits_per_sample();
		if(bits_per_sample != streaminfo_.data.stream_info.bits_per_sample) {
			printf("FAILED, returned %u, expected %u\n", bits_per_sample, streaminfo_.data.stream_info.bits_per_sample);
			return false;
		}
	}
	printf("OK\n");

	printf("testing get_sample_rate()... ");
	{
		unsigned sample_rate = decoder->get_sample_rate();
		if(sample_rate != streaminfo_.data.stream_info.sample_rate) {
			printf("FAILED, returned %u, expected %u\n", sample_rate, streaminfo_.data.stream_info.sample_rate);
			return false;
		}
	}
	printf("OK\n");

	printf("testing get_blocksize()... ");
	{
		unsigned blocksize = decoder->get_blocksize();
		/* value could be anything since we're at the last block, so accept any answer */
		printf("returned %u... OK\n", blocksize);
	}

	printf("testing get_channel_assignment()... ");
	{
		::FLAC__ChannelAssignment ca = decoder->get_channel_assignment();
		printf("returned %u (%s)... OK\n", (unsigned)ca, ::FLAC__ChannelAssignmentString[ca]);
	}

	printf("testing reset()... ");
	if(!decoder->reset())
		return decoder->die("returned false");
	printf("OK\n");

	decoder->current_metadata_number_ = 0;

	printf("rewinding input... ");
	if(::fseek(decoder->file_, 0, SEEK_SET) < 0) {
		printf("FAILED, errno = %d\n", errno);
		return false;
	}
	printf("OK\n");

	printf("testing process_until_end_of_stream()... ");
	if(!decoder->process_until_end_of_stream())
		return decoder->die("returned false");
	printf("OK\n");

	printf("testing finish()... ");
	decoder->finish();
	printf("OK\n");

	/*
	 * respond all
	 */

	printf("testing set_metadata_respond_all()... ");
	if(!decoder->set_metadata_respond_all()) {
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

	if(!decoder->test_respond())
		return false;

	/*
	 * ignore all
	 */

	printf("testing set_metadata_ignore_all()... ");
	if(!decoder->set_metadata_ignore_all()) {
		printf("FAILED, returned false\n");
		return false;
	}
	printf("OK\n");

	num_expected_ = 0;

	if(!decoder->test_respond())
		return false;

	/*
	 * respond all, ignore VORBIS_COMMENT
	 */

	printf("testing set_metadata_respond_all()... ");
	if(!decoder->set_metadata_respond_all()) {
		printf("FAILED, returned false\n");
		return false;
	}
	printf("OK\n");

	printf("testing set_metadata_ignore(VORBIS_COMMENT)... ");
	if(!decoder->set_metadata_ignore(FLAC__METADATA_TYPE_VORBIS_COMMENT)) {
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

	if(!decoder->test_respond())
		return false;

	/*
	 * respond all, ignore APPLICATION
	 */

	printf("testing set_metadata_respond_all()... ");
	if(!decoder->set_metadata_respond_all()) {
		printf("FAILED, returned false\n");
		return false;
	}
	printf("OK\n");

	printf("testing set_metadata_ignore(APPLICATION)... ");
	if(!decoder->set_metadata_ignore(FLAC__METADATA_TYPE_APPLICATION)) {
		printf("FAILED, returned false\n");
		return false;
	}
	printf("OK\n");

	num_expected_ = 0;
	expected_metadata_sequence_[num_expected_++] = &streaminfo_;
	expected_metadata_sequence_[num_expected_++] = &padding_;
	expected_metadata_sequence_[num_expected_++] = &seektable_;
	expected_metadata_sequence_[num_expected_++] = &vorbiscomment_;

	if(!decoder->test_respond())
		return false;

	/*
	 * respond all, ignore APPLICATION id of app#1
	 */

	printf("testing set_metadata_respond_all()... ");
	if(!decoder->set_metadata_respond_all()) {
		printf("FAILED, returned false\n");
		return false;
	}
	printf("OK\n");

	printf("testing set_metadata_ignore_application(of app block #1)... ");
	if(!decoder->set_metadata_ignore_application(application1_.data.application.id)) {
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

	if(!decoder->test_respond())
		return false;

	/*
	 * respond all, ignore APPLICATION id of app#1 & app#2
	 */

	printf("testing set_metadata_respond_all()... ");
	if(!decoder->set_metadata_respond_all()) {
		printf("FAILED, returned false\n");
		return false;
	}
	printf("OK\n");

	printf("testing set_metadata_ignore_application(of app block #1)... ");
	if(!decoder->set_metadata_ignore_application(application1_.data.application.id)) {
		printf("FAILED, returned false\n");
		return false;
	}
	printf("OK\n");

	printf("testing set_metadata_ignore_application(of app block #2)... ");
	if(!decoder->set_metadata_ignore_application(application2_.data.application.id)) {
		printf("FAILED, returned false\n");
		return false;
	}
	printf("OK\n");

	num_expected_ = 0;
	expected_metadata_sequence_[num_expected_++] = &streaminfo_;
	expected_metadata_sequence_[num_expected_++] = &padding_;
	expected_metadata_sequence_[num_expected_++] = &seektable_;
	expected_metadata_sequence_[num_expected_++] = &vorbiscomment_;

	if(!decoder->test_respond())
		return false;

	/*
	 * ignore all, respond VORBIS_COMMENT
	 */

	printf("testing set_metadata_ignore_all()... ");
	if(!decoder->set_metadata_ignore_all()) {
		printf("FAILED, returned false\n");
		return false;
	}
	printf("OK\n");

	printf("testing set_metadata_respond(VORBIS_COMMENT)... ");
	if(!decoder->set_metadata_respond(FLAC__METADATA_TYPE_VORBIS_COMMENT)) {
		printf("FAILED, returned false\n");
		return false;
	}
	printf("OK\n");

	num_expected_ = 0;
	expected_metadata_sequence_[num_expected_++] = &vorbiscomment_;

	if(!decoder->test_respond())
		return false;

	/*
	 * ignore all, respond APPLICATION
	 */

	printf("testing set_metadata_ignore_all()... ");
	if(!decoder->set_metadata_ignore_all()) {
		printf("FAILED, returned false\n");
		return false;
	}
	printf("OK\n");

	printf("testing set_metadata_respond(APPLICATION)... ");
	if(!decoder->set_metadata_respond(FLAC__METADATA_TYPE_APPLICATION)) {
		printf("FAILED, returned false\n");
		return false;
	}
	printf("OK\n");

	num_expected_ = 0;
	expected_metadata_sequence_[num_expected_++] = &application1_;
	expected_metadata_sequence_[num_expected_++] = &application2_;

	if(!decoder->test_respond())
		return false;

	/*
	 * ignore all, respond APPLICATION id of app#1
	 */

	printf("testing set_metadata_ignore_all()... ");
	if(!decoder->set_metadata_ignore_all()) {
		printf("FAILED, returned false\n");
		return false;
	}
	printf("OK\n");

	printf("testing set_metadata_respond_application(of app block #1)... ");
	if(!decoder->set_metadata_respond_application(application1_.data.application.id)) {
		printf("FAILED, returned false\n");
		return false;
	}
	printf("OK\n");

	num_expected_ = 0;
	expected_metadata_sequence_[num_expected_++] = &application1_;

	if(!decoder->test_respond())
		return false;

	/*
	 * ignore all, respond APPLICATION id of app#1 & app#2
	 */

	printf("testing set_metadata_ignore_all()... ");
	if(!decoder->set_metadata_ignore_all()) {
		printf("FAILED, returned false\n");
		return false;
	}
	printf("OK\n");

	printf("testing set_metadata_respond_application(of app block #1)... ");
	if(!decoder->set_metadata_respond_application(application1_.data.application.id)) {
		printf("FAILED, returned false\n");
		return false;
	}
	printf("OK\n");

	printf("testing set_metadata_respond_application(of app block #2)... ");
	if(!decoder->set_metadata_respond_application(application2_.data.application.id)) {
		printf("FAILED, returned false\n");
		return false;
	}
	printf("OK\n");

	num_expected_ = 0;
	expected_metadata_sequence_[num_expected_++] = &application1_;
	expected_metadata_sequence_[num_expected_++] = &application2_;

	if(!decoder->test_respond())
		return false;

	/*
	 * respond all, ignore APPLICATION, respond APPLICATION id of app#1
	 */

	printf("testing set_metadata_respond_all()... ");
	if(!decoder->set_metadata_respond_all()) {
		printf("FAILED, returned false\n");
		return false;
	}
	printf("OK\n");

	printf("testing set_metadata_ignore(APPLICATION)... ");
	if(!decoder->set_metadata_ignore(FLAC__METADATA_TYPE_APPLICATION)) {
		printf("FAILED, returned false\n");
		return false;
	}
	printf("OK\n");

	printf("testing set_metadata_respond_application(of app block #1)... ");
	if(!decoder->set_metadata_respond_application(application1_.data.application.id)) {
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

	if(!decoder->test_respond())
		return false;

	/*
	 * ignore all, respond APPLICATION, ignore APPLICATION id of app#1
	 */

	printf("testing set_metadata_ignore_all()... ");
	if(!decoder->set_metadata_ignore_all()) {
		printf("FAILED, returned false\n");
		return false;
	}
	printf("OK\n");

	printf("testing set_metadata_respond(APPLICATION)... ");
	if(!decoder->set_metadata_respond(FLAC__METADATA_TYPE_APPLICATION)) {
		printf("FAILED, returned false\n");
		return false;
	}
	printf("OK\n");

	printf("testing set_metadata_ignore_application(of app block #1)... ");
	if(!decoder->set_metadata_ignore_application(application1_.data.application.id)) {
		printf("FAILED, returned false\n");
		return false;
	}
	printf("OK\n");

	num_expected_ = 0;
	expected_metadata_sequence_[num_expected_++] = &application2_;

	if(!decoder->test_respond())
		return false;

	/* done, now leave the sequence the way we found it... */
	num_expected_ = 0;
	expected_metadata_sequence_[num_expected_++] = &streaminfo_;
	expected_metadata_sequence_[num_expected_++] = &padding_;
	expected_metadata_sequence_[num_expected_++] = &seektable_;
	expected_metadata_sequence_[num_expected_++] = &application1_;
	expected_metadata_sequence_[num_expected_++] = &application2_;
	expected_metadata_sequence_[num_expected_++] = &vorbiscomment_;

	::fclose(decoder->file_);

	printf("freeing decoder instance... ");
	delete decoder;
	printf("OK\n");

	printf("\nPASSED!\n");

	return true;
}

class FileDecoder : public FLAC::Decoder::File, public DecoderCommon {
public:
	FileDecoder(): FLAC::Decoder::File(), DecoderCommon() { }
	~FileDecoder() { }

	// from FLAC::Decoder::File
	::FLAC__StreamDecoderWriteStatus write_callback(const ::FLAC__Frame *frame, const FLAC__int32 * const buffer[]);
	void metadata_callback(const ::FLAC__StreamMetadata *metadata);
	void error_callback(::FLAC__StreamDecoderErrorStatus status);

	bool die(const char *msg = 0) const;

	bool test_respond();
};

::FLAC__StreamDecoderWriteStatus FileDecoder::write_callback(const ::FLAC__Frame *frame, const FLAC__int32 * const buffer[])
{
	(void)buffer;
	return common_write_callback_(frame);
}

void FileDecoder::metadata_callback(const ::FLAC__StreamMetadata *metadata)
{
	common_metadata_callback_(metadata);
}

void FileDecoder::error_callback(::FLAC__StreamDecoderErrorStatus status)
{
	common_error_callback_(status);
}

bool FileDecoder::die(const char *msg) const
{
	State state = get_state();

	if(msg)
		printf("FAILED, %s, state = %u (%s)\n", msg, (unsigned)((::FLAC__FileDecoderState)state), state.as_cstring());
	else
		printf("FAILED, state = %u (%s)\n", (unsigned)((::FLAC__FileDecoderState)state), state.as_cstring());

	return false;
}

bool FileDecoder::test_respond()
{
	if(!set_filename(flacfilename_)) {
		printf("FAILED at set_filename(), returned false\n");
		return false;
	}

	if(!set_md5_checking(true)) {
		printf("FAILED at set_md5_checking(), returned false\n");
		return false;
	}

	printf("testing init()... ");
	if(init() != ::FLAC__FILE_DECODER_OK)
		return die();
	printf("OK\n");

	current_metadata_number_ = 0;

	printf("testing process_until_end_of_file()... ");
	if(!process_until_end_of_file()) {
		State state = get_state();
		printf("FAILED, returned false, state = %u (%s)\n", (unsigned)((::FLAC__FileDecoderState)state), state.as_cstring());
		return false;
	}
	printf("OK\n");

	printf("testing finish()... ");
	finish();
	printf("OK\n");

	return true;
}

static bool test_file_decoder()
{
	FileDecoder *decoder;

	printf("\n+++ libFLAC++ unit test: FLAC::Decoder::File\n\n");

	//
	// test new -> delete
	//
	printf("allocating decoder instance... ");
	decoder = new FileDecoder();
	if(0 == decoder) {
		printf("FAILED, new returned NULL\n");
		return false;
	}
	printf("OK\n");

	printf("testing is_valid()... ");
	if(!decoder->is_valid()) {
		printf("FAILED, returned false\n");
		return false;
	}
	printf("OK\n");

	printf("freeing decoder instance... ");
	delete decoder;
	printf("OK\n");

	//
	// test new -> init -> delete
	//
	printf("allocating decoder instance... ");
	decoder = new FileDecoder();
	if(0 == decoder) {
		printf("FAILED, new returned NULL\n");
		return false;
	}
	printf("OK\n");

	printf("testing is_valid()... ");
	if(!decoder->is_valid()) {
		printf("FAILED, returned false\n");
		return false;
	}
	printf("OK\n");

	printf("testing init()... ");
	if(decoder->init() != ::FLAC__SEEKABLE_STREAM_DECODER_OK)
		return decoder->die();
	printf("OK\n");

	printf("freeing decoder instance... ");
	delete decoder;
	printf("OK\n");

	//
	// test normal usage
	//
	num_expected_ = 0;
	expected_metadata_sequence_[num_expected_++] = &streaminfo_;

	printf("allocating decoder instance... ");
	decoder = new FileDecoder();
	if(0 == decoder) {
		printf("FAILED, new returned NULL\n");
		return false;
	}
	printf("OK\n");

	printf("testing is_valid()... ");
	if(!decoder->is_valid()) {
		printf("FAILED, returned false\n");
		return false;
	}
	printf("OK\n");

	printf("testing set_filename()... ");
	if(!decoder->set_filename(flacfilename_)) {
		printf("FAILED, returned false\n");
		return false;
	}
	printf("OK\n");

	printf("testing set_md5_checking()... ");
	if(!decoder->set_md5_checking(true)) {
		printf("FAILED, returned false\n");
		return false;
	}
	printf("OK\n");

	printf("testing init()... ");
	if(decoder->init() != ::FLAC__FILE_DECODER_OK)
		return decoder->die();
	printf("OK\n");

	printf("testing get_state()... ");
	FLAC::Decoder::File::State state = decoder->get_state();
	printf("returned state = %u (%s)... OK\n", (unsigned)((::FLAC__FileDecoderState)state), state.as_cstring());

	printf("testing get_seekable_stream_decoder_state()... ");
	FLAC::Decoder::SeekableStream::State state_ = decoder->get_seekable_stream_decoder_state();
	printf("returned state = %u (%s)... OK\n", (unsigned)((::FLAC__SeekableStreamDecoderState)state_), state_.as_cstring());

	printf("testing get_stream_decoder_state()... ");
	FLAC::Decoder::Stream::State state__ = decoder->get_stream_decoder_state();
	printf("returned state = %u (%s)... OK\n", (unsigned)((::FLAC__StreamDecoderState)state__), state__.as_cstring());

	decoder->current_metadata_number_ = 0;
	decoder->ignore_errors_ = false;
	decoder->error_occurred_ = false;

	printf("testing get_md5_checking()... ");
	if(!decoder->get_md5_checking()) {
		printf("FAILED, returned false, expected true\n");
		return false;
	}
	printf("OK\n");

	printf("testing process_until_end_of_metadata()... ");
	if(!decoder->process_until_end_of_metadata())
		return decoder->die("returned false");
	printf("OK\n");

	printf("testing process_single()... ");
	if(!decoder->process_single())
		return decoder->die("returned false");
	printf("OK\n");

	printf("testing seek_absolute()... ");
	if(!decoder->seek_absolute(0))
		return decoder->die("returned false");
	printf("OK\n");

	printf("testing process_until_end_of_file()... ");
	if(!decoder->process_until_end_of_file())
		return decoder->die("returned false");
	printf("OK\n");

	printf("testing get_channels()... ");
	{
		unsigned channels = decoder->get_channels();
		if(channels != streaminfo_.data.stream_info.channels) {
			printf("FAILED, returned %u, expected %u\n", channels, streaminfo_.data.stream_info.channels);
			return false;
		}
	}
	printf("OK\n");

	printf("testing get_bits_per_sample()... ");
	{
		unsigned bits_per_sample = decoder->get_bits_per_sample();
		if(bits_per_sample != streaminfo_.data.stream_info.bits_per_sample) {
			printf("FAILED, returned %u, expected %u\n", bits_per_sample, streaminfo_.data.stream_info.bits_per_sample);
			return false;
		}
	}
	printf("OK\n");

	printf("testing get_sample_rate()... ");
	{
		unsigned sample_rate = decoder->get_sample_rate();
		if(sample_rate != streaminfo_.data.stream_info.sample_rate) {
			printf("FAILED, returned %u, expected %u\n", sample_rate, streaminfo_.data.stream_info.sample_rate);
			return false;
		}
	}
	printf("OK\n");

	printf("testing get_blocksize()... ");
	{
		unsigned blocksize = decoder->get_blocksize();
		/* value could be anything since we're at the last block, so accept any answer */
		printf("returned %u... OK\n", blocksize);
	}

	printf("testing get_channel_assignment()... ");
	{
		::FLAC__ChannelAssignment ca = decoder->get_channel_assignment();
		printf("returned %u (%s)... OK\n", (unsigned)ca, ::FLAC__ChannelAssignmentString[ca]);
	}

	printf("testing finish()... ");
	decoder->finish();
	printf("OK\n");

	/*
	 * respond all
	 */

	printf("testing set_metadata_respond_all()... ");
	if(!decoder->set_metadata_respond_all()) {
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

	if(!decoder->test_respond())
		return false;

	/*
	 * ignore all
	 */

	printf("testing set_metadata_ignore_all()... ");
	if(!decoder->set_metadata_ignore_all()) {
		printf("FAILED, returned false\n");
		return false;
	}
	printf("OK\n");

	num_expected_ = 0;

	if(!decoder->test_respond())
		return false;

	/*
	 * respond all, ignore VORBIS_COMMENT
	 */

	printf("testing set_metadata_respond_all()... ");
	if(!decoder->set_metadata_respond_all()) {
		printf("FAILED, returned false\n");
		return false;
	}
	printf("OK\n");

	printf("testing set_metadata_ignore(VORBIS_COMMENT)... ");
	if(!decoder->set_metadata_ignore(FLAC__METADATA_TYPE_VORBIS_COMMENT)) {
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

	if(!decoder->test_respond())
		return false;

	/*
	 * respond all, ignore APPLICATION
	 */

	printf("testing set_metadata_respond_all()... ");
	if(!decoder->set_metadata_respond_all()) {
		printf("FAILED, returned false\n");
		return false;
	}
	printf("OK\n");

	printf("testing set_metadata_ignore(APPLICATION)... ");
	if(!decoder->set_metadata_ignore(FLAC__METADATA_TYPE_APPLICATION)) {
		printf("FAILED, returned false\n");
		return false;
	}
	printf("OK\n");

	num_expected_ = 0;
	expected_metadata_sequence_[num_expected_++] = &streaminfo_;
	expected_metadata_sequence_[num_expected_++] = &padding_;
	expected_metadata_sequence_[num_expected_++] = &seektable_;
	expected_metadata_sequence_[num_expected_++] = &vorbiscomment_;

	if(!decoder->test_respond())
		return false;

	/*
	 * respond all, ignore APPLICATION id of app#1
	 */

	printf("testing set_metadata_respond_all()... ");
	if(!decoder->set_metadata_respond_all()) {
		printf("FAILED, returned false\n");
		return false;
	}
	printf("OK\n");

	printf("testing set_metadata_ignore_application(of app block #1)... ");
	if(!decoder->set_metadata_ignore_application(application1_.data.application.id)) {
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

	if(!decoder->test_respond())
		return false;

	/*
	 * respond all, ignore APPLICATION id of app#1 & app#2
	 */

	printf("testing set_metadata_respond_all()... ");
	if(!decoder->set_metadata_respond_all()) {
		printf("FAILED, returned false\n");
		return false;
	}
	printf("OK\n");

	printf("testing set_metadata_ignore_application(of app block #1)... ");
	if(!decoder->set_metadata_ignore_application(application1_.data.application.id)) {
		printf("FAILED, returned false\n");
		return false;
	}
	printf("OK\n");

	printf("testing set_metadata_ignore_application(of app block #2)... ");
	if(!decoder->set_metadata_ignore_application(application2_.data.application.id)) {
		printf("FAILED, returned false\n");
		return false;
	}
	printf("OK\n");

	num_expected_ = 0;
	expected_metadata_sequence_[num_expected_++] = &streaminfo_;
	expected_metadata_sequence_[num_expected_++] = &padding_;
	expected_metadata_sequence_[num_expected_++] = &seektable_;
	expected_metadata_sequence_[num_expected_++] = &vorbiscomment_;

	if(!decoder->test_respond())
		return false;

	/*
	 * ignore all, respond VORBIS_COMMENT
	 */

	printf("testing set_metadata_ignore_all()... ");
	if(!decoder->set_metadata_ignore_all()) {
		printf("FAILED, returned false\n");
		return false;
	}
	printf("OK\n");

	printf("testing set_metadata_respond(VORBIS_COMMENT)... ");
	if(!decoder->set_metadata_respond(FLAC__METADATA_TYPE_VORBIS_COMMENT)) {
		printf("FAILED, returned false\n");
		return false;
	}
	printf("OK\n");

	num_expected_ = 0;
	expected_metadata_sequence_[num_expected_++] = &vorbiscomment_;

	if(!decoder->test_respond())
		return false;

	/*
	 * ignore all, respond APPLICATION
	 */

	printf("testing set_metadata_ignore_all()... ");
	if(!decoder->set_metadata_ignore_all()) {
		printf("FAILED, returned false\n");
		return false;
	}
	printf("OK\n");

	printf("testing set_metadata_respond(APPLICATION)... ");
	if(!decoder->set_metadata_respond(FLAC__METADATA_TYPE_APPLICATION)) {
		printf("FAILED, returned false\n");
		return false;
	}
	printf("OK\n");

	num_expected_ = 0;
	expected_metadata_sequence_[num_expected_++] = &application1_;
	expected_metadata_sequence_[num_expected_++] = &application2_;

	if(!decoder->test_respond())
		return false;

	/*
	 * ignore all, respond APPLICATION id of app#1
	 */

	printf("testing set_metadata_ignore_all()... ");
	if(!decoder->set_metadata_ignore_all()) {
		printf("FAILED, returned false\n");
		return false;
	}
	printf("OK\n");

	printf("testing set_metadata_respond_application(of app block #1)... ");
	if(!decoder->set_metadata_respond_application(application1_.data.application.id)) {
		printf("FAILED, returned false\n");
		return false;
	}
	printf("OK\n");

	num_expected_ = 0;
	expected_metadata_sequence_[num_expected_++] = &application1_;

	if(!decoder->test_respond())
		return false;

	/*
	 * ignore all, respond APPLICATION id of app#1 & app#2
	 */

	printf("testing set_metadata_ignore_all()... ");
	if(!decoder->set_metadata_ignore_all()) {
		printf("FAILED, returned false\n");
		return false;
	}
	printf("OK\n");

	printf("testing set_metadata_respond_application(of app block #1)... ");
	if(!decoder->set_metadata_respond_application(application1_.data.application.id)) {
		printf("FAILED, returned false\n");
		return false;
	}
	printf("OK\n");

	printf("testing set_metadata_respond_application(of app block #2)... ");
	if(!decoder->set_metadata_respond_application(application2_.data.application.id)) {
		printf("FAILED, returned false\n");
		return false;
	}
	printf("OK\n");

	num_expected_ = 0;
	expected_metadata_sequence_[num_expected_++] = &application1_;
	expected_metadata_sequence_[num_expected_++] = &application2_;

	if(!decoder->test_respond())
		return false;

	/*
	 * respond all, ignore APPLICATION, respond APPLICATION id of app#1
	 */

	printf("testing set_metadata_respond_all()... ");
	if(!decoder->set_metadata_respond_all()) {
		printf("FAILED, returned false\n");
		return false;
	}
	printf("OK\n");

	printf("testing set_metadata_ignore(APPLICATION)... ");
	if(!decoder->set_metadata_ignore(FLAC__METADATA_TYPE_APPLICATION)) {
		printf("FAILED, returned false\n");
		return false;
	}
	printf("OK\n");

	printf("testing set_metadata_respond_application(of app block #1)... ");
	if(!decoder->set_metadata_respond_application(application1_.data.application.id)) {
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

	if(!decoder->test_respond())
		return false;

	/*
	 * ignore all, respond APPLICATION, ignore APPLICATION id of app#1
	 */

	printf("testing set_metadata_ignore_all()... ");
	if(!decoder->set_metadata_ignore_all()) {
		printf("FAILED, returned false\n");
		return false;
	}
	printf("OK\n");

	printf("testing set_metadata_respond(APPLICATION)... ");
	if(!decoder->set_metadata_respond(FLAC__METADATA_TYPE_APPLICATION)) {
		printf("FAILED, returned false\n");
		return false;
	}
	printf("OK\n");

	printf("testing set_metadata_ignore_application(of app block #1)... ");
	if(!decoder->set_metadata_ignore_application(application1_.data.application.id)) {
		printf("FAILED, returned false\n");
		return false;
	}
	printf("OK\n");

	num_expected_ = 0;
	expected_metadata_sequence_[num_expected_++] = &application2_;

	if(!decoder->test_respond())
		return false;

	/* done, now leave the sequence the way we found it... */
	num_expected_ = 0;
	expected_metadata_sequence_[num_expected_++] = &streaminfo_;
	expected_metadata_sequence_[num_expected_++] = &padding_;
	expected_metadata_sequence_[num_expected_++] = &seektable_;
	expected_metadata_sequence_[num_expected_++] = &application1_;
	expected_metadata_sequence_[num_expected_++] = &application2_;
	expected_metadata_sequence_[num_expected_++] = &vorbiscomment_;

	printf("freeing decoder instance... ");
	delete decoder;
	printf("OK\n");

	printf("\nPASSED!\n");

	return true;
}

bool test_decoders()
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

	(void) file_utils__remove_file(flacfilename_);
	free_metadata_blocks_();

	return true;
}
