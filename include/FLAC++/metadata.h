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

/** \file include/FLAC++/metadata.h
 *
 *  \brief
 *  This module provides classes for creating and manipulating FLAC
 *  metadata blocks in memory, and three progressively more powerful
 *  interfaces for traversing and editing metadata in FLAC files.
 *
 *  See the detailed documentation for each interface in the
 *  \link flacpp_metadata metadata \endlink module.
 */

/** \defgroup flacpp_metadata FLAC++/metadata.h: metadata interfaces
 *  \ingroup flacpp
 *
 *  \brief
 *  This module provides classes for creating and manipulating FLAC
 *  metadata blocks in memory, and three progressively more powerful
 *  interfaces for traversing and editing metadata in FLAC files.
 *
 *  The behavior closely mimics the C layer interface; be sure to read
 *  the detailed description of the
 *  \link flac_metadata C metadata module \endlink.
 */


namespace FLAC {
	namespace Metadata {

		// ============================================================
		//
		//  Metadata objects
		//
		// ============================================================

		/** \defgroup flacpp_metadata_object FLAC++/metadata.h: metadata object classes
		 *  \ingroup flacpp_metadata
		 *
		 * This module contains classes representing FLAC metadata
		 * blocks in memory.
		 *
		 * The behavior closely mimics the C layer interface; be
		 * sure to read the detailed description of the
		 * \link flac_metadata_object C metadata object module \endlink.
		 *
		 * Any time a metadata object is constructed or assigned, you
		 * should check is_valid() to make sure the underlying
		 * ::FLAC__StreamMetadata object was able to be created.
		 *
		 * \warning
		 * When the get_*() methods of any metadata object method
		 * return you a const pointer, DO NOT disobey and write into it.
		 * Always use the set_*() methods.
		 *
		 * \{
		 */

		/** Base class for all metadata block types.
		 */
		class Prototype {
		protected:
			//@{
			/** Constructs a copy of the given object.  This form
			 *  always performs a deep copy.
			 */
			Prototype(const Prototype &);
			Prototype(const ::FLAC__StreamMetadata &);
			Prototype(const ::FLAC__StreamMetadata *);
			//@}

			/** Constructs an object with copy control.  When \a copy
			 *  is \c true, behaves identically to
			 *  FLAC::Metadata::Prototype::Prototype(const ::FLAC__StreamMetadata *object).
			 *  When \a copy is \c false, the instance takes ownership of
			 *  the pointer and the ::FLAC__StreamMetadata object will
			 *  be freed by the destructor.
			 *
			 *  \assert
			 *    \code object != NULL \endcode
			 */
			Prototype(::FLAC__StreamMetadata *object, bool copy);

			//@{
			/** Assign from another object.  Always performs a deep copy. */
			void operator=(const Prototype &);
			void operator=(const ::FLAC__StreamMetadata &);
			void operator=(const ::FLAC__StreamMetadata *);
			//@}

			/** Deletes the underlying ::FLAC__StreamMetadata object.
			 */
			virtual void clear();

			::FLAC__StreamMetadata *object_;
		public:
			/** Deletes the underlying ::FLAC__StreamMetadata object.
			 */
			virtual ~Prototype();

			//@{
			/** Check for equality, performing a deep compare by following pointers. */
			inline bool operator==(const Prototype &) const;
			inline bool operator==(const ::FLAC__StreamMetadata &) const;
			inline bool operator==(const ::FLAC__StreamMetadata *) const;
			//@}

			//@{
			/** Check for inequality, performing a deep compare by following pointers. */
			inline bool operator!=(const Prototype &) const;
			inline bool operator!=(const ::FLAC__StreamMetadata &) const;
			inline bool operator!=(const ::FLAC__StreamMetadata *) const;
			//@}

			friend class SimpleIterator;
			friend class Iterator;

			/** Returns \c true if the object was correctly constructed
			 *  (i.e. the underlying ::FLAC__StreamMetadata object was
			 *  properly allocated), else \c false.
			 */
			inline bool is_valid() const;

			/** Returns \c true if this block is the last block in a
			 *  stream, else \c false.
			 *
			 * \assert
			 *   \code is_valid() \endcode
			 */
			bool get_is_last() const;

