/* flac - Command-line FLAC encoder/decoder
 * Copyright (C) 2000,2001  Josh Coalson
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

#if defined _WIN32 && !defined __CYGWIN__
/* where MSVC puts unlink() */
# include <io.h>
#else
# include <unistd.h>
#endif
#include <assert.h> /* for FILE */
#include <stdio.h> /* for FILE */
#include <string.h> /* for strcmp() */
#include "FLAC/all.h"
#include "decode.h"

typedef struct {
	FILE *fout;
	bool abort_flag;
	bool analysis_mode;
	analysis_options aopts;
	bool test_only;
	bool is_wave_out;
	bool is_big_endian;
	bool is_unsigned_samples;
	uint64 total_samples;
	unsigned bps;
	unsigned channels;
	unsigned sample_rate;
	bool verbose;
	uint64 skip;
	uint64 samples_processed;
	unsigned frame_counter;
} stream_info_struct;

static FLAC__FileDecoder *decoder;
static bool is_big_endian_host;

/* local routines */
static bool init(const char *infile, stream_info_struct *stream_info);
static bool write_little_endian_uint16(FILE *f, uint16 val);
static bool write_little_endian_uint32(FILE *f, uint32 val);
static FLAC__StreamDecoderWriteStatus write_callback(const FLAC__FileDecoder *decoder, const FLAC__Frame *frame, const int32 *buffer[], void *client_data);
static void metadata_callback(const FLAC__FileDecoder *decoder, const FLAC__StreamMetaData *metadata, void *client_data);
static void error_callback(const FLAC__FileDecoder *decoder, FLAC__StreamDecoderErrorStatus status, void *client_data);
static void print_stats(const stream_info_struct *stream_info);

int decode_wav(const char *infile, const char *outfile, bool analysis_mode, analysis_options aopts, bool verbose, uint64 skip)
{
	bool md5_failure = false;
	stream_info_struct stream_info;

	decoder = 0;
	stream_info.abort_flag = false;
	stream_info.analysis_mode = analysis_mode;
	stream_info.aopts = aopts;
	stream_info.test_only = (outfile == 0);
	stream_info.is_wave_out = true;
	stream_info.verbose = verbose;
	stream_info.skip = skip;
	stream_info.samples_processed = 0;
	stream_info.frame_counter = 0;

	assert(!(stream_info.test_only && stream_info.analysis_mode));

	if(!stream_info.test_only) {
		if(0 == strcmp(outfile, "-")) {
			stream_info.fout = stdout;
		}
		else {
			if(0 == (stream_info.fout = fopen(outfile, "wb"))) {
				fprintf(stderr, "ERROR: can't open output file %s\n", outfile);
				return false;
			}
		}
	}

	if(analysis_mode)
		analyze_init();

	if(!init(infile, &stream_info))
		goto wav_abort_;

	if(skip > 0) {
		if(!FLAC__file_decoder_process_metadata(decoder)) {
			fprintf(stderr, "%s: ERROR during decoding\n", infile);
			goto wav_abort_;
		}
		if(!FLAC__file_decoder_seek_absolute(decoder, skip)) {
			fprintf(stderr, "%s: ERROR seeking while skipping bytes\n", infile);
			goto wav_abort_;
		}
		if(!FLAC__file_decoder_process_remaining_frames(decoder)) {
			if(verbose) { printf("\n"); fflush(stdout); }
			fprintf(stderr, "%s: ERROR during decoding\n", infile);
			goto wav_abort_;
		}
		if(decoder->state != FLAC__FILE_DECODER_OK && decoder->state != FLAC__FILE_DECODER_END_OF_FILE) {
			if(verbose) { printf("\n"); fflush(stdout); }
			fprintf(stderr, "%s: ERROR during decoding\n", infile);
			goto wav_abort_;
		}
	}
	else {
		if(!FLAC__file_decoder_process_whole_file(decoder)) {
			if(verbose) { printf("\n"); fflush(stdout); }
			fprintf(stderr, "%s: ERROR during decoding\n", infile);
			goto wav_abort_;
		}
		if(decoder->state != FLAC__FILE_DECODER_OK && decoder->state != FLAC__FILE_DECODER_END_OF_FILE) {
			if(verbose) { printf("\n"); fflush(stdout); }
			fprintf(stderr, "%s: ERROR during decoding, state=%d:%s\n", infile, decoder->state, FLAC__FileDecoderStateString[decoder->state]);
			goto wav_abort_;
		}
	}

	if(decoder) {
		if(decoder->state != FLAC__FILE_DECODER_UNINITIALIZED)
			md5_failure = !FLAC__file_decoder_finish(decoder);
		print_stats(&stream_info);
		FLAC__file_decoder_free_instance(decoder);
	}
	if(!stream_info.test_only)
		fclose(stream_info.fout);
	if(verbose)
		printf("\n");
	fflush(stdout);
	if(analysis_mode)
		analyze_finish();
	if(md5_failure) {
		fprintf(stderr, "%s: WARNING, MD5 signature mismatch\n", infile);
		return 1;
	}
	else {
		if(stream_info.test_only)
			printf("%s: ok\n", infile);
	}
	return 0;
wav_abort_:
	if(decoder) {
		if(decoder->state != FLAC__FILE_DECODER_UNINITIALIZED)
			FLAC__file_decoder_finish(decoder);
		FLAC__file_decoder_free_instance(decoder);
	}
	if(!stream_info.test_only) {
		fclose(stream_info.fout);
		unlink(outfile);
	}
	if(analysis_mode)
		analyze_finish();
	return 1;
}

