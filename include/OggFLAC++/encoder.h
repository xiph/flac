/* libOggFLAC++ - Free Lossless Audio Codec + Ogg library
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

#ifndef OggFLACPP__ENCODER_H
#define OggFLACPP__ENCODER_H

#include "OggFLAC/stream_encoder.h"
#include "decoder.h"
// we only need these for the state abstractions really...
#include "FLAC++/decoder.h"
#include "FLAC++/encoder.h"

// ===============================================================
//
//  Full documentation for the encoder interfaces can be found
//  in the C layer in include/OggFLAC/ *_encoder.h
//
// ===============================================================


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
 *  Brief XXX.
 *
 * Detailed encoder XXX.
 */

namespace OggFLAC {
	namespace Encoder {

		// ============================================================
		//
		//  Equivalent: OggFLAC__StreamEncoder
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

		/** \defgroup oggflacpp_stream_encoder OggFLAC++/encoder.h: stream encoder class
		 *  \ingroup oggflacpp_encoder
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
				inline State(::OggFLAC__StreamEncoderState state): state_(state) { }
				inline operator ::OggFLAC__StreamEncoderState() const { return state_; }
				inline const char *as_cstring() const { return ::OggFLAC__StreamEncoderStateString[state_]; }
			protected:
				::OggFLAC__StreamEncoderState state_;
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

			// Initialize the instance; as with the C interface,
			// init() should be called after construction and 'set'
			// calls but before any of the 'process' calls.
			State init();

			void finish();

			bool process(const FLAC__int32 * const buffer[], unsigned samples);
			bool process_interleaved(const FLAC__int32 buffer[], unsigned samples);
		protected:
			virtual ::FLAC__StreamEncoderWriteStatus write_callback(const FLAC__byte buffer[], unsigned bytes, unsigned samples, unsigned current_frame) = 0;

			::OggFLAC__StreamEncoder *encoder_;
		private:
			static ::FLAC__StreamEncoderWriteStatus write_callback_(const ::OggFLAC__StreamEncoder *encoder, const FLAC__byte buffer[], unsigned bytes, unsigned samples, unsigned current_frame, void *client_data);

			// Private and undefined so you can't use them:
			Stream(const Stream &);
			void operator=(const Stream &);
		};

		/* \} */

	};
};

#endif