			/** Returns the type of the block.
			 *
			 * \assert
			 *   \code is_valid() \endcode
			 */
			::FLAC__MetadataType get_type() const;

			/** Returns the stream length of the metadata block.
			 *
			 * \note
			 *   The length does not include the metadata block header,
			 *   per spec.
			 *
			 * \assert
			 *   \code is_valid() \endcode
			 */
			unsigned get_length() const;

			/** Sets the "is_last" flag for the block.  When using the iterators
			 *  it is not necessary to set this flag; they will do it for you.
			 *
			 * \assert
			 *   \code is_valid() \endcode
			 */
			void set_is_last(bool);
		private:
			/** Private and undefined so you can't use it. */
			Prototype();

			// These are used only by Iterator
			bool is_reference_;
			inline void set_reference(bool x) { is_reference_ = x; }
		};

		inline bool Prototype::operator==(const Prototype &object) const 
		{ return (bool)::FLAC__metadata_object_is_equal(object_, object.object_); }

		inline bool Prototype::operator==(const ::FLAC__StreamMetadata &object) const 
		{ return (bool)::FLAC__metadata_object_is_equal(object_, &object); }

		inline bool Prototype::operator==(const ::FLAC__StreamMetadata *object) const 
		{ return (bool)::FLAC__metadata_object_is_equal(object_, object); }

		inline bool Prototype::operator!=(const Prototype &object) const 
		{ return !operator==(object); }

		inline bool Prototype::operator!=(const ::FLAC__StreamMetadata &object) const 
		{ return !operator==(object); }

		inline bool Prototype::operator!=(const ::FLAC__StreamMetadata *object) const 
		{ return !operator==(object); }

		inline bool Prototype::is_valid() const
		{ return 0 != object_; }

		/** Create a deep copy of an object and return it. */
		Prototype *clone(const Prototype *);


		/** STREAMINFO metadata block.
		 *  See <A HREF="../format.html#metadata_block_streaminfo">format specification</A>.
		 */
		class StreamInfo : public Prototype {
		public:
			StreamInfo();

			//@{
			/** Constructs a copy of the given object.  This form
			 *  always performs a deep copy.
			 */
			inline StreamInfo(const StreamInfo &object): Prototype(object) { }
			inline StreamInfo(const ::FLAC__StreamMetadata &object): Prototype(object) { }
			inline StreamInfo(const ::FLAC__StreamMetadata *object): Prototype(object) { }
			//@}

			/** Constructs an object with copy control.  See
			 *  Prototype(::FLAC__StreamMetadata *object, bool copy).
			 */
			inline StreamInfo(::FLAC__StreamMetadata *object, bool copy): Prototype(object, copy) { }

			~StreamInfo();

			//@{
			/** Assign from another object.  Always performs a deep copy. */
			inline void operator=(const StreamInfo &object) { Prototype::operator=(object); }
			inline void operator=(const ::FLAC__StreamMetadata &object) { Prototype::operator=(object); }
			inline void operator=(const ::FLAC__StreamMetadata *object) { Prototype::operator=(object); }
			//@}

			//@{
			/** Check for equality, performing a deep compare by following pointers. */
			inline bool operator==(const StreamInfo &object) const { return Prototype::operator==(object); }
			inline bool operator==(const ::FLAC__StreamMetadata &object) const { return Prototype::operator==(object); }
			inline bool operator==(const ::FLAC__StreamMetadata *object) const { return Prototype::operator==(object); }
			//@}

			//@{
			/** Check for inequality, performing a deep compare by following pointers. */
			inline bool operator!=(const StreamInfo &object) const { return Prototype::operator!=(object); }
			inline bool operator!=(const ::FLAC__StreamMetadata &object) const { return Prototype::operator!=(object); }
			inline bool operator!=(const ::FLAC__StreamMetadata *object) const { return Prototype::operator!=(object); }
			//@}

			//@{
			/** See <A HREF="../format.html#metadata_block_streaminfo">format specification</A>. */
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
			//@}
		};

		/** PADDING metadata block.
		 *  See <A HREF="../format.html#metadata_block_padding">format specification</A>.
		 */
		class Padding : public Prototype {
		public:
			Padding();