int decode_raw(const char *infile, const char *outfile, bool analysis_mode, analysis_options aopts, bool verbose, uint64 skip, bool is_big_endian, bool is_unsigned_samples)
{
	bool md5_failure = false;
	stream_info_struct stream_info;

	decoder = 0;
	stream_info.abort_flag = false;
	stream_info.analysis_mode = analysis_mode;
	stream_info.aopts = aopts;
	stream_info.test_only = (outfile == 0);
	stream_info.is_wave_out = false;
	stream_info.is_big_endian = is_big_endian;
	stream_info.is_unsigned_samples = is_unsigned_samples;
	stream_info.verbose = verbose;
	stream_info.skip = skip;
	stream_info.samples_processed = 0;
	stream_info.frame_counter = 0;

	assert(!(stream_info.test_only && stream_info.analysis_mode));

	if(!stream_info.test_only) {
		if(0 == strcmp(outfile, "-")) {
			stream_info.fout = stdout;
		}
		else {
			if(0 == (stream_info.fout = fopen(outfile, "wb"))) {
				fprintf(stderr, "ERROR: can't open output file %s\n", outfile);
				return false;
			}
		}
	}

	if(analysis_mode)
		analyze_init();

	if(!init(infile, &stream_info))
		goto raw_abort_;

	if(skip > 0) {
		if(!FLAC__file_decoder_process_metadata(decoder)) {
			fprintf(stderr, "%s: ERROR during decoding\n", infile);
			goto raw_abort_;
		}
		if(!FLAC__file_decoder_seek_absolute(decoder, skip)) {
			fprintf(stderr, "%s: ERROR seeking while skipping bytes\n", infile);
			goto raw_abort_;
		}
		if(!FLAC__file_decoder_process_remaining_frames(decoder)) {
			if(verbose) { printf("\n"); fflush(stdout); }
			fprintf(stderr, "%s: ERROR during decoding\n", infile);
			goto raw_abort_;
		}
		if(decoder->state != FLAC__FILE_DECODER_OK && decoder->state != FLAC__FILE_DECODER_END_OF_FILE) {
			if(verbose) { printf("\n"); fflush(stdout); }
			fprintf(stderr, "%s: ERROR during decoding\n", infile);
			goto raw_abort_;
		}
	}
	else {
		if(!FLAC__file_decoder_process_whole_file(decoder)) {
			if(verbose) { printf("\n"); fflush(stdout); }
			fprintf(stderr, "%s: ERROR during decoding\n", infile);
			goto raw_abort_;
		}
		if(decoder->state != FLAC__FILE_DECODER_OK && decoder->state != FLAC__FILE_DECODER_END_OF_FILE) {
			if(verbose) { printf("\n"); fflush(stdout); }
			fprintf(stderr, "%s: ERROR during decoding\n", infile);
			goto raw_abort_;
		}
	}

	if(decoder) {
		if(decoder->state != FLAC__FILE_DECODER_UNINITIALIZED)
			md5_failure = !FLAC__file_decoder_finish(decoder);
		print_stats(&stream_info);
		FLAC__file_decoder_free_instance(decoder);
	}
	if(!stream_info.test_only)
		fclose(stream_info.fout);
	if(verbose)
		printf("\n");
	fflush(stdout);
	if(analysis_mode)
		analyze_finish();
	if(md5_failure) {
		fprintf(stderr, "%s: WARNING, MD5 signature mismatch\n", infile);
		return 1;
	}
	else {
		if(stream_info.test_only)
			printf("%s: ok\n", infile);
	}
	return 0;
raw_abort_:
	if(decoder) {
		if(decoder->state != FLAC__FILE_DECODER_UNINITIALIZED)
			FLAC__file_decoder_finish(decoder);
		FLAC__file_decoder_free_instance(decoder);
	}
	if(!stream_info.test_only) {
		fclose(stream_info.fout);
		unlink(outfile);
	}
	if(analysis_mode)
		analyze_finish();
	return 1;
}

