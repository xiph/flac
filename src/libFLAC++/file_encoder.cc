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

#include "FLAC++/encoder.h"
#include "FLAC/assert.h"

namespace FLAC {
	namespace Encoder {

		File::File():
		encoder_(::FLAC__file_encoder_new())
		{ }

		File::~File()
		{
			if(0 != encoder_) {
				::FLAC__file_encoder_finish(encoder_);
				::FLAC__file_encoder_delete(encoder_);
			}
		}

		bool File::is_valid() const
		{
			return 0 != encoder_;
		}

		bool File::set_verify(bool value)
		{
			FLAC__ASSERT(is_valid());
			return (bool)::FLAC__file_encoder_set_verify(encoder_, value);
		}

		bool File::set_streamable_subset(bool value)
		{
			FLAC__ASSERT(is_valid());
			return (bool)::FLAC__file_encoder_set_streamable_subset(encoder_, value);
		}

		bool File::set_do_mid_side_stereo(bool value)
		{
			FLAC__ASSERT(is_valid());
			return (bool)::FLAC__file_encoder_set_do_mid_side_stereo(encoder_, value);
		}

		bool File::set_loose_mid_side_stereo(bool value)
		{
			FLAC__ASSERT(is_valid());
			return (bool)::FLAC__file_encoder_set_loose_mid_side_stereo(encoder_, value);
		}

		bool File::set_channels(unsigned value)
		{
			FLAC__ASSERT(is_valid());
			return (bool)::FLAC__file_encoder_set_channels(encoder_, value);
		}

		bool File::set_bits_per_sample(unsigned value)
		{
			FLAC__ASSERT(is_valid());
			return (bool)::FLAC__file_encoder_set_bits_per_sample(encoder_, value);
		}

		bool File::set_sample_rate(unsigned value)
		{
			FLAC__ASSERT(is_valid());
			return (bool)::FLAC__file_encoder_set_sample_rate(encoder_, value);
		}

		bool File::set_blocksize(unsigned value)
		{
			FLAC__ASSERT(is_valid());
			return (bool)::FLAC__file_encoder_set_blocksize(encoder_, value);
		}

		bool File::set_max_lpc_order(unsigned value)
		{
			FLAC__ASSERT(is_valid());
			return (bool)::FLAC__file_encoder_set_max_lpc_order(encoder_, value);
		}

		bool File::set_qlp_coeff_precision(unsigned value)
		{
			FLAC__ASSERT(is_valid());
			return (bool)::FLAC__file_encoder_set_qlp_coeff_precision(encoder_, value);
		}

		bool File::set_do_qlp_coeff_prec_search(bool value)
		{
			FLAC__ASSERT(is_valid());
			return (bool)::FLAC__file_encoder_set_do_qlp_coeff_prec_search(encoder_, value);
		}

		bool File::set_do_escape_coding(bool value)
		{
			FLAC__ASSERT(is_valid());
			return (bool)::FLAC__file_encoder_set_do_escape_coding(encoder_, value);
		}

		bool File::set_do_exhaustive_model_search(bool value)
		{
			FLAC__ASSERT(is_valid());
			return (bool)::FLAC__file_encoder_set_do_exhaustive_model_search(encoder_, value);
		}

		bool File::set_min_residual_partition_order(unsigned value)
		{
			FLAC__ASSERT(is_valid());
			return (bool)::FLAC__file_encoder_set_min_residual_partition_order(encoder_, value);
		}

		bool File::set_max_residual_partition_order(unsigned value)
		{
			FLAC__ASSERT(is_valid());
			return (bool)::FLAC__file_encoder_set_max_residual_partition_order(encoder_, value);
		}

		bool File::set_rice_parameter_search_dist(unsigned value)
		{
			FLAC__ASSERT(is_valid());
			return (bool)::FLAC__file_encoder_set_rice_parameter_search_dist(encoder_, value);
		}

		bool File::set_total_samples_estimate(FLAC__uint64 value)
		{
			FLAC__ASSERT(is_valid());
			return (bool)::FLAC__file_encoder_set_total_samples_estimate(encoder_, value);
		}

		bool File::set_metadata(::FLAC__StreamMetadata **metadata, unsigned num_blocks)
		{
			FLAC__ASSERT(is_valid());
			return (bool)::FLAC__file_encoder_set_metadata(encoder_, metadata, num_blocks);
		}

		bool File::set_filename(const char *value)
		{
			FLAC__ASSERT(is_valid());
			return (bool)::FLAC__file_encoder_set_filename(encoder_, value);
		}

		File::State File::get_state() const
		{
			FLAC__ASSERT(is_valid());
			return State(::FLAC__file_encoder_get_state(encoder_));
		}

		SeekableStream::State File::get_seekable_stream_encoder_state() const
		{
			FLAC__ASSERT(is_valid());
			return SeekableStream::State(::FLAC__file_encoder_get_seekable_stream_encoder_state(encoder_));
		}

		Stream::State File::get_stream_encoder_state() const
		{
			FLAC__ASSERT(is_valid());
			return Stream::State(::FLAC__file_encoder_get_stream_encoder_state(encoder_));
		}