			//@{
			/** Constructs a copy of the given object.  This form
			 *  always performs a deep copy.
			 */
			inline Padding(const Padding &object): Prototype(object) { }
			inline Padding(const ::FLAC__StreamMetadata &object): Prototype(object) { }
			inline Padding(const ::FLAC__StreamMetadata *object): Prototype(object) { }
			//@}

			/** Constructs an object with copy control.  See
			 *  Prototype(::FLAC__StreamMetadata *object, bool copy).
			 */
			inline Padding(::FLAC__StreamMetadata *object, bool copy): Prototype(object, copy) { }

			~Padding();

			//@{
			/** Assign from another object.  Always performs a deep copy. */
			inline void operator=(const Padding &object) { Prototype::operator=(object); }
			inline void operator=(const ::FLAC__StreamMetadata &object) { Prototype::operator=(object); }
			inline void operator=(const ::FLAC__StreamMetadata *object) { Prototype::operator=(object); }
			//@}

			//@{
			/** Check for equality, performing a deep compare by following pointers. */
			inline bool operator==(const Padding &object) const { return Prototype::operator==(object); }
			inline bool operator==(const ::FLAC__StreamMetadata &object) const { return Prototype::operator==(object); }
			inline bool operator==(const ::FLAC__StreamMetadata *object) const { return Prototype::operator==(object); }
			//@}

			//@{
			/** Check for inequality, performing a deep compare by following pointers. */
			inline bool operator!=(const Padding &object) const { return Prototype::operator!=(object); }
			inline bool operator!=(const ::FLAC__StreamMetadata &object) const { return Prototype::operator!=(object); }
			inline bool operator!=(const ::FLAC__StreamMetadata *object) const { return Prototype::operator!=(object); }
			//@}

			void set_length(unsigned length);
		};

		/** APPLICATION metadata block.
		 *  See <A HREF="../format.html#metadata_block_application">format specification</A>.
		 */
		class Application : public Prototype {
		public:
			Application();
			//
			//@{
			/** Constructs a copy of the given object.  This form
			 *  always performs a deep copy.
			 */
			inline Application(const Application &object): Prototype(object) { }
			inline Application(const ::FLAC__StreamMetadata &object): Prototype(object) { }
			inline Application(const ::FLAC__StreamMetadata *object): Prototype(object) { }
			//@}

			/** Constructs an object with copy control.  See
			 *  Prototype(::FLAC__StreamMetadata *object, bool copy).
			 */
			inline Application(::FLAC__StreamMetadata *object, bool copy): Prototype(object, copy) { }

			~Application();

			//@{
			/** Assign from another object.  Always performs a deep copy. */
			inline void operator=(const Application &object) { Prototype::operator=(object); }
			inline void operator=(const ::FLAC__StreamMetadata &object) { Prototype::operator=(object); }
			inline void operator=(const ::FLAC__StreamMetadata *object) { Prototype::operator=(object); }
			//@}

			//@{
			/** Check for equality, performing a deep compare by following pointers. */
			inline bool operator==(const Application &object) const { return Prototype::operator==(object); }
			inline bool operator==(const ::FLAC__StreamMetadata &object) const { return Prototype::operator==(object); }
			inline bool operator==(const ::FLAC__StreamMetadata *object) const { return Prototype::operator==(object); }
			//@}

			//@{
			/** Check for inequality, performing a deep compare by following pointers. */
			inline bool operator!=(const Application &object) const { return Prototype::operator!=(object); }
			inline bool operator!=(const ::FLAC__StreamMetadata &object) const { return Prototype::operator!=(object); }
			inline bool operator!=(const ::FLAC__StreamMetadata *object) const { return Prototype::operator!=(object); }
			//@}

			const FLAC__byte *get_id() const;
			const FLAC__byte *get_data() const;

			void set_id(const FLAC__byte value[4]);
			//! This form always copies \a data
			bool set_data(const FLAC__byte *data, unsigned length);
			bool set_data(FLAC__byte *data, unsigned length, bool copy);
		};

		/** SEEKTABLE metadata block.
		 *  See <A HREF="../format.html#metadata_block_seektable">format specification</A>.
		 */
		class SeekTable : public Prototype {
		public:
			SeekTable();

