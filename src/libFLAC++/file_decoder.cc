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
			return ::FLAC__file_decoder_set_md5_checking(decoder_, value);
		}

		bool File::set_filename(const char *value)
		{
			FLAC__ASSERT(0 != decoder_);
			return ::FLAC__file_decoder_set_filename(decoder_, value);
		}

		bool File::set_metadata_respond(::FLAC__MetaDataType type)
		{
			FLAC__ASSERT(0 != decoder_);
			return ::FLAC__file_decoder_set_metadata_respond(decoder_, type);
		}

		bool File::set_metadata_respond_application(const FLAC__byte id[4])
		{
			FLAC__ASSERT(0 != decoder_);
			return ::FLAC__file_decoder_set_metadata_respond_application(decoder_, id);
		}

		bool File::set_metadata_respond_all()
		{
			FLAC__ASSERT(0 != decoder_);
			return ::FLAC__file_decoder_set_metadata_respond_all(decoder_);
		}

		bool File::set_metadata_ignore(::FLAC__MetaDataType type)
		{
			FLAC__ASSERT(0 != decoder_);
			return ::FLAC__file_decoder_set_metadata_ignore(decoder_, type);
		}

		bool File::set_metadata_ignore_application(const FLAC__byte id[4])
		{
			FLAC__ASSERT(0 != decoder_);
			return ::FLAC__file_decoder_set_metadata_ignore_application(decoder_, id);
		}

		bool File::set_metadata_ignore_all()
		{
			FLAC__ASSERT(0 != decoder_);
			return ::FLAC__file_decoder_set_metadata_ignore_all(decoder_);
		}

		File::State File::get_state() const
		{
			FLAC__ASSERT(0 != decoder_);
			return State(::FLAC__file_decoder_get_state(decoder_));
		}

		bool File::get_md5_checking() const
		{
			FLAC__ASSERT(0 != decoder_);
			return ::FLAC__file_decoder_get_md5_checking(decoder_);
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
			return ::FLAC__file_decoder_finish(decoder_);
		}

		bool File::process_whole_file()
		{
			FLAC__ASSERT(0 != decoder_);
			return ::FLAC__file_decoder_process_whole_file(decoder_);
		}

		bool File::process_metadata()
		{
			FLAC__ASSERT(0 != decoder_);
			return ::FLAC__file_decoder_process_metadata(decoder_);
		}

		bool File::process_one_frame()
		{
			FLAC__ASSERT(0 != decoder_);
			return ::FLAC__file_decoder_process_one_frame(decoder_);
		}

		bool File::process_remaining_frames()
		{
			FLAC__ASSERT(0 != decoder_);
			return ::FLAC__file_decoder_process_remaining_frames(decoder_);
		}

		bool File::seek_absolute(FLAC__uint64 sample)
		{
			FLAC__ASSERT(0 != decoder_);
			return ::FLAC__file_decoder_seek_absolute(decoder_, sample);
		}

		::FLAC__StreamDecoderWriteStatus File::write_callback_(const ::FLAC__FileDecoder *decoder, const ::FLAC__Frame *frame, const FLAC__int32 *buffer[], void *client_data)
		{
			(void) decoder;
			FLAC__ASSERT(0 != client_data);
			File *instance = reinterpret_cast<File *>(client_data);
			FLAC__ASSERT(0 != instance);
			return instance->write_callback(frame, buffer);
		}

		void File::metadata_callback_(const ::FLAC__FileDecoder *decoder, const ::FLAC__StreamMetaData *metadata, void *client_data)
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

	};
};
