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

#include <assert.h>
#include <stdlib.h> /* for malloc() */
#include <string.h> /* for memcpy(), memset() */
#include "private/bitbuffer.h"
#include "private/bitmath.h"
#include "private/crc.h"

/*
 * Along the way you will see two versions of some functions, selected
 * by a FLAC__NO_MANUAL_INLINING macro.  One is the simplified, more
 * readable, and slow version, and the other is the same function
 * where crucial parts have been manually inlined and are much faster.
 *
 */

/* This should be at least twice as large as the largest number of bytes required to represent any 'number' (in any encoding) you are going to read. */
static const unsigned FLAC__BITBUFFER_DEFAULT_CAPACITY = 65536; /* bytes */

#define BYTE_BIT_TO_MASK(b) (((byte)'\x80') >> (b))

#ifdef min
#undef min
#endif
#define min(x,y) ((x)<(y)?(x):(y))
#ifdef max
#undef max
#endif
#define max(x,y) ((x)>(y)?(x):(y))

static bool bitbuffer_resize_(FLAC__BitBuffer *bb, unsigned new_capacity)
{
	byte *new_buffer;

	assert(bb != 0);
	assert(bb->buffer != 0);

	if(bb->capacity == new_capacity)
		return true;

	new_buffer = (byte*)malloc(sizeof(byte) * new_capacity);
	if(new_buffer == 0)
		return false;
	memset(new_buffer, 0, new_capacity);
	memcpy(new_buffer, bb->buffer, sizeof(byte)*min(bb->bytes+(bb->bits?1:0), new_capacity));
	if(new_capacity < bb->bytes+(bb->bits?1:0)) {
		bb->bytes = new_capacity;
		bb->bits = 0;
		bb->total_bits = (new_capacity<<3);
	}
	if(new_capacity < bb->consumed_bytes+(bb->consumed_bits?1:0)) {
		bb->consumed_bytes = new_capacity;
		bb->consumed_bits = 0;
		bb->total_consumed_bits = (new_capacity<<3);
	}
	bb->buffer = new_buffer;
	bb->capacity = new_capacity;
	return true;
}

static bool bitbuffer_grow_(FLAC__BitBuffer *bb, unsigned min_bytes_to_add)
{
	unsigned new_capacity;

	assert(min_bytes_to_add > 0);

	new_capacity = max(bb->capacity * 4, bb->capacity + min_bytes_to_add);
	return bitbuffer_resize_(bb, new_capacity);
}

static bool bitbuffer_ensure_size_(FLAC__BitBuffer *bb, unsigned bits_to_add)
{
	assert(bb != 0);
	assert(bb->buffer != 0);
	if((bb->capacity<<3) < bb->total_bits + bits_to_add)
		return bitbuffer_grow_(bb, (bits_to_add>>3)+2);
	else
		return true;
}

static bool bitbuffer_read_from_client_(FLAC__BitBuffer *bb, bool (*read_callback)(byte buffer[], unsigned *bytes, void *client_data), void *client_data)
{
	unsigned bytes;

	/* first shift the unconsumed buffer data toward the front as much as possible */
	if(bb->total_consumed_bits >= 8) {
		unsigned l = 0, r = bb->consumed_bytes, r_end = bb->bytes;
		for( ; r < r_end; l++, r++)
			bb->buffer[l] = bb->buffer[r];
		for( ; l < r_end; l++)
			bb->buffer[l] = 0;
		bb->bytes -= bb->consumed_bytes;
		bb->total_bits -= (bb->consumed_bytes<<3);
		bb->consumed_bytes = 0;
		bb->total_consumed_bits = bb->consumed_bits;
	}
	/* grow if we need to */
	if(bb->capacity <= 1) {
		if(!bitbuffer_resize_(bb, 16))
			return false;
	}
	/* finally, read in some data */
	bytes = bb->capacity - bb->bytes;
	if(!read_callback(bb->buffer+bb->bytes, &bytes, client_data))
		return false;
	bb->bytes += bytes;
	bb->total_bits += (bytes<<3);
	return true;
}

void FLAC__bitbuffer_init(FLAC__BitBuffer *bb)
{
	assert(bb != 0);
	bb->buffer = 0;
	bb->capacity = 0;
	bb->bytes = bb->bits = bb->total_bits = 0;
	bb->consumed_bytes = bb->consumed_bits = bb->total_consumed_bits = 0;
}

bool FLAC__bitbuffer_init_from(FLAC__BitBuffer *bb, const byte buffer[], unsigned bytes)
{
	assert(bb != 0);
	FLAC__bitbuffer_init(bb);
	if(bytes == 0)
		return true;
	else {
		assert(buffer != 0);
		bb->buffer = (byte*)malloc(sizeof(byte)*bytes);
		if(bb->buffer == 0)
			return false;
		memcpy(bb->buffer, buffer, sizeof(byte)*bytes);
		bb->capacity = bb->bytes = bytes;
		bb->bits = 0;
		bb->total_bits = (bytes<<3);
		bb->consumed_bytes = bb->consumed_bits = bb->total_consumed_bits = 0;
		return true;
	}
}

void FLAC__bitbuffer_init_read_crc16(FLAC__BitBuffer *bb, uint16 seed)
{
	assert(bb != 0);

	bb->read_crc16 = seed;
}

bool FLAC__bitbuffer_concatenate_aligned(FLAC__BitBuffer *dest, const FLAC__BitBuffer *src)
{
	static const byte mask_[9] = { 0x00, 0x01, 0x03, 0x07, 0x0f, 0x1f, 0x3f, 0x7f, 0xff };
	unsigned bits_to_add = src->total_bits - src->total_consumed_bits;

	assert(dest != 0);
	assert(src != 0);

	if(bits_to_add == 0)
		return true;
	if(dest->bits != src->consumed_bits)
		return false;
	if(!bitbuffer_ensure_size_(dest, bits_to_add))
		return false;
	if(dest->bits == 0) {
		memcpy(dest->buffer+dest->bytes, src->buffer+src->consumed_bytes, src->bytes-src->consumed_bytes + ((src->bits)? 1:0));
	}
	else if(dest->bits + bits_to_add > 8) {
		dest->buffer[dest->bytes] <<= (8 - dest->bits);
		dest->buffer[dest->bytes] |= (src->buffer[src->consumed_bytes] & mask_[8-dest->bits]);
		memcpy(dest->buffer+dest->bytes+1, src->buffer+src->consumed_bytes+1, src->bytes-src->consumed_bytes-1 + ((src->bits)? 1:0));
	}
	else {
		dest->buffer[dest->bytes] <<= bits_to_add;
		dest->buffer[dest->bytes] |= (src->buffer[src->consumed_bytes] & mask_[bits_to_add]);
	}
	dest->bits = src->bits;
	dest->total_bits += bits_to_add;
	dest->bytes = dest->total_bits / 8;

	return true;
}

void FLAC__bitbuffer_free(FLAC__BitBuffer *bb)
{
	assert(bb != 0);
	if(bb->buffer != 0)
		free(bb->buffer);
	bb->buffer = 0;
	bb->capacity = 0;
	bb->bytes = bb->bits = bb->total_bits = 0;
	bb->consumed_bytes = bb->consumed_bits = bb->total_consumed_bits = 0;
}

bool FLAC__bitbuffer_clear(FLAC__BitBuffer *bb)
{
	if(bb->buffer == 0) {
		bb->capacity = FLAC__BITBUFFER_DEFAULT_CAPACITY;
		bb->buffer = (byte*)malloc(sizeof(byte) * bb->capacity);
		if(bb->buffer == 0)
			return false;
		memset(bb->buffer, 0, bb->capacity);
	}
	else {
		memset(bb->buffer, 0, bb->bytes + (bb->bits?1:0));
	}
	bb->bytes = bb->bits = bb->total_bits = 0;
	bb->consumed_bytes = bb->consumed_bits = bb->total_consumed_bits = 0;
	return true;
}

