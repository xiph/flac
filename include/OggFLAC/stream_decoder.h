/* libOggFLAC - Free Lossless Audio Codec + Ogg library
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

#ifndef OggFLAC__STREAM_DECODER_H
#define OggFLAC__STREAM_DECODER_H

#include <stdio.h> /* for FILE */
#include "export.h"
#include "FLAC/stream_decoder.h"

#ifdef __cplusplus
extern "C" {
#endif


/** \file include/OggFLAC/stream_decoder.h
 *
 *  \brief
 *  This module contains the functions which implement the stream
 *  decoder.
 *
 *  See the detailed documentation in the
 *  \link oggflac_stream_decoder stream decoder \endlink module.
 */

/** \defgroup oggflac_decoder OggFLAC/ *_decoder.h: decoder interfaces
 *  \ingroup oggflac
 *
 *  \brief
 *  This module describes the three decoder layers provided by libOggFLAC.
 *
 * libOggFLAC currently provides the same three layers of access as
 * libFLAC; the interfaces are nearly identical, with th addition of a
 * method for specifying the Ogg serial number.  See the
 * \link flac_decoder FLAC decoder module \endlink for full documentation.
 */

/** \defgroup oggflac_stream_decoder OggFLAC/stream_decoder.h: stream decoder interface
 *  \ingroup oggflac_decoder
 *
 *  \brief
 *  This module contains the functions which implement the stream
 *  decoder.
 *
 * The interface here is nearly identical to FLAC's stream decoder,
 * including the callbacks, with the addition of
 * OggFLAC__stream_decoder_set_serial_number().  See the
 * \link flac_stream_decoder FLAC stream decoder module \endlink
 * for full documentation.
 *
 * \{
 */


/** State values for an OggFLAC__StreamDecoder
 *
 *  The decoder's state can be obtained by calling OggFLAC__stream_decoder_get_state().
 */
typedef enum {

	OggFLAC__STREAM_DECODER_OK = 0,
	/**< The decoder is in the normal OK state. */

	OggFLAC__STREAM_DECODER_END_OF_STREAM,
	/**< The decoder has reached the end of the stream. */

	OggFLAC__STREAM_DECODER_OGG_ERROR,
	/**< An error occurred in the underlying Ogg layer.  */

	OggFLAC__STREAM_DECODER_FLAC_STREAM_DECODER_ERROR,
	/**< An error occurred in the underlying FLAC stream decoder;
	 * check OggFLAC__stream_decoder_get_FLAC_stream_decoder_state().
	 */

	OggFLAC__STREAM_DECODER_SEEK_ERROR,
	/**< An error occurred while seeking or the seek or tell
	 * callback returned an error.  The decoder must be flushed with
	 * OggFLAC__stream_decoder_flush() or reset with
	 * OggFLAC__stream_decoder_reset() before decoding can continue.
	 */

	OggFLAC__STREAM_DECODER_READ_ERROR,
	/**< The read callback returned an error. */

	OggFLAC__STREAM_DECODER_MEMORY_ALLOCATION_ERROR,
	/**< An error occurred allocating memory.  The decoder is in an invalid
	 * state and can no longer be used.
	 */

	OggFLAC__STREAM_DECODER_UNINITIALIZED
	/**< The decoder is in the uninitialized state; one of the
	 * OggFLAC__stream_decoder_init_*() functions must be called before samples
	 * can be processed.
	 */

} OggFLAC__StreamDecoderState;

/** Maps an OggFLAC__StreamDecoderState to a C string.
 *
 *  Using an OggFLAC__StreamDecoderState as the index to this array
 *  will give the string equivalent.  The contents should not be modified.
 */
extern OggFLAC_API const char * const OggFLAC__StreamDecoderStateString[];


/***********************************************************************
 *
 * class OggFLAC__StreamDecoder
 *
 ***********************************************************************/

struct OggFLAC__StreamDecoderProtected;
struct OggFLAC__StreamDecoderPrivate;
/** The opaque structure definition for the stream decoder type.
 *  See the \link oggflac_stream_decoder stream decoder module \endlink
 *  for a detailed description.
 */
typedef struct {
	FLAC__StreamDecoder super_; /* parentclass@@@@@@ */
	struct OggFLAC__StreamDecoderProtected *protected_; /* avoid the C++ keyword 'protected' */
	struct OggFLAC__StreamDecoderPrivate *private_; /* avoid the C++ keyword 'private' */
} OggFLAC__StreamDecoder;


/***********************************************************************
 *
 * Class constructor/destructor
 *
 ***********************************************************************/

/** Create a new stream decoder instance.  The instance is created with
 *  default settings; see the individual OggFLAC__stream_decoder_set_*()
 *  functions for each setting's default.
 *
 * \retval OggFLAC__StreamDecoder*
 *    \c NULL if there was an error allocating memory, else the new instance.
 */
OggFLAC_API OggFLAC__StreamDecoder *OggFLAC__stream_decoder_new();

/** Free a decoder instance.  Deletes the object pointed to by \a decoder.
 *
 * \param decoder  A pointer to an existing decoder.
 * \assert
 *    \code decoder != NULL \endcode
 */
OggFLAC_API void OggFLAC__stream_decoder_delete(OggFLAC__StreamDecoder *decoder);


/***********************************************************************
 *
 * Public class method prototypes
 *
 ***********************************************************************/

/** Set the serial number for the Ogg stream.
 * The default behavior is to use the serial number of the first Ogg
 * page.  Setting a serial number here will explicitly specify which
 * stream is to be decoded.
 *
 * \default \c use serial number of first page
 * \param  decoder        A decoder instance to set.
 * \param  serial_number  See above.
 * \assert
 *    \code decoder != NULL \endcode
 * \retval FLAC__bool
 *    \c false if the decoder is already initialized, else \c true.
 */
OggFLAC_API FLAC__bool OggFLAC__stream_decoder_set_serial_number(OggFLAC__StreamDecoder *decoder, long serial_number);

/** Set the "MD5 signature checking" flag.
 * This is inherited from FLAC__StreamDecoder; see FLAC__stream_decoder_set_md5_checking()
 *
 * \default \c false
 * \param  decoder  A decoder instance to set.
 * \param  value    Flag value (see above).
 * \assert
 *    \code decoder != NULL \endcode
 * \retval FLAC__bool
 *    \c false if the decoder is already initialized, else \c true.
 */
OggFLAC_API FLAC__bool OggFLAC__stream_decoder_set_md5_checking(OggFLAC__StreamDecoder *decoder, FLAC__bool value);

/** Direct the decoder to pass on all metadata blocks of type \a type.
 * This is inherited from FLAC__StreamDecoder; see FLAC__stream_decoder_set_metadata_respond()
 *
 * \default By default, only the \c STREAMINFO block is returned via the
 *          metadata callback.
 * \param  decoder  A decoder instance to set.
 * \param  type     See above.
 * \assert
 *    \code decoder != NULL \endcode
 *    \a type is valid
 * \retval FLAC__bool
 *    \c false if the decoder is already initialized, else \c true.
 */
OggFLAC_API FLAC__bool OggFLAC__stream_decoder_set_metadata_respond(OggFLAC__StreamDecoder *decoder, FLAC__MetadataType type);

/** Direct the decoder to pass on all APPLICATION metadata blocks of the
 *  given \a id.
 * This is inherited from FLAC__StreamDecoder; see FLAC__stream_decoder_set_metadata_respond_application()
 *
 * \default By default, only the \c STREAMINFO block is returned via the
 *          metadata callback.
 * \param  decoder  A decoder instance to set.
 * \param  id       See above.
 * \assert
 *    \code decoder != NULL \endcode
 *    \code id != NULL \endcode
 * \retval FLAC__bool
 *    \c false if the decoder is already initialized, else \c true.
 */
OggFLAC_API FLAC__bool OggFLAC__stream_decoder_set_metadata_respond_application(OggFLAC__StreamDecoder *decoder, const FLAC__byte id[4]);

/** Direct the decoder to pass on all metadata blocks of any type.
 * This is inherited from FLAC__StreamDecoder; see FLAC__stream_decoder_set_metadata_respond_all()
 *
 * \default By default, only the \c STREAMINFO block is returned via the
 *          metadata callback.
 * \param  decoder  A decoder instance to set.
 * \assert
 *    \code decoder != NULL \endcode
 * \retval FLAC__bool
 *    \c false if the decoder is already initialized, else \c true.
 */
OggFLAC_API FLAC__bool OggFLAC__stream_decoder_set_metadata_respond_all(OggFLAC__StreamDecoder *decoder);

/** Direct the decoder to filter out all metadata blocks of type \a type.
 * This is inherited from FLAC__StreamDecoder; see FLAC__stream_decoder_set_metadata_ignore()
 *
 * \default By default, only the \c STREAMINFO block is returned via the
 *          metadata callback.
 * \param  decoder  A decoder instance to set.
 * \param  type     See above.
 * \assert
 *    \code decoder != NULL \endcode
 *    \a type is valid
 * \retval FLAC__bool
 *    \c false if the decoder is already initialized, else \c true.
 */
OggFLAC_API FLAC__bool OggFLAC__stream_decoder_set_metadata_ignore(OggFLAC__StreamDecoder *decoder, FLAC__MetadataType type);

/** Direct the decoder to filter out all APPLICATION metadata blocks of
 *  the given \a id.
 * This is inherited from FLAC__StreamDecoder; see FLAC__stream_decoder_set_metadata_ignore_application()
 *
 * \default By default, only the \c STREAMINFO block is returned via the
 *          metadata callback.
 * \param  decoder  A decoder instance to set.
 * \param  id       See above.
 * \assert
 *    \code decoder != NULL \endcode
 *    \code id != NULL \endcode
 * \retval FLAC__bool
 *    \c false if the decoder is already initialized, else \c true.
 */
OggFLAC_API FLAC__bool OggFLAC__stream_decoder_set_metadata_ignore_application(OggFLAC__StreamDecoder *decoder, const FLAC__byte id[4]);

/** Direct the decoder to filter out all metadata blocks of any type.
 * This is inherited from FLAC__StreamDecoder; see FLAC__stream_decoder_set_metadata_ignore_all()
 *
 * \default By default, only the \c STREAMINFO block is returned via the
 *          metadata callback.
 * \param  decoder  A decoder instance to set.
 * \assert
 *    \code decoder != NULL \endcode
 * \retval FLAC__bool
 *    \c false if the decoder is already initialized, else \c true.
 */
OggFLAC_API FLAC__bool OggFLAC__stream_decoder_set_metadata_ignore_all(OggFLAC__StreamDecoder *decoder);

/** Get the current decoder state.
 *
 * \param  decoder  A decoder instance to query.
 * \assert
 *    \code decoder != NULL \endcode
 * \retval OggFLAC__StreamDecoderState
 *    The current decoder state.
 */
OggFLAC_API OggFLAC__StreamDecoderState OggFLAC__stream_decoder_get_state(const OggFLAC__StreamDecoder *decoder);

/** Get the state of the underlying FLAC stream decoder.
 *  Useful when the stream decoder state is
 *  \c OggFLAC__STREAM_DECODER_FLAC_STREAM_DECODER_ERROR.
 *
 * \param  decoder  A decoder instance to query.
 * \assert
 *    \code decoder != NULL \endcode
 * \retval FLAC__StreamDecoderState
 *    The FLAC stream decoder state.
 */
OggFLAC_API FLAC__StreamDecoderState OggFLAC__stream_decoder_get_FLAC_stream_decoder_state(const OggFLAC__StreamDecoder *decoder);

/** Get the current decoder state as a C string.
 *  This version automatically resolves
 *  \c OggFLAC__STREAM_DECODER_FLAC_STREAM_DECODER_ERROR
 *  by getting the FLAC stream decoder's state.
 *
 * \param  decoder  A decoder instance to query.
 * \assert
 *    \code decoder != NULL \endcode
 * \retval const char *
 *    The decoder state as a C string.  Do not modify the contents.
 */
OggFLAC_API const char *OggFLAC__stream_decoder_get_resolved_state_string(const OggFLAC__StreamDecoder *decoder);

/** This is inherited from FLAC__StreamDecoder; see FLAC__stream_decoder_get_md5_checking()
 *
 * \param  decoder  A decoder instance to query.
 * \assert
 *    \code decoder != NULL \endcode
 * \retval FLAC__bool
 *    See above.
 */
OggFLAC_API FLAC__bool OggFLAC__stream_decoder_get_md5_checking(const OggFLAC__StreamDecoder *decoder);

/** This is inherited from FLAC__StreamDecoder; see FLAC__stream_decoder_get_total_samples()
 *
 * \param  decoder  A decoder instance to query.
 * \assert
 *    \code decoder != NULL \endcode
 * \retval unsigned
 *    See above.
 */
OggFLAC_API FLAC__uint64 OggFLAC__stream_decoder_get_total_samples(const OggFLAC__StreamDecoder *decoder);

/** This is inherited from FLAC__StreamDecoder; see FLAC__stream_decoder_get_channels()
 *
 * \param  decoder  A decoder instance to query.
 * \assert
 *    \code decoder != NULL \endcode
 * \retval unsigned
 *    See above.
 */
OggFLAC_API unsigned OggFLAC__stream_decoder_get_channels(const OggFLAC__StreamDecoder *decoder);

/** This is inherited from FLAC__StreamDecoder; see FLAC__stream_decoder_get_channel_assignment()
 *
 * \param  decoder  A decoder instance to query.
 * \assert
 *    \code decoder != NULL \endcode
 * \retval OggFLAC__ChannelAssignment
 *    See above.
 */
OggFLAC_API FLAC__ChannelAssignment OggFLAC__stream_decoder_get_channel_assignment(const OggFLAC__StreamDecoder *decoder);

/** This is inherited from FLAC__StreamDecoder; see FLAC__stream_decoder_get_bits_per_sample()
 *
 * \param  decoder  A decoder instance to query.
 * \assert
 *    \code decoder != NULL \endcode
 * \retval unsigned
 *    See above.
 */
OggFLAC_API unsigned OggFLAC__stream_decoder_get_bits_per_sample(const OggFLAC__StreamDecoder *decoder);

/** This is inherited from FLAC__StreamDecoder; see FLAC__stream_decoder_get_sample_rate()
 *
 * \param  decoder  A decoder instance to query.
 * \assert
 *    \code decoder != NULL \endcode
 * \retval unsigned
 *    See above.
 */
OggFLAC_API unsigned OggFLAC__stream_decoder_get_sample_rate(const OggFLAC__StreamDecoder *decoder);

/** This is inherited from FLAC__StreamDecoder; see FLAC__stream_decoder_get_blocksize()
 *
 * \param  decoder  A decoder instance to query.
 * \assert
 *    \code decoder != NULL \endcode
 * \retval unsigned
 *    See above.
 */
OggFLAC_API unsigned OggFLAC__stream_decoder_get_blocksize(const OggFLAC__StreamDecoder *decoder);

/** Initialize the decoder instance.
 *
 *  This flavor of initialization sets up the decoder to decode from a stream.
 *  I/O is performed via callbacks to the client.  For decoding from a plain file
 *  via filename or open FILE*, OggFLAC__stream_decoder_init_file() and
 *  OggFLAC__stream_decoder_init_FILE() provide a simpler interface.
 *
 *  This function should be called after OggFLAC__stream_decoder_new() and
 *  OggFLAC__stream_decoder_set_*() but before any of the
 *  OggFLAC__stream_decoder_process_*() functions.  Will set and return the
 *  decoder state, which will be OggFLAC__STREAM_DECODER_OK
 *  if initialization succeeded.
 *
 * \param  decoder            An uninitialized decoder instance.
 * \param  read_callback      See FLAC__StreamDecoderReadCallback.  This
 *                            pointer must not be \c NULL.
 * \param  seek_callback      See FLAC__StreamDecoderSeekCallback.  This
 *                            pointer may be \c NULL if seeking is not
 *                            supported.  If \a seek_callback is not \c NULL then a
 *                            \a tell_callback, \a length_callback, and \a eof_callback must also be supplied.
 *                            Alternatively, a dummy seek callback that just
 *                            returns \c FLAC__STREAM_DECODER_SEEK_STATUS_UNSUPPORTED
 *                            may also be supplied, all though this is slightly
 *                            less efficient for the decoder.
 * \param  tell_callback      See FLAC__StreamDecoderTellCallback.  This
 *                            pointer may be \c NULL if not supported by the client.  If
 *                            \a seek_callback is not \c NULL then a
 *                            \a tell_callback must also be supplied.
 *                            Alternatively, a dummy tell callback that just
 *                            returns \c FLAC__STREAM_DECODER_TELL_STATUS_UNSUPPORTED
 *                            may also be supplied, all though this is slightly
 *                            less efficient for the decoder.
 * \param  length_callback    See FLAC__StreamDecoderLengthCallback.  This
 *                            pointer may be \c NULL if not supported by the client.  If
 *                            \a seek_callback is not \c NULL then a
 *                            \a length_callback must also be supplied.
 *                            Alternatively, a dummy length callback that just
 *                            returns \c FLAC__STREAM_DECODER_LENGTH_STATUS_UNSUPPORTED
 *                            may also be supplied, all though this is slightly
 *                            less efficient for the decoder.
 * \param  eof_callback       See FLAC__StreamDecoderEofCallback.  This
 *                            pointer may be \c NULL if not supported by the client.  If
 *                            \a seek_callback is not \c NULL then a
 *                            \a eof_callback must also be supplied.
 *                            Alternatively, a dummy length callback that just
 *                            returns \c false
 *                            may also be supplied, all though this is slightly
 *                            less efficient for the decoder.
 * \param  write_callback     See FLAC__StreamDecoderWriteCallback.  This
 *                            pointer must not be \c NULL.
 * \param  metadata_callback  See FLAC__StreamDecoderMetadataCallback.  This
 *                            pointer may be \c NULL if the callback is not
 *                            desired.
 * \param  error_callback     See FLAC__StreamDecoderErrorCallback.  This
 *                            pointer must not be \c NULL.
 * \param  client_data        This value will be supplied to callbacks in their
 *                            \a client_data argument.
 * \assert
 *    \code decoder != NULL \endcode
 * \retval FLAC__StreamDecoderInitStatus
 *    \c FLAC__STREAM_DECODER_INIT_STATUS_OK if initialization was successful;
 *    see FLAC__StreamDecoderInitStatus for the meanings of other return values.
 */
OggFLAC_API FLAC__StreamDecoderInitStatus OggFLAC__stream_decoder_init_stream(
	OggFLAC__StreamDecoder *decoder,
	FLAC__StreamDecoderReadCallback read_callback,
	FLAC__StreamDecoderSeekCallback seek_callback,
	FLAC__StreamDecoderTellCallback tell_callback,
	FLAC__StreamDecoderLengthCallback length_callback,
	FLAC__StreamDecoderEofCallback eof_callback,
	FLAC__StreamDecoderWriteCallback write_callback,
	FLAC__StreamDecoderMetadataCallback metadata_callback,
	FLAC__StreamDecoderErrorCallback error_callback,
	void *client_data
);

/** Initialize the decoder instance.
 *
 *  This flavor of initialization sets up the decoder to decode from a plain
 *  file.  For non-stdio streams, you must use
 *  OggFLAC__stream_decoder_init_stream() and provide callbacks for the I/O.
 *
 *  This function should be called after OggFLAC__stream_decoder_new() and
 *  OggFLAC__stream_decoder_set_*() but before any of the
 *  OggFLAC__stream_decoder_process_*() functions.  Will set and return the
 *  decoder state, which will be OggFLAC__STREAM_DECODER_OK
 *  if initialization succeeded.
 *
 * \param  decoder            An uninitialized decoder instance.
 * \param  file               An open Ogg FLAC file.  The file should have been
 *                            opened with mode \c "rb" and rewound.  The file
 *                            becomes owned by the decoder and should not be
 *                            manipulated by the client while decoding.
 *                            Unless \a file is \c stdin, it will be closed
 *                            when OggFLAC__stream_decoder_finish() is called.
 *                            Note however that seeking will not work when
 *                            decoding from \c stdout since it is not seekable.
 * \param  write_callback     See FLAC__StreamDecoderWriteCallback.  This
 *                            pointer must not be \c NULL.
 * \param  metadata_callback  See FLAC__StreamDecoderMetadataCallback.  This
 *                            pointer may be \c NULL if the callback is not
 *                            desired.
 * \param  error_callback     See FLAC__StreamDecoderErrorCallback.  This
 *                            pointer must not be \c NULL.
 * \param  client_data        This value will be supplied to callbacks in their
 *                            \a client_data argument.
 * \assert
 *    \code decoder != NULL \endcode
 *    \code file != NULL \endcode
 * \retval FLAC__StreamDecoderInitStatus
 *    \c FLAC__STREAM_DECODER_INIT_STATUS_OK if initialization was successful;
 *    see FLAC__StreamDecoderInitStatus for the meanings of other return values.
 */
OggFLAC_API FLAC__StreamDecoderInitStatus OggFLAC__stream_decoder_init_FILE(
	OggFLAC__StreamDecoder *decoder,
	FILE *file,
	FLAC__StreamDecoderWriteCallback write_callback,
	FLAC__StreamDecoderMetadataCallback metadata_callback,
	FLAC__StreamDecoderErrorCallback error_callback,
	void *client_data
);

/** Initialize the decoder instance.
 *
 *  This flavor of initialization sets up the decoder to decode from a plain
 *  file.  If POSIX fopen() semantics are not sufficient, (for example, with
 *  Unicode filenames on Windows), you must use
 *  OggFLAC__stream_decoder_init_FILE(), or OggFLAC__stream_decoder_init_stream()
 *  and provide callbacks for the I/O.
 *
 *  This function should be called after OggFLAC__stream_decoder_new() and
 *  OggFLAC__stream_decoder_set_*() but before any of the
 *  OggFLAC__stream_decoder_process_*() functions.  Will set and return the
 *  decoder state, which will be OggFLAC__STREAM_DECODER_OK
 *  if initialization succeeded.
 *
 * \param  decoder            An uninitialized decoder instance.
 * \param  filename           The name of the file to decode from.  The file will
 *                            be opened with fopen().  Use \c NULL to decode from
 *                            \c stdin.  Note that \c stdin is not seekable.
 * \param  write_callback     See FLAC__StreamDecoderWriteCallback.  This
 *                            pointer must not be \c NULL.
 * \param  metadata_callback  See FLAC__StreamDecoderMetadataCallback.  This
 *                            pointer may be \c NULL if the callback is not
 *                            desired.
 * \param  error_callback     See FLAC__StreamDecoderErrorCallback.  This
 *                            pointer must not be \c NULL.
 * \param  client_data        This value will be supplied to callbacks in their
 *                            \a client_data argument.
 * \assert
 *    \code decoder != NULL \endcode
 * \retval FLAC__StreamDecoderInitStatus
 *    \c FLAC__STREAM_DECODER_INIT_STATUS_OK if initialization was successful;
 *    see FLAC__StreamDecoderInitStatus for the meanings of other return values.
 */
OggFLAC_API FLAC__StreamDecoderInitStatus OggFLAC__stream_decoder_init_file(
	OggFLAC__StreamDecoder *decoder,
	const char *filename,
	FLAC__StreamDecoderWriteCallback write_callback,
	FLAC__StreamDecoderMetadataCallback metadata_callback,
	FLAC__StreamDecoderErrorCallback error_callback,
	void *client_data
);

/** Finish the decoding process.
 *  Flushes the decoding buffer, releases resources, resets the decoder
 *  settings to their defaults, and returns the decoder state to
 *  OggFLAC__STREAM_DECODER_UNINITIALIZED.
 *
 *  In the event of a prematurely-terminated decode, it is not strictly
 *  necessary to call this immediately before OggFLAC__stream_decoder_delete()
 *  but it is good practice to match every OggFLAC__stream_decoder_init()
 *  with an OggFLAC__stream_decoder_finish().
 *
 * \param  decoder  An uninitialized decoder instance.
 * \assert
 *    \code decoder != NULL \endcode
 */
OggFLAC_API FLAC__bool OggFLAC__stream_decoder_finish(OggFLAC__StreamDecoder *decoder);

/** This is inherited from FLAC__StreamDecoder; see FLAC__stream_decoder_flush()
 *
 * \param  decoder  A decoder instance.
 * \assert
 *    \code decoder != NULL \endcode
 * \retval FLAC__bool
 *    \c true if successful, else \c false if a memory allocation
 *    error occurs.
 */
OggFLAC_API FLAC__bool OggFLAC__stream_decoder_flush(OggFLAC__StreamDecoder *decoder);

/** This is inherited from FLAC__StreamDecoder; see FLAC__stream_decoder_reset()
 *
 * \param  decoder  A decoder instance.
 * \assert
 *    \code decoder != NULL \endcode
 * \retval FLAC__bool
 *    \c true if successful, else \c false if a memory allocation
 *    or seek error occurs.
 */
OggFLAC_API FLAC__bool OggFLAC__stream_decoder_reset(OggFLAC__StreamDecoder *decoder);

/** Decode one metadata block or audio frame.
 *  This is inherited from FLAC__StreamDecoder; see FLAC__stream_decoder_process_single()
 *
 * \param  decoder  An initialized decoder instance in the state
 *                  \c OggFLAC__STREAM_DECODER_OK.
 * \assert
 *    \code decoder != NULL \endcode
 *    \code OggFLAC__stream_decoder_get_state(decoder) == OggFLAC__STREAM_DECODER_OK \endcode
 * \retval FLAC__bool
 *    \c false if any fatal read, write, or memory allocation error
 *    occurred (meaning decoding must stop), else \c true; for more
 *    information about the decoder, check the decoder state with
 *    OggFLAC__stream_decoder_get_state().
 */
OggFLAC_API FLAC__bool OggFLAC__stream_decoder_process_single(OggFLAC__StreamDecoder *decoder);

/** Decode until the end of the metadata.
 *  This is inherited from FLAC__StreamDecoder; see FLAC__stream_decoder_process_until_end_of_metadata()
 *
 * \param  decoder  An initialized decoder instance in the state
 *                  \c OggFLAC__STREAM_DECODER_OK.
 * \assert
 *    \code decoder != NULL \endcode
 *    \code OggFLAC__stream_decoder_get_state(decoder) == OggFLAC__STREAM_DECODER_OK \endcode
 * \retval FLAC__bool
 *    \c false if any fatal read, write, or memory allocation error
 *    occurred (meaning decoding must stop), else \c true; for more
 *    information about the decoder, check the decoder state with
 *    OggFLAC__stream_decoder_get_state().
 */
OggFLAC_API FLAC__bool OggFLAC__stream_decoder_process_until_end_of_metadata(OggFLAC__StreamDecoder *decoder);

/** Decode until the end of the stream.
 *  This is inherited from FLAC__StreamDecoder; see FLAC__stream_decoder_process_until_end_of_stream()
 *
 * \param  decoder  An initialized decoder instance in the state
 *                  \c OggFLAC__STREAM_DECODER_OK.
 * \assert
 *    \code decoder != NULL \endcode
 *    \code OggFLAC__stream_decoder_get_state(decoder) == OggFLAC__STREAM_DECODER_OK \endcode
 * \retval FLAC__bool
 *    \c false if any fatal read, write, or memory allocation error
 *    occurred (meaning decoding must stop), else \c true; for more
 *    information about the decoder, check the decoder state with
 *    OggFLAC__stream_decoder_get_state().
 */
OggFLAC_API FLAC__bool OggFLAC__stream_decoder_process_until_end_of_stream(OggFLAC__StreamDecoder *decoder);

/** Skip one audio frame.
 *  This is inherited from FLAC__StreamDecoder; see FLAC__stream_decoder_skip_single_frame()
 *
 * \param  decoder  An initialized decoder instance not in a metadata
 *                  state.
 * \assert
 *    \code decoder != NULL \endcode
 * \retval FLAC__bool
 *    \c false if any fatal read, write, or memory allocation error
 *    occurred (meaning decoding must stop), or if the underlying FLAC
 *    stream decoder is in the FLAC__STREAM_DECODER_SEARCH_FOR_METADATA or
 *    FLAC__STREAM_DECODER_READ_METADATA state, else \c true; for more
 *    information about the decoder, check the decoder state with
 *    OggFLAC__stream_decoder_get_FLAC_stream_decoder_state().
 */
OggFLAC_API FLAC__bool OggFLAC__stream_decoder_skip_single_frame(OggFLAC__StreamDecoder *decoder);

/** Flush the input and seek to an absolute sample.
 *  This is inherited from FLAC__StreamDecoder; see FLAC__stream_decoder_seek_absolute().
 *
 * \param  decoder  A decoder instance.
 * \param  sample   The target sample number to seek to.
 * \assert
 *    \code decoder != NULL \endcode
 * \retval FLAC__bool
 *    \c true if successful, else \c false.
 */
OggFLAC_API FLAC__bool OggFLAC__stream_decoder_seek_absolute(OggFLAC__StreamDecoder *decoder, FLAC__uint64 sample);

/* \} */

#ifdef __cplusplus
}
#endif

#endif
