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

#include "FLAC++/metadata.h"
#include "FLAC/assert.h"
#include <string.h> // for memcpy()

namespace FLAC {
	namespace Metadata {

		//
		// Prototype
		//

		Prototype::Prototype(::FLAC__StreamMetaData *object, bool copy)
		{
			FLAC__ASSERT(0 != object);
			object_ = copy? ::FLAC__metadata_object_copy(object) : object;
		}

		Prototype::~Prototype()
		{
			if(0 != object_)
				FLAC__metadata_object_delete(object_);
		}

		bool Prototype::get_is_last() const
		{
			FLAC__ASSERT(is_valid());
			return object_->is_last;
		}

		FLAC__MetaDataType Prototype::get_type() const
		{
			FLAC__ASSERT(is_valid());
			return object_->type;
		}

		unsigned Prototype::get_length() const
		{
			FLAC__ASSERT(is_valid());
			return object_->length;
		}

		//
		// StreamInfo
		//

		StreamInfo::StreamInfo():
		Prototype(FLAC__metadata_object_new(FLAC__METADATA_TYPE_STREAMINFO), /*copy=*/false)
		{ }

		StreamInfo::StreamInfo(::FLAC__StreamMetaData *object, bool copy):
		Prototype(object, copy)
		{ }

		StreamInfo::~StreamInfo()
		{ }

		unsigned StreamInfo::get_min_blocksize() const
		{
			FLAC__ASSERT(is_valid());
			return object_->data.stream_info.min_blocksize;
		}

		unsigned StreamInfo::get_max_blocksize() const
		{
			FLAC__ASSERT(is_valid());
			return object_->data.stream_info.max_blocksize;
		}

		unsigned StreamInfo::get_min_framesize() const
		{
			FLAC__ASSERT(is_valid());
			return object_->data.stream_info.min_framesize;
		}

		unsigned StreamInfo::get_max_framesize() const
		{
			FLAC__ASSERT(is_valid());
			return object_->data.stream_info.max_framesize;
		}

		unsigned StreamInfo::get_sample_rate() const
		{
			FLAC__ASSERT(is_valid());
			return object_->data.stream_info.sample_rate;
		}

		unsigned StreamInfo::get_channels() const
		{
			FLAC__ASSERT(is_valid());
			return object_->data.stream_info.channels;
		}

		unsigned StreamInfo::get_bits_per_sample() const
		{
			FLAC__ASSERT(is_valid());
			return object_->data.stream_info.bits_per_sample;
		}

		FLAC__uint64 StreamInfo::get_total_samples() const
		{
			FLAC__ASSERT(is_valid());
			return object_->data.stream_info.total_samples;
		}

		const FLAC__byte *StreamInfo::get_md5sum() const
		{
			FLAC__ASSERT(is_valid());
			return object_->data.stream_info.md5sum;
		}

		void StreamInfo::set_min_blocksize(unsigned value)
		{
			FLAC__ASSERT(is_valid());
			FLAC__ASSERT(value >= FLAC__MIN_BLOCK_SIZE);
			FLAC__ASSERT(value <= FLAC__MAX_BLOCK_SIZE);
			object_->data.stream_info.min_blocksize = value;
		}

		void StreamInfo::set_max_blocksize(unsigned value)
		{
			FLAC__ASSERT(is_valid());
			FLAC__ASSERT(value >= FLAC__MIN_BLOCK_SIZE);
			FLAC__ASSERT(value <= FLAC__MAX_BLOCK_SIZE);
			object_->data.stream_info.max_blocksize = value;
		}

		void StreamInfo::set_min_framesize(unsigned value)
		{
			FLAC__ASSERT(is_valid());
			FLAC__ASSERT(value < (1u < FLAC__STREAM_METADATA_STREAMINFO_MIN_FRAME_SIZE_LEN));
			object_->data.stream_info.min_framesize = value;
		}