bool init(const char *infile, stream_info_struct *stream_info)
{
	uint32 test = 1;

	is_big_endian_host = (*((byte*)(&test)))? false : true;

	decoder = FLAC__file_decoder_get_new_instance();
	if(0 == decoder) {
		fprintf(stderr, "ERROR creating the decoder instance\n");
		return false;
	}
	decoder->check_md5 = true;

	if(FLAC__file_decoder_init(decoder, infile, write_callback, metadata_callback, error_callback, stream_info) != FLAC__FILE_DECODER_OK) {
		fprintf(stderr, "ERROR initializing decoder, state = %d\n", decoder->state);
		return false;
	}

	return true;
}

bool write_little_endian_uint16(FILE *f, uint16 val)
{
	byte *b = (byte*)(&val);
	if(is_big_endian_host) {
		byte tmp;
		tmp = b[1]; b[1] = b[0]; b[0] = tmp;
	}
	return fwrite(b, 1, 2, f) == 2;
}

bool write_little_endian_uint32(FILE *f, uint32 val)
{
	byte *b = (byte*)(&val);
	if(is_big_endian_host) {
		byte tmp;
		tmp = b[3]; b[3] = b[0]; b[0] = tmp;
		tmp = b[2]; b[2] = b[1]; b[1] = tmp;
	}
	return fwrite(b, 1, 4, f) == 4;
}

