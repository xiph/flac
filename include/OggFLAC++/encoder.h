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

#ifndef OggFLACPP__ENCODER_H
#define OggFLACPP__ENCODER_H

#include "export.h"

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
 *  This module describes the encoder layers provided by libOggFLAC++.
 *
 * The libOggFLAC++ encoder classes are object wrappers around their
 * counterparts in libOggFLAC.  All decoding layers available in
 * libOggFLAC are also provided here.  The interface is very similar;
 * make sure to read the \link oggflac_encoder libOggFLAC encoder module \endlink.
 *
 * There are only two significant differences here.  First, instead of
 * passing in C function pointers for callbacks, you inherit from the
 * encoder class and provide implementations for the callbacks in your
 * derived class; because of this there is no need for a 'client_data'
 * property.
 *
 * Second, there are two stream encoder classes.  OggFLAC::Encoder::Stream
 * is used for the same cases that OggFLAC__stream_encoder_init_stream() is
 * used, and OggFLAC::Encoder::File is used for the same cases that
 * OggFLAC__stream_encoder_init_FILE() and OggFLAC__stream_encoder_init_file()
 * are used.
 *
 * Note that OggFLAC::Encoder::Stream inherits from FLAC::Encoder::Stream so
 * it is possible to support both FLAC and Ogg FLAC encoding with only a
 * little specialized code (just up to initialization).  For example,
 * \code
 * class MyFLACEncoder: public FLAC::Encoder::Stream
 * {
 *     // implement callbacks
 * };
 * FLAC::Encoder::Stream *flac = new MyFLACEncoder();
 * if(*flac) {
 *     flac->set_...();
 *     // continue set_ calls()
 *     if(flac->init() == ::FLAC__STREAM_ENCODER_INIT_STATUS_OK)
 *         my_process_stream(flac);
 * }
 * 
 * ...
 * 
 * class MyOggFLACEncoder: public OggFLAC::Encoder::Stream
 * {
 *     // implement callbacks
 * };
 * FLAC::Encoder::Stream *oggflac = new MyOggFLACEncoder();
 * if(*oggflac) {
 *     oggflac->set_serial_number();
 *     // continue set_ calls()
 *     if(oggflac->init() == ::FLAC__STREAM_ENCODER_INIT_STATUS_OK)
 *         my_process_stream(oggflac);
 * }
 * \endcode
 */

namespace OggFLAC {
	namespace Encoder {

		/** \defgroup oggflacpp_stream_encoder OggFLAC++/encoder.h: stream encoder class
		 *  \ingroup oggflacpp_encoder
		 *
		 *  \brief
		 *  This class wraps the ::OggFLAC__StreamEncoder.
		 *
		 * See the \link oggflac_stream_encoder libOggFLAC stream encoder module \endlink
		 * for basic usage.
		 *
		 * \{
		 */

		/** This class wraps the ::OggFLAC__StreamEncoder.  If you are
		 *  encoding to a file, OggFLAC::Encoder::File may be more
		 *  convenient.
		 *
		 * The usage of this class is similar to OggFLAC__StreamEncoder,
		 * except instead of providing callbacks to
		 * OggFLAC__stream_encoder_init_stream(), you will inherit from this
		 * class and override the virtual callback functions with your
		 * own implementations, then call Stream::init().  The rest of
		 * the calls work the same as in the C layer.
		 *
		 * Only the write callback is mandatory.  The others are
		 * optional; this class provides default implementations that do
		 * nothing.  In order for some STREAMINFO and SEEKTABLE data to
		 * be written properly, you must overide read_callback(), seek_callback() and
		 * tell_callback(); see OggFLAC__stream_encoder_init_stream() as to
		 * why.
		 */
		class OggFLACPP_API Stream: public FLAC::Encoder::Stream {
		public:
			class OggFLACPP_API State {
			public:
				inline State(::OggFLAC__StreamEncoderState state): state_(state) { }
				inline operator ::OggFLAC__StreamEncoderState() const { return state_; }
				inline const char *as_cstring() const { return ::OggFLAC__StreamEncoderStateString[state_]; }
				inline const char *resolved_as_cstring(const Stream &encoder) const { return ::OggFLAC__stream_encoder_get_resolved_state_string((const OggFLAC__StreamEncoder*)encoder.encoder_); }
			protected:
				::OggFLAC__StreamEncoderState state_;
			};

			Stream();
			virtual ~Stream();

