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

#ifndef FLAC__PRIVATE__BITBUFFER_H
#define FLAC__PRIVATE__BITBUFFER_H

#include <stdio.h> /* for FILE */
#include "FLAC/ordinals.h"

/* @@@ This should be configurable. Valid values are currently 8 and 32. */
/* @@@ WATCHOUT!  do not use 32 with a little endian system yet. */
#define FLAC__BITS_PER_BLURB 8

#if FLAC__BITS_PER_BLURB == 8
typedef FLAC__byte FLAC__blurb;
#elif FLAC__BITS_PER_BLURB == 32
typedef FLAC__uint32 FLAC__blurb;
#else
/* ERROR, only sizes of 8 and 32 are supported */
#endif

/*
 * opaque structure definition
 */
struct FLAC__BitBuffer;
typedef struct FLAC__BitBuffer FLAC__BitBuffer;

/*
 * construction, deletion, initialization, cloning functions
 */
FLAC__BitBuffer *FLAC__bitbuffer_new();
void FLAC__bitbuffer_delete(FLAC__BitBuffer *bb);
FLAC__bool FLAC__bitbuffer_init(FLAC__BitBuffer *bb);
FLAC__bool FLAC__bitbuffer_init_from(FLAC__BitBuffer *bb, const FLAC__byte buffer[], unsigned bytes);
FLAC__bool FLAC__bitbuffer_concatenate_aligned(FLAC__BitBuffer *dest, const FLAC__BitBuffer *src);
void FLAC__bitbuffer_free(FLAC__BitBuffer *bb); /* does not 'free(buffer)' */
FLAC__bool FLAC__bitbuffer_clear(FLAC__BitBuffer *bb);
FLAC__bool FLAC__bitbuffer_clone(FLAC__BitBuffer *dest, const FLAC__BitBuffer *src);

/*
 * CRC functions
 */
void FLAC__bitbuffer_reset_read_crc16(FLAC__BitBuffer *bb, FLAC__uint16 seed);
FLAC__uint16 FLAC__bitbuffer_get_read_crc16(FLAC__BitBuffer *bb);
FLAC__uint16 FLAC__bitbuffer_get_write_crc16(const FLAC__BitBuffer *bb);
FLAC__byte FLAC__bitbuffer_get_write_crc8(const FLAC__BitBuffer *bb);

/*
 * info functions
 */
FLAC__bool FLAC__bitbuffer_is_byte_aligned(const FLAC__BitBuffer *bb);
FLAC__bool FLAC__bitbuffer_is_consumed_byte_aligned(const FLAC__BitBuffer *bb);
unsigned FLAC__bitbuffer_bits_left_for_byte_alignment(const FLAC__BitBuffer *bb);
unsigned FLAC__bitbuffer_get_input_bytes_unconsumed(const FLAC__BitBuffer *bb); /* do not call unless byte-aligned */

/*
 * direct buffer access
 */
void FLAC__bitbuffer_get_buffer(FLAC__BitBuffer *bb, const FLAC__byte **buffer, unsigned *bytes);
void FLAC__bitbuffer_release_buffer(FLAC__BitBuffer *bb);

/*
 * write functions
 */
