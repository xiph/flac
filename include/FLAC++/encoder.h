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

#ifndef FLACPP__ENCODER_H
#define FLACPP__ENCODER_H

#include "FLAC/file_encoder.h"
#include "FLAC/seekable_stream_encoder.h"
#include "FLAC/stream_encoder.h"
#include "decoder.h"

// ===============================================================
//
//  Full documentation for the encoder interfaces can be found
//  in the C layer in include/FLAC/ *_encoder.h
//
// ===============================================================


/** \file include/FLAC++/encoder.h
 *
 *  \brief
 *  This module contains the classes which implement the various
 *  encoders.
 *
 *  See the detailed documentation in the
 *  \link flacpp_encoder encoder \endlink module.
 */

/** \defgroup flacpp_encoder FLAC++/encoder.h: encoder classes
 *  \ingroup flacpp
 *
 *  \brief
 *  Brief XXX.
 *
 * Detailed encoder XXX.
 */

namespace FLAC {
	namespace Encoder {

		// ============================================================
		//
		//  Equivalent: FLAC__StreamEncoder
		//
		//  ----------------------------------------------------------
		//
		//  The only real difference here is that instead of passing
		//  in C function pointers for callbacks, you inherit from
		//  stream and provide implementations for the callbacks in
		//  the derived class; because of this there is no need for a
		//  'client_data' property.
		//
		// ============================================================

		/** \defgroup flacpp_stream_encoder FLAC++/encoder.h: stream encoder class
		 *  \ingroup flacpp_encoder
		 *
		 *  \brief
		 *  Brief XXX.
		 *
		 * Detailed stream encoder XXX.
		 * \{
		 */

		/** stream encoder XXX.
		 */
		class Stream {
		public:
			class State {
			public:
				inline State(::FLAC__StreamEncoderState state): state_(state) { }
				inline operator ::FLAC__StreamEncoderState() const { return state_; }
				inline const char *as_cstring() const { return ::FLAC__StreamEncoderStateString[state_]; }
			protected:
				::FLAC__StreamEncoderState state_;
			};

			Stream();
			virtual ~Stream();

			bool is_valid() const;
			inline operator bool() const { return is_valid(); }

			bool set_verify(bool value);
			bool set_streamable_subset(bool value);
			bool set_do_mid_side_stereo(bool value);
			bool set_loose_mid_side_stereo(bool value);
			bool set_channels(unsigned value);
			bool set_bits_per_sample(unsigned value);
			bool set_sample_rate(unsigned value);
			bool set_blocksize(unsigned value);
			bool set_max_lpc_order(unsigned value);
			bool set_qlp_coeff_precision(unsigned value);
			bool set_do_qlp_coeff_prec_search(bool value);
			bool set_do_escape_coding(bool value);
			bool set_do_exhaustive_model_search(bool value);
			bool set_min_residual_partition_order(unsigned value);
			bool set_max_residual_partition_order(unsigned value);
			bool set_rice_parameter_search_dist(unsigned value);
			bool set_total_samples_estimate(FLAC__uint64 value);
			bool set_metadata(::FLAC__StreamMetadata **metadata, unsigned num_blocks);

			State    get_state() const;
			Decoder::Stream::State get_verify_decoder_state() const;
			void get_verify_decoder_error_stats(FLAC__uint64 *absolute_sample, unsigned *frame_number, unsigned *channel, unsigned *sample, FLAC__int32 *expected, FLAC__int32 *got);
			bool     get_verify() const;
			bool     get_streamable_subset() const;
			bool     get_do_mid_side_stereo() const;
			bool     get_loose_mid_side_stereo() const;
			unsigned get_channels() const;
			unsigned get_bits_per_sample() const;
			unsigned get_sample_rate() const;
			unsigned get_blocksize() const;
			unsigned get_max_lpc_order() const;
			unsigned get_qlp_coeff_precision() const;
			bool     get_do_qlp_coeff_prec_search() const;
			bool     get_do_escape_coding() const;
			bool     get_do_exhaustive_model_search() const;
			unsigned get_min_residual_partition_order() const;
			unsigned get_max_residual_partition_order() const;
			unsigned get_rice_parameter_search_dist() const;
			FLAC__uint64 get_total_samples_estimate() const;

			// Initialize the instance; as with the C interface,
			// init() should be called after construction and 'set'
			// calls but before any of the 'process' calls.
			State init();

			void finish();

