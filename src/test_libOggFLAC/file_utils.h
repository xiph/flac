/* test_libOggFLAC - Unit tester for libOggFLAC
 * Copyright (C) 2002,2003  Josh Coalson
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

#ifndef OggFLAC__TEST_LIBOGGFLAC_FILE_UTILS_H
#define OggFLAC__TEST_LIBOGGFLAC_FILE_UTILS_H

#include "FLAC/format.h"

extern const long file_utils__serial_number;

FLAC__bool file_utils__generate_oggflacfile(const char *output_filename, unsigned *output_filesize, unsigned length, const FLAC__StreamMetadata *streaminfo, FLAC__StreamMetadata **metadata, unsigned num_metadata);

#endif
