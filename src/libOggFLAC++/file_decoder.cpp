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

		File::File():
			Stream()
		{ }

		File::~File()
		{
		}

		::FLAC__StreamDecoderInitStatus File::init(FILE *file)
		{
			FLAC__ASSERT(0 != decoder_);
			return ::OggFLAC__stream_decoder_init_FILE((OggFLAC__StreamDecoder*)decoder_, file, write_callback_, metadata_callback_, error_callback_, /*client_data=*/(void*)this);
		}

		::FLAC__StreamDecoderInitStatus File::init(const char *filename)
		{
			FLAC__ASSERT(0 != decoder_);
			return ::OggFLAC__stream_decoder_init_file((OggFLAC__StreamDecoder*)decoder_, filename, write_callback_, metadata_callback_, error_callback_, /*client_data=*/(void*)this);
		}

		::FLAC__StreamDecoderInitStatus File::init(const std::string &filename)
		{
			return init(filename.c_str());
		}

		// This is a dummy to satisfy the pure virtual from Stream; the
		// read callback will never be called since we are initializing
		// with FLAC__stream_decoder_init_FILE() or
		// FLAC__stream_decoder_init_file() and those supply the read
		// callback internally.
		::FLAC__StreamDecoderReadStatus File::read_callback(FLAC__byte buffer[], unsigned *bytes)
		{
			(void)buffer, (void)bytes;
			FLAC__ASSERT(false);
			return ::FLAC__STREAM_DECODER_READ_STATUS_ABORT; // double protection
		}

	}
}
