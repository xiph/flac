/* flac - Command-line FLAC encoder/decoder
 * Copyright (C) 2000,2001,2002,2003,2004,2005,2006,2007  Josh Coalson
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

#ifndef flac__foreign_metadata_h
#define flac__foreign_metadata_h

#if HAVE_CONFIG_H
#  include <config.h>
#endif

#include "FLAC/metadata.h"
#include "utils.h"

typedef struct {
	/* for encoding, this will be the offset in the WAVE/AIFF file of the chunk */
	/* for decoding, this will be the offset in the FLAC file of the chunk data inside the APPLICATION block */
	off_t offset;
	FLAC__uint32 size;
} foreign_block_t;

typedef struct {
	foreign_block_t *blocks;
	size_t num_blocks;
	size_t format_block; /* block number of 'fmt ' or 'COMM' chunk */
	size_t audio_block; /* block number of 'data' or 'SSND' chunk */
} foreign_metadata_t;

foreign_metadata_t *flac__foreign_metadata_new(void);

void flac__foreign_metadata_delete(foreign_metadata_t *fm);

FLAC__bool flac__foreign_metadata_read_from_aiff(foreign_metadata_t *fm, const char *filename, const char **error);
FLAC__bool flac__foreign_metadata_read_from_wave(foreign_metadata_t *fm, const char *filename, const char **error);
FLAC__bool flac__foreign_metadata_write_to_flac(foreign_metadata_t *fm, const char *infilename, const char *outfilename, const char **error);

FLAC__bool flac__foreign_metadata_read_from_flac(foreign_metadata_t *fm, const char *filename, const char **error);
FLAC__bool flac__foreign_metadata_write_to_aiff(foreign_metadata_t *fm, const char *infilename, const char *outfilename, const char **error);
FLAC__bool flac__foreign_metadata_write_to_wave(foreign_metadata_t *fm, const char *infilename, const char *outfilename, const char **error);

#endif
