/* flac - Command-line FLAC encoder/decoder
 * Copyright (C) 2000,2001,2002  Josh Coalson
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
#include <math.h> /* for floor() */
#include <stdio.h> /* for FILE et al. */
#include <string.h> /* for strcmp() */
#include "FLAC/all.h"
#include "decode.h"
#include "file.h"
#ifdef FLAC__HAS_OGG
#include "ogg/ogg.h"
#endif

#ifdef FLAC__HAS_OGG
typedef struct {
	ogg_sync_state oy;
	ogg_stream_state os;
} ogg_info_struct;
#endif

typedef struct {
	const char *inbasefilename;
#ifdef FLAC__HAS_OGG
	FILE *fin;
#endif
	FILE *fout;
	FLAC__bool abort_flag;
	FLAC__bool analysis_mode;
	analysis_options aopts;
	FLAC__bool test_only;
	FLAC__bool is_wave_out;
	FLAC__bool is_big_endian;
	FLAC__bool is_unsigned_samples;
	FLAC__uint64 total_samples;
	unsigned bps;
	unsigned channels;
	unsigned sample_rate;
	FLAC__bool verbose;
	FLAC__bool continue_through_decode_errors;
	struct {
		FLAC__bool needs_fixup;
		unsigned riff_offset;
		unsigned data_offset;
	} wave_chunk_size_fixup;
	FLAC__uint64 skip;
	FLAC__uint64 samples_processed;
	unsigned frame_counter;
#ifdef FLAC__HAS_OGG
	FLAC__bool is_ogg;
#endif
	union {
		FLAC__FileDecoder *file;
		FLAC__StreamDecoder *stream;
	} decoder;
#ifdef FLAC__HAS_OGG
	ogg_info_struct ogg;
#endif
} stream_info_struct;

static FLAC__bool is_big_endian_host;

/* local routines */
static FLAC__bool init(const char *infilename, stream_info_struct *stream_info);
static FLAC__bool write_little_endian_uint16(FILE *f, FLAC__uint16 val);
static FLAC__bool write_little_endian_uint32(FILE *f, FLAC__uint32 val);
static FLAC__bool fixup_wave_chunk_size(const char *outfilename, unsigned riff_offset, unsigned data_offset, FLAC__uint32 data_size);
#ifdef FLAC__HAS_OGG
static FLAC__StreamDecoderReadStatus read_callback(const FLAC__StreamDecoder *decoder, FLAC__byte buffer[], unsigned *bytes, void *client_data);
#endif
/*
 * We use 'void *' so that we can use the same callbacks for the
 * FLAC__StreamDecoder and FLAC__FileDecoder.  The 'decoder' argument is
 * actually never used in the callbacks.
 */
static FLAC__StreamDecoderWriteStatus write_callback(const void *decoder, const FLAC__Frame *frame, const FLAC__int32 *buffer[], void *client_data);
static void metadata_callback(const void *decoder, const FLAC__StreamMetaData *metadata, void *client_data);
static void error_callback(const void *decoder, FLAC__StreamDecoderErrorStatus status, void *client_data);
static void print_stats(const stream_info_struct *stream_info);


