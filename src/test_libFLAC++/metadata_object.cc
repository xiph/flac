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

#include "FLAC/assert.h"
#include "FLAC++/metadata.h"
#include <stdio.h>
#include <stdlib.h> /* for malloc() */
#include <string.h> /* for memcmp() */

static ::FLAC__StreamMetaData streaminfo_, padding_, seektable_, application_, vorbiscomment_;

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
	seektable_.data.seek_table.points = (::FLAC__StreamMetaData_SeekPoint*)malloc_or_die_(seektable_.data.seek_table.num_points * sizeof(::FLAC__StreamMetaData_SeekPoint));
	seektable_.data.seek_table.points[0].sample_number = 0;
	seektable_.data.seek_table.points[0].stream_offset = 0;
	seektable_.data.seek_table.points[0].frame_samples = streaminfo_.data.stream_info.min_blocksize;
	seektable_.data.seek_table.points[1].sample_number = ::FLAC__STREAM_METADATA_SEEKPOINT_PLACEHOLDER;
	seektable_.data.seek_table.points[1].stream_offset = 1000;
	seektable_.data.seek_table.points[1].frame_samples = streaminfo_.data.stream_info.min_blocksize;

    application_.is_last = false;
    application_.type = ::FLAC__METADATA_TYPE_APPLICATION;
    application_.length = 8;
	memcpy(application_.data.application.id, "\xfe\xdc\xba\x98", 4);
	application_.data.application.data = (FLAC__byte*)malloc_or_die_(4);
	memcpy(application_.data.application.data, "\xf0\xe1\xd2\xc3", 4);

    vorbiscomment_.is_last = true;
    vorbiscomment_.type = ::FLAC__METADATA_TYPE_VORBIS_COMMENT;
    vorbiscomment_.length = (4 + 8) + 4 + (4 + 5) + (4 + 0);
	vorbiscomment_.data.vorbis_comment.vendor_string.length = 8;
	vorbiscomment_.data.vorbis_comment.vendor_string.entry = (FLAC__byte*)malloc_or_die_(8);
	memcpy(vorbiscomment_.data.vorbis_comment.vendor_string.entry, "flac 1.x", 8);
	vorbiscomment_.data.vorbis_comment.num_comments = 2;
	vorbiscomment_.data.vorbis_comment.comments = (::FLAC__StreamMetaData_VorbisComment_Entry*)malloc_or_die_(vorbiscomment_.data.vorbis_comment.num_comments * sizeof(::FLAC__StreamMetaData_VorbisComment_Entry));
	vorbiscomment_.data.vorbis_comment.comments[0].length = 5;
	vorbiscomment_.data.vorbis_comment.comments[0].entry = (FLAC__byte*)malloc_or_die_(5);
	memcpy(vorbiscomment_.data.vorbis_comment.comments[0].entry, "ab=cd", 5);
	vorbiscomment_.data.vorbis_comment.comments[1].length = 0;
	vorbiscomment_.data.vorbis_comment.comments[1].entry = 0;
}

static void free_metadata_blocks_()
{
	free(seektable_.data.seek_table.points);
	free(application_.data.application.data);
	free(vorbiscomment_.data.vorbis_comment.vendor_string.entry);
	free(vorbiscomment_.data.vorbis_comment.comments[0].entry);
	free(vorbiscomment_.data.vorbis_comment.comments);
}