			bool set_serial_number(long value);                     ///< See OggFLAC__stream_encoder_set_serial_number()
			bool set_verify(bool value);                            ///< See OggFLAC__stream_encoder_set_verify()
			bool set_streamable_subset(bool value);                 ///< See OggFLAC__stream_encoder_set_streamable_subset()
			bool set_do_mid_side_stereo(bool value);                ///< See OggFLAC__stream_encoder_set_do_mid_side_stereo()
			bool set_loose_mid_side_stereo(bool value);             ///< See OggFLAC__stream_encoder_set_loose_mid_side_stereo()
			bool set_channels(unsigned value);                      ///< See OggFLAC__stream_encoder_set_channels()
			bool set_bits_per_sample(unsigned value);               ///< See OggFLAC__stream_encoder_set_bits_per_sample()
			bool set_sample_rate(unsigned value);                   ///< See OggFLAC__stream_encoder_set_sample_rate()
			bool set_blocksize(unsigned value);                     ///< See OggFLAC__stream_encoder_set_blocksize()
			bool set_apodization(const char *specification);        ///< See OggFLAC__stream_encoder_set_apodization()
			bool set_max_lpc_order(unsigned value);                 ///< See OggFLAC__stream_encoder_set_max_lpc_order()
			bool set_qlp_coeff_precision(unsigned value);           ///< See OggFLAC__stream_encoder_set_qlp_coeff_precision()
			bool set_do_qlp_coeff_prec_search(bool value);          ///< See OggFLAC__stream_encoder_set_do_qlp_coeff_prec_search()
			bool set_do_escape_coding(bool value);                  ///< See OggFLAC__stream_encoder_set_do_escape_coding()
			bool set_do_exhaustive_model_search(bool value);        ///< See OggFLAC__stream_encoder_set_do_exhaustive_model_search()
			bool set_min_residual_partition_order(unsigned value);  ///< See OggFLAC__stream_encoder_set_min_residual_partition_order()
			bool set_max_residual_partition_order(unsigned value);  ///< See OggFLAC__stream_encoder_set_max_residual_partition_order()
			bool set_rice_parameter_search_dist(unsigned value);    ///< See OggFLAC__stream_encoder_set_rice_parameter_search_dist()
			bool set_total_samples_estimate(FLAC__uint64 value);    ///< See OggFLAC__stream_encoder_set_total_samples_estimate()
			bool set_metadata(::FLAC__StreamMetadata **metadata, unsigned num_blocks);     ///< See OggFLAC__stream_encoder_set_metadata()
			bool set_metadata(FLAC::Metadata::Prototype **metadata, unsigned num_blocks); ///< See OggFLAC__stream_encoder_set_metadata()

			State    get_state() const;                                         ///< See OggFLAC__stream_encoder_get_state()
			FLAC::Encoder::Stream::State get_FLAC_stream_encoder_state() const; ///< See OggFLAC__stream_encoder_get_FLAC_stream_encoder_state()
			FLAC::Decoder::Stream::State get_verify_decoder_state() const;      ///< See OggFLAC__stream_encoder_get_verify_decoder_state()
			void get_verify_decoder_error_stats(FLAC__uint64 *absolute_sample, unsigned *frame_number, unsigned *channel, unsigned *sample, FLAC__int32 *expected, FLAC__int32 *got); ///< See OggFLAC__stream_encoder_get_verify_decoder_error_stats()
			bool     get_verify() const;                       ///< See OggFLAC__stream_encoder_get_verify()
			bool     get_streamable_subset() const;            ///< See OggFLAC__stream_encoder_get_streamable_subset()
			bool     get_do_mid_side_stereo() const;           ///< See OggFLAC__stream_encoder_get_do_mid_side_stereo()
			bool     get_loose_mid_side_stereo() const;        ///< See OggFLAC__stream_encoder_get_loose_mid_side_stereo()
			unsigned get_channels() const;                     ///< See OggFLAC__stream_encoder_get_channels()
			unsigned get_bits_per_sample() const;              ///< See OggFLAC__stream_encoder_get_bits_per_sample()
			unsigned get_sample_rate() const;                  ///< See OggFLAC__stream_encoder_get_sample_rate()
			unsigned get_blocksize() const;                    ///< See OggFLAC__stream_encoder_get_blocksize()
			unsigned get_max_lpc_order() const;                ///< See OggFLAC__stream_encoder_get_max_lpc_order()
			unsigned get_qlp_coeff_precision() const;          ///< See OggFLAC__stream_encoder_get_qlp_coeff_precision()
			bool     get_do_qlp_coeff_prec_search() const;     ///< See OggFLAC__stream_encoder_get_do_qlp_coeff_prec_search()
			bool     get_do_escape_coding() const;             ///< See OggFLAC__stream_encoder_get_do_escape_coding()
			bool     get_do_exhaustive_model_search() const;   ///< See OggFLAC__stream_encoder_get_do_exhaustive_model_search()
			unsigned get_min_residual_partition_order() const; ///< See OggFLAC__stream_encoder_get_min_residual_partition_order()
			unsigned get_max_residual_partition_order() const; ///< See OggFLAC__stream_encoder_get_max_residual_partition_order()
			unsigned get_rice_parameter_search_dist() const;   ///< See OggFLAC__stream_encoder_get_rice_parameter_search_dist()
			FLAC__uint64 get_total_samples_estimate() const;   ///< See OggFLAC__stream_encoder_get_total_samples_estimate()

