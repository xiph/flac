/* libFLAC - Free Lossless Audio Coder library
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

#ifndef FLAC__FILE_DECODER_H
#define FLAC__FILE_DECODER_H

#include "stream_decoder.h"

typedef enum {
    FLAC__FILE_DECODER_OK = 0,
	FLAC__FILE_DECODER_SEEKING,
	FLAC__FILE_DECODER_END_OF_FILE,
    FLAC__FILE_DECODER_ERROR_OPENING_FILE,
    FLAC__FILE_DECODER_MEMORY_ALLOCATION_ERROR,
	FLAC__FILE_DECODER_SEEK_ERROR,
	FLAC__FILE_DECODER_STREAM_ERROR,
	FLAC__FILE_DECODER_MD5_ERROR,
    FLAC__FILE_DECODER_UNINITIALIZED
} FLAC__FileDecoderState;
extern const char *FLAC__FileDecoderStateString[];

struct FLAC__FileDecoderPrivate;
typedef struct {
	/* this field may not change once FLAC__file_decoder_init() is called */
	bool check_md5; /* if true, generate MD5 signature of decoded data and compare against signature in the Encoding metadata block */

	FLAC__FileDecoderState state; /* must be FLAC__FILE_DECODER_UNINITIALIZED when passed to FLAC__file_decoder_init() */
	struct FLAC__FileDecoderPrivate *guts; /* must be 0 when passed to FLAC__file_decoder_init() */
} FLAC__FileDecoder;

FLAC__FileDecoder *FLAC__file_decoder_get_new_instance();
void FLAC__file_decoder_free_instance(FLAC__FileDecoder *decoder);
FLAC__FileDecoderState FLAC__file_decoder_init(
	FLAC__FileDecoder *decoder,
	const char *filename,
	FLAC__StreamDecoderWriteStatus (*write_callback)(const FLAC__FileDecoder *decoder, const FLAC__FrameHeader *header, const int32 *buffer[], void *client_data),
	void (*metadata_callback)(const FLAC__FileDecoder *decoder, const FLAC__StreamMetaData *metadata, void *client_data),
	void (*error_callback)(const FLAC__FileDecoder *decoder, FLAC__StreamDecoderErrorStatus status, void *client_data),
	void *client_data
);
/* only returns false if check_md5 is set AND the stored MD5 sum is non-zero AND the stored MD5 sum and computed MD5 sum do not match */
bool FLAC__file_decoder_finish(FLAC__FileDecoder *decoder);
bool FLAC__file_decoder_process_whole_file(FLAC__FileDecoder *decoder);
bool FLAC__file_decoder_process_metadata(FLAC__FileDecoder *decoder);
bool FLAC__file_decoder_process_one_frame(FLAC__FileDecoder *decoder);
bool FLAC__file_decoder_process_remaining_frames(FLAC__FileDecoder *decoder);
bool FLAC__file_decoder_seek_absolute(FLAC__FileDecoder *decoder, uint64 sample);

#endif
