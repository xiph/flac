/* example_c_decode_file - Simple FLAC file decoder using libFLAC
 * Copyright (C) 2007-2009  Josh Coalson
 * Copyright (C) 2011-2025  Xiph.Org Foundation
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
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

/*
 * This example shows how to use libFLAC to decode a FLAC file to a WAVE
 * file.  It supports 16-bit stereo integer files and 32-bit stereo float files.
 *
 * Complete API documentation can be found at:
 *   http://xiph.org/flac/api/
 */

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include "share/compat.h"
#include "FLAC/stream_decoder.h"

static FLAC__StreamDecoderWriteStatus write_callback(const FLAC__StreamDecoder *decoder, const FLAC__Frame *frame, const FLAC__int32 * const buffer[], void *client_data);
static void metadata_callback(const FLAC__StreamDecoder *decoder, const FLAC__StreamMetadata *metadata, void *client_data);
static void error_callback(const FLAC__StreamDecoder *decoder, FLAC__StreamDecoderErrorStatus status, void *client_data);

static FLAC__uint64 total_samples = 0;
static unsigned sample_rate = 0;
static unsigned channels = 0;
static unsigned bps = 0;
#if ENABLE_EXPERIMENTAL_FLOAT_SAMPLE_CODING
static FLAC__SampleType sample_type = FLAC__SAMPLE_TYPE_INT;
#endif

static FLAC__bool write_little_endian_uint16(FILE *f, FLAC__uint16 x)
{
	return
		fputc(x, f) != EOF &&
		fputc(x >> 8, f) != EOF
	;
}

static FLAC__bool write_little_endian_int16(FILE *f, FLAC__int16 x)
{
	return write_little_endian_uint16(f, (FLAC__uint16)x);
}

static FLAC__bool write_little_endian_uint32(FILE *f, FLAC__uint32 x)
{
	return
		fputc(x, f) != EOF &&
		fputc(x >> 8, f) != EOF &&
		fputc(x >> 16, f) != EOF &&
		fputc(x >> 24, f) != EOF
	;
}

int main(int argc, char *argv[])
{
	FLAC__bool ok = true;
	FLAC__StreamDecoder *decoder = 0;
	FLAC__StreamDecoderInitStatus init_status;
	FILE *fout;

	if(argc != 3) {
		fprintf(stderr, "usage: %s infile.flac outfile.wav\n", argv[0]);
		return 1;
	}

	if((fout = fopen(argv[2], "wb")) == NULL) {
		fprintf(stderr, "ERROR: opening %s for output\n", argv[2]);
		return 1;
	}

	if((decoder = FLAC__stream_decoder_new()) == NULL) {
		fprintf(stderr, "ERROR: allocating decoder\n");
		fclose(fout);
		return 1;
	}

	(void)FLAC__stream_decoder_set_md5_checking(decoder, true);

	init_status = FLAC__stream_decoder_init_file(decoder, argv[1], write_callback, metadata_callback, error_callback, /*client_data=*/fout);
	if(init_status != FLAC__STREAM_DECODER_INIT_STATUS_OK) {
		fprintf(stderr, "ERROR: initializing decoder: %s\n", FLAC__StreamDecoderInitStatusString[init_status]);
		ok = false;
	}

	if(ok) {
		ok = FLAC__stream_decoder_process_until_end_of_stream(decoder);
		fprintf(stderr, "decoding: %s\n", ok? "succeeded" : "FAILED");
		fprintf(stderr, "   state: %s\n", FLAC__StreamDecoderStateString[FLAC__stream_decoder_get_state(decoder)]);
	}

	FLAC__stream_decoder_delete(decoder);
	fclose(fout);

	return 0;
}

