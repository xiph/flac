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
#include <stdlib.h> // for malloc(), free()
#include <string.h> // for memcpy() etc.

namespace FLAC {
	namespace Metadata {

		// local utility routines

		namespace local {

			Prototype *construct_block(::FLAC__StreamMetadata *object)
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
					case FLAC__METADATA_TYPE_CUESHEET:
						ret = new CueSheet(object, /*copy=*/false);
						break;
					default:
						FLAC__ASSERT(0);
						break;
				}
				return ret;
			}

		};

		FLACPP_API Prototype *clone(const Prototype *object)
		{
			FLAC__ASSERT(0 != object);

			const StreamInfo *streaminfo = dynamic_cast<const StreamInfo *>(object);
			const Padding *padding = dynamic_cast<const Padding *>(object);
			const Application *application = dynamic_cast<const Application *>(object);
			const SeekTable *seektable = dynamic_cast<const SeekTable *>(object);
			const VorbisComment *vorbiscomment = dynamic_cast<const VorbisComment *>(object);
			const CueSheet *cuesheet = dynamic_cast<const CueSheet *>(object);

			if(0 != streaminfo)
				return new StreamInfo(*streaminfo);
			else if(0 != padding)
				return new Padding(*padding);
			else if(0 != application)
				return new Application(*application);
			else if(0 != seektable)
				return new SeekTable(*seektable);
			else if(0 != vorbiscomment)
				return new VorbisComment(*vorbiscomment);
			else if(0 != cuesheet)
				return new CueSheet(*cuesheet);
			else {
				FLAC__ASSERT(0);
				return 0;
			}
		}

		//
		// Prototype
		//

		Prototype::Prototype(const Prototype &object):
		object_(::FLAC__metadata_object_clone(object.object_)),
		is_reference_(false)
		{
			FLAC__ASSERT(object.is_valid());
		}

		Prototype::Prototype(const ::FLAC__StreamMetadata &object):
		object_(::FLAC__metadata_object_clone(&object)),
		is_reference_(false)
		{
		}

		Prototype::Prototype(const ::FLAC__StreamMetadata *object):
		object_(::FLAC__metadata_object_clone(object)),
		is_reference_(false)
		{
			FLAC__ASSERT(0 != object);
		}

		Prototype::Prototype(::FLAC__StreamMetadata *object, bool copy):
		object_(copy? ::FLAC__metadata_object_clone(object) : object),
		is_reference_(false)
		{
			FLAC__ASSERT(0 != object);
		}

		Prototype::~Prototype()
		{
			clear();
		}

		void Prototype::clear()
		{
			if(0 != object_ && !is_reference_)
				FLAC__metadata_object_delete(object_);
			object_ = 0;
		}

		void Prototype::operator=(const Prototype &object)
		{
			FLAC__ASSERT(object.is_valid());
			clear();
			is_reference_ = false;
			object_ = ::FLAC__metadata_object_clone(object.object_);
		}

		void Prototype::operator=(const ::FLAC__StreamMetadata &object)
		{
			clear();
			is_reference_ = false;
			object_ = ::FLAC__metadata_object_clone(&object);
		}

		void Prototype::operator=(const ::FLAC__StreamMetadata *object)
		{
			FLAC__ASSERT(0 != object);
			clear();
			is_reference_ = false;
			object_ = ::FLAC__metadata_object_clone(object);
		}

		bool Prototype::get_is_last() const
		{
			FLAC__ASSERT(is_valid());
			return (bool)object_->is_last;
		}

		FLAC__MetadataType Prototype::get_type() const
		{
			FLAC__ASSERT(is_valid());
			return object_->type;
		}

		unsigned Prototype::get_length() const
		{
			FLAC__ASSERT(is_valid());
			return object_->length;
		}

		void Prototype::set_is_last(bool value)
		{
			FLAC__ASSERT(is_valid());
			object_->is_last = value;
		}


		//
		// StreamInfo
		//

		StreamInfo::StreamInfo():
		Prototype(FLAC__metadata_object_new(FLAC__METADATA_TYPE_STREAMINFO), /*copy=*/false)
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
			FLAC__ASSERT(FLAC__format_sample_rate_is_valid(value));
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

		Padding::~Padding()
		{ }

		void Padding::set_length(unsigned length)
		{
			FLAC__ASSERT(is_valid());
			object_->length = length;
		}


		//
		// Application
		//

		Application::Application():
		Prototype(FLAC__metadata_object_new(FLAC__METADATA_TYPE_APPLICATION), /*copy=*/false)
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

		void Application::set_id(const FLAC__byte value[4])
		{
			FLAC__ASSERT(is_valid());
			FLAC__ASSERT(0 != value);
			memcpy(object_->data.application.id, value, 4);
		}

		bool Application::set_data(const FLAC__byte *data, unsigned length)
		{
			FLAC__ASSERT(is_valid());
			return (bool)::FLAC__metadata_object_application_set_data(object_, (FLAC__byte*)data, length, true);
		}

		bool Application::set_data(FLAC__byte *data, unsigned length, bool copy)
		{
			FLAC__ASSERT(is_valid());
			return (bool)::FLAC__metadata_object_application_set_data(object_, data, length, copy);
		}


		//
		// SeekTable
		//

		SeekTable::SeekTable():
		Prototype(FLAC__metadata_object_new(FLAC__METADATA_TYPE_SEEKTABLE), /*copy=*/false)
		{ }

		SeekTable::~SeekTable()
		{ }

		unsigned SeekTable::get_num_points() const
		{
			FLAC__ASSERT(is_valid());
			return object_->data.seek_table.num_points;
		}

		::FLAC__StreamMetadata_SeekPoint SeekTable::get_point(unsigned index) const
		{
			FLAC__ASSERT(is_valid());
			FLAC__ASSERT(index < object_->data.seek_table.num_points);
			return object_->data.seek_table.points[index];
		}

		void SeekTable::set_point(unsigned index, const ::FLAC__StreamMetadata_SeekPoint &point)
		{
			FLAC__ASSERT(is_valid());
			FLAC__ASSERT(index < object_->data.seek_table.num_points);
			::FLAC__metadata_object_seektable_set_point(object_, index, point);
		}

		bool SeekTable::insert_point(unsigned index, const ::FLAC__StreamMetadata_SeekPoint &point)
		{
			FLAC__ASSERT(is_valid());
			FLAC__ASSERT(index <= object_->data.seek_table.num_points);
			return (bool)::FLAC__metadata_object_seektable_insert_point(object_, index, point);
		}

		bool SeekTable::delete_point(unsigned index)
		{
			FLAC__ASSERT(is_valid());
			FLAC__ASSERT(index < object_->data.seek_table.num_points);
			return (bool)::FLAC__metadata_object_seektable_delete_point(object_, index);
		}

		bool SeekTable::is_legal() const
		{
			FLAC__ASSERT(is_valid());
			return (bool)::FLAC__metadata_object_seektable_is_legal(object_);
		}


		//
		// VorbisComment::Entry
		//

		VorbisComment::Entry::Entry()
		{
			zero();
		}

		VorbisComment::Entry::Entry(const char *field, unsigned field_length)
		{
			zero();
			construct(field, field_length);
		}

		VorbisComment::Entry::Entry(const char *field_name, const char *field_value, unsigned field_value_length)
		{
			zero();
			construct(field_name, field_value, field_value_length);
		}

		VorbisComment::Entry::Entry(const Entry &entry)
		{
			FLAC__ASSERT(entry.is_valid());
			zero();
			construct((const char *)entry.entry_.entry, entry.entry_.length);
		}

		void VorbisComment::Entry::operator=(const Entry &entry)
		{
			FLAC__ASSERT(entry.is_valid());
			clear();
			construct((const char *)entry.entry_.entry, entry.entry_.length);
		}

		VorbisComment::Entry::~Entry()
		{
			clear();
		}

		bool VorbisComment::Entry::is_valid() const
		{
			return is_valid_;
		}

		unsigned VorbisComment::Entry::get_field_length() const
		{
			FLAC__ASSERT(is_valid());
			return entry_.length;
		}

		unsigned VorbisComment::Entry::get_field_name_length() const
		{
			FLAC__ASSERT(is_valid());
			return field_name_length_;
		}

		unsigned VorbisComment::Entry::get_field_value_length() const
		{
			FLAC__ASSERT(is_valid());
			return field_value_length_;
		}

		::FLAC__StreamMetadata_VorbisComment_Entry VorbisComment::Entry::get_entry() const
		{
			FLAC__ASSERT(is_valid());
			return entry_;
		}

		const char *VorbisComment::Entry::get_field() const
		{
			FLAC__ASSERT(is_valid());
			return (const char *)entry_.entry;
		}

		const char *VorbisComment::Entry::get_field_name() const
		{
			FLAC__ASSERT(is_valid());
			return field_name_;
		}

		const char *VorbisComment::Entry::get_field_value() const
		{
			FLAC__ASSERT(is_valid());
			return field_value_;
		}

		bool VorbisComment::Entry::set_field(const char *field, unsigned field_length)
		{
			FLAC__ASSERT(is_valid());
			FLAC__ASSERT(0 != field);

			clear_entry();

			if(0 == (entry_.entry = (FLAC__byte*)malloc(field_length))) {
				is_valid_ = false;
			}
			else {
				entry_.length = field_length;
				memcpy(entry_.entry, field, field_length);
				(void) parse_field();
			}

			return is_valid_;
		}

		bool VorbisComment::Entry::set_field_name(const char *field_name)
		{
			FLAC__ASSERT(is_valid());
			FLAC__ASSERT(0 != field_name);

			clear_field_name();

			if(0 == (field_name_ = strdup(field_name))) {
				is_valid_ = false;
			}
			else {
				field_name_length_ = strlen(field_name_);
				compose_field();
			}

			return is_valid_;
		}

		bool VorbisComment::Entry::set_field_value(const char *field_value, unsigned field_value_length)
		{
			FLAC__ASSERT(is_valid());
			FLAC__ASSERT(0 != field_value);

			clear_field_value();

			if(0 == (field_value_ = (char *)malloc(field_value_length))) {
				is_valid_ = false;
			}
			else {
				field_value_length_ = field_value_length;
				memcpy(field_value_, field_value, field_value_length);
				compose_field();
			}

			return is_valid_;
		}

		void VorbisComment::Entry::zero()
		{
			is_valid_ = true;
			entry_.length = 0;
			entry_.entry = 0;
			field_name_ = 0;
			field_name_length_ = 0;
			field_value_ = 0;
			field_value_length_ = 0;
		}

		void VorbisComment::Entry::clear()
		{
			clear_entry();
			clear_field_name();
			clear_field_value();
			is_valid_ = true;
		}

		void VorbisComment::Entry::clear_entry()
		{
			if(0 != entry_.entry) {
				free(entry_.entry);
				entry_.entry = 0;
				entry_.length = 0;
			}
		}

		void VorbisComment::Entry::clear_field_name()
		{
			if(0 != field_name_) {
				free(field_name_);
				field_name_ = 0;
				field_name_length_ = 0;
			}
		}

		void VorbisComment::Entry::clear_field_value()
		{
			if(0 != field_value_) {
				free(field_value_);
				field_value_ = 0;
				field_value_length_ = 0;
			}
		}

		void VorbisComment::Entry::construct(const char *field, unsigned field_length)
		{
			if(set_field(field, field_length))
				parse_field();
		}

		void VorbisComment::Entry::construct(const char *field_name, const char *field_value, unsigned field_value_length)
		{
			if(set_field_name(field_name) && set_field_value(field_value, field_value_length))
				compose_field();
		}

		void VorbisComment::Entry::compose_field()
		{
			clear_entry();

			if(0 == (entry_.entry = (FLAC__byte*)malloc(field_name_length_ + 1 + field_value_length_))) {
				is_valid_ = false;
			}
			else {
				memcpy(entry_.entry, field_name_, field_name_length_);
				entry_.length += field_name_length_;
				memcpy(entry_.entry + entry_.length, "=", 1);
				entry_.length += 1;
				memcpy(entry_.entry + entry_.length, field_value_, field_value_length_);
				entry_.length += field_value_length_;
				is_valid_ = true;
			}
		}

		void VorbisComment::Entry::parse_field()
		{
			clear_field_name();
			clear_field_value();

			const char *p = (const char *)memchr(entry_.entry, '=', entry_.length);

			if(0 == p)
				p = (const char *)entry_.entry + entry_.length;

			field_name_length_ = p - (const char *)entry_.entry;
			if(0 == (field_name_ = (char *)malloc(field_name_length_ + 1))) { // +1 for the trailing \0
				is_valid_ = false;
				return;
			}
			memcpy(field_name_, entry_.entry, field_name_length_);
			field_name_[field_name_length_] = '\0';

			if(entry_.length - field_name_length_ == 0) {
				field_value_length_ = 0;
				if(0 == (field_value_ = (char *)malloc(0))) {
					is_valid_ = false;
					return;
				}
			}
			else {
				field_value_length_ = entry_.length - field_name_length_ - 1;
				if(0 == (field_value_ = (char *)malloc(field_value_length_))) {
					is_valid_ = false;
					return;
				}
				memcpy(field_value_, ++p, field_value_length_);
			}

			is_valid_ = true;
		}


		//
		// VorbisComment
		//

		VorbisComment::VorbisComment():
		Prototype(FLAC__metadata_object_new(FLAC__METADATA_TYPE_VORBIS_COMMENT), /*copy=*/false)
		{ }

		VorbisComment::~VorbisComment()
		{ }

		unsigned VorbisComment::get_num_comments() const
		{
			FLAC__ASSERT(is_valid());
			return object_->data.vorbis_comment.num_comments;
		}

		VorbisComment::Entry VorbisComment::get_vendor_string() const
		{
			FLAC__ASSERT(is_valid());
			return Entry((const char *)object_->data.vorbis_comment.vendor_string.entry, object_->data.vorbis_comment.vendor_string.length);
		}

		VorbisComment::Entry VorbisComment::get_comment(unsigned index) const
		{
			FLAC__ASSERT(is_valid());
			FLAC__ASSERT(index < object_->data.vorbis_comment.num_comments);
			return Entry((const char *)object_->data.vorbis_comment.comments[index].entry, object_->data.vorbis_comment.comments[index].length);
		}

		bool VorbisComment::set_vendor_string(const VorbisComment::Entry &entry)
		{
			FLAC__ASSERT(is_valid());
			// vendor_string is a special kind of entry
			::FLAC__StreamMetadata_VorbisComment_Entry vendor_string;
			vendor_string.length = entry.get_field_name_length();
			vendor_string.entry = (FLAC__byte*)entry.get_field_name(); // we can cheat on const-ness because we make a copy below:
			return (bool)::FLAC__metadata_object_vorbiscomment_set_vendor_string(object_, vendor_string, /*copy=*/true);
		}

		bool VorbisComment::set_comment(unsigned index, const VorbisComment::Entry &entry)
		{
			FLAC__ASSERT(is_valid());
			FLAC__ASSERT(index < object_->data.vorbis_comment.num_comments);
			return (bool)::FLAC__metadata_object_vorbiscomment_set_comment(object_, index, entry.get_entry(), /*copy=*/true);
		}

		bool VorbisComment::insert_comment(unsigned index, const VorbisComment::Entry &entry)
		{
			FLAC__ASSERT(is_valid());
			FLAC__ASSERT(index <= object_->data.vorbis_comment.num_comments);
			return (bool)::FLAC__metadata_object_vorbiscomment_insert_comment(object_, index, entry.get_entry(), /*copy=*/true);
		}

		bool VorbisComment::delete_comment(unsigned index)
		{
			FLAC__ASSERT(is_valid());
			FLAC__ASSERT(index < object_->data.vorbis_comment.num_comments);
			return (bool)::FLAC__metadata_object_vorbiscomment_delete_comment(object_, index);
		}


		//
		// CueSheet::Track
		//

		CueSheet::Track::Track():
		object_(::FLAC__metadata_object_cuesheet_track_new())
		{ }

		CueSheet::Track::Track(const ::FLAC__StreamMetadata_CueSheet_Track *track):
		object_(::FLAC__metadata_object_cuesheet_track_clone(track))
		{ }

		CueSheet::Track::Track(const Track &track):
		object_(::FLAC__metadata_object_cuesheet_track_clone(track.object_))
		{ }

		void CueSheet::Track::operator=(const Track &track)
		{
			object_ = ::FLAC__metadata_object_cuesheet_track_clone(track.object_);
		}

		CueSheet::Track::~Track()
		{
			if(0 != object_)
				delete object_;
		}

		bool CueSheet::Track::is_valid() const
		{
			return(0 != object_);
		}

		::FLAC__StreamMetadata_CueSheet_Index CueSheet::Track::get_index(unsigned i) const
		{
			FLAC__ASSERT(is_valid());
			FLAC__ASSERT(i < object_->num_indices);
			return object_->indices[i];
		}

		void CueSheet::Track::set_isrc(const char value[12])
		{
			FLAC__ASSERT(is_valid());
			FLAC__ASSERT(0 != value);
			memcpy(object_->isrc, value, 12);
			object_->isrc[12] = '\0';
		}

		void CueSheet::Track::set_type(unsigned value)
		{
			FLAC__ASSERT(is_valid());
			FLAC__ASSERT(value <= 1);
			object_->type = value;
		}

 		void CueSheet::Track::set_index(unsigned i, const ::FLAC__StreamMetadata_CueSheet_Index &index)
 		{
 			FLAC__ASSERT(is_valid());
 			FLAC__ASSERT(i < object_->num_indices);
 			object_->indices[i] = index;
 		}


		//
		// CueSheet
		//

		CueSheet::CueSheet():
		Prototype(FLAC__metadata_object_new(FLAC__METADATA_TYPE_CUESHEET), /*copy=*/false)
		{ }

		CueSheet::~CueSheet()
		{ }

		const char *CueSheet::get_media_catalog_number() const
		{
			FLAC__ASSERT(is_valid());
			return object_->data.cue_sheet.media_catalog_number;
		}

		FLAC__uint64 CueSheet::get_lead_in() const
		{
			FLAC__ASSERT(is_valid());
			return object_->data.cue_sheet.lead_in;
		}

		unsigned CueSheet::get_num_tracks() const
		{
			FLAC__ASSERT(is_valid());
			return object_->data.cue_sheet.num_tracks;
		}

		CueSheet::Track CueSheet::get_track(unsigned i) const
		{
			FLAC__ASSERT(is_valid());
			FLAC__ASSERT(i < object_->data.cue_sheet.num_tracks);
			return Track(object_->data.cue_sheet.tracks + i);
		}

		void CueSheet::set_media_catalog_number(const char value[128])
		{
			FLAC__ASSERT(is_valid());
			FLAC__ASSERT(0 != value);
			memcpy(object_->data.cue_sheet.media_catalog_number, value, 128);
			object_->data.cue_sheet.media_catalog_number[128] = '\0';
		}

		void CueSheet::set_lead_in(FLAC__uint64 value)
		{
			FLAC__ASSERT(is_valid());
			object_->data.cue_sheet.lead_in = value;
		}

		void CueSheet::set_index(unsigned track_num, unsigned index_num, const ::FLAC__StreamMetadata_CueSheet_Index &index)
		{
			FLAC__ASSERT(is_valid());
			FLAC__ASSERT(track_num < object_->data.cue_sheet.num_tracks);
			FLAC__ASSERT(index_num < object_->data.cue_sheet.tracks[track_num].num_indices);
			object_->data.cue_sheet.tracks[track_num].indices[index_num] = index;
		}

		bool CueSheet::insert_index(unsigned track_num, unsigned index_num, const ::FLAC__StreamMetadata_CueSheet_Index &index)
		{
			FLAC__ASSERT(is_valid());
			FLAC__ASSERT(track_num < object_->data.cue_sheet.num_tracks);
			FLAC__ASSERT(index_num <= object_->data.cue_sheet.tracks[track_num].num_indices);
			return (bool)::FLAC__metadata_object_cuesheet_track_insert_index(object_, track_num, index_num, index);
		}

		bool CueSheet::delete_index(unsigned track_num, unsigned index_num)
		{
			FLAC__ASSERT(is_valid());
			FLAC__ASSERT(track_num < object_->data.cue_sheet.num_tracks);
			FLAC__ASSERT(index_num < object_->data.cue_sheet.tracks[track_num].num_indices);
			return (bool)::FLAC__metadata_object_cuesheet_track_delete_index(object_, track_num, index_num);
		}

		bool CueSheet::set_track(unsigned i, const CueSheet::Track &track)
		{
			FLAC__ASSERT(is_valid());
			FLAC__ASSERT(i < object_->data.cue_sheet.num_tracks);
			// We can safely const_cast since copy=true
			return (bool)::FLAC__metadata_object_cuesheet_set_track(object_, i, const_cast< ::FLAC__StreamMetadata_CueSheet_Track*>(track.get_track()), /*copy=*/true);
		}

		bool CueSheet::insert_track(unsigned i, const CueSheet::Track &track)
		{
			FLAC__ASSERT(is_valid());
			FLAC__ASSERT(i <= object_->data.cue_sheet.num_tracks);
			// We can safely const_cast since copy=true
			return (bool)::FLAC__metadata_object_cuesheet_insert_track(object_, i, const_cast< ::FLAC__StreamMetadata_CueSheet_Track*>(track.get_track()), /*copy=*/true);
		}

		bool CueSheet::delete_track(unsigned i)
		{
			FLAC__ASSERT(is_valid());
			FLAC__ASSERT(i < object_->data.cue_sheet.num_tracks);
			return (bool)::FLAC__metadata_object_cuesheet_delete_track(object_, i);
		}

		bool CueSheet::is_legal(bool check_cd_da_subset, const char **violation) const
		{
			FLAC__ASSERT(is_valid());
			return (bool)::FLAC__metadata_object_cuesheet_is_legal(object_, check_cd_da_subset, violation);
		}


		// ============================================================
		//
		//  Level 0
		//
		// ============================================================

		FLACPP_API bool get_streaminfo(const char *filename, StreamInfo &streaminfo)
		{
			FLAC__ASSERT(0 != filename);

			::FLAC__StreamMetadata s;

			if(::FLAC__metadata_get_streaminfo(filename, &s)) {
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

		bool SimpleIterator::init(const char *filename, bool read_only, bool preserve_file_stats)
		{
			FLAC__ASSERT(0 != filename);
			FLAC__ASSERT(is_valid());
			return (bool)::FLAC__metadata_simple_iterator_init(iterator_, filename, read_only, preserve_file_stats);
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
			return (bool)::FLAC__metadata_simple_iterator_is_writable(iterator_);
		}

		bool SimpleIterator::next()
		{
			FLAC__ASSERT(is_valid());
			return (bool)::FLAC__metadata_simple_iterator_next(iterator_);
		}

		bool SimpleIterator::prev()
		{
			FLAC__ASSERT(is_valid());
			return (bool)::FLAC__metadata_simple_iterator_prev(iterator_);
		}

		::FLAC__MetadataType SimpleIterator::get_block_type() const
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
			return (bool)::FLAC__metadata_simple_iterator_set_block(iterator_, block->object_, use_padding);
		}

		bool SimpleIterator::insert_block_after(Prototype *block, bool use_padding)
		{
			FLAC__ASSERT(0 != block);
			FLAC__ASSERT(is_valid());
			return (bool)::FLAC__metadata_simple_iterator_insert_block_after(iterator_, block->object_, use_padding);
		}

		bool SimpleIterator::delete_block(bool use_padding)
		{
			FLAC__ASSERT(is_valid());
			return (bool)::FLAC__metadata_simple_iterator_delete_block(iterator_, use_padding);
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
			return (bool)::FLAC__metadata_chain_read(chain_, filename);
		}

		bool Chain::write(bool use_padding, bool preserve_file_stats)
		{
			FLAC__ASSERT(is_valid());
			return (bool)::FLAC__metadata_chain_write(chain_, use_padding, preserve_file_stats);
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

		void Iterator::init(Chain &chain)
		{
			FLAC__ASSERT(is_valid());
			FLAC__ASSERT(chain.is_valid());
			::FLAC__metadata_iterator_init(iterator_, chain.chain_);
		}

		bool Iterator::next()
		{
			FLAC__ASSERT(is_valid());
			return (bool)::FLAC__metadata_iterator_next(iterator_);
		}

		bool Iterator::prev()
		{
			FLAC__ASSERT(is_valid());
			return (bool)::FLAC__metadata_iterator_prev(iterator_);
		}

		::FLAC__MetadataType Iterator::get_block_type() const
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
			bool ret = (bool)::FLAC__metadata_iterator_set_block(iterator_, block->object_);
			if(ret) {
				block->set_reference(true);
				delete block;
			}
			return ret;
		}

		bool Iterator::delete_block(bool replace_with_padding)
		{
			FLAC__ASSERT(is_valid());
			return (bool)::FLAC__metadata_iterator_delete_block(iterator_, replace_with_padding);
		}

		bool Iterator::insert_block_before(Prototype *block)
		{
			FLAC__ASSERT(0 != block);
			FLAC__ASSERT(is_valid());
			bool ret = (bool)::FLAC__metadata_iterator_insert_block_before(iterator_, block->object_);
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
			bool ret = (bool)::FLAC__metadata_iterator_insert_block_after(iterator_, block->object_);
			if(ret) {
				block->set_reference(true);
				delete block;
			}
			return ret;
		}

	};
};