			//@{
			/** Constructs a copy of the given object.  This form
			 *  always performs a deep copy.
			 */
			inline SeekTable(const SeekTable &object): Prototype(object) { }
			inline SeekTable(const ::FLAC__StreamMetadata &object): Prototype(object) { }
			inline SeekTable(const ::FLAC__StreamMetadata *object): Prototype(object) { }
			//@}

			/** Constructs an object with copy control.  See
			 *  Prototype(::FLAC__StreamMetadata *object, bool copy).
			 */
			inline SeekTable(::FLAC__StreamMetadata *object, bool copy): Prototype(object, copy) { }

			~SeekTable();

			//@{
			/** Assign from another object.  Always performs a deep copy. */
			inline void operator=(const SeekTable &object) { Prototype::operator=(object); }
			inline void operator=(const ::FLAC__StreamMetadata &object) { Prototype::operator=(object); }
			inline void operator=(const ::FLAC__StreamMetadata *object) { Prototype::operator=(object); }
			//@}

			//@{
			/** Check for equality, performing a deep compare by following pointers. */
			inline bool operator==(const SeekTable &object) const { return Prototype::operator==(object); }
			inline bool operator==(const ::FLAC__StreamMetadata &object) const { return Prototype::operator==(object); }
			inline bool operator==(const ::FLAC__StreamMetadata *object) const { return Prototype::operator==(object); }
			//@}

			//@{
			/** Check for inequality, performing a deep compare by following pointers. */
			inline bool operator!=(const SeekTable &object) const { return Prototype::operator!=(object); }
			inline bool operator!=(const ::FLAC__StreamMetadata &object) const { return Prototype::operator!=(object); }
			inline bool operator!=(const ::FLAC__StreamMetadata *object) const { return Prototype::operator!=(object); }
			//@}

			unsigned get_num_points() const;
			::FLAC__StreamMetadata_SeekPoint get_point(unsigned index) const;

			//! See FLAC__metadata_object_seektable_set_point()
			void set_point(unsigned index, const ::FLAC__StreamMetadata_SeekPoint &point);

			//! See FLAC__metadata_object_seektable_insert_point()
			bool insert_point(unsigned index, const ::FLAC__StreamMetadata_SeekPoint &point);

			//! See FLAC__metadata_object_seektable_delete_point()
			bool delete_point(unsigned index);

			//! See FLAC__metadata_object_seektable_is_legal()
			bool is_legal() const;
		};

		/** VORBIS_COMMENT metadata block.
		 *  See <A HREF="../format.html#metadata_block_vorbis_comment">format specification</A>.
		 */
		class VorbisComment : public Prototype {
		public:
			/** Convenience class for encapsulating Vorbis comment
			 *  entries.  An entry is a vendor string or a comment
			 *  field.  In the case of a vendor string, the field
			 *  name is undefined; only the field value is relevant.
			 *
			 *  A \a field as used in the methods refers to an
			 *  entire 'NAME=VALUE' string; the string is not null-
			 *  terminated and a length field is required since the
			 *  string may contain embedded nulls.
			 *
			 *  A \a field_name is what is on the left side of the
			 *  first '=' in the \a field.  By definition it is ASCII
			 *  and so is null-terminated and does not require a
			 *  length to describe it.  \a field_name is undefined
			 *  for a vendor string entry.
			 *
			 *  A \a field_value is what is on the right side of the
			 *  first '=' in the \a field.  By definition, this may
			 *  contain embedded nulls and so a \a field_value_length
			 *  is requires to describe it.
			 *
			 *  Always check is_valid() after the constructor or operator=
			 *  to make sure memory was properly allocated.
			 */
			class Entry {
			public:
				Entry();
				Entry(const char *field, unsigned field_length);
				Entry(const char *field_name, const char *field_value, unsigned field_value_length);
				Entry(const Entry &entry);
				void operator=(const Entry &entry);

				virtual ~Entry();

				virtual bool is_valid() const;

				unsigned get_field_length() const;
				unsigned get_field_name_length() const;
				unsigned get_field_value_length() const;

				::FLAC__StreamMetadata_VorbisComment_Entry get_entry() const;
				const char *get_field() const;
				const char *get_field_name() const;
				const char *get_field_value() const;