bool FLAC__bitbuffer_clone(FLAC__BitBuffer *dest, const FLAC__BitBuffer *src)
{
	if(dest->capacity < src->capacity)
		if(!bitbuffer_resize_(dest, src->capacity))
			return false;
	memcpy(dest->buffer, src->buffer, sizeof(byte)*min(src->capacity, src->bytes+1));
	dest->bytes = src->bytes;
	dest->bits = src->bits;
	dest->total_bits = src->total_bits;
	dest->consumed_bytes = src->consumed_bytes;
	dest->consumed_bits = src->consumed_bits;
	dest->total_consumed_bits = src->total_consumed_bits;
	dest->read_crc16 = src->read_crc16;
	return true;
}

bool FLAC__bitbuffer_write_zeroes(FLAC__BitBuffer *bb, unsigned bits)
{
	unsigned n, k;

	assert(bb != 0);
	assert(bb->buffer != 0);

	if(bits == 0)
		return true;
	if(!bitbuffer_ensure_size_(bb, bits))
		return false;
	bb->total_bits += bits;
	while(bits > 0) {
		n = min(8 - bb->bits, bits);
		k = bits - n;
		bb->buffer[bb->bytes] <<= n;
		bits -= n;
		bb->bits += n;
		if(bb->bits == 8) {
			bb->bytes++;
			bb->bits = 0;
		}
	}
	return true;
}

bool FLAC__bitbuffer_write_raw_uint32(FLAC__BitBuffer *bb, uint32 val, unsigned bits)
{
	unsigned n, k;

	assert(bb != 0);
	assert(bb->buffer != 0);

	assert(bits <= 32);
	if(bits == 0)
		return true;
	if(!bitbuffer_ensure_size_(bb, bits))
		return false;
	val &= (~(0xffffffffu << bits)); /* zero-out unused bits */
	bb->total_bits += bits;
	while(bits > 0) {
		n = 8 - bb->bits;
		if(n == 8) { /* i.e. bb->bits == 0 */
			if(bits < 8) {
				bb->buffer[bb->bytes] = val;
				bb->bits = bits;
				break;
			}
			else if(bits == 8) {
				bb->buffer[bb->bytes++] = val;
				break;
			}
			else {
				k = bits - 8;
				bb->buffer[bb->bytes++] = val >> k;
				val &= (~(0xffffffff << k));
				bits -= 8;
			}
		}
		else if(bits <= n) {
			bb->buffer[bb->bytes] <<= bits;
			bb->buffer[bb->bytes] |= val;
			if(bits == n) {
				bb->bytes++;
				bb->bits = 0;
			}
			else
				bb->bits += bits;
			break;
		}
		else {
			k = bits - n;
			bb->buffer[bb->bytes] <<= n;
			bb->buffer[bb->bytes] |= (val>>k);
			val &= (~(0xffffffff << k));
			bits -= n;
			bb->bytes++;
			bb->bits = 0;
		}
	}

	return true;
}

bool FLAC__bitbuffer_write_raw_int32(FLAC__BitBuffer *bb, int32 val, unsigned bits)
{
	return FLAC__bitbuffer_write_raw_uint32(bb, (uint32)val, bits);
}

bool FLAC__bitbuffer_write_raw_uint64(FLAC__BitBuffer *bb, uint64 val, unsigned bits)
{
	static const uint64 mask[] = {
		0,
		0x0000000000000001, 0x0000000000000003, 0x0000000000000007, 0x000000000000000F,
		0x000000000000001F, 0x000000000000003F, 0x000000000000007F, 0x00000000000000FF,
		0x00000000000001FF, 0x00000000000003FF, 0x00000000000007FF, 0x0000000000000FFF,
		0x0000000000001FFF, 0x0000000000003FFF, 0x0000000000007FFF, 0x000000000000FFFF,
		0x000000000001FFFF, 0x000000000003FFFF, 0x000000000007FFFF, 0x00000000000FFFFF,
		0x00000000001FFFFF, 0x00000000003FFFFF, 0x00000000007FFFFF, 0x0000000000FFFFFF,
		0x0000000001FFFFFF, 0x0000000003FFFFFF, 0x0000000007FFFFFF, 0x000000000FFFFFFF,
		0x000000001FFFFFFF, 0x000000003FFFFFFF, 0x000000007FFFFFFF, 0x00000000FFFFFFFF,
		0x00000001FFFFFFFF, 0x00000003FFFFFFFF, 0x00000007FFFFFFFF, 0x0000000FFFFFFFFF,
		0x0000001FFFFFFFFF, 0x0000003FFFFFFFFF, 0x0000007FFFFFFFFF, 0x000000FFFFFFFFFF,
		0x000001FFFFFFFFFF, 0x000003FFFFFFFFFF, 0x000007FFFFFFFFFF, 0x00000FFFFFFFFFFF,
		0x00001FFFFFFFFFFF, 0x00003FFFFFFFFFFF, 0x00007FFFFFFFFFFF, 0x0000FFFFFFFFFFFF,
		0x0001FFFFFFFFFFFF, 0x0003FFFFFFFFFFFF, 0x0007FFFFFFFFFFFF, 0x000FFFFFFFFFFFFF,
		0x001FFFFFFFFFFFFF, 0x003FFFFFFFFFFFFF, 0x007FFFFFFFFFFFFF, 0x00FFFFFFFFFFFFFF,
		0x01FFFFFFFFFFFFFF, 0x03FFFFFFFFFFFFFF, 0x07FFFFFFFFFFFFFF, 0x0FFFFFFFFFFFFFFF,
		0x1FFFFFFFFFFFFFFF, 0x3FFFFFFFFFFFFFFF, 0x7FFFFFFFFFFFFFFF, 0xFFFFFFFFFFFFFFFF
	};
	unsigned n, k;

	assert(bb != 0);
	assert(bb->buffer != 0);

	assert(bits <= 64);
	if(bits == 0)
		return true;
	if(!bitbuffer_ensure_size_(bb, bits))
		return false;
	val &= mask[bits];
	bb->total_bits += bits;
	while(bits > 0) {
		if(bb->bits == 0) {
			if(bits < 8) {
				bb->buffer[bb->bytes] = val;
				bb->bits = bits;
				break;
			}
			else if(bits == 8) {
				bb->buffer[bb->bytes++] = val;
				break;
			}
			else {
				k = bits - 8;
				bb->buffer[bb->bytes++] = val >> k;
				val &= (~(0xffffffffffffffff << k));
				bits -= 8;
			}
		}
		else {
			n = min(8 - bb->bits, bits);
			k = bits - n;
			bb->buffer[bb->bytes] <<= n;
			bb->buffer[bb->bytes] |= (val>>k);
			val &= (~(0xffffffffffffffff << k));
			bits -= n;
			bb->bits += n;
			if(bb->bits == 8) {
				bb->bytes++;
				bb->bits = 0;
			}
		}
	}

	return true;
}

bool FLAC__bitbuffer_write_raw_int64(FLAC__BitBuffer *bb, int64 val, unsigned bits)
{
	return FLAC__bitbuffer_write_raw_uint64(bb, (uint64)val, bits);
}

bool FLAC__bitbuffer_write_unary_unsigned(FLAC__BitBuffer *bb, unsigned val)
{
	if(val < 32)
		return FLAC__bitbuffer_write_raw_uint32(bb, 1, ++val);
	else if(val < 64)
		return FLAC__bitbuffer_write_raw_uint64(bb, 1, ++val);
	else {
		if(!FLAC__bitbuffer_write_zeroes(bb, val))
			return false;
		return FLAC__bitbuffer_write_raw_uint32(bb, 1, 1);
	}
}

