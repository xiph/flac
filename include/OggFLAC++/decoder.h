/* libOggFLAC++ - Free Lossless Audio Codec + Ogg library
 * Copyright (C) 2002,2003,2004,2005,2006 Josh Coalson
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

#ifndef OggFLACPP__DECODER_H
#define OggFLACPP__DECODER_H

#include "export.h"

#include "OggFLAC/stream_decoder.h"
// we only need this for the state abstraction really...
#include "FLAC++/decoder.h"


/** \file include/OggFLAC++/decoder.h
 *
 *  \brief
 *  This module contains the classes which implement the various
 *  decoders.
 *
 *  See the detailed documentation in the
 *  \link oggflacpp_decoder decoder \endlink module.
 */

/** \defgroup oggflacpp_decoder OggFLAC++/decoder.h: decoder classes
 *  \ingroup oggflacpp
 *
 *  \brief
 *  This module describes the decoder layers provided by libOggFLAC++.
 *
 * The libOggFLAC++ decoder classes are object wrappers around their
 * counterparts in libOggFLAC.  All decoding layers available in
 * libOggFLAC are also provided here.  The interface is very similar;
 * make sure to read the \link oggflac_decoder libOggFLAC decoder module \endlink.
 *
 * There are only two significant differences here.  First, instead of
 * passing in C function pointers for callbacks, you inherit from the
 * decoder class and provide implementations for the callbacks in your
 * derived class; because of this there is no need for a 'client_data'
 * property.
 *
 * Second, there are two stream decoder classes.  OggFLAC::Decoder::Stream
 * is used for the same cases that OggFLAC__stream_decoder_init_stream() is
 * used, and OggFLAC::Decoder::File is used for the same cases that
 * OggFLAC__stream_decoder_init_FILE() and OggFLAC__stream_decoder_init_file()
 * are used.
 *
 * Note that OggFLAC::Decoder::Stream inherits from FLAC::Decoder::Stream so
 * it is possible to support both FLAC and Ogg FLAC decoding with only a
 * little specialized code (just up to initialization).  For example,
 * \code
 * class MyFLACDecoder: public FLAC::Decoder::Stream
 * {
 *     // implement callbacks
 * };
 * FLAC::Decoder::Stream *flac = new MyFLACDecoder();
 * if(*flac) {
 *     flac->set_...();
 *     // continue set_ calls()
 *     if(flac->init() == ::FLAC__STREAM_DECODER_INIT_STATUS_OK)
 *         my_process_stream(flac);
 * }
 * 
 * ...
 * 
 * class MyOggFLACDecoder: public OggFLAC::Decoder::Stream
 * {
 *     // implement callbacks
 * };
 * FLAC::Decoder::Stream *oggflac = new MyOggFLACDecoder();
 * if(*oggflac) {
 *     oggflac->set_serial_number();
 *     // continue set_ calls()
 *     if(oggflac->init() == ::FLAC__STREAM_DECODER_INIT_STATUS_OK)
 *         my_process_stream(oggflac);
 * }
 * \endcode
 */

namespace OggFLAC {
	namespace Decoder {

		/** \defgroup oggflacpp_stream_decoder OggFLAC++/decoder.h: stream decoder class
		 *  \ingroup oggflacpp_decoder
		 *
		 *  \brief
		 *  This class wraps the ::OggFLAC__StreamDecoder.
		 *
		 * See the \link oggflac_stream_decoder libOggFLAC stream decoder module \endlink
		 * for basic usage.
		 *
		 * \{
		 */

		/** This class wraps the ::OggFLAC__StreamDecoder.  If you are
		 *  decoding from a file, OggFLAC::Decoder::File may be more
		 *  convenient.
		 *
		 * The usage of this class is similar to OggFLAC__StreamDecoder,
		 * except instead of providing callbacks to
		 * OggFLAC__stream_decoder_init_stream(), you will inherit from this
		 * class and override the virtual callback functions with your
		 * own implementations, then call Stream::init().  The rest of
		 * the calls work the same as in the C layer.
		 *
		 * Only the read, write, and error callbacks are mandatory.  The
		 * others are optional; this class provides default
		 * implementations that do nothing.  In order for seeking to work
		 * you must overide seek_callback(), tell_callback(),
		 * length_callback(), and eof_callback().
		 */
		class OggFLACPP_API Stream: public FLAC::Decoder::Stream {
		public:
			/** This class is a wrapper around OggFLAC__StreamDecoderState.
			 */
			class OggFLACPP_API State {
			public:
				inline State(::OggFLAC__StreamDecoderState state): state_(state) { }
				inline operator ::OggFLAC__StreamDecoderState() const { return state_; }
				inline const char *as_cstring() const { return ::OggFLAC__StreamDecoderStateString[state_]; }
				inline const char *resolved_as_cstring(const Stream &decoder) const { return ::OggFLAC__stream_decoder_get_resolved_state_string((const OggFLAC__StreamDecoder*)decoder.decoder_); }
			protected:
				::OggFLAC__StreamDecoderState state_;
			};

			Stream();
			virtual ~Stream();

