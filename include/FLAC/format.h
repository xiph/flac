/* libFLAC - Free Lossless Audio Codec library
 * Copyright (C) 2000,2001,2002  Josh Coalson
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

#ifndef FLAC__FORMAT_H
#define FLAC__FORMAT_H

#include "ordinals.h"

#ifdef __cplusplus
extern "C" {
#endif

/** \file include/FLAC/format.h
 *
 *  \brief
 *  This module contains structure definitions for the representation
 *  of FLAC format components in memory.  These are the basic
 *  structures used by the rest of the interfaces.
 *
 *  See the detailed documentation in the
 *  \link flac_format format \endlink module.
 */

/** \defgroup flac_format FLAC/format.h: format components
 *  \ingroup flac
 *
 *  \brief
 *  This module contains structure definitions for the representation
 *  of FLAC format components in memory.  These are the basic
 *  structures used by the rest of the interfaces.
 *
 *  First, you should be familiar with the XXX FLAC format XXX.  Many
 *  of the values here follow directly from the specification.  As a
 *  user of libFLAC, the interesting parts really are the structures
 *  that describe the frame header and metadata blocks.
 *
 *  The format structures here are very primitive, designed to store
 *  information in an efficient way.  Reading information from the
 *  structures is easy but creating or modifying them directly is
 *  more complex.  For the most part, as a user of a library, editing
 *  is not necessary; however, for metadata blocks it is, so there are
 *  convenience functions provided in the XXX metadata XXX module
 *  to simplify the manipulation of metadata blocks.
 *
 * \note
 * It's not the best convention, but symbols ending in _LEN are in bits
 * and _LENGTH are in bytes.  _LENGTH symbols are #defines instead of
 * global variables because they are usually used when declaring byte
 * arrays and some compilers require compile-time knowledge of array
 * sizes when declared on the stack.
 *
 * \{
 */


/*
	Most of the values described in this file are defined by the FLAC
	format specification.  There is nothing to tune here.
*/

/** The minimum block size, in samples, permitted by the format. */
#define FLAC__MIN_BLOCK_SIZE (16u)

/** The maximum block size, in samples, permitted by the format. */
#define FLAC__MAX_BLOCK_SIZE (65535u)

/** The maximum number of channels permitted by the format. */
#define FLAC__MAX_CHANNELS (8u)

/** The minimum sample resolution permitted by the format. */
#define FLAC__MIN_BITS_PER_SAMPLE (4u)

/** The maximum sample resolution permitted by the format. */
#define FLAC__MAX_BITS_PER_SAMPLE (32u)

/** The maximum sample resolution permitted by libFLAC.
 *
 * \warning
 * FLAC__MAX_BITS_PER_SAMPLE is the limit of the FLAC format.  However,
 * the reference encoder/decoder is currently limited to 24 bits because
 * of prevalent 32-bit math, so make sure and use this value when
 * appropriate.
 */
#define FLAC__REFERENCE_CODEC_MAX_BITS_PER_SAMPLE (24u)

/** The maximum sample rate permitted by the format.  The value is
 *  ((2 ** 16) - 1) * 10; see XXX format.html XXX as to why.
 */
#define FLAC__MAX_SAMPLE_RATE (655350u)

/** The maximum LPC order permitted by the format. */
#define FLAC__MAX_LPC_ORDER (32u)

/** The minimum quantized linear predictor coefficient precision
 *  permitted by the format.
 */
#define FLAC__MIN_QLP_COEFF_PRECISION (5u)

/** The maximum order of the fixed predictors permitted by the format. */
#define FLAC__MAX_FIXED_ORDER (4u)

/** The maximum Rice partition order permitted by the format. */
#define FLAC__MAX_RICE_PARTITION_ORDER (15u)

/* VERSION should come from configure */
#ifdef VERSION
/** The version string of the current library.
 *
 * \note
 * This does not correspond to the shared library version number, which
 * is used to determine binary compatibility.
 */
#define FLAC__VERSION_STRING VERSION
#endif

/** The byte string representation of the beginning of a FLAC stream. */
extern const FLAC__byte FLAC__STREAM_SYNC_STRING[4]; /* = "fLaC" */;

/** The 32-bit integer big-endian representation of the beginning of
 *  a FLAC stream.
 */
extern const unsigned FLAC__STREAM_SYNC; /* = 0x664C6143 */;

/** The length of the FLAC signature in bits. */
extern const unsigned FLAC__STREAM_SYNC_LEN; /* = 32 bits */;

/** The length of the FLAC signature in bytes. */
#define FLAC__STREAM_SYNC_LENGTH (4u)


/*****************************************************************************
 * @@@ REMOVE?
 * NOTE: Within the bitstream, all fixed-width numbers are big-endian coded.
 *       All numbers are unsigned unless otherwise noted.
 *
 *****************************************************************************/


/*****************************************************************************
 *
 * Subframe structures
 *
 *****************************************************************************/

/*****************************************************************************/

/** An enumeration of the available entropy coding methods. */
typedef enum {
	FLAC__ENTROPY_CODING_METHOD_PARTITIONED_RICE = 0
	/**< Residual is coded by partitioning into contexts, each with it's own
     * Rice parameter. */
} FLAC__EntropyCodingMethodType;

/** Maps a FLAC__EntropyCodingMethodType to a C string.
 *
 *  Using a FLAC__EntropyCodingMethodType as the index to this array will
 *  give the string equivalent.  The contents should not be modified.
 */
extern const char * const FLAC__EntropyCodingMethodTypeString[];


/** Header for a Rice partition.  (XXX format XXX)
 */
typedef struct {

	unsigned order;
	/**< The partition order, i.e. # of contexts = 2 ** order. */

	unsigned parameters[1 << FLAC__MAX_RICE_PARTITION_ORDER];
	/**< The Rice parameters for each context. */

	unsigned raw_bits[1 << FLAC__MAX_RICE_PARTITION_ORDER];
	/**< Widths for escape-coded partitions. */

} FLAC__EntropyCodingMethod_PartitionedRice;

extern const unsigned FLAC__ENTROPY_CODING_METHOD_PARTITIONED_RICE_ORDER_LEN; /**< == 4 (bits) */
extern const unsigned FLAC__ENTROPY_CODING_METHOD_PARTITIONED_RICE_PARAMETER_LEN; /**< == 4 (bits) */
extern const unsigned FLAC__ENTROPY_CODING_METHOD_PARTITIONED_RICE_RAW_LEN; /**< == 5 (bits) */

extern const unsigned FLAC__ENTROPY_CODING_METHOD_PARTITIONED_RICE_ESCAPE_PARAMETER;
/**< == (1<<FLAC__ENTROPY_CODING_METHOD_PARTITIONED_RICE_PARAMETER_LEN)-1 */

/** Header for the entropy coding method.  (XXX format XXX)
 */
typedef struct {
	FLAC__EntropyCodingMethodType type;
	union {
		FLAC__EntropyCodingMethod_PartitionedRice partitioned_rice;
	} data;
} FLAC__EntropyCodingMethod;

extern const unsigned FLAC__ENTROPY_CODING_METHOD_TYPE_LEN; /**< == 2 (bits) */

/*****************************************************************************/

/** An enumeration of the available subframe types. */
typedef enum {
	FLAC__SUBFRAME_TYPE_CONSTANT = 0, /**< constant signal */
	FLAC__SUBFRAME_TYPE_VERBATIM = 1, /**< uncompressed signal */
	FLAC__SUBFRAME_TYPE_FIXED = 2, /**< fixed polynomial prediction */
	FLAC__SUBFRAME_TYPE_LPC = 3 /**< linear prediction */
} FLAC__SubframeType;

/** Maps a FLAC__SubframeType to a C string.
 *
 *  Using a FLAC__SubframeType as the index to this array will
 *  give the string equivalent.  The contents should not be modified.
 */
extern const char * const FLAC__SubframeTypeString[];


/** CONSTANT subframe.  (XXX format XXX)
 */
typedef struct {
	FLAC__int32 value; /**< constant signal value */
} FLAC__Subframe_Constant;


/** VERBATIM subframe.  (XXX format XXX)
 */
typedef struct {
	const FLAC__int32 *data; /**< pointer to verbatim signal */
} FLAC__Subframe_Verbatim;


/** FIXED subframe.  (XXX format XXX)
 */
typedef struct {
	FLAC__EntropyCodingMethod entropy_coding_method;
	/**< residual coding method */

	unsigned order;
	/**< polynomial order */

	FLAC__int32 warmup[FLAC__MAX_FIXED_ORDER];
	/**< warmup samples to prime the predictor, length is order */

	const FLAC__int32 *residual;
	/**< residual signal, length is (blocksize minus order) samples */
} FLAC__Subframe_Fixed;


