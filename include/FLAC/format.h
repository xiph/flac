/* libFLAC - Free Lossless Audio Coder library
 * Copyright (C) 2000  Josh Coalson
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
/*NOTE: only up to 24 because of the current predictor coefficient quantization and the fact we use int32s for all work */
#define FLAC__MAX_BITS_PER_SAMPLE (24u)
/* the following is ((2 ** 20) - 1) div 10 */
#define FLAC__MAX_SAMPLE_RATE (1048570u)
#define FLAC__MAX_LPC_ORDER (32u)
#define FLAC__MIN_QLP_COEFF_PRECISION (5u)
/* changing this also means changing all of fixed.c and more, so DON'T! */
#define FLAC__MAX_FIXED_ORDER (4u)
#define FLAC__MAX_RICE_PARTITION_ORDER (15u)

#define FLAC__VERSION_STRING "0.5"
extern const unsigned FLAC__MAJOR_VERSION;
extern const unsigned FLAC__MINOR_VERSION;

extern const byte     FLAC__STREAM_SYNC_STRING[4]; /* = "fLaC" */;
extern const unsigned FLAC__STREAM_SYNC; /* = 0x664C6143 */;
extern const unsigned FLAC__STREAM_SYNC_LEN; /* = 32 bits */;


/*****************************************************************************
 *
 * NOTE: Within the bitstream, all fixed-width numbers are big-endian coded.
 *       All numbers are unsigned unless otherwise noted.
 *
 *****************************************************************************/

typedef enum {
	FLAC__METADATA_TYPE_ENCODING = 0
} FLAC__MetaDataType;

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
} FLAC__StreamMetaData_Encoding;

extern const unsigned FLAC__STREAM_METADATA_ENCODING_MIN_BLOCK_SIZE_LEN; /* = 16 bits */
extern const unsigned FLAC__STREAM_METADATA_ENCODING_MAX_BLOCK_SIZE_LEN; /* = 16 bits */
extern const unsigned FLAC__STREAM_METADATA_ENCODING_MIN_FRAME_SIZE_LEN; /* = 24 bits */
extern const unsigned FLAC__STREAM_METADATA_ENCODING_MAX_FRAME_SIZE_LEN; /* = 24 bits */
extern const unsigned FLAC__STREAM_METADATA_ENCODING_SAMPLE_RATE_LEN; /* = 20 bits */
extern const unsigned FLAC__STREAM_METADATA_ENCODING_CHANNELS_LEN; /* = 3 bits */
extern const unsigned FLAC__STREAM_METADATA_ENCODING_BITS_PER_SAMPLE_LEN; /* = 5 bits */
extern const unsigned FLAC__STREAM_METADATA_ENCODING_TOTAL_SAMPLES_LEN; /* = 36 bits */
extern const unsigned FLAC__STREAM_METADATA_ENCODING_MD5SUM_LEN; /* = 128 bits */
extern const unsigned FLAC__STREAM_METADATA_ENCODING_LENGTH; /* = 34 bytes */

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
		FLAC__StreamMetaData_Encoding encoding;
	} data;
} FLAC__StreamMetaData;

extern const unsigned FLAC__STREAM_METADATA_IS_LAST_LEN; /* = 1 bits */
extern const unsigned FLAC__STREAM_METADATA_TYPE_LEN; /* = 7 bits */
extern const unsigned FLAC__STREAM_METADATA_LENGTH_LEN; /* = 24 bits */

/*****************************************************************************/

typedef enum {
	FLAC__ENTROPY_CODING_METHOD_PARTITIONED_RICE = 0
} FLAC__EntropyCodingMethodType;

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
	FLAC__CHANNEL_ASSIGNMENT_INDEPENDENT = 0,
	FLAC__CHANNEL_ASSIGNMENT_LEFT_SIDE = 1,
	FLAC__CHANNEL_ASSIGNMENT_RIGHT_SIDE = 2,
	FLAC__CHANNEL_ASSIGNMENT_MID_SIDE = 3
} FLAC__ChannelAssignment;