				bool set_field(const char *field, unsigned field_length);
				bool set_field_name(const char *field_name);
				bool set_field_value(const char *field_value, unsigned field_value_length);
			protected:
				bool is_valid_;
				::FLAC__StreamMetadata_VorbisComment_Entry entry_;
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

			//@{
			/** Constructs a copy of the given object.  This form
			 *  always performs a deep copy.
			 */
			inline VorbisComment(const VorbisComment &object): Prototype(object) { }
			inline VorbisComment(const ::FLAC__StreamMetadata &object): Prototype(object) { }
			inline VorbisComment(const ::FLAC__StreamMetadata *object): Prototype(object) { }
			//@}

			/** Constructs an object with copy control.  See
			 *  Prototype(::FLAC__StreamMetadata *object, bool copy).
			 */
			inline VorbisComment(::FLAC__StreamMetadata *object, bool copy): Prototype(object, copy) { }

			~VorbisComment();

			//@{
			/** Assign from another object.  Always performs a deep copy. */
			inline void operator=(const VorbisComment &object) { Prototype::operator=(object); }
			inline void operator=(const ::FLAC__StreamMetadata &object) { Prototype::operator=(object); }
			inline void operator=(const ::FLAC__StreamMetadata *object) { Prototype::operator=(object); }
			//@}

			//@{
			/** Check for equality, performing a deep compare by following pointers. */
			inline bool operator==(const VorbisComment &object) const { return Prototype::operator==(object); }
			inline bool operator==(const ::FLAC__StreamMetadata &object) const { return Prototype::operator==(object); }
			inline bool operator==(const ::FLAC__StreamMetadata *object) const { return Prototype::operator==(object); }
			//@}

			//@{
			/** Check for inequality, performing a deep compare by following pointers. */
			inline bool operator!=(const VorbisComment &object) const { return Prototype::operator!=(object); }
			inline bool operator!=(const ::FLAC__StreamMetadata &object) const { return Prototype::operator!=(object); }
			inline bool operator!=(const ::FLAC__StreamMetadata *object) const { return Prototype::operator!=(object); }
			//@}

			unsigned get_num_comments() const;
			Entry get_vendor_string() const; // only the Entry's field name should be used
			Entry get_comment(unsigned index) const;

			//! See FLAC__metadata_object_vorbiscomment_set_vendor_string()
			//! \note Only the Entry's field name will be used.
			bool set_vendor_string(const Entry &entry); // only the Entry's field name will be used

			//! See FLAC__metadata_object_vorbiscomment_set_comment()
			bool set_comment(unsigned index, const Entry &entry);

			//! See FLAC__metadata_object_vorbiscomment_insert_comment()
			bool insert_comment(unsigned index, const Entry &entry);

			//! See FLAC__metadata_object_vorbiscomment_delete_comment()
			bool delete_comment(unsigned index);
		};

		/* \} */


		/** \defgroup flacpp_metadata_level0 FLAC++/metadata.h: metadata level 0 interface
		 *  \ingroup flacpp_metadata
		 *
		 *  \brief
		 *  Level 0 metadata iterator.
		 *
		 *  See the \link flac_metadata_level0 C layer equivalent \endlink
		 *  for more.
		 *
		 * \{
		 */

	 	//! See FLAC__metadata_get_streaminfo().
		bool get_streaminfo(const char *filename, StreamInfo &streaminfo);

		/* \} */


		/** \defgroup flacpp_metadata_level1 FLAC++/metadata.h: metadata level 1 interface
		 *  \ingroup flacpp_metadata
		 *
		 *  \brief
		 *  Level 1 metadata iterator.
		 *
		 *  The flow through the iterator in the C++ layer is similar
		 *  to the C layer:
		 *    - Create a SimpleIterator instance
		 *    - Check SimpleIterator::is_valid()
		 *    - Call SimpleIterator::init() and check the return
		 *    - Traverse and/or edit.  Edits are written to file
		 *      immediately.
		 *    - Destroy the SimpleIterator instance
		 *
		 *  The ownership of pointers in the C++ layer follows that in
		 *  the C layer, i.e.
		 *    - The objects returned by get_block() are yours to
		 *      modify, but changes are not reflected in the FLAC file
		 *      until you call set_block().  The objects are also
		 *      yours to delete; they are not automatically deleted
		 *      when passed to set_block() or insert_block_after().
		 *
		 *  See the \link flac_metadata_level1 C layer equivalent \endlink
		 *  for more.
		 *
		 * \{
		 */