/** LPC subframe.  (XXX format XXX)
 */
typedef struct {
	FLAC__EntropyCodingMethod entropy_coding_method;
	/**< residual coding method */

	unsigned order;
	/**< FIR order */

	unsigned qlp_coeff_precision;
	/**< quantized FIR filter coefficient precision in bits

	int quantization_level;
	/**< qlp coeff shift needed */

	FLAC__int32 qlp_coeff[FLAC__MAX_LPC_ORDER];
	/**< FIR filter coefficients */

	FLAC__int32 warmup[FLAC__MAX_LPC_ORDER];
	/**< warmup samples to prime the predictor, length is order */

	const FLAC__int32 *residual;
	/**< residual signal, length is (blocksize minus order) samples */
} FLAC__Subframe_LPC;

extern const unsigned FLAC__SUBFRAME_LPC_QLP_COEFF_PRECISION_LEN; /**< == 4 (bits) */
extern const unsigned FLAC__SUBFRAME_LPC_QLP_SHIFT_LEN; /**< == 5 (bits) */


/** FLAC subframe structure.  (XXX format XXX)
 */
typedef struct {
	FLAC__SubframeType type;
	union {
		FLAC__Subframe_Constant constant;
		FLAC__Subframe_Fixed fixed;
		FLAC__Subframe_LPC lpc;
		FLAC__Subframe_Verbatim verbatim;
	} data;
	unsigned wasted_bits;
} FLAC__Subframe;

extern const unsigned FLAC__SUBFRAME_ZERO_PAD_LEN; /**< == 1 (bit) */
extern const unsigned FLAC__SUBFRAME_TYPE_LEN; /**< == 6 (bits) */
extern const unsigned FLAC__SUBFRAME_WASTED_BITS_FLAG_LEN; /**< == 1 (bit) */

extern const unsigned FLAC__SUBFRAME_TYPE_CONSTANT_BYTE_ALIGNED_MASK; /* = 0x00 */
extern const unsigned FLAC__SUBFRAME_TYPE_VERBATIM_BYTE_ALIGNED_MASK; /* = 0x02 */
extern const unsigned FLAC__SUBFRAME_TYPE_FIXED_BYTE_ALIGNED_MASK; /* = 0x10 */
extern const unsigned FLAC__SUBFRAME_TYPE_LPC_BYTE_ALIGNED_MASK; /* = 0x40 */

/*****************************************************************************/


/*****************************************************************************
 *
 * Frame structures
 *
 *****************************************************************************/

/** An enumeration of the available channel assignments. */
typedef enum {
	FLAC__CHANNEL_ASSIGNMENT_INDEPENDENT = 0, /**< independent channels */
	FLAC__CHANNEL_ASSIGNMENT_LEFT_SIDE = 1, /**< left+side stereo */
	FLAC__CHANNEL_ASSIGNMENT_RIGHT_SIDE = 2, /**< right+side stereo */
	FLAC__CHANNEL_ASSIGNMENT_MID_SIDE = 3 /**< mid+side stereo */
} FLAC__ChannelAssignment;

/** Maps a FLAC__ChannelAssignment to a C string.
 *
 *  Using a FLAC__ChannelAssignment as the index to this array will
 *  give the string equivalent.  The contents should not be modified.
 */
extern const char * const FLAC__ChannelAssignmentString[];

/** An enumeration of the possible frame numbering methods. */
typedef enum {
	FLAC__FRAME_NUMBER_TYPE_FRAME_NUMBER, /**< number contains the frame number */
	FLAC__FRAME_NUMBER_TYPE_SAMPLE_NUMBER /**< number contains the sample number of first sample in frame */
} FLAC__FrameNumberType;

/** Maps a FLAC__FrameNumberType to a C string.
 *
 *  Using a FLAC__FrameNumberType as the index to this array will
 *  give the string equivalent.  The contents should not be modified.
 */
extern const char * const FLAC__FrameNumberTypeString[];


/** FLAC frame header structure.  (XXX format XXX)
 */
typedef struct {
	unsigned blocksize;
	/**< number of samples per subframe */

	unsigned sample_rate;
	/**< sample rate in Hz */

	unsigned channels;
	/**< number of channels == number of subframes */

	FLAC__ChannelAssignment channel_assignment;
	/**< channel assignment for the frame */

	unsigned bits_per_sample;
	/**< sample resolution */

	FLAC__FrameNumberType number_type;
	/**< numbering scheme used for the frame */

	union {
		FLAC__uint32 frame_number;
		FLAC__uint64 sample_number;
	} number;
	/**< frame number or sample number of first sample in frame; check number_type */

	FLAC__uint8 crc;
	/**< CRC-8 (polynomial = x^8 + x^2 + x^1 + x^0, initialized with 0) of the
	 * raw frame header bytes, meaning everything before the CRC byte including
	 * the sync code
	 */
} FLAC__FrameHeader;

extern const unsigned FLAC__FRAME_HEADER_SYNC; /**< == 0x3ffe; the frame header sync code */
extern const unsigned FLAC__FRAME_HEADER_SYNC_LEN; /**< == 14 (bits) */
extern const unsigned FLAC__FRAME_HEADER_RESERVED_LEN; /**< == 2 (bits) */
extern const unsigned FLAC__FRAME_HEADER_BLOCK_SIZE_LEN; /**< == 4 (bits) */
extern const unsigned FLAC__FRAME_HEADER_SAMPLE_RATE_LEN; /**< == 4 (bits) */
extern const unsigned FLAC__FRAME_HEADER_CHANNEL_ASSIGNMENT_LEN; /**< == 4 (bits) */
extern const unsigned FLAC__FRAME_HEADER_BITS_PER_SAMPLE_LEN; /**< == 3 (bits) */
extern const unsigned FLAC__FRAME_HEADER_ZERO_PAD_LEN; /**< == 1 (bit) */
extern const unsigned FLAC__FRAME_HEADER_CRC_LEN; /**< == 8 (bits) */


/** FLAC frame footer structure.  (XXX format XXX)
 */
typedef struct {
	FLAC__uint16 crc;
	/**< CRC-16 (polynomial = x^16 + x^15 + x^2 + x^0, initialized with 0)
	 * of the bytes before the crc, back to and including the frame header
	 * sync code
	 */
} FLAC__FrameFooter;

extern const unsigned FLAC__FRAME_FOOTER_CRC_LEN; /**< == 16 (bits) */


/** FLAC frame structure.  (XXX format XXX)
 */
typedef struct {
	FLAC__FrameHeader header;
	FLAC__Subframe subframes[FLAC__MAX_CHANNELS];
	FLAC__FrameFooter footer;
} FLAC__Frame;

/*****************************************************************************/


/*****************************************************************************
 *
 * Meta-data structures
 *
 *****************************************************************************/

typedef enum {
	FLAC__METADATA_TYPE_STREAMINFO = 0,
	FLAC__METADATA_TYPE_PADDING = 1,
	FLAC__METADATA_TYPE_APPLICATION = 2,
	FLAC__METADATA_TYPE_SEEKTABLE = 3,
	FLAC__METADATA_TYPE_VORBIS_COMMENT = 4
} FLAC__MetadataType;
extern const char * const FLAC__MetadataTypeString[];

/*****************************************************************************
 *
 * 16: minimum blocksize (in samples) of all blocks in the stream
 * 16: maximum blocksize (in samples) of all blocks in the stream
 * 24: minimum framesize (in bytes) of all frames in the stream; 0 => unknown
 * 24: maximum framesize (in bytes) of all frames in the stream; 0 => unknown
 * 20: sample rate in Hz, 0 is invalid
 *  3: (number of channels)-1
 *  5: (bits per sample)-1
 * 36: total samples, 0 => unknown
 *128: MD5 digest of the original unencoded audio data
 *---- -----------------
 * 34  bytes total
 */
typedef struct {
	unsigned min_blocksize, max_blocksize;
	unsigned min_framesize, max_framesize;
	unsigned sample_rate;
	unsigned channels;
	unsigned bits_per_sample;
	FLAC__uint64 total_samples;
	FLAC__byte md5sum[16];
} FLAC__StreamMetadata_StreamInfo;

