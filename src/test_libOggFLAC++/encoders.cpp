/* test_libOggFLAC++ - Unit tester for libOggFLAC++
 * Copyright (C) 2002,2003  Josh Coalson
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

#include "encoders.h"
extern "C" {
#include "file_utils.h"
#include "metadata_utils.h"
}
#include "FLAC/assert.h"
#include "OggFLAC++/encoder.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static ::FLAC__StreamMetadata streaminfo_, padding_, seektable_, application1_, application2_, vorbiscomment_, cuesheet_, unknown_;
static ::FLAC__StreamMetadata *metadata_sequence_[] = { &padding_, &seektable_, &application1_, &application2_, &vorbiscomment_, &cuesheet_, &unknown_ };
static const unsigned num_metadata_ = sizeof(metadata_sequence_) / sizeof(metadata_sequence_[0]);

static void init_metadata_blocks_()
{
	mutils__init_metadata_blocks(&streaminfo_, &padding_, &seektable_, &application1_, &application2_, &vorbiscomment_, &cuesheet_, &unknown_);
}

static void free_metadata_blocks_()
{
	mutils__free_metadata_blocks(&streaminfo_, &padding_, &seektable_, &application1_, &application2_, &vorbiscomment_, &cuesheet_, &unknown_);
}

class StreamEncoder : public OggFLAC::Encoder::Stream {
public:
	StreamEncoder(): OggFLAC::Encoder::Stream() { }
	~StreamEncoder() { }

	// from OggFLAC::Encoder::Stream
	::FLAC__StreamEncoderWriteStatus write_callback(const FLAC__byte buffer[], unsigned bytes, unsigned samples, unsigned current_frame);
	void metadata_callback(const FLAC__StreamMetadata *metadata);

	bool die(const char *msg = 0) const;
};

::FLAC__StreamEncoderWriteStatus StreamEncoder::write_callback(const FLAC__byte buffer[], unsigned bytes, unsigned samples, unsigned current_frame)
{
	(void)buffer, (void)bytes, (void)samples, (void)current_frame;

	return ::FLAC__STREAM_ENCODER_WRITE_STATUS_OK;
}

void StreamEncoder::metadata_callback(const FLAC__StreamMetadata *metadata)
{
	(void)metadata;
}

bool StreamEncoder::die(const char *msg) const
{
	State state = get_state();

	if(msg)
		printf("FAILED, %s", msg);
	else
		printf("FAILED");

	printf(", state = %u (%s)\n", (unsigned)((::OggFLAC__StreamEncoderState)state), state.as_cstring());
	if(state == ::OggFLAC__STREAM_ENCODER_FLAC_STREAM_ENCODER_ERROR) {
		FLAC::Encoder::Stream::State state_ = get_FLAC_stream_encoder_state();
		printf("      FLAC stream encoder state = %u (%s)\n", (unsigned)((::FLAC__StreamEncoderState)state_), state_.as_cstring());
		if(state_ == ::FLAC__STREAM_ENCODER_VERIFY_DECODER_ERROR) {
			FLAC::Decoder::Stream::State dstate = get_verify_decoder_state();
			printf("      verify decoder state = %u (%s)\n", (unsigned)((::FLAC__StreamDecoderState)dstate), dstate.as_cstring());
		}
	}

	return false;
}

static bool test_stream_encoder()
{
	StreamEncoder *encoder;
	FLAC__int32 samples[1024];
	FLAC__int32 *samples_array[1] = { samples };
	unsigned i;

	printf("\n+++ libOggFLAC++ unit test: OggFLAC::Encoder::Stream\n\n");

	printf("allocating encoder instance... ");
	encoder = new StreamEncoder();
	if(0 == encoder) {
		printf("FAILED, new returned NULL\n");
		return false;
	}
	printf("OK\n");

	printf("testing is_valid()... ");
	if(!encoder->is_valid()) {
		printf("FAILED, returned false\n");
		return false;
	}
	printf("OK\n");

	printf("testing set_serial_number()... ");
	if(!encoder->set_serial_number(file_utils__serial_number))
		return encoder->die("returned false");
	printf("OK\n");

	printf("testing set_verify()... ");
	if(!encoder->set_verify(true))
		return encoder->die("returned false");
	printf("OK\n");

	printf("testing set_streamable_subset()... ");
	if(!encoder->set_streamable_subset(true))
		return encoder->die("returned false");
	printf("OK\n");

	printf("testing set_do_mid_side_stereo()... ");
	if(!encoder->set_do_mid_side_stereo(false))
		return encoder->die("returned false");
	printf("OK\n");

	printf("testing set_loose_mid_side_stereo()... ");
	if(!encoder->set_loose_mid_side_stereo(false))
		return encoder->die("returned false");
	printf("OK\n");

	printf("testing set_channels()... ");
	if(!encoder->set_channels(streaminfo_.data.stream_info.channels))
		return encoder->die("returned false");
	printf("OK\n");

	printf("testing set_bits_per_sample()... ");
	if(!encoder->set_bits_per_sample(streaminfo_.data.stream_info.bits_per_sample))
		return encoder->die("returned false");
	printf("OK\n");

	printf("testing set_sample_rate()... ");
	if(!encoder->set_sample_rate(streaminfo_.data.stream_info.sample_rate))
		return encoder->die("returned false");
	printf("OK\n");

	printf("testing set_blocksize()... ");
	if(!encoder->set_blocksize(streaminfo_.data.stream_info.min_blocksize))
		return encoder->die("returned false");
	printf("OK\n");

	printf("testing set_max_lpc_order()... ");
	if(!encoder->set_max_lpc_order(0))
		return encoder->die("returned false");
	printf("OK\n");

	printf("testing set_qlp_coeff_precision()... ");
	if(!encoder->set_qlp_coeff_precision(0))
		return encoder->die("returned false");
	printf("OK\n");

	printf("testing set_do_qlp_coeff_prec_search()... ");
	if(!encoder->set_do_qlp_coeff_prec_search(false))
		return encoder->die("returned false");
	printf("OK\n");

	printf("testing set_do_escape_coding()... ");
	if(!encoder->set_do_escape_coding(false))
		return encoder->die("returned false");
	printf("OK\n");

	printf("testing set_do_exhaustive_model_search()... ");
	if(!encoder->set_do_exhaustive_model_search(false))
		return encoder->die("returned false");
	printf("OK\n");

	printf("testing set_min_residual_partition_order()... ");
	if(!encoder->set_min_residual_partition_order(0))
		return encoder->die("returned false");
	printf("OK\n");

	printf("testing set_max_residual_partition_order()... ");
	if(!encoder->set_max_residual_partition_order(0))
		return encoder->die("returned false");
	printf("OK\n");

	printf("testing set_rice_parameter_search_dist()... ");
	if(!encoder->set_rice_parameter_search_dist(0))
		return encoder->die("returned false");
	printf("OK\n");

	printf("testing set_total_samples_estimate()... ");
	if(!encoder->set_total_samples_estimate(streaminfo_.data.stream_info.total_samples))
		return encoder->die("returned false");
	printf("OK\n");

	printf("testing set_metadata()... ");
	if(!encoder->set_metadata(metadata_sequence_, num_metadata_))
		return encoder->die("returned false");
	printf("OK\n");

	printf("testing init()... ");
	if(encoder->init() != ::OggFLAC__STREAM_ENCODER_OK)
		return encoder->die();
	printf("OK\n");

	printf("testing get_state()... ");
	OggFLAC::Encoder::Stream::State state = encoder->get_state();
	printf("returned state = %u (%s)... OK\n", (unsigned)((::OggFLAC__StreamEncoderState)state), state.as_cstring());

	printf("testing get_FLAC_stream_encoder_state()... ");
	FLAC::Encoder::Stream::State state_ = encoder->get_FLAC_stream_encoder_state();
	printf("returned state = %u (%s)... OK\n", (unsigned)((::FLAC__StreamEncoderState)state_), state_.as_cstring());

	printf("testing get_verify_decoder_state()... ");
	FLAC::Decoder::Stream::State dstate = encoder->get_verify_decoder_state();
	printf("returned state = %u (%s)... OK\n", (unsigned)((::FLAC__StreamDecoderState)dstate), dstate.as_cstring());

	{
		FLAC__uint64 absolute_sample;
		unsigned frame_number;
		unsigned channel;
		unsigned sample;
		FLAC__int32 expected;
		FLAC__int32 got;

		printf("testing get_verify_decoder_error_stats()... ");
		encoder->get_verify_decoder_error_stats(&absolute_sample, &frame_number, &channel, &sample, &expected, &got);
		printf("OK\n");
	}

	printf("testing get_verify()... ");
	if(encoder->get_verify() != true) {
		printf("FAILED, expected true, got false\n");
		return false;
	}
	printf("OK\n");

	printf("testing get_streamable_subset()... ");
	if(encoder->get_streamable_subset() != true) {
		printf("FAILED, expected true, got false\n");
		return false;
	}
	printf("OK\n");

	printf("testing get_do_mid_side_stereo()... ");
	if(encoder->get_do_mid_side_stereo() != false) {
		printf("FAILED, expected false, got true\n");
		return false;
	}
	printf("OK\n");

	printf("testing get_loose_mid_side_stereo()... ");
	if(encoder->get_loose_mid_side_stereo() != false) {
		printf("FAILED, expected false, got true\n");
		return false;
	}
	printf("OK\n");

	printf("testing get_channels()... ");
	if(encoder->get_channels() != streaminfo_.data.stream_info.channels) {
		printf("FAILED, expected %u, got %u\n", streaminfo_.data.stream_info.channels, encoder->get_channels());
		return false;
	}
	printf("OK\n");

	printf("testing get_bits_per_sample()... ");
	if(encoder->get_bits_per_sample() != streaminfo_.data.stream_info.bits_per_sample) {
		printf("FAILED, expected %u, got %u\n", streaminfo_.data.stream_info.bits_per_sample, encoder->get_bits_per_sample());
		return false;
	}
	printf("OK\n");

	printf("testing get_sample_rate()... ");
	if(encoder->get_sample_rate() != streaminfo_.data.stream_info.sample_rate) {
		printf("FAILED, expected %u, got %u\n", streaminfo_.data.stream_info.sample_rate, encoder->get_sample_rate());
		return false;
	}
	printf("OK\n");

	printf("testing get_blocksize()... ");
	if(encoder->get_blocksize() != streaminfo_.data.stream_info.min_blocksize) {
		printf("FAILED, expected %u, got %u\n", streaminfo_.data.stream_info.min_blocksize, encoder->get_blocksize());
		return false;
	}
	printf("OK\n");

	printf("testing get_max_lpc_order()... ");
	if(encoder->get_max_lpc_order() != 0) {
		printf("FAILED, expected %u, got %u\n", 0, encoder->get_max_lpc_order());
		return false;
	}
	printf("OK\n");

	printf("testing get_qlp_coeff_precision()... ");
	(void)encoder->get_qlp_coeff_precision();
	/* we asked the encoder to auto select this so we accept anything */
	printf("OK\n");

	printf("testing get_do_qlp_coeff_prec_search()... ");
	if(encoder->get_do_qlp_coeff_prec_search() != false) {
		printf("FAILED, expected false, got true\n");
		return false;
	}
	printf("OK\n");

	printf("testing get_do_escape_coding()... ");
	if(encoder->get_do_escape_coding() != false) {
		printf("FAILED, expected false, got true\n");
		return false;
	}
	printf("OK\n");

	printf("testing get_do_exhaustive_model_search()... ");
	if(encoder->get_do_exhaustive_model_search() != false) {
		printf("FAILED, expected false, got true\n");
		return false;
	}
	printf("OK\n");

	printf("testing get_min_residual_partition_order()... ");
	if(encoder->get_min_residual_partition_order() != 0) {
		printf("FAILED, expected %u, got %u\n", 0, encoder->get_min_residual_partition_order());
		return false;
	}
	printf("OK\n");

	printf("testing get_max_residual_partition_order()... ");
	if(encoder->get_max_residual_partition_order() != 0) {
		printf("FAILED, expected %u, got %u\n", 0, encoder->get_max_residual_partition_order());
		return false;
	}
	printf("OK\n");

	printf("testing get_rice_parameter_search_dist()... ");
	if(encoder->get_rice_parameter_search_dist() != 0) {
		printf("FAILED, expected %u, got %u\n", 0, encoder->get_rice_parameter_search_dist());
		return false;
	}
	printf("OK\n");

	printf("testing get_total_samples_estimate()... ");
	if(encoder->get_total_samples_estimate() != streaminfo_.data.stream_info.total_samples) {
		printf("FAILED, expected %llu, got %llu\n", streaminfo_.data.stream_info.total_samples, encoder->get_total_samples_estimate());
		return false;
	}
	printf("OK\n");

	/* init the dummy sample buffer */
	for(i = 0; i < sizeof(samples) / sizeof(FLAC__int32); i++)
		samples[i] = i & 7;

	printf("testing process()... ");
	if(!encoder->process(samples_array, sizeof(samples) / sizeof(FLAC__int32)))
		return encoder->die("returned false");
	printf("OK\n");

	printf("testing process_interleaved()... ");
	if(!encoder->process_interleaved(samples, sizeof(samples) / sizeof(FLAC__int32)))
		return encoder->die("returned false");
	printf("OK\n");

	printf("testing finish()... ");
	encoder->finish();
	printf("OK\n");

	printf("freeing encoder instance... ");
	delete encoder;
	printf("OK\n");

	printf("\nPASSED!\n");

	return true;
}

bool test_encoders()
{
	init_metadata_blocks_();

	if(!test_stream_encoder())
		return false;

	free_metadata_blocks_();

	return true;
}
