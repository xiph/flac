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

#include <assert.h>
#include "private/memory.h"

void *FLAC__memory_alloc(size_t bytes, void **aligned_address)
{
	void *x;

	assert(0 != aligned_address);

#ifdef FLAC__ALIGN_MALLOC_DATA
	/* align on 32-byte (256-bit) boundary */
	x = malloc(bytes+31);
	*aligned_address = (void*)(((unsigned)x + 31) & -32);
fprintf(stderr, "FLAC__memory_alloc(unaligned=%p, aligned=%p)\n",x,*aligned_address);
#else
	x = malloc(bytes);
	*aligned_address = x;
#endif
	return x;
}