extern const unsigned FLAC__STREAM_METADATA_STREAMINFO_MIN_BLOCK_SIZE_LEN; /**< == 16 (bits) */
extern const unsigned FLAC__STREAM_METADATA_STREAMINFO_MAX_BLOCK_SIZE_LEN; /**< == 16 (bits) */
extern const unsigned FLAC__STREAM_METADATA_STREAMINFO_MIN_FRAME_SIZE_LEN; /**< == 24 (bits) */
extern const unsigned FLAC__STREAM_METADATA_STREAMINFO_MAX_FRAME_SIZE_LEN; /**< == 24 (bits) */
extern const unsigned FLAC__STREAM_METADATA_STREAMINFO_SAMPLE_RATE_LEN; /**< == 20 (bits) */
extern const unsigned FLAC__STREAM_METADATA_STREAMINFO_CHANNELS_LEN; /**< == 3 (bits) */
extern const unsigned FLAC__STREAM_METADATA_STREAMINFO_BITS_PER_SAMPLE_LEN; /**< == 5 (bits) */
extern const unsigned FLAC__STREAM_METADATA_STREAMINFO_TOTAL_SAMPLES_LEN; /**< == 36 (bits) */
extern const unsigned FLAC__STREAM_METADATA_STREAMINFO_MD5SUM_LEN; /**< == 128 (bits) */

#define FLAC__STREAM_METADATA_STREAMINFO_LENGTH (34u)

/*****************************************************************************
 *
 *   n: '0' bits
 *----- -----------------
 * n/8  bytes total
 */
typedef struct {
	int dummy; /* conceptually this is an empty struct since we don't store the padding bytes */
	           /* empty structs are allowed by C++ but not C, hence the 'dummy' */
} FLAC__StreamMetadata_Padding;

/*****************************************************************************
 *
 *    32: Registered application ID
 *     n: Application data
 *------- -----------------
 * 4+n/8  bytes total
 */
typedef struct {
	FLAC__byte id[4];
	FLAC__byte *data;
} FLAC__StreamMetadata_Application;

extern const unsigned FLAC__STREAM_METADATA_APPLICATION_ID_LEN; /**< == 32 (bits) */

/*****************************************************************************
 *
 *  64: sample number of target frame
 *  64: offset, in bytes, of target frame with respect to beginning of first frame
 *  16: number of samples in the target frame
 *----- -----------------
 *  18  bytes total
 */
typedef struct {
	FLAC__uint64 sample_number;
	FLAC__uint64 stream_offset;
	unsigned frame_samples;
} FLAC__StreamMetadata_SeekPoint;

extern const unsigned FLAC__STREAM_METADATA_SEEKPOINT_SAMPLE_NUMBER_LEN; /**< == 64 (bits) */
extern const unsigned FLAC__STREAM_METADATA_SEEKPOINT_STREAM_OFFSET_LEN; /**< == 64 (bits) */
extern const unsigned FLAC__STREAM_METADATA_SEEKPOINT_FRAME_SAMPLES_LEN; /**< == 16 (bits) */

#define FLAC__STREAM_METADATA_SEEKPOINT_LENGTH (18u)

extern const FLAC__uint64 FLAC__STREAM_METADATA_SEEKPOINT_PLACEHOLDER; /* = 0xffffffffffffffff */

/*****************************************************************************
 *
 *      0: num_points is implied by the metadata block 'length' field (i.e. num_points = length / 18)
 * n*18*8: seek points (n = num_points, 18 is the size of a seek point in bytes)
 * ------- -----------------
 *   n*18  bytes total
 *
 * NOTE: the seek points must be sorted by ascending sample number.
 * NOTE: each seek point's sample number must be the first sample of the target frame.
 * NOTE: each seek point's sample number must be unique within the table.
 * NOTE: existence of a SEEKTABLE block implies a correct setting of total_samples in the stream_info block.
 * NOTE: behavior is undefined when more than one SEEKTABLE block is present in a stream.
 */
typedef struct {
	unsigned num_points;
	FLAC__StreamMetadata_SeekPoint *points;
} FLAC__StreamMetadata_SeekTable;

/*****************************************************************************
 *
 *     32: Entry length in bytes (WATCHOUT: this is little-endian coded)
 *      n: Entry (n = 8 * length)
 *-------- -----------------
 * 32+n/8  bytes total
 */
typedef struct {
	FLAC__uint32 length;
	FLAC__byte *entry;
} FLAC__StreamMetadata_VorbisComment_Entry;

extern const unsigned FLAC__STREAM_METADATA_VORBIS_COMMENT_ENTRY_LENGTH_LEN; /**< == 32 (bits) */

/*****************************************************************************
 *
 *          n: Vendor string entry
 *         32: Number of comment fields (WATCHOUT: this is little-endian coded)
 *          m: Comment entries
 *------------ -----------------
 * (32+m+n)/8  bytes total
 */
typedef struct {
	FLAC__StreamMetadata_VorbisComment_Entry vendor_string;
	FLAC__uint32 num_comments;
	FLAC__StreamMetadata_VorbisComment_Entry *comments;
} FLAC__StreamMetadata_VorbisComment;

extern const unsigned FLAC__STREAM_METADATA_VORBIS_COMMENT_NUM_COMMENTS_LEN; /**< == 32 (bits) */

/*****************************************************************************
 *
 *  1: =1 if this is the last meta-data block, else =0
 *  7: meta-data type (c.f. FLAC__MetadataType)
 * 24: length (in bytes) of the block-specific data to follow
 *---- -----------------
 *  4  bytes total
 */
typedef struct {
	FLAC__MetadataType type;
	FLAC__bool is_last;
	unsigned length; /* in bytes */
	union {
		FLAC__StreamMetadata_StreamInfo stream_info;
		FLAC__StreamMetadata_Padding padding;
		FLAC__StreamMetadata_Application application;
		FLAC__StreamMetadata_SeekTable seek_table;
		FLAC__StreamMetadata_VorbisComment vorbis_comment;
	} data;
} FLAC__StreamMetadata;

extern const unsigned FLAC__STREAM_METADATA_IS_LAST_LEN; /**< == 1 (bit) */
extern const unsigned FLAC__STREAM_METADATA_TYPE_LEN; /**< == 7 (bits) */
extern const unsigned FLAC__STREAM_METADATA_LENGTH_LEN; /**< == 24 (bits) */

#define FLAC__STREAM_METADATA_HEADER_LENGTH (4u)

/*****************************************************************************/


/*****************************************************************************
 *
 * Utility functions
 *
 *****************************************************************************/

/*
 * Since the rules for valid sample rates are slightly complex, they are
 * encapsulated here:
 */
FLAC__bool FLAC__format_is_valid_sample_rate(unsigned sample_rate);

/* \} */

#ifdef __cplusplus
}
#endif

#endif
#if 0


/** \defgroup flac_metadata_level0 FLAC/metadata.h: metadata level 0 interface
 *  \ingroup flac_metadata
 * 
 *  \brief
 *  The level 0 interface consists of a single routine to read the
 *  STREAMINFO block.
 *
 *  It skips any ID3v2 tag at the head of the file.
 *
 * \{
 */

/** Read the STREAMINFO metadata block of the given FLAC file.  This function
 *  will skip any ID3v2 tag at the head of the file.
 *
 * \param filename    The path to the FLAC file to read.
 * \param streaminfo  A pointer to space for the STREAMINFO block.
 * \assert
 *    \code filename != NULL \endcode
 *    \code streaminfo != NULL \endcode
 * \retval FLAC__bool
 *    \c true if a valid STREAMINFO block was read from \a filename.  Returns
 *    \c false if there was a memory allocation error, a file decoder error,
 *    or the file contained no STREAMINFO block.
 */
FLAC__bool FLAC__metadata_get_streaminfo(const char *filename, FLAC__StreamMetadata *streaminfo);

/* \} */