		Decoder::Stream::State File::get_verify_decoder_state() const
		{
			FLAC__ASSERT(is_valid());
			return Decoder::Stream::State(::FLAC__file_encoder_get_verify_decoder_state(encoder_));
		}

		bool File::get_verify() const
		{
			FLAC__ASSERT(is_valid());
			return (bool)::FLAC__file_encoder_get_verify(encoder_);
		}

		bool File::get_streamable_subset() const
		{
			FLAC__ASSERT(is_valid());
			return (bool)::FLAC__file_encoder_get_streamable_subset(encoder_);
		}

		bool File::get_do_mid_side_stereo() const
		{
			FLAC__ASSERT(is_valid());
			return (bool)::FLAC__file_encoder_get_do_mid_side_stereo(encoder_);
		}

		bool File::get_loose_mid_side_stereo() const
		{
			FLAC__ASSERT(is_valid());
			return (bool)::FLAC__file_encoder_get_loose_mid_side_stereo(encoder_);
		}

		unsigned File::get_channels() const
		{
			FLAC__ASSERT(is_valid());
			return ::FLAC__file_encoder_get_channels(encoder_);
		}

		unsigned File::get_bits_per_sample() const
		{
			FLAC__ASSERT(is_valid());
			return ::FLAC__file_encoder_get_bits_per_sample(encoder_);
		}

		unsigned File::get_sample_rate() const
		{
			FLAC__ASSERT(is_valid());
			return ::FLAC__file_encoder_get_sample_rate(encoder_);
		}

		unsigned File::get_blocksize() const
		{
			FLAC__ASSERT(is_valid());
			return ::FLAC__file_encoder_get_blocksize(encoder_);
		}

		unsigned File::get_max_lpc_order() const
		{
			FLAC__ASSERT(is_valid());
			return ::FLAC__file_encoder_get_max_lpc_order(encoder_);
		}

		unsigned File::get_qlp_coeff_precision() const
		{
			FLAC__ASSERT(is_valid());
			return ::FLAC__file_encoder_get_qlp_coeff_precision(encoder_);
		}

		bool File::get_do_qlp_coeff_prec_search() const
		{
			FLAC__ASSERT(is_valid());
			return (bool)::FLAC__file_encoder_get_do_qlp_coeff_prec_search(encoder_);
		}

		bool File::get_do_escape_coding() const
		{
			FLAC__ASSERT(is_valid());
			return (bool)::FLAC__file_encoder_get_do_escape_coding(encoder_);
		}

		bool File::get_do_exhaustive_model_search() const
		{
			FLAC__ASSERT(is_valid());
			return (bool)::FLAC__file_encoder_get_do_exhaustive_model_search(encoder_);
		}

		unsigned File::get_min_residual_partition_order() const
		{
			FLAC__ASSERT(is_valid());
			return ::FLAC__file_encoder_get_min_residual_partition_order(encoder_);
		}

		unsigned File::get_max_residual_partition_order() const
		{
			FLAC__ASSERT(is_valid());
			return ::FLAC__file_encoder_get_max_residual_partition_order(encoder_);
		}

		unsigned File::get_rice_parameter_search_dist() const
		{
			FLAC__ASSERT(is_valid());
			return ::FLAC__file_encoder_get_rice_parameter_search_dist(encoder_);
		}

		FLAC__uint64 File::get_total_samples_estimate() const
		{
			FLAC__ASSERT(is_valid());
			return ::FLAC__file_encoder_get_total_samples_estimate(encoder_);
		}

		File::State File::init()
		{
			FLAC__ASSERT(is_valid());
			::FLAC__file_encoder_set_progress_callback(encoder_, progress_callback_);
			::FLAC__file_encoder_set_client_data(encoder_, (void*)this);
			return State(::FLAC__file_encoder_init(encoder_));
		}

		void File::finish()
		{
			FLAC__ASSERT(is_valid());
			::FLAC__file_encoder_finish(encoder_);
		}

		bool File::process(const FLAC__int32 * const buffer[], unsigned samples)
		{
			FLAC__ASSERT(is_valid());
			return (bool)::FLAC__file_encoder_process(encoder_, buffer, samples);
		}

		bool File::process_interleaved(const FLAC__int32 buffer[], unsigned samples)
		{
			FLAC__ASSERT(is_valid());
			return (bool)::FLAC__file_encoder_process_interleaved(encoder_, buffer, samples);
		}

		void File::progress_callback(unsigned current_frame, unsigned total_frames_estimate)
		{
			(void)current_frame, (void)total_frames_estimate;
		}

		void File::progress_callback_(const ::FLAC__FileEncoder *encoder, unsigned current_frame, unsigned total_frames_estimate, void *client_data)
		{
			(void)encoder;
			FLAC__ASSERT(0 != client_data);
			File *instance = reinterpret_cast<File *>(client_data);
			FLAC__ASSERT(0 != instance);
			instance->progress_callback(current_frame, total_frames_estimate);
		}

	};
};
