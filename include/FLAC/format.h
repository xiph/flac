/* libFLAC - Free Lossless Audio Codec library
 * Copyright (C) 2000,2001  Josh Coalson
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

/* changing the following values to be higher will break the framing and hence the stream format, so DON'T! */
#define FLAC__MIN_BLOCK_SIZE (16u)
#define FLAC__MAX_BLOCK_SIZE (65535u)
#define FLAC__MAX_CHANNELS (8u)
#define FLAC__MIN_BITS_PER_SAMPLE (4u)
/*NOTE: only up to 24 because of the current predictor coefficient quantization and the fact we use int32s for all work */
#define FLAC__MAX_BITS_PER_SAMPLE (24u)
/* the following is ((2 ** 20) - 1) div 10 */
#define FLAC__MAX_SAMPLE_RATE (1048570u)
#define FLAC__MAX_LPC_ORDER (32u)
#define FLAC__MIN_QLP_COEFF_PRECISION (5u)
/* changing this also means changing all of fixed.c and more, so DON'T! */
#define FLAC__MAX_FIXED_ORDER (4u)
#define FLAC__MAX_RICE_PARTITION_ORDER (15u)

/* VERSION should come from configure */
#ifdef VERSION
#define FLAC__VERSION_STRING VERSION
#else
#define FLAC__VERSION_STRING "0.9"
#endif

extern const byte     FLAC__STREAM_SYNC_STRING[4]; /* = "fLaC" */;
extern const unsigned FLAC__STREAM_SYNC; /* = 0x664C6143 */;
extern const unsigned FLAC__STREAM_SYNC_LEN; /* = 32 bits */;


/*****************************************************************************
 *
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

typedef enum {
	FLAC__ENTROPY_CODING_METHOD_PARTITIONED_RICE = 0
} FLAC__EntropyCodingMethodType;
extern const char *FLAC__EntropyCodingMethodTypeString[];

/*****************************************************************************
 *
 *  4: partition order => (2 ** order) subdivisions
 */
typedef struct {
	unsigned order;
	unsigned parameters[1 << FLAC__MAX_RICE_PARTITION_ORDER];
} FLAC__EntropyCodingMethod_PartitionedRice;

extern const unsigned FLAC__ENTROPY_CODING_METHOD_PARTITIONED_RICE_ORDER_LEN; /* = 4 bits */
extern const unsigned FLAC__ENTROPY_CODING_METHOD_PARTITIONED_RICE_PARAMETER_LEN; /* = 4 bits */

/*****************************************************************************
 *
 *  2: entropy coding method:
 *     00: partitioned rice coding
 *     01-11: reserved
 *  ?: entropy coding method data
 */
typedef struct {
	FLAC__EntropyCodingMethodType type;
	union {
		FLAC__EntropyCodingMethod_PartitionedRice partitioned_rice;
	} data;
} FLAC__EntropyCodingMethod;

extern const unsigned FLAC__ENTROPY_CODING_METHOD_TYPE_LEN; /* = 2 bits */

/*****************************************************************************/

typedef enum {
	FLAC__SUBFRAME_TYPE_CONSTANT = 0,
	FLAC__SUBFRAME_TYPE_VERBATIM = 1,
	FLAC__SUBFRAME_TYPE_FIXED = 2,
	FLAC__SUBFRAME_TYPE_LPC = 3
} FLAC__SubframeType;
extern const char *FLAC__SubframeTypeString[];

/*****************************************************************************
 *
 * n: constant value for signal; n = frame's bits-per-sample
 */
typedef struct {
	int32 value;
} FLAC__Subframe_Constant;

/*****************************************************************************
 *
 * n*i: unencoded signal; n = frame's bits-per-sample, i = frame's blocksize
 */
typedef struct {
	const int32 *data;
} FLAC__Subframe_Verbatim;

/*****************************************************************************
 *
 *  n: unencoded warm-up samples (n = fixed-predictor order * bits per sample)
 *  ?: entropy coding method info
 *  ?: encoded residual ((blocksize minus fixed-predictor order) samples)
 *  The order is stored in the main subframe header
 */
typedef struct {
	FLAC__EntropyCodingMethod entropy_coding_method;
	unsigned order;
	int32 warmup[FLAC__MAX_FIXED_ORDER];
	const int32 *residual;
} FLAC__Subframe_Fixed;

