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

		File::File():
			Stream()
		{ }

		File::~File()
		{ }

		::FLAC__StreamEncoderInitStatus File::init(FILE *file)
		{
			FLAC__ASSERT(is_valid());
			return ::OggFLAC__stream_encoder_init_FILE((OggFLAC__StreamEncoder*)encoder_, file, progress_callback_, /*client_data=*/(void*)this);
		}

		::FLAC__StreamEncoderInitStatus File::init(const char *filename)
		{
			FLAC__ASSERT(is_valid());
			return ::OggFLAC__stream_encoder_init_file((OggFLAC__StreamEncoder*)encoder_, filename, progress_callback_, /*client_data=*/(void*)this);
		}

		::FLAC__StreamEncoderInitStatus File::init(const std::string &filename)
		{
			return init(filename.c_str());
		}

		// This is a dummy to satisfy the pure virtual from Stream; the
		// read callback will never be called since we are initializing
		// with OggFLAC__stream_decoder_init_FILE() or
		// OggFLAC__stream_decoder_init_file() and those supply the read
		// callback internally.
		::FLAC__StreamEncoderWriteStatus File::write_callback(const FLAC__byte buffer[], unsigned bytes, unsigned samples, unsigned current_frame)
		{
			(void)buffer, (void)bytes, (void)samples, (void)current_frame;
			FLAC__ASSERT(false);
			return ::FLAC__STREAM_ENCODER_WRITE_STATUS_FATAL_ERROR; // double protection
		}

		void File::progress_callback(FLAC__uint64 bytes_written, FLAC__uint64 samples_written, unsigned frames_written, unsigned total_frames_estimate)
		{
			(void)bytes_written, (void)samples_written, (void)frames_written, (void)total_frames_estimate;
		}

		void File::progress_callback_(const ::FLAC__StreamEncoder *encoder, FLAC__uint64 bytes_written, FLAC__uint64 samples_written, unsigned frames_written, unsigned total_frames_estimate, void *client_data)
		{
			(void)encoder;
			FLAC__ASSERT(0 != client_data);
			File *instance = reinterpret_cast<File *>(client_data);
			FLAC__ASSERT(0 != instance);
			instance->progress_callback(bytes_written, samples_written, frames_written, total_frames_estimate);
		}

	}
}
