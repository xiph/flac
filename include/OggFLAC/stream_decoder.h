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

#ifndef OggFLAC__STREAM_DECODER_H
#define OggFLAC__STREAM_DECODER_H

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
 * For decoding OggFLAC streams, libOggFLAC provides three layers of access.  The
 * lowest layer is non-seekable stream-level decoding, the next is seekable
 * stream-level decoding, and the highest layer is file-level decoding.  The
 * interfaces are described in the \link oggflac_stream_decoder stream decoder
 * \endlink, \link oggflac_seekable_stream_decoder seekable stream decoder
 * \endlink, and \link oggflac_file_decoder file decoder \endlink modules
 * respectively.  Typically you will choose the highest layer that your input
 * source will support.
 *
 * The stream decoder relies on callbacks for all input and output and has no
 * provisions for seeking.  The seekable stream decoder wraps the stream
 * decoder and exposes functions for seeking.  However, you must provide
 * extra callbacks for seek-related operations on your stream, like seek and
 * tell.  The file decoder wraps the seekable stream decoder and supplies
 * most of the callbacks internally, simplifying the processing of standard
 * files.
 */

/** \defgroup oggflac_stream_decoder OggFLAC/stream_decoder.h: stream decoder interface
 *  \ingroup oggflac_decoder
 *
 *  \brief
 *  This module contains the functions which implement the stream
 *  decoder.
 *
 * The basic usage of this decoder is as follows:
 * - The program creates an instance of a decoder using
 *   OggFLAC__stream_decoder_new().
 * - The program overrides the default settings and sets callbacks for
 *   reading, writing, error reporting, and metadata reporting using
 *   OggFLAC__stream_decoder_set_*() functions.
 * - The program initializes the instance to validate the settings and
 *   prepare for decoding using OggFLAC__stream_decoder_init().
 * - The program calls the OggFLAC__stream_decoder_process_*() functions
 *   to decode data, which subsequently calls the callbacks.
 * - The program finishes the decoding with OggFLAC__stream_decoder_finish(),
 *   which flushes the input and output and resets the decoder to the
 *   uninitialized state.
 * - The instance may be used again or deleted with
 *   OggFLAC__stream_decoder_delete().
 *
 * In more detail, the program will create a new instance by calling
 * OggFLAC__stream_decoder_new(), then call OggFLAC__stream_decoder_set_*()
 * functions to set the callbacks and client data, and call
 * OggFLAC__stream_decoder_init().  The required callbacks are:
 *
 * - Read callback - This function will be called when the decoder needs
 *   more input data.  The address of the buffer to be filled is supplied,
 *   along with the number of bytes the buffer can hold.  The callback may
 *   choose to supply less data and modify the byte count but must be careful
 *   not to overflow the buffer.  The callback then returns a status code
 *   chosen from OggFLAC__StreamDecoderReadStatus.
 * - Write callback - This function will be called when the decoder has
 *   decoded a single frame of data.  The decoder will pass the frame
 *   metadata as well as an array of pointers (one for each channel)
 *   pointing to the decoded audio.
 * - Metadata callback - This function will be called when the decoder has
 *   decoded a metadata block.  There will always be one STREAMINFO block
 *   per stream, followed by zero or more other metadata blocks.  These will
 *   be supplied by the decoder in the same order as they appear in the
 *   stream and always before the first audio frame (i.e. write callback).
 *   The metadata block that is passed in must not be modified, and it
 *   doesn't live beyond the callback, so you should make a copy of it with
 *   OggFLAC__metadata_object_clone() if you will need it elsewhere.  Since
 *   metadata blocks can potentially be large, by default the decoder only
 *   calls the metadata callback for the STREAMINFO block; you can instruct
 *   the decoder to pass or filter other blocks with
 *   OggFLAC__stream_decoder_set_metadata_*() calls.
 * - Error callback - This function will be called whenever an error occurs
 *   during decoding.
 *
 * Once the decoder is initialized, your program will call one of several
 * functions to start the decoding process:
 *
 * - OggFLAC__stream_decoder_process_single() - Tells the decoder to process at
 *   most one metadata block or audio frame and return, calling either the
 *   metadata callback or write callback, respectively, once.  If the decoder
 *   loses sync it will return with only the error callback being called.
 * - OggFLAC__stream_decoder_process_until_end_of_metadata() - Tells the decoder
 *   to process the stream from the current location and stop upon reaching
 *   the first audio frame.  The user will get one metadata, write, or error
 *   callback per metadata block, audio frame, or sync error, respectively.
 * - OggFLAC__stream_decoder_process_until_end_of_stream() - Tells the decoder
 *   to process the stream from the current location until the read callback
 *   returns OggFLAC__STREAM_DECODER_READ_STATUS_END_OF_STREAM or
 *   OggFLAC__STREAM_DECODER_READ_STATUS_ABORT.  The user will get one metadata,
 *   write, or error callback per metadata block, audio frame, or sync error,
 *   respectively.
 *
 * When the decoder has finished decoding (normally or through an abort),
 * the instance is finished by calling OggFLAC__stream_decoder_finish(), which
 * ensures the decoder is in the correct state and frees memory.  Then the
 * instance may be deleted with OggFLAC__stream_decoder_delete() or initialized
 * again to decode another stream.
 *
 * Note that the stream decoder has no real concept of stream position, it
 * just converts data.  To seek within a stream the callbacks have only to
 * flush the decoder using OggFLAC__stream_decoder_flush() and start feeding
 * data from the new position through the read callback.  The seekable
 * stream decoder does just this.
 *
 * The OggFLAC__stream_decoder_set_metadata_*() functions deserve special
 * attention.  By default, the decoder only calls the metadata_callback for
 * the STREAMINFO block.  These functions allow you to tell the decoder
 * explicitly which blocks to parse and return via the metadata_callback
 * and/or which to skip.  Use a OggFLAC__stream_decoder_respond_all(),
 * OggFLAC__stream_decoder_ignore() ... or OggFLAC__stream_decoder_ignore_all(),
 * OggFLAC__stream_decoder_respond() ... sequence to exactly specify which
 * blocks to return.  Remember that some metadata blocks can be big so
 * filtering out the ones you don't use can reduce the memory requirements
 * of the decoder.  Also note the special forms
 * OggFLAC__stream_decoder_respond_application(id) and 
 * OggFLAC__stream_decoder_ignore_application(id) for filtering APPLICATION
 * blocks based on the application ID.
 *
 * STREAMINFO and SEEKTABLE blocks are always parsed and used internally, but
 * they still can legally be filtered from the metadata_callback.
 *
 * \note
 * The "set" functions may only be called when the decoder is in the
 * state OggFLAC__STREAM_DECODER_UNINITIALIZED, i.e. after
 * OggFLAC__stream_decoder_new() or OggFLAC__stream_decoder_finish(), but
 * before OggFLAC__stream_decoder_init().  If this is the case they will
 * return \c true, otherwise \c false.
 *
 * \note
 * OggFLAC__stream_decoder_finish() resets all settings to the constructor
 * defaults, including the callbacks.
 *
 * \{
 */