bool test_metadata_object_streaminfo()
{
	FLAC::Metadata::StreamInfo block;
	unsigned expected_length;

	printf("\n+++ unit test: metadata objects (libFLAC++)\n\n");


	printf("testing FLAC::Metadata::StreamInfo\n");

	printf("testing FLAC::Metadata::StreamInfo::StreamInfo()... ");
	if(!block.is_valid()) {
		printf("FAILED, !block.is_valid()\n");
		return false;
	}
	expected_length = FLAC__STREAM_METADATA_STREAMINFO_LENGTH;
	if(block.get_length() != expected_length) {
		printf("FAILED, bad length, expected %u, got %u\n", expected_length, block.get_length());
		return false;
    }
	printf("OK\n");

	printf("testing FLAC::MetaData::StreamInfo::StreamInfo(const StreamInfo &)... ");
	{
		FLAC::Metadata::StreamInfo blockcopy(block);
		if(blockcopy != block) {
			printf("FAILED, copy is not identical to original\n");
			return false;
		}
		printf("OK\n");

		printf("testing FLAC::Metadata::StreamInfo::~StreamInfo()... ");
	}
	printf("OK\n");

	printf("testing FLAC::Metadata::StreamInfo::operator=(const ::FLAC__StreamMetaData &)... ");
	{
		FLAC::Metadata::StreamInfo blockcopy(streaminfo_, /*copy=*/true);
		if(!blockcopy.is_valid()) {
			printf("FAILED, !block.is_valid()\n");
			return false;
		}
		if(blockcopy != streaminfo_) {
			printf("FAILED, copy is not identical to original\n");
			return false;
		}
	}
	printf("OK\n");

	printf("testing FLAC::Metadata::StreamInfo::operator=(const ::FLAC__StreamMetaData *)... ");
	{
		FLAC::Metadata::StreamInfo blockcopy(&streaminfo_, /*copy=*/true);
		if(!blockcopy.is_valid()) {
			printf("FAILED, !block.is_valid()\n");
			return false;
		}
		if(blockcopy != streaminfo_) {
			printf("FAILED, copy is not identical to original\n");
			return false;
		}
	}
	printf("OK\n");

	printf("testing FLAC::Metadata::StreamInfo::operator=(const StreamInfo &)... ");
	{
		FLAC::Metadata::StreamInfo blockcopy = block;
		if(!blockcopy.is_valid()) {
			printf("FAILED, !block.is_valid()\n");
			return false;
		}
		if(blockcopy != block) {
			printf("FAILED, copy is not identical to original\n");
			return false;
		}
	}
	printf("OK\n");

	return true;
}

bool test_metadata_object()
{
	init_metadata_blocks_();

	if(!test_metadata_object_streaminfo())
		return false;

	free_metadata_blocks_();

	return true;
}

