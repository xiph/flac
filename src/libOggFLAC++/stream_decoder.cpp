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

#include "OggFLAC++/decoder.h"
#include "FLAC/assert.h"

#ifdef _MSC_VER
// warning C4800: 'int' : forcing to bool 'true' or 'false' (performance warning)
#pragma warning ( disable : 4800 )
#endif

namespace OggFLAC {
	namespace Decoder {

		// We can inherit from FLAC::Decoder::Stream because we jam a
		// OggFLAC__StreamDecoder pointer into the decoder_ member,
		// hence the pointer casting on decoder_ everywhere.

		Stream::Stream():
		FLAC::Decoder::Stream((FLAC__StreamDecoder*)::OggFLAC__stream_decoder_new())
		{ }

		Stream::~Stream()
		{
			if(0 != decoder_) {
				::OggFLAC__stream_decoder_finish((OggFLAC__StreamDecoder*)decoder_);
				::OggFLAC__stream_decoder_delete((OggFLAC__StreamDecoder*)decoder_);
                // this is our signal to FLAC::Decoder::Stream::~Stream()
                // that we already deleted the decoder our way, so it
                // doesn't need to:
                decoder_ = 0;
			}
		}

		bool Stream::set_md5_checking(bool value)
		{
			FLAC__ASSERT(is_valid());
			return (bool)::OggFLAC__stream_decoder_set_md5_checking((OggFLAC__StreamDecoder*)decoder_, value);
		}

		bool Stream::set_serial_number(long value)
		{
			FLAC__ASSERT(is_valid());
			return (bool)::OggFLAC__stream_decoder_set_serial_number((OggFLAC__StreamDecoder*)decoder_, value);
		}

		bool Stream::set_metadata_respond(::FLAC__MetadataType type)
		{
			FLAC__ASSERT(is_valid());
			return (bool)::OggFLAC__stream_decoder_set_metadata_respond((OggFLAC__StreamDecoder*)decoder_, type);
		}

		bool Stream::set_metadata_respond_application(const FLAC__byte id[4])
		{
			FLAC__ASSERT(is_valid());
			return (bool)::OggFLAC__stream_decoder_set_metadata_respond_application((OggFLAC__StreamDecoder*)decoder_, id);
		}

		bool Stream::set_metadata_respond_all()
		{
			FLAC__ASSERT(is_valid());
			return (bool)::OggFLAC__stream_decoder_set_metadata_respond_all((OggFLAC__StreamDecoder*)decoder_);
		}

		bool Stream::set_metadata_ignore(::FLAC__MetadataType type)
		{
			FLAC__ASSERT(is_valid());
			return (bool)::OggFLAC__stream_decoder_set_metadata_ignore((OggFLAC__StreamDecoder*)decoder_, type);
		}

		bool Stream::set_metadata_ignore_application(const FLAC__byte id[4])
		{
			FLAC__ASSERT(is_valid());
			return (bool)::OggFLAC__stream_decoder_set_metadata_ignore_application((OggFLAC__StreamDecoder*)decoder_, id);
		}

		bool Stream::set_metadata_ignore_all()
		{
			FLAC__ASSERT(is_valid());
			return (bool)::OggFLAC__stream_decoder_set_metadata_ignore_all((OggFLAC__StreamDecoder*)decoder_);
		}

		Stream::State Stream::get_state() const
		{
			FLAC__ASSERT(is_valid());
			return State(::OggFLAC__stream_decoder_get_state((const OggFLAC__StreamDecoder*)decoder_));
		}

		FLAC::Decoder::Stream::State Stream::get_FLAC_stream_decoder_state() const
		{
			FLAC__ASSERT(is_valid());
			return FLAC::Decoder::Stream::State(::OggFLAC__stream_decoder_get_FLAC_stream_decoder_state((const OggFLAC__StreamDecoder*)decoder_));
		}

		bool Stream::get_md5_checking() const
		{
			FLAC__ASSERT(is_valid());
			return (bool)::OggFLAC__stream_decoder_get_md5_checking((const OggFLAC__StreamDecoder*)decoder_);
		}

		FLAC__uint64 Stream::get_total_samples() const
		{
			FLAC__ASSERT(is_valid());
			return ::OggFLAC__stream_decoder_get_total_samples((const OggFLAC__StreamDecoder*)decoder_);
		}

		unsigned Stream::get_channels() const
		{
			FLAC__ASSERT(is_valid());
			return ::OggFLAC__stream_decoder_get_channels((const OggFLAC__StreamDecoder*)decoder_);
		}

		::FLAC__ChannelAssignment Stream::get_channel_assignment() const
		{
			FLAC__ASSERT(is_valid());
			return ::OggFLAC__stream_decoder_get_channel_assignment((const OggFLAC__StreamDecoder*)decoder_);
		}

		unsigned Stream::get_bits_per_sample() const
		{
			FLAC__ASSERT(is_valid());
			return ::OggFLAC__stream_decoder_get_bits_per_sample((const OggFLAC__StreamDecoder*)decoder_);
		}

		unsigned Stream::get_sample_rate() const
		{
			FLAC__ASSERT(is_valid());
			return ::OggFLAC__stream_decoder_get_sample_rate((const OggFLAC__StreamDecoder*)decoder_);
		}

		unsigned Stream::get_blocksize() const
		{
			FLAC__ASSERT(is_valid());
			return ::OggFLAC__stream_decoder_get_blocksize((const OggFLAC__StreamDecoder*)decoder_);
		}

		::FLAC__StreamDecoderInitStatus Stream::init()
		{
			FLAC__ASSERT(is_valid());
			return ::OggFLAC__stream_decoder_init_stream((OggFLAC__StreamDecoder*)decoder_, read_callback_, seek_callback_, tell_callback_, length_callback_, eof_callback_, write_callback_, metadata_callback_, error_callback_, /*client_data=*/(void*)this);
		}

		void Stream::finish()
		{
			FLAC__ASSERT(is_valid());
			::OggFLAC__stream_decoder_finish((OggFLAC__StreamDecoder*)decoder_);
		}

		bool Stream::flush()
		{
			FLAC__ASSERT(is_valid());
			return (bool)::OggFLAC__stream_decoder_flush((OggFLAC__StreamDecoder*)decoder_);
		}

		bool Stream::reset()
		{
			FLAC__ASSERT(is_valid());
			return (bool)::OggFLAC__stream_decoder_reset((OggFLAC__StreamDecoder*)decoder_);
		}

		bool Stream::process_single()
		{
			FLAC__ASSERT(is_valid());
			return (bool)::OggFLAC__stream_decoder_process_single((OggFLAC__StreamDecoder*)decoder_);
		}

		bool Stream::process_until_end_of_metadata()
		{
			FLAC__ASSERT(is_valid());
			return (bool)::OggFLAC__stream_decoder_process_until_end_of_metadata((OggFLAC__StreamDecoder*)decoder_);
		}

		bool Stream::process_until_end_of_stream()
		{
			FLAC__ASSERT(is_valid());
			return (bool)::OggFLAC__stream_decoder_process_until_end_of_stream((OggFLAC__StreamDecoder*)decoder_);
		}

		bool Stream::skip_single_frame()
		{
			FLAC__ASSERT(is_valid());
			return (bool)::OggFLAC__stream_decoder_skip_single_frame((OggFLAC__StreamDecoder*)decoder_);
		}

		bool Stream::seek_absolute(FLAC__uint64 sample)
		{
			FLAC__ASSERT(is_valid());
			return (bool)::OggFLAC__stream_decoder_seek_absolute((OggFLAC__StreamDecoder*)decoder_, sample);
		}

	}
}
