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

#include "FLAC/seek_table.h"

FLAC__bool FLAC__seek_table_is_valid(const FLAC__StreamMetaData_SeekTable *seek_table)
{
	unsigned i;
	FLAC__uint64 last_sample_number = 0;
	FLAC__bool got_last = false;

	for(i = 0; i < seek_table->num_points; i++) {
		if(seek_table->points[i].sample_number != FLAC__STREAM_METADATA_SEEKPOINT_PLACEHOLDER) {
			if(got_last) {
				if(seek_table->points[i].sample_number <= last_sample_number)
					return false;
			}
			last_sample_number = seek_table->points[i].sample_number;
			got_last = true;
		}
	}

	return true;
}