/** State values for a OggFLAC__StreamDecoder
 *
 *  The decoder's state can be obtained by calling OggFLAC__stream_decoder_get_state().
 */
typedef enum {

	OggFLAC__STREAM_DECODER_SEARCH_FOR_METADATA = 0,
	/**< The decoder is ready to search for metadata. */

	OggFLAC__STREAM_DECODER_READ_METADATA,
	/**< The decoder is ready to or is in the process of reading metadata. */

	OggFLAC__STREAM_DECODER_SEARCH_FOR_FRAME_SYNC,
	/**< The decoder is ready to or is in the process of searching for the frame sync code. */

	OggFLAC__STREAM_DECODER_READ_FRAME,
	/**< The decoder is ready to or is in the process of reading a frame. */

	OggFLAC__STREAM_DECODER_END_OF_STREAM,
	/**< The decoder has reached the end of the stream. */

	OggFLAC__STREAM_DECODER_ABORTED,
	/**< The decoder was aborted by the read callback. */

	OggFLAC__STREAM_DECODER_UNPARSEABLE_STREAM,
	/**< The decoder encountered reserved fields in use in the stream. */

	OggFLAC__STREAM_DECODER_MEMORY_ALLOCATION_ERROR,
	/**< An error occurred allocating memory. */

	OggFLAC__STREAM_DECODER_ALREADY_INITIALIZED,
	/**< OggFLAC__stream_decoder_init() was called when the decoder was
	 * already initialized, usually because
	 * OggFLAC__stream_decoder_finish() was not called.
	 */

	OggFLAC__STREAM_DECODER_INVALID_CALLBACK,
	/**< OggFLAC__stream_decoder_init() was called without all callbacks being set. */

	OggFLAC__STREAM_DECODER_UNINITIALIZED
	/**< The decoder is in the uninitialized state. */

} OggFLAC__StreamDecoderState;