#if 0


		// ============================================================
		//
		//  Metadata objects
		//
		// ============================================================

		// NOTE: When the get_*() methods return you a const pointer,
		// DO NOT disobey and write into it.  Always use the set_*()
		// methods.

		// base class for all metadata blocks
		class Prototype {
		protected:
			Prototype(::FLAC__StreamMetaData *object, bool copy);
			virtual ~Prototype();

			void operator=(const Prototype &);
			void operator=(const ::FLAC__StreamMetaData &);
			void operator=(const ::FLAC__StreamMetaData *);

			inline bool operator==(const FLAC::Metadata::Prototype &block)
			{ return ::FLAC__metadata_object_is_equal(object_, block.object_); }

			virtual void clear();

			::FLAC__StreamMetaData *object_;
		public:
			friend class SimpleIterator;
			friend class Iterator;

			inline bool is_valid() const { return 0 != object_; }
			inline operator bool() const { return is_valid(); }

			bool get_is_last() const;
			FLAC__MetaDataType get_type() const;
			unsigned get_length() const; // NOTE: does not include the header, per spec
		private:
			Prototype(); // Private and undefined so you can't use it

			// These are used only by Iterator
			bool is_reference_;
			inline void set_reference(bool x) { is_reference_ = x; }
		};

		class StreamInfo : public Prototype {
		public:
			unsigned get_min_blocksize() const;
			unsigned get_max_blocksize() const;
			unsigned get_min_framesize() const;
			unsigned get_max_framesize() const;
			unsigned get_sample_rate() const;
			unsigned get_channels() const;
			unsigned get_bits_per_sample() const;
			FLAC__uint64 get_total_samples() const;
			const FLAC__byte *get_md5sum() const;

			void set_min_blocksize(unsigned value);
			void set_max_blocksize(unsigned value);
			void set_min_framesize(unsigned value);
			void set_max_framesize(unsigned value);
			void set_sample_rate(unsigned value);
			void set_channels(unsigned value);
			void set_bits_per_sample(unsigned value);
			void set_total_samples(FLAC__uint64 value);
			void set_md5sum(const FLAC__byte value[16]);
		};

		class Padding : public Prototype {
		public:
			Padding();
			Padding(::FLAC__StreamMetaData *object, bool copy = false);
			~Padding();

			inline void operator=(const Padding &object) { Prototype::operator=(object); }
			inline void operator=(const ::FLAC__StreamMetaData &object) { Prototype::operator=(object); }
			inline void operator=(const ::FLAC__StreamMetaData *object) { Prototype::operator=(object); }

			void set_length(unsigned length);
		};

		class Application : public Prototype {
		public:
			Application();
			Application(::FLAC__StreamMetaData *object, bool copy = false);
			~Application();

			inline void operator=(const Application &object) { Prototype::operator=(object); }
			inline void operator=(const ::FLAC__StreamMetaData &object) { Prototype::operator=(object); }
			inline void operator=(const ::FLAC__StreamMetaData *object) { Prototype::operator=(object); }

			const FLAC__byte *get_id() const;
			const FLAC__byte *get_data() const;

			void set_id(FLAC__byte value[4]);
			bool set_data(FLAC__byte *data, unsigned length, bool copy = false);
		};

		class SeekTable : public Prototype {
		public:
			SeekTable();
			SeekTable(::FLAC__StreamMetaData *object, bool copy = false);
			~SeekTable();

			inline void operator=(const SeekTable &object) { Prototype::operator=(object); }
			inline void operator=(const ::FLAC__StreamMetaData &object) { Prototype::operator=(object); }
			inline void operator=(const ::FLAC__StreamMetaData *object) { Prototype::operator=(object); }

			unsigned get_num_points() const;
			::FLAC__StreamMetaData_SeekPoint get_point(unsigned index) const;

			void set_point(unsigned index, const ::FLAC__StreamMetaData_SeekPoint &point);
			bool insert_point(unsigned index, const ::FLAC__StreamMetaData_SeekPoint &point);
			bool delete_point(unsigned index);

			bool is_legal() const;
		};

		class VorbisComment : public Prototype {
		public:
			class Entry {
			public:
				Entry();
				Entry(const char *field, unsigned field_length);
				Entry(const char *field_name, const char *field_value, unsigned field_value_length);
				Entry(const Entry &entry);
				void operator=(const Entry &entry);

				virtual ~Entry();

				virtual bool is_valid() const;
				inline operator bool() const { return is_valid(); }

				unsigned get_field_length() const;
				unsigned get_field_name_length() const;
				unsigned get_field_value_length() const;

				::FLAC__StreamMetaData_VorbisComment_Entry get_entry() const;
				const char *get_field() const;
				const char *get_field_name() const;
				const char *get_field_value() const;

				bool set_field(const char *field, unsigned field_length);
				bool set_field_name(const char *field_name);
				bool set_field_value(const char *field_value, unsigned field_value_length);
			protected:
				bool is_valid_;
				::FLAC__StreamMetaData_VorbisComment_Entry entry_;
				char *field_name_;
				unsigned field_name_length_;
				char *field_value_;
				unsigned field_value_length_;
			private:
				void zero();
				void clear();
				void clear_entry();
				void clear_field_name();
				void clear_field_value();
				void construct(const char *field, unsigned field_length);
				void construct(const char *field_name, const char *field_value, unsigned field_value_length);
				void compose_field();
				void parse_field();
			};

			VorbisComment();
			VorbisComment(::FLAC__StreamMetaData *object, bool copy = false);
			~VorbisComment();

			inline void operator=(const VorbisComment &object) { Prototype::operator=(object); }
			inline void operator=(const ::FLAC__StreamMetaData &object) { Prototype::operator=(object); }
			inline void operator=(const ::FLAC__StreamMetaData *object) { Prototype::operator=(object); }

			unsigned get_num_comments() const;
			Entry get_vendor_string() const;
			Entry get_comment(unsigned index) const;

			bool set_vendor_string(const Entry &entry);
			bool set_comment(unsigned index, const Entry &entry);
			bool insert_comment(unsigned index, const Entry &entry);
			bool delete_comment(unsigned index);
		};





#endif
