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

#ifndef FLAC__PRIVATE__FORMAT_H
#define FLAC__PRIVATE__FORMAT_H

#include "FLAC/format.h"

unsigned FLAC__format_get_max_rice_partition_order(unsigned blocksize, unsigned predictor_order);
unsigned FLAC__format_get_max_rice_partition_order_from_blocksize(unsigned blocksize);
unsigned FLAC__format_get_max_rice_partition_order_from_blocksize_limited_max_and_predictor_order(unsigned limit, unsigned blocksize, unsigned predictor_order);
void FLAC__format_entropy_coding_method_partitioned_rice_init(FLAC__EntropyCodingMethod_PartitionedRice *object);
void FLAC__format_entropy_coding_method_partitioned_rice_clear(FLAC__EntropyCodingMethod_PartitionedRice *object);
FLAC__bool FLAC__format_entropy_coding_method_partitioned_rice_ensure_size(FLAC__EntropyCodingMethod_PartitionedRice *object, unsigned max_partition_order);

#endif
