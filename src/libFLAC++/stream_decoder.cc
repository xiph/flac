/* libFLAC++ - Free Lossless Audio Codec library
 * Copyright (C) 2002  Josh Coalson
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

		Stream::Stream():
		decoder_(::FLAC__stream_decoder_new())
		{ }

		Stream::~Stream()
		{
			if(0 != decoder_) {
				::FLAC__stream_decoder_finish(decoder_);
				::FLAC__stream_decoder_delete(decoder_);
			}
		}

		bool Stream::is_valid() const
		{
			return 0 != decoder_;
		}

		bool Stream::set_metadata_respond(::FLAC__MetaDataType type)
		{
			FLAC__ASSERT(is_valid());
			return (bool)::FLAC__stream_decoder_set_metadata_respond(decoder_, type);
		}

		bool Stream::set_metadata_respond_application(const FLAC__byte id[4])
		{
			FLAC__ASSERT(is_valid());
			return (bool)::FLAC__stream_decoder_set_metadata_respond_application(decoder_, id);
		}

		bool Stream::set_metadata_respond_all()
		{
			FLAC__ASSERT(is_valid());
			return (bool)::FLAC__stream_decoder_set_metadata_respond_all(decoder_);
		}

		bool Stream::set_metadata_ignore(::FLAC__MetaDataType type)
		{
			FLAC__ASSERT(is_valid());
			return (bool)::FLAC__stream_decoder_set_metadata_ignore(decoder_, type);
		}

		bool Stream::set_metadata_ignore_application(const FLAC__byte id[4])
		{
			FLAC__ASSERT(is_valid());
			return (bool)::FLAC__stream_decoder_set_metadata_ignore_application(decoder_, id);
		}

		bool Stream::set_metadata_ignore_all()
		{
			FLAC__ASSERT(is_valid());
			return (bool)::FLAC__stream_decoder_set_metadata_ignore_all(decoder_);
		}

		Stream::State Stream::get_state() const
		{
			FLAC__ASSERT(is_valid());
			return State(::FLAC__stream_decoder_get_state(decoder_));
		}

		unsigned Stream::get_channels() const
		{
			FLAC__ASSERT(is_valid());
			return ::FLAC__stream_decoder_get_channels(decoder_);
		}

		::FLAC__ChannelAssignment Stream::get_channel_assignment() const
		{
			FLAC__ASSERT(is_valid());
			return ::FLAC__stream_decoder_get_channel_assignment(decoder_);
		}

		unsigned Stream::get_bits_per_sample() const
		{
			FLAC__ASSERT(is_valid());
			return ::FLAC__stream_decoder_get_bits_per_sample(decoder_);
		}

		unsigned Stream::get_sample_rate() const
		{
			FLAC__ASSERT(is_valid());
			return ::FLAC__stream_decoder_get_sample_rate(decoder_);
		}

		unsigned Stream::get_blocksize() const
		{
			FLAC__ASSERT(is_valid());
			return ::FLAC__stream_decoder_get_blocksize(decoder_);
		}

		Stream::State Stream::init()
		{
			FLAC__ASSERT(is_valid());
			::FLAC__stream_decoder_set_read_callback(decoder_, read_callback_);
			::FLAC__stream_decoder_set_write_callback(decoder_, write_callback_);
			::FLAC__stream_decoder_set_metadata_callback(decoder_, metadata_callback_);
			::FLAC__stream_decoder_set_error_callback(decoder_, error_callback_);
			::FLAC__stream_decoder_set_client_data(decoder_, (void*)this);
			return State(::FLAC__stream_decoder_init(decoder_));
		}

		void Stream::finish()
		{
			FLAC__ASSERT(is_valid());
			::FLAC__stream_decoder_finish(decoder_);
		}

		bool Stream::flush()
		{
			FLAC__ASSERT(is_valid());
			return (bool)::FLAC__stream_decoder_flush(decoder_);
		}

		bool Stream::reset()
		{
			FLAC__ASSERT(is_valid());
			return (bool)::FLAC__stream_decoder_reset(decoder_);
		}

		bool Stream::process_whole_stream()
		{
			FLAC__ASSERT(is_valid());
			return (bool)::FLAC__stream_decoder_process_whole_stream(decoder_);
		}

		bool Stream::process_metadata()
		{
			FLAC__ASSERT(is_valid());
			return (bool)::FLAC__stream_decoder_process_metadata(decoder_);
		}

		bool Stream::process_one_frame()
		{
			FLAC__ASSERT(is_valid());
			return (bool)::FLAC__stream_decoder_process_one_frame(decoder_);
		}

		bool Stream::process_remaining_frames()
		{
			FLAC__ASSERT(is_valid());
			return (bool)::FLAC__stream_decoder_process_remaining_frames(decoder_);
		}

		::FLAC__StreamDecoderReadStatus Stream::read_callback_(const ::FLAC__StreamDecoder *decoder, FLAC__byte buffer[], unsigned *bytes, void *client_data)
		{
			(void)decoder;
			FLAC__ASSERT(0 != client_data);
			Stream *instance = reinterpret_cast<Stream *>(client_data);
			FLAC__ASSERT(0 != instance);
			return instance->read_callback(buffer, bytes);
		}

		::FLAC__StreamDecoderWriteStatus Stream::write_callback_(const ::FLAC__StreamDecoder *decoder, const ::FLAC__Frame *frame, const FLAC__int32 *buffer[], void *client_data)
		{
			(void)decoder;
			FLAC__ASSERT(0 != client_data);
			Stream *instance = reinterpret_cast<Stream *>(client_data);
			FLAC__ASSERT(0 != instance);
			return instance->write_callback(frame, buffer);
		}

		void Stream::metadata_callback_(const ::FLAC__StreamDecoder *decoder, const ::FLAC__StreamMetaData *metadata, void *client_data)
		{
			(void)decoder;
			FLAC__ASSERT(0 != client_data);
			Stream *instance = reinterpret_cast<Stream *>(client_data);
			FLAC__ASSERT(0 != instance);
			instance->metadata_callback(metadata);
		}

		void Stream::error_callback_(const ::FLAC__StreamDecoder *decoder, ::FLAC__StreamDecoderErrorStatus status, void *client_data)
		{
			(void)decoder;
			FLAC__ASSERT(0 != client_data);
			Stream *instance = reinterpret_cast<Stream *>(client_data);
			FLAC__ASSERT(0 != instance);
			instance->error_callback(status);
		}

	};
};