			bool process(const FLAC__int32 * const buffer[], unsigned samples);
			bool process_interleaved(const FLAC__int32 buffer[], unsigned samples);
		protected:
			virtual ::FLAC__StreamEncoderWriteStatus write_callback(const FLAC__byte buffer[], unsigned bytes, unsigned samples, unsigned current_frame) = 0;
			virtual void metadata_callback(const ::FLAC__StreamMetadata *metadata) = 0;

			::FLAC__StreamEncoder *encoder_;
		private:
			static ::FLAC__StreamEncoderWriteStatus write_callback_(const ::FLAC__StreamEncoder *encoder, const FLAC__byte buffer[], unsigned bytes, unsigned samples, unsigned current_frame, void *client_data);
			static void metadata_callback_(const ::FLAC__StreamEncoder *encoder, const ::FLAC__StreamMetadata *metadata, void *client_data);

			// Private and undefined so you can't use them:
			Stream(const Stream &);
			void operator=(const Stream &);
		};

		/* \} */

		/** \defgroup flacpp_seekable_stream_encoder FLAC++/encoder.h: seekable stream encoder class
		 *  \ingroup flacpp_encoder
		 *
		 *  \brief
		 *  Brief XXX.
		 *
		 * Detailed seekable stream encoder XXX.
		 * \{
		 */

		/** seekable stream encoder XXX.
		 */
		class SeekableStream {
		public:
			class State {
			public:
				inline State(::FLAC__SeekableStreamEncoderState state): state_(state) { }
				inline operator ::FLAC__SeekableStreamEncoderState() const { return state_; }
				inline const char *as_cstring() const { return ::FLAC__SeekableStreamEncoderStateString[state_]; }
			protected:
				::FLAC__SeekableStreamEncoderState state_;
			};

			SeekableStream();
			virtual ~SeekableStream();

			bool is_valid() const;
			inline operator bool() const { return is_valid(); }

			bool set_verify(bool value);
			bool set_streamable_subset(bool value);
			bool set_do_mid_side_stereo(bool value);
			bool set_loose_mid_side_stereo(bool value);
			bool set_channels(unsigned value);
			bool set_bits_per_sample(unsigned value);
			bool set_sample_rate(unsigned value);
			bool set_blocksize(unsigned value);
			bool set_max_lpc_order(unsigned value);
			bool set_qlp_coeff_precision(unsigned value);
			bool set_do_qlp_coeff_prec_search(bool value);
			bool set_do_escape_coding(bool value);
			bool set_do_exhaustive_model_search(bool value);
			bool set_min_residual_partition_order(unsigned value);
			bool set_max_residual_partition_order(unsigned value);
			bool set_rice_parameter_search_dist(unsigned value);
			bool set_total_samples_estimate(FLAC__uint64 value);
			bool set_metadata(::FLAC__StreamMetadata **metadata, unsigned num_blocks);

			State    get_state() const;
			Stream::State get_stream_encoder_state() const;
			Decoder::Stream::State get_verify_decoder_state() const;
			void get_verify_decoder_error_stats(FLAC__uint64 *absolute_sample, unsigned *frame_number, unsigned *channel, unsigned *sample, FLAC__int32 *expected, FLAC__int32 *got);
			bool     get_verify() const;
			bool     get_streamable_subset() const;
			bool     get_do_mid_side_stereo() const;
			bool     get_loose_mid_side_stereo() const;
			unsigned get_channels() const;
			unsigned get_bits_per_sample() const;
			unsigned get_sample_rate() const;
			unsigned get_blocksize() const;
			unsigned get_max_lpc_order() const;
			unsigned get_qlp_coeff_precision() const;
			bool     get_do_qlp_coeff_prec_search() const;
			bool     get_do_escape_coding() const;
			bool     get_do_exhaustive_model_search() const;
			unsigned get_min_residual_partition_order() const;
			unsigned get_max_residual_partition_order() const;
			unsigned get_rice_parameter_search_dist() const;
			FLAC__uint64 get_total_samples_estimate() const;

			// Initialize the instance; as with the C interface,
			// init() should be called after construction and 'set'
			// calls but before any of the 'process' calls.
			State init();

			void finish();

			bool process(const FLAC__int32 * const buffer[], unsigned samples);
			bool process_interleaved(const FLAC__int32 buffer[], unsigned samples);
		protected:
			virtual FLAC__SeekableStreamEncoderSeekStatus seek_callback(FLAC__uint64 absolute_byte_offset) = 0;
			virtual FLAC__StreamEncoderWriteStatus write_callback(const FLAC__byte buffer[], unsigned bytes, unsigned samples, unsigned current_frame) = 0;