/** Maps a OggFLAC__StreamDecoderState to a C string.
 *
 *  Using a OggFLAC__StreamDecoderState as the index to this array
 *  will give the string equivalent.  The contents should not be modified.
 */
extern const char * const OggFLAC__StreamDecoderStateString[];


/** Return values for the OggFLAC__StreamDecoder read callback.
 */
typedef enum {

	OggFLAC__STREAM_DECODER_READ_STATUS_CONTINUE,
	/**< The read was OK and decoding can continue. */

	OggFLAC__STREAM_DECODER_READ_STATUS_END_OF_STREAM,
	/**< The read was attempted at the end of the stream. */

	OggFLAC__STREAM_DECODER_READ_STATUS_ABORT
	/**< An unrecoverable error occurred.  The decoder will return from the process call. */

} OggFLAC__StreamDecoderReadStatus;

/** Maps a OggFLAC__StreamDecoderReadStatus to a C string.
 *
 *  Using a OggFLAC__StreamDecoderReadStatus as the index to this array
 *  will give the string equivalent.  The contents should not be modified.
 */
extern const char * const OggFLAC__StreamDecoderReadStatusString[];


/** Return values for the OggFLAC__StreamDecoder write callback.
 */
typedef enum {

	OggFLAC__STREAM_DECODER_WRITE_STATUS_CONTINUE,
	/**< The write was OK and decoding can continue. */

	OggFLAC__STREAM_DECODER_WRITE_STATUS_ABORT
	/**< An unrecoverable error occurred.  The decoder will return from the process call. */

} OggFLAC__StreamDecoderWriteStatus;

/** Maps a OggFLAC__StreamDecoderWriteStatus to a C string.
 *
 *  Using a OggFLAC__StreamDecoderWriteStatus as the index to this array
 *  will give the string equivalent.  The contents should not be modified.
 */
extern const char * const OggFLAC__StreamDecoderWriteStatusString[];


/** Possible values passed in to the OggFLAC__StreamDecoder error callback.
 */
typedef enum {

	OggFLAC__STREAM_DECODER_ERROR_STATUS_LOST_SYNC,
	/**< An error in the stream caused the decoder to lose synchronization. */

	OggFLAC__STREAM_DECODER_ERROR_STATUS_BAD_HEADER,
	/**< The decoder encountered a corrupted frame header. */

	OggFLAC__STREAM_DECODER_ERROR_STATUS_FRAME_CRC_MISMATCH
	/**< The frame's data did not match the CRC in the footer. */

} OggFLAC__StreamDecoderErrorStatus;

/** Maps a OggFLAC__StreamDecoderErrorStatus to a C string.
 *
 *  Using a OggFLAC__StreamDecoderErrorStatus as the index to this array
 *  will give the string equivalent.  The contents should not be modified.
 */
extern const char * const OggFLAC__StreamDecoderErrorStatusString[];


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

typedef OggFLAC__StreamDecoderReadStatus (*OggFLAC__StreamDecoderReadCallback)(const OggFLAC__StreamDecoder *decoder, FLAC__byte buffer[], unsigned *bytes, void *client_data);
typedef OggFLAC__StreamDecoderWriteStatus (*OggFLAC__StreamDecoderWriteCallback)(const OggFLAC__StreamDecoder *decoder, const OggFLAC__Frame *frame, const FLAC__int32 * const buffer[], void *client_data);
typedef void (*OggFLAC__StreamDecoderMetadataCallback)(const OggFLAC__StreamDecoder *decoder, const OggFLAC__StreamMetadata *metadata, void *client_data);
typedef void (*OggFLAC__StreamDecoderErrorCallback)(const OggFLAC__StreamDecoder *decoder, OggFLAC__StreamDecoderErrorStatus status, void *client_data);


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
OggFLAC__StreamDecoder *OggFLAC__stream_decoder_new();

