/* test_libOggFLAC - Unit tester for libOggFLAC
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

#include "file_utils.h"
#include "FLAC/assert.h"
#include "OggFLAC/stream_encoder.h"
#include <stdio.h>
#include <stdlib.h>
#if defined _MSC_VER || defined __MINGW32__
#include <io.h> /* for chmod(), unlink */
#endif
#include <sys/stat.h> /* for stat(), chmod() */
#if defined _WIN32 && !defined __CYGWIN__
#else
#include <unistd.h> /* for unlink() */
#endif

#ifdef min
#undef min
#endif
#define min(a,b) ((a)<(b)?(a):(b))

const long file_utils__serial_number = 12345;

typedef struct {
	FILE *file;
} encoder_client_struct;

static FLAC__StreamEncoderWriteStatus encoder_write_callback_(const OggFLAC__StreamEncoder *encoder, const FLAC__byte buffer[], unsigned bytes, unsigned samples, unsigned current_frame, void *client_data)
{
	encoder_client_struct *ecd = (encoder_client_struct*)client_data;

	(void)encoder, (void)samples, (void)current_frame;

	if(fwrite(buffer, 1, bytes, ecd->file) != bytes)
		return FLAC__STREAM_ENCODER_WRITE_STATUS_FATAL_ERROR;
	else
		return FLAC__STREAM_ENCODER_WRITE_STATUS_OK;
}

FLAC__bool file_utils__change_stats(const char *filename, FLAC__bool read_only)
{
	struct stat stats;

	if(0 == stat(filename, &stats)) {
#if !defined _MSC_VER && !defined __MINGW32__
		if(read_only) {
			stats.st_mode &= ~S_IWUSR;
			stats.st_mode &= ~S_IWGRP;
			stats.st_mode &= ~S_IWOTH;
		}
		else {
			stats.st_mode |= S_IWUSR;
			stats.st_mode |= S_IWGRP;
			stats.st_mode |= S_IWOTH;
		}
#else
		if(read_only)
			stats.st_mode &= ~S_IWRITE;
		else
			stats.st_mode |= S_IWRITE;
#endif
		if(0 != chmod(filename, stats.st_mode))
			return false;
	}
	else
		return false;

	return true;
}

FLAC__bool file_utils__remove_file(const char *filename)
{
	return file_utils__change_stats(filename, /*read_only=*/false) && 0 == unlink(filename);
}

FLAC__bool file_utils__generate_oggflacfile(const char *output_filename, unsigned *output_filesize, unsigned length, const FLAC__StreamMetadata *streaminfo, FLAC__StreamMetadata **metadata, unsigned num_metadata)
{
	FLAC__int32 samples[1024];
	OggFLAC__StreamEncoder *encoder;
	encoder_client_struct encoder_client_data;
	unsigned i, n;

	FLAC__ASSERT(0 != output_filename);
	FLAC__ASSERT(0 != streaminfo);
	FLAC__ASSERT(streaminfo->type == FLAC__METADATA_TYPE_STREAMINFO);
	FLAC__ASSERT((streaminfo->is_last && num_metadata == 0) || (!streaminfo->is_last && num_metadata > 0));

	if(0 == (encoder_client_data.file = fopen(output_filename, "wb")))
		return false;

	encoder = OggFLAC__stream_encoder_new();
	if(0 == encoder) {
		fclose(encoder_client_data.file);
		return false;
	}

	OggFLAC__stream_encoder_set_serial_number(encoder, file_utils__serial_number);
	OggFLAC__stream_encoder_set_verify(encoder, true);
	OggFLAC__stream_encoder_set_streamable_subset(encoder, true);
	OggFLAC__stream_encoder_set_do_mid_side_stereo(encoder, false);
	OggFLAC__stream_encoder_set_loose_mid_side_stereo(encoder, false);
	OggFLAC__stream_encoder_set_channels(encoder, streaminfo->data.stream_info.channels);
	OggFLAC__stream_encoder_set_bits_per_sample(encoder, streaminfo->data.stream_info.bits_per_sample);
	OggFLAC__stream_encoder_set_sample_rate(encoder, streaminfo->data.stream_info.sample_rate);
	OggFLAC__stream_encoder_set_blocksize(encoder, streaminfo->data.stream_info.min_blocksize);
	OggFLAC__stream_encoder_set_max_lpc_order(encoder, 0);
	OggFLAC__stream_encoder_set_qlp_coeff_precision(encoder, 0);
	OggFLAC__stream_encoder_set_do_qlp_coeff_prec_search(encoder, false);
	OggFLAC__stream_encoder_set_do_escape_coding(encoder, false);
	OggFLAC__stream_encoder_set_do_exhaustive_model_search(encoder, false);
	OggFLAC__stream_encoder_set_min_residual_partition_order(encoder, 0);
	OggFLAC__stream_encoder_set_max_residual_partition_order(encoder, 0);
	OggFLAC__stream_encoder_set_rice_parameter_search_dist(encoder, 0);
	OggFLAC__stream_encoder_set_total_samples_estimate(encoder, streaminfo->data.stream_info.total_samples);
	OggFLAC__stream_encoder_set_metadata(encoder, metadata, num_metadata);
	OggFLAC__stream_encoder_set_write_callback(encoder, encoder_write_callback_);
	OggFLAC__stream_encoder_set_client_data(encoder, &encoder_client_data);

	if(OggFLAC__stream_encoder_init(encoder) != OggFLAC__STREAM_ENCODER_OK) {
		fclose(encoder_client_data.file);
		return false;
	}

	/* init the dummy sample buffer */
	for(i = 0; i < sizeof(samples) / sizeof(FLAC__int32); i++)
		samples[i] = i & 7;

	while(length > 0) {
		n = min(length, sizeof(samples) / sizeof(FLAC__int32));

		if(!OggFLAC__stream_encoder_process_interleaved(encoder, samples, n)) {
			fclose(encoder_client_data.file);
			return false;
		}

		length -= n;
	}

	OggFLAC__stream_encoder_finish(encoder);

	fclose(encoder_client_data.file);

	OggFLAC__stream_encoder_delete(encoder);

	if(0 != output_filesize) {
		struct stat filestats;

		if(stat(output_filename, &filestats) != 0)
			return false;
		else
			*output_filesize = (unsigned)filestats.st_size;
	}

	return true;
}
