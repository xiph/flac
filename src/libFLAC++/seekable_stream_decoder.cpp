/* libFLAC++ - Free Lossless Audio Codec library
 * Copyright (C) 2002,2003  Josh Coalson
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

#include "FLAC++/decoder.h"
#include "FLAC/assert.h"

namespace FLAC {
	namespace Decoder {

		const char *SeekableStream::State::resolved_as_cstring(const SeekableStream &decoder) const
		{
			if(state_ == ::FLAC__SEEKABLE_STREAM_DECODER_STREAM_DECODER_ERROR)
				return decoder.get_stream_decoder_state().as_cstring();
			else
				return as_cstring();
		}

		SeekableStream::SeekableStream():
		decoder_(::FLAC__seekable_stream_decoder_new())
		{ }

		SeekableStream::~SeekableStream()
		{
			if(0 != decoder_) {
				(void) ::FLAC__seekable_stream_decoder_finish(decoder_);
				::FLAC__seekable_stream_decoder_delete(decoder_);
			}
		}

		bool SeekableStream::is_valid() const
		{
			return 0 != decoder_;
		}

		bool SeekableStream::set_md5_checking(bool value)
		{
			FLAC__ASSERT(is_valid());
			return (bool)::FLAC__seekable_stream_decoder_set_md5_checking(decoder_, value);
		}

		bool SeekableStream::set_metadata_respond(::FLAC__MetadataType type)
		{
			FLAC__ASSERT(is_valid());
			return (bool)::FLAC__seekable_stream_decoder_set_metadata_respond(decoder_, type);
		}

		bool SeekableStream::set_metadata_respond_application(const FLAC__byte id[4])
		{
			FLAC__ASSERT(is_valid());
			return (bool)::FLAC__seekable_stream_decoder_set_metadata_respond_application(decoder_, id);
		}

		bool SeekableStream::set_metadata_respond_all()
		{
			FLAC__ASSERT(is_valid());
			return (bool)::FLAC__seekable_stream_decoder_set_metadata_respond_all(decoder_);
		}

		bool SeekableStream::set_metadata_ignore(::FLAC__MetadataType type)
		{
			FLAC__ASSERT(is_valid());
			return (bool)::FLAC__seekable_stream_decoder_set_metadata_ignore(decoder_, type);
		}

		bool SeekableStream::set_metadata_ignore_application(const FLAC__byte id[4])
		{
			FLAC__ASSERT(is_valid());
			return (bool)::FLAC__seekable_stream_decoder_set_metadata_ignore_application(decoder_, id);
		}

		bool SeekableStream::set_metadata_ignore_all()
		{
			FLAC__ASSERT(is_valid());
			return (bool)::FLAC__seekable_stream_decoder_set_metadata_ignore_all(decoder_);
		}

		SeekableStream::State SeekableStream::get_state() const
		{
			FLAC__ASSERT(is_valid());
			return State(::FLAC__seekable_stream_decoder_get_state(decoder_));
		}

		Stream::State SeekableStream::get_stream_decoder_state() const
		{
			FLAC__ASSERT(is_valid());
			return Stream::State(::FLAC__seekable_stream_decoder_get_stream_decoder_state(decoder_));
		}

		bool SeekableStream::get_md5_checking() const
		{
			FLAC__ASSERT(is_valid());
			return (bool)::FLAC__seekable_stream_decoder_get_md5_checking(decoder_);
		}

		unsigned SeekableStream::get_channels() const
		{
			FLAC__ASSERT(is_valid());
			return ::FLAC__seekable_stream_decoder_get_channels(decoder_);
		}

		::FLAC__ChannelAssignment SeekableStream::get_channel_assignment() const
		{
			FLAC__ASSERT(is_valid());
			return ::FLAC__seekable_stream_decoder_get_channel_assignment(decoder_);
		}

		unsigned SeekableStream::get_bits_per_sample() const
		{
			FLAC__ASSERT(is_valid());
			return ::FLAC__seekable_stream_decoder_get_bits_per_sample(decoder_);
		}

		unsigned SeekableStream::get_sample_rate() const
		{
			FLAC__ASSERT(is_valid());
			return ::FLAC__seekable_stream_decoder_get_sample_rate(decoder_);
		}

		unsigned SeekableStream::get_blocksize() const
		{
			FLAC__ASSERT(is_valid());
			return ::FLAC__seekable_stream_decoder_get_blocksize(decoder_);
		}

		SeekableStream::State SeekableStream::init()
		{
			FLAC__ASSERT(is_valid());
			::FLAC__seekable_stream_decoder_set_read_callback(decoder_, read_callback_);
			::FLAC__seekable_stream_decoder_set_seek_callback(decoder_, seek_callback_);
			::FLAC__seekable_stream_decoder_set_tell_callback(decoder_, tell_callback_);
			::FLAC__seekable_stream_decoder_set_length_callback(decoder_, length_callback_);
			::FLAC__seekable_stream_decoder_set_eof_callback(decoder_, eof_callback_);
			::FLAC__seekable_stream_decoder_set_write_callback(decoder_, write_callback_);
			::FLAC__seekable_stream_decoder_set_metadata_callback(decoder_, metadata_callback_);
			::FLAC__seekable_stream_decoder_set_error_callback(decoder_, error_callback_);
			::FLAC__seekable_stream_decoder_set_client_data(decoder_, (void*)this);
			return State(::FLAC__seekable_stream_decoder_init(decoder_));
		}

		bool SeekableStream::finish()
		{
			FLAC__ASSERT(is_valid());
			return (bool)::FLAC__seekable_stream_decoder_finish(decoder_);
		}

		bool SeekableStream::flush()
		{
			FLAC__ASSERT(is_valid());
			return (bool)::FLAC__seekable_stream_decoder_flush(decoder_);
		}

		bool SeekableStream::reset()
		{
			FLAC__ASSERT(is_valid());
			return (bool)::FLAC__seekable_stream_decoder_reset(decoder_);
		}

		bool SeekableStream::process_single()
		{
			FLAC__ASSERT(is_valid());
			return (bool)::FLAC__seekable_stream_decoder_process_single(decoder_);
		}

		bool SeekableStream::process_until_end_of_metadata()
		{
			FLAC__ASSERT(is_valid());
			return (bool)::FLAC__seekable_stream_decoder_process_until_end_of_metadata(decoder_);
		}

		bool SeekableStream::process_until_end_of_stream()
		{
			FLAC__ASSERT(is_valid());
			return (bool)::FLAC__seekable_stream_decoder_process_until_end_of_stream(decoder_);
		}

		bool SeekableStream::seek_absolute(FLAC__uint64 sample)
		{
			FLAC__ASSERT(is_valid());
			return (bool)::FLAC__seekable_stream_decoder_seek_absolute(decoder_, sample);
		}

		::FLAC__SeekableStreamDecoderReadStatus SeekableStream::read_callback_(const ::FLAC__SeekableStreamDecoder *decoder, FLAC__byte buffer[], unsigned *bytes, void *client_data)
		{
			(void) decoder;
			FLAC__ASSERT(0 != client_data);
			SeekableStream *instance = reinterpret_cast<SeekableStream *>(client_data);
			FLAC__ASSERT(0 != instance);
			return instance->read_callback(buffer, bytes);
		}

		::FLAC__SeekableStreamDecoderSeekStatus SeekableStream::seek_callback_(const ::FLAC__SeekableStreamDecoder *decoder, FLAC__uint64 absolute_byte_offset, void *client_data)
		{
			(void) decoder;
			FLAC__ASSERT(0 != client_data);
			SeekableStream *instance = reinterpret_cast<SeekableStream *>(client_data);
			FLAC__ASSERT(0 != instance);
			return instance->seek_callback(absolute_byte_offset);
		}

		::FLAC__SeekableStreamDecoderTellStatus SeekableStream::tell_callback_(const ::FLAC__SeekableStreamDecoder *decoder, FLAC__uint64 *absolute_byte_offset, void *client_data)
		{
			(void) decoder;
			FLAC__ASSERT(0 != client_data);
			SeekableStream *instance = reinterpret_cast<SeekableStream *>(client_data);
			FLAC__ASSERT(0 != instance);
			return instance->tell_callback(absolute_byte_offset);
		}

		::FLAC__SeekableStreamDecoderLengthStatus SeekableStream::length_callback_(const ::FLAC__SeekableStreamDecoder *decoder, FLAC__uint64 *stream_length, void *client_data)
		{
			(void) decoder;
			FLAC__ASSERT(0 != client_data);
			SeekableStream *instance = reinterpret_cast<SeekableStream *>(client_data);
			FLAC__ASSERT(0 != instance);
			return instance->length_callback(stream_length);
		}

		FLAC__bool SeekableStream::eof_callback_(const ::FLAC__SeekableStreamDecoder *decoder, void *client_data)
		{
			(void) decoder;
			FLAC__ASSERT(0 != client_data);
			SeekableStream *instance = reinterpret_cast<SeekableStream *>(client_data);
			FLAC__ASSERT(0 != instance);
			return instance->eof_callback();
		}

		::FLAC__StreamDecoderWriteStatus SeekableStream::write_callback_(const ::FLAC__SeekableStreamDecoder *decoder, const ::FLAC__Frame *frame, const FLAC__int32 * const buffer[], void *client_data)
		{
			(void) decoder;
			FLAC__ASSERT(0 != client_data);
			SeekableStream *instance = reinterpret_cast<SeekableStream *>(client_data);
			FLAC__ASSERT(0 != instance);
			return instance->write_callback(frame, buffer);
		}

		void SeekableStream::metadata_callback_(const ::FLAC__SeekableStreamDecoder *decoder, const ::FLAC__StreamMetadata *metadata, void *client_data)
		{
			(void) decoder;
			FLAC__ASSERT(0 != client_data);
			SeekableStream *instance = reinterpret_cast<SeekableStream *>(client_data);
			FLAC__ASSERT(0 != instance);
			instance->metadata_callback(metadata);
		}

		void SeekableStream::error_callback_(const ::FLAC__SeekableStreamDecoder *decoder, ::FLAC__StreamDecoderErrorStatus status, void *client_data)
		{
			(void) decoder;
			FLAC__ASSERT(0 != client_data);
			SeekableStream *instance = reinterpret_cast<SeekableStream *>(client_data);
			FLAC__ASSERT(0 != instance);
			instance->error_callback(status);
		}

	};
};