			bool set_serial_number(long value);                            ///< See OggFLAC__stream_decoder_set_serial_number()
			bool set_md5_checking(bool value);                             ///< See OggFLAC__stream_decoder_set_md5_checking()
			bool set_metadata_respond(::FLAC__MetadataType type);          ///< See OggFLAC__stream_decoder_set_metadata_respond()
			bool set_metadata_respond_application(const FLAC__byte id[4]); ///< See OggFLAC__stream_decoder_set_metadata_respond_application()
			bool set_metadata_respond_all();                               ///< See OggFLAC__stream_decoder_set_metadata_respond_all()
			bool set_metadata_ignore(::FLAC__MetadataType type);           ///< See OggFLAC__stream_decoder_set_metadata_ignore()
			bool set_metadata_ignore_application(const FLAC__byte id[4]);  ///< See OggFLAC__stream_decoder_set_metadata_ignore_application()
			bool set_metadata_ignore_all();                                ///< See OggFLAC__stream_decoder_set_metadata_ignore_all()

			State get_state() const;                                  ///< See OggFLAC__stream_decoder_get_state()
			FLAC::Decoder::Stream::State get_FLAC_stream_decoder_state() const; ///< See OggFLAC__stream_decoder_get_FLAC_stream_decoder_state()
			bool get_md5_checking() const;                            ///< See OggFLAC__stream_decoder_get_md5_checking()
			FLAC__uint64 get_total_samples() const;                   ///< See OggFLAC__stream_decoder_get_total_samples()
			unsigned get_channels() const;                            ///< See OggFLAC__stream_decoder_get_channels()
			::FLAC__ChannelAssignment get_channel_assignment() const; ///< See OggFLAC__stream_decoder_get_channel_assignment()
			unsigned get_bits_per_sample() const;                     ///< See OggFLAC__stream_decoder_get_bits_per_sample()
			unsigned get_sample_rate() const;                         ///< See OggFLAC__stream_decoder_get_sample_rate()
			unsigned get_blocksize() const;                           ///< See OggFLAC__stream_decoder_get_blocksize()

			/** Initialize the instance; as with the C interface,
			 *  init() should be called after construction and 'set'
			 *  calls but before any of the 'process' calls.
			 *
			 *  See OggFLAC__stream_decoder_init_stream().
			 */
			::FLAC__StreamDecoderInitStatus init();

			void finish(); ///< See OggFLAC__stream_decoder_finish()

			bool flush(); ///< See OggFLAC__stream_decoder_flush()
			bool reset(); ///< See OggFLAC__stream_decoder_reset()

			bool process_single();                ///< See OggFLAC__stream_decoder_process_single()
			bool process_until_end_of_metadata(); ///< See OggFLAC__stream_decoder_process_until_end_of_metadata()
			bool process_until_end_of_stream();   ///< See OggFLAC__stream_decoder_process_until_end_of_stream()
			bool skip_single_frame();             ///< See OggFLAC__stream_decoder_skip_single_frame()

			bool seek_absolute(FLAC__uint64 sample); ///< See OggFLAC__stream_decoder_seek_absolute()
		protected:
#if (defined _MSC_VER) || (defined __GNUG__ && (__GNUG__ < 2 || (__GNUG__ == 2 && __GNUC_MINOR__ < 96))) || (defined __SUNPRO_CC)
			// lame hack: some MSVC/GCC versions can't see a protected decoder_ from nested State::resolved_as_cstring()
			friend State;
#endif
		private:
			// Private and undefined so you can't use them:
			Stream(const Stream &);
			void operator=(const Stream &);
		};

		/* \} */

		/** \defgroup oggflacpp_file_decoder OggFLAC++/decoder.h: file decoder class
		 *  \ingroup oggflacpp_decoder
		 *
		 *  \brief
		 *  This class wraps the ::OggFLAC__FileDecoder.
		 *
		 * See the \link oggflac_stream_decoder libOggFLAC stream decoder module \endlink
		 * for basic usage.
		 *
		 * \{
		 */

		/** This class wraps the ::OggFLAC__StreamDecoder.  If you are
		 *  not decoding from a file, you may need to use
		 *  OggFLAC::Decoder::Stream.
		 *
		 * The usage of this class is similar to OggFLAC__StreamDecoder,
		 * except instead of providing callbacks to
		 * OggFLAC__stream_decoder_init_FILE() or
		 * OggFLAC__stream_decoder_init_file(), you will inherit from this
		 * class and override the virtual callback functions with your
		 * own implementations, then call File::init().  The rest of
		 * the calls work the same as in the C layer.
		 *
		 * Only the write, and error callbacks from OggFLAC::Decoder::Stream
		 * are mandatory.  The others are optional; this class provides
		 * full working implementations for all other callbacks and
		 * supports seeking.
		 */
		class OggFLACPP_API File: public Stream {
		public:
			File();
			virtual ~File();

			//@{
			/** Initialize the instance; as with the C interface,
			 *  init() should be called after construction and 'set'
			 *  calls but before any of the 'process' calls.
			 *
			 *  See OggFLAC__stream_decoder_init_FILE() and
			 *  OggFLAC__stream_decoder_init_file().
			 */
			::FLAC__StreamDecoderInitStatus init(FILE *file);
			::FLAC__StreamDecoderInitStatus init(const char *filename);
			::FLAC__StreamDecoderInitStatus init(const std::string &filename);
			//@}
		protected:
			// this is a dummy implementation to satisfy the pure virtual in Stream that is actually supplied internally by the C layer
			virtual ::FLAC__StreamDecoderReadStatus read_callback(FLAC__byte buffer[], unsigned *bytes);
		private:
			// Private and undefined so you can't use them:
			File(const File &);
			void operator=(const File &);
		};

		/* \} */

	}
}

#endif
