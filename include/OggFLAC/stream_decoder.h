/* libOggFLAC - Free Lossless Audio Codec + Ogg library
 * Copyright (C) 2002,2003,2004,2005  Josh Coalson
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

	OggFLAC__STREAM_DECODER_READ_ERROR,
	/**< The read callback returned an error. */

	OggFLAC__STREAM_DECODER_FLAC_STREAM_DECODER_ERROR,
	/**< An error occurred in the underlying FLAC stream decoder;
	 * check OggFLAC__stream_decoder_get_FLAC_stream_decoder_state().
	 */

	OggFLAC__STREAM_DECODER_MEMORY_ALLOCATION_ERROR,
	/**< Memory allocation failed. */

	OggFLAC__STREAM_DECODER_ALREADY_INITIALIZED,
	/**< OggFLAC__stream_decoder_init() was called when the decoder was
	 * already initialized, usually because
	 * OggFLAC__stream_decoder_finish() was not called.
	 */

	OggFLAC__STREAM_DECODER_INVALID_CALLBACK,
	/**< The decoder was initialized before setting all the required callbacks. */

	OggFLAC__STREAM_DECODER_UNINITIALIZED
	/**< The decoder is in the uninitialized state. */

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
	struct OggFLAC__StreamDecoderProtected *protected_; /* avoid the C++ keyword 'protected' */
	struct OggFLAC__StreamDecoderPrivate *private_; /* avoid the C++ keyword 'private' */
} OggFLAC__StreamDecoder;

/** Signature for the read callback.
 *  See OggFLAC__stream_decoder_set_read_callback()
 *  and FLAC__StreamDecoderReadCallback for more info.
 *
 * \param  decoder  The decoder instance calling the callback.
 * \param  buffer   A pointer to a location for the callee to store
 *                  data to be decoded.
 * \param  bytes    A pointer to the size of the buffer.  On entry
 *                  to the callback, it contains the maximum number
 *                  of bytes that may be stored in \a buffer.  The
 *                  callee must set it to the actual number of bytes
 *                  stored before returning.
 * \param  client_data  The callee's client data set through
 *                      OggFLAC__stream_decoder_set_client_data().
 * \retval FLAC__StreamDecoderReadStatus
 *    The callee's return status.
 */
typedef FLAC__StreamDecoderReadStatus (*OggFLAC__StreamDecoderReadCallback)(const OggFLAC__StreamDecoder *decoder, FLAC__byte buffer[], unsigned *bytes, void *client_data);

/** Signature for the write callback.
 *  See OggFLAC__stream_decoder_set_write_callback()
 *  and FLAC__StreamDecoderWriteCallback for more info.
 *
 * \param  decoder  The decoder instance calling the callback.
 * \param  frame    The description of the decoded frame.  See
 *                  FLAC__Frame.
 * \param  buffer   An array of pointers to decoded channels of data.
 *                  Each pointer will point to an array of signed
 *                  samples of length \a frame->header.blocksize.
 *                  Currently, the channel order has no meaning
 *                  except for stereo streams; in this case channel
 *                  0 is left and 1 is right.
 * \param  client_data  The callee's client data set through
 *                      OggFLAC__stream_decoder_set_client_data().
 * \retval FLAC__StreamDecoderWriteStatus
 *    The callee's return status.
 */
typedef FLAC__StreamDecoderWriteStatus (*OggFLAC__StreamDecoderWriteCallback)(const OggFLAC__StreamDecoder *decoder, const FLAC__Frame *frame, const FLAC__int32 * const buffer[], void *client_data);

/** Signature for the metadata callback.
 *  See OggFLAC__stream_decoder_set_metadata_callback()
 *  and FLAC__StreamDecoderMetadataCallback for more info.
 *
 * \param  decoder  The decoder instance calling the callback.
 * \param  metadata The decoded metadata block.
 * \param  client_data  The callee's client data set through
 *                      OggFLAC__stream_decoder_set_client_data().
 */
typedef void (*OggFLAC__StreamDecoderMetadataCallback)(const OggFLAC__StreamDecoder *decoder, const FLAC__StreamMetadata *metadata, void *client_data);

/** Signature for the error callback.
 *  See OggFLAC__stream_decoder_set_error_callback()
 *  and FLAC__StreamDecoderErrorCallback for more info.
 *
 * \param  decoder  The decoder instance calling the callback.
 * \param  status   The error encountered by the decoder.
 * \param  client_data  The callee's client data set through
 *                      OggFLAC__stream_decoder_set_client_data().
 */
typedef void (*OggFLAC__StreamDecoderErrorCallback)(const OggFLAC__StreamDecoder *decoder, FLAC__StreamDecoderErrorStatus status, void *client_data);


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

/** Set the read callback.
 * This is inherited from FLAC__StreamDecoder; see FLAC__stream_decoder_set_read_callback()
 *
 * \note
 * The callback is mandatory and must be set before initialization.
 *
 * \default \c NULL
 * \param  decoder  A decoder instance to set.
 * \param  value    See above.
 * \assert
 *    \code decoder != NULL \endcode
 *    \code value != NULL \endcode
 * \retval FLAC__bool
 *    \c false if the decoder is already initialized, else \c true.
 */