/** \defgroup flac_metadata_level1 FLAC/metadata.h: metadata level 1 interface
 *  \ingroup flac_metadata
 * 
 * \brief
 * The level 1 interface provides read-write access to FLAC file metadata and
 * operates directly on the FLAC file.
 *
 * The general usage of this interface is:
 *
 * - Create an iterator using FLAC__metadata_simple_iterator_new()
 * - Attach it to a file using FLAC__metadata_simple_iterator_init() and check
 *   the exit code.  Call FLAC__metadata_simple_iterator_is_writable() to
 *   see if the file is writable, or read-only access is allowed.
 * - Use FLAC__metadata_simple_iterator_next() and
 *   FLAC__metadata_simple_iterator_prev() to move around the blocks.
 *   This is does not read the actual blocks themselves.
 *   FLAC__metadata_simple_iterator_next() is relatively fast.
 *   FLAC__metadata_simple_iterator_prev() is slower since it needs to search
 *   forward from the front of the file.
 * - Use FLAC__metadata_simple_iterator_get_block_type() or
 *   FLAC__metadata_simple_iterator_get_block() to access the actual data at
 *   the current iterator position.  The returned object is yours to modify
 *   and free.
 * - Use FLAC__metadata_simple_iterator_set_block() to write a modified block
 *   back.  You must have write permission to the original file.  Make sure to
 *   read the whole comment to FLAC__metadata_simple_iterator_set_block()
 *   below.
 * - Use FLAC__metadata_simple_iterator_insert_block_after() to add new blocks.
 *   Use the object creation functions from
 *   \link flac_metadata_object here \endlink to generate new objects.
 * - Use FLAC__metadata_simple_iterator_delete_block() to remove the block
 *   currently referred to by the iterator, or replace it with padding.
 * - Destroy the iterator with FLAC__metadata_simple_iterator_delete() when
 *   finished.
 *
 * \note
 * The FLAC file remains open the whole time between
 * FLAC__metadata_simple_iterator_init() and
 * FLAC__metadata_simple_iterator_delete(), so make sure you are not altering
 * the file during this time.
 *
 * \note
 * Do not modify the \a is_last, \a length, or \a type fields of returned
 * FLAC__MetadataType objects.  These are managed automatically.
 *
 * \note
 * If any of the modification functions
 * (FLAC__metadata_simple_iterator_set_block(),
 * FLAC__metadata_simple_iterator_delete_block(),
 * FLAC__metadata_simple_iterator_insert_block_after(), etc.) return \c false,
 * you should delete the iterator as it may no longer be valid.
 *
 * \{
 */

/** \typedef FLAC__Metadata_SimpleIterator
 *  The opaque structure definition for the level 1 iterator type.
 */
struct FLAC__Metadata_SimpleIterator;
typedef struct FLAC__Metadata_SimpleIterator FLAC__Metadata_SimpleIterator;

/** Status type for FLAC__Metadata_SimpleIterator.
 *
 *  The iterator's current status can be obtained by calling FLAC__metadata_simple_iterator_status().
 */
typedef enum {

	FLAC__METADATA_SIMPLE_ITERATOR_STATUS_OK = 0,
	/**< The iterator is in the normal OK state */

	FLAC__METADATA_SIMPLE_ITERATOR_STATUS_ILLEGAL_INPUT,
	/**< The data passed into a function violated the function's usage criteria */

	FLAC__METADATA_SIMPLE_ITERATOR_STATUS_ERROR_OPENING_FILE,
	/**< The iterator could not open the target file */

	FLAC__METADATA_SIMPLE_ITERATOR_STATUS_NOT_A_FLAC_FILE,
	/**< The iterator could not find the FLAC signature at the start of the file */

	FLAC__METADATA_SIMPLE_ITERATOR_STATUS_NOT_WRITABLE,
	/**< The iterator tried to write to a file that was not writable */

	FLAC__METADATA_SIMPLE_ITERATOR_STATUS_BAD_METADATA,
	/**< The iterator encountered input that does not conform to the FLAC metadata specification */

	FLAC__METADATA_SIMPLE_ITERATOR_STATUS_READ_ERROR,
	/**< The iterator encountered an error while reading the FLAC file */

	FLAC__METADATA_SIMPLE_ITERATOR_STATUS_SEEK_ERROR,
	/**< The iterator encountered an error while seeking in the FLAC file */

	FLAC__METADATA_SIMPLE_ITERATOR_STATUS_WRITE_ERROR,
	/**< The iterator encountered an error while writing the FLAC file */

	FLAC__METADATA_SIMPLE_ITERATOR_STATUS_RENAME_ERROR,
	/**< The iterator encountered an error renaming the FLAC file */

	FLAC__METADATA_SIMPLE_ITERATOR_STATUS_UNLINK_ERROR,
	/**< The iterator encountered an error removing the temporary file */

	FLAC__METADATA_SIMPLE_ITERATOR_STATUS_MEMORY_ALLOCATION_ERROR,
	/**< Memory allocation failed */

	FLAC__METADATA_SIMPLE_ITERATOR_STATUS_INTERNAL_ERROR
	/**< The caller violated an assertion or an unexpected error occurred */

} FLAC__Metadata_SimpleIteratorStatus;

/** Maps a FLAC__Metadata_SimpleIteratorStatus to a C string.
 *
 *  Using a FLAC__Metadata_SimpleIteratorStatus as the index to this array
 *  will give the string equivalent.  The contents should not be modified.
 */
extern const char * const FLAC__Metadata_SimpleIteratorStatusString[];


/** Create a new iterator instance.
 *
 * \retval FLAC__Metadata_SimpleIterator*
 *    \c NULL if there was an error allocating memory, else the new instance.
 */
FLAC__Metadata_SimpleIterator *FLAC__metadata_simple_iterator_new();

/** Free an iterator instance.  Deletes the object pointed to by \a iterator.
 *
 * \param iterator  A pointer to an existing iterator.
 * \assert
 *    \code iterator != NULL \endcode
 */
void FLAC__metadata_simple_iterator_delete(FLAC__Metadata_SimpleIterator *iterator);

/** Get the current status of the iterator.  Call this after a function
 *  returns \c false to get the reason for the error.  Also resets the status
 *  to FLAC__METADATA_SIMPLE_ITERATOR_STATUS_OK.
 *
 * \param iterator  A pointer to an existing iterator.
 * \assert
 *    \code iterator != NULL \endcode
 * \retval FLAC__Metadata_SimpleIteratorStatus
 *    The current status of the iterator.
 */
FLAC__Metadata_SimpleIteratorStatus FLAC__metadata_simple_iterator_status(FLAC__Metadata_SimpleIterator *iterator);

/** Initialize the iterator to point to the first metadata block in the
 *  given FLAC file.  If \a preserve_file_stats is \c true, the owner and
 *  modification time will be preserved even if the FLAC file is written.
 *
 * \param iterator             A pointer to an existing iterator.
 * \param filename             The path to the FLAC file.
 * \param preserve_file_stats  See above.
 * \assert
 *    \code iterator != NULL \endcode
 *    \code filename != NULL \endcode
 * \retval FLAC__bool
 *    \c false if a memory allocation error occurs, the file can't be
 *    opened, or another error occurs, else \c true.
 */
FLAC__bool FLAC__metadata_simple_iterator_init(FLAC__Metadata_SimpleIterator *iterator, const char *filename, FLAC__bool preserve_file_stats);

/** Returns \c true if the FLAC file is writable.  If \c false, calls to
 *  FLAC__metadata_simple_iterator_set_block() and
 *  FLAC__metadata_simple_iterator_insert_block_after() will fail.
 *
 * \param iterator             A pointer to an existing iterator.
 * \assert
 *    \code iterator != NULL \endcode
 * \retval FLAC__bool
 *    See above.
 */
FLAC__bool FLAC__metadata_simple_iterator_is_writable(const FLAC__Metadata_SimpleIterator *iterator);

/** Moves the iterator forward one metadata block, returning \c false if
 *  already at the end.
 *
 * \param iterator  A pointer to an existing initialized iterator.
 * \assert
 *    \code iterator != NULL \endcode
 *    \a iterator has been successfully initialized with
 *    FLAC__metadata_simple_iterator_init()
 * \retval FLAC__bool
 *    \c false if already at the last metadata block of the chain, else
 *    \c true.
 */
FLAC__bool FLAC__metadata_simple_iterator_next(FLAC__Metadata_SimpleIterator *iterator);

/** Moves the iterator backward one metadata block, returning \c false if
 *  already at the beginning.
 *
 * \param iterator  A pointer to an existing initialized iterator.
 * \assert
 *    \code iterator != NULL \endcode
 *    \a iterator has been successfully initialized with
 *    FLAC__metadata_simple_iterator_init()
 * \retval FLAC__bool
 *    \c false if already at the first metadata block of the chain, else
 *    \c true.
 */
FLAC__bool FLAC__metadata_simple_iterator_prev(FLAC__Metadata_SimpleIterator *iterator);

/** Get the type of the metadata block at the current position.  This
 *  avoids reading the actual block data which can save time for large
 *  blocks.
 *
 * \param iterator  A pointer to an existing initialized iterator.
 * \assert
 *    \code iterator != NULL \endcode
 *    \a iterator has been successfully initialized with
 *    FLAC__metadata_simple_iterator_init()
 * \retval FLAC__MetadataType
 *    The type of the metadata block at the current iterator position.
 */

FLAC__MetadataType FLAC__metadata_simple_iterator_get_block_type(const FLAC__Metadata_SimpleIterator *iterator);