FLAC__bool FLAC__bitbuffer_write_zeroes(FLAC__BitBuffer *bb, unsigned bits);
FLAC__bool FLAC__bitbuffer_write_raw_uint32(FLAC__BitBuffer *bb, FLAC__uint32 val, unsigned bits);
FLAC__bool FLAC__bitbuffer_write_raw_int32(FLAC__BitBuffer *bb, FLAC__int32 val, unsigned bits);
FLAC__bool FLAC__bitbuffer_write_raw_uint64(FLAC__BitBuffer *bb, FLAC__uint64 val, unsigned bits);
#if 0 /* UNUSED */
FLAC__bool FLAC__bitbuffer_write_raw_int64(FLAC__BitBuffer *bb, FLAC__int64 val, unsigned bits);
#endif
FLAC__bool FLAC__bitbuffer_write_raw_uint32_little_endian(FLAC__BitBuffer *bb, FLAC__uint32 val); /*only for bits=32*/
FLAC__bool FLAC__bitbuffer_write_byte_block(FLAC__BitBuffer *bb, const FLAC__byte vals[], unsigned nvals);
FLAC__bool FLAC__bitbuffer_write_unary_unsigned(FLAC__BitBuffer *bb, unsigned val);
unsigned FLAC__bitbuffer_rice_bits(int val, unsigned parameter);
#if 0 /* UNUSED */
unsigned FLAC__bitbuffer_golomb_bits_signed(int val, unsigned parameter);
unsigned FLAC__bitbuffer_golomb_bits_unsigned(unsigned val, unsigned parameter);
#endif
#ifdef FLAC__SYMMETRIC_RICE
FLAC__bool FLAC__bitbuffer_write_symmetric_rice_signed(FLAC__BitBuffer *bb, int val, unsigned parameter);
#if 0 /* UNUSED */
FLAC__bool FLAC__bitbuffer_write_symmetric_rice_signed_guarded(FLAC__BitBuffer *bb, int val, unsigned parameter, unsigned max_bits, FLAC__bool *overflow);
#endif
FLAC__bool FLAC__bitbuffer_write_symmetric_rice_signed_escape(FLAC__BitBuffer *bb, int val, unsigned parameter);
#endif
FLAC__bool FLAC__bitbuffer_write_rice_signed(FLAC__BitBuffer *bb, int val, unsigned parameter);
#if 0 /* UNUSED */
FLAC__bool FLAC__bitbuffer_write_rice_signed_guarded(FLAC__BitBuffer *bb, int val, unsigned parameter, unsigned max_bits, FLAC__bool *overflow);
#endif
#if 0 /* UNUSED */
FLAC__bool FLAC__bitbuffer_write_golomb_signed(FLAC__BitBuffer *bb, int val, unsigned parameter);
FLAC__bool FLAC__bitbuffer_write_golomb_signed_guarded(FLAC__BitBuffer *bb, int val, unsigned parameter, unsigned max_bits, FLAC__bool *overflow);
FLAC__bool FLAC__bitbuffer_write_golomb_unsigned(FLAC__BitBuffer *bb, unsigned val, unsigned parameter);
FLAC__bool FLAC__bitbuffer_write_golomb_unsigned_guarded(FLAC__BitBuffer *bb, unsigned val, unsigned parameter, unsigned max_bits, FLAC__bool *overflow);
#endif
FLAC__bool FLAC__bitbuffer_write_utf8_uint32(FLAC__BitBuffer *bb, FLAC__uint32 val);
FLAC__bool FLAC__bitbuffer_write_utf8_uint64(FLAC__BitBuffer *bb, FLAC__uint64 val);
FLAC__bool FLAC__bitbuffer_zero_pad_to_byte_boundary(FLAC__BitBuffer *bb);

/*
 * read functions
 */
