/* test_libFLAC - Unit tester for libFLAC
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

#ifndef FLAC__TEST_LIBFLAC_METADATA_H
#define FLAC__TEST_LIBFLAC_METADATA_H

/*
 * These are not tests, just utility functions used by the metadata tests
 */

#include "FLAC/format.h"
#include <stdio.h>
#include <stdlib.h> /* for malloc() */
#include <string.h> /* for memcmp() */

FLAC__bool compare_block_data_streaminfo_(const FLAC__StreamMetadata_StreamInfo *block, const FLAC__StreamMetadata_StreamInfo *blockcopy);

FLAC__bool compare_block_data_padding_(const FLAC__StreamMetadata_Padding *block, const FLAC__StreamMetadata_Padding *blockcopy, unsigned block_length);

FLAC__bool compare_block_data_application_(const FLAC__StreamMetadata_Application *block, const FLAC__StreamMetadata_Application *blockcopy, unsigned block_length);

FLAC__bool compare_block_data_seektable_(const FLAC__StreamMetadata_SeekTable *block, const FLAC__StreamMetadata_SeekTable *blockcopy);

FLAC__bool compare_block_data_vorbiscomment_(const FLAC__StreamMetadata_VorbisComment *block, const FLAC__StreamMetadata_VorbisComment *blockcopy);

FLAC__bool compare_block_data_cuesheet_(const FLAC__StreamMetadata_CueSheet *block, const FLAC__StreamMetadata_CueSheet *blockcopy);

FLAC__bool compare_block_(const FLAC__StreamMetadata *block, const FLAC__StreamMetadata *blockcopy);

#endif