/*****************************************************************************
 *
 *  n: unencoded warm-up samples (n = lpc order * bits per sample)
 *  4: (qlp coeff precision in bits)-1 (1111 = invalid, use to check for erroneous sync)
 *  5: qlp shift needed in bits (signed)
 *  n: unencoded predictor coefficients (n = lpc order * qlp coeff precision)
 *  ?: entropy coding method info
 *  ?: encoded residual ((blocksize minus lpc order) samples)
 *  The order is stored in the main subframe header
 */
typedef struct {
	FLAC__EntropyCodingMethod entropy_coding_method;
	unsigned order;
	unsigned qlp_coeff_precision;
	int quantization_level;
	int32 qlp_coeff[FLAC__MAX_LPC_ORDER];
	int32 warmup[FLAC__MAX_LPC_ORDER];
	const int32 *residual;
} FLAC__Subframe_LPC;

extern const unsigned FLAC__SUBFRAME_LPC_QLP_COEFF_PRECISION_LEN; /* = 4 bits */
extern const unsigned FLAC__SUBFRAME_LPC_QLP_SHIFT_LEN; /* = 5 bits */

/*****************************************************************************
 *
 *  1: zero pad, to prevent sync-fooling string of 1s (use to check for erroneous sync)
 *  6: subframe type
 *       000000: constant value
 *       000001: verbatim
 *       00001x: reserved
 *       0001xx: reserved
 *       001xxx: fixed predictor, xxx=order <= 4, else reserved
 *       01xxxx: reserved
 *       1xxxxx: lpc, xxxxx=order-1
 *  1: 'wasted bits' flag
 *       0: no wasted bits in source subblock
 *       1: all samples in source subblock contain n 0 least significant bits.  n-1 follows, unary coded, i.e. n=3, 001 follows, n=7, 0000001 follows.
 *             ?: unary coded (n-1)
 *  ?: subframe-specific data (c.f. FLAC__Subframe_*)
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

extern const unsigned FLAC__SUBFRAME_ZERO_PAD_LEN; /* = 1 bit */
extern const unsigned FLAC__SUBFRAME_TYPE_LEN; /* = 6 bits */
extern const unsigned FLAC__SUBFRAME_WASTED_BITS_FLAG_LEN; /* = 1 bit */

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

typedef enum {
	FLAC__CHANNEL_ASSIGNMENT_INDEPENDENT = 0,
	FLAC__CHANNEL_ASSIGNMENT_LEFT_SIDE = 1,
	FLAC__CHANNEL_ASSIGNMENT_RIGHT_SIDE = 2,
	FLAC__CHANNEL_ASSIGNMENT_MID_SIDE = 3
} FLAC__ChannelAssignment;
extern const char *FLAC__ChannelAssignmentString[];

/*****************************************************************************
 *
 * 14: sync code '11111111111110'
 *  2: reserved
 *        00: currently required value
 *        01-11: reserved
 *  4: blocksize in samples
 *        0000: get from stream header => implies constant blocksize throughout stream
 *        0001: 192 samples (AES/EBU) => implies constant blocksize throughout stream
 *        0010-0101: 576 * (2^(n-2)) samples, i.e. 576/1152/2304/4608 => implies constant blocksize throughout stream
 *        0110: get 8 bit (blocksize-1) from end of header => possibly variable blocksize throughout stream unless it's the last frame
 *        0111: get 16 bit (blocksize-1) from end of header => possibly variable blocksize throughout stream unless it's the last frame
 *        1000-1111: 256 * (2^(n-8)) samples, i.e. 256/512/1024/2048/4096/8192/16384/32768 => implies constant blocksize throughout stream
 *  4: sample rate:
 *        0000: get from stream header
 *        0001-0011: reserved
 *        0100: 8kHz
 *        0101: 16kHz
 *        0110: 22.05kHz
 *        0111: 24kHz
 *        1000: 32kHz
 *        1001: 44.1kHz
 *        1010: 48kHz
 *        1011: 96kHz
 *        1100: get 8 bit sample rate (in kHz) from end of header
 *        1101: get 16 bit sample rate (in Hz) from end of header
 *        1110: get 16 bit sample rate (in tens of Hz) from end of header
 *        1111: invalid, to prevent sync-fooling string of 1s (use to check for erroneous sync)
 *  4: channel assignment
 *     0000-0111: (number of independent channels)-1.  when == 0001, channel 0 is the left channel and channel 1 is the right
 *     1000: left/side stereo : channel 0 is the left             channel, channel 1 is the side(difference) channel
 *     1001: right/side stereo: channel 0 is the side(difference) channel, channel 1 is the right            channel
 *     1010: mid/side stereo  : channel 0 is the mid(average)     channel, channel 1 is the side(difference) channel
 *     1011-1111: reserved
 *  3: sample size in bits
 *        000: get from stream header
 *        001: 8 bits per sample
 *        010: 12 bits per sample
 *        011: reserved
 *        100: 16 bits per sample
 *        101: 20 bits per sample
 *        110: 24 bits per sample
 *        111: reserved
 *  1: zero pad, to prevent sync-fooling string of 1s (use to check for erroneous sync)
 *  ?: if(variable blocksize)
 *        8-56: 'UTF-8' coded sample number (decoded number is 0-36 bits) (use to check for erroneous sync)
 *     else
 *        8-48: 'UTF-8' coded frame number (decoded number is 0-31 bits) (use to check for erroneous sync)
 *  ?: if(blocksize bits == 11x)
 *        8/16 bit (blocksize-1)
 *  ?: if(sample rate bits == 11xx)
 *        8/16 bit sample rate
 *  8: CRC-8 (polynomial = x^8 + x^2 + x^1 + x^0, initialized with 0) of everything before the crc, including the sync code
 */