		void StreamInfo::set_max_framesize(unsigned value)
		{
			FLAC__ASSERT(is_valid());
			FLAC__ASSERT(value < (1u < FLAC__STREAM_METADATA_STREAMINFO_MAX_FRAME_SIZE_LEN));
			object_->data.stream_info.max_framesize = value;
		}

		void StreamInfo::set_sample_rate(unsigned value)
		{
			FLAC__ASSERT(is_valid());
			FLAC__ASSERT(FLAC__format_is_valid_sample_rate(value));
			object_->data.stream_info.sample_rate = value;
		}

		void StreamInfo::set_channels(unsigned value)
		{
			FLAC__ASSERT(is_valid());
			FLAC__ASSERT(value > 0);
			FLAC__ASSERT(value <= FLAC__MAX_CHANNELS);
			object_->data.stream_info.channels = value;
		}

		void StreamInfo::set_bits_per_sample(unsigned value)
		{
			FLAC__ASSERT(is_valid());
			FLAC__ASSERT(value >= FLAC__MIN_BITS_PER_SAMPLE);
			FLAC__ASSERT(value <= FLAC__MAX_BITS_PER_SAMPLE);
			object_->data.stream_info.bits_per_sample = value;
		}

		void StreamInfo::set_total_samples(FLAC__uint64 value)
		{
			FLAC__ASSERT(is_valid());
			FLAC__ASSERT(value < (1u << FLAC__STREAM_METADATA_STREAMINFO_TOTAL_SAMPLES_LEN));
			object_->data.stream_info.total_samples = value;
		}

		void StreamInfo::set_md5sum(const FLAC__byte value[16])
		{
			FLAC__ASSERT(is_valid());
			FLAC__ASSERT(0 != value);
			memcpy(object_->data.stream_info.md5sum, value, 16);
		}

		//
		// Padding
		//

		Padding::Padding():
		Prototype(FLAC__metadata_object_new(FLAC__METADATA_TYPE_PADDING), /*copy=*/false)
		{ }

		Padding::Padding(::FLAC__StreamMetaData *object, bool copy):
		Prototype(object, copy)
		{ }

		Padding::~Padding()
		{ }

		//
		// Application
		//

		Application::Application():
		Prototype(FLAC__metadata_object_new(FLAC__METADATA_TYPE_APPLICATION), /*copy=*/false)
		{ }

		Application::Application(::FLAC__StreamMetaData *object, bool copy):
		Prototype(object, copy)
		{ }

		Application::~Application()
		{ }

		const FLAC__byte *Application::get_id() const
		{
			FLAC__ASSERT(is_valid());
			return object_->data.application.id;
		}

		const FLAC__byte *Application::get_data() const
		{
			FLAC__ASSERT(is_valid());
			return object_->data.application.data;
		}

		void Application::set_id(FLAC__byte value[4])
		{
			FLAC__ASSERT(is_valid());
			FLAC__ASSERT(0 != value);
			memcpy(object_->data.application.id, value, 4);
		}

		bool Application::set_data(FLAC__byte *data, unsigned length, bool copy)
		{
			FLAC__ASSERT(is_valid());
			return FLAC__metadata_object_application_set_data(object_, data, length, copy);
		}

		//
		// SeekTable
		//

		SeekTable::SeekTable():
		Prototype(FLAC__metadata_object_new(FLAC__METADATA_TYPE_SEEKTABLE), /*copy=*/false)
		{ }

		SeekTable::SeekTable(::FLAC__StreamMetaData *object, bool copy):
		Prototype(object, copy)
		{ }

		SeekTable::~SeekTable()
		{ }

		//
		// VorbisComment
		//

		VorbisComment::VorbisComment():
		Prototype(FLAC__metadata_object_new(FLAC__METADATA_TYPE_VORBIS_COMMENT), /*copy=*/false)
		{ }

		VorbisComment::VorbisComment(::FLAC__StreamMetaData *object, bool copy):
		Prototype(object, copy)
		{ }

		VorbisComment::~VorbisComment()
		{ }

	};
};
