/* libOggFLAC++ - Free Lossless Audio Codec + Ogg library
 * Copyright (C) 2002,2003,2004,2005  Josh Coalson
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

#ifndef OggFLACPP__ENCODER_H
#define OggFLACPP__ENCODER_H

#include "export.h"

#include "OggFLAC/file_encoder.h"
#include "OggFLAC/seekable_stream_encoder.h"
#include "OggFLAC/stream_encoder.h"
#include "decoder.h"
// we only need these for the state abstractions really...
#include "FLAC++/decoder.h"
#include "FLAC++/encoder.h"


/** \file include/OggFLAC++/encoder.h
 *
 *  \brief
 *  This module contains the classes which implement the various
 *  encoders.
 *
 *  See the detailed documentation in the
 *  \link oggflacpp_encoder encoder \endlink module.
 */

/** \defgroup oggflacpp_encoder OggFLAC++/encoder.h: encoder classes
 *  \ingroup oggflacpp
 *
 *  \brief
 *  This module describes the three encoder layers provided by libOggFLAC++.
 *
 * The libOggFLAC++ encoder classes are object wrappers around their
 * counterparts in libOggFLAC.  All three encoding layers available in
 * libOggFLAC are also provided here.  The interface is very similar;
 * make sure to read the \link oggflac_encoder libOggFLAC encoder module \endlink.
 *
 * The only real difference here is that instead of passing in C function
 * pointers for callbacks, you inherit from the encoder class and provide
 * implementations for the callbacks in the derived class; because of this
 * there is no need for a 'client_data' property.
 */

namespace OggFLAC {
	namespace Encoder {

		// ============================================================
		//
		//  Equivalent: OggFLAC__StreamEncoder
		//
		// ============================================================

		/** \defgroup oggflacpp_stream_encoder OggFLAC++/encoder.h: stream encoder class
		 *  \ingroup oggflacpp_encoder
		 *
		 *  \brief
		 *  This class wraps the ::OggFLAC__StreamEncoder.
		 *
		 * See the \link oggflac_stream_encoder libOggFLAC stream encoder module \endlink.
		 *
		 * \{
		 */

		/** This class wraps the ::OggFLAC__StreamEncoder.
		 */
		class OggFLACPP_API Stream {
		public:
			class OggFLACPP_API State {
			public:
				inline State(::OggFLAC__StreamEncoderState state): state_(state) { }
				inline operator ::OggFLAC__StreamEncoderState() const { return state_; }
				inline const char *as_cstring() const { return ::OggFLAC__StreamEncoderStateString[state_]; }
				inline const char *resolved_as_cstring(const Stream &encoder) const { return ::OggFLAC__stream_encoder_get_resolved_state_string(encoder.encoder_); }
			protected:
				::OggFLAC__StreamEncoderState state_;
			};

			Stream();
			virtual ~Stream();

			bool is_valid() const;
			inline operator bool() const { return is_valid(); }

			bool set_serial_number(long value);
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
			FLAC::Encoder::Stream::State get_FLAC_stream_encoder_state() const;
			FLAC::Decoder::Stream::State get_verify_decoder_state() const;
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

			State init();

			void finish();

			bool process(const FLAC__int32 * const buffer[], unsigned samples);
			bool process_interleaved(const FLAC__int32 buffer[], unsigned samples);
		protected:
			virtual ::FLAC__StreamEncoderWriteStatus write_callback(const FLAC__byte buffer[], unsigned bytes, unsigned samples, unsigned current_frame) = 0;
			virtual void metadata_callback(const ::FLAC__StreamMetadata *metadata) = 0;

#if (defined _MSC_VER) || (defined __GNUG__ && (__GNUG__ < 2 || (__GNUG__ == 2 && __GNUC_MINOR__ < 96))) || (defined __SUNPRO_CC)
			// lame hack: some MSVC/GCC versions can't see a protected encoder_ from nested State::resolved_as_cstring()
			friend State;
#endif
			::OggFLAC__StreamEncoder *encoder_;
		private:
			static ::FLAC__StreamEncoderWriteStatus write_callback_(const ::OggFLAC__StreamEncoder *encoder, const FLAC__byte buffer[], unsigned bytes, unsigned samples, unsigned current_frame, void *client_data);
			static void metadata_callback_(const ::OggFLAC__StreamEncoder *encoder, const ::FLAC__StreamMetadata *metadata, void *client_data);

			// Private and undefined so you can't use them:
			Stream(const Stream &);
			void operator=(const Stream &);
		};

		/* \} */

		/** \defgroup oggflacpp_seekable_stream_encoder OggFLAC++/encoder.h: seekable stream encoder class
		 *  \ingroup oggflacpp_encoder
		 *
		 *  \brief
		 *  This class wraps the ::OggFLAC__SeekableStreamEncoder.
		 *
		 * See the \link oggflac_seekable_stream_encoder libOggFLAC seekable stream encoder module \endlink.
		 *
		 * \{
		 */

		/** This class wraps the ::OggFLAC__SeekableStreamEncoder.
		 */
		class OggFLACPP_API SeekableStream {
		public:
			class OggFLACPP_API State {
			public:
				inline State(::OggFLAC__SeekableStreamEncoderState state): state_(state) { }
				inline operator ::OggFLAC__SeekableStreamEncoderState() const { return state_; }
				inline const char *as_cstring() const { return ::OggFLAC__SeekableStreamEncoderStateString[state_]; }
				inline const char *resolved_as_cstring(const SeekableStream &encoder) const { return ::OggFLAC__seekable_stream_encoder_get_resolved_state_string(encoder.encoder_); }
			protected:
				::OggFLAC__SeekableStreamEncoderState state_;
			};