/** Get the metadata block at the current position.  You can modify the
 *  block but must use FLAC__metadata_simple_iterator_set_block() to
 *  write it back to the FLAC file.
 *
 *  You must call FLAC__metadata_object_delete() on the returned object
 *  when you are finished with it.
 *
 * \param iterator  A pointer to an existing initialized iterator.
 * \assert
 *    \code iterator != NULL \endcode
 *    \a iterator has been successfully initialized with
 *    FLAC__metadata_simple_iterator_init()
 * \retval FLAC__StreamMetadata*
 *    The current metadata block.
 */
FLAC__StreamMetadata *FLAC__metadata_simple_iterator_get_block(FLAC__Metadata_SimpleIterator *iterator);

/** Write a block back to the FLAC file.  This function tries to be
 *  as efficient as possible; how the block is actually written is
 *  shown by the following:
 *
 *  Existing block is a STREAMINFO block and the new block is a
 *  STREAMINFO block: the new block is written in place.  Make sure
 *  you know what you're doing when changing the values of a
 *  STREAMINFO block.
 *
 *  Existing block is a STREAMINFO block and the new block is a
 *  not a STREAMINFO block: this is an error since the first block
 *  must be a STREAMINFO block.  Returns \c false without altering the
 *  file.
 *
 *  Existing block is not a STREAMINFO block and the new block is a
 *  STREAMINFO block: this is an error since there may be only one
 *  STREAMINFO block.  Returns \c false without altering the file.
 *
 *  Existing block and new block are the same length: the existing
 *  block will be replaced by the new block, written in place.
 *
 *  Existing block is longer than new block: if use_padding is \c true,
 *  the existing block will be overwritten in place with the new
 *  block followed by a PADDING block, if possible, to make the total
 *  size the same as the existing block.  Remember that a padding
 *  block requires at least four bytes so if the difference in size
 *  between the new block and existing block is less than that, the
 *  entire file will have to be rewritten, using the new block's
 *  exact size.  If use_padding is \c false, the entire file will be
 *  rewritten, replacing the existing block by the new block.
 *
 *  Existing block is shorter than new block: if use_padding is \c true,
 *  the function will try and expand the new block into the following
 *  PADDING block, if it exists and doing so won't shrink the PADDING
 *  block to less than 4 bytes.  If there is no following PADDING
 *  block, or it will shrink to less than 4 bytes, or use_padding is
 *  \c false, the entire file is rewritten, replacing the existing block
 *  with the new block.  Note that in this case any following PADDING
 *  block is preserved as is.
 *
 *  After writing the block, the iterator will remain in the same
 *  place, i.e. pointing to the new block.
 *
 * \param iterator     A pointer to an existing initialized iterator.
 * \param block        The block to set.
 * \param use_padding  See above.
 * \assert
 *    \code iterator != NULL \endcode
 *    \a iterator has been successfully initialized with
 *    FLAC__metadata_simple_iterator_init()
 *    \code block != NULL \endcode
 * \retval FLAC__bool
 *    \c true if successful, else \c false.
 */
FLAC__bool FLAC__metadata_simple_iterator_set_block(FLAC__Metadata_SimpleIterator *iterator, FLAC__StreamMetadata *block, FLAC__bool use_padding);

/** This is similar to FLAC__metadata_simple_iterator_set_block()
 *  except that instead of writing over an existing block, it appends
 *  a block after the existing block.  \a use_padding is again used to
 *  tell the function to try an expand into following padding in an
 *  attempt to avoid rewriting the entire file.
 *
 *  This function will fail and return \c false if given a STREAMINFO
 *  block.
 *
 *  After writing the block, the iterator will be pointing to the
 *  new block.
 *
 * \param iterator     A pointer to an existing initialized iterator.
 * \param block        The block to set.
 * \param use_padding  See above.
 * \assert
 *    \code iterator != NULL \endcode
 *    \a iterator has been successfully initialized with
 *    FLAC__metadata_simple_iterator_init()
 *    \code block != NULL \endcode
 * \retval FLAC__bool
 *    \c true if successful, else \c false.
 */
FLAC__bool FLAC__metadata_simple_iterator_insert_block_after(FLAC__Metadata_SimpleIterator *iterator, FLAC__StreamMetadata *block, FLAC__bool use_padding);

/** Deletes the block at the current position.  This will cause the
 *  entire FLAC file to be rewritten, unless \a use_padding is \c true,
 *  in which case the block will be replaced by an equal-sized PADDING
 *  block.  The iterator will be left pointing to the block before the
 *  one just deleted.
 *
 *  You may not delete the STREAMINFO block.
 *
 * \param iterator     A pointer to an existing initialized iterator.
 * \param use_padding  See above.
 * \assert
 *    \code iterator != NULL \endcode
 *    \a iterator has been successfully initialized with
 *    FLAC__metadata_simple_iterator_init()
 * \retval FLAC__bool
 *    \c true if successful, else \c false.
 */
FLAC__bool FLAC__metadata_simple_iterator_delete_block(FLAC__Metadata_SimpleIterator *iterator, FLAC__bool use_padding);

/* \} */


/** \defgroup flac_metadata_level2 FLAC/metadata.h: metadata level 2 interface
 *  \ingroup flac_metadata
 * 
 * \brief
 * The level 2 interface provides read-write access to FLAC file metadata;
 * all metadata is read into memory, operated on in memory, and then written
 * to file, which is more efficient than level 1 when editing multiple blocks.
 *
 * The general usage of this interface is:
 *
 * - Create a new chain using FLAC__metadata_chain_new().  A chain is a
 *   linked list of FLAC metadata blocks.
 * - Read all metadata into the the chain from a FLAC file using
 *   FLAC__metadata_chain_read() and check the status.
 * - Optionally, consolidate the padding using
 *   FLAC__metadata_chain_merge_padding() or
 *   FLAC__metadata_chain_sort_padding().
 * - Create a new iterator using FLAC__metadata_iterator_new()
 * - Initialize the iterator to point to the first element in the chain
 *   using FLAC__metadata_iterator_init()
 * - Traverse the chain using FLAC__metadata_iterator_next and
 *   FLAC__metadata_iterator_prev().
 * - Get a block for reading or modification using
 *   FLAC__metadata_iterator_get_block().  The pointer to the object
 *   inside the chain is returned, so the block is yours to modify.
 *   Changes will be reflected in the FLAC file when you write the
 *   chain.  You can also add and delete blocks (see functions below).
 * - When done, write out the chain using FLAC__metadata_chain_write().
 *   Make sure to read the whole comment to the function below.
 * - Delete the chain using FLAC__metadata_chain_delete().
 *
 * \note
 * Even though the FLAC file is not open while the chain is being
 * manipulated, you must not alter the file externally during
 * this time.  The chain assumes the FLAC file will not change
 * between the time of FLAC__metadata_chain_read() and
 * FLAC__metadata_chain_write().
 *
 * \note
 * Do not modify the is_last, length, or type fields of returned
 * FLAC__MetadataType objects.  These are managed automatically.
 *
 * \note
 * The metadata objects returned by FLAC__metadata_iterator_get_block()
 * are owned by the chain; do not FLAC__metadata_object_delete() them.
 * In the same way, blocks passed to FLAC__metadata_iterator_set_block()
 * become owned by the chain and they will be deleted when the chain is
 * deleted.
 *
 * \{
 */

/** \typedef FLAC__Metadata_Chain
 *  The opaque structure definition for the level 2 chain type.
 */
struct FLAC__Metadata_Chain;
typedef struct FLAC__Metadata_Chain FLAC__Metadata_Chain;

/** \typedef FLAC__Metadata_Iterator
 *  The opaque structure definition for the level 2 iterator type.
 */
struct FLAC__Metadata_Iterator;
typedef struct FLAC__Metadata_Iterator FLAC__Metadata_Iterator;

typedef enum {
	FLAC__METADATA_CHAIN_STATUS_OK = 0,
	/**< The chain is in the normal OK state */

	FLAC__METADATA_CHAIN_STATUS_ILLEGAL_INPUT,
	/**< The data passed into a function violated the function's usage criteria */

	FLAC__METADATA_CHAIN_STATUS_ERROR_OPENING_FILE,
	/**< The chain could not open the target file */

	FLAC__METADATA_CHAIN_STATUS_NOT_A_FLAC_FILE,
	/**< The chain could not find the FLAC signature at the start of the file */

	FLAC__METADATA_CHAIN_STATUS_NOT_WRITABLE,
	/**< The chain tried to write to a file that was not writable */

	FLAC__METADATA_CHAIN_STATUS_BAD_METADATA,
	/**< The chain encountered input that does not conform to the FLAC metadata specification */

	FLAC__METADATA_CHAIN_STATUS_READ_ERROR,
	/**< The chain encountered an error while reading the FLAC file */

	FLAC__METADATA_CHAIN_STATUS_SEEK_ERROR,
	/**< The chain encountered an error while seeking in the FLAC file */

	FLAC__METADATA_CHAIN_STATUS_WRITE_ERROR,
	/**< The chain encountered an error while writing the FLAC file */

	FLAC__METADATA_CHAIN_STATUS_RENAME_ERROR,
	/**< The chain encountered an error renaming the FLAC file */

	FLAC__METADATA_CHAIN_STATUS_UNLINK_ERROR,
	/**< The chain encountered an error removing the temporary file */

	FLAC__METADATA_CHAIN_STATUS_MEMORY_ALLOCATION_ERROR,
	/**< Memory allocation failed */

	FLAC__METADATA_CHAIN_STATUS_INTERNAL_ERROR
	/**< The caller violated an assertion or an unexpected error occurred */

} FLAC__Metadata_ChainStatus;