FLAC__StreamDecoderWriteStatus write_callback(const FLAC__StreamDecoder *decoder, const FLAC__Frame *frame, const FLAC__int32 * const buffer[], void *client_data)
{
	FILE *f = (FILE*)client_data;
	const FLAC__uint32 total_size = (FLAC__uint32)(total_samples * channels * (bps/8));
	size_t i;

#if ENABLE_EXPERIMENTAL_FLOAT_SAMPLE_CODING
	sample_type = FLAC__stream_decoder_get_sample_type(decoder);
#endif

	if(total_samples == 0) {
		fprintf(stderr, "ERROR: this example only works for FLAC files that have a total_samples count in STREAMINFO\n");
		return FLAC__STREAM_DECODER_WRITE_STATUS_ABORT;
	}
#if ENABLE_EXPERIMENTAL_FLOAT_SAMPLE_CODING
	if(channels != 2 || (sample_type == FLAC__SAMPLE_TYPE_INT && bps != 16) || (sample_type == FLAC__SAMPLE_TYPE_FLOAT && bps != 32)) {
		fprintf(stderr, "ERROR: this example only supports 16bit stereo integer or 32bit stereo float streams\n");
		return FLAC__STREAM_DECODER_WRITE_STATUS_ABORT;
	}
#else
	if(channels != 2 || bps != 16) {
		fprintf(stderr, "ERROR: this example only supports 16bit stereo streams\n");
		return FLAC__STREAM_DECODER_WRITE_STATUS_ABORT;
	}
#endif
	if(frame->header.channels != 2) {
		fprintf(stderr, "ERROR: This frame contains %u channels (should be 2)\n", frame->header.channels);
		return FLAC__STREAM_DECODER_WRITE_STATUS_ABORT;
	}
	if(buffer [0] == NULL) {
		fprintf(stderr, "ERROR: buffer [0] is NULL\n");
		return FLAC__STREAM_DECODER_WRITE_STATUS_ABORT;
	}
	if(buffer [1] == NULL) {
		fprintf(stderr, "ERROR: buffer [1] is NULL\n");
		return FLAC__STREAM_DECODER_WRITE_STATUS_ABORT;
	}

	/* write WAVE header before we write the first frame */
	if(frame->header.number.sample_number == 0) {
#if ENABLE_EXPERIMENTAL_FLOAT_SAMPLE_CODING
		const FLAC__uint16 format_tag = (sample_type == FLAC__SAMPLE_TYPE_FLOAT) ? 3 : 1;
#else
		const FLAC__uint16 format_tag = 1;
#endif
		if(
			fwrite("RIFF", 1, 4, f) < 4 ||
			!write_little_endian_uint32(f, total_size + 36) ||
			fwrite("WAVEfmt ", 1, 8, f) < 8 ||
			!write_little_endian_uint32(f, 16) ||
			!write_little_endian_uint16(f, format_tag) ||
			!write_little_endian_uint16(f, (FLAC__uint16)channels) ||
			!write_little_endian_uint32(f, sample_rate) ||
			!write_little_endian_uint32(f, sample_rate * channels * (bps/8)) ||
			!write_little_endian_uint16(f, (FLAC__uint16)(channels * (bps/8))) || /* block align */
			!write_little_endian_uint16(f, (FLAC__uint16)bps) ||
			fwrite("data", 1, 4, f) < 4 ||
			!write_little_endian_uint32(f, total_size)
		) {
			fprintf(stderr, "ERROR: write error\n");
			return FLAC__STREAM_DECODER_WRITE_STATUS_ABORT;
		}
	}

	/* write decoded samples */
#if ENABLE_EXPERIMENTAL_FLOAT_SAMPLE_CODING
	if(sample_type == FLAC__SAMPLE_TYPE_FLOAT) {
		for(i = 0; i < frame->header.blocksize; i++) {
			if(
				!write_little_endian_uint32(f, (FLAC__uint32)buffer[0][i]) ||  /* left channel */
				!write_little_endian_uint32(f, (FLAC__uint32)buffer[1][i])     /* right channel */
			) {
				fprintf(stderr, "ERROR: write error\n");
				return FLAC__STREAM_DECODER_WRITE_STATUS_ABORT;
			}
		}
	} else
#endif
	{
		for(i = 0; i < frame->header.blocksize; i++) {
			if(
				!write_little_endian_int16(f, (FLAC__int16)buffer[0][i]) ||  /* left channel */
				!write_little_endian_int16(f, (FLAC__int16)buffer[1][i])     /* right channel */
			) {
				fprintf(stderr, "ERROR: write error\n");
				return FLAC__STREAM_DECODER_WRITE_STATUS_ABORT;
			}
		}
	}

	return FLAC__STREAM_DECODER_WRITE_STATUS_CONTINUE;
}

void metadata_callback(const FLAC__StreamDecoder *decoder, const FLAC__StreamMetadata *metadata, void *client_data)
{
	(void)decoder, (void)client_data;

	/* print some stats */
	if(metadata->type == FLAC__METADATA_TYPE_STREAMINFO) {
		/* save for later */
		total_samples = metadata->data.stream_info.total_samples;
		sample_rate = metadata->data.stream_info.sample_rate;
		channels = metadata->data.stream_info.channels;
		bps = metadata->data.stream_info.bits_per_sample;

		fprintf(stderr, "sample rate    : %u Hz\n", sample_rate);
		fprintf(stderr, "channels       : %u\n", channels);
		fprintf(stderr, "bits per sample: %u\n", bps);
		fprintf(stderr, "total samples  : %" PRIu64 "\n", total_samples);
	}
#if ENABLE_EXPERIMENTAL_FLOAT_SAMPLE_CODING
	else if(metadata->type == FLAC__METADATA_TYPE_STREAMINFO_EXTENSION) {
		/* save for later */
		sample_rate = (unsigned)metadata->data.stream_info_extension.sample_rate;
		bps = metadata->data.stream_info_extension.bits_per_sample;
		channels = metadata->data.stream_info_extension.channels;
		sample_type = metadata->data.stream_info_extension.sample_type;

		fprintf(stderr, "sample rate    : %f Hz\n", metadata->data.stream_info_extension.sample_rate);
		fprintf(stderr, "bits per sample: %u\n", metadata->data.stream_info_extension.bits_per_sample);
		fprintf(stderr, "sample type    : %s\n", FLAC__get_sample_type_string(metadata->data.stream_info_extension.sample_type));
		fprintf(stderr, "channels       : %u\n", metadata->data.stream_info_extension.channels);
		fprintf(stderr, "channel mask   : %u\n", metadata->data.stream_info_extension.channel_mask);
	}
#endif
}

void error_callback(const FLAC__StreamDecoder *decoder, FLAC__StreamDecoderErrorStatus status, void *client_data)
{
	(void)decoder, (void)client_data;

	fprintf(stderr, "Got error callback: %s\n", FLAC__StreamDecoderErrorStatusString[status]);
}
