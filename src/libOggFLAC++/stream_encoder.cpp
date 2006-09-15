/* libOggFLAC++ - Free Lossless Audio Codec + Ogg library
 * Copyright (C) 2002,2003,2004,2005,2006  Josh Coalson
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

#include "OggFLAC++/encoder.h"
#include "FLAC/assert.h"

#ifdef _MSC_VER
// warning C4800: 'int' : forcing to bool 'true' or 'false' (performance warning)
#pragma warning ( disable : 4800 )
#endif

namespace OggFLAC {
	namespace Encoder {

		// We can inherit from FLAC::Encoder::Stream because we jam a
		// OggFLAC__StreamEncoder pointer into the encoder_ member,
		// hence the pointer casting on encoder_ everywhere.

		Stream::Stream():
		FLAC::Encoder::Stream((FLAC__StreamEncoder*)::OggFLAC__stream_encoder_new())
		{ }

		Stream::~Stream()
		{
			if(0 != encoder_) {
				::OggFLAC__stream_encoder_finish((OggFLAC__StreamEncoder*)encoder_);
				::OggFLAC__stream_encoder_delete((OggFLAC__StreamEncoder*)encoder_);
				// this is our signal to FLAC::Encoder::Stream::~Stream()
				// that we already deleted the encoder our way, so it
				// doesn't need to:
				encoder_ = 0;
			}
		}

		bool Stream::set_serial_number(long value)
		{
			FLAC__ASSERT(is_valid());
			return (bool)::OggFLAC__stream_encoder_set_serial_number((OggFLAC__StreamEncoder*)encoder_, value);
		}

		bool Stream::set_verify(bool value)
		{
			FLAC__ASSERT(is_valid());
			return (bool)::OggFLAC__stream_encoder_set_verify((OggFLAC__StreamEncoder*)encoder_, value);
		}

		bool Stream::set_streamable_subset(bool value)
		{
			FLAC__ASSERT(is_valid());
			return (bool)::OggFLAC__stream_encoder_set_streamable_subset((OggFLAC__StreamEncoder*)encoder_, value);
		}

		bool Stream::set_do_mid_side_stereo(bool value)
		{
			FLAC__ASSERT(is_valid());
			return (bool)::OggFLAC__stream_encoder_set_do_mid_side_stereo((OggFLAC__StreamEncoder*)encoder_, value);
		}

		bool Stream::set_loose_mid_side_stereo(bool value)
		{
			FLAC__ASSERT(is_valid());
			return (bool)::OggFLAC__stream_encoder_set_loose_mid_side_stereo((OggFLAC__StreamEncoder*)encoder_, value);
		}

		bool Stream::set_channels(unsigned value)
		{
			FLAC__ASSERT(is_valid());
			return (bool)::OggFLAC__stream_encoder_set_channels((OggFLAC__StreamEncoder*)encoder_, value);
		}

		bool Stream::set_bits_per_sample(unsigned value)
		{
			FLAC__ASSERT(is_valid());
			return (bool)::OggFLAC__stream_encoder_set_bits_per_sample((OggFLAC__StreamEncoder*)encoder_, value);
		}

		bool Stream::set_sample_rate(unsigned value)
		{
			FLAC__ASSERT(is_valid());
			return (bool)::OggFLAC__stream_encoder_set_sample_rate((OggFLAC__StreamEncoder*)encoder_, value);
		}

		bool Stream::set_blocksize(unsigned value)
		{
			FLAC__ASSERT(is_valid());
			return (bool)::OggFLAC__stream_encoder_set_blocksize((OggFLAC__StreamEncoder*)encoder_, value);
		}

		bool Stream::set_apodization(const char *specification)
		{
			FLAC__ASSERT(is_valid());
			return (bool)::OggFLAC__stream_encoder_set_apodization((OggFLAC__StreamEncoder*)encoder_, specification);
		}

		bool Stream::set_max_lpc_order(unsigned value)
		{
			FLAC__ASSERT(is_valid());
			return (bool)::OggFLAC__stream_encoder_set_max_lpc_order((OggFLAC__StreamEncoder*)encoder_, value);
		}

		bool Stream::set_qlp_coeff_precision(unsigned value)
		{
			FLAC__ASSERT(is_valid());
			return (bool)::OggFLAC__stream_encoder_set_qlp_coeff_precision((OggFLAC__StreamEncoder*)encoder_, value);
		}

		bool Stream::set_do_qlp_coeff_prec_search(bool value)
		{
			FLAC__ASSERT(is_valid());
			return (bool)::OggFLAC__stream_encoder_set_do_qlp_coeff_prec_search((OggFLAC__StreamEncoder*)encoder_, value);
		}

		bool Stream::set_do_escape_coding(bool value)
		{
			FLAC__ASSERT(is_valid());
			return (bool)::OggFLAC__stream_encoder_set_do_escape_coding((OggFLAC__StreamEncoder*)encoder_, value);
		}

		bool Stream::set_do_exhaustive_model_search(bool value)
		{
			FLAC__ASSERT(is_valid());
			return (bool)::OggFLAC__stream_encoder_set_do_exhaustive_model_search((OggFLAC__StreamEncoder*)encoder_, value);
		}

		bool Stream::set_min_residual_partition_order(unsigned value)
		{
			FLAC__ASSERT(is_valid());
			return (bool)::OggFLAC__stream_encoder_set_min_residual_partition_order((OggFLAC__StreamEncoder*)encoder_, value);
		}

		bool Stream::set_max_residual_partition_order(unsigned value)
		{
			FLAC__ASSERT(is_valid());
			return (bool)::OggFLAC__stream_encoder_set_max_residual_partition_order((OggFLAC__StreamEncoder*)encoder_, value);
		}

		bool Stream::set_rice_parameter_search_dist(unsigned value)
		{
			FLAC__ASSERT(is_valid());
			return (bool)::OggFLAC__stream_encoder_set_rice_parameter_search_dist((OggFLAC__StreamEncoder*)encoder_, value);
		}

		bool Stream::set_total_samples_estimate(FLAC__uint64 value)
		{
			FLAC__ASSERT(is_valid());
			return (bool)::OggFLAC__stream_encoder_set_total_samples_estimate((OggFLAC__StreamEncoder*)encoder_, value);
		}

		bool Stream::set_metadata(::FLAC__StreamMetadata **metadata, unsigned num_blocks)
		{
			FLAC__ASSERT(is_valid());
			return (bool)::OggFLAC__stream_encoder_set_metadata((OggFLAC__StreamEncoder*)encoder_, metadata, num_blocks);
		}

		bool Stream::set_metadata(FLAC::Metadata::Prototype **metadata, unsigned num_blocks)
		{
			FLAC__ASSERT(is_valid());
#if (defined _MSC_VER) || (defined __SUNPRO_CC)
			// MSVC++ can't handle:
			// ::FLAC__StreamMetadata *m[num_blocks];
			// so we do this ugly workaround
			::FLAC__StreamMetadata **m = new ::FLAC__StreamMetadata*[num_blocks];
#else
			::FLAC__StreamMetadata *m[num_blocks];
#endif
			for(unsigned i = 0; i < num_blocks; i++) {
				// we can get away with the const_cast since we know the encoder will only correct the is_last flags
				m[i] = const_cast< ::FLAC__StreamMetadata*>((const ::FLAC__StreamMetadata*)metadata[i]);
			}
#if (defined _MSC_VER) || (defined __SUNPRO_CC)
			// complete the hack
			const bool ok = (bool)::OggFLAC__stream_encoder_set_metadata((OggFLAC__StreamEncoder*)encoder_, m, num_blocks);
			delete [] m;
			return ok;
#else
			return (bool)::OggFLAC__stream_encoder_set_metadata((OggFLAC__StreamEncoder*)encoder_, m, num_blocks);
#endif
		}

		Stream::State Stream::get_state() const
		{
			FLAC__ASSERT(is_valid());
			return State(::OggFLAC__stream_encoder_get_state((const OggFLAC__StreamEncoder*)encoder_));
		}

		FLAC::Encoder::Stream::State Stream::get_FLAC_stream_encoder_state() const
		{
			FLAC__ASSERT(is_valid());
			return FLAC::Encoder::Stream::State(::OggFLAC__stream_encoder_get_FLAC_stream_encoder_state((const OggFLAC__StreamEncoder*)encoder_));
		}

		FLAC::Decoder::Stream::State Stream::get_verify_decoder_state() const
		{
			FLAC__ASSERT(is_valid());
			return FLAC::Decoder::Stream::State(::OggFLAC__stream_encoder_get_verify_decoder_state((const OggFLAC__StreamEncoder*)encoder_));
		}

		void Stream::get_verify_decoder_error_stats(FLAC__uint64 *absolute_sample, unsigned *frame_number, unsigned *channel, unsigned *sample, FLAC__int32 *expected, FLAC__int32 *got)
		{
			FLAC__ASSERT(is_valid());
			::OggFLAC__stream_encoder_get_verify_decoder_error_stats((const OggFLAC__StreamEncoder*)encoder_, absolute_sample, frame_number, channel, sample, expected, got);
		}

		bool Stream::get_verify() const
		{
			FLAC__ASSERT(is_valid());
			return (bool)::OggFLAC__stream_encoder_get_verify((const OggFLAC__StreamEncoder*)encoder_);
		}

		bool Stream::get_streamable_subset() const
		{
			FLAC__ASSERT(is_valid());
			return (bool)::OggFLAC__stream_encoder_get_streamable_subset((const OggFLAC__StreamEncoder*)encoder_);
		}

		bool Stream::get_do_mid_side_stereo() const
		{
			FLAC__ASSERT(is_valid());
			return (bool)::OggFLAC__stream_encoder_get_do_mid_side_stereo((const OggFLAC__StreamEncoder*)encoder_);
		}

		bool Stream::get_loose_mid_side_stereo() const
		{
			FLAC__ASSERT(is_valid());
			return (bool)::OggFLAC__stream_encoder_get_loose_mid_side_stereo((const OggFLAC__StreamEncoder*)encoder_);
		}

		unsigned Stream::get_channels() const
		{
			FLAC__ASSERT(is_valid());
			return ::OggFLAC__stream_encoder_get_channels((const OggFLAC__StreamEncoder*)encoder_);
		}

		unsigned Stream::get_bits_per_sample() const
		{
			FLAC__ASSERT(is_valid());
			return ::OggFLAC__stream_encoder_get_bits_per_sample((const OggFLAC__StreamEncoder*)encoder_);
		}

		unsigned Stream::get_sample_rate() const
		{
			FLAC__ASSERT(is_valid());
			return ::OggFLAC__stream_encoder_get_sample_rate((const OggFLAC__StreamEncoder*)encoder_);
		}

		unsigned Stream::get_blocksize() const
		{
			FLAC__ASSERT(is_valid());
			return ::OggFLAC__stream_encoder_get_blocksize((const OggFLAC__StreamEncoder*)encoder_);
		}

		unsigned Stream::get_max_lpc_order() const
		{
			FLAC__ASSERT(is_valid());
			return ::OggFLAC__stream_encoder_get_max_lpc_order((const OggFLAC__StreamEncoder*)encoder_);
		}

		unsigned Stream::get_qlp_coeff_precision() const
		{
			FLAC__ASSERT(is_valid());
			return ::OggFLAC__stream_encoder_get_qlp_coeff_precision((const OggFLAC__StreamEncoder*)encoder_);
		}

		bool Stream::get_do_qlp_coeff_prec_search() const
		{
			FLAC__ASSERT(is_valid());
			return (bool)::OggFLAC__stream_encoder_get_do_qlp_coeff_prec_search((const OggFLAC__StreamEncoder*)encoder_);
		}

		bool Stream::get_do_escape_coding() const
		{
			FLAC__ASSERT(is_valid());
			return (bool)::OggFLAC__stream_encoder_get_do_escape_coding((const OggFLAC__StreamEncoder*)encoder_);
		}

		bool Stream::get_do_exhaustive_model_search() const
		{
			FLAC__ASSERT(is_valid());
			return (bool)::OggFLAC__stream_encoder_get_do_exhaustive_model_search((const OggFLAC__StreamEncoder*)encoder_);
		}

		unsigned Stream::get_min_residual_partition_order() const
		{
			FLAC__ASSERT(is_valid());
			return ::OggFLAC__stream_encoder_get_min_residual_partition_order((const OggFLAC__StreamEncoder*)encoder_);
		}

		unsigned Stream::get_max_residual_partition_order() const
		{
			FLAC__ASSERT(is_valid());
			return ::OggFLAC__stream_encoder_get_max_residual_partition_order((const OggFLAC__StreamEncoder*)encoder_);
		}

		unsigned Stream::get_rice_parameter_search_dist() const
		{
			FLAC__ASSERT(is_valid());
			return ::OggFLAC__stream_encoder_get_rice_parameter_search_dist((const OggFLAC__StreamEncoder*)encoder_);
		}

		FLAC__uint64 Stream::get_total_samples_estimate() const
		{
			FLAC__ASSERT(is_valid());
			return ::OggFLAC__stream_encoder_get_total_samples_estimate((const OggFLAC__StreamEncoder*)encoder_);
		}

		::FLAC__StreamEncoderInitStatus Stream::init()
		{
			FLAC__ASSERT(is_valid());
			return ::OggFLAC__stream_encoder_init_stream((OggFLAC__StreamEncoder*)encoder_, read_callback_, write_callback_, seek_callback_, tell_callback_, metadata_callback_, /*client_data=*/(void*)this);
		}

		void Stream::finish()
		{
			FLAC__ASSERT(is_valid());
			::OggFLAC__stream_encoder_finish((OggFLAC__StreamEncoder*)encoder_);
		}

		bool Stream::process(const FLAC__int32 * const buffer[], unsigned samples)
		{
			FLAC__ASSERT(is_valid());
			return (bool)::OggFLAC__stream_encoder_process((OggFLAC__StreamEncoder*)encoder_, buffer, samples);
		}

		bool Stream::process_interleaved(const FLAC__int32 buffer[], unsigned samples)
		{
			FLAC__ASSERT(is_valid());
			return (bool)::OggFLAC__stream_encoder_process_interleaved((OggFLAC__StreamEncoder*)encoder_, buffer, samples);
		}

		::OggFLAC__StreamEncoderReadStatus Stream::read_callback(FLAC__byte buffer[], unsigned *bytes)
		{
			(void)buffer, (void)bytes;
			return ::OggFLAC__STREAM_ENCODER_READ_STATUS_UNSUPPORTED;
		}

		::OggFLAC__StreamEncoderReadStatus Stream::read_callback_(const ::OggFLAC__StreamEncoder *encoder, FLAC__byte buffer[], unsigned *bytes, void *client_data)
		{
			(void)encoder;
			FLAC__ASSERT(0 != client_data);
			Stream *instance = reinterpret_cast<Stream *>(client_data);
			FLAC__ASSERT(0 != instance);
			return instance->read_callback(buffer, bytes);
		}

	}
}
