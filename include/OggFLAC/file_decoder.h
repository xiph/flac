/* libOggFLAC - Free Lossless Audio Codec + Ogg library
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

#ifndef OggFLAC__FILE_DECODER_H
#define OggFLAC__FILE_DECODER_H

#include "FLAC/file_decoder.h"

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
 * The interface here is identical to FLAC's file decoder.  See the
 * defaults, including the callbacks.  See the \link flac_file_decoder
 * FLAC file decoder module \endlink for full documentation.
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

	OggFLAC__FILE_DECODER_FLAC_FILE_DECODER_ERROR,
	/**< An error occurred in the underlying FLAC file decoder;
	 * check OggFLAC__file_decoder_get_FLAC_file_decoder_state().
	 */

	OggFLAC__FILE_DECODER_INVALID_CALLBACK,
	/**< The decoder was initialized before setting all the required callbacks. */

	OggFLAC__FILE_DECODER_MEMORY_ALLOCATION_ERROR,
	/**< Memory allocation failed. */

	OggFLAC__FILE_DECODER_ALREADY_INITIALIZED,
	/**< OggFLAC__file_decoder_init() was called when the decoder was
	 * already initialized, usually because
	 * OggFLAC__file_decoder_finish() was not called.
	 */

	OggFLAC__FILE_DECODER_UNINITIALIZED
	/**< The decoder is in the uninitialized state. */

} OggFLAC__FileDecoderState;

/** Maps an OggFLAC__FileDecoderState to a C string.
 *
 *  Using an OggFLAC__FileDecoderState as the index to this array
 *  will give the string equivalent.  The contents should not be modified.
 */
extern const char * const OggFLAC__FileDecoderStateString[];


/***********************************************************************
 *
 * class OggFLAC__FileDecoder : public FLAC__FileDecoder
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

typedef FLAC__StreamDecoderWriteStatus (*OggFLAC__FileDecoderWriteCallback)(const OggFLAC__FileDecoder *decoder, const FLAC__Frame *frame, const FLAC__int32 * const buffer[], void *client_data);
typedef void (*OggFLAC__FileDecoderMetadataCallback)(const OggFLAC__FileDecoder *decoder, const FLAC__StreamMetadata *metadata, void *client_data);
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
OggFLAC__FileDecoder *OggFLAC__file_decoder_new();

/** Free a decoder instance.  Deletes the object pointed to by \a decoder.
 *
 * \param decoder  A pointer to an existing decoder.
 * \assert
 *    \code decoder != NULL \endcode
 */
void OggFLAC__file_decoder_delete(OggFLAC__FileDecoder *decoder);


/***********************************************************************
 *
 * Public class method prototypes
 *
 ***********************************************************************/

/*@@@inherit set_serial_number*/

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
FLAC__bool OggFLAC__file_decoder_set_md5_checking(OggFLAC__FileDecoder *decoder, FLAC__bool value);

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
FLAC__bool OggFLAC__file_decoder_set_filename(OggFLAC__FileDecoder *decoder, const char *value);

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
FLAC__bool OggFLAC__file_decoder_set_write_callback(OggFLAC__FileDecoder *decoder, OggFLAC__FileDecoderWriteCallback value);

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
FLAC__bool OggFLAC__file_decoder_set_metadata_callback(OggFLAC__FileDecoder *decoder, OggFLAC__FileDecoderMetadataCallback value);

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
FLAC__bool OggFLAC__file_decoder_set_error_callback(OggFLAC__FileDecoder *decoder, OggFLAC__FileDecoderErrorCallback value);

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
FLAC__bool OggFLAC__file_decoder_set_client_data(OggFLAC__FileDecoder *decoder, void *value);

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
FLAC__bool OggFLAC__file_decoder_set_metadata_respond(OggFLAC__FileDecoder *decoder, OggFLAC__MetadataType type);

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
FLAC__bool OggFLAC__file_decoder_set_metadata_respond_application(OggFLAC__FileDecoder *decoder, const FLAC__byte id[4]);

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
FLAC__bool OggFLAC__file_decoder_set_metadata_respond_all(OggFLAC__FileDecoder *decoder);

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
FLAC__bool OggFLAC__file_decoder_set_metadata_ignore(OggFLAC__FileDecoder *decoder, OggFLAC__MetadataType type);

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
FLAC__bool OggFLAC__file_decoder_set_metadata_ignore_application(OggFLAC__FileDecoder *decoder, const FLAC__byte id[4]);

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
FLAC__bool OggFLAC__file_decoder_set_metadata_ignore_all(OggFLAC__FileDecoder *decoder);