unsigned FLAC__bitbuffer_rice_bits(int val, unsigned parameter)
{
	unsigned msbs, uval;

	/* convert signed to unsigned */
	if(val < 0)
		/* equivalent to
		 *     (unsigned)(((--val) << 1) - 1);
		 * but without the overflow problem at -MAXINT
		 */
		uval = (unsigned)(((-(++val)) << 1) + 1);
	else
		uval = (unsigned)(val << 1);

	msbs = uval >> parameter;

	return 1 + parameter + msbs;
}

unsigned FLAC__bitbuffer_golomb_bits_signed(int val, unsigned parameter)
{
	unsigned bits, msbs, uval;
	unsigned k;

	assert(parameter > 0);

	/* convert signed to unsigned */
	if(val < 0)
		/* equivalent to
		 *     (unsigned)(((--val) << 1) - 1);
		 * but without the overflow problem at -MAXINT
		 */
		uval = (unsigned)(((-(++val)) << 1) + 1);
	else
		uval = (unsigned)(val << 1);

	k = FLAC__bitmath_ilog2(parameter);
	if(parameter == 1u<<k) {
		assert(k <= 30);

		msbs = uval >> k;
		bits = 1 + k + msbs;
	}
	else {
		unsigned q, r, d;

		d = (1 << (k+1)) - parameter;
		q = uval / parameter;
		r = uval - (q * parameter);

		bits = 1 + q + k;
		if(r >= d)
			bits++;
	}
	return bits;
}

unsigned FLAC__bitbuffer_golomb_bits_unsigned(unsigned uval, unsigned parameter)
{
	unsigned bits, msbs;
	unsigned k;

	assert(parameter > 0);

	k = FLAC__bitmath_ilog2(parameter);
	if(parameter == 1u<<k) {
		assert(k <= 30);

		msbs = uval >> k;
		bits = 1 + k + msbs;
	}
	else {
		unsigned q, r, d;

		d = (1 << (k+1)) - parameter;
		q = uval / parameter;
		r = uval - (q * parameter);

		bits = 1 + q + k;
		if(r >= d)
			bits++;
	}
	return bits;
}

bool FLAC__bitbuffer_write_symmetric_rice_signed(FLAC__BitBuffer *bb, int val, unsigned parameter)
{
	unsigned total_bits, interesting_bits, msbs;
	uint32 pattern;

	assert(bb != 0);
	assert(bb->buffer != 0);
	assert(parameter <= 31);

	/* init pattern with the unary end bit and the sign bit */
	if(val < 0) {
		pattern = 3;
		val = -val;
	}
	else
		pattern = 2;

	msbs = val >> parameter;
	interesting_bits = 2 + parameter;
	total_bits = interesting_bits + msbs;
	pattern <<= parameter;
	pattern |= (val & ((1<<parameter)-1)); /* the binary LSBs */

	if(total_bits <= 32) {
		if(!FLAC__bitbuffer_write_raw_uint32(bb, pattern, total_bits))
			return false;
	}
	else {
		/* write the unary MSBs */
		if(!FLAC__bitbuffer_write_zeroes(bb, msbs))
			return false;
		/* write the unary end bit, the sign bit, and binary LSBs */
		if(!FLAC__bitbuffer_write_raw_uint32(bb, pattern, interesting_bits))
			return false;
	}
	return true;
}

bool FLAC__bitbuffer_write_symmetric_rice_signed_guarded(FLAC__BitBuffer *bb, int val, unsigned parameter, unsigned max_bits, bool *overflow)
{
	unsigned total_bits, interesting_bits, msbs;
	uint32 pattern;

	assert(bb != 0);
	assert(bb->buffer != 0);
	assert(parameter <= 31);

	*overflow = false;

	/* init pattern with the unary end bit and the sign bit */
	if(val < 0) {
		pattern = 3;
		val = -val;
	}
	else
		pattern = 2;

	msbs = val >> parameter;
	interesting_bits = 2 + parameter;
	total_bits = interesting_bits + msbs;
	pattern <<= parameter;
	pattern |= (val & ((1<<parameter)-1)); /* the binary LSBs */

	if(total_bits <= 32) {
		if(!FLAC__bitbuffer_write_raw_uint32(bb, pattern, total_bits))
			return false;
	}
	else if(total_bits > max_bits) {
		*overflow = true;
		return true;
	}
	else {
		/* write the unary MSBs */
		if(!FLAC__bitbuffer_write_zeroes(bb, msbs))
			return false;
		/* write the unary end bit, the sign bit, and binary LSBs */
		if(!FLAC__bitbuffer_write_raw_uint32(bb, pattern, interesting_bits))
			return false;
	}
	return true;
}

bool FLAC__bitbuffer_write_symmetric_rice_signed_escape(FLAC__BitBuffer *bb, int val, unsigned parameter)
{
	unsigned total_bits, val_bits;
	uint32 pattern;

	assert(bb != 0);
	assert(bb->buffer != 0);
	assert(parameter <= 31);

	val_bits = FLAC__bitmath_silog2(val);
	total_bits = 2 + parameter + 5 + val_bits;

	if(total_bits <= 32) {
		pattern = 3;
		pattern <<= (parameter + 5);
		pattern |= val_bits;
		pattern <<= val_bits;
		pattern |= (val & ((1 << val_bits) - 1));
		if(!FLAC__bitbuffer_write_raw_uint32(bb, pattern, total_bits))
			return false;
	}
	else {
		/* write the '-0' escape code first */
		if(!FLAC__bitbuffer_write_raw_uint32(bb, 3u << parameter, 2+parameter))
			return false;
		/* write the length */
		if(!FLAC__bitbuffer_write_raw_uint32(bb, val_bits, 5))
			return false;
		/* write the value */
		if(!FLAC__bitbuffer_write_raw_int32(bb, val, val_bits))
			return false;
	}
	return true;
}

bool FLAC__bitbuffer_write_rice_signed(FLAC__BitBuffer *bb, int val, unsigned parameter)
{
	unsigned total_bits, interesting_bits, msbs, uval;
	uint32 pattern;

	assert(bb != 0);
	assert(bb->buffer != 0);
	assert(parameter <= 30);

	/* convert signed to unsigned */
	if(val < 0)
		/* equivalent to
		 *     (unsigned)(((--val) << 1) - 1);
		 * but without the overflow problem at -MAXINT
		 */
		uval = (unsigned)(((-(++val)) << 1) + 1);
	else
		uval = (unsigned)(val << 1);

	msbs = uval >> parameter;
	interesting_bits = 1 + parameter;
	total_bits = interesting_bits + msbs;
	pattern = 1 << parameter; /* the unary end bit */
	pattern |= (uval & ((1<<parameter)-1)); /* the binary LSBs */

	if(total_bits <= 32) {
		if(!FLAC__bitbuffer_write_raw_uint32(bb, pattern, total_bits))
			return false;
	}
	else {
		/* write the unary MSBs */
		if(!FLAC__bitbuffer_write_zeroes(bb, msbs))
			return false;
		/* write the unary end bit and binary LSBs */
		if(!FLAC__bitbuffer_write_raw_uint32(bb, pattern, interesting_bits))
			return false;
	}
	return true;
}

bool FLAC__bitbuffer_write_rice_signed_guarded(FLAC__BitBuffer *bb, int val, unsigned parameter, unsigned max_bits, bool *overflow)
{
	unsigned total_bits, interesting_bits, msbs, uval;
	uint32 pattern;

	assert(bb != 0);
	assert(bb->buffer != 0);
	assert(parameter <= 30);

	*overflow = false;

	/* convert signed to unsigned */
	if(val < 0)
		/* equivalent to
		 *     (unsigned)(((--val) << 1) - 1);
		 * but without the overflow problem at -MAXINT
		 */
		uval = (unsigned)(((-(++val)) << 1) + 1);
	else
		uval = (unsigned)(val << 1);

	msbs = uval >> parameter;
	interesting_bits = 1 + parameter;
	total_bits = interesting_bits + msbs;
	pattern = 1 << parameter; /* the unary end bit */
	pattern |= (uval & ((1<<parameter)-1)); /* the binary LSBs */

	if(total_bits <= 32) {
		if(!FLAC__bitbuffer_write_raw_uint32(bb, pattern, total_bits))
			return false;
	}
	else if(total_bits > max_bits) {
		*overflow = true;
		return true;
	}
	else {
		/* write the unary MSBs */
		if(!FLAC__bitbuffer_write_zeroes(bb, msbs))
			return false;
		/* write the unary end bit and binary LSBs */
		if(!FLAC__bitbuffer_write_raw_uint32(bb, pattern, interesting_bits))
			return false;
	}
	return true;
}

