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

typedef struct {
	FLAC__byte *buffer;
	unsigned capacity; /* in bytes */
	unsigned bytes, bits;
	unsigned total_bits; /* must always == 8*bytes+bits */
	unsigned consumed_bytes, consumed_bits;
	unsigned total_consumed_bits; /* must always == 8*consumed_bytes+consumed_bits */
	FLAC__uint16 read_crc16;
} FLAC__BitBuffer;

void FLAC__bitbuffer_init(FLAC__BitBuffer *bb);
FLAC__bool FLAC__bitbuffer_init_from(FLAC__BitBuffer *bb, const FLAC__byte buffer[], unsigned bytes);
void FLAC__bitbuffer_init_read_crc16(FLAC__BitBuffer *bb, FLAC__uint16 seed);
FLAC__bool FLAC__bitbuffer_concatenate_aligned(FLAC__BitBuffer *dest, const FLAC__BitBuffer *src);
void FLAC__bitbuffer_free(FLAC__BitBuffer *bb); /* does not 'free(buffer)' */
FLAC__bool FLAC__bitbuffer_clear(FLAC__BitBuffer *bb);
FLAC__bool FLAC__bitbuffer_clone(FLAC__BitBuffer *dest, const FLAC__BitBuffer *src);
FLAC__bool FLAC__bitbuffer_write_zeroes(FLAC__BitBuffer *bb, unsigned bits);
FLAC__bool FLAC__bitbuffer_write_raw_uint32(FLAC__BitBuffer *bb, FLAC__uint32 val, unsigned bits);
FLAC__bool FLAC__bitbuffer_write_raw_int32(FLAC__BitBuffer *bb, FLAC__int32 val, unsigned bits);
FLAC__bool FLAC__bitbuffer_write_raw_uint64(FLAC__BitBuffer *bb, FLAC__uint64 val, unsigned bits);
FLAC__bool FLAC__bitbuffer_write_raw_int64(FLAC__BitBuffer *bb, FLAC__int64 val, unsigned bits);
FLAC__bool FLAC__bitbuffer_write_unary_unsigned(FLAC__BitBuffer *bb, unsigned val);
unsigned FLAC__bitbuffer_rice_bits(int val, unsigned parameter);
unsigned FLAC__bitbuffer_golomb_bits_signed(int val, unsigned parameter);
unsigned FLAC__bitbuffer_golomb_bits_unsigned(unsigned val, unsigned parameter);
FLAC__bool FLAC__bitbuffer_write_symmetric_rice_signed(FLAC__BitBuffer *bb, int val, unsigned parameter);
FLAC__bool FLAC__bitbuffer_write_symmetric_rice_signed_guarded(FLAC__BitBuffer *bb, int val, unsigned parameter, unsigned max_bits, FLAC__bool *overflow);
FLAC__bool FLAC__bitbuffer_write_symmetric_rice_signed_escape(FLAC__BitBuffer *bb, int val, unsigned parameter);
FLAC__bool FLAC__bitbuffer_write_rice_signed(FLAC__BitBuffer *bb, int val, unsigned parameter);
FLAC__bool FLAC__bitbuffer_write_rice_signed_guarded(FLAC__BitBuffer *bb, int val, unsigned parameter, unsigned max_bits, FLAC__bool *overflow);
FLAC__bool FLAC__bitbuffer_write_golomb_signed(FLAC__BitBuffer *bb, int val, unsigned parameter);
FLAC__bool FLAC__bitbuffer_write_golomb_signed_guarded(FLAC__BitBuffer *bb, int val, unsigned parameter, unsigned max_bits, FLAC__bool *overflow);
FLAC__bool FLAC__bitbuffer_write_golomb_unsigned(FLAC__BitBuffer *bb, unsigned val, unsigned parameter);
FLAC__bool FLAC__bitbuffer_write_golomb_unsigned_guarded(FLAC__BitBuffer *bb, unsigned val, unsigned parameter, unsigned max_bits, FLAC__bool *overflow);
FLAC__bool FLAC__bitbuffer_write_utf8_uint32(FLAC__BitBuffer *bb, FLAC__uint32 val);
FLAC__bool FLAC__bitbuffer_write_utf8_uint64(FLAC__BitBuffer *bb, FLAC__uint64 val);
FLAC__bool FLAC__bitbuffer_zero_pad_to_byte_boundary(FLAC__BitBuffer *bb);
FLAC__bool FLAC__bitbuffer_peek_bit(FLAC__BitBuffer *bb, unsigned *val, FLAC__bool (*read_callback)(FLAC__byte buffer[], unsigned *bytes, void *client_data), void *client_data);
FLAC__bool FLAC__bitbuffer_read_bit(FLAC__BitBuffer *bb, unsigned *val, FLAC__bool (*read_callback)(FLAC__byte buffer[], unsigned *bytes, void *client_data), void *client_data);
FLAC__bool FLAC__bitbuffer_read_bit_to_uint32(FLAC__BitBuffer *bb, FLAC__uint32 *val, FLAC__bool (*read_callback)(FLAC__byte buffer[], unsigned *bytes, void *client_data), void *client_data);
FLAC__bool FLAC__bitbuffer_read_bit_to_uint64(FLAC__BitBuffer *bb, FLAC__uint64 *val, FLAC__bool (*read_callback)(FLAC__byte buffer[], unsigned *bytes, void *client_data), void *client_data);
FLAC__bool FLAC__bitbuffer_read_raw_uint32(FLAC__BitBuffer *bb, FLAC__uint32 *val, const unsigned bits, FLAC__bool (*read_callback)(FLAC__byte buffer[], unsigned *bytes, void *client_data), void *client_data);
FLAC__bool FLAC__bitbuffer_read_raw_int32(FLAC__BitBuffer *bb, FLAC__int32 *val, const unsigned bits, FLAC__bool (*read_callback)(FLAC__byte buffer[], unsigned *bytes, void *client_data), void *client_data);
FLAC__bool FLAC__bitbuffer_read_raw_uint64(FLAC__BitBuffer *bb, FLAC__uint64 *val, const unsigned bits, FLAC__bool (*read_callback)(FLAC__byte buffer[], unsigned *bytes, void *client_data), void *client_data);
FLAC__bool FLAC__bitbuffer_read_raw_int64(FLAC__BitBuffer *bb, FLAC__int64 *val, const unsigned bits, FLAC__bool (*read_callback)(FLAC__byte buffer[], unsigned *bytes, void *client_data), void *client_data);
FLAC__bool FLAC__bitbuffer_read_unary_unsigned(FLAC__BitBuffer *bb, unsigned *val, FLAC__bool (*read_callback)(FLAC__byte buffer[], unsigned *bytes, void *client_data), void *client_data);
FLAC__bool FLAC__bitbuffer_read_symmetric_rice_signed(FLAC__BitBuffer *bb, int *val, unsigned parameter, FLAC__bool (*read_callback)(FLAC__byte buffer[], unsigned *bytes, void *client_data), void *client_data);
FLAC__bool FLAC__bitbuffer_read_rice_signed(FLAC__BitBuffer *bb, int *val, unsigned parameter, FLAC__bool (*read_callback)(FLAC__byte buffer[], unsigned *bytes, void *client_data), void *client_data);
FLAC__bool FLAC__bitbuffer_read_golomb_signed(FLAC__BitBuffer *bb, int *val, unsigned parameter, FLAC__bool (*read_callback)(FLAC__byte buffer[], unsigned *bytes, void *client_data), void *client_data);
FLAC__bool FLAC__bitbuffer_read_golomb_unsigned(FLAC__BitBuffer *bb, unsigned *val, unsigned parameter, FLAC__bool (*read_callback)(FLAC__byte buffer[], unsigned *bytes, void *client_data), void *client_data);
FLAC__bool FLAC__bitbuffer_read_utf8_uint32(FLAC__BitBuffer *bb, FLAC__uint32 *val, FLAC__bool (*read_callback)(FLAC__byte buffer[], unsigned *bytes, void *client_data), void *client_data, FLAC__byte *raw, unsigned *rawlen);
FLAC__bool FLAC__bitbuffer_read_utf8_uint64(FLAC__BitBuffer *bb, FLAC__uint64 *val, FLAC__bool (*read_callback)(FLAC__byte buffer[], unsigned *bytes, void *client_data), void *client_data, FLAC__byte *raw, unsigned *rawlen);
void FLAC__bitbuffer_dump(const FLAC__BitBuffer *bb, FILE *out);

#endif