			/** Initialize the instance; as with the C interface,
			 *  init() should be called after construction and 'set'
			 *  calls but before any of the 'process' calls.
			 *
			 *  See OggFLAC__stream_encoder_init_stream().
			 */
			::FLAC__StreamEncoderInitStatus init();

			void finish(); ///< See OggFLAC__stream_encoder_finish()

			bool process(const FLAC__int32 * const buffer[], unsigned samples);     ///< See OggFLAC__stream_encoder_process()
			bool process_interleaved(const FLAC__int32 buffer[], unsigned samples); ///< See OggFLAC__stream_encoder_process_interleaved()
		protected:
			/// See OggFLAC__StreamEncoderReadCallback
			virtual ::OggFLAC__StreamEncoderReadStatus read_callback(FLAC__byte buffer[], unsigned *bytes);

#if (defined _MSC_VER) || (defined __GNUG__ && (__GNUG__ < 2 || (__GNUG__ == 2 && __GNUC_MINOR__ < 96))) || (defined __SUNPRO_CC)
			// lame hack: some MSVC/GCC versions can't see a protected encoder_ from nested State::resolved_as_cstring()
			friend State;
#endif
			static ::OggFLAC__StreamEncoderReadStatus read_callback_(const ::OggFLAC__StreamEncoder *encoder, FLAC__byte buffer[], unsigned *bytes, void *client_data);
		private:
			// Private and undefined so you can't use them:
			Stream(const Stream &);
			void operator=(const Stream &);
		};

		/* \} */

		/** \defgroup oggflacpp_file_encoder OggFLAC++/encoder.h: file encoder class
		 *  \ingroup oggflacpp_encoder
		 *
		 *  \brief
		 *  This class wraps the ::OggFLAC__FileEncoder.
		 *
		 * See the \link oggflac_file_encoder libOggFLAC file encoder module \endlink
		 * for basic usage.
		 *
		 * \{
		 */

		/** This class wraps the ::OggFLAC__StreamEncoder.  If you are
		 *  not encoding to a file, you may need to use
		 *  OggFLAC::Encoder::Stream.
		 *
		 * The usage of this class is similar to OggFLAC__StreamEncoder,
		 * except instead of providing callbacks to
		 * OggFLAC__stream_encoder_init_FILE() or
		 * OggFLAC__stream_encoder_init_file(), you will inherit from this
		 * class and override the virtual callback functions with your
		 * own implementations, then call File::init().  The rest of
		 * the calls work the same as in the C layer.
		 *
		 * There are no mandatory callbacks; all the callbacks from
		 * OggFLAC::Encoder::Stream are implemented here fully and support
		 * full post-encode STREAMINFO and SEEKTABLE updating.  There is
		 * only an optional progress callback which you may override to
		 * get periodic reports on the progress of the encode.
		 */
		class OggFLACPP_API File: public Stream {
		public:
			/** Initialize the instance; as with the C interface,
			 *  init() should be called after construction and 'set'
			 *  calls but before any of the 'process' calls.
			 *
			 *  See OggFLAC__stream_encoder_init_FILE() and
			 *  OggFLAC__stream_encoder_init_file().
			 *  \{
			 */
			File();
			virtual ~File();
			/*  \} */

			::FLAC__StreamEncoderInitStatus init(FILE *file);
			::FLAC__StreamEncoderInitStatus init(const char *filename);
			::FLAC__StreamEncoderInitStatus init(const std::string &filename);
		protected:
			/// See FLAC__StreamEncoderProgressCallback
			virtual void progress_callback(FLAC__uint64 bytes_written, FLAC__uint64 samples_written, unsigned frames_written, unsigned total_frames_estimate);

			/// This is a dummy implementation to satisfy the pure virtual in Stream that is actually supplied internally by the C layer
			virtual ::FLAC__StreamEncoderWriteStatus write_callback(const FLAC__byte buffer[], unsigned bytes, unsigned samples, unsigned current_frame);
		private:
			static void progress_callback_(const ::FLAC__StreamEncoder *encoder, FLAC__uint64 bytes_written, FLAC__uint64 samples_written, unsigned frames_written, unsigned total_frames_estimate, void *client_data);

			// Private and undefined so you can't use them:
			File(const Stream &);
			void operator=(const Stream &);
		};

		/* \} */

	}
}

#endif