			SeekableStream();
			virtual ~SeekableStream();

			bool is_valid() const;
			inline operator bool() const { return is_valid(); }

			bool set_serial_number(long value);
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
			FLAC::Encoder::Stream::State get_FLAC_stream_encoder_state() const;
			FLAC::Decoder::Stream::State get_verify_decoder_state() const;
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

			State init();

			void finish();

			bool process(const FLAC__int32 * const buffer[], unsigned samples);
			bool process_interleaved(const FLAC__int32 buffer[], unsigned samples);
		protected:
			virtual ::OggFLAC__SeekableStreamEncoderReadStatus read_callback(FLAC__byte buffer[], unsigned *bytes) = 0;
			virtual ::FLAC__SeekableStreamEncoderSeekStatus seek_callback(FLAC__uint64 absolute_byte_offset) = 0;
			virtual ::FLAC__SeekableStreamEncoderTellStatus tell_callback(FLAC__uint64 *absolute_byte_offset) = 0;
			virtual ::FLAC__StreamEncoderWriteStatus write_callback(const FLAC__byte buffer[], unsigned bytes, unsigned samples, unsigned current_frame) = 0;

#if (defined _MSC_VER) || (defined __GNUG__ && (__GNUG__ < 2 || (__GNUG__ == 2 && __GNUC_MINOR__ < 96))) || (defined __SUNPRO_CC)
			// lame hack: some MSVC/GCC versions can't see a protected encoder_ from nested State::resolved_as_cstring()
			friend State;
#endif
			::OggFLAC__SeekableStreamEncoder *encoder_;
		private:
			static ::OggFLAC__SeekableStreamEncoderReadStatus read_callback_(const OggFLAC__SeekableStreamEncoder *encoder, FLAC__byte buffer[], unsigned *bytes, void *client_data);
			static ::FLAC__SeekableStreamEncoderSeekStatus seek_callback_(const OggFLAC__SeekableStreamEncoder *encoder, FLAC__uint64 absolute_byte_offset, void *client_data);
			static ::FLAC__SeekableStreamEncoderTellStatus tell_callback_(const OggFLAC__SeekableStreamEncoder *encoder, FLAC__uint64 *absolute_byte_offset, void *client_data);
			static ::FLAC__StreamEncoderWriteStatus write_callback_(const OggFLAC__SeekableStreamEncoder *encoder, const FLAC__byte buffer[], unsigned bytes, unsigned samples, unsigned current_frame, void *client_data);

			// Private and undefined so you can't use them:
			SeekableStream(const SeekableStream &);
			void operator=(const SeekableStream &);
		};

		/* \} */

		/** \defgroup oggflacpp_file_encoder OggFLAC++/encoder.h: file encoder class
		 *  \ingroup oggflacpp_encoder
		 *
		 *  \brief
		 *  This class wraps the ::OggFLAC__FileEncoder.
		 *
		 * See the \link oggflac_file_encoder libOggFLAC file encoder module \endlink.
		 *
		 * \{
		 */

		/** This class wraps the ::OggFLAC__FileEncoder.
		 */
		class OggFLACPP_API File {
		public:
			class OggFLACPP_API State {
			public:
				inline State(::OggFLAC__FileEncoderState state): state_(state) { }
				inline operator ::OggFLAC__FileEncoderState() const { return state_; }
				inline const char *as_cstring() const { return ::OggFLAC__FileEncoderStateString[state_]; }
				inline const char *resolved_as_cstring(const File &encoder) const { return ::OggFLAC__file_encoder_get_resolved_state_string(encoder.encoder_); }
			protected:
				::OggFLAC__FileEncoderState state_;
			};

			File();
			virtual ~File();

			bool is_valid() const;
			inline operator bool() const { return is_valid(); }

			bool set_serial_number(long value);
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
			FLAC::Encoder::Stream::State get_FLAC_stream_encoder_state() const;
			FLAC::Decoder::Stream::State get_verify_decoder_state() const;
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

			State init();

			void finish();

			bool process(const FLAC__int32 * const buffer[], unsigned samples);
			bool process_interleaved(const FLAC__int32 buffer[], unsigned samples);
		protected:
			virtual void progress_callback(FLAC__uint64 bytes_written, FLAC__uint64 samples_written, unsigned frames_written, unsigned total_frames_estimate);

#if (defined _MSC_VER) || (defined __GNUG__ && (__GNUG__ < 2 || (__GNUG__ == 2 && __GNUC_MINOR__ < 96))) || (defined __SUNPRO_CC)
			// lame hack: some MSVC/GCC versions can't see a protected encoder_ from nested State::resolved_as_cstring()
			friend State;
#endif
			::OggFLAC__FileEncoder *encoder_;
		private:
			static void progress_callback_(const ::OggFLAC__FileEncoder *encoder, FLAC__uint64 bytes_written, FLAC__uint64 samples_written, unsigned frames_written, unsigned total_frames_estimate, void *client_data);

			// Private and undefined so you can't use them:
			File(const Stream &);
			void operator=(const Stream &);
		};

		/* \} */

	}
}

#endif