		/** XXX class SimpleIterator
		 */
		class SimpleIterator {
		public:
			/** XXX class SimpleIterator::Status
			 */
			class Status {
			public:
				inline Status(::FLAC__Metadata_SimpleIteratorStatus status): status_(status) { }
				inline operator ::FLAC__Metadata_SimpleIteratorStatus() const { return status_; }
				inline const char *as_cstring() const { return ::FLAC__Metadata_SimpleIteratorStatusString[status_]; }
			protected:
				::FLAC__Metadata_SimpleIteratorStatus status_;
			};

			SimpleIterator();
			virtual ~SimpleIterator();

			bool init(const char *filename, bool preserve_file_stats = false);

			bool is_valid() const;
			Status status();
			bool is_writable() const;

			bool next();
			bool prev();

			::FLAC__MetadataType get_block_type() const;
			Prototype *get_block();
			bool set_block(Prototype *block, bool use_padding = true);
			bool insert_block_after(Prototype *block, bool use_padding = true);
			bool delete_block(bool use_padding = true);

		protected:
			::FLAC__Metadata_SimpleIterator *iterator_;
			void clear();
		};

		/* \} */


		/** \defgroup flacpp_metadata_level2 FLAC++/metadata.h: metadata level 2 interface
		 *  \ingroup flacpp_metadata
		 *
		 *  \brief
		 *  Level 2 metadata iterator.
		 *
		 *  The flow through the iterator in the C++ layer is similar
		 *  to the C layer:
		 *    - Create a Chain instance
		 *    - Check Chain::is_valid()
		 *    - Call Chain::read() and check the return
		 *    - Traverse and/or edit with an Iterator or with
		 *      Chain::merge_padding() or Chain::sort_padding()
		 *    - Write changes back to FLAC file with Chain::write()
		 *    - Destroy the Chain instance
		 *
		 *  The ownership of pointers in the C++ layer follows that in
		 *  the C layer, i.e.
		 *    - The objects returned by Iterator::get_block() are
		 *      owned by the iterator and should not be deleted.
		 *      When you modify the block, you are directly editing
		 *      what's in the chain and do not need to call
		 *      Iterator::set_block().  However the changes will not
		 *      be reflected in the FLAC file until the chain is
		 *      written with Chain::write().
		 *    - When you pass an object to Iterator::set_block(),
		 *      Iterator::insert_block_before(), or
		 *      Iterator::insert_block_after(), the iterator takes
		 *      ownership of the block and it will be deleted with the
		 *      chain.
		 *
		 *  See the \link flac_metadata_level2 C layer equivalent \endlink
		 *  for more.
		 *
		 * \{
		 */

		/** XXX class Chain
		 */
		class Chain {
		public:
			/** XXX class Chain::Status
			 */
			class Status {
			public:
				inline Status(::FLAC__Metadata_ChainStatus status): status_(status) { }
				inline operator ::FLAC__Metadata_ChainStatus() const { return status_; }
				inline const char *as_cstring() const { return ::FLAC__Metadata_ChainStatusString[status_]; }
			protected:
				::FLAC__Metadata_ChainStatus status_;
			};

			Chain();
			virtual ~Chain();

			friend class Iterator;

			bool is_valid() const;
			Status status();

			bool read(const char *filename);
			bool write(bool use_padding = true, bool preserve_file_stats = false);

			void merge_padding();
			void sort_padding();

		protected:
			::FLAC__Metadata_Chain *chain_;
			virtual void clear();
		};

		/** XXX class Iterator
		 */
		class Iterator {
		public:
			Iterator();
			virtual ~Iterator();

			bool is_valid() const;

			void init(Chain &chain);

			bool next();
			bool prev();

			::FLAC__MetadataType get_block_type() const;
			Prototype *get_block();
			bool set_block(Prototype *block);
			bool delete_block(bool replace_with_padding);
			bool insert_block_before(Prototype *block);
			bool insert_block_after(Prototype *block);

		protected:
			::FLAC__Metadata_Iterator *iterator_;
			virtual void clear();
		};

		/* \} */

	};
};

#endif