/** Free a decoder instance.  Deletes the object pointed to by \a decoder.
 *
 * \param decoder  A pointer to an existing decoder.
 * \assert
 *    \code decoder != NULL \endcode
 */
void OggFLAC__stream_decoder_delete(OggFLAC__StreamDecoder *decoder);


/***********************************************************************
 *
 * Public class method prototypes
 *
 ***********************************************************************/

/** Set the read callback.
 *  The supplied function will be called when the decoder needs more input
 *  data.  The address of the buffer to be filled is supplied, along with
 *  the number of bytes the buffer can hold.  The callback may choose to
 *  supply less data and modify the byte count but must be careful not to
 *  overflow the buffer.  The callback then returns a status code chosen
 *  from OggFLAC__StreamDecoderReadStatus.
 *
 * \note
 * The callback is mandatory and must be set before initialization.
 *
 * \default \c NULL
 * \param  decoder  An decoder instance to set.
 * \param  value    See above.
 * \assert
 *    \code decoder != NULL \endcode
 *    \code value != NULL \endcode
 * \retval FLAC__bool
 *    \c false if the decoder is already initialized, else \c true.
 */
FLAC__bool OggFLAC__stream_decoder_set_read_callback(OggFLAC__StreamDecoder *decoder, OggFLAC__StreamDecoderReadCallback);

/** Set the write callback.
 *  The supplied function will be called when the decoder has decoded a
 *  single frame of data.  The decoder will pass the frame metadata as
 *  well as an array of pointers (one for each channel) pointing to the
 *  decoded audio.
 *
 * \note
 * The callback is mandatory and must be set before initialization.
 *
 * \default \c NULL
 * \param  decoder  An decoder instance to set.
 * \param  value    See above.
 * \assert
 *    \code decoder != NULL \endcode
 *    \code value != NULL \endcode
 * \retval FLAC__bool
 *    \c false if the decoder is already initialized, else \c true.
 */
FLAC__bool OggFLAC__stream_decoder_set_write_callback(OggFLAC__StreamDecoder *decoder, OggFLAC__StreamDecoderWriteCallback);

/** Set the metadata callback.
 *  The supplied function will be called when the decoder has decoded a
 *  metadata block.  There will always be one STREAMINFO block per stream,
 *  followed by zero or more other metadata blocks.  These will be supplied
 *  by the decoder in the same order as they appear in the stream and always
 *  before the first audio frame (i.e. write callback).  The metadata block
 *  that is passed in must not be modified, and it doesn't live beyond the
 *  callback, so you should make a copy of it with
 *  OggFLAC__metadata_object_clone() if you will need it elsewhere.  Since
 *  metadata blocks can potentially be large, by default the decoder only
 *  calls the metadata callback for the STREAMINFO block; you can instruct
 *  the decoder to pass or filter other blocks with
 *  OggFLAC__stream_decoder_set_metadata_*() calls.
 *
 * \note
 * The callback is mandatory and must be set before initialization.
 *
 * \default \c NULL
 * \param  decoder  An decoder instance to set.
 * \param  value    See above.
 * \assert
 *    \code decoder != NULL \endcode
 *    \code value != NULL \endcode
 * \retval FLAC__bool
 *    \c false if the decoder is already initialized, else \c true.
 */
FLAC__bool OggFLAC__stream_decoder_set_metadata_callback(OggFLAC__StreamDecoder *decoder, OggFLAC__StreamDecoderMetadataCallback);

/** Set the error callback.
 *  The supplied function will be called whenever an error occurs during
 *  decoding.
 *
 * \note
 * The callback is mandatory and must be set before initialization.
 *
 * \default \c NULL
 * \param  decoder  An decoder instance to set.
 * \param  value    See above.
 * \assert
 *    \code decoder != NULL \endcode
 *    \code value != NULL \endcode
 * \retval FLAC__bool
 *    \c false if the decoder is already initialized, else \c true.
 */