OggFLAC_API FLAC__bool OggFLAC__stream_decoder_set_read_callback(OggFLAC__StreamDecoder *decoder, OggFLAC__StreamDecoderReadCallback value);

/** Set the write callback.
 * This is inherited from FLAC__StreamDecoder; see FLAC__stream_decoder_set_write_callback()
 *
 * \note
 * The callback is mandatory and must be set before initialization.
 *
 * \default \c NULL
 * \param  decoder  A decoder instance to set.
 * \param  value    See above.
 * \assert
 *    \code decoder != NULL \endcode
 *    \code value != NULL \endcode
 * \retval FLAC__bool
 *    \c false if the decoder is already initialized, else \c true.
 */
OggFLAC_API FLAC__bool OggFLAC__stream_decoder_set_write_callback(OggFLAC__StreamDecoder *decoder, OggFLAC__StreamDecoderWriteCallback value);

/** Set the metadata callback.
 * This is inherited from FLAC__StreamDecoder; see FLAC__stream_decoder_set_metadata_callback()
 *
 * \note
 * The callback is mandatory and must be set before initialization.
 *
 * \default \c NULL
 * \param  decoder  A decoder instance to set.
 * \param  value    See above.
 * \assert
 *    \code decoder != NULL \endcode
 *    \code value != NULL \endcode
 * \retval FLAC__bool
 *    \c false if the decoder is already initialized, else \c true.
 */
OggFLAC_API FLAC__bool OggFLAC__stream_decoder_set_metadata_callback(OggFLAC__StreamDecoder *decoder, OggFLAC__StreamDecoderMetadataCallback value);

/** Set the error callback.
 * This is inherited from FLAC__StreamDecoder; see FLAC__stream_decoder_set_error_callback()
 *
 * \note
 * The callback is mandatory and must be set before initialization.
 *
 * \default \c NULL
 * \param  decoder  A decoder instance to set.
 * \param  value    See above.
 * \assert
 *    \code decoder != NULL \endcode
 *    \code value != NULL \endcode
 * \retval FLAC__bool
 *    \c false if the decoder is already initialized, else \c true.
 */
OggFLAC_API FLAC__bool OggFLAC__stream_decoder_set_error_callback(OggFLAC__StreamDecoder *decoder, OggFLAC__StreamDecoderErrorCallback value);

/** Set the client data to be passed back to callbacks.
 *  This value will be supplied to callbacks in their \a client_data
 *  argument.
 *
 * \default \c NULL
 * \param  decoder  A decoder instance to set.
 * \param  value    See above.
 * \assert
 *    \code decoder != NULL \endcode
 * \retval FLAC__bool
 *    \c false if the decoder is already initialized, else \c true.
 */
OggFLAC_API FLAC__bool OggFLAC__stream_decoder_set_client_data(OggFLAC__StreamDecoder *decoder, void *value);

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
 *  Should be called after OggFLAC__stream_decoder_new() and
 *  OggFLAC__stream_decoder_set_*() but before any of the
 *  OggFLAC__stream_decoder_process_*() functions.  Will set and return the
 *  decoder state, which will be OggFLAC__STREAM_DECODER_OK
 *  if initialization succeeded.
 *
 * \param  decoder  An uninitialized decoder instance.
 * \assert
 *    \code decoder != NULL \endcode
 * \retval OggFLAC__StreamDecoderState
 *    \c OggFLAC__STREAM_DECODER_OK if initialization was
 *    successful; see OggFLAC__StreamDecoderState for the meanings of other
 *    return values.
 */
OggFLAC_API OggFLAC__StreamDecoderState OggFLAC__stream_decoder_init(OggFLAC__StreamDecoder *decoder);

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
OggFLAC_API void OggFLAC__stream_decoder_finish(OggFLAC__StreamDecoder *decoder);

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
 *    error occurs.
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
 *    \c false if any read or write error occurred (except
 *    \c FLAC__STREAM_DECODER_ERROR_STATUS_LOST_SYNC), else \c true;
 *    in any case, check the decoder state with
 *    OggFLAC__stream_decoder_get_state() to see what went wrong or to
 *    check for lost synchronization (a sign of stream corruption).
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
 *    \c false if any read or write error occurred (except
 *    \c OggFLAC__STREAM_DECODER_ERROR_STATUS_LOST_SYNC), else \c true;
 *    in any case, check the decoder state with
 *    OggFLAC__stream_decoder_get_state() to see what went wrong or to
 *    check for lost synchronization (a sign of stream corruption).
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
 *    \c false if any read or write error occurred (except
 *    \c OggFLAC__STREAM_DECODER_ERROR_STATUS_LOST_SYNC), else \c true;
 *    in any case, check the decoder state with
 *    OggFLAC__stream_decoder_get_state() to see what went wrong or to
 *    check for lost synchronization (a sign of stream corruption).
 */
OggFLAC_API FLAC__bool OggFLAC__stream_decoder_process_until_end_of_stream(OggFLAC__StreamDecoder *decoder);

/* \} */

#ifdef __cplusplus
}
#endif

#endif
