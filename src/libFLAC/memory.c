/* libFLAC - Free Lossless Audio Codec library
 * Copyright (C) 2001  Josh Coalson
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

#include "private/memory.h"
#include "FLAC/assert.h"

void *FLAC__memory_alloc_aligned(size_t bytes, void **aligned_address)
{
	void *x;

	FLAC__ASSERT(0 != aligned_address);

#ifdef FLAC__ALIGN_MALLOC_DATA
	/* align on 32-byte (256-bit) boundary */
	x = malloc(bytes+31);
	*aligned_address = (void*)(((unsigned)x + 31) & -32);
#else
	x = malloc(bytes);
	*aligned_address = x;
#endif
	return x;
}

bool FLAC__memory_alloc_aligned_int32_array(unsigned elements, int32 **unaligned_pointer, int32 **aligned_pointer)
{
	int32 *pa, *pu; /* aligned pointer, unaligned pointer */

	FLAC__ASSERT(elements > 0);
	FLAC__ASSERT(0 != unaligned_pointer);
	FLAC__ASSERT(0 != aligned_pointer);
	FLAC__ASSERT(unaligned_pointer != aligned_pointer);

	pu = (int32*)FLAC__memory_alloc_aligned(sizeof(int32) * elements, (void*)&pa);
	if(0 == pu) {
		return false;
	}
	else {
		if(*unaligned_pointer != 0)
			free(*unaligned_pointer);
		*unaligned_pointer = pu;
		*aligned_pointer = pa;
		return true;
	}
}

bool FLAC__memory_alloc_aligned_uint32_array(unsigned elements, uint32 **unaligned_pointer, uint32 **aligned_pointer)
{
	uint32 *pa, *pu; /* aligned pointer, unaligned pointer */

	FLAC__ASSERT(elements > 0);
	FLAC__ASSERT(0 != unaligned_pointer);
	FLAC__ASSERT(0 != aligned_pointer);
	FLAC__ASSERT(unaligned_pointer != aligned_pointer);

	pu = (uint32*)FLAC__memory_alloc_aligned(sizeof(uint32) * elements, (void*)&pa);
	if(0 == pu) {
		return false;
	}
	else {
		if(*unaligned_pointer != 0)
			free(*unaligned_pointer);
		*unaligned_pointer = pu;
		*aligned_pointer = pa;
		return true;
	}
}

bool FLAC__memory_alloc_aligned_unsigned_array(unsigned elements, unsigned **unaligned_pointer, unsigned **aligned_pointer)
{
	unsigned *pa, *pu; /* aligned pointer, unaligned pointer */

	FLAC__ASSERT(elements > 0);
	FLAC__ASSERT(0 != unaligned_pointer);
	FLAC__ASSERT(0 != aligned_pointer);
	FLAC__ASSERT(unaligned_pointer != aligned_pointer);

	pu = (unsigned*)FLAC__memory_alloc_aligned(sizeof(unsigned) * elements, (void*)&pa);
	if(0 == pu) {
		return false;
	}
	else {
		if(*unaligned_pointer != 0)
			free(*unaligned_pointer);
		*unaligned_pointer = pu;
		*aligned_pointer = pa;
		return true;
	}
}

bool FLAC__memory_alloc_aligned_real_array(unsigned elements, real **unaligned_pointer, real **aligned_pointer)
{
	real *pa, *pu; /* aligned pointer, unaligned pointer */

	FLAC__ASSERT(elements > 0);
	FLAC__ASSERT(0 != unaligned_pointer);
	FLAC__ASSERT(0 != aligned_pointer);
	FLAC__ASSERT(unaligned_pointer != aligned_pointer);

	pu = (real*)FLAC__memory_alloc_aligned(sizeof(real) * elements, (void*)&pa);
	if(0 == pu) {
		return false;
	}
	else {
		if(*unaligned_pointer != 0)
			free(*unaligned_pointer);
		*unaligned_pointer = pu;
		*aligned_pointer = pa;
		return true;
	}
}