FLAC__StreamDecoderWriteStatus write_callback(const FLAC__FileDecoder *decoder, const FLAC__Frame *frame, const int32 *buffer[], void *client_data)
{
	stream_info_struct *stream_info = (stream_info_struct *)client_data;
	FILE *fout = stream_info->fout;
	unsigned bps = stream_info->bps, channels = stream_info->channels;
	bool is_big_endian = (stream_info->is_wave_out? false : stream_info->is_big_endian);
	bool is_unsigned_samples = (stream_info->is_wave_out? bps<=8 : stream_info->is_unsigned_samples);
	unsigned wide_samples = frame->header.blocksize, wide_sample, sample, channel, byte;
	static int8 s8buffer[FLAC__MAX_BLOCK_SIZE * FLAC__MAX_CHANNELS * sizeof(int32)]; /* WATCHOUT: can be up to 2 megs */
	/* WATCHOUT: we say 'sizeof(int32)' above instead of '(FLAC__MAX_BITS_PER_SAMPLE+7)/8' because we have to use an array int32 even for 24 bps */
	uint8  *u8buffer  = (uint8  *)s8buffer;
	int16  *s16buffer = (int16  *)s8buffer;
	uint16 *u16buffer = (uint16 *)s8buffer;
	int32  *s32buffer = (int32  *)s8buffer;
	uint32 *u32buffer = (uint32 *)s8buffer;

	(void)decoder;

	if(stream_info->abort_flag)
		return FLAC__STREAM_DECODER_WRITE_ABORT;

	stream_info->samples_processed += wide_samples;
	stream_info->frame_counter++;

	if(stream_info->verbose && !(stream_info->frame_counter & 0x1f))
		print_stats(stream_info);

	if(stream_info->analysis_mode) {
		analyze_frame(frame, stream_info->frame_counter-1, stream_info->aopts, fout);
	}
	else if(!stream_info->test_only) {
		if(bps == 8) {
			if(is_unsigned_samples) {
				for(sample = wide_sample = 0; wide_sample < wide_samples; wide_sample++)
					for(channel = 0; channel < channels; channel++, sample++)
						u8buffer[sample] = buffer[channel][wide_sample] + 0x80;
			}
			else {
				for(sample = wide_sample = 0; wide_sample < wide_samples; wide_sample++)
					for(channel = 0; channel < channels; channel++, sample++)
						s8buffer[sample] = buffer[channel][wide_sample];
			}
			if(fwrite(u8buffer, 1, sample, fout) != sample)
				return FLAC__STREAM_DECODER_WRITE_ABORT;
		}
		else if(bps == 16) {
			if(is_unsigned_samples) {
				for(sample = wide_sample = 0; wide_sample < wide_samples; wide_sample++)
					for(channel = 0; channel < channels; channel++, sample++)
						u16buffer[sample] = buffer[channel][wide_sample] + 0x8000;
			}
			else {
				for(sample = wide_sample = 0; wide_sample < wide_samples; wide_sample++)
					for(channel = 0; channel < channels; channel++, sample++)
						s16buffer[sample] = buffer[channel][wide_sample];
			}
			if(is_big_endian != is_big_endian_host) {
				unsigned char tmp;
				const unsigned bytes = sample * 2;
				for(byte = 0; byte < bytes; byte += 2) {
					tmp = u8buffer[byte];
					u8buffer[byte] = u8buffer[byte+1];
					u8buffer[byte+1] = tmp;
				}
			}
			if(fwrite(u16buffer, 2, sample, fout) != sample)
				return FLAC__STREAM_DECODER_WRITE_ABORT;
		}
		else if(bps == 24) {
			if(is_unsigned_samples) {
				for(sample = wide_sample = 0; wide_sample < wide_samples; wide_sample++)
					for(channel = 0; channel < channels; channel++, sample++)
						u32buffer[sample] = buffer[channel][wide_sample] + 0x800000;
			}
			else {
				for(sample = wide_sample = 0; wide_sample < wide_samples; wide_sample++)
					for(channel = 0; channel < channels; channel++, sample++)
						s32buffer[sample] = buffer[channel][wide_sample];
			}
			if(is_big_endian != is_big_endian_host) {
				unsigned char tmp;
				const unsigned bytes = sample * 4;
				for(byte = 0; byte < bytes; byte += 4) {
					tmp = u8buffer[byte];
					u8buffer[byte] = u8buffer[byte+3];
					u8buffer[byte+3] = tmp;
					tmp = u8buffer[byte+1];
					u8buffer[byte+1] = u8buffer[byte+2];
					u8buffer[byte+2] = tmp;
				}
			}
			if(is_big_endian) {
				unsigned lbyte;
				const unsigned bytes = sample * 4;
				for(lbyte = byte = 0; byte < bytes; ) {
					byte++;
					u8buffer[lbyte++] = u8buffer[byte++];
					u8buffer[lbyte++] = u8buffer[byte++];
					u8buffer[lbyte++] = u8buffer[byte++];
				}
			}
			else {
				unsigned lbyte;
				const unsigned bytes = sample * 4;
				for(lbyte = byte = 0; byte < bytes; ) {
					u8buffer[lbyte++] = u8buffer[byte++];
					u8buffer[lbyte++] = u8buffer[byte++];
					u8buffer[lbyte++] = u8buffer[byte++];
					byte++;
				}
			}
			if(fwrite(u8buffer, 3, sample, fout) != sample)
				return FLAC__STREAM_DECODER_WRITE_ABORT;
		}
		else {
			assert(0);
		}
	}
	return FLAC__STREAM_DECODER_WRITE_CONTINUE;
}