int flac__decode_wav(const char *infilename, const char *outfilename, FLAC__bool analysis_mode, analysis_options aopts, wav_decode_options_t options)
{
	FLAC__bool md5_failure = false;
	stream_info_struct stream_info;

	stream_info.abort_flag = false;
	stream_info.analysis_mode = analysis_mode;
	stream_info.aopts = aopts;
	stream_info.test_only = (outfilename == 0);
	stream_info.is_wave_out = true;
	stream_info.verbose = options.common.verbose;
	stream_info.continue_through_decode_errors = options.common.continue_through_decode_errors;
	stream_info.wave_chunk_size_fixup.needs_fixup = false;
	stream_info.skip = options.common.skip;
	stream_info.samples_processed = 0;
	stream_info.frame_counter = 0;
#ifdef FLAC__HAS_OGG
	stream_info.is_ogg = options.common.is_ogg;
#endif
	stream_info.decoder.file = 0; /* this zeroes stream_info.decoder.stream also */
	stream_info.inbasefilename = flac__file_get_basename(infilename);
	stream_info.fout = 0; /* initialized with an open file later if necessary */

	FLAC__ASSERT(!(stream_info.test_only && stream_info.analysis_mode));

	if(!stream_info.test_only) {
		if(0 == strcmp(outfilename, "-")) {
			stream_info.fout = file__get_binary_stdout();
		}
		else {
			if(0 == (stream_info.fout = fopen(outfilename, "wb"))) {
				fprintf(stderr, "%s: ERROR: can't open output file %s\n", stream_info.inbasefilename, outfilename);
				return 1;
			}
		}
	}

#ifdef FLAC__HAS_OGG
	if(stream_info.is_ogg) {
		if (0 == strcmp(infilename, "-")) {
			stream_info.fin = file__get_binary_stdin();
		} else {
			if (0 == (stream_info.fin = fopen(infilename, "rb"))) {
				fprintf(stderr, "%s: ERROR: can't open input file %s\n", stream_info.inbasefilename, infilename);
				if(0 != stream_info.fout && stream_info.fout != stdout)
					fclose(stream_info.fout);
				return 1;
			}
		}
	}
#endif

	if(analysis_mode)
		flac__analyze_init(aopts);

	if(!init(infilename, &stream_info))
		goto wav_abort_;

	if(stream_info.skip > 0) {
#ifdef FLAC__HAS_OGG
		if(stream_info.is_ogg) { /*@@@ (move this check into main.c) */
			fprintf(stderr, "%s: ERROR, can't skip when decoding Ogg-FLAC yet; convert to native-FLAC first\n", stream_info.inbasefilename);
			goto wav_abort_;
		}
#endif
		if(!FLAC__file_decoder_process_metadata(stream_info.decoder.file)) {
			fprintf(stderr, "%s: ERROR while decoding metadata, state=%d:%s\n", stream_info.inbasefilename, FLAC__file_decoder_get_state(stream_info.decoder.file), FLAC__FileDecoderStateString[FLAC__file_decoder_get_state(stream_info.decoder.file)]);
			goto wav_abort_;
		}
		if(stream_info.abort_flag)
			goto wav_abort_;
		if(!FLAC__file_decoder_seek_absolute(stream_info.decoder.file, stream_info.skip)) {
			fprintf(stderr, "%s: ERROR seeking while skipping bytes, state=%d:%s\n", stream_info.inbasefilename, FLAC__file_decoder_get_state(stream_info.decoder.file), FLAC__FileDecoderStateString[FLAC__file_decoder_get_state(stream_info.decoder.file)]);
			goto wav_abort_;
		}
		if(!FLAC__file_decoder_process_remaining_frames(stream_info.decoder.file)) {
			if(stream_info.verbose) fprintf(stderr, "\n");
			fprintf(stderr, "%s: ERROR while decoding frames, state=%d:%s\n", stream_info.inbasefilename, FLAC__file_decoder_get_state(stream_info.decoder.file), FLAC__FileDecoderStateString[FLAC__file_decoder_get_state(stream_info.decoder.file)]);
			goto wav_abort_;
		}
		if(FLAC__file_decoder_get_state(stream_info.decoder.file) != FLAC__FILE_DECODER_OK && FLAC__file_decoder_get_state(stream_info.decoder.file) != FLAC__FILE_DECODER_END_OF_FILE) {
			if(stream_info.verbose) fprintf(stderr, "\n");
			fprintf(stderr, "%s: ERROR during decoding, state=%d:%s\n", stream_info.inbasefilename, FLAC__file_decoder_get_state(stream_info.decoder.file), FLAC__FileDecoderStateString[FLAC__file_decoder_get_state(stream_info.decoder.file)]);
			goto wav_abort_;
		}
	}
	else {
#ifdef FLAC__HAS_OGG
		if(stream_info.is_ogg) {
			if(!FLAC__stream_decoder_process_whole_stream(stream_info.decoder.stream)) {
				if(stream_info.verbose) fprintf(stderr, "\n");
				fprintf(stderr, "%s: ERROR while decoding data, state=%d:%s\n", stream_info.inbasefilename, FLAC__stream_decoder_get_state(stream_info.decoder.stream), FLAC__StreamDecoderStateString[FLAC__stream_decoder_get_state(stream_info.decoder.stream)]);
				goto wav_abort_;
			}
			if(FLAC__stream_decoder_get_state(stream_info.decoder.stream) != FLAC__STREAM_DECODER_SEARCH_FOR_METADATA && FLAC__stream_decoder_get_state(stream_info.decoder.stream) != FLAC__STREAM_DECODER_END_OF_STREAM) {
				if(stream_info.verbose) fprintf(stderr, "\n");
				fprintf(stderr, "%s: ERROR during decoding, state=%d:%s\n", stream_info.inbasefilename, FLAC__stream_decoder_get_state(stream_info.decoder.stream), FLAC__StreamDecoderStateString[FLAC__stream_decoder_get_state(stream_info.decoder.stream)]);
				goto wav_abort_;
			}
		}
		else
#endif
		{
			if(!FLAC__file_decoder_process_whole_file(stream_info.decoder.file)) {
				if(stream_info.verbose) fprintf(stderr, "\n");
				fprintf(stderr, "%s: ERROR while decoding data, state=%d:%s\n", stream_info.inbasefilename, FLAC__file_decoder_get_state(stream_info.decoder.file), FLAC__FileDecoderStateString[FLAC__file_decoder_get_state(stream_info.decoder.file)]);
				goto wav_abort_;
			}
			if(FLAC__file_decoder_get_state(stream_info.decoder.file) != FLAC__FILE_DECODER_OK && FLAC__file_decoder_get_state(stream_info.decoder.file) != FLAC__FILE_DECODER_END_OF_FILE) {
				if(stream_info.verbose) fprintf(stderr, "\n");
				fprintf(stderr, "%s: ERROR during decoding, state=%d:%s\n", stream_info.inbasefilename, FLAC__file_decoder_get_state(stream_info.decoder.file), FLAC__FileDecoderStateString[FLAC__file_decoder_get_state(stream_info.decoder.file)]);
				goto wav_abort_;
			}
		}
	}

#ifdef FLAC__HAS_OGG
	if(stream_info.is_ogg) {
		if(stream_info.decoder.stream) {
			FLAC__stream_decoder_finish(stream_info.decoder.stream);
			md5_failure = false;
			print_stats(&stream_info);
			FLAC__stream_decoder_delete(stream_info.decoder.stream);
		}
	}
	else
#endif
	{
		if(stream_info.decoder.file) {
			md5_failure = !FLAC__file_decoder_finish(stream_info.decoder.file);
			print_stats(&stream_info);
			FLAC__file_decoder_delete(stream_info.decoder.file);
		}
	}
	if(0 != stream_info.fout && stream_info.fout != stdout)
		fclose(stream_info.fout);
#ifdef FLAC__HAS_OGG
	if(stream_info.is_ogg) {
		if(0 != stream_info.fin && stream_info.fin != stdin)
			fclose(stream_info.fin);
	}
#endif
	if(analysis_mode)
		flac__analyze_finish(aopts);
	if(stream_info.wave_chunk_size_fixup.needs_fixup)
		if(!fixup_wave_chunk_size(outfilename, stream_info.wave_chunk_size_fixup.riff_offset, stream_info.wave_chunk_size_fixup.data_offset, (FLAC__uint32)stream_info.samples_processed))
			return 1;
	if(md5_failure) {
		fprintf(stderr, "\r%s: WARNING, MD5 signature mismatch\n", stream_info.inbasefilename);
	}
	else {
		if(stream_info.verbose)
			fprintf(stderr, "\r%s: %s         \n", stream_info.inbasefilename, stream_info.test_only? "ok           ":analysis_mode?"done           ":"done");
	}
	return 0;
wav_abort_:
#ifdef FLAC__HAS_OGG
	if(stream_info.is_ogg) {
		if(stream_info.decoder.stream) {
			FLAC__stream_decoder_finish(stream_info.decoder.stream);
			FLAC__stream_decoder_delete(stream_info.decoder.stream);
		}
	}
	else
#endif
	{
		if(stream_info.decoder.file) {
			FLAC__file_decoder_finish(stream_info.decoder.file);
			FLAC__file_decoder_delete(stream_info.decoder.file);
		}
	}
	if(0 != stream_info.fout && stream_info.fout != stdout) {
		fclose(stream_info.fout);
		unlink(outfilename);
	}
#ifdef FLAC__HAS_OGG
	if(stream_info.is_ogg) {
		if(0 != stream_info.fin && stream_info.fin != stdin)
			fclose(stream_info.fin);
	}
#endif
	if(analysis_mode)
		flac__analyze_finish(aopts);
	return 1;
}