bool FLAC__bitbuffer_write_golomb_signed(FLAC__BitBuffer *bb, int val, unsigned parameter)
{
	unsigned total_bits, msbs, uval;
	unsigned k;

	assert(bb != 0);
	assert(bb->buffer != 0);
	assert(parameter > 0);

	/* convert signed to unsigned */
	if(val < 0)
		/* equivalent to
		 *     (unsigned)(((--val) << 1) - 1);
		 * but without the overflow problem at -MAXINT
		 */
		uval = (unsigned)(((-(++val)) << 1) + 1);
	else
		uval = (unsigned)(val << 1);

	k = FLAC__bitmath_ilog2(parameter);
	if(parameter == 1u<<k) {
		unsigned pattern;

		assert(k <= 30);

		msbs = uval >> k;
		total_bits = 1 + k + msbs;
		pattern = 1 << k; /* the unary end bit */
		pattern |= (uval & ((1u<<k)-1)); /* the binary LSBs */

		if(total_bits <= 32) {
			if(!FLAC__bitbuffer_write_raw_uint32(bb, pattern, total_bits))
				return false;
		}
		else {
			/* write the unary MSBs */
			if(!FLAC__bitbuffer_write_zeroes(bb, msbs))
				return false;
			/* write the unary end bit and binary LSBs */
			if(!FLAC__bitbuffer_write_raw_uint32(bb, pattern, k+1))
				return false;
		}
	}
	else {
		unsigned q, r, d;

		d = (1 << (k+1)) - parameter;
		q = uval / parameter;
		r = uval - (q * parameter);
		/* write the unary MSBs */
		if(!FLAC__bitbuffer_write_zeroes(bb, q))
			return false;
		/* write the unary end bit */
		if(!FLAC__bitbuffer_write_raw_uint32(bb, 1, 1))
			return false;
		/* write the binary LSBs */
		if(r >= d) {
			if(!FLAC__bitbuffer_write_raw_uint32(bb, r+d, k+1))
				return false;
		}
		else {
			if(!FLAC__bitbuffer_write_raw_uint32(bb, r, k))
				return false;
		}
	}
	return true;
}

bool FLAC__bitbuffer_write_golomb_unsigned(FLAC__BitBuffer *bb, unsigned uval, unsigned parameter)
{
	unsigned total_bits, msbs;
	unsigned k;

	assert(bb != 0);
	assert(bb->buffer != 0);
	assert(parameter > 0);

	k = FLAC__bitmath_ilog2(parameter);
	if(parameter == 1u<<k) {
		unsigned pattern;

		assert(k <= 30);

		msbs = uval >> k;
		total_bits = 1 + k + msbs;
		pattern = 1 << k; /* the unary end bit */
		pattern |= (uval & ((1u<<k)-1)); /* the binary LSBs */

		if(total_bits <= 32) {
			if(!FLAC__bitbuffer_write_raw_uint32(bb, pattern, total_bits))
				return false;
		}
		else {
			/* write the unary MSBs */
			if(!FLAC__bitbuffer_write_zeroes(bb, msbs))
				return false;
			/* write the unary end bit and binary LSBs */
			if(!FLAC__bitbuffer_write_raw_uint32(bb, pattern, k+1))
				return false;
		}
	}
	else {
		unsigned q, r, d;

		d = (1 << (k+1)) - parameter;
		q = uval / parameter;
		r = uval - (q * parameter);
		/* write the unary MSBs */
		if(!FLAC__bitbuffer_write_zeroes(bb, q))
			return false;
		/* write the unary end bit */
		if(!FLAC__bitbuffer_write_raw_uint32(bb, 1, 1))
			return false;
		/* write the binary LSBs */
		if(r >= d) {
			if(!FLAC__bitbuffer_write_raw_uint32(bb, r+d, k+1))
				return false;
		}
		else {
			if(!FLAC__bitbuffer_write_raw_uint32(bb, r, k))
				return false;
		}
	}
	return true;
}

bool FLAC__bitbuffer_write_utf8_uint32(FLAC__BitBuffer *bb, uint32 val)
{
	bool ok = 1;

	assert(bb != 0);
	assert(bb->buffer != 0);

	assert(!(val & 0x80000000)); /* this version only handles 31 bits */

	if(val < 0x80) {
		return FLAC__bitbuffer_write_raw_uint32(bb, val, 8);
	}
	else if(val < 0x800) {
		ok &= FLAC__bitbuffer_write_raw_uint32(bb, 0xC0 | (val>>6), 8);
		ok &= FLAC__bitbuffer_write_raw_uint32(bb, 0x80 | (val&0x3F), 8);
	}
	else if(val < 0x10000) {
		ok &= FLAC__bitbuffer_write_raw_uint32(bb, 0xE0 | (val>>12), 8);
		ok &= FLAC__bitbuffer_write_raw_uint32(bb, 0x80 | ((val>>6)&0x3F), 8);
		ok &= FLAC__bitbuffer_write_raw_uint32(bb, 0x80 | (val&0x3F), 8);
	}
	else if(val < 0x200000) {
		ok &= FLAC__bitbuffer_write_raw_uint32(bb, 0xF0 | (val>>18), 8);
		ok &= FLAC__bitbuffer_write_raw_uint32(bb, 0x80 | ((val>>12)&0x3F), 8);
		ok &= FLAC__bitbuffer_write_raw_uint32(bb, 0x80 | ((val>>6)&0x3F), 8);
		ok &= FLAC__bitbuffer_write_raw_uint32(bb, 0x80 | (val&0x3F), 8);
	}
	else if(val < 0x4000000) {
		ok &= FLAC__bitbuffer_write_raw_uint32(bb, 0xF8 | (val>>24), 8);
		ok &= FLAC__bitbuffer_write_raw_uint32(bb, 0x80 | ((val>>18)&0x3F), 8);
		ok &= FLAC__bitbuffer_write_raw_uint32(bb, 0x80 | ((val>>12)&0x3F), 8);
		ok &= FLAC__bitbuffer_write_raw_uint32(bb, 0x80 | ((val>>6)&0x3F), 8);
		ok &= FLAC__bitbuffer_write_raw_uint32(bb, 0x80 | (val&0x3F), 8);
	}
	else {
		ok &= FLAC__bitbuffer_write_raw_uint32(bb, 0xFC | (val>>30), 8);
		ok &= FLAC__bitbuffer_write_raw_uint32(bb, 0x80 | ((val>>24)&0x3F), 8);
		ok &= FLAC__bitbuffer_write_raw_uint32(bb, 0x80 | ((val>>18)&0x3F), 8);
		ok &= FLAC__bitbuffer_write_raw_uint32(bb, 0x80 | ((val>>12)&0x3F), 8);
		ok &= FLAC__bitbuffer_write_raw_uint32(bb, 0x80 | ((val>>6)&0x3F), 8);
		ok &= FLAC__bitbuffer_write_raw_uint32(bb, 0x80 | (val&0x3F), 8);
	}

	return ok;
}