			::FLAC__SeekableStreamEncoder *encoder_;
		private:
			static FLAC__SeekableStreamEncoderSeekStatus seek_callback_(const FLAC__SeekableStreamEncoder *encoder, FLAC__uint64 absolute_byte_offset, void *client_data);
			static FLAC__StreamEncoderWriteStatus write_callback_(const FLAC__SeekableStreamEncoder *encoder, const FLAC__byte buffer[], unsigned bytes, unsigned samples, unsigned current_frame, void *client_data);

			// Private and undefined so you can't use them:
			SeekableStream(const SeekableStream &);
			void operator=(const SeekableStream &);
		};

		/* \} */

		/** \defgroup flacpp_file_encoder FLAC++/encoder.h: file encoder class
		 *  \ingroup flacpp_encoder
		 *
		 *  \brief
		 *  Brief XXX.
		 *
		 * Detailed file encoder XXX.
		 * \{
		 */

		/** file encoder XXX.
		 */
		class File {
		public:
			class State {
			public:
				inline State(::FLAC__FileEncoderState state): state_(state) { }
				inline operator ::FLAC__FileEncoderState() const { return state_; }
				inline const char *as_cstring() const { return ::FLAC__FileEncoderStateString[state_]; }
			protected:
				::FLAC__FileEncoderState state_;
			};

			File();
			virtual ~File();

			bool is_valid() const;
			inline operator bool() const { return is_valid(); }

			bool set_verify(bool value);
			bool set_streamable_subset(bool value);
			bool set_do_mid_side_stereo(bool value);
			bool set_loose_mid_side_stereo(bool value);
			bool set_channels(unsigned value);
			bool set_bits_per_sample(unsigned value);
			bool set_sample_rate(unsigned value);
			bool set_blocksize(unsigned value);
			bool set_max_lpc_order(unsigned value);
			bool set_qlp_coeff_precision(unsigned value);
			bool set_do_qlp_coeff_prec_search(bool value);
			bool set_do_escape_coding(bool value);
			bool set_do_exhaustive_model_search(bool value);
			bool set_min_residual_partition_order(unsigned value);
			bool set_max_residual_partition_order(unsigned value);
			bool set_rice_parameter_search_dist(unsigned value);
			bool set_total_samples_estimate(FLAC__uint64 value);
			bool set_metadata(::FLAC__StreamMetadata **metadata, unsigned num_blocks);
			bool set_filename(const char *value);

			State    get_state() const;
			SeekableStream::State get_seekable_stream_encoder_state() const;
			Stream::State get_stream_encoder_state() const;
			Decoder::Stream::State get_verify_decoder_state() const;
			void get_verify_decoder_error_stats(FLAC__uint64 *absolute_sample, unsigned *frame_number, unsigned *channel, unsigned *sample, FLAC__int32 *expected, FLAC__int32 *got);
			bool     get_verify() const;
			bool     get_streamable_subset() const;
			bool     get_do_mid_side_stereo() const;
			bool     get_loose_mid_side_stereo() const;
			unsigned get_channels() const;
			unsigned get_bits_per_sample() const;
			unsigned get_sample_rate() const;
			unsigned get_blocksize() const;
			unsigned get_max_lpc_order() const;
			unsigned get_qlp_coeff_precision() const;
			bool     get_do_qlp_coeff_prec_search() const;
			bool     get_do_escape_coding() const;
			bool     get_do_exhaustive_model_search() const;
			unsigned get_min_residual_partition_order() const;
			unsigned get_max_residual_partition_order() const;
			unsigned get_rice_parameter_search_dist() const;
			FLAC__uint64 get_total_samples_estimate() const;

			// Initialize the instance; as with the C interface,
			// init() should be called after construction and 'set'
			// calls but before any of the 'process' calls.
			State init();

			void finish();

			bool process(const FLAC__int32 * const buffer[], unsigned samples);
			bool process_interleaved(const FLAC__int32 buffer[], unsigned samples);
		protected:
			virtual void progress_callback(FLAC__uint64 bytes_written, FLAC__uint64 samples_written, unsigned frames_written, unsigned total_frames_estimate);

			::FLAC__FileEncoder *encoder_;
		private:
			static void progress_callback_(const ::FLAC__FileEncoder *encoder, FLAC__uint64 bytes_written, FLAC__uint64 samples_written, unsigned frames_written, unsigned total_frames_estimate, void *client_data);

			// Private and undefined so you can't use them:
			File(const Stream &);
			void operator=(const Stream &);
		};

		/* \} */

	};
};

#endif
