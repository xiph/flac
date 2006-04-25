/* libFLAC++ - Free Lossless Audio Codec library
 * Copyright (C) 2002,2003,2004,2005,2006  Josh Coalson
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * - Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *
 * - Redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in the
 * documentation and/or other materials provided with the distribution.
 *
 * - Neither the name of the Xiph.org Foundation nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "FLAC++/decoder.h"
#include "FLAC/assert.h"

#ifdef _MSC_VER
// warning C4800: 'int' : forcing to bool 'true' or 'false' (performance warning)
#pragma warning ( disable : 4800 )
#endif

namespace FLAC {
	namespace Decoder {

		File::File():
		decoder_(::FLAC__file_decoder_new())
		{ }

		File::~File()
		{
			if(0 != decoder_) {
				(void) ::FLAC__file_decoder_finish(decoder_);
				::FLAC__file_decoder_delete(decoder_);
			}
		}

		bool File::is_valid() const
		{
			return 0 != decoder_;
		}

		bool File::set_md5_checking(bool value)
		{
			FLAC__ASSERT(0 != decoder_);
			return (bool)::FLAC__file_decoder_set_md5_checking(decoder_, value);
		}

		bool File::set_filename(const char *value)
		{
			FLAC__ASSERT(0 != decoder_);
			return (bool)::FLAC__file_decoder_set_filename(decoder_, value);
		}

		bool File::set_metadata_respond(::FLAC__MetadataType type)
		{
			FLAC__ASSERT(0 != decoder_);
			return (bool)::FLAC__file_decoder_set_metadata_respond(decoder_, type);
		}

		bool File::set_metadata_respond_application(const FLAC__byte id[4])
		{
			FLAC__ASSERT(0 != decoder_);
			return (bool)::FLAC__file_decoder_set_metadata_respond_application(decoder_, id);
		}

		bool File::set_metadata_respond_all()
		{
			FLAC__ASSERT(0 != decoder_);
			return (bool)::FLAC__file_decoder_set_metadata_respond_all(decoder_);
		}

		bool File::set_metadata_ignore(::FLAC__MetadataType type)
		{
			FLAC__ASSERT(0 != decoder_);
			return (bool)::FLAC__file_decoder_set_metadata_ignore(decoder_, type);
		}

		bool File::set_metadata_ignore_application(const FLAC__byte id[4])
		{
			FLAC__ASSERT(0 != decoder_);
			return (bool)::FLAC__file_decoder_set_metadata_ignore_application(decoder_, id);
		}

		bool File::set_metadata_ignore_all()
		{
			FLAC__ASSERT(0 != decoder_);
			return (bool)::FLAC__file_decoder_set_metadata_ignore_all(decoder_);
		}

		File::State File::get_state() const
		{
			FLAC__ASSERT(0 != decoder_);
			return State(::FLAC__file_decoder_get_state(decoder_));
		}

		SeekableStream::State File::get_seekable_stream_decoder_state() const
		{
			FLAC__ASSERT(0 != decoder_);
			return SeekableStream::State(::FLAC__file_decoder_get_seekable_stream_decoder_state(decoder_));
		}

		Stream::State File::get_stream_decoder_state() const
		{
			FLAC__ASSERT(0 != decoder_);
			return Stream::State(::FLAC__file_decoder_get_stream_decoder_state(decoder_));
		}

		bool File::get_md5_checking() const
		{
			FLAC__ASSERT(0 != decoder_);
			return (bool)::FLAC__file_decoder_get_md5_checking(decoder_);
		}

		unsigned File::get_channels() const
		{
			FLAC__ASSERT(is_valid());
			return ::FLAC__file_decoder_get_channels(decoder_);
		}

		::FLAC__ChannelAssignment File::get_channel_assignment() const
		{
			FLAC__ASSERT(is_valid());
			return ::FLAC__file_decoder_get_channel_assignment(decoder_);
		}

		unsigned File::get_bits_per_sample() const
		{
			FLAC__ASSERT(is_valid());
			return ::FLAC__file_decoder_get_bits_per_sample(decoder_);
		}

		unsigned File::get_sample_rate() const
		{
			FLAC__ASSERT(is_valid());
			return ::FLAC__file_decoder_get_sample_rate(decoder_);
		}

		unsigned File::get_blocksize() const
		{
			FLAC__ASSERT(is_valid());
			return ::FLAC__file_decoder_get_blocksize(decoder_);
		}

		File::State File::init()
		{
			FLAC__ASSERT(0 != decoder_);
			::FLAC__file_decoder_set_write_callback(decoder_, write_callback_);
			::FLAC__file_decoder_set_metadata_callback(decoder_, metadata_callback_);
			::FLAC__file_decoder_set_error_callback(decoder_, error_callback_);
			::FLAC__file_decoder_set_client_data(decoder_, (void*)this);
			return State(::FLAC__file_decoder_init(decoder_));
		}

		bool File::finish()
		{
			FLAC__ASSERT(0 != decoder_);
			return (bool)::FLAC__file_decoder_finish(decoder_);
		}

		bool File::process_single()
		{
			FLAC__ASSERT(0 != decoder_);
			return (bool)::FLAC__file_decoder_process_single(decoder_);
		}

		bool File::process_until_end_of_metadata()
		{
			FLAC__ASSERT(0 != decoder_);
			return (bool)::FLAC__file_decoder_process_until_end_of_metadata(decoder_);
		}

		bool File::process_until_end_of_file()
		{
			FLAC__ASSERT(0 != decoder_);
			return (bool)::FLAC__file_decoder_process_until_end_of_file(decoder_);
		}

		bool File::skip_single_frame()
		{
			FLAC__ASSERT(is_valid());
			return (bool)::FLAC__file_decoder_skip_single_frame(decoder_);
		}

		bool File::seek_absolute(FLAC__uint64 sample)
		{
			FLAC__ASSERT(0 != decoder_);
			return (bool)::FLAC__file_decoder_seek_absolute(decoder_, sample);
		}

		::FLAC__StreamDecoderWriteStatus File::write_callback_(const ::FLAC__FileDecoder *decoder, const ::FLAC__Frame *frame, const FLAC__int32 * const buffer[], void *client_data)
		{
			(void) decoder;
			FLAC__ASSERT(0 != client_data);
			File *instance = reinterpret_cast<File *>(client_data);
			FLAC__ASSERT(0 != instance);
			return instance->write_callback(frame, buffer);
		}

		void File::metadata_callback_(const ::FLAC__FileDecoder *decoder, const ::FLAC__StreamMetadata *metadata, void *client_data)
		{
			(void) decoder;
			FLAC__ASSERT(0 != client_data);
			File *instance = reinterpret_cast<File *>(client_data);
			FLAC__ASSERT(0 != instance);
			instance->metadata_callback(metadata);
		}

		void File::error_callback_(const ::FLAC__FileDecoder *decoder, ::FLAC__StreamDecoderErrorStatus status, void *client_data)
		{
			(void) decoder;
			FLAC__ASSERT(0 != client_data);
			File *instance = reinterpret_cast<File *>(client_data);
			FLAC__ASSERT(0 != instance);
			instance->error_callback(status);
		}

	}
}