bool FLAC__bitbuffer_write_utf8_uint64(FLAC__BitBuffer *bb, uint64 val)
{
	bool ok = 1;

	assert(bb != 0);
	assert(bb->buffer != 0);

	assert(!(val & 0xFFFFFFF000000000)); /* this version only handles 36 bits */

	if(val < 0x80) {
		return FLAC__bitbuffer_write_raw_uint32(bb, (uint32)val, 8);
	}
	else if(val < 0x800) {
		ok &= FLAC__bitbuffer_write_raw_uint32(bb, 0xC0 | (uint32)(val>>6), 8);
		ok &= FLAC__bitbuffer_write_raw_uint32(bb, 0x80 | (uint32)(val&0x3F), 8);
	}
	else if(val < 0x10000) {
		ok &= FLAC__bitbuffer_write_raw_uint32(bb, 0xE0 | (uint32)(val>>12), 8);
		ok &= FLAC__bitbuffer_write_raw_uint32(bb, 0x80 | (uint32)((val>>6)&0x3F), 8);
		ok &= FLAC__bitbuffer_write_raw_uint32(bb, 0x80 | (uint32)(val&0x3F), 8);
	}
	else if(val < 0x200000) {
		ok &= FLAC__bitbuffer_write_raw_uint32(bb, 0xF0 | (uint32)(val>>18), 8);
		ok &= FLAC__bitbuffer_write_raw_uint32(bb, 0x80 | (uint32)((val>>12)&0x3F), 8);
		ok &= FLAC__bitbuffer_write_raw_uint32(bb, 0x80 | (uint32)((val>>6)&0x3F), 8);
		ok &= FLAC__bitbuffer_write_raw_uint32(bb, 0x80 | (uint32)(val&0x3F), 8);
	}
	else if(val < 0x4000000) {
		ok &= FLAC__bitbuffer_write_raw_uint32(bb, 0xF8 | (uint32)(val>>24), 8);
		ok &= FLAC__bitbuffer_write_raw_uint32(bb, 0x80 | (uint32)((val>>18)&0x3F), 8);
		ok &= FLAC__bitbuffer_write_raw_uint32(bb, 0x80 | (uint32)((val>>12)&0x3F), 8);
		ok &= FLAC__bitbuffer_write_raw_uint32(bb, 0x80 | (uint32)((val>>6)&0x3F), 8);
		ok &= FLAC__bitbuffer_write_raw_uint32(bb, 0x80 | (uint32)(val&0x3F), 8);
	}
	else if(val < 0x80000000) {
		ok &= FLAC__bitbuffer_write_raw_uint32(bb, 0xFC | (uint32)(val>>30), 8);
		ok &= FLAC__bitbuffer_write_raw_uint32(bb, 0x80 | (uint32)((val>>24)&0x3F), 8);
		ok &= FLAC__bitbuffer_write_raw_uint32(bb, 0x80 | (uint32)((val>>18)&0x3F), 8);
		ok &= FLAC__bitbuffer_write_raw_uint32(bb, 0x80 | (uint32)((val>>12)&0x3F), 8);
		ok &= FLAC__bitbuffer_write_raw_uint32(bb, 0x80 | (uint32)((val>>6)&0x3F), 8);
		ok &= FLAC__bitbuffer_write_raw_uint32(bb, 0x80 | (uint32)(val&0x3F), 8);
	}
	else {
		ok &= FLAC__bitbuffer_write_raw_uint32(bb, 0xFE, 8);
		ok &= FLAC__bitbuffer_write_raw_uint32(bb, 0x80 | (uint32)((val>>30)&0x3F), 8);
		ok &= FLAC__bitbuffer_write_raw_uint32(bb, 0x80 | (uint32)((val>>24)&0x3F), 8);
		ok &= FLAC__bitbuffer_write_raw_uint32(bb, 0x80 | (uint32)((val>>18)&0x3F), 8);
		ok &= FLAC__bitbuffer_write_raw_uint32(bb, 0x80 | (uint32)((val>>12)&0x3F), 8);
		ok &= FLAC__bitbuffer_write_raw_uint32(bb, 0x80 | (uint32)((val>>6)&0x3F), 8);
		ok &= FLAC__bitbuffer_write_raw_uint32(bb, 0x80 | (uint32)(val&0x3F), 8);
	}

	return ok;
}

bool FLAC__bitbuffer_zero_pad_to_byte_boundary(FLAC__BitBuffer *bb)
{
	/* 0-pad to byte boundary */
	if(bb->bits != 0)
		return FLAC__bitbuffer_write_zeroes(bb, 8 - bb->bits);
	else
		return true;
}

bool FLAC__bitbuffer_peek_bit(FLAC__BitBuffer *bb, unsigned *val, bool (*read_callback)(byte buffer[], unsigned *bytes, void *client_data), void *client_data)
{
	/* to avoid a drastic speed penalty we don't:
	assert(bb != 0);
	assert(bb->buffer != 0);
	assert(bb->bits == 0);
	*/

	while(1) {
		if(bb->total_consumed_bits < bb->total_bits) {
			*val = (bb->buffer[bb->consumed_bytes] & BYTE_BIT_TO_MASK(bb->consumed_bits))? 1 : 0;
			return true;
		}
		else {
			if(!bitbuffer_read_from_client_(bb, read_callback, client_data))
				return false;
		}
	}
}

bool FLAC__bitbuffer_read_bit(FLAC__BitBuffer *bb, unsigned *val, bool (*read_callback)(byte buffer[], unsigned *bytes, void *client_data), void *client_data)
{
	/* to avoid a drastic speed penalty we don't:
	assert(bb != 0);
	assert(bb->buffer != 0);
	assert(bb->bits == 0);
	*/

	while(1) {
		if(bb->total_consumed_bits < bb->total_bits) {
			*val = (bb->buffer[bb->consumed_bytes] & BYTE_BIT_TO_MASK(bb->consumed_bits))? 1 : 0;
			bb->consumed_bits++;
			if(bb->consumed_bits == 8) {
				FLAC__CRC16_UPDATE(bb->buffer[bb->consumed_bytes], bb->read_crc16);
				bb->consumed_bytes++;
				bb->consumed_bits = 0;
			}
			bb->total_consumed_bits++;
			return true;
		}
		else {
			if(!bitbuffer_read_from_client_(bb, read_callback, client_data))
				return false;
		}
	}
}

bool FLAC__bitbuffer_read_bit_to_uint32(FLAC__BitBuffer *bb, uint32 *val, bool (*read_callback)(byte buffer[], unsigned *bytes, void *client_data), void *client_data)
{
	/* to avoid a drastic speed penalty we don't:
	assert(bb != 0);
	assert(bb->buffer != 0);
	assert(bb->bits == 0);
	*/

	while(1) {
		if(bb->total_consumed_bits < bb->total_bits) {
			*val <<= 1;
			*val |= (bb->buffer[bb->consumed_bytes] & BYTE_BIT_TO_MASK(bb->consumed_bits))? 1 : 0;
			bb->consumed_bits++;
			if(bb->consumed_bits == 8) {
				FLAC__CRC16_UPDATE(bb->buffer[bb->consumed_bytes], bb->read_crc16);
				bb->consumed_bytes++;
				bb->consumed_bits = 0;
			}
			bb->total_consumed_bits++;
			return true;
		}
		else {
			if(!bitbuffer_read_from_client_(bb, read_callback, client_data))
				return false;
		}
	}
}

bool FLAC__bitbuffer_read_bit_to_uint64(FLAC__BitBuffer *bb, uint64 *val, bool (*read_callback)(byte buffer[], unsigned *bytes, void *client_data), void *client_data)
{
	/* to avoid a drastic speed penalty we don't:
	assert(bb != 0);
	assert(bb->buffer != 0);
	assert(bb->bits == 0);
	*/

	while(1) {
		if(bb->total_consumed_bits < bb->total_bits) {
			*val <<= 1;
			*val |= (bb->buffer[bb->consumed_bytes] & BYTE_BIT_TO_MASK(bb->consumed_bits))? 1 : 0;
			bb->consumed_bits++;
			if(bb->consumed_bits == 8) {
				FLAC__CRC16_UPDATE(bb->buffer[bb->consumed_bytes], bb->read_crc16);
				bb->consumed_bytes++;
				bb->consumed_bits = 0;
			}
			bb->total_consumed_bits++;
			return true;
		}
		else {
			if(!bitbuffer_read_from_client_(bb, read_callback, client_data))
				return false;
		}
	}
}