FLAC__bool OggFLAC__stream_decoder_set_error_callback(OggFLAC__StreamDecoder *decoder, OggFLAC__StreamDecoderErrorCallback);

/** Set the client data to be passed back to callbacks.
 *  This value will be supplied to callbacks in their \a client_data
 *  argument.
 *
 * \default \c NULL
 * \param  decoder  An decoder instance to set.
 * \param  value    See above.
 * \assert
 *    \code decoder != NULL \endcode
 * \retval FLAC__bool
 *    \c false if the decoder is already initialized, else \c true.
 */
FLAC__bool OggFLAC__stream_decoder_set_client_data(OggFLAC__StreamDecoder *decoder, void *value);

/** Direct the decoder to pass on all metadata blocks of type \a type.
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
FLAC__bool OggFLAC__stream_decoder_set_metadata_respond(OggFLAC__StreamDecoder *decoder, OggFLAC__MetadataType type);

/** Direct the decoder to pass on all APPLICATION metadata blocks of the
 *  given \a id.
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
FLAC__bool OggFLAC__stream_decoder_set_metadata_respond_application(OggFLAC__StreamDecoder *decoder, const FLAC__byte id[4]);

/** Direct the decoder to pass on all metadata blocks of any type.
 *
 * \default By default, only the \c STREAMINFO block is returned via the
 *          metadata callback.
 * \param  decoder  A decoder instance to set.
 * \assert
 *    \code decoder != NULL \endcode
 * \retval FLAC__bool
 *    \c false if the decoder is already initialized, else \c true.
 */
FLAC__bool OggFLAC__stream_decoder_set_metadata_respond_all(OggFLAC__StreamDecoder *decoder);

/** Direct the decoder to filter out all metadata blocks of type \a type.
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
FLAC__bool OggFLAC__stream_decoder_set_metadata_ignore(OggFLAC__StreamDecoder *decoder, OggFLAC__MetadataType type);

/** Direct the decoder to filter out all APPLICATION metadata blocks of
 *  the given \a id.
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
FLAC__bool OggFLAC__stream_decoder_set_metadata_ignore_application(OggFLAC__StreamDecoder *decoder, const FLAC__byte id[4]);

/** Direct the decoder to filter out all metadata blocks of any type.
 *
 * \default By default, only the \c STREAMINFO block is returned via the
 *          metadata callback.
 * \param  decoder  A decoder instance to set.
 * \assert
 *    \code decoder != NULL \endcode
 * \retval FLAC__bool
 *    \c false if the decoder is already initialized, else \c true.
 */
FLAC__bool OggFLAC__stream_decoder_set_metadata_ignore_all(OggFLAC__StreamDecoder *decoder);

/** Get the current decoder state.
 *
 * \param  decoder  A decoder instance to query.
 * \assert
 *    \code decoder != NULL \endcode
 * \retval OggFLAC__StreamDecoderState
 *    The current decoder state.
 */
OggFLAC__StreamDecoderState OggFLAC__stream_decoder_get_state(const OggFLAC__StreamDecoder *decoder);

/** Get the current number of channels in the stream being decoded.
 *  Will only be valid after decoding has started and will contain the
 *  value from the most recently decoded frame header.
 *
 * \param  decoder  A decoder instance to query.
 * \assert
 *    \code decoder != NULL \endcode
 * \retval unsigned
 *    See above.
 */
unsigned OggFLAC__stream_decoder_get_channels(const OggFLAC__StreamDecoder *decoder);

/** Get the current channel assignment in the stream being decoded.
 *  Will only be valid after decoding has started and will contain the
 *  value from the most recently decoded frame header.
 *
 * \param  decoder  A decoder instance to query.
 * \assert
 *    \code decoder != NULL \endcode
 * \retval OggFLAC__ChannelAssignment
 *    See above.
 */
OggFLAC__ChannelAssignment OggFLAC__stream_decoder_get_channel_assignment(const OggFLAC__StreamDecoder *decoder);

