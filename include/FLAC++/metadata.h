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

namespace FLAC {
	namespace Metadata {

		// NOTE: When the get_*() methods return you a const pointer,
		// absolutely DO NOT write into it.  Always use the set_*()
		// methods.

		// base class for all metadata blocks
		class Prototype {
		protected:
			Prototype(::FLAC__StreamMetaData *object, bool copy);
			virtual ~Prototype();

			::FLAC__StreamMetaData *object_;
		public:
			inline bool is_valid() const { return 0 != object_; }
			inline operator bool() const { return is_valid(); }

			bool get_is_last() const;
			FLAC__MetaDataType get_type() const;
			unsigned get_length() const; // NOTE: does not include the header, per spec
		};

		class StreamInfo : public Prototype {
		public:
			StreamInfo();
			StreamInfo(::FLAC__StreamMetaData *object, bool copy = false);
			~StreamInfo();

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
		};

		class Application : public Prototype {
		public:
			Application();
			Application(::FLAC__StreamMetaData *object, bool copy = false);
			~Application();

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
		};

		class VorbisComment : public Prototype {
		public:
			VorbisComment();
			VorbisComment(::FLAC__StreamMetaData *object, bool copy = false);
			~VorbisComment();
		};

	};
};

#endif