bool FLAC__bitbuffer_read_raw_uint32(FLAC__BitBuffer *bb, uint32 *val, const unsigned bits, bool (*read_callback)(byte buffer[], unsigned *bytes, void *client_data), void *client_data)
#ifdef FLAC__NO_MANUAL_INLINING
{
	unsigned i;

	assert(bb != 0);
	assert(bb->buffer != 0);

	assert(bits <= 32);

	*val = 0;
	for(i = 0; i < bits; i++) {
		if(!FLAC__bitbuffer_read_bit_to_uint32(bb, val, read_callback, client_data))
			return false;
	}
	return true;
}
#else
{
	unsigned i, bits_ = bits;
	uint32 v = 0;

	assert(bb != 0);
	assert(bb->buffer != 0);

	assert(bits <= 32);
	assert((bb->capacity*8) * 2 >= bits);

	while(bb->total_consumed_bits + bits > bb->total_bits) {
		if(!bitbuffer_read_from_client_(bb, read_callback, client_data))
			return false;
	}
	if(bb->consumed_bits) {
		i = 8 - bb->consumed_bits;
		if(i <= bits_) {
			v = bb->buffer[bb->consumed_bytes] & (0xff >> bb->consumed_bits);
			bits_ -= i;
			FLAC__CRC16_UPDATE(bb->buffer[bb->consumed_bytes], bb->read_crc16);
			bb->consumed_bytes++;
			bb->consumed_bits = 0;
			/* we hold off updating bb->total_consumed_bits until the end */
		}
		else {
			*val = (bb->buffer[bb->consumed_bytes] & (0xff >> bb->consumed_bits)) >> (i-bits_);
			bb->consumed_bits += bits_;
			bb->total_consumed_bits += bits_;
			return true;
		}
	}
	while(bits_ >= 8) {
		v <<= 8;
		v |= bb->buffer[bb->consumed_bytes];
		bits_ -= 8;
		FLAC__CRC16_UPDATE(bb->buffer[bb->consumed_bytes], bb->read_crc16);
		bb->consumed_bytes++;
		/* bb->consumed_bits is already 0 */
		/* we hold off updating bb->total_consumed_bits until the end */
	}
	if(bits_ > 0) {
		v <<= bits_;
		v |= (bb->buffer[bb->consumed_bytes] >> (8-bits_));
		bb->consumed_bits = bits_;
		/* we hold off updating bb->total_consumed_bits until the end */
	}
	bb->total_consumed_bits += bits;
	*val = v;
	return true;
}
#endif

bool FLAC__bitbuffer_read_raw_int32(FLAC__BitBuffer *bb, int32 *val, const unsigned bits, bool (*read_callback)(byte buffer[], unsigned *bytes, void *client_data), void *client_data)
#ifdef FLAC__NO_MANUAL_INLINING
{
	unsigned i;
	uint32 v;

	assert(bb != 0);
	assert(bb->buffer != 0);

	assert(bits <= 32);

	v = 0;
	for(i = 0; i < bits; i++) {
		if(!FLAC__bitbuffer_read_bit_to_uint32(bb, &v, read_callback, client_data))
			return false;
	}

	/* fix the sign */
	i = 32 - bits;
	if(i) {
		v <<= i;
		*val = (int32)v;
		*val >>= i;
	}
	else
		*val = (int32)v;

	return true;
}
#else
{
	unsigned i, bits_ = bits;
	uint32 v = 0;

	assert(bb != 0);
	assert(bb->buffer != 0);

	assert(bits <= 32);
	assert((bb->capacity*8) * 2 >= bits);

	while(bb->total_consumed_bits + bits > bb->total_bits) {
		if(!bitbuffer_read_from_client_(bb, read_callback, client_data))
			return false;
	}
	if(bb->consumed_bits) {
		i = 8 - bb->consumed_bits;
		if(i <= bits_) {
			v = bb->buffer[bb->consumed_bytes] & (0xff >> bb->consumed_bits);
			bits_ -= i;
			FLAC__CRC16_UPDATE(bb->buffer[bb->consumed_bytes], bb->read_crc16);
			bb->consumed_bytes++;
			bb->consumed_bits = 0;
			/* we hold off updating bb->total_consumed_bits until the end */
		}
		else {
			*val = (bb->buffer[bb->consumed_bytes] & (0xff >> bb->consumed_bits)) >> (i-bits_);
			bb->consumed_bits += bits_;
			bb->total_consumed_bits += bits_;
			return true;
		}
	}
	while(bits_ >= 8) {
		v <<= 8;
		v |= bb->buffer[bb->consumed_bytes];
		bits_ -= 8;
		FLAC__CRC16_UPDATE(bb->buffer[bb->consumed_bytes], bb->read_crc16);
		bb->consumed_bytes++;
		/* bb->consumed_bits is already 0 */
		/* we hold off updating bb->total_consumed_bits until the end */
	}
	if(bits_ > 0) {
		v <<= bits_;
		v |= (bb->buffer[bb->consumed_bytes] >> (8-bits_));
		bb->consumed_bits = bits_;
		/* we hold off updating bb->total_consumed_bits until the end */
	}
	bb->total_consumed_bits += bits;

	/* fix the sign */
	i = 32 - bits;
	if(i) {
		v <<= i;
		*val = (int32)v;
		*val >>= i;
	}
	else
		*val = (int32)v;

	return true;
}
#endif

bool FLAC__bitbuffer_read_raw_uint64(FLAC__BitBuffer *bb, uint64 *val, const unsigned bits, bool (*read_callback)(byte buffer[], unsigned *bytes, void *client_data), void *client_data)
#ifdef FLAC__NO_MANUAL_INLINING
{
	unsigned i;

	assert(bb != 0);
	assert(bb->buffer != 0);

	assert(bits <= 64);

	*val = 0;
	for(i = 0; i < bits; i++) {
		if(!FLAC__bitbuffer_read_bit_to_uint64(bb, val, read_callback, client_data))
			return false;
	}
	return true;
}
#else
{
	unsigned i, bits_ = bits;
	uint64 v = 0;

	assert(bb != 0);
	assert(bb->buffer != 0);

	assert(bits <= 64);
	assert((bb->capacity*8) * 2 >= bits);

	while(bb->total_consumed_bits + bits > bb->total_bits) {
		if(!bitbuffer_read_from_client_(bb, read_callback, client_data))
			return false;
	}
	if(bb->consumed_bits) {
		i = 8 - bb->consumed_bits;
		if(i <= bits_) {
			v = bb->buffer[bb->consumed_bytes] & (0xff >> bb->consumed_bits);
			bits_ -= i;
			FLAC__CRC16_UPDATE(bb->buffer[bb->consumed_bytes], bb->read_crc16);
			bb->consumed_bytes++;
			bb->consumed_bits = 0;
			/* we hold off updating bb->total_consumed_bits until the end */
		}
		else {
			*val = (bb->buffer[bb->consumed_bytes] & (0xff >> bb->consumed_bits)) >> (i-bits_);
			bb->consumed_bits += bits_;
			bb->total_consumed_bits += bits_;
			return true;
		}
	}
	while(bits_ >= 8) {
		v <<= 8;
		v |= bb->buffer[bb->consumed_bytes];
		bits_ -= 8;
		FLAC__CRC16_UPDATE(bb->buffer[bb->consumed_bytes], bb->read_crc16);
		bb->consumed_bytes++;
		/* bb->consumed_bits is already 0 */
		/* we hold off updating bb->total_consumed_bits until the end */
	}
	if(bits_ > 0) {
		v <<= bits_;
		v |= (bb->buffer[bb->consumed_bytes] >> (8-bits_));
		bb->consumed_bits = bits_;
		/* we hold off updating bb->total_consumed_bits until the end */
	}
	bb->total_consumed_bits += bits;
	*val = v;
	return true;
}
#endif