typedef struct {
	unsigned blocksize; /* in samples */
	unsigned sample_rate; /* in Hz */
	unsigned channels;
	FLAC__ChannelAssignment channel_assignment;
	unsigned bits_per_sample;
	union {
		uint32 frame_number;
		uint64 sample_number;
	} number;
	uint8 crc;
} FLAC__FrameHeader;

extern const unsigned FLAC__FRAME_HEADER_SYNC; /* = 0x3ffe */
extern const unsigned FLAC__FRAME_HEADER_SYNC_LEN; /* = 14 bits */
extern const unsigned FLAC__FRAME_HEADER_RESERVED_LEN; /* = 2 bits */
extern const unsigned FLAC__FRAME_HEADER_BLOCK_SIZE_LEN; /* = 4 bits */
extern const unsigned FLAC__FRAME_HEADER_SAMPLE_RATE_LEN; /* = 4 bits */
extern const unsigned FLAC__FRAME_HEADER_CHANNEL_ASSIGNMENT_LEN; /* = 4 bits */
extern const unsigned FLAC__FRAME_HEADER_BITS_PER_SAMPLE_LEN; /* = 3 bits */
extern const unsigned FLAC__FRAME_HEADER_ZERO_PAD_LEN; /* = 1 bit */
extern const unsigned FLAC__FRAME_HEADER_CRC_LEN; /* = 8 bits */

/*****************************************************************************
 *
 * 16: CRC-16 (polynomial = x^16 + x^15 + x^2 + x^0, initialized with 0) of everything before the crc, back to and including the frame header sync code
 */
typedef struct {
	uint16 crc;
} FLAC__FrameFooter;

extern const unsigned FLAC__FRAME_FOOTER_CRC_LEN; /* = 16 bits */

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
	FLAC__METADATA_TYPE_SEEKTABLE = 3
} FLAC__MetaDataType;
extern const char *FLAC__MetaDataTypeString[];

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
	uint64 total_samples;
	byte md5sum[16];
} FLAC__StreamMetaData_StreamInfo;

extern const unsigned FLAC__STREAM_METADATA_STREAMINFO_MIN_BLOCK_SIZE_LEN; /* = 16 bits */
extern const unsigned FLAC__STREAM_METADATA_STREAMINFO_MAX_BLOCK_SIZE_LEN; /* = 16 bits */
extern const unsigned FLAC__STREAM_METADATA_STREAMINFO_MIN_FRAME_SIZE_LEN; /* = 24 bits */
extern const unsigned FLAC__STREAM_METADATA_STREAMINFO_MAX_FRAME_SIZE_LEN; /* = 24 bits */
extern const unsigned FLAC__STREAM_METADATA_STREAMINFO_SAMPLE_RATE_LEN; /* = 20 bits */
extern const unsigned FLAC__STREAM_METADATA_STREAMINFO_CHANNELS_LEN; /* = 3 bits */
extern const unsigned FLAC__STREAM_METADATA_STREAMINFO_BITS_PER_SAMPLE_LEN; /* = 5 bits */
extern const unsigned FLAC__STREAM_METADATA_STREAMINFO_TOTAL_SAMPLES_LEN; /* = 36 bits */
extern const unsigned FLAC__STREAM_METADATA_STREAMINFO_MD5SUM_LEN; /* = 128 bits */
extern const unsigned FLAC__STREAM_METADATA_STREAMINFO_LENGTH; /* = 34 bytes */