/** Get the current sample resolution in the stream being decoded.
 *  Will only be valid after decoding has started and will contain the
 *  value from the most recently decoded frame header.
 *
 * \param  decoder  A decoder instance to query.
 * \assert
 *    \code decoder != NULL \endcode
 * \retval unsigned
 *    See above.
 */
unsigned OggFLAC__stream_decoder_get_bits_per_sample(const OggFLAC__StreamDecoder *decoder);

/** Get the current sample rate in Hz of the stream being decoded.
 *  Will only be valid after decoding has started and will contain the
 *  value from the most recently decoded frame header.
 *
 * \param  decoder  A decoder instance to query.
 * \assert
 *    \code decoder != NULL \endcode
 * \retval unsigned
 *    See above.
 */
unsigned OggFLAC__stream_decoder_get_sample_rate(const OggFLAC__StreamDecoder *decoder);

/** Get the current blocksize of the stream being decoded.
 *  Will only be valid after decoding has started and will contain the
 *  value from the most recently decoded frame header.
 *
 * \param  decoder  A decoder instance to query.
 * \assert
 *    \code decoder != NULL \endcode
 * \retval unsigned
 *    See above.
 */
unsigned OggFLAC__stream_decoder_get_blocksize(const OggFLAC__StreamDecoder *decoder);

/** Initialize the decoder instance.
 *  Should be called after OggFLAC__stream_decoder_new() and
 *  OggFLAC__stream_decoder_set_*() but before any of the
 *  OggFLAC__stream_decoder_process_*() functions.  Will set and return the
 *  decoder state, which will be OggFLAC__STREAM_DECODER_SEARCH_FOR_METADATA
 *  if initialization succeeded.
 *
 * \param  decoder  An uninitialized decoder instance.
 * \assert
 *    \code decoder != NULL \endcode
 * \retval OggFLAC__StreamDecoderState
 *    \c OggFLAC__STREAM_DECODER_SEARCH_FOR_MEATADATA if initialization was
 *    successful; see OggFLAC__StreamDecoderState for the meanings of other
 *    return values.
 */
OggFLAC__StreamDecoderState OggFLAC__stream_decoder_init(OggFLAC__StreamDecoder *decoder);

/** Finish the decoding process.
 *  Flushes the decoding buffer, releases resources, resets the decoder
 *  settings to their defaults, and returns the decoder state to
 *  OggFLAC__STREAM_DECODER_UNINITIALIZED.
 *
 *  In the event of a prematurely-terminated decode, it is not strictly
 *  necessary to call this immediately before OggFLAC__stream_decoder_delete()
 *  but it is good practice to match every OggFLAC__stream_decoder_init()
 *  with a OggFLAC__stream_decoder_finish().
 *
 * \param  decoder  An uninitialized decoder instance.
 * \assert
 *    \code decoder != NULL \endcode
 */
void OggFLAC__stream_decoder_finish(OggFLAC__StreamDecoder *decoder);

/** Flush the stream input.
 *  The decoder's input buffer will be cleared and the state set to
 *  \c OggFLAC__STREAM_DECODER_SEARCH_FOR_FRAME_SYNC.
 *
 * \param  decoder  A decoder instance.
 * \assert
 *    \code decoder != NULL \endcode
 * \retval FLAC__bool
 *    \c true if successful, else \c false if a memory allocation
 *    error occurs.
 */
FLAC__bool OggFLAC__stream_decoder_flush(OggFLAC__StreamDecoder *decoder);

/** Reset the decoding process.
 *  The decoder's input buffer will be cleared and the state set to
 *  \c OggFLAC__STREAM_DECODER_SEARCH_FOR_METADATA.  This is similar to
 *  OggFLAC__stream_decoder_finish() except that the settings are
 *  preserved; there is no need to call OggFLAC__stream_decoder_init()
 *  before decoding again.
 *
 * \param  decoder  A decoder instance.
 * \assert
 *    \code decoder != NULL \endcode
 * \retval FLAC__bool
 *    \c true if successful, else \c false if a memory allocation
 *    error occurs.
 */
FLAC__bool OggFLAC__stream_decoder_reset(OggFLAC__StreamDecoder *decoder);