int flac__decode_raw(const char *infilename, const char *outfilename, FLAC__bool analysis_mode, analysis_options aopts, raw_decode_options_t options)
{
	FLAC__bool md5_failure = false;
	stream_info_struct stream_info;

	stream_info.abort_flag = false;
	stream_info.analysis_mode = analysis_mode;
	stream_info.aopts = aopts;
	stream_info.test_only = (outfilename == 0);
	stream_info.is_wave_out = false;
	stream_info.is_big_endian = options.is_big_endian;
	stream_info.is_unsigned_samples = options.is_unsigned_samples;
	stream_info.verbose = options.common.verbose;
	stream_info.continue_through_decode_errors = options.common.continue_through_decode_errors;
	stream_info.wave_chunk_size_fixup.needs_fixup = false;
	stream_info.skip = options.common.skip;
	stream_info.samples_processed = 0;
	stream_info.frame_counter = 0;
#ifdef FLAC__HAS_OGG
	stream_info.is_ogg = options.common.is_ogg;
#endif
	stream_info.decoder.file = 0; /* this zeroes stream_info.decoder.stream also */
	stream_info.inbasefilename = flac__file_get_basename(infilename);
	stream_info.fout = 0; /* initialized with an open file later if necessary */

	FLAC__ASSERT(!(stream_info.test_only && stream_info.analysis_mode));

	if(!stream_info.test_only) {
		if(0 == strcmp(outfilename, "-")) {
			stream_info.fout = file__get_binary_stdout();
		}
		else {
			if(0 == (stream_info.fout = fopen(outfilename, "wb"))) {
				fprintf(stderr, "%s: ERROR: can't open output file %s\n", stream_info.inbasefilename, outfilename);
				return 1;
			}
		}
	}

#ifdef FLAC__HAS_OGG
	if(stream_info.is_ogg) {
		if (0 == strcmp(infilename, "-")) {
			stream_info.fin = file__get_binary_stdin();
		} else {
			if (0 == (stream_info.fin = fopen(infilename, "rb"))) {
				fprintf(stderr, "%s: ERROR: can't open input file %s\n", stream_info.inbasefilename, infilename);
				if(0 != stream_info.fout && stream_info.fout != stdout)
					fclose(stream_info.fout);
				return 1;
			}
		}
	}
#endif

	if(analysis_mode)
		flac__analyze_init(aopts);

	if(!init(infilename, &stream_info))
		goto raw_abort_;

	if(stream_info.skip > 0) {
#ifdef FLAC__HAS_OGG
		if(stream_info.is_ogg) { /*@@@ (move this check into main.c) */
			fprintf(stderr, "%s: ERROR, can't skip when decoding Ogg-FLAC yet; convert to native-FLAC first\n", stream_info.inbasefilename);
			goto raw_abort_;
		}
#endif
		if(!FLAC__file_decoder_process_metadata(stream_info.decoder.file)) {
			fprintf(stderr, "%s: ERROR while decoding metadata, state=%d:%s\n", stream_info.inbasefilename, FLAC__file_decoder_get_state(stream_info.decoder.file), FLAC__FileDecoderStateString[FLAC__file_decoder_get_state(stream_info.decoder.file)]);
			goto raw_abort_;
		}
		if(stream_info.abort_flag)
			goto raw_abort_;
		if(!FLAC__file_decoder_seek_absolute(stream_info.decoder.file, stream_info.skip)) {
			fprintf(stderr, "%s: ERROR seeking while skipping bytes, state=%d:%s\n", stream_info.inbasefilename, FLAC__file_decoder_get_state(stream_info.decoder.file), FLAC__FileDecoderStateString[FLAC__file_decoder_get_state(stream_info.decoder.file)]);
			goto raw_abort_;
		}
		if(!FLAC__file_decoder_process_remaining_frames(stream_info.decoder.file)) {
			if(stream_info.verbose) fprintf(stderr, "\n");
			fprintf(stderr, "%s: ERROR while decoding frames, state=%d:%s\n", stream_info.inbasefilename, FLAC__file_decoder_get_state(stream_info.decoder.file), FLAC__FileDecoderStateString[FLAC__file_decoder_get_state(stream_info.decoder.file)]);
			goto raw_abort_;
		}
		if(FLAC__file_decoder_get_state(stream_info.decoder.file) != FLAC__FILE_DECODER_OK && FLAC__file_decoder_get_state(stream_info.decoder.file) != FLAC__FILE_DECODER_END_OF_FILE) {
			if(stream_info.verbose) fprintf(stderr, "\n");
			fprintf(stderr, "%s: ERROR during decoding, state=%d:%s\n", stream_info.inbasefilename, FLAC__file_decoder_get_state(stream_info.decoder.file), FLAC__FileDecoderStateString[FLAC__file_decoder_get_state(stream_info.decoder.file)]);
			goto raw_abort_;
		}
	}
	else {
#ifdef FLAC__HAS_OGG
		if(stream_info.is_ogg) {
			if(!FLAC__stream_decoder_process_whole_stream(stream_info.decoder.stream)) {
				if(stream_info.verbose) fprintf(stderr, "\n");
				fprintf(stderr, "%s: ERROR while decoding data, state=%d:%s\n", stream_info.inbasefilename, FLAC__stream_decoder_get_state(stream_info.decoder.stream), FLAC__StreamDecoderStateString[FLAC__stream_decoder_get_state(stream_info.decoder.stream)]);
				goto raw_abort_;
			}
			if(FLAC__stream_decoder_get_state(stream_info.decoder.stream) != FLAC__STREAM_DECODER_SEARCH_FOR_METADATA && FLAC__stream_decoder_get_state(stream_info.decoder.stream) != FLAC__STREAM_DECODER_END_OF_STREAM) {
				if(stream_info.verbose) fprintf(stderr, "\n");
				fprintf(stderr, "%s: ERROR during decoding, state=%d:%s\n", stream_info.inbasefilename, FLAC__stream_decoder_get_state(stream_info.decoder.stream), FLAC__StreamDecoderStateString[FLAC__stream_decoder_get_state(stream_info.decoder.stream)]);
				goto raw_abort_;
			}
		}
		else
#endif
		{
			if(!FLAC__file_decoder_process_whole_file(stream_info.decoder.file)) {
				if(stream_info.verbose) fprintf(stderr, "\n");
				fprintf(stderr, "%s: ERROR while decoding data, state=%d:%s\n", stream_info.inbasefilename, FLAC__file_decoder_get_state(stream_info.decoder.file), FLAC__FileDecoderStateString[FLAC__file_decoder_get_state(stream_info.decoder.file)]);
				goto raw_abort_;
			}
			if(FLAC__file_decoder_get_state(stream_info.decoder.file) != FLAC__FILE_DECODER_OK && FLAC__file_decoder_get_state(stream_info.decoder.file) != FLAC__FILE_DECODER_END_OF_FILE) {
				if(stream_info.verbose) fprintf(stderr, "\n");
				fprintf(stderr, "%s: ERROR during decoding, state=%d:%s\n", stream_info.inbasefilename, FLAC__file_decoder_get_state(stream_info.decoder.file), FLAC__FileDecoderStateString[FLAC__file_decoder_get_state(stream_info.decoder.file)]);
				goto raw_abort_;
			}
		}
	}

#ifdef FLAC__HAS_OGG
	if(stream_info.is_ogg) {
		if(stream_info.decoder.stream) {
			FLAC__stream_decoder_finish(stream_info.decoder.stream);
			md5_failure = false;
			print_stats(&stream_info);
			FLAC__stream_decoder_delete(stream_info.decoder.stream);
		}
	}
	else
#endif
	{
		if(stream_info.decoder.file) {
			md5_failure = !FLAC__file_decoder_finish(stream_info.decoder.file);
			print_stats(&stream_info);
			FLAC__file_decoder_delete(stream_info.decoder.file);
		}
	}
	if(0 != stream_info.fout && stream_info.fout != stdout)
		fclose(stream_info.fout);
#ifdef FLAC__HAS_OGG
	if(stream_info.is_ogg) {
		if(0 != stream_info.fin && stream_info.fin != stdin)
			fclose(stream_info.fin);
	}
#endif
	if(analysis_mode)
		flac__analyze_finish(aopts);
	if(md5_failure) {
		fprintf(stderr, "\r%s: WARNING, MD5 signature mismatch\n", stream_info.inbasefilename);
	}
	else {
		if(stream_info.verbose)
			fprintf(stderr, "\r%s: %s         \n", stream_info.inbasefilename, stream_info.test_only? "ok           ":analysis_mode?"done           ":"done");
	}
	return 0;
raw_abort_:
#ifdef FLAC__HAS_OGG
	if(stream_info.is_ogg) {
		if(stream_info.decoder.stream) {
			FLAC__stream_decoder_finish(stream_info.decoder.stream);
			FLAC__stream_decoder_delete(stream_info.decoder.stream);
		}
	}
	else
#endif
	{
		if(stream_info.decoder.file) {
			FLAC__file_decoder_finish(stream_info.decoder.file);
			FLAC__file_decoder_delete(stream_info.decoder.file);
		}
	}
	if(0 != stream_info.fout && stream_info.fout != stdout) {
		fclose(stream_info.fout);
		unlink(outfilename);
	}
#ifdef FLAC__HAS_OGG
	if(stream_info.is_ogg) {
		if(0 != stream_info.fin && stream_info.fin != stdin)
			fclose(stream_info.fin);
	}
#endif
	if(analysis_mode)
		flac__analyze_finish(aopts);
	return 1;
}

