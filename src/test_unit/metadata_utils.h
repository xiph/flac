/* test_unit - Simple FLAC unit tester
 * Copyright (C) 2002  Josh Coalson
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

#ifndef FLAC__TEST_UNIT_METADATA_H
#define FLAC__TEST_UNIT_METADATA_H

/*
 * These are not tests, just utility functions used by the metadata tests
 */

#include "FLAC/format.h"
#include <stdio.h>
#include <stdlib.h> /* for malloc() */
#include <string.h> /* for memcmp() */

FLAC__bool compare_block_data_streaminfo_(const FLAC__StreamMetaData_StreamInfo *block, const FLAC__StreamMetaData_StreamInfo *blockcopy);

FLAC__bool compare_block_data_padding_(const FLAC__StreamMetaData_Padding *block, const FLAC__StreamMetaData_Padding *blockcopy, unsigned block_length);

FLAC__bool compare_block_data_application_(const FLAC__StreamMetaData_Application *block, const FLAC__StreamMetaData_Application *blockcopy, unsigned block_length);

FLAC__bool compare_block_data_seektable_(const FLAC__StreamMetaData_SeekTable *block, const FLAC__StreamMetaData_SeekTable *blockcopy);

FLAC__bool compare_block_data_vorbiscomment_(const FLAC__StreamMetaData_VorbisComment *block, const FLAC__StreamMetaData_VorbisComment *blockcopy);

FLAC__bool compare_block_(const FLAC__StreamMetaData *block, const FLAC__StreamMetaData *blockcopy);

#endif