/** Maps a FLAC__Metadata_ChainStatus to a C string.
 *
 *  Using a FLAC__Metadata_ChainStatus as the index to this array
 *  will give the string equivalent.  The contents should not be modified.
 */
extern const char * const FLAC__Metadata_ChainStatusString[];

/*********** FLAC__Metadata_Chain ***********/

/** Create a new chain instance.
 *
 * \retval FLAC__Metadata_Chain*
 *    \c NULL if there was an error allocating memory, else the new instance.
 */
FLAC__Metadata_Chain *FLAC__metadata_chain_new();

/** Free a chain instance.  Deletes the object pointed to by \a chain.
 *
 * \param chain  A pointer to an existing chain.
 * \assert
 *    \code chain != NULL \endcode
 */
void FLAC__metadata_chain_delete(FLAC__Metadata_Chain *chain);

/** Get the current status of the chain.  Call this after a function
 *  returns \c false to get the reason for the error.  Also resets the
 *  status to FLAC__METADATA_CHAIN_STATUS_OK.
 *
 * \param chain    A pointer to an existing chain.
 * \assert
 *    \code chain != NULL \endcode
 * \retval FLAC__Metadata_ChainStatus
 *    The current status of the chain.
 */
FLAC__Metadata_ChainStatus FLAC__metadata_chain_status(FLAC__Metadata_Chain *chain);

/** Read all metadata from a FLAC file into the chain.
 *
 * \param chain    A pointer to an existing chain.
 * \param filename The path to the FLAC file to read.
 * \assert
 *    \code chain != NULL \endcode
 *    \code filename != NULL \endcode
 * \retval FLAC__bool
 *    \c true if a valid list of metadata blocks was read from
 *    \a filename, else \c false.  On failure, check the status with
 *    FLAC__metadata_chain_status().
 */
FLAC__bool FLAC__metadata_chain_read(FLAC__Metadata_Chain *chain, const char *filename);

/** Write all metadata out to the FLAC file.  This function tries to be as
 *  efficient as possible; how the metadata is actually written is shown by
 *  the following:
 *
 *  If the current chain is the same size as the existing metadata, the new
 *  data is written in place.
 *
 *  If the current chain is longer than the existing metadata, and
 *  \a use_padding is \c true, and the last block is a PADDING block of
 *  sufficient length, the function will truncate the final padding block
 *  so that the overall size of the metadata is the same as the existing
 *  metadata, and then just rewrite the metadata.  Otherwise, if not all of
 *  the above conditions are met, the entire FLAC file must be rewritten.
 *  If you want to use padding this way it is a good idea to call
 *  FLAC__metadata_chain_sort_padding() first so that you have the maximum
 *  amount of padding to work with, unless you need to preserve ordering
 *  of the PADDING blocks for some reason.
 *
 *  If the current chain is shorter than the existing metadata, and
 *  \a use_padding is \c true, and the final block is a PADDING block, the padding
 *  is extended to make the overall size the same as the existing data.  If
 *  \a use_padding is \c true and the last block is not a PADDING block, a new
 *  PADDING block is added to the end of the new data to make it the same
 *  size as the existing data (if possible, see the note to
 *  FLAC__metadata_simple_iterator_set_block() about the four byte limit)
 *  and the new data is written in place.  If none of the above apply or
 *  \a use_padding is \c false, the entire FLAC file is rewritten.
 *
 *  If \a preserve_file_stats is \c true, the owner and modification time will
 *  be preserved even if the FLAC file is written.
 *
 * \param chain               A pointer to an existing chain.
 * \param use_padding         See above.
 * \param preserve_file_stats See above.
 * \assert
 *    \code chain != NULL \endcode
 * \retval FLAC__bool
 *    \c true if the write succeeded, else \c false.  On failure,
 *    check the status with FLAC__metadata_chain_status().
 */
FLAC__bool FLAC__metadata_chain_write(FLAC__Metadata_Chain *chain, FLAC__bool use_padding, FLAC__bool preserve_file_stats);

/** Merge adjacent PADDING blocks into a single block.
 *
 * \note This function does not write to the FLAC file, it only
 * modifies the chain.
 *
 * \warning Any iterator on the current chain will become invalid after this
 * call.  You should delete the iterator and get a new one.
 *
 * \param chain               A pointer to an existing chain.
 * \assert
 *    \code chain != NULL \endcode
 */
void FLAC__metadata_chain_merge_padding(FLAC__Metadata_Chain *chain);

/** This function will move all PADDING blocks to the end on the metadata,
 *  then merge them into a single block.
 *
 * \note This function does not write to the FLAC file, it only
 * modifies the chain.
 *
 * \warning Any iterator on the current chain will become invalid after this
 * call.  You should delete the iterator and get a new one.
 *
 * \param chain  A pointer to an existing chain.
 * \assert
 *    \code chain != NULL \endcode
 */
void FLAC__metadata_chain_sort_padding(FLAC__Metadata_Chain *chain);


/*********** FLAC__Metadata_Iterator ***********/

/** Create a new iterator instance.
 *
 * \retval FLAC__Metadata_Iterator*
 *    \c NULL if there was an error allocating memory, else the new instance.
 */
FLAC__Metadata_Iterator *FLAC__metadata_iterator_new();

/** Free an iterator instance.  Deletes the object pointed to by \a iterator.
 *
 * \param iterator  A pointer to an existing iterator.
 * \assert
 *    \code iterator != NULL \endcode
 */
void FLAC__metadata_iterator_delete(FLAC__Metadata_Iterator *iterator);

/** Initialize the iterator to point to the first metadata block in the
 *  given chain.
 *
 * \param iterator  A pointer to an existing iterator.
 * \param chain     A pointer to an existing and initialized (read) chain.
 * \assert
 *    \code iterator != NULL \endcode
 *    \code chain != NULL \endcode
 */
void FLAC__metadata_iterator_init(FLAC__Metadata_Iterator *iterator, FLAC__Metadata_Chain *chain);

/** Moves the iterator forward one metadata block, returning \c false if
 *  already at the end.
 *
 * \param iterator  A pointer to an existing initialized iterator.
 * \assert
 *    \code iterator != NULL \endcode
 *    \a iterator has been successfully initialized with
 *    FLAC__metadata_iterator_init()
 * \retval FLAC__bool
 *    \c false if already at the last metadata block of the chain, else
 *    \c true.
 */
FLAC__bool FLAC__metadata_iterator_next(FLAC__Metadata_Iterator *iterator);

/** Moves the iterator backward one metadata block, returning \c false if
 *  already at the beginning.
 *
 * \param iterator  A pointer to an existing initialized iterator.
 * \assert
 *    \code iterator != NULL \endcode
 *    \a iterator has been successfully initialized with
 *    FLAC__metadata_iterator_init()
 * \retval FLAC__bool
 *    \c false if already at the first metadata block of the chain, else
 *    \c true.
 */
FLAC__bool FLAC__metadata_iterator_prev(FLAC__Metadata_Iterator *iterator);

/** Get the type of the metadata block at the current position.
 *
 * \param iterator  A pointer to an existing initialized iterator.
 * \assert
 *    \code iterator != NULL \endcode
 *    \a iterator has been successfully initialized with
 *    FLAC__metadata_iterator_init()
 * \retval FLAC__MetadataType
 *    The type of the metadata block at the current iterator position.
 */
FLAC__MetadataType FLAC__metadata_iterator_get_block_type(const FLAC__Metadata_Iterator *iterator);

