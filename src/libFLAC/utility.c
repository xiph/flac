/* libFLAC - Free Lossless Audio Codec library
 * Copyright (C) 2001,2002  Josh Coalson
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

#include "FLAC/all.h"

static FLAC__StreamDecoderWriteStatus write_callback_(const FLAC__FileDecoder *decoder, const FLAC__Frame *frame, const FLAC__int32 *buffer[], void *client_data);
static void metadata_callback_(const FLAC__FileDecoder *decoder, const FLAC__StreamMetaData *metadata, void *client_data);
static void error_callback_(const FLAC__FileDecoder *decoder, FLAC__StreamDecoderErrorStatus status, void *client_data);

FLAC__bool FLAC__utility_get_streaminfo(const char *filename, FLAC__StreamMetaData_StreamInfo *streaminfo)
{
	FLAC__FileDecoder *decoder = FLAC__file_decoder_new();

	if(0 == decoder)
		return false;

	FLAC__file_decoder_set_md5_checking(decoder, false);
	FLAC__file_decoder_set_filename(decoder, filename);
	FLAC__file_decoder_set_write_callback(decoder, write_callback_);
	FLAC__file_decoder_set_metadata_callback(decoder, metadata_callback_);
	FLAC__file_decoder_set_error_callback(decoder, error_callback_);
	FLAC__file_decoder_set_client_data(decoder, &streaminfo);

	if(FLAC__file_decoder_init(decoder) != FLAC__FILE_DECODER_OK)
		return false;

	if(!FLAC__file_decoder_process_metadata(decoder))
		return false;

	if(FLAC__file_decoder_get_state(decoder) != FLAC__FILE_DECODER_UNINITIALIZED)
		FLAC__file_decoder_finish(decoder);

	FLAC__file_decoder_delete(decoder);

	return 0 != streaminfo; /* the metadata_callback_() will set streaminfo to 0 on an error */
}

FLAC__StreamDecoderWriteStatus write_callback_(const FLAC__FileDecoder *decoder, const FLAC__Frame *frame, const FLAC__int32 *buffer[], void *client_data)
{
	(void)decoder, (void)frame, (void)buffer, (void)client_data;

	return FLAC__STREAM_DECODER_WRITE_CONTINUE;
}

void metadata_callback_(const FLAC__FileDecoder *decoder, const FLAC__StreamMetaData *metadata, void *client_data)
{
	FLAC__StreamMetaData_StreamInfo **streaminfo = (FLAC__StreamMetaData_StreamInfo **)client_data;
	(void)decoder;

	if(metadata->type == FLAC__METADATA_TYPE_STREAMINFO && 0 != *streaminfo)
		**streaminfo = metadata->data.stream_info;
}

void error_callback_(const FLAC__FileDecoder *decoder, FLAC__StreamDecoderErrorStatus status, void *client_data)
{
	FLAC__StreamMetaData_StreamInfo **streaminfo = (FLAC__StreamMetaData_StreamInfo **)client_data;
	(void)decoder;

	if(status != FLAC__STREAM_DECODER_ERROR_LOST_SYNC)
		*streaminfo = 0;
}