FLAC__bool init(const char *infilename, stream_info_struct *stream_info)
{
	FLAC__uint32 test = 1;

	is_big_endian_host = (*((FLAC__byte*)(&test)))? false : true;

#ifdef FLAC__HAS_OGG
	if(stream_info->is_ogg) {
		stream_info->decoder.stream = FLAC__stream_decoder_new();

		if(0 == stream_info->decoder.stream) {
			fprintf(stderr, "%s: ERROR creating the decoder instance\n", stream_info->inbasefilename);
			return false;
		}

		FLAC__stream_decoder_set_read_callback(stream_info->decoder.stream, read_callback);
		/*
		 * The three ugly casts here are to 'downcast' the 'void *' argument of
		 * the callback down to 'FLAC__StreamDecoder *'.  In C++ this would be
		 * unnecessary but here the cast makes the C compiler happy.
		 */
		FLAC__stream_decoder_set_write_callback(stream_info->decoder.stream, (FLAC__StreamDecoderWriteStatus (*)(const FLAC__StreamDecoder *, const FLAC__Frame *, const FLAC__int32 *[], void *))write_callback);
		FLAC__stream_decoder_set_metadata_callback(stream_info->decoder.stream, (void (*)(const FLAC__StreamDecoder *, const FLAC__StreamMetaData *, void *))metadata_callback);
		FLAC__stream_decoder_set_error_callback(stream_info->decoder.stream, (void (*)(const FLAC__StreamDecoder *, FLAC__StreamDecoderErrorStatus, void *))error_callback);
		FLAC__stream_decoder_set_client_data(stream_info->decoder.stream, stream_info);

		if(FLAC__stream_decoder_init(stream_info->decoder.stream) != FLAC__STREAM_DECODER_SEARCH_FOR_METADATA) {
			fprintf(stderr, "%s: ERROR initializing decoder, state=%d:%s\n", stream_info->inbasefilename, FLAC__stream_decoder_get_state(stream_info->decoder.stream), FLAC__StreamDecoderStateString[FLAC__stream_decoder_get_state(stream_info->decoder.stream)]);
			return false;
		}

		ogg_stream_init(&stream_info->ogg.os, 0);
		ogg_sync_init(&stream_info->ogg.oy);
	}
	else
#endif
	{
		stream_info->decoder.file = FLAC__file_decoder_new();

		if(0 == stream_info->decoder.file) {
			fprintf(stderr, "%s: ERROR creating the decoder instance\n", stream_info->inbasefilename);
			return false;
		}

		FLAC__file_decoder_set_md5_checking(stream_info->decoder.file, true);
		FLAC__file_decoder_set_filename(stream_info->decoder.file, infilename);
		/*
		 * The three ugly casts here are to 'downcast' the 'void *' argument of
		 * the callback down to 'FLAC__FileDecoder *'.  In C++ this would be
		 * unnecessary but here the cast makes the C compiler happy.
		 */
		FLAC__file_decoder_set_write_callback(stream_info->decoder.file, (FLAC__StreamDecoderWriteStatus (*)(const FLAC__FileDecoder *, const FLAC__Frame *, const FLAC__int32 *[], void *))write_callback);
		FLAC__file_decoder_set_metadata_callback(stream_info->decoder.file, (void (*)(const FLAC__FileDecoder *, const FLAC__StreamMetaData *, void *))metadata_callback);
		FLAC__file_decoder_set_error_callback(stream_info->decoder.file, (void (*)(const FLAC__FileDecoder *, FLAC__StreamDecoderErrorStatus, void *))error_callback);
		FLAC__file_decoder_set_client_data(stream_info->decoder.file, stream_info);

		if(FLAC__file_decoder_init(stream_info->decoder.file) != FLAC__FILE_DECODER_OK) {
			fprintf(stderr, "%s: ERROR initializing decoder, state=%d:%s\n", stream_info->inbasefilename, FLAC__file_decoder_get_state(stream_info->decoder.file), FLAC__FileDecoderStateString[FLAC__file_decoder_get_state(stream_info->decoder.file)]);
			return false;
		}
	}

	return true;
}