/** Get the current decoder state.
 *
 * \param  decoder  A decoder instance to query.
 * \assert
 *    \code decoder != NULL \endcode
 * \retval OggFLAC__FileDecoderState
 *    The current decoder state.
 */
OggFLAC__FileDecoderState OggFLAC__file_decoder_get_state(const OggFLAC__FileDecoder *decoder);

/** Get the state of the underlying FLAC file decoder.
 *  Useful when the file decoder state is
 *  \c OggFLAC__FILE_DECODER_FLAC_FILE_DECODER_ERROR.
 *
 * \param  decoder  A decoder instance to query.
 * \assert
 *    \code decoder != NULL \endcode
 * \retval FLAC__FileDecoderState
 *    The FLAC file decoder state.
 */
FLAC__FileDecoderState OggFLAC__file_decoder_get_FLAC_file_decoder_state(const OggFLAC__FileDecoder *decoder);

/** Get the state of the underlying FLAC file decoder's seekable stream decoder.
 *  Useful when the file decoder state is
 *  \c OggFLAC__FILE_DECODER_FLAC_FILE_DECODER_ERROR and the FLAC file decoder state is
 *  \c FLAC__FILE_DECODER_SEEKABLE_STREAM_DECODER_ERROR.
 *
 * \param  decoder  A decoder instance to query.
 * \assert
 *    \code decoder != NULL \endcode
 * \retval FLAC__SeekableStreamDecoderState
 *    The FLAC seekable stream decoder state.
 */
FLAC__SeekableStreamDecoderState OggFLAC__file_decoder_get_FLAC_seekable_stream_decoder_state(const OggFLAC__FileDecoder *decoder);

/** Get the state of the underlying FLAC file decoder's stream decoder.
 *  Useful when the file decoder state is
 *  \c OggFLAC__FILE_DECODER_FLAC_FILE_DECODER_ERROR and the FLAC file decoder state is
 *  \c FLAC__FILE_DECODER_SEEKABLE_STREAM_DECODER_ERROR and the
 *  FLAC seekable stream decoder state is \c FLAC__SEEKABLE_STREAM_DECODER_STREAM_DECODER_ERROR.
 *
 * \param  decoder  A decoder instance to query.
 * \assert
 *    \code decoder != NULL \endcode
 * \retval FLAC__StreamDecoderState
 *    The FLAC stream decoder state.
 */
FLAC__StreamDecoderState OggFLAC__file_decoder_get_FLAC_stream_decoder_state(const OggFLAC__FileDecoder *decoder);

/** This is inherited from FLAC__FileDecoder; see
 *  FLAC__file_decoder_get_md5_checking().
 *
 * \param  decoder  A decoder instance to query.
 * \assert
 *    \code decoder != NULL \endcode
 * \retval FLAC__bool
 *    See above.
 */
FLAC__bool OggFLAC__file_decoder_get_md5_checking(const OggFLAC__FileDecoder *decoder);

/** This is inherited from FLAC__FileDecoder; see
 *  FLAC__file_decoder_get_channels().
 *
 * \param  decoder  A decoder instance to query.
 * \assert
 *    \code decoder != NULL \endcode
 * \retval unsigned
 *    See above.
 */
unsigned OggFLAC__file_decoder_get_channels(const OggFLAC__FileDecoder *decoder);