bool FLAC__bitbuffer_read_raw_int64(FLAC__BitBuffer *bb, int64 *val, const unsigned bits, bool (*read_callback)(byte buffer[], unsigned *bytes, void *client_data), void *client_data)
#ifdef FLAC__NO_MANUAL_INLINING
{
	unsigned i;
	uint64 v;

	assert(bb != 0);
	assert(bb->buffer != 0);

	assert(bits <= 64);

	v = 0;
	for(i = 0; i < bits; i++) {
		if(!FLAC__bitbuffer_read_bit_to_uint64(bb, &v, read_callback, client_data))
			return false;
	}
	/* fix the sign */
	i = 64 - bits;
	if(i) {
		v <<= i;
		*val = (int64)v;
		*val >>= i;
	}
	else
		*val = (int64)v;

	return true;
}
#else
{
	unsigned i, bits_ = bits;
	uint64 v = 0;

	assert(bb != 0);
	assert(bb->buffer != 0);

	assert(bits <= 64);
	assert((bb->capacity*8) * 2 >= bits);

	while(bb->total_consumed_bits + bits > bb->total_bits) {
		if(!bitbuffer_read_from_client_(bb, read_callback, client_data))
			return false;
	}
	if(bb->consumed_bits) {
		i = 8 - bb->consumed_bits;
		if(i <= bits_) {
			v = bb->buffer[bb->consumed_bytes] & (0xff >> bb->consumed_bits);
			bits_ -= i;
			FLAC__CRC16_UPDATE(bb->buffer[bb->consumed_bytes], bb->read_crc16);
			bb->consumed_bytes++;
			bb->consumed_bits = 0;
			/* we hold off updating bb->total_consumed_bits until the end */
		}
		else {
			*val = (bb->buffer[bb->consumed_bytes] & (0xff >> bb->consumed_bits)) >> (i-bits_);
			bb->consumed_bits += bits_;
			bb->total_consumed_bits += bits_;
			return true;
		}
	}
	while(bits_ >= 8) {
		v <<= 8;
		v |= bb->buffer[bb->consumed_bytes];
		bits_ -= 8;
		FLAC__CRC16_UPDATE(bb->buffer[bb->consumed_bytes], bb->read_crc16);
		bb->consumed_bytes++;
		/* bb->consumed_bits is already 0 */
		/* we hold off updating bb->total_consumed_bits until the end */
	}
	if(bits_ > 0) {
		v <<= bits_;
		v |= (bb->buffer[bb->consumed_bytes] >> (8-bits_));
		bb->consumed_bits = bits_;
		/* we hold off updating bb->total_consumed_bits until the end */
	}
	bb->total_consumed_bits += bits;

	/* fix the sign */
	i = 64 - bits;
	if(i) {
		v <<= i;
		*val = (int64)v;
		*val >>= i;
	}
	else
		*val = (int64)v;

	return true;
}
#endif

bool FLAC__bitbuffer_read_unary_unsigned(FLAC__BitBuffer *bb, unsigned *val, bool (*read_callback)(byte buffer[], unsigned *bytes, void *client_data), void *client_data)
#ifdef FLAC__NO_MANUAL_INLINING
{
	unsigned bit, val_ = 0;

	assert(bb != 0);
	assert(bb->buffer != 0);

	while(1) {
		if(!FLAC__bitbuffer_read_bit(bb, &bit, read_callback, client_data))
			return false;
		if(bit)
			break;
		else
			val_++;
	}
	*val = val_;
	return true;
}
#else
{
	unsigned i, val_ = 0;
	unsigned total_bytes_ = (bb->total_bits + 7) / 8;
	byte b;

	assert(bb != 0);
	assert(bb->buffer != 0);

	if(bb->consumed_bits) {
		b = bb->buffer[bb->consumed_bytes] << bb->consumed_bits;
		if(b) {
			for(i = 0; !(b & 0x80); i++)
				b <<= 1;
			*val = i;
			i++;
			bb->consumed_bits += i;
			bb->total_consumed_bits += i;
			if(bb->consumed_bits == 8) {
				FLAC__CRC16_UPDATE(bb->buffer[bb->consumed_bytes], bb->read_crc16);
				bb->consumed_bytes++;
				bb->consumed_bits = 0;
			}
			return true;
		}
		else {
			val_ = 8 - bb->consumed_bits;
			FLAC__CRC16_UPDATE(bb->buffer[bb->consumed_bytes], bb->read_crc16);
			bb->consumed_bytes++;
			bb->consumed_bits = 0;
			bb->total_consumed_bits += val_;
		}
	}
	while(1) {
		if(bb->consumed_bytes >= total_bytes_) {
			if(!bitbuffer_read_from_client_(bb, read_callback, client_data))
				return false;
			total_bytes_ = (bb->total_bits + 7) / 8;
		}
		b = bb->buffer[bb->consumed_bytes];
		if(b) {
			for(i = 0; !(b & 0x80); i++)
				b <<= 1;
			val_ += i;
			i++;
			bb->consumed_bits = i;
			*val = val_;
			if(i == 8) {
				FLAC__CRC16_UPDATE(bb->buffer[bb->consumed_bytes], bb->read_crc16);
				bb->consumed_bytes++;
				bb->consumed_bits = 0;
			}
			bb->total_consumed_bits += i;
			return true;
		}
		else {
			val_ += 8;
			FLAC__CRC16_UPDATE(0, bb->read_crc16);
			bb->consumed_bytes++;
			/* bb->consumed_bits is already 0 */
			/* we hold off updating bb->total_consumed_bits until the end */
			bb->total_consumed_bits += 8;
		}
	}
}
#endif

bool FLAC__bitbuffer_read_symmetric_rice_signed(FLAC__BitBuffer *bb, int *val, unsigned parameter, bool (*read_callback)(byte buffer[], unsigned *bytes, void *client_data), void *client_data)
{
	uint32 sign = 0, lsbs = 0, msbs = 0;

	assert(bb != 0);
	assert(bb->buffer != 0);
	assert(parameter <= 31);

	/* read the unary MSBs and end bit */
	if(!FLAC__bitbuffer_read_unary_unsigned(bb, &msbs, read_callback, client_data))
		return false;

	/* read the sign bit */
	if(!FLAC__bitbuffer_read_bit_to_uint32(bb, &sign, read_callback, client_data))
		return false;

	/* read the binary LSBs */
	if(!FLAC__bitbuffer_read_raw_uint32(bb, &lsbs, parameter, read_callback, client_data))
		return false;

	/* compose the value */
	*val = (msbs << parameter) | lsbs;
	if(sign)
		*val = -(*val);

	return true;
}

bool FLAC__bitbuffer_read_rice_signed(FLAC__BitBuffer *bb, int *val, unsigned parameter, bool (*read_callback)(byte buffer[], unsigned *bytes, void *client_data), void *client_data)
{
	uint32 lsbs = 0, msbs = 0;
	unsigned uval;

	assert(bb != 0);
	assert(bb->buffer != 0);
	assert(parameter <= 31);

	/* read the unary MSBs and end bit */
	if(!FLAC__bitbuffer_read_unary_unsigned(bb, &msbs, read_callback, client_data))
		return false;

	/* read the binary LSBs */
	if(!FLAC__bitbuffer_read_raw_uint32(bb, &lsbs, parameter, read_callback, client_data))
		return false;

	/* compose the value */
	uval = (msbs << parameter) | lsbs;
	if(uval & 1)
		*val = -((int)(uval >> 1)) - 1;
	else
		*val = (int)(uval >> 1);

	return true;
}