FLAC__bool write_little_endian_uint16(FILE *f, FLAC__uint16 val)
{
	FLAC__byte *b = (FLAC__byte*)(&val);
	if(is_big_endian_host) {
		FLAC__byte tmp;
		tmp = b[1]; b[1] = b[0]; b[0] = tmp;
	}
	return fwrite(b, 1, 2, f) == 2;
}

FLAC__bool write_little_endian_uint32(FILE *f, FLAC__uint32 val)
{
	FLAC__byte *b = (FLAC__byte*)(&val);
	if(is_big_endian_host) {
		FLAC__byte tmp;
		tmp = b[3]; b[3] = b[0]; b[0] = tmp;
		tmp = b[2]; b[2] = b[1]; b[1] = tmp;
	}
	return fwrite(b, 1, 4, f) == 4;
}

FLAC__bool fixup_wave_chunk_size(const char *outfilename, unsigned riff_offset, unsigned data_offset, FLAC__uint32 data_size)
{
	FILE *f = fopen(outfilename, "r+b");
	if(0 == f) {
		fprintf(stderr, "ERROR, couldn't open file %s while fixing up WAVE chunk size\n", outfilename);
		return false;
	}
	if(fseek(f, riff_offset, SEEK_SET) < 0) {
		fprintf(stderr, "ERROR, couldn't seek in file %s while fixing up WAVE chunk size\n", outfilename);
		fclose(f);
		return false;
	}
	if(!write_little_endian_uint32(f, data_size + 36)) {
		fprintf(stderr, "ERROR, couldn't write size in file %s while fixing up WAVE chunk size\n", outfilename);
		fclose(f);
		return false;
	}
	if(fseek(f, data_offset, SEEK_SET) < 0) {
		fprintf(stderr, "ERROR, couldn't seek in file %s while fixing up WAVE chunk size\n", outfilename);
		fclose(f);
		return false;
	}
	if(!write_little_endian_uint32(f, data_size)) {
		fprintf(stderr, "ERROR, couldn't write size in file %s while fixing up WAVE chunk size\n", outfilename);
		fclose(f);
		return false;
	}
	fclose(f);
	return true;
}