/** This is inherited from FLAC__FileDecoder; see
 *  FLAC__file_decoder_get_channel_assignment().
 *
 * \param  decoder  A decoder instance to query.
 * \assert
 *    \code decoder != NULL \endcode
 * \retval OggFLAC__ChannelAssignment
 *    See above.
 */
OggFLAC__ChannelAssignment OggFLAC__file_decoder_get_channel_assignment(const OggFLAC__FileDecoder *decoder);

/** This is inherited from FLAC__FileDecoder; see
 *  FLAC__file_decoder_get_bits_per_sample().
 *
 * \param  decoder  A decoder instance to query.
 * \assert
 *    \code decoder != NULL \endcode
 * \retval unsigned
 *    See above.
 */
unsigned OggFLAC__file_decoder_get_bits_per_sample(const OggFLAC__FileDecoder *decoder);

/** This is inherited from FLAC__FileDecoder; see
 *  FLAC__file_decoder_get_sample_rate().
 *
 * \param  decoder  A decoder instance to query.
 * \assert
 *    \code decoder != NULL \endcode
 * \retval unsigned
 *    See above.
 */
unsigned OggFLAC__file_decoder_get_sample_rate(const OggFLAC__FileDecoder *decoder);

/** This is inherited from FLAC__FileDecoder; see
 *  FLAC__file_decoder_get_blocksize().
 *
 * \param  decoder  A decoder instance to query.
 * \assert
 *    \code decoder != NULL \endcode
 * \retval unsigned
 *    See above.
 */
unsigned OggFLAC__file_decoder_get_blocksize(const OggFLAC__FileDecoder *decoder);

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
OggFLAC__FileDecoderState OggFLAC__file_decoder_init(OggFLAC__FileDecoder *decoder);

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
FLAC__bool OggFLAC__file_decoder_finish(OggFLAC__FileDecoder *decoder);

/** This is inherited from FLAC__FileDecoder; see
 *  FLAC__file_decoder_process_single().
 *
 * \param  decoder  A decoder instance.
 * \assert
 *    \code decoder != NULL \endcode
 * \retval FLAC__bool
 *    See above.
 */
FLAC__bool OggFLAC__file_decoder_process_single(OggFLAC__FileDecoder *decoder);

/** This is inherited from FLAC__FileDecoder; see
 *  FLAC__file_decoder_process_until_end_of_metadata().
 *
 * \param  decoder  A decoder instance.
 * \assert
 *    \code decoder != NULL \endcode
 * \retval FLAC__bool
 *    See above.
 */
FLAC__bool OggFLAC__file_decoder_process_until_end_of_metadata(OggFLAC__FileDecoder *decoder);

/** This is inherited from FLAC__FileDecoder; see
 *  FLAC__file_decoder_process_until_end_of_stream().
 *
 * \param  decoder  A decoder instance.
 * \assert
 *    \code decoder != NULL \endcode
 * \retval FLAC__bool
 *    See above.
 */
FLAC__bool OggFLAC__file_decoder_process_until_end_of_file(OggFLAC__FileDecoder *decoder);

/** This is inherited from FLAC__FileDecoder; see
 *  FLAC__file_decoder_process_remaining_frames().
 *
 * \param  decoder  A decoder instance.
 * \assert
 *    \code decoder != NULL \endcode
 * \retval FLAC__bool
 *    See above.
 */
FLAC__bool OggFLAC__file_decoder_process_remaining_frames(OggFLAC__FileDecoder *decoder);

/** This is inherited from FLAC__FileDecoder; see
 *  FLAC__file_decoder_seek_absolute().
 *
 * \param  decoder  A decoder instance.
 * \param  sample   The target sample number to seek to.
 * \assert
 *    \code decoder != NULL \endcode
 * \retval FLAC__bool
 *    \c true if successful, else \c false.
 */
FLAC__bool OggFLAC__file_decoder_seek_absolute(OggFLAC__FileDecoder *decoder, FLAC__uint64 sample);

/* \} */

#ifdef __cplusplus
}
#endif

#endif