void metadata_callback(const FLAC__FileDecoder *decoder, const FLAC__StreamMetaData *metadata, void *client_data)
{
	stream_info_struct *stream_info = (stream_info_struct *)client_data;
	(void)decoder;
	if(metadata->type == FLAC__METADATA_TYPE_STREAMINFO) {
		stream_info->total_samples = metadata->data.stream_info.total_samples - stream_info->skip;
		stream_info->bps = metadata->data.stream_info.bits_per_sample;
		stream_info->channels = metadata->data.stream_info.channels;
		stream_info->sample_rate = metadata->data.stream_info.sample_rate;

		if(stream_info->bps != 8 && stream_info->bps != 16 && stream_info->bps != 24) {
			fprintf(stderr, "ERROR: bits per sample is not 8/16/24\n");
			stream_info->abort_flag = true;
			return;
		}

		/* write the WAVE headers if necessary */
		if(!stream_info->analysis_mode && !stream_info->test_only && stream_info->is_wave_out) {
			uint64 data_size = stream_info->total_samples * stream_info->channels * ((stream_info->bps+7)/8);
			if(data_size >= 0xFFFFFFDC) {
				fprintf(stderr, "ERROR: stream is too big for a wave file\n");
				stream_info->abort_flag = true;
				return;
			}
			if(fwrite("RIFF", 1, 4, stream_info->fout) != 4) stream_info->abort_flag = true;
			if(!write_little_endian_uint32(stream_info->fout, (uint32)(data_size+36))) stream_info->abort_flag = true; /* filesize-8 */
			if(fwrite("WAVEfmt ", 1, 8, stream_info->fout) != 8) stream_info->abort_flag = true;
			if(fwrite("\020\000\000\000", 1, 4, stream_info->fout) != 4) stream_info->abort_flag = true; /* chunk size = 16 */
			if(fwrite("\001\000", 1, 2, stream_info->fout) != 2) stream_info->abort_flag = true; /* compression code == 1 */
			if(!write_little_endian_uint16(stream_info->fout, (uint16)(stream_info->channels))) stream_info->abort_flag = true;
			if(!write_little_endian_uint32(stream_info->fout, stream_info->sample_rate)) stream_info->abort_flag = true;
			if(!write_little_endian_uint32(stream_info->fout, stream_info->sample_rate * stream_info->channels * ((stream_info->bps+7) / 8))) stream_info->abort_flag = true; /* @@@ or is it (sample_rate*channels*bps) / 8 ??? */
			if(!write_little_endian_uint16(stream_info->fout, (uint16)(stream_info->channels * ((stream_info->bps+7) / 8)))) stream_info->abort_flag = true; /* block align */
			if(!write_little_endian_uint16(stream_info->fout, (uint16)(stream_info->bps))) stream_info->abort_flag = true; /* bits per sample */
			if(fwrite("data", 1, 4, stream_info->fout) != 4) stream_info->abort_flag = true;
			if(!write_little_endian_uint32(stream_info->fout, (uint32)data_size)) stream_info->abort_flag = true; /* data size */
		}
	}
}

void error_callback(const FLAC__FileDecoder *decoder, FLAC__StreamDecoderErrorStatus status, void *client_data)
{
	stream_info_struct *stream_info = (stream_info_struct *)client_data;
	(void)decoder;
	fprintf(stderr, "*** Got error code %d\n", status);
	stream_info->abort_flag = true;
}

void print_stats(const stream_info_struct *stream_info)
{
	if(stream_info->verbose) {
		printf("\r%s %u of %u samples, %6.2f%% complete",
			stream_info->test_only? "tested" : stream_info->analysis_mode? "analyzed" : "wrote",
			(unsigned)stream_info->samples_processed,
			(unsigned)stream_info->total_samples,
#ifdef _MSC_VER
			/* with VC++ you have to spoon feed it the casting */
			(double)(int64)stream_info->samples_processed / (double)(int64)stream_info->total_samples * 100.0
#else
			(double)stream_info->samples_processed / (double)stream_info->total_samples * 100.0
#endif
		);
		fflush(stdout);
	}
}