/*****************************************************************************
 *
 *  9: sync code '111111110'
 *  3: blocksize in samples
 *        000: get from stream header => implies constant blocksize throughout stream
 *        001: 192 samples (AES/EBU) => implies constant blocksize throughout stream
 *        010-101: 576 * (2^(2-n)) samples, i.e. 576/1152/2304/4608 => implies constant blocksize throughout stream
 *        110: get 8 bit (blocksize-1) from end of header => variable blocksize throughout stream unless it's the last frame
 *        111: get 16 bit (blocksize-1) from end of header => variable blocksize throughout stream unless it's the last frame
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
 *  8: CRC-8 (polynomial = x^8 + x^2 + x + 1) of everything before the crc, including the sync code
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
} FLAC__FrameHeader;

extern const unsigned FLAC__FRAME_HEADER_SYNC; /* = 0x1fe */
extern const unsigned FLAC__FRAME_HEADER_SYNC_LEN; /* = 9 bits */
extern const unsigned FLAC__FRAME_HEADER_BLOCK_SIZE_LEN; /* = 3 bits */
extern const unsigned FLAC__FRAME_HEADER_SAMPLE_RATE_LEN; /* = 4 bits */
extern const unsigned FLAC__FRAME_HEADER_CHANNEL_ASSIGNMENT_LEN; /* = 4 bits */
extern const unsigned FLAC__FRAME_HEADER_BITS_PER_SAMPLE_LEN; /* = 3 bits */
extern const unsigned FLAC__FRAME_HEADER_ZERO_PAD_LEN; /* = 1 bit */
extern const unsigned FLAC__FRAME_HEADER_CRC8_LEN; /* = 8 bits */

/*****************************************************************************/

typedef enum {
	FLAC__SUBFRAME_TYPE_CONSTANT = 0,
	FLAC__SUBFRAME_TYPE_VERBATIM = 1,
	FLAC__SUBFRAME_TYPE_FIXED = 2,
	FLAC__SUBFRAME_TYPE_LPC = 3
} FLAC__SubframeType;

/*****************************************************************************
 *
 * n: constant value for signal; n = frame's bits-per-sample
 */
typedef struct {
	int32 value;
} FLAC__SubframeHeader_Constant;

/*****************************************************************************
 *
 * n*i: unencoded signal; n = frame's bits-per-sample, i = frame's blocksize
 */
/* There is no (trivial) for structure FLAC__SubframeHeader_Verbatim */

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
} FLAC__SubframeHeader_Fixed;

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
} FLAC__SubframeHeader_LPC;

extern const unsigned FLAC__SUBFRAME_HEADER_LPC_QLP_COEFF_PRECISION_LEN; /* = 4 bits */
extern const unsigned FLAC__SUBFRAME_HEADER_LPC_QLP_SHIFT_LEN; /* = 5 bits */

/*****************************************************************************
 *
 *  8: subframe type
 *       xxxxxxx1: invalid, to prevent sync-fooling string of 1s (use to check for erroneous sync)
 *       00000000: constant value
 *       00000010: verbatim
 *       000001x0: reserved
 *       00001xx0: reserved
 *       0001xxx0: fixed predictor, xxx=order <= 4, else reserved
 *       001xxxx0: reserved
 *       01xxxxx0: lpc, xxxxx=order-1
 *       1xxxxxxx: invalid, to prevent sync-fooling string of 1s (use to check for erroneous sync)
 *  ?: subframe-specific header (c.f. FLAC__SubframeHeader_*)
 */
typedef struct {
	FLAC__SubframeType type;
	union {
		FLAC__SubframeHeader_Constant constant;
		FLAC__SubframeHeader_Fixed fixed;
		FLAC__SubframeHeader_LPC lpc;
	} data; /* data will be undefined for FLAC__SUBFRAME_TYPE_VERBATIM */
} FLAC__SubframeHeader;

extern const unsigned FLAC__SUBFRAME_HEADER_TYPE_CONSTANT; /* = 0x00 */
extern const unsigned FLAC__SUBFRAME_HEADER_TYPE_VERBATIM; /* = 0x02 */
extern const unsigned FLAC__SUBFRAME_HEADER_TYPE_FIXED; /* = 0x10 */
extern const unsigned FLAC__SUBFRAME_HEADER_TYPE_LPC; /* = 0x40 */
extern const unsigned FLAC__SUBFRAME_HEADER_TYPE_LEN; /* = 8 bits */

/*****************************************************************************/

#endif
