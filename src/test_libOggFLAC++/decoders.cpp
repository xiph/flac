/* test_libOggFLAC++ - Unit tester for libOggFLAC++
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
extern "C" {
#include "file_utils.h"
#include "metadata_utils.h"
}
#include "FLAC/assert.h"
#include "FLAC/metadata.h" // for ::FLAC__metadata_object_is_equal()
#include "OggFLAC++/decoder.h"
#include "share/grabbag.h"
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _MSC_VER
// warning C4800: 'int' : forcing to bool 'true' or 'false' (performance warning)
#pragma warning ( disable : 4800 )
#endif

static ::FLAC__StreamMetadata streaminfo_, padding_, seektable_, application1_, application2_, vorbiscomment_, cuesheet_, unknown_;
static ::FLAC__StreamMetadata *expected_metadata_sequence_[8];
static unsigned num_expected_;
static const char *oggflacfilename_ = "metadata.ogg";
static unsigned oggflacfilesize_;

static bool die_(const char *msg)
{
	printf("ERROR: %s\n", msg);
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

static bool generate_file_()
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

	if(feof(file_)) {
		*bytes = 0;
		return ::FLAC__STREAM_DECODER_READ_STATUS_END_OF_STREAM;
	}
	else if(*bytes > 0) {
		*bytes = ::fread(buffer, 1, *bytes, file_);
		if(*bytes == 0) {
			if(feof(file_))
				return ::FLAC__STREAM_DECODER_READ_STATUS_END_OF_STREAM;
			else
				return ::FLAC__STREAM_DECODER_READ_STATUS_CONTINUE;
		}
		else {
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

class StreamDecoder : public OggFLAC::Decoder::Stream, public DecoderCommon {
public:
	StreamDecoder(): OggFLAC::Decoder::Stream(), DecoderCommon() { }
	~StreamDecoder() { }

	// from OggFLAC::Decoder::Stream
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
		printf("FAILED, %s", msg);
	else
		printf("FAILED");

	printf(", state = %u (%s)\n", (unsigned)((::OggFLAC__StreamDecoderState)state), state.as_cstring());

	return false;
}

bool StreamDecoder::test_respond()
{
	printf("testing init()... ");
	if(init() != ::OggFLAC__STREAM_DECODER_OK)
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
		printf("FAILED, returned false, state = %u (%s)\n", (unsigned)((::OggFLAC__StreamDecoderState)state), state.as_cstring());
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

	printf("\n+++ libOggFLAC++ unit test: OggFLAC::Decoder::Stream\n\n");

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
	if(decoder->init() != ::OggFLAC__STREAM_DECODER_OK)
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

	printf("testing set_serial_number()... ");
	if(!decoder->set_serial_number(file_utils__serial_number))
		return decoder->die("returned false");
	printf("OK\n");

	printf("testing init()... ");
	if(decoder->init() != ::OggFLAC__STREAM_DECODER_OK)
		return decoder->die();
	printf("OK\n");

	printf("testing get_state()... ");
	OggFLAC::Decoder::Stream::State state = decoder->get_state();
	printf("returned state = %u (%s)... OK\n", (unsigned)((::OggFLAC__StreamDecoderState)state), state.as_cstring());

	printf("testing get_FLAC_stream_decoder_state()... ");
	FLAC::Decoder::Stream::State state_ = decoder->get_FLAC_stream_decoder_state();
	printf("returned state = %u (%s)... OK\n", (unsigned)((::FLAC__StreamDecoderState)state_), state_.as_cstring());

	decoder->current_metadata_number_ = 0;
	decoder->ignore_errors_ = false;
	decoder->error_occurred_ = false;

	printf("opening Ogg FLAC file... ");
	decoder->file_ = ::fopen(oggflacfilename_, "rb");
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
	expected_metadata_sequence_[num_expected_++] = &vorbiscomment_;
	expected_metadata_sequence_[num_expected_++] = &padding_;
	expected_metadata_sequence_[num_expected_++] = &seektable_;
	expected_metadata_sequence_[num_expected_++] = &application1_;
	expected_metadata_sequence_[num_expected_++] = &application2_;
	expected_metadata_sequence_[num_expected_++] = &cuesheet_;
	expected_metadata_sequence_[num_expected_++] = &unknown_;

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
	expected_metadata_sequence_[num_expected_++] = &cuesheet_;
	expected_metadata_sequence_[num_expected_++] = &unknown_;

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
	expected_metadata_sequence_[num_expected_++] = &vorbiscomment_;
	expected_metadata_sequence_[num_expected_++] = &padding_;
	expected_metadata_sequence_[num_expected_++] = &seektable_;
	expected_metadata_sequence_[num_expected_++] = &cuesheet_;
	expected_metadata_sequence_[num_expected_++] = &unknown_;

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
	expected_metadata_sequence_[num_expected_++] = &vorbiscomment_;
	expected_metadata_sequence_[num_expected_++] = &padding_;
	expected_metadata_sequence_[num_expected_++] = &seektable_;
	expected_metadata_sequence_[num_expected_++] = &application2_;
	expected_metadata_sequence_[num_expected_++] = &cuesheet_;
	expected_metadata_sequence_[num_expected_++] = &unknown_;

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
	expected_metadata_sequence_[num_expected_++] = &vorbiscomment_;
	expected_metadata_sequence_[num_expected_++] = &padding_;
	expected_metadata_sequence_[num_expected_++] = &seektable_;
	expected_metadata_sequence_[num_expected_++] = &cuesheet_;
	expected_metadata_sequence_[num_expected_++] = &unknown_;

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
	expected_metadata_sequence_[num_expected_++] = &vorbiscomment_;
	expected_metadata_sequence_[num_expected_++] = &padding_;
	expected_metadata_sequence_[num_expected_++] = &seektable_;
	expected_metadata_sequence_[num_expected_++] = &application1_;
	expected_metadata_sequence_[num_expected_++] = &cuesheet_;
	expected_metadata_sequence_[num_expected_++] = &unknown_;

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
	expected_metadata_sequence_[num_expected_++] = &vorbiscomment_;
	expected_metadata_sequence_[num_expected_++] = &padding_;
	expected_metadata_sequence_[num_expected_++] = &seektable_;
	expected_metadata_sequence_[num_expected_++] = &application1_;
	expected_metadata_sequence_[num_expected_++] = &application2_;
	expected_metadata_sequence_[num_expected_++] = &cuesheet_;
	expected_metadata_sequence_[num_expected_++] = &unknown_;

	::fclose(decoder->file_);

	printf("freeing decoder instance... ");
	delete decoder;
	printf("OK\n");

	printf("\nPASSED!\n");

	return true;
}

class SeekableStreamDecoder : public OggFLAC::Decoder::SeekableStream, public DecoderCommon {
public:
	SeekableStreamDecoder(): OggFLAC::Decoder::SeekableStream(), DecoderCommon() { }
	~SeekableStreamDecoder() { }

	// from OggFLAC::Decoder::SeekableStream
	::OggFLAC__SeekableStreamDecoderReadStatus read_callback(FLAC__byte buffer[], unsigned *bytes);
	::OggFLAC__SeekableStreamDecoderSeekStatus seek_callback(FLAC__uint64 absolute_byte_offset);
	::OggFLAC__SeekableStreamDecoderTellStatus tell_callback(FLAC__uint64 *absolute_byte_offset);
	::OggFLAC__SeekableStreamDecoderLengthStatus length_callback(FLAC__uint64 *stream_length);
	bool eof_callback();
	::FLAC__StreamDecoderWriteStatus write_callback(const ::FLAC__Frame *frame, const FLAC__int32 * const buffer[]);
	void metadata_callback(const ::FLAC__StreamMetadata *metadata);
	void error_callback(::FLAC__StreamDecoderErrorStatus status);

	bool die(const char *msg = 0) const;

	bool test_respond();
};

::OggFLAC__SeekableStreamDecoderReadStatus SeekableStreamDecoder::read_callback(FLAC__byte buffer[], unsigned *bytes)
{
	switch(common_read_callback_(buffer, bytes)) {
		case ::FLAC__STREAM_DECODER_READ_STATUS_CONTINUE:
		case ::FLAC__STREAM_DECODER_READ_STATUS_END_OF_STREAM:
			return ::OggFLAC__SEEKABLE_STREAM_DECODER_READ_STATUS_OK;
		case ::FLAC__STREAM_DECODER_READ_STATUS_ABORT:
			return ::OggFLAC__SEEKABLE_STREAM_DECODER_READ_STATUS_ERROR;
		default:
			FLAC__ASSERT(0);
			return ::OggFLAC__SEEKABLE_STREAM_DECODER_READ_STATUS_ERROR;
	}
}

::OggFLAC__SeekableStreamDecoderSeekStatus SeekableStreamDecoder::seek_callback(FLAC__uint64 absolute_byte_offset)
{
	if(error_occurred_)
		return ::OggFLAC__SEEKABLE_STREAM_DECODER_SEEK_STATUS_ERROR;

	if(::fseek(file_, (long)absolute_byte_offset, SEEK_SET) < 0) {
		error_occurred_ = true;
		return ::OggFLAC__SEEKABLE_STREAM_DECODER_SEEK_STATUS_ERROR;
	}

	return ::OggFLAC__SEEKABLE_STREAM_DECODER_SEEK_STATUS_OK;
}

::OggFLAC__SeekableStreamDecoderTellStatus SeekableStreamDecoder::tell_callback(FLAC__uint64 *absolute_byte_offset)
{
	if(error_occurred_)
		return ::OggFLAC__SEEKABLE_STREAM_DECODER_TELL_STATUS_ERROR;

	long offset = ::ftell(file_);
	*absolute_byte_offset = (FLAC__uint64)offset;

	if(offset < 0) {
		error_occurred_ = true;
		return ::OggFLAC__SEEKABLE_STREAM_DECODER_TELL_STATUS_ERROR;
	}

	return ::OggFLAC__SEEKABLE_STREAM_DECODER_TELL_STATUS_OK;
}

::OggFLAC__SeekableStreamDecoderLengthStatus SeekableStreamDecoder::length_callback(FLAC__uint64 *stream_length)
{
	if(error_occurred_)
		return ::OggFLAC__SEEKABLE_STREAM_DECODER_LENGTH_STATUS_ERROR;

	*stream_length = (FLAC__uint64)oggflacfilesize_;
	return ::OggFLAC__SEEKABLE_STREAM_DECODER_LENGTH_STATUS_OK;
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
		printf("FAILED, %s", msg);
	else
		printf("FAILED");

	printf(", state = %u (%s)\n", (unsigned)((::OggFLAC__SeekableStreamDecoderState)state), state.as_cstring());
	if(state == ::OggFLAC__SEEKABLE_STREAM_DECODER_STREAM_DECODER_ERROR) {
		OggFLAC::Decoder::Stream::State state_ = get_stream_decoder_state();
		printf("      stream decoder state = %u (%s)\n", (unsigned)((::OggFLAC__StreamDecoderState)state_), state_.as_cstring());
		if(state_ == ::OggFLAC__STREAM_DECODER_FLAC_STREAM_DECODER_ERROR) {
			FLAC::Decoder::Stream::State state__ = get_FLAC_stream_decoder_state();
			printf("      FLAC stream decoder state = %u (%s)\n", (unsigned)((::FLAC__StreamDecoderState)state__), state__.as_cstring());
		}
	}

	return false;
}

bool SeekableStreamDecoder::test_respond()
{
	if(!set_md5_checking(true)) {
		printf("FAILED at set_md5_checking(), returned false\n");
		return false;
	}

	printf("testing init()... ");
	if(init() != ::OggFLAC__SEEKABLE_STREAM_DECODER_OK)
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
		printf("FAILED, returned false, state = %u (%s)\n", (unsigned)((::OggFLAC__SeekableStreamDecoderState)state), state.as_cstring());
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

	printf("\n+++ libOggFLAC++ unit test: OggFLAC::Decoder::SeekableStream\n\n");

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
	if(decoder->init() != ::OggFLAC__SEEKABLE_STREAM_DECODER_OK)
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

	printf("testing set_serial_number()... ");
	if(!decoder->set_serial_number(file_utils__serial_number))
		return decoder->die("returned false");
	printf("OK\n");

	printf("testing init()... ");
	if(decoder->init() != ::OggFLAC__SEEKABLE_STREAM_DECODER_OK)
		return decoder->die();
	printf("OK\n");

	printf("testing get_state()... ");
	OggFLAC::Decoder::SeekableStream::State state = decoder->get_state();
	printf("returned state = %u (%s)... OK\n", (unsigned)((::OggFLAC__SeekableStreamDecoderState)state), state.as_cstring());

	printf("testing get_stream_decoder_state()... ");
	OggFLAC::Decoder::Stream::State state_ = decoder->get_stream_decoder_state();
	printf("returned state = %u (%s)... OK\n", (unsigned)((::OggFLAC__StreamDecoderState)state_), state_.as_cstring());

	printf("testing get_FLAC_stream_decoder_state()... ");
	FLAC::Decoder::Stream::State state__ = decoder->get_FLAC_stream_decoder_state();
	printf("returned state = %u (%s)... OK\n", (unsigned)((::FLAC__StreamDecoderState)state__), state__.as_cstring());

	decoder->current_metadata_number_ = 0;
	decoder->ignore_errors_ = false;
	decoder->error_occurred_ = false;

	printf("opening Ogg FLAC file... ");
	decoder->file_ = ::fopen(oggflacfilename_, "rb");
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
	expected_metadata_sequence_[num_expected_++] = &vorbiscomment_;
	expected_metadata_sequence_[num_expected_++] = &padding_;
	expected_metadata_sequence_[num_expected_++] = &seektable_;
	expected_metadata_sequence_[num_expected_++] = &application1_;
	expected_metadata_sequence_[num_expected_++] = &application2_;
	expected_metadata_sequence_[num_expected_++] = &cuesheet_;
	expected_metadata_sequence_[num_expected_++] = &unknown_;

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
	expected_metadata_sequence_[num_expected_++] = &cuesheet_;
	expected_metadata_sequence_[num_expected_++] = &unknown_;

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
	expected_metadata_sequence_[num_expected_++] = &vorbiscomment_;
	expected_metadata_sequence_[num_expected_++] = &padding_;
	expected_metadata_sequence_[num_expected_++] = &seektable_;
	expected_metadata_sequence_[num_expected_++] = &cuesheet_;
	expected_metadata_sequence_[num_expected_++] = &unknown_;

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
	expected_metadata_sequence_[num_expected_++] = &vorbiscomment_;
	expected_metadata_sequence_[num_expected_++] = &padding_;
	expected_metadata_sequence_[num_expected_++] = &seektable_;
	expected_metadata_sequence_[num_expected_++] = &application2_;
	expected_metadata_sequence_[num_expected_++] = &cuesheet_;
	expected_metadata_sequence_[num_expected_++] = &unknown_;

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
	expected_metadata_sequence_[num_expected_++] = &vorbiscomment_;
	expected_metadata_sequence_[num_expected_++] = &padding_;
	expected_metadata_sequence_[num_expected_++] = &seektable_;
	expected_metadata_sequence_[num_expected_++] = &cuesheet_;
	expected_metadata_sequence_[num_expected_++] = &unknown_;

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
	expected_metadata_sequence_[num_expected_++] = &vorbiscomment_;
	expected_metadata_sequence_[num_expected_++] = &padding_;
	expected_metadata_sequence_[num_expected_++] = &seektable_;
	expected_metadata_sequence_[num_expected_++] = &application1_;
	expected_metadata_sequence_[num_expected_++] = &cuesheet_;
	expected_metadata_sequence_[num_expected_++] = &unknown_;

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
	expected_metadata_sequence_[num_expected_++] = &vorbiscomment_;
	expected_metadata_sequence_[num_expected_++] = &padding_;
	expected_metadata_sequence_[num_expected_++] = &seektable_;
	expected_metadata_sequence_[num_expected_++] = &application1_;
	expected_metadata_sequence_[num_expected_++] = &application2_;
	expected_metadata_sequence_[num_expected_++] = &cuesheet_;
	expected_metadata_sequence_[num_expected_++] = &unknown_;

	::fclose(decoder->file_);

	printf("freeing decoder instance... ");
	delete decoder;
	printf("OK\n");

	printf("\nPASSED!\n");

	return true;
}

class FileDecoder : public OggFLAC::Decoder::File, public DecoderCommon {
public:
	FileDecoder(): OggFLAC::Decoder::File(), DecoderCommon() { }
	~FileDecoder() { }

	// from OggFLAC::Decoder::File
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
		printf("FAILED, %s", msg);
	else
		printf("FAILED");

	printf(", state = %u (%s)\n", (unsigned)((::OggFLAC__FileDecoderState)state), state.as_cstring());
	if(state == ::OggFLAC__FILE_DECODER_SEEKABLE_STREAM_DECODER_ERROR) {
		OggFLAC::Decoder::SeekableStream::State state_ = get_seekable_stream_decoder_state();
		printf("      seekable stream decoder state = %u (%s)\n", (unsigned)((::OggFLAC__SeekableStreamDecoderState)state_), state_.as_cstring());
		if(state_ == ::OggFLAC__SEEKABLE_STREAM_DECODER_STREAM_DECODER_ERROR) {
			OggFLAC::Decoder::Stream::State state__ = get_stream_decoder_state();
			printf("      stream decoder state = %u (%s)\n", (unsigned)((::OggFLAC__StreamDecoderState)state__), state__.as_cstring());
			if(state__ == ::OggFLAC__STREAM_DECODER_FLAC_STREAM_DECODER_ERROR) {
				FLAC::Decoder::Stream::State state___ = get_FLAC_stream_decoder_state();
				printf("      FLAC stream decoder state = %u (%s)\n", (unsigned)((::FLAC__StreamDecoderState)state___), state___.as_cstring());
			}
		}
	}

	return false;
}

bool FileDecoder::test_respond()
{
	if(!set_filename(oggflacfilename_)) {
		printf("FAILED at set_filename(), returned false\n");
		return false;
	}

	if(!set_md5_checking(true)) {
		printf("FAILED at set_md5_checking(), returned false\n");
		return false;
	}

	printf("testing init()... ");
	if(init() != ::OggFLAC__FILE_DECODER_OK)
		return die();
	printf("OK\n");

	current_metadata_number_ = 0;

	printf("testing process_until_end_of_file()... ");
	if(!process_until_end_of_file()) {
		State state = get_state();
		printf("FAILED, returned false, state = %u (%s)\n", (unsigned)((::OggFLAC__FileDecoderState)state), state.as_cstring());
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

	printf("\n+++ libOggFLAC++ unit test: OggFLAC::Decoder::File\n\n");

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
	if(decoder->init() != ::OggFLAC__FILE_DECODER_OK)
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
	if(!decoder->set_filename(oggflacfilename_)) {
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

	printf("testing set_serial_number()... ");
	if(!decoder->set_serial_number(file_utils__serial_number))
		return decoder->die("returned false");
	printf("OK\n");

	printf("testing init()... ");
	if(decoder->init() != ::OggFLAC__FILE_DECODER_OK)
		return decoder->die();
	printf("OK\n");

	printf("testing get_state()... ");
	OggFLAC::Decoder::File::State state = decoder->get_state();
	printf("returned state = %u (%s)... OK\n", (unsigned)((::OggFLAC__FileDecoderState)state), state.as_cstring());

	printf("testing get_seekable_stream_decoder_state()... ");
	OggFLAC::Decoder::SeekableStream::State state_ = decoder->get_seekable_stream_decoder_state();
	printf("returned state = %u (%s)... OK\n", (unsigned)((::OggFLAC__SeekableStreamDecoderState)state_), state_.as_cstring());

	printf("testing get_stream_decoder_state()... ");
	OggFLAC::Decoder::Stream::State state__ = decoder->get_stream_decoder_state();
	printf("returned state = %u (%s)... OK\n", (unsigned)((::OggFLAC__StreamDecoderState)state__), state__.as_cstring());

	printf("testing get_FLAC_stream_decoder_state()... ");
	FLAC::Decoder::Stream::State state___ = decoder->get_FLAC_stream_decoder_state();
	printf("returned state = %u (%s)... OK\n", (unsigned)((::FLAC__StreamDecoderState)state___), state___.as_cstring());

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
	expected_metadata_sequence_[num_expected_++] = &vorbiscomment_;
	expected_metadata_sequence_[num_expected_++] = &padding_;
	expected_metadata_sequence_[num_expected_++] = &seektable_;
	expected_metadata_sequence_[num_expected_++] = &application1_;
	expected_metadata_sequence_[num_expected_++] = &application2_;
	expected_metadata_sequence_[num_expected_++] = &cuesheet_;
	expected_metadata_sequence_[num_expected_++] = &unknown_;

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
	expected_metadata_sequence_[num_expected_++] = &cuesheet_;
	expected_metadata_sequence_[num_expected_++] = &unknown_;

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
	expected_metadata_sequence_[num_expected_++] = &vorbiscomment_;
	expected_metadata_sequence_[num_expected_++] = &padding_;
	expected_metadata_sequence_[num_expected_++] = &seektable_;
	expected_metadata_sequence_[num_expected_++] = &cuesheet_;
	expected_metadata_sequence_[num_expected_++] = &unknown_;

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
	expected_metadata_sequence_[num_expected_++] = &vorbiscomment_;
	expected_metadata_sequence_[num_expected_++] = &padding_;
	expected_metadata_sequence_[num_expected_++] = &seektable_;
	expected_metadata_sequence_[num_expected_++] = &application2_;
	expected_metadata_sequence_[num_expected_++] = &cuesheet_;
	expected_metadata_sequence_[num_expected_++] = &unknown_;

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
	expected_metadata_sequence_[num_expected_++] = &vorbiscomment_;
	expected_metadata_sequence_[num_expected_++] = &padding_;
	expected_metadata_sequence_[num_expected_++] = &seektable_;
	expected_metadata_sequence_[num_expected_++] = &cuesheet_;
	expected_metadata_sequence_[num_expected_++] = &unknown_;

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
	expected_metadata_sequence_[num_expected_++] = &vorbiscomment_;
	expected_metadata_sequence_[num_expected_++] = &padding_;
	expected_metadata_sequence_[num_expected_++] = &seektable_;
	expected_metadata_sequence_[num_expected_++] = &application1_;
	expected_metadata_sequence_[num_expected_++] = &cuesheet_;
	expected_metadata_sequence_[num_expected_++] = &unknown_;

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
	expected_metadata_sequence_[num_expected_++] = &vorbiscomment_;
	expected_metadata_sequence_[num_expected_++] = &padding_;
	expected_metadata_sequence_[num_expected_++] = &seektable_;
	expected_metadata_sequence_[num_expected_++] = &application1_;
	expected_metadata_sequence_[num_expected_++] = &application2_;
	expected_metadata_sequence_[num_expected_++] = &cuesheet_;
	expected_metadata_sequence_[num_expected_++] = &unknown_;

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

	(void) grabbag__file_remove_file(oggflacfilename_);

	free_metadata_blocks_();

	return true;
}
