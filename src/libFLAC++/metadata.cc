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

		// local utility routines

		namespace local {

			Prototype *construct_block(::FLAC__StreamMetaData *object)
			{
				Prototype *ret = 0;
				switch(object->type) {
					case FLAC__METADATA_TYPE_STREAMINFO:
						ret = new StreamInfo(object, /*copy=*/false);
						break;
					case FLAC__METADATA_TYPE_PADDING:
						ret = new Padding(object, /*copy=*/false);
						break;
					case FLAC__METADATA_TYPE_APPLICATION:
						ret = new Application(object, /*copy=*/false);
						break;
					case FLAC__METADATA_TYPE_SEEKTABLE:
						ret = new SeekTable(object, /*copy=*/false);
						break;
					case FLAC__METADATA_TYPE_VORBIS_COMMENT:
						ret = new VorbisComment(object, /*copy=*/false);
						break;
					default:
						FLAC__ASSERT(0);
						break;
				}
				return ret;
			}

		};

		//
		// Prototype
		//

		Prototype::Prototype(::FLAC__StreamMetaData *object, bool copy)
		{
			FLAC__ASSERT(0 != object);
			clear();
			object_ = copy? ::FLAC__metadata_object_copy(object) : object;
			is_reference_ = false;
		}

		Prototype::~Prototype()
		{
			clear();
		}

		void Prototype::clear()
		{
			if(0 != object_)
				FLAC__metadata_object_delete(object_);
			object_ = 0;
		}

		void Prototype::operator=(const Prototype &object)
		{
			clear();
			is_reference_ = object.is_reference_;
			if(is_reference_)
				object_ = object.object_;
			else
				object_ = ::FLAC__metadata_object_copy(object.object_);
		}

		void Prototype::operator=(const ::FLAC__StreamMetaData &object)
		{
			clear();
			is_reference_ = false;
			object_ = ::FLAC__metadata_object_copy(&object);
		}

		void Prototype::operator=(const ::FLAC__StreamMetaData *object)
		{
			FLAC__ASSERT(0 != object);
			clear();
			is_reference_ = false;
			object_ = ::FLAC__metadata_object_copy(object);
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


		// ============================================================
		//
		//  Level 0
		//
		// ============================================================

		bool get_streaminfo(const char *filename, StreamInfo &streaminfo)
		{
			FLAC__ASSERT(0 != filename);

			FLAC__StreamMetaData s;

			if(FLAC__metadata_get_streaminfo(filename, &s.data.stream_info)) {
				streaminfo = s;
				return true;
			}
			else
				return false;
		}


		// ============================================================
		//
		//  Level 1
		//
		// ============================================================

		SimpleIterator::SimpleIterator():
		iterator_(::FLAC__metadata_simple_iterator_new())
		{ }

		SimpleIterator::~SimpleIterator()
		{
			clear();
		}

		void SimpleIterator::clear()
		{
			if(0 != iterator_)
				FLAC__metadata_simple_iterator_delete(iterator_);
			iterator_ = 0;
		}

		bool SimpleIterator::init(const char *filename, bool preserve_file_stats)
		{
			FLAC__ASSERT(0 != filename);
			FLAC__ASSERT(is_valid());
			return ::FLAC__metadata_simple_iterator_init(iterator_, filename, preserve_file_stats);
		}

		bool SimpleIterator::is_valid() const
		{
			return 0 != iterator_;
		}

		SimpleIterator::Status SimpleIterator::status()
		{
			FLAC__ASSERT(is_valid());
			return Status(::FLAC__metadata_simple_iterator_status(iterator_));
		}

		bool SimpleIterator::is_writable() const
		{
			FLAC__ASSERT(is_valid());
			return ::FLAC__metadata_simple_iterator_is_writable(iterator_);
		}

		bool SimpleIterator::next()
		{
			FLAC__ASSERT(is_valid());
			return ::FLAC__metadata_simple_iterator_next(iterator_);
		}

		bool SimpleIterator::prev()
		{
			FLAC__ASSERT(is_valid());
			return ::FLAC__metadata_simple_iterator_prev(iterator_);
		}

		::FLAC__MetaDataType SimpleIterator::get_block_type() const
		{
			FLAC__ASSERT(is_valid());
			return ::FLAC__metadata_simple_iterator_get_block_type(iterator_);
		}

		Prototype *SimpleIterator::get_block()
		{
			FLAC__ASSERT(is_valid());
			return local::construct_block(::FLAC__metadata_simple_iterator_get_block(iterator_));
		}

		bool SimpleIterator::set_block(Prototype *block, bool use_padding)
		{
			FLAC__ASSERT(0 != block);
			FLAC__ASSERT(is_valid());
			return ::FLAC__metadata_simple_iterator_set_block(iterator_, block->object_, use_padding);
		}

		bool SimpleIterator::insert_block_after(Prototype *block, bool use_padding)
		{
			FLAC__ASSERT(0 != block);
			FLAC__ASSERT(is_valid());
			return ::FLAC__metadata_simple_iterator_insert_block_after(iterator_, block->object_, use_padding);
		}

		bool SimpleIterator::delete_block(bool use_padding)
		{
			FLAC__ASSERT(is_valid());
			return ::FLAC__metadata_simple_iterator_delete_block(iterator_, use_padding);
		}


		// ============================================================
		//
		//  Level 2
		//
		// ============================================================

		Chain::Chain():
		chain_(::FLAC__metadata_chain_new())
		{ }

		Chain::~Chain()
		{
			clear();
		}

		void Chain::clear()
		{
			if(0 != chain_)
				FLAC__metadata_chain_delete(chain_);
			chain_ = 0;
		}

		bool Chain::is_valid() const
		{
			return 0 != chain_;
		}

		Chain::Status Chain::status()
		{
			FLAC__ASSERT(is_valid());
			return Status(::FLAC__metadata_chain_status(chain_));
		}

		bool Chain::read(const char *filename)
		{
			FLAC__ASSERT(0 != filename);
			FLAC__ASSERT(is_valid());
			return ::FLAC__metadata_chain_read(chain_, filename);
		}

		bool Chain::write(bool use_padding, bool preserve_file_stats)
		{
			FLAC__ASSERT(is_valid());
			return ::FLAC__metadata_chain_write(chain_, use_padding, preserve_file_stats);
		}

		void Chain::merge_padding()
		{
			FLAC__ASSERT(is_valid());
			::FLAC__metadata_chain_merge_padding(chain_);
		}

		void Chain::sort_padding()
		{
			FLAC__ASSERT(is_valid());
			::FLAC__metadata_chain_sort_padding(chain_);
		}


		Iterator::Iterator():
		iterator_(::FLAC__metadata_iterator_new())
		{ }

		Iterator::~Iterator()
		{
			clear();
		}

		void Iterator::clear()
		{
			if(0 != iterator_)
				FLAC__metadata_iterator_delete(iterator_);
			iterator_ = 0;
		}

		bool Iterator::is_valid() const
		{
			return 0 != iterator_;
		}

		void Iterator::init(Chain *chain)
		{
			FLAC__ASSERT(0 != chain);
			FLAC__ASSERT(is_valid());
			FLAC__ASSERT(chain->is_valid());
			::FLAC__metadata_iterator_init(iterator_, chain->chain_);
		}

		bool Iterator::next()
		{
			FLAC__ASSERT(is_valid());
			return ::FLAC__metadata_iterator_next(iterator_);
		}

		bool Iterator::prev()
		{
			FLAC__ASSERT(is_valid());
			return ::FLAC__metadata_iterator_prev(iterator_);
		}

		::FLAC__MetaDataType Iterator::get_block_type() const 
		{
			FLAC__ASSERT(is_valid());
			return ::FLAC__metadata_iterator_get_block_type(iterator_);
		}

		Prototype *Iterator::get_block()
		{
			FLAC__ASSERT(is_valid());
			Prototype *block = local::construct_block(::FLAC__metadata_iterator_get_block(iterator_));
			if(0 != block)
				block->set_reference(true);
			return block;
		}

		bool Iterator::set_block(Prototype *block)
		{
			FLAC__ASSERT(0 != block);
			FLAC__ASSERT(is_valid());
			bool ret = ::FLAC__metadata_iterator_set_block(iterator_, block->object_);
			if(ret) {
				block->set_reference(true);
				delete block;
			}
			return ret;
		}

		bool Iterator::delete_block(bool replace_with_padding)
		{
			FLAC__ASSERT(is_valid());
			return ::FLAC__metadata_iterator_delete_block(iterator_, replace_with_padding);
		}

		bool Iterator::insert_block_before(Prototype *block)
		{
			FLAC__ASSERT(0 != block);
			FLAC__ASSERT(is_valid());
			bool ret = ::FLAC__metadata_iterator_insert_block_before(iterator_, block->object_);
			if(ret) {
				block->set_reference(true);
				delete block;
			}
			return ret;
		}

		bool Iterator::insert_block_after(Prototype *block)
		{
			FLAC__ASSERT(0 != block);
			FLAC__ASSERT(is_valid());
			bool ret = ::FLAC__metadata_iterator_insert_block_after(iterator_, block->object_);
			if(ret) {
				block->set_reference(true);
				delete block;
			}
			return ret;
		}

	};
};