FLAC__bool FLAC__bitbuffer_peek_bit(FLAC__BitBuffer *bb, unsigned *val, FLAC__bool (*read_callback)(FLAC__byte buffer[], unsigned *bytes, void *client_data), void *client_data);
FLAC__bool FLAC__bitbuffer_read_bit(FLAC__BitBuffer *bb, unsigned *val, FLAC__bool (*read_callback)(FLAC__byte buffer[], unsigned *bytes, void *client_data), void *client_data);
FLAC__bool FLAC__bitbuffer_read_bit_to_uint32(FLAC__BitBuffer *bb, FLAC__uint32 *val, FLAC__bool (*read_callback)(FLAC__byte buffer[], unsigned *bytes, void *client_data), void *client_data);
FLAC__bool FLAC__bitbuffer_read_bit_to_uint64(FLAC__BitBuffer *bb, FLAC__uint64 *val, FLAC__bool (*read_callback)(FLAC__byte buffer[], unsigned *bytes, void *client_data), void *client_data);
FLAC__bool FLAC__bitbuffer_read_raw_uint32(FLAC__BitBuffer *bb, FLAC__uint32 *val, const unsigned bits, FLAC__bool (*read_callback)(FLAC__byte buffer[], unsigned *bytes, void *client_data), void *client_data);
FLAC__bool FLAC__bitbuffer_read_raw_int32(FLAC__BitBuffer *bb, FLAC__int32 *val, const unsigned bits, FLAC__bool (*read_callback)(FLAC__byte buffer[], unsigned *bytes, void *client_data), void *client_data);
FLAC__bool FLAC__bitbuffer_read_raw_uint64(FLAC__BitBuffer *bb, FLAC__uint64 *val, const unsigned bits, FLAC__bool (*read_callback)(FLAC__byte buffer[], unsigned *bytes, void *client_data), void *client_data);
#if 0 /* UNUSED */
FLAC__bool FLAC__bitbuffer_read_raw_int64(FLAC__BitBuffer *bb, FLAC__int64 *val, const unsigned bits, FLAC__bool (*read_callback)(FLAC__byte buffer[], unsigned *bytes, void *client_data), void *client_data);
#endif
FLAC__bool FLAC__bitbuffer_read_raw_uint32_little_endian(FLAC__BitBuffer *bb, FLAC__uint32 *val, FLAC__bool (*read_callback)(FLAC__byte buffer[], unsigned *bytes, void *client_data), void *client_data); /*only for bits=32*/
FLAC__bool FLAC__bitbuffer_read_byte_block_aligned(FLAC__BitBuffer *bb, FLAC__byte *val, unsigned nvals, FLAC__bool (*read_callback)(FLAC__byte buffer[], unsigned *bytes, void *client_data), void *client_data); /* val may be 0 to skip bytes instead of reading them */
FLAC__bool FLAC__bitbuffer_read_unary_unsigned(FLAC__BitBuffer *bb, unsigned *val, FLAC__bool (*read_callback)(FLAC__byte buffer[], unsigned *bytes, void *client_data), void *client_data);
#ifdef FLAC__SYMMETRIC_RICE
FLAC__bool FLAC__bitbuffer_read_symmetric_rice_signed(FLAC__BitBuffer *bb, int *val, unsigned parameter, FLAC__bool (*read_callback)(FLAC__byte buffer[], unsigned *bytes, void *client_data), void *client_data);
#endif
FLAC__bool FLAC__bitbuffer_read_rice_signed(FLAC__BitBuffer *bb, int *val, unsigned parameter, FLAC__bool (*read_callback)(FLAC__byte buffer[], unsigned *bytes, void *client_data), void *client_data);
FLAC__bool FLAC__bitbuffer_read_rice_signed_block(FLAC__BitBuffer *bb, int vals[], unsigned nvals, unsigned parameter, FLAC__bool (*read_callback)(FLAC__byte buffer[], unsigned *bytes, void *client_data), void *client_data);
#if 0 /* UNUSED */
FLAC__bool FLAC__bitbuffer_read_golomb_signed(FLAC__BitBuffer *bb, int *val, unsigned parameter, FLAC__bool (*read_callback)(FLAC__byte buffer[], unsigned *bytes, void *client_data), void *client_data);
FLAC__bool FLAC__bitbuffer_read_golomb_unsigned(FLAC__BitBuffer *bb, unsigned *val, unsigned parameter, FLAC__bool (*read_callback)(FLAC__byte buffer[], unsigned *bytes, void *client_data), void *client_data);
#endif
FLAC__bool FLAC__bitbuffer_read_utf8_uint32(FLAC__BitBuffer *bb, FLAC__uint32 *val, FLAC__bool (*read_callback)(FLAC__byte buffer[], unsigned *bytes, void *client_data), void *client_data, FLAC__byte *raw, unsigned *rawlen);
FLAC__bool FLAC__bitbuffer_read_utf8_uint64(FLAC__BitBuffer *bb, FLAC__uint64 *val, FLAC__bool (*read_callback)(FLAC__byte buffer[], unsigned *bytes, void *client_data), void *client_data, FLAC__byte *raw, unsigned *rawlen);
void FLAC__bitbuffer_dump(const FLAC__BitBuffer *bb, FILE *out);

#endif