#ifdef FLAC__HAS_OGG
#define OGG_READ_BUFFER_SIZE 4096
FLAC__StreamDecoderReadStatus read_callback(const FLAC__StreamDecoder *decoder, FLAC__byte buffer[], unsigned *bytes, void *client_data)
{
	stream_info_struct *stream_info = (stream_info_struct *)client_data;
	FILE *fin = stream_info->fin;
	size_t bytes_read;
	ogg_page og;
	char *oggbuf;
	unsigned int offset = 0;

	*bytes = 0;

	if (stream_info->abort_flag)
		return FLAC__STREAM_DECODER_READ_STATUS_ABORT;

	oggbuf = ogg_sync_buffer(&stream_info->ogg.oy, OGG_READ_BUFFER_SIZE);

	(void)decoder; /* avoid compiler warning */

	if(feof(fin))
		return FLAC__STREAM_DECODER_READ_STATUS_END_OF_STREAM;

	bytes_read = fread(oggbuf, 1, OGG_READ_BUFFER_SIZE, fin);

	if(ferror(fin))
		return FLAC__STREAM_DECODER_READ_STATUS_ABORT;

	if(ogg_sync_wrote(&stream_info->ogg.oy, bytes_read) < 0)
		return FLAC__STREAM_DECODER_READ_STATUS_ABORT;

	while(ogg_sync_pageout(&stream_info->ogg.oy, &og) == 1) {
		if(ogg_stream_pagein(&stream_info->ogg.os, &og) == 0) {
			ogg_packet op;

			while(ogg_stream_packetout(&stream_info->ogg.os, &op) == 1) {
				memcpy(buffer + offset, op.packet, op.bytes);
				*bytes += op.bytes;
				offset += op.bytes;
			}
		} else {
			return FLAC__STREAM_DECODER_READ_STATUS_ABORT;
		}
	}

	return FLAC__STREAM_DECODER_READ_STATUS_CONTINUE;
}
#endif