/** Get the metadata block at the current position.  You can modify
 *  the block in place but must write the chain before the changes
 *  are reflected to the FLAC file.  You do not need to call
 *  FLAC__metadata_iterator_set_block() to reflect the changes;
 *  the pointer returned by FLAC__metadata_iterator_get_block()
 *  points directly into the chain.
 *
 * \warning
 * Do not call FLAC__metadata_object_delete() on the returned object;
 * to delete a block use FLAC__metadata_iterator_delete_block().
 *
 * \param iterator  A pointer to an existing initialized iterator.
 * \assert
 *    \code iterator != NULL \endcode
 *    \a iterator has been successfully initialized with
 *    FLAC__metadata_iterator_init()
 * \retval FLAC__StreamMetadata*
 *    The current metadata block.
 */
FLAC__StreamMetadata *FLAC__metadata_iterator_get_block(FLAC__Metadata_Iterator *iterator);

/** Set the metadata block at the current position, replacing the existing
 *  block.  The new block passed in becomes owned by the chain and it will be
 *  deleted when the chain is deleted.
 *
 * \param iterator  A pointer to an existing initialized iterator.
 * \param block     A pointer to a metadata block.
 * \assert
 *    \code iterator != NULL \endcode
 *    \a iterator has been successfully initialized with
 *    FLAC__metadata_iterator_init()
 *    \code block != NULL \endcode
 * \retval FLAC__bool
 *    \c false if the conditions in the above description are not met, or
 *    a memory allocation error occurs, otherwise \c true.
 */
FLAC__bool FLAC__metadata_iterator_set_block(FLAC__Metadata_Iterator *iterator, FLAC__StreamMetadata *block);

/** Removes the current block from the chain.  If \a replace_with_padding is
 *  \c true, the block will instead be replaced with a padding block of equal
 *  size.  You can not delete the STREAMINFO block.  The iterator will be
 *  left pointing to the block before the one just "deleted", even if
 *  \a replace_with_padding is \c true.
 *
 * \param iterator              A pointer to an existing initialized iterator.
 * \param replace_with_padding  See above.
 * \assert
 *    \code iterator != NULL \endcode
 *    \a iterator has been successfully initialized with
 *    FLAC__metadata_iterator_init()
 * \retval FLAC__bool
 *    \c false if the conditions in the above description are not met,
 *    otherwise \c true.
 */
FLAC__bool FLAC__metadata_iterator_delete_block(FLAC__Metadata_Iterator *iterator, FLAC__bool replace_with_padding);

/** Insert a new block before the current block.  You cannot insert a block
 *  before the first STREAMINFO block.  You cannot insert a STREAMINFO block
 *  as there can be only one, the one that already exists at the head when you
 *  read in a chain.  The chain takes ownership of the new block and it will be
 *  deleted when the chain is deleted.  The iterator will be left pointing to
 *  the new block.
 *
 * \param iterator  A pointer to an existing initialized iterator.
 * \param block     A pointer to a metadata block to insert.
 * \assert
 *    \code iterator != NULL \endcode
 *    \a iterator has been successfully initialized with
 *    FLAC__metadata_iterator_init()
 * \retval FLAC__bool
 *    \c false if the conditions in the above description are not met, or
 *    a memory allocation error occurs, otherwise \c true.
 */
FLAC__bool FLAC__metadata_iterator_insert_block_before(FLAC__Metadata_Iterator *iterator, FLAC__StreamMetadata *block);

/** Insert a new block after the current block.  You cannot insert a STREAMINFO
 *  block as there can be only one, the one that already exists at the head when
 *  you read in a chain.  The chain takes ownership of the new block and it will
 *  be deleted when the chain is deleted.  The iterator will be left pointing to
 *  the new block.
 *
 * \param iterator  A pointer to an existing initialized iterator.
 * \param block     A pointer to a metadata block to insert.
 * \assert
 *    \code iterator != NULL \endcode
 *    \a iterator has been successfully initialized with
 *    FLAC__metadata_iterator_init()
 * \retval FLAC__bool
 *    \c false if the conditions in the above description are not met, or
 *    a memory allocation error occurs, otherwise \c true.
 */
FLAC__bool FLAC__metadata_iterator_insert_block_after(FLAC__Metadata_Iterator *iterator, FLAC__StreamMetadata *block);

/* \} */


/** \defgroup flac_metadata_object FLAC/metadata.h: metadata object methods
 *  \ingroup flac_metadata
 *
 * \brief
 * This module contains methods for manipulating FLAC metadata objects.
 *
 * Since many are variable length we have to be careful about the memory
 * management.  We decree that all pointers to data in the object are
 * owned by the object and memory-managed by the object.
 *
 * Use the FLAC__metadata_object_new() and FLAC__metadata_object_delete()
 * functions to create all instances.  When using the
 * FLAC__metadata_object_set_*() functions to set pointers to data, set
 * \a copy to \c true to have the function make it's own copy of the data, or
 * to \c false to give the object ownership of your data.  In the latter case
 * your pointer must be freeable by free() and will be free()d when the object
 * is FLAC__metadata_object_delete()d.  It is legal to pass a null pointer as
 * the data pointer to a FLAC__metadata_object_set_*() function as long as
 * the length argument is 0 and the \a copy argument is \c false.
 *
 * The FLAC__metadata_object_new() and FLAC__metadata_object_clone() function
 * will return \c NULL in the case of a memory allocation error, otherwise a new
 * object.  The FLAC__metadata_object_set_*() functions return \c false in the
 * case of a memory allocation error.
 *
 * We don't have the convenience of C++ here, so note that the library relies
 * on you to keep the types straight.  In other words, if you pass, for
 * example, a FLAC__StreamMetadata* that represents a STREAMINFO block to
 * FLAC__metadata_object_application_set_data(), you will get an assertion
 * failure.
 *
 * There is no need to recalculate the length field on metadata blocks you
 * have modified.  They will be calculated automatically before they  are
 * written back to a file.
 *
 * \{
 */


/** Create a new metadata object instance of the given type.
 *
 *  The object will be "empty"; i.e. values and data pointers will be \c 0.
 *
 * \param type  Type of object to create
 * \retval FLAC__StreamMetadata*
 *    \c NULL if there was an error allocating memory, else the new instance.
 */
FLAC__StreamMetadata *FLAC__metadata_object_new(FLAC__MetadataType type);

/** Create a copy of an existing metadata object.
 *
 *  The copy is a "deep" copy, i.e. dynamically allocated data within the
 *  object is also copied.  The caller takes ownership of the new block and
 *  is responsible for freeing it with FLAC__metadata_object_delete().
 *
 * \param object  Pointer to object to copy.
 * \assert
 *    \code object != NULL \endcode
 * \retval FLAC__StreamMetadata*
 *    \c NULL if there was an error allocating memory, else the new instance.
 */
FLAC__StreamMetadata *FLAC__metadata_object_clone(const FLAC__StreamMetadata *object);

/** Free a metadata object.  Deletes the object pointed to by \a object.
 *
 *  The delete is a "deep" delete, i.e. dynamically allocated data within the
 *  object is also deleted.
 *
 * \param object  A pointer to an existing object.
 * \assert
 *    \code object != NULL \endcode
 */
void FLAC__metadata_object_delete(FLAC__StreamMetadata *object);

/* Does a deep comparison of the block data */
/** Compares two metadata objects.
 *
 *  The compare is "deep", i.e. dynamically allocated data within the
 *  object is also compared.
 *
 * \param block1  A pointer to an existing object.
 * \param block2  A pointer to an existing object.
 * \assert
 *    \code block1 != NULL \endcode
 *    \code block2 != NULL \endcode
 * \retval FLAC__bool
 *    \c true if objects are identical, else \c false.
 */
FLAC__bool FLAC__metadata_object_is_equal(const FLAC__StreamMetadata *block1, const FLAC__StreamMetadata *block2);

/** Sets the application data of an APPLICATION block.
 *
 *  If \a copy is \c true, a copy of the data is stored; otherwise, the object
 *  takes ownership of the pointer.  Returns \c false if \a copy == \c true
 *  and malloc fails.
 *
 * \param object  A pointer to an existing APPLICATION object.
 * \param data    A pointer to the data to set.
 * \param length  The length of \a data in bytes.
 * \param copy    See above.
 * \assert
 *    \code object != NULL \endcode
 *    \code object->type == FLAC__METADATA_TYPE_APPLICATION \endcode
 *    \code (data != NULL && length > 0) ||
 * (data == NULL && length == 0 && copy == false) \endcode
 * \retval FLAC__bool
 *    \c false if \a copy is \c true and malloc fails, else \c true.
 */
FLAC__bool FLAC__metadata_object_application_set_data(FLAC__StreamMetadata *object, FLAC__byte *data, unsigned length, FLAC__bool copy);