/** Decode one metadata block or audio frame.
 *  This version instructs the decoder to decode a either a single metadata
 *  block or a single frame and stop, unless the callbacks return a fatal
 *  error or the read callback returns
 *  \c OggFLAC__STREAM_DECODER_READ_STATUS_END_OF_STREAM.
 *
 *  As the decoder needs more input it will call the read callback.
 *  Depending on what was decoded, the metadata or write callback will be
 *  called with the decoded metadata block or audio frame, unless an error
 *  occurred.  If the decoder loses sync it will call the error callback
 *  instead.
 * 
 * \param  decoder  An initialized decoder instance in the state
 *                  \c OggFLAC__STREAM_DECODER_SEARCH_FOR_FRAME_SYNC.
 * \assert
 *    \code decoder != NULL \endcode
 *    \code OggFLAC__stream_decoder_get_state(decoder) == OggFLAC__STREAM_DECODER_SEARCH_FOR_FRAME_SYNC \endcode
 * \retval FLAC__bool
 *    \c false if any read or write error occurred (except
 *    \c OggFLAC__STREAM_DECODER_ERROR_STATUS_LOST_SYNC), else \c false;
 *    in any case, check the decoder state with
 *    OggFLAC__stream_decoder_get_state() to see what went wrong or to
 *    check for lost synchronization (a sign of stream corruption).
 */
FLAC__bool OggFLAC__stream_decoder_process_single(OggFLAC__StreamDecoder *decoder);

/** Decode until the end of the metadata.
 *  This version instructs the decoder to decode from the current position
 *  and continue until all the metadata has been read, or until the
 *  callbacks return a fatal error or the read callback returns
 *  \c OggFLAC__STREAM_DECODER_READ_STATUS_END_OF_STREAM.
 *
 *  As the decoder needs more input it will call the read callback.
 *  As each metadata block is decoded, the metadata callback will be called
 *  with the decoded metadata.  If the decoder loses sync it will call the
 *  error callback.
 * 
 * \param  decoder  An initialized decoder instance in the state
 *                  \c OggFLAC__STREAM_DECODER_SEARCH_FOR_METADATA.
 * \assert
 *    \code decoder != NULL \endcode
 *    \code OggFLAC__stream_decoder_get_state(decoder) == OggFLAC__STREAM_DECODER_SEARCH_FOR_METADATA \endcode
 * \retval FLAC__bool
 *    \c false if any read or write error occurred (except
 *    \c OggFLAC__STREAM_DECODER_ERROR_STATUS_LOST_SYNC), else \c false;
 *    in any case, check the decoder state with
 *    OggFLAC__stream_decoder_get_state() to see what went wrong or to
 *    check for lost synchronization (a sign of stream corruption).
 */
FLAC__bool OggFLAC__stream_decoder_process_until_end_of_metadata(OggFLAC__StreamDecoder *decoder);

/** Decode until the end of the stream.
 *  This version instructs the decoder to decode from the current position
 *  and continue until the end of stream (the read callback returns
 *  \c OggFLAC__STREAM_DECODER_READ_STATUS_END_OF_STREAM), or until the
 *  callbacks return a fatal error.
 *
 *  As the decoder needs more input it will call the read callback.
 *  As each metadata block and frame is decoded, the metadata or write
 *  callback will be called with the decoded metadata or frame.  If the
 *  decoder loses sync it will call the error callback.
 * 
 * \param  decoder  An initialized decoder instance in the state
 *                  \c OggFLAC__STREAM_DECODER_SEARCH_FOR_METADATA.
 * \assert
 *    \code decoder != NULL \endcode
 *    \code OggFLAC__stream_decoder_get_state(decoder) == OggFLAC__STREAM_DECODER_SEARCH_FOR_METADATA \endcode
 * \retval FLAC__bool
 *    \c false if any read or write error occurred (except
 *    \c OggFLAC__STREAM_DECODER_ERROR_STATUS_LOST_SYNC), else \c false;
 *    in any case, check the decoder state with
 *    OggFLAC__stream_decoder_get_state() to see what went wrong or to
 *    check for lost synchronization (a sign of stream corruption).
 */
FLAC__bool OggFLAC__stream_decoder_process_until_end_of_stream(OggFLAC__StreamDecoder *decoder);

/* \} */

#ifdef __cplusplus
}
#endif

#endif