FLAC__StreamDecoderWriteStatus write_callback(const void *decoder, const FLAC__Frame *frame, const FLAC__int32 *buffer[], void *client_data)
{
	stream_info_struct *stream_info = (stream_info_struct *)client_data;
	FILE *fout = stream_info->fout;
	unsigned bps = stream_info->bps, channels = stream_info->channels;
	FLAC__bool is_big_endian = (stream_info->is_wave_out? false : stream_info->is_big_endian);
	FLAC__bool is_unsigned_samples = (stream_info->is_wave_out? bps<=8 : stream_info->is_unsigned_samples);
	unsigned wide_samples = frame->header.blocksize, wide_sample, sample, channel, byte;
	static FLAC__int8 s8buffer[FLAC__MAX_BLOCK_SIZE * FLAC__MAX_CHANNELS * sizeof(FLAC__int32)]; /* WATCHOUT: can be up to 2 megs */
	FLAC__uint8  *u8buffer  = (FLAC__uint8  *)s8buffer;
	FLAC__int16  *s16buffer = (FLAC__int16  *)s8buffer;
	FLAC__uint16 *u16buffer = (FLAC__uint16 *)s8buffer;
	FLAC__int32  *s32buffer = (FLAC__int32  *)s8buffer;
	FLAC__uint32 *u32buffer = (FLAC__uint32 *)s8buffer;

	(void)decoder;

	if(stream_info->abort_flag)
		return FLAC__STREAM_DECODER_WRITE_STATUS_ABORT;

	stream_info->samples_processed += wide_samples;
	stream_info->frame_counter++;

	if(stream_info->verbose && !(stream_info->frame_counter & 0x7f))
		print_stats(stream_info);

	if(stream_info->analysis_mode) {
		flac__analyze_frame(frame, stream_info->frame_counter-1, stream_info->aopts, fout);
	}
	else if(!stream_info->test_only) {
		if(bps == 8) {
			if(is_unsigned_samples) {
				for(sample = wide_sample = 0; wide_sample < wide_samples; wide_sample++)
					for(channel = 0; channel < channels; channel++, sample++)
						u8buffer[sample] = (FLAC__uint8)(buffer[channel][wide_sample] + 0x80);
			}
			else {
				for(sample = wide_sample = 0; wide_sample < wide_samples; wide_sample++)
					for(channel = 0; channel < channels; channel++, sample++)
						s8buffer[sample] = (FLAC__int8)(buffer[channel][wide_sample]);
			}
			if(fwrite(u8buffer, 1, sample, fout) != sample)
				return FLAC__STREAM_DECODER_WRITE_STATUS_ABORT;
		}
		else if(bps == 16) {
			if(is_unsigned_samples) {
				for(sample = wide_sample = 0; wide_sample < wide_samples; wide_sample++)
					for(channel = 0; channel < channels; channel++, sample++)
						u16buffer[sample] = (FLAC__uint16)(buffer[channel][wide_sample] + 0x8000);
			}
			else {
				for(sample = wide_sample = 0; wide_sample < wide_samples; wide_sample++)
					for(channel = 0; channel < channels; channel++, sample++)
						s16buffer[sample] = (FLAC__int16)(buffer[channel][wide_sample]);
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
				return FLAC__STREAM_DECODER_WRITE_STATUS_ABORT;
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
				return FLAC__STREAM_DECODER_WRITE_STATUS_ABORT;
		}
		else {
			FLAC__ASSERT(0);
		}
	}
	return FLAC__STREAM_DECODER_WRITE_STATUS_CONTINUE;
}

void metadata_callback(const void *decoder, const FLAC__StreamMetaData *metadata, void *client_data)
{
	stream_info_struct *stream_info = (stream_info_struct *)client_data;
	(void)decoder;
	if(metadata->type == FLAC__METADATA_TYPE_STREAMINFO) {
		/* remember, metadata->data.stream_info.total_samples can be 0, meaning 'unknown' */
		if(metadata->data.stream_info.total_samples > 0 && stream_info->skip >= metadata->data.stream_info.total_samples) {
			fprintf(stderr, "%s: ERROR trying to skip more samples than in stream\n", stream_info->inbasefilename);
			stream_info->abort_flag = true;
			return;
		}
		else if(metadata->data.stream_info.total_samples == 0 && stream_info->skip > 0) {
			fprintf(stderr, "%s: ERROR, can't skip when FLAC metadata has total sample count of 0\n", stream_info->inbasefilename);
			stream_info->abort_flag = true;
			return;
		}
		else
			stream_info->total_samples = metadata->data.stream_info.total_samples - stream_info->skip;
		stream_info->bps = metadata->data.stream_info.bits_per_sample;
		stream_info->channels = metadata->data.stream_info.channels;
		stream_info->sample_rate = metadata->data.stream_info.sample_rate;

		if(stream_info->bps != 8 && stream_info->bps != 16 && stream_info->bps != 24) {
			fprintf(stderr, "%s: ERROR: bits per sample is not 8/16/24\n", stream_info->inbasefilename);
			stream_info->abort_flag = true;
			return;
		}

		/* write the WAVE headers if necessary */
		if(!stream_info->analysis_mode && !stream_info->test_only && stream_info->is_wave_out) {
			FLAC__uint64 data_size = stream_info->total_samples * stream_info->channels * ((stream_info->bps+7)/8);
			if(stream_info->total_samples == 0) {
				if(stream_info->fout == stdout) {
					fprintf(stderr, "%s: WARNING, don't have accurate sample count available for WAVE header.\n", stream_info->inbasefilename);
					fprintf(stderr, "             Generated WAVE file will have a data chunk size of 0.  Try\n");
					fprintf(stderr, "             decoding directly to a file instead.\n");
				}
				else {
					stream_info->wave_chunk_size_fixup.needs_fixup = true;
				}
			}
			if(data_size >= 0xFFFFFFDC) {
				fprintf(stderr, "%s: ERROR: stream is too big to fit in a single WAVE file chunk\n", stream_info->inbasefilename);
				stream_info->abort_flag = true;
				return;
			}
			if(fwrite("RIFF", 1, 4, stream_info->fout) != 4) stream_info->abort_flag = true;
			if(stream_info->wave_chunk_size_fixup.needs_fixup)
				stream_info->wave_chunk_size_fixup.riff_offset = ftell(stream_info->fout);
			if(!write_little_endian_uint32(stream_info->fout, (FLAC__uint32)(data_size+36))) stream_info->abort_flag = true; /* filesize-8 */
			if(fwrite("WAVEfmt ", 1, 8, stream_info->fout) != 8) stream_info->abort_flag = true;
			if(fwrite("\020\000\000\000", 1, 4, stream_info->fout) != 4) stream_info->abort_flag = true; /* chunk size = 16 */
			if(fwrite("\001\000", 1, 2, stream_info->fout) != 2) stream_info->abort_flag = true; /* compression code == 1 */
			if(!write_little_endian_uint16(stream_info->fout, (FLAC__uint16)(stream_info->channels))) stream_info->abort_flag = true;
			if(!write_little_endian_uint32(stream_info->fout, stream_info->sample_rate)) stream_info->abort_flag = true;
			if(!write_little_endian_uint32(stream_info->fout, stream_info->sample_rate * stream_info->channels * ((stream_info->bps+7) / 8))) stream_info->abort_flag = true; /* @@@ or is it (sample_rate*channels*bps) / 8 ??? */
			if(!write_little_endian_uint16(stream_info->fout, (FLAC__uint16)(stream_info->channels * ((stream_info->bps+7) / 8)))) stream_info->abort_flag = true; /* block align */
			if(!write_little_endian_uint16(stream_info->fout, (FLAC__uint16)(stream_info->bps))) stream_info->abort_flag = true; /* bits per sample */
			if(fwrite("data", 1, 4, stream_info->fout) != 4) stream_info->abort_flag = true;
			if(stream_info->wave_chunk_size_fixup.needs_fixup)
				stream_info->wave_chunk_size_fixup.data_offset = ftell(stream_info->fout);
			if(!write_little_endian_uint32(stream_info->fout, (FLAC__uint32)data_size)) stream_info->abort_flag = true; /* data size */
		}
	}
}

void error_callback(const void *decoder, FLAC__StreamDecoderErrorStatus status, void *client_data)
{
	stream_info_struct *stream_info = (stream_info_struct *)client_data;
	(void)decoder;
	fprintf(stderr, "%s: *** Got error code %d:%s\n", stream_info->inbasefilename, status, FLAC__StreamDecoderErrorStatusString[status]);
	if(!stream_info->continue_through_decode_errors)
		stream_info->abort_flag = true;
}

void print_stats(const stream_info_struct *stream_info)
{
	if(stream_info->verbose) {
#if defined _MSC_VER || defined __MINGW32__
		/* with VC++ you have to spoon feed it the casting */
		const double progress = (double)(FLAC__int64)stream_info->samples_processed / (double)(FLAC__int64)stream_info->total_samples * 100.0;
#else
		const double progress = (double)stream_info->samples_processed / (double)stream_info->total_samples * 100.0;
#endif
		if(stream_info->total_samples > 0) {
			fprintf(stderr, "\r%s: %s%u%% complete",
				stream_info->inbasefilename,
				stream_info->test_only? "testing, " : stream_info->analysis_mode? "analyzing, " : "",
				(unsigned)floor(progress + 0.5)
			);
		}
		else {
			fprintf(stderr, "\r%s: %s %u samples",
				stream_info->inbasefilename,
				stream_info->test_only? "tested" : stream_info->analysis_mode? "analyzed" : "wrote",
				(unsigned)stream_info->samples_processed
			);
		}
	}
}
#if 0
void metadata_callback(const FLAC__StreamEncoder *encoder, const FLAC__StreamMetaData *metadata, void *client_data)
{
	encoder_wrapper_struct *encoder_wrapper = (encoder_wrapper_struct *)client_data;
	FLAC__byte b;
	FILE *f = encoder_wrapper->fout;
	const FLAC__uint64 samples = metadata->data.stream_info.total_samples;
	const unsigned min_framesize = metadata->data.stream_info.min_framesize;
	const unsigned max_framesize = metadata->data.stream_info.max_framesize;

	FLAC__ASSERT(metadata->type == FLAC__METADATA_TYPE_STREAMINFO);

	/*
	 * If we are writing to an ogg stream, there is no need to go back
	 * and update the STREAMINFO or SEEKTABLE blocks; the values we would
	 * update are not necessary with Ogg as the transport.  We can't do
	 * it reliably anyway without knowing the Ogg structure.
	 */
#ifdef FLAC__HAS_OGG
	if(encoder_wrapper->use_ogg)
		return;
#endif

	/*
	 * we get called by the encoder when the encoding process has
	 * finished so that we can update the STREAMINFO and SEEKTABLE
	 * blocks.
	 */

	(void)encoder; /* silence compiler warning about unused parameter */

	if(f != stdout) {
		fclose(encoder_wrapper->fout);
		if(0 == (f = fopen(encoder_wrapper->outfilename, "r+b")))
			return;
	}

	/* all this is based on intimate knowledge of the stream header
	 * layout, but a change to the header format that would break this
	 * would also break all streams encoded in the previous format.
	 */

	if(-1 == fseek(f, 26, SEEK_SET)) goto end_;
	fwrite(metadata->data.stream_info.md5sum, 1, 16, f);

samples_:
	/* if we get this far we know we can seek so no need to check the
	 * return value from fseek()
	 */
	fseek(f, 21, SEEK_SET);
	if(fread(&b, 1, 1, f) != 1) goto framesize_;
	fseek(f, 21, SEEK_SET);
	b = (b & 0xf0) | (FLAC__byte)((samples >> 32) & 0x0F);
	if(fwrite(&b, 1, 1, f) != 1) goto framesize_;
	b = (FLAC__byte)((samples >> 24) & 0xFF);
	if(fwrite(&b, 1, 1, f) != 1) goto framesize_;
	b = (FLAC__byte)((samples >> 16) & 0xFF);
	if(fwrite(&b, 1, 1, f) != 1) goto framesize_;
	b = (FLAC__byte)((samples >> 8) & 0xFF);
	if(fwrite(&b, 1, 1, f) != 1) goto framesize_;
	b = (FLAC__byte)(samples & 0xFF);
	if(fwrite(&b, 1, 1, f) != 1) goto framesize_;

framesize_:
	fseek(f, 12, SEEK_SET);
	b = (FLAC__byte)((min_framesize >> 16) & 0xFF);
	if(fwrite(&b, 1, 1, f) != 1) goto seektable_;
	b = (FLAC__byte)((min_framesize >> 8) & 0xFF);
	if(fwrite(&b, 1, 1, f) != 1) goto seektable_;
	b = (FLAC__byte)(min_framesize & 0xFF);
	if(fwrite(&b, 1, 1, f) != 1) goto seektable_;
	b = (FLAC__byte)((max_framesize >> 16) & 0xFF);
	if(fwrite(&b, 1, 1, f) != 1) goto seektable_;
	b = (FLAC__byte)((max_framesize >> 8) & 0xFF);
	if(fwrite(&b, 1, 1, f) != 1) goto seektable_;
	b = (FLAC__byte)(max_framesize & 0xFF);
	if(fwrite(&b, 1, 1, f) != 1) goto seektable_;

seektable_:
	if(encoder_wrapper->seek_table.num_points > 0) {
		long pos;
		unsigned i;

		/* convert any unused seek points to placeholders */
		for(i = 0; i < encoder_wrapper->seek_table.num_points; i++) {
			if(encoder_wrapper->seek_table.points[i].sample_number == FLAC__STREAM_METADATA_SEEKPOINT_PLACEHOLDER)
				break;
			else if(encoder_wrapper->seek_table.points[i].frame_samples == 0)
				encoder_wrapper->seek_table.points[i].sample_number = FLAC__STREAM_METADATA_SEEKPOINT_PLACEHOLDER;
		}

		/* the offset of the seek table data 'pos' should be after then stream sync and STREAMINFO block and SEEKTABLE header */
		pos = (FLAC__STREAM_SYNC_LEN + FLAC__STREAM_METADATA_IS_LAST_LEN + FLAC__STREAM_METADATA_TYPE_LEN + FLAC__STREAM_METADATA_LENGTH_LEN) / 8;
		pos += metadata->length;
		pos += (FLAC__STREAM_METADATA_IS_LAST_LEN + FLAC__STREAM_METADATA_TYPE_LEN + FLAC__STREAM_METADATA_LENGTH_LEN) / 8;
		fseek(f, pos, SEEK_SET);
		for(i = 0; i < encoder_wrapper->seek_table.num_points; i++) {
			if(!write_big_endian_uint64(f, encoder_wrapper->seek_table.points[i].sample_number)) goto end_;
			if(!write_big_endian_uint64(f, encoder_wrapper->seek_table.points[i].stream_offset)) goto end_;
			if(!write_big_endian_uint16(f, (FLAC__uint16)encoder_wrapper->seek_table.points[i].frame_samples)) goto end_;
		}
	}

end_:
	fclose(f);
	return;
}
#endif