bool FLAC__bitbuffer_read_golomb_signed(FLAC__BitBuffer *bb, int *val, unsigned parameter, bool (*read_callback)(byte buffer[], unsigned *bytes, void *client_data), void *client_data)
{
	uint32 lsbs = 0, msbs = 0;
	unsigned bit, uval, k;

	assert(bb != 0);
	assert(bb->buffer != 0);

	k = FLAC__bitmath_ilog2(parameter);

	/* read the unary MSBs and end bit */
	if(!FLAC__bitbuffer_read_unary_unsigned(bb, &msbs, read_callback, client_data))
		return false;

	/* read the binary LSBs */
	if(!FLAC__bitbuffer_read_raw_uint32(bb, &lsbs, k, read_callback, client_data))
		return false;

	if(parameter == 1u<<k) {
		/* compose the value */
		uval = (msbs << k) | lsbs;
	}
	else {
		unsigned d = (1 << (k+1)) - parameter;
		if(lsbs >= d) {
			if(!FLAC__bitbuffer_read_bit(bb, &bit, read_callback, client_data))
				return false;
			lsbs <<= 1;
			lsbs |= bit;
			lsbs -= d;
		}
		/* compose the value */
		uval = msbs * parameter + lsbs;
	}

	/* unfold unsigned to signed */
	if(uval & 1)
		*val = -((int)(uval >> 1)) - 1;
	else
		*val = (int)(uval >> 1);

	return true;
}

bool FLAC__bitbuffer_read_golomb_unsigned(FLAC__BitBuffer *bb, unsigned *val, unsigned parameter, bool (*read_callback)(byte buffer[], unsigned *bytes, void *client_data), void *client_data)
{
	uint32 lsbs, msbs = 0;
	unsigned bit, k;

	assert(bb != 0);
	assert(bb->buffer != 0);

	k = FLAC__bitmath_ilog2(parameter);

	/* read the unary MSBs and end bit */
	if(!FLAC__bitbuffer_read_unary_unsigned(bb, &msbs, read_callback, client_data))
		return false;

	/* read the binary LSBs */
	if(!FLAC__bitbuffer_read_raw_uint32(bb, &lsbs, k, read_callback, client_data))
		return false;

	if(parameter == 1u<<k) {
		/* compose the value */
		*val = (msbs << k) | lsbs;
	}
	else {
		unsigned d = (1 << (k+1)) - parameter;
		if(lsbs >= d) {
			if(!FLAC__bitbuffer_read_bit(bb, &bit, read_callback, client_data))
				return false;
			lsbs <<= 1;
			lsbs |= bit;
			lsbs -= d;
		}
		/* compose the value */
		*val = msbs * parameter + lsbs;
	}

	return true;
}

/* on return, if *val == 0xffffffff then the utf-8 sequence was invalid, but the return value will be true */
bool FLAC__bitbuffer_read_utf8_uint32(FLAC__BitBuffer *bb, uint32 *val, bool (*read_callback)(byte buffer[], unsigned *bytes, void *client_data), void *client_data, byte *raw, unsigned *rawlen)
{
	uint32 v = 0;
	uint32 x;
	unsigned i;

	if(!FLAC__bitbuffer_read_raw_uint32(bb, &x, 8, read_callback, client_data))
		return false;
	if(raw)
		raw[(*rawlen)++] = (byte)x;
	if(!(x & 0x80)) { /* 0xxxxxxx */
		v = x;
		i = 0;
	}
	else if(x & 0xC0 && !(x & 0x20)) { /* 110xxxxx */
		v = x & 0x1F;
		i = 1;
	}
	else if(x & 0xE0 && !(x & 0x10)) { /* 1110xxxx */
		v = x & 0x0F;
		i = 2;
	}
	else if(x & 0xF0 && !(x & 0x08)) { /* 11110xxx */
		v = x & 0x07;
		i = 3;
	}
	else if(x & 0xF8 && !(x & 0x04)) { /* 111110xx */
		v = x & 0x03;
		i = 4;
	}
	else if(x & 0xFC && !(x & 0x02)) { /* 1111110x */
		v = x & 0x01;
		i = 5;
	}
	else {
		*val = 0xffffffff;
		return true;
	}
	for( ; i; i--) {
		if(!FLAC__bitbuffer_read_raw_uint32(bb, &x, 8, read_callback, client_data))
			return false;
		if(raw)
			raw[(*rawlen)++] = (byte)x;
		if(!(x & 0x80) || (x & 0x40)) { /* 10xxxxxx */
			*val = 0xffffffff;
			return true;
		}
		v <<= 6;
		v |= (x & 0x3F);
	}
	*val = v;
	return true;
}

/* on return, if *val == 0xffffffffffffffff then the utf-8 sequence was invalid, but the return value will be true */
bool FLAC__bitbuffer_read_utf8_uint64(FLAC__BitBuffer *bb, uint64 *val, bool (*read_callback)(byte buffer[], unsigned *bytes, void *client_data), void *client_data, byte *raw, unsigned *rawlen)
{
	uint64 v = 0;
	uint32 x;
	unsigned i;

	if(!FLAC__bitbuffer_read_raw_uint32(bb, &x, 8, read_callback, client_data))
		return false;
	if(raw)
		raw[(*rawlen)++] = (byte)x;
	if(!(x & 0x80)) { /* 0xxxxxxx */
		v = x;
		i = 0;
	}
	else if(x & 0xC0 && !(x & 0x20)) { /* 110xxxxx */
		v = x & 0x1F;
		i = 1;
	}
	else if(x & 0xE0 && !(x & 0x10)) { /* 1110xxxx */
		v = x & 0x0F;
		i = 2;
	}
	else if(x & 0xF0 && !(x & 0x08)) { /* 11110xxx */
		v = x & 0x07;
		i = 3;
	}
	else if(x & 0xF8 && !(x & 0x04)) { /* 111110xx */
		v = x & 0x03;
		i = 4;
	}
	else if(x & 0xFC && !(x & 0x02)) { /* 1111110x */
		v = x & 0x01;
		i = 5;
	}
	else if(x & 0xFE && !(x & 0x01)) { /* 11111110 */
		v = 0;
		i = 6;
	}
	else {
		*val = 0xffffffffffffffff;
		return true;
	}
	for( ; i; i--) {
		if(!FLAC__bitbuffer_read_raw_uint32(bb, &x, 8, read_callback, client_data))
			return false;
		if(raw)
			raw[(*rawlen)++] = (byte)x;
		if(!(x & 0x80) || (x & 0x40)) { /* 10xxxxxx */
			*val = 0xffffffffffffffff;
			return true;
		}
		v <<= 6;
		v |= (x & 0x3F);
	}
	*val = v;
	return true;
}

void FLAC__bitbuffer_dump(const FLAC__BitBuffer *bb, FILE *out)
{
	unsigned i, j;
	if(bb == 0) {
		fprintf(out, "bitbuffer is NULL\n");
	}
	else {
		fprintf(out, "bitbuffer: capacity=%u bytes=%u bits=%u total_bits=%u consumed: bytes=%u, bits=%u, total_bits=%u\n", bb->capacity, bb->bytes, bb->bits, bb->total_bits, bb->consumed_bytes, bb->consumed_bits, bb->total_consumed_bits);
		for(i = 0; i < bb->bytes; i++) {
			fprintf(out, "%08X: ", i);
			for(j = 0; j < 8; j++)
				if(i*8+j < bb->total_consumed_bits)
					fprintf(out, ".");
				else
					fprintf(out, "%01u", bb->buffer[i] & (1 << (8-j-1)) ? 1:0);
			fprintf(out, "\n");
		}
		if(bb->bits > 0) {
			fprintf(out, "%08X: ", i);
			for(j = 0; j < bb->bits; j++)
				if(i*8+j < bb->total_consumed_bits)
					fprintf(out, ".");
				else
					fprintf(out, "%01u", bb->buffer[i] & (1 << (bb->bits-j-1)) ? 1:0);
			fprintf(out, "\n");
		}
	}
}