/** Resize the seekpoint array.
 *
 *  If the size shrinks, elements will truncated; if it grows, new placeholder
 *  points will be added to the end.
 *
 * \param object          A pointer to an existing SEEKTABLE object.
 * \param new_num_points  The desired length of the array; may be \c 0.
 * \assert
 *    \code object != NULL \endcode
 *    \code object->type == FLAC__METADATA_TYPE_SEEKTABLE \endcode
 *    \code (object->data.seek_table.points == NULL && object->data.seek_table.num_points == 0) ||
 * (object->data.seek_table.points != NULL && object->data.seek_table.num_points > 0) \endcode
 * \retval FLAC__bool
 *    \c false if memory allocation error, else \c true.
 */
FLAC__bool FLAC__metadata_object_seektable_resize_points(FLAC__StreamMetadata *object, unsigned new_num_points);

/** Set a seekpoint in a seektable.
 *
 * \param object     A pointer to an existing SEEKTABLE object.
 * \param point_num  Index into seekpoint array to set.
 * \param point      The point to set.
 * \assert
 *    \code object != NULL \endcode
 *    \code object->type == FLAC__METADATA_TYPE_SEEKTABLE \endcode
 *    \code object->data.seek_table.num_points > point_num \endcode
 */
void FLAC__metadata_object_seektable_set_point(FLAC__StreamMetadata *object, unsigned point_num, FLAC__StreamMetadata_SeekPoint point);

/** Insert a seekpoint into a seektable.
 *
 * \param object     A pointer to an existing SEEKTABLE object.
 * \param point_num  Index into seekpoint array to set.
 * \param point      The point to set.
 * \assert
 *    \code object != NULL \endcode
 *    \code object->type == FLAC__METADATA_TYPE_SEEKTABLE \endcode
 *    \code object->data.seek_table.num_points >= point_num \endcode
 * \retval FLAC__bool
 *    \c false if memory allocation error, else \c true.
 */
FLAC__bool FLAC__metadata_object_seektable_insert_point(FLAC__StreamMetadata *object, unsigned point_num, FLAC__StreamMetadata_SeekPoint point);

/** Delete a seekpoint from a seektable.
 *
 * \param object     A pointer to an existing SEEKTABLE object.
 * \param point_num  Index into seekpoint array to set.
 * \assert
 *    \code object != NULL \endcode
 *    \code object->type == FLAC__METADATA_TYPE_SEEKTABLE \endcode
 *    \code object->data.seek_table.num_points > point_num \endcode
 * \retval FLAC__bool
 *    \c false if memory allocation error, else \c true.
 */
FLAC__bool FLAC__metadata_object_seektable_delete_point(FLAC__StreamMetadata *object, unsigned point_num);

/** Check a seektable to see if it conforms to the FLAC specification.
 *  See the format specification for limits on the contents of the
 *  seektable.
 *
 * \param object     A pointer to an existing SEEKTABLE object.
 * \assert
 *    \code object != NULL \endcode
 *    \code object->type == FLAC__METADATA_TYPE_SEEKTABLE \endcode
 * \retval FLAC__bool
 *    \c false if seektable is illegal, else \c true.
 */
FLAC__bool FLAC__metadata_object_seektable_is_legal(const FLAC__StreamMetadata *object);

/** Sets the vendor string in a VORBIS_COMMENT block.
 *
 *  If \a copy is \c true, a copy of the entry is stored; otherwise, the object
 *  takes ownership of the \c entry->entry pointer.  Returns \c false if
 *  \a copy == \c true and malloc fails.
 *
 * \param object  A pointer to an existing VORBIS_COMMENT object.
 * \param entry   The entry to set the vendor string to.
 * \param copy    See above.
 * \assert
 *    \code object != NULL \endcode
 *    \code object->type == FLAC__METADATA_TYPE_VORBIS_COMMENT \endcode
 *    \code (entry->entry != NULL && entry->length > 0) ||
 * (entry->entry == NULL && entry->length == 0 && copy == false) \endcode
 * \retval FLAC__bool
 *    \c false if \a copy is \c true and malloc fails, else \c true.
 */
FLAC__bool FLAC__metadata_object_vorbiscomment_set_vendor_string(FLAC__StreamMetadata *object, FLAC__StreamMetadata_VorbisComment_Entry entry, FLAC__bool copy);

/** Resize the comment array.
 *
 *  If the size shrinks, elements will truncated; if it grows, new empty
 *  fields will be added to the end.
 *
 * \param object            A pointer to an existing VORBIS_COMMENT object.
 * \param new_num_comments  The desired length of the array; may be \c 0.
 * \assert
 *    \code object != NULL \endcode
 *    \code object->type == FLAC__METADATA_TYPE_VORBIS_COMMENT \endcode
 *    \code (object->data.vorbis_comment.comments == NULL && object->data.vorbis_comment.num_comments == 0) ||
 * (object->data.vorbis_comment.comments != NULL && object->data.vorbis_comment.num_comments > 0) \endcode
 * \retval FLAC__bool
 *    \c false if memory allocation error, else \c true.
 */
FLAC__bool FLAC__metadata_object_vorbiscomment_resize_comments(FLAC__StreamMetadata *object, unsigned new_num_comments);

/** Sets a comment in a VORBIS_COMMENT block.
 *
 *  If \a copy is \c true, a copy of the entry is stored; otherwise, the object
 *  takes ownership of the \c entry->entry pointer.  Returns \c false if
 *  \a copy == \c true and malloc fails.
 *
 * \param object       A pointer to an existing VORBIS_COMMENT object.
 * \param comment_num  Index into comment array to set.
 * \param entry        The entry to set the comment to.
 * \param copy         See above.
 * \assert
 *    \code object != NULL \endcode
 *    \code object->type == FLAC__METADATA_TYPE_VORBIS_COMMENT \endcode
 *    \code (entry->entry != NULL && entry->length > 0) ||
 * (entry->entry == NULL && entry->length == 0 && copy == false) \endcode
 * \retval FLAC__bool
 *    \c false if \a copy is \c true and malloc fails, else \c true.
 */
FLAC__bool FLAC__metadata_object_vorbiscomment_set_comment(FLAC__StreamMetadata *object, unsigned comment_num, FLAC__StreamMetadata_VorbisComment_Entry entry, FLAC__bool copy);

/** Insert a comment in a VORBIS_COMMENT block at the given index.
 *
 *  If \a copy is \c true, a copy of the entry is stored; otherwise, the object
 *  takes ownership of the \c entry->entry pointer.  Returns \c false if
 *  \a copy == \c true and malloc fails.
 *
 * \param object       A pointer to an existing VORBIS_COMMENT object.
 * \param comment_num  The index at which to insert the comment.  The comments
 *                     at and after \a comment_num move right one position.
 *                     To append a comment to the end, set \a comment_num to
 *                     \c object->data.vorbis_comment.num_comments .
 * \param entry        The comment to insert.
 * \param copy         See above.
 * \assert
 *    \code object != NULL \endcode
 *    \code object->type == FLAC__METADATA_TYPE_VORBIS_COMMENT \endcode
 *    \code object->data.vorbis_comment.num_comments >= comment_num \endcode
 *    \code (entry->entry != NULL && entry->length > 0) ||
 * (entry->entry == NULL && entry->length == 0 && copy == false) \endcode
 * \retval FLAC__bool
 *    \c false if \a copy is \c true and malloc fails, else \c true.
 */
FLAC__bool FLAC__metadata_object_vorbiscomment_insert_comment(FLAC__StreamMetadata *object, unsigned comment_num, FLAC__StreamMetadata_VorbisComment_Entry entry, FLAC__bool copy);

/** Delete a comment in a VORBIS_COMMENT block at the given index.
 *
 *  If \a copy is \c true, a copy of the entry is stored; otherwise, the object
 *  takes ownership of the \c entry->entry pointer.  Returns \c false if
 *  \a copy == \c true and malloc fails.
 *
 * \param object       A pointer to an existing VORBIS_COMMENT object.
 * \param comment_num  The index of the comment to delete.
 * \assert
 *    \code object != NULL \endcode
 *    \code object->type == FLAC__METADATA_TYPE_VORBIS_COMMENT \endcode
 *    \code object->data.vorbis_comment.num_comments > comment_num \endcode
 *    \code (entry->entry != NULL && entry->length > 0) ||
 * (entry->entry == NULL && entry->length == 0 && copy == false) \endcode
 * \retval FLAC__bool
 *    \c false if realloc fails, else \c true.
 */
FLAC__bool FLAC__metadata_object_vorbiscomment_delete_comment(FLAC__StreamMetadata *object, unsigned comment_num);

/* \} */

#endif