/*****************************************************************************
 *
 *   n: '0' bits
 *----- -----------------
 * n/8  bytes total
 */
typedef struct {
	int dummy; /* conceptually this is an empty struct since we don't store the padding bytes */
	           /* empty structs are allowed by C++ but not C, hence the 'dummy' */
} FLAC__StreamMetaData_Padding;

/*****************************************************************************
 *
 *    32: Registered application ID
 *     n: Application data
 *------- -----------------
 * 4+n/8  bytes total
 */
typedef struct {
	byte id[16];
	byte *data;
} FLAC__StreamMetaData_Application;

extern const unsigned FLAC__STREAM_METADATA_APPLICATION_ID_LEN; /* = 32 bits */

/*****************************************************************************
 *
 *  64: sample number
 *  64: offset, in bytes, from beginning of first frame to target frame
 *  16: offset, in samples, from the first sample in the target frame to the target sample
 *----- -----------------
 *  18  bytes total
 */
typedef struct {
	uint64 sample_number;
	uint64 stream_offset;
	unsigned block_offset;
} FLAC__StreamMetaData_SeekPoint;

extern const unsigned FLAC__STREAM_METADATA_SEEKPOINT_SAMPLE_NUMBER_LEN; /* = 64 bits */
extern const unsigned FLAC__STREAM_METADATA_SEEKPOINT_STREAM_OFFSET_LEN; /* = 64 bits */
extern const unsigned FLAC__STREAM_METADATA_SEEKPOINT_BLOCK_OFFSET_LEN; /* = 16 bits */
extern const unsigned FLAC__STREAM_METADATA_SEEKPOINT_LEN; /* = 18 bytes */

/*****************************************************************************
 *
 *      0: num_points is implied by the metadata block 'length' field (i.e. num_points = length / 18)
 * n*18*8: seek points (n = num_points, 18 is the size of a seek point in bytes)
 * ------- -----------------
 *   n*18  bytes total
 *
 * NOTE: the seek points must be sorted by ascending sample number.
 * NOTE: each seek point's sample number must be unique within the table.
 * NOTE: existence of a SEEKTABLE block implies a correct setting of total_samples in the stream_info block.
 * NOTE: behavior is undefined when more than one SEEKTABLE block is present in a stream.
 */
typedef struct {
	unsigned num_points;
	FLAC__StreamMetaData_SeekPoint *points;
} FLAC__StreamMetaData_SeekTable;

/*****************************************************************************
 *
 *  1: =1 if this is the last meta-data block, else =0
 *  7: meta-data type (c.f. FLAC__MetaDataType)
 * 24: length (in bytes) of the block-specific data to follow
 *---- -----------------
 *  4  bytes total
 */
typedef struct {
	FLAC__MetaDataType type;
	bool is_last;
	unsigned length; /* in bytes */
	union {
		FLAC__StreamMetaData_StreamInfo stream_info;
		FLAC__StreamMetaData_Padding padding;
		FLAC__StreamMetaData_Application application;
		FLAC__StreamMetaData_SeekTable seek_table;
	} data;
} FLAC__StreamMetaData;

extern const unsigned FLAC__STREAM_METADATA_IS_LAST_LEN; /* = 1 bits */
extern const unsigned FLAC__STREAM_METADATA_TYPE_LEN; /* = 7 bits */
extern const unsigned FLAC__STREAM_METADATA_LENGTH_LEN; /* = 24 bits */

/*****************************************************************************/


/*****************************************************************************
 *
 * Stream structures
 *
 *****************************************************************************/

typedef struct {
	FLAC__StreamMetaData_StreamInfo stream_info;
	FLAC__Frame *frames;
} FLAC__Stream;

/*****************************************************************************/

#endif
