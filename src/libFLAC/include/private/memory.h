/* libFLAC - Free Lossless Audio Codec library
 * Copyright (C) 2001,2002,2003  Josh Coalson
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

#ifndef FLAC__PRIVATE__MEMORY_H
#define FLAC__PRIVATE__MEMORY_H

#include <stdlib.h> /* for size_t */

#include "FLAC/ordinals.h" /* for FLAC__bool */

/* Returns the unaligned address returned by malloc.
 * Use free() on this address to deallocate.
 */
void *FLAC__memory_alloc_aligned(size_t bytes, void **aligned_address);
FLAC__bool FLAC__memory_alloc_aligned_int32_array(unsigned elements, FLAC__int32 **unaligned_pointer, FLAC__int32 **aligned_pointer);
FLAC__bool FLAC__memory_alloc_aligned_uint32_array(unsigned elements, FLAC__uint32 **unaligned_pointer, FLAC__uint32 **aligned_pointer);
FLAC__bool FLAC__memory_alloc_aligned_uint64_array(unsigned elements, FLAC__uint64 **unaligned_pointer, FLAC__uint64 **aligned_pointer);
FLAC__bool FLAC__memory_alloc_aligned_unsigned_array(unsigned elements, unsigned **unaligned_pointer, unsigned **aligned_pointer);
FLAC__bool FLAC__memory_alloc_aligned_real_array(unsigned elements, FLAC__real **unaligned_pointer, FLAC__real **aligned_pointer);

#endif
