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

#ifndef OggFLAC__FILE_DECODER_H
#define OggFLAC__FILE_DECODER_H

#include "export.h"

#include "seekable_stream_decoder.h"

#ifdef __cplusplus
extern "C" {
#endif


/** \file include/OggFLAC/file_decoder.h
 *
 *  \brief
 *  This module contains the functions which implement the file
 *  decoder.
 *
 *  See the detailed documentation in the
 *  \link oggflac_file_decoder file decoder \endlink module.
 */

/** \defgroup oggflac_file_decoder OggFLAC/file_decoder.h: file decoder interface
 *  \ingroup oggflac_decoder
 *
 *  \brief
 *  This module contains the functions which implement the file
 *  decoder.
 *
 * The interface here is nearly identical to FLAC's file decoder,
 * including the callbacks, with the addition of
 * OggFLAC__file_decoder_set_serial_number().  See the
 * \link flac_file_decoder FLAC file decoder module \endlink
 * for full documentation.
 *
 * \{
 */


/** State values for an OggFLAC__FileDecoder
 *
 *  The decoder's state can be obtained by calling OggFLAC__file_decoder_get_state().
 */
typedef enum {

	OggFLAC__FILE_DECODER_OK = 0,
	/**< The decoder is in the normal OK state. */

	OggFLAC__FILE_DECODER_END_OF_FILE,
	/**< The decoder has reached the end of the file. */

	OggFLAC__FILE_DECODER_ERROR_OPENING_FILE,
	/**< An error occurred opening the input file. */

	OggFLAC__FILE_DECODER_SEEK_ERROR,
	/**< An error occurred while seeking. */

	OggFLAC__FILE_DECODER_SEEKABLE_STREAM_DECODER_ERROR,
	/**< An error occurred in the underlying seekable stream decoder;
	 * check OggFLAC__file_decoder_get_seekable_stream_decoder_state().
	 */

	OggFLAC__FILE_DECODER_MEMORY_ALLOCATION_ERROR,
	/**< Memory allocation failed. */

	OggFLAC__FILE_DECODER_ALREADY_INITIALIZED,
	/**< OggFLAC__file_decoder_init() was called when the decoder was
	 * already initialized, usually because
	 * OggFLAC__file_decoder_finish() was not called.
	 */

	OggFLAC__FILE_DECODER_INVALID_CALLBACK,
	/**< The decoder was initialized before setting all the required callbacks. */

	OggFLAC__FILE_DECODER_UNINITIALIZED
	/**< The decoder is in the uninitialized state. */

} OggFLAC__FileDecoderState;

/** Maps an OggFLAC__FileDecoderState to a C string.
 *
 *  Using an OggFLAC__FileDecoderState as the index to this array
 *  will give the string equivalent.  The contents should not be modified.
 */
extern OggFLAC_API const char * const OggFLAC__FileDecoderStateString[];


/***********************************************************************
 *
 * class OggFLAC__FileDecoder : public OggFLAC__SeekableStreamDecoder
 *
 ***********************************************************************/

struct OggFLAC__FileDecoderProtected;
struct OggFLAC__FileDecoderPrivate;
/** The opaque structure definition for the file decoder type.  See the
 *  \link oggflac_file_decoder file decoder module \endlink for a detailed
 *  description.
 */
typedef struct {
	struct OggFLAC__FileDecoderProtected *protected_; /* avoid the C++ keyword 'protected' */
	struct OggFLAC__FileDecoderPrivate *private_; /* avoid the C++ keyword 'private' */
} OggFLAC__FileDecoder;

/** Signature for the write callback.
 *  See OggFLAC__file_decoder_set_write_callback()
 *  and OggFLAC__SeekableStreamDecoderWriteCallback for more info.
 *
 * \param  decoder  The decoder instance calling the callback.
 * \param  frame    The description of the decoded frame.
 * \param  buffer   An array of pointers to decoded channels of data.
 * \param  client_data  The callee's client data set through
 *                      OggFLAC__file_decoder_set_client_data().
 * \retval FLAC__StreamDecoderWriteStatus
 *    The callee's return status.
 */
typedef FLAC__StreamDecoderWriteStatus (*OggFLAC__FileDecoderWriteCallback)(const OggFLAC__FileDecoder *decoder, const FLAC__Frame *frame, const FLAC__int32 * const buffer[], void *client_data);

/** Signature for the metadata callback.
 *  See OggFLAC__file_decoder_set_metadata_callback()
 *  and OggFLAC__SeekableStreamDecoderMetadataCallback for more info.
 *
 * \param  decoder  The decoder instance calling the callback.
 * \param  metadata The decoded metadata block.
 * \param  client_data  The callee's client data set through
 *                      OggFLAC__file_decoder_set_client_data().
 */
typedef void (*OggFLAC__FileDecoderMetadataCallback)(const OggFLAC__FileDecoder *decoder, const FLAC__StreamMetadata *metadata, void *client_data);

/** Signature for the error callback.
 *  See OggFLAC__file_decoder_set_error_callback()
 *  and OggFLAC__SeekableStreamDecoderErrorCallback for more info.
 *
 * \param  decoder  The decoder instance calling the callback.
 * \param  status   The error encountered by the decoder.
 * \param  client_data  The callee's client data set through
 *                      OggFLAC__file_decoder_set_client_data().
 */
typedef void (*OggFLAC__FileDecoderErrorCallback)(const OggFLAC__FileDecoder *decoder, FLAC__StreamDecoderErrorStatus status, void *client_data);


/***********************************************************************
 *
 * Class constructor/destructor
 *
 ***********************************************************************/

/** Create a new file decoder instance.  The instance is created with
 *  default settings; see the individual OggFLAC__file_decoder_set_*()
 *  functions for each setting's default.
 *
 * \retval OggFLAC__FileDecoder*
 *    \c NULL if there was an error allocating memory, else the new instance.
 */
OggFLAC_API OggFLAC__FileDecoder *OggFLAC__file_decoder_new();

/** Free a decoder instance.  Deletes the object pointed to by \a decoder.
 *
 * \param decoder  A pointer to an existing decoder.
 * \assert
 *    \code decoder != NULL \endcode
 */
OggFLAC_API void OggFLAC__file_decoder_delete(OggFLAC__FileDecoder *decoder);


/***********************************************************************
 *
 * Public class method prototypes
 *
 ***********************************************************************/

/** Set the "MD5 signature checking" flag.
 *  This is inherited from FLAC__FileDecoder; see
 *  FLAC__file_decoder_set_md5_checking().
 *
 * \default \c false
 * \param  decoder  A decoder instance to set.
 * \param  value    See above.
 * \assert
 *    \code decoder != NULL \endcode
 * \retval FLAC__bool
 *    \c false if the decoder is already initialized, else \c true.
 */
OggFLAC_API FLAC__bool OggFLAC__file_decoder_set_md5_checking(OggFLAC__FileDecoder *decoder, FLAC__bool value);

/** Set the input file name to decode.
 *  This is inherited from FLAC__FileDecoder; see
 *  FLAC__file_decoder_set_filename().
 *
 * \default \c "-"
 * \param  decoder  A decoder instance to set.
 * \param  value    The input file name, or "-" for \c stdin.
 * \assert
 *    \code decoder != NULL \endcode
 *    \code value != NULL \endcode
 * \retval FLAC__bool
 *    \c false if the decoder is already initialized, or there was a memory
 *    allocation error, else \c true.
 */
OggFLAC_API FLAC__bool OggFLAC__file_decoder_set_filename(OggFLAC__FileDecoder *decoder, const char *value);

/** Set the write callback.
 *  This is inherited from FLAC__FileDecoder; see
 *  FLAC__file_decoder_set_write_callback().
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
OggFLAC_API FLAC__bool OggFLAC__file_decoder_set_write_callback(OggFLAC__FileDecoder *decoder, OggFLAC__FileDecoderWriteCallback value);

/** Set the metadata callback.
 *  This is inherited from FLAC__FileDecoder; see
 *  FLAC__file_decoder_set_metadata_callback().
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
OggFLAC_API FLAC__bool OggFLAC__file_decoder_set_metadata_callback(OggFLAC__FileDecoder *decoder, OggFLAC__FileDecoderMetadataCallback value);

/** Set the error callback.
 *  This is inherited from FLAC__FileDecoder; see
 *  FLAC__file_decoder_set_error_callback().
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
OggFLAC_API FLAC__bool OggFLAC__file_decoder_set_error_callback(OggFLAC__FileDecoder *decoder, OggFLAC__FileDecoderErrorCallback value);

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
OggFLAC_API FLAC__bool OggFLAC__file_decoder_set_client_data(OggFLAC__FileDecoder *decoder, void *value);

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
OggFLAC_API FLAC__bool OggFLAC__file_decoder_set_serial_number(OggFLAC__FileDecoder *decoder, long serial_number);

/** This is inherited from FLAC__FileDecoder; see
 *  FLAC__file_decoder_set_metadata_respond().
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
OggFLAC_API FLAC__bool OggFLAC__file_decoder_set_metadata_respond(OggFLAC__FileDecoder *decoder, FLAC__MetadataType type);

/** This is inherited from FLAC__FileDecoder; see
 *  FLAC__file_decoder_set_metadata_respond_application().
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
OggFLAC_API FLAC__bool OggFLAC__file_decoder_set_metadata_respond_application(OggFLAC__FileDecoder *decoder, const FLAC__byte id[4]);

/** This is inherited from FLAC__FileDecoder; see
 *  FLAC__file_decoder_set_metadata_respond_all().
 *
 * \default By default, only the \c STREAMINFO block is returned via the
 *          metadata callback.
 * \param  decoder  A decoder instance to set.
 * \assert
 *    \code decoder != NULL \endcode
 * \retval FLAC__bool
 *    \c false if the decoder is already initialized, else \c true.
 */
OggFLAC_API FLAC__bool OggFLAC__file_decoder_set_metadata_respond_all(OggFLAC__FileDecoder *decoder);

/** This is inherited from FLAC__FileDecoder; see
 *  FLAC__file_decoder_set_metadata_ignore().
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
OggFLAC_API FLAC__bool OggFLAC__file_decoder_set_metadata_ignore(OggFLAC__FileDecoder *decoder, FLAC__MetadataType type);

/** This is inherited from FLAC__FileDecoder; see
 *  FLAC__file_decoder_set_metadata_ignore_application().
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
OggFLAC_API FLAC__bool OggFLAC__file_decoder_set_metadata_ignore_application(OggFLAC__FileDecoder *decoder, const FLAC__byte id[4]);

/** This is inherited from FLAC__FileDecoder; see
 *  FLAC__file_decoder_set_metadata_ignore_all().
 *
 * \default By default, only the \c STREAMINFO block is returned via the
 *          metadata callback.
 * \param  decoder  A decoder instance to set.
 * \assert
 *    \code decoder != NULL \endcode
 * \retval FLAC__bool
 *    \c false if the decoder is already initialized, else \c true.
 */
OggFLAC_API FLAC__bool OggFLAC__file_decoder_set_metadata_ignore_all(OggFLAC__FileDecoder *decoder);

/** Get the current decoder state.
 *
 * \param  decoder  A decoder instance to query.
 * \assert
 *    \code decoder != NULL \endcode
 * \retval OggFLAC__FileDecoderState
 *    The current decoder state.
 */
OggFLAC_API OggFLAC__FileDecoderState OggFLAC__file_decoder_get_state(const OggFLAC__FileDecoder *decoder);

/** Get the state of the underlying seekable stream decoder.
 *  Useful when the file decoder state is
 *  \c OggFLAC__FILE_DECODER_SEEKABLE_STREAM_DECODER_ERROR.
 *
 * \param  decoder  A decoder instance to query.
 * \assert
 *    \code decoder != NULL \endcode
 * \retval FLAC__FileDecoderState
 *    The seekable stream decoder state.
 */
OggFLAC_API OggFLAC__SeekableStreamDecoderState OggFLAC__file_decoder_get_seekable_stream_decoder_state(const OggFLAC__FileDecoder *decoder);

/** Get the state of the underlying stream decoder.
 *  Useful when the file decoder state is
 *  \c OggFLAC__FILE_DECODER_SEEKABLE_STREAM_DECODER_ERROR
 *  and the seekable stream decoder state is
 *  \c OggFLAC__SEEKABLE_STREAM_DECODER_STREAM_DECODER_ERROR.
 *
 * \param  decoder  A decoder instance to query.
 * \assert
 *    \code decoder != NULL \endcode
 * \retval OggFLAC__StreamDecoderState
 *    The stream decoder state.
 */
OggFLAC_API OggFLAC__StreamDecoderState OggFLAC__file_decoder_get_stream_decoder_state(const OggFLAC__FileDecoder *decoder);

/** Get the state of the underlying FLAC stream decoder.
 *  Useful when the file decoder state is
 *  \c OggFLAC__FILE_DECODER_SEEKABLE_STREAM_DECODER_ERROR
 *  and the seekable stream decoder state is
 *  \c OggFLAC__SEEKABLE_STREAM_DECODER_STREAM_DECODER_ERROR
 *  and the stream decoder state is
 *  \c OggFLAC__STREAM_DECODER_FLAC_STREAM_DECODER_ERROR.
 *
 * \param  decoder  A decoder instance to query.
 * \assert
 *    \code decoder != NULL \endcode
 * \retval FLAC__StreamDecoderState
 *    The FLAC stream decoder state.
 */
OggFLAC_API FLAC__StreamDecoderState OggFLAC__file_decoder_get_FLAC_stream_decoder_state(const OggFLAC__FileDecoder *decoder);

/** Get the current decoder state as a C string.
 *  This version automatically resolves
 *  \c OggFLAC__FILE_DECODER_SEEKABLE_STREAM_DECODER_ERROR
 *  by getting the seekable stream decoder's state.
 *
 * \param  decoder  A decoder instance to query.
 * \assert
 *    \code decoder != NULL \endcode
 * \retval const char *
 *    The decoder state as a C string.  Do not modify the contents.
 */
OggFLAC_API const char *OggFLAC__file_decoder_get_resolved_state_string(const OggFLAC__FileDecoder *decoder);

/** This is inherited from FLAC__FileDecoder; see
 *  FLAC__file_decoder_get_md5_checking().
 *
 * \param  decoder  A decoder instance to query.
 * \assert
 *    \code decoder != NULL \endcode
 * \retval FLAC__bool
 *    See above.
 */
OggFLAC_API FLAC__bool OggFLAC__file_decoder_get_md5_checking(const OggFLAC__FileDecoder *decoder);

/** This is inherited from FLAC__FileDecoder; see
 *  FLAC__file_decoder_get_channels().
 *
 * \param  decoder  A decoder instance to query.
 * \assert
 *    \code decoder != NULL \endcode
 * \retval unsigned
 *    See above.
 */
OggFLAC_API unsigned OggFLAC__file_decoder_get_channels(const OggFLAC__FileDecoder *decoder);

/** This is inherited from FLAC__FileDecoder; see
 *  FLAC__file_decoder_get_channel_assignment().
 *
 * \param  decoder  A decoder instance to query.
 * \assert
 *    \code decoder != NULL \endcode
 * \retval OggFLAC__ChannelAssignment
 *    See above.
 */
OggFLAC_API FLAC__ChannelAssignment OggFLAC__file_decoder_get_channel_assignment(const OggFLAC__FileDecoder *decoder);

/** This is inherited from FLAC__FileDecoder; see
 *  FLAC__file_decoder_get_bits_per_sample().
 *
 * \param  decoder  A decoder instance to query.
 * \assert
 *    \code decoder != NULL \endcode
 * \retval unsigned
 *    See above.
 */
OggFLAC_API unsigned OggFLAC__file_decoder_get_bits_per_sample(const OggFLAC__FileDecoder *decoder);

/** This is inherited from FLAC__FileDecoder; see
 *  FLAC__file_decoder_get_sample_rate().
 *
 * \param  decoder  A decoder instance to query.
 * \assert
 *    \code decoder != NULL \endcode
 * \retval unsigned
 *    See above.
 */
OggFLAC_API unsigned OggFLAC__file_decoder_get_sample_rate(const OggFLAC__FileDecoder *decoder);

/** This is inherited from FLAC__FileDecoder; see
 *  FLAC__file_decoder_get_blocksize().
 *
 * \param  decoder  A decoder instance to query.
 * \assert
 *    \code decoder != NULL \endcode
 * \retval unsigned
 *    See above.
 */
OggFLAC_API unsigned OggFLAC__file_decoder_get_blocksize(const OggFLAC__FileDecoder *decoder);

/** Initialize the decoder instance.
 *  Should be called after OggFLAC__file_decoder_new() and
 *  OggFLAC__file_decoder_set_*() but before any of the
 *  OggFLAC__file_decoder_process_*() functions.  Will set and return
 *  the decoder state, which will be OggFLAC__FILE_DECODER_OK if
 *  initialization succeeded.
 *
 * \param  decoder  An uninitialized decoder instance.
 * \assert
 *    \code decoder != NULL \endcode
 * \retval OggFLAC__FileDecoderState
 *    \c OggFLAC__FILE_DECODER_OK if initialization was successful; see
 *    OggFLAC__FileDecoderState for the meanings of other return values.
 */
OggFLAC_API OggFLAC__FileDecoderState OggFLAC__file_decoder_init(OggFLAC__FileDecoder *decoder);

/** Finish the decoding process.
 *  Flushes the decoding buffer, releases resources, resets the decoder
 *  settings to their defaults, and returns the decoder state to
 *  OggFLAC__FILE_DECODER_UNINITIALIZED.
 *
 *  In the event of a prematurely-terminated decode, it is not strictly
 *  necessary to call this immediately before OggFLAC__file_decoder_delete()
 *  but it is good practice to match every OggFLAC__file_decoder_init() with
 *  an OggFLAC__file_decoder_finish().
 *
 * \param  decoder  An uninitialized decoder instance.
 * \assert
 *    \code decoder != NULL \endcode
 * \retval FLAC__bool
 *    \c false if MD5 checking is on AND a STREAMINFO block was available
 *    AND the MD5 signature in the STREAMINFO block was non-zero AND the
 *    signature does not match the one computed by the decoder; else
 *    \c true.
 */
OggFLAC_API FLAC__bool OggFLAC__file_decoder_finish(OggFLAC__FileDecoder *decoder);

/** This is inherited from FLAC__FileDecoder; see
 *  FLAC__file_decoder_process_single().
 *
 * \param  decoder  A decoder instance.
 * \assert
 *    \code decoder != NULL \endcode
 * \retval FLAC__bool
 *    See above.
 */
OggFLAC_API FLAC__bool OggFLAC__file_decoder_process_single(OggFLAC__FileDecoder *decoder);

/** This is inherited from FLAC__FileDecoder; see
 *  FLAC__file_decoder_process_until_end_of_metadata().
 *
 * \param  decoder  A decoder instance.
 * \assert
 *    \code decoder != NULL \endcode
 * \retval FLAC__bool
 *    See above.
 */
OggFLAC_API FLAC__bool OggFLAC__file_decoder_process_until_end_of_metadata(OggFLAC__FileDecoder *decoder);

/** This is inherited from FLAC__FileDecoder; see
 *  FLAC__file_decoder_process_until_end_of_stream().
 *
 * \param  decoder  A decoder instance.
 * \assert
 *    \code decoder != NULL \endcode
 * \retval FLAC__bool
 *    See above.
 */
OggFLAC_API FLAC__bool OggFLAC__file_decoder_process_until_end_of_file(OggFLAC__FileDecoder *decoder);

/** This is inherited from OggFLAC__SeekableStreamDecoder; see
 *  OggFLAC__seekable_stream_decoder_seek_absolute().
 *
 * \param  decoder  A decoder instance.
 * \param  sample   The target sample number to seek to.
 * \assert
 *    \code decoder != NULL \endcode
 * \retval FLAC__bool
 *    \c true if successful, else \c false.
 */
OggFLAC_API FLAC__bool OggFLAC__file_decoder_seek_absolute(OggFLAC__FileDecoder *decoder, FLAC__uint64 sample);

/* \} */

#ifdef __cplusplus
}
#endif

#endif
