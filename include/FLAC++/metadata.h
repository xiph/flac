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

#ifndef FLACPP__METADATA_H
#define FLACPP__METADATA_H

#include "FLAC/metadata.h"

// ===============================================================
//
//  Full documentation for the metadata interface can be found
//  in the C layer in include/FLAC/metadata.h
//
// ===============================================================


namespace FLAC {
	namespace Metadata {

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
			StreamInfo();
			StreamInfo(::FLAC__StreamMetaData *object, bool copy = false);
			~StreamInfo();

			inline void operator=(const StreamInfo &object) { Prototype::operator=(object); }
			inline void operator=(const ::FLAC__StreamMetaData &object) { Prototype::operator=(object); }
			inline void operator=(const ::FLAC__StreamMetaData *object) { Prototype::operator=(object); }

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


		// ============================================================
		//
		//  Level 0
		//
		// ============================================================

		bool get_streaminfo(const char *filename, StreamInfo &streaminfo);


		// ============================================================
		//
		//  Level 1
		//
		//  ----------------------------------------------------------
		//
		//  The flow through the iterator in the C++ layer is similar
		//  to the C layer:
		//
		//    * Create a SimpleIterator instance
		//    * Check SimpleIterator::is_valid()
		//    * Call SimpleIterator::init() and check the return
		//    * Traverse and/or edit.  Edits are written to file
		//      immediately.
		//    * Destroy the SimpleIterator instance
		//
		//  ----------------------------------------------------------
		//
		//  The ownership of pointers in the C++ layer follows that in
		//  the C layer, i.e.
		//    * The objects returned by get_block() are yours to
		//      modify, but changes are not reflected in the FLAC file
		//      until you call set_block().  The objects are also
		//      yours to delete; they are not automatically deleted
		//      when passed to set_block() or insert_block_after().
		//
		// ============================================================

		class SimpleIterator {
		public:
			class Status {
			public:
				inline Status(::FLAC__MetaData_SimpleIteratorStatus status): status_(status) { }
				inline operator ::FLAC__MetaData_SimpleIteratorStatus() const { return status_; }
				inline const char *as_cstring() const { return ::FLAC__MetaData_SimpleIteratorStatusString[status_]; }
			protected:
				::FLAC__MetaData_SimpleIteratorStatus status_;
			};

			SimpleIterator();
			virtual ~SimpleIterator();

			bool init(const char *filename, bool preserve_file_stats = false);

			bool is_valid() const;
			inline operator bool() const { return is_valid(); }
			Status status();
			bool is_writable() const;

			bool next();
			bool prev();

			::FLAC__MetaDataType get_block_type() const;
			Prototype *get_block();
			bool set_block(Prototype *block, bool use_padding = true);
			bool insert_block_after(Prototype *block, bool use_padding = true);
			bool delete_block(bool use_padding = true);

		protected:
			::FLAC__MetaData_SimpleIterator *iterator_;
			void clear();
		};


		// ============================================================
		//
		//  Level 2
		//
		//  ----------------------------------------------------------
		//
		//  The flow through the iterator in the C++ layer is similar
		//  to the C layer:
		//
		//    * Create a Chain instance
		//    * Check Chain::is_valid()
		//    * Call Chain::read() and check the return
		//    * Traverse and/or edit with an Iterator or with
		//      Chain::merge_padding() or Chain::sort_padding()
		//    * Write changes back to FLAC file with Chain::write()
		//    * Destroy the Chain instance
		//
		//  ----------------------------------------------------------
		//
		//  The ownership of pointers in the C++ layer follows that in
		//  the C layer, i.e.
		//    * The objects returned by Iterator::get_block() are
		//      owned by the iterator and should not be deleted.
		//      When you modify the block, you are directly editing
		//      what's in the chain and do not need to call
		//      Iterator::set_block().  However the changes will not
		//      be reflected in the FLAC file until the chain is
		//      written with Chain::write().
		//
		//    * When you pass an object to Iterator::set_block(),
		//      Iterator::insert_block_before(), or
		//      Iterator::insert_block_after(), the iterator takes
		//      ownership of the block and it will be deleted with the
		//      chain.
		//
		// ============================================================

		class Chain {
		public:
			class Status {
			public:
				inline Status(::FLAC__MetaData_ChainStatus status): status_(status) { }
				inline operator ::FLAC__MetaData_ChainStatus() const { return status_; }
				inline const char *as_cstring() const { return ::FLAC__MetaData_ChainStatusString[status_]; }
			protected:
				::FLAC__MetaData_ChainStatus status_;
			};

			Chain();
			virtual ~Chain();

			friend class Iterator;

			bool is_valid() const;
			inline operator bool() const { return is_valid(); }
			Status status();

			bool read(const char *filename);
			bool write(bool use_padding = true, bool preserve_file_stats = false);

			void merge_padding();
			void sort_padding();

		protected:
			::FLAC__MetaData_Chain *chain_;
			virtual void clear();
		};

		class Iterator {
		public:
			Iterator();
			virtual ~Iterator();

			bool is_valid() const;
			inline operator bool() const { return is_valid(); }

			void init(Chain *chain);

			bool next();
			bool prev();

			::FLAC__MetaDataType get_block_type() const;
			Prototype *get_block();
			bool set_block(Prototype *block);
			bool delete_block(bool replace_with_padding);
			bool insert_block_before(Prototype *block);
			bool insert_block_after(Prototype *block);

		protected:
			::FLAC__MetaData_Iterator *iterator_;
			virtual void clear();
		};

	};
};

#endif
