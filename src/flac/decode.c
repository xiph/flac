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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

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
#include "share/grabbag.h"
#include "decode.h"

#ifdef FLAC__HAS_OGG
#include "OggFLAC/stream_decoder.h"
#endif

typedef struct {
#ifdef FLAC__HAS_OGG
	FLAC__bool is_ogg;
#endif

	FLAC__bool verbose;
	FLAC__bool is_wave_out;
	FLAC__bool continue_through_decode_errors;
	FLAC__bool test_only;
	FLAC__bool analysis_mode;
	analysis_options aopts;
	utils__SkipUntilSpecification *skip_specification;

	const char *inbasefilename;
	const char *outfilename;

	FLAC__uint64 samples_processed;
	unsigned frame_counter;
	FLAC__bool abort_flag;

	struct {
		FLAC__bool needs_fixup;
		unsigned riff_offset;
		unsigned data_offset;
	} wave_chunk_size_fixup;

	FLAC__bool is_big_endian;
	FLAC__bool is_unsigned_samples;
	FLAC__uint64 total_samples;
	unsigned bps;
	unsigned channels;
	unsigned sample_rate;

	union {
		union {
			FLAC__FileDecoder *file;
		} flac;
#ifdef FLAC__HAS_OGG
		union {
			OggFLAC__StreamDecoder *stream;
		} ogg;
#endif
	} decoder;

#ifdef FLAC__HAS_OGG
	FILE *fin;
#endif
	FILE *fout;
} DecoderSession;


static FLAC__bool is_big_endian_host_;


/*
 * local routines
 */
static FLAC__bool DecoderSession_construct(DecoderSession *d, FLAC__bool is_ogg, FLAC__bool verbose, FLAC__bool is_wave_out, FLAC__bool continue_through_decode_errors, FLAC__bool analysis_mode, analysis_options aopts, utils__SkipUntilSpecification *skip_specification, const char *infilename, const char *outfilename);
static void DecoderSession_destroy(DecoderSession *d, FLAC__bool error_occurred);
static FLAC__bool DecoderSession_init_decoder(DecoderSession *d, decode_options_t decode_options, const char *infilename);
static FLAC__bool DecoderSession_process(DecoderSession *d);
static int DecoderSession_finish_ok(DecoderSession *d);
static int DecoderSession_finish_error(DecoderSession *d);
static FLAC__bool write_little_endian_uint16(FILE *f, FLAC__uint16 val);
static FLAC__bool write_little_endian_uint32(FILE *f, FLAC__uint32 val);
static FLAC__bool fixup_wave_chunk_size(const char *outfilename, unsigned riff_offset, unsigned data_offset, FLAC__uint32 data_size);
#ifdef FLAC__HAS_OGG
static FLAC__StreamDecoderReadStatus read_callback(const OggFLAC__StreamDecoder *decoder, FLAC__byte buffer[], unsigned *bytes, void *client_data);
#endif
/*
 * We use 'void *' so that we can use the same callbacks for the
 * FLAC__StreamDecoder and FLAC__FileDecoder.  The 'decoder' argument is
 * actually never used in the callbacks.
 */
static FLAC__StreamDecoderWriteStatus write_callback(const void *decoder, const FLAC__Frame *frame, const FLAC__int32 * const buffer[], void *client_data);
static void metadata_callback(const void *decoder, const FLAC__StreamMetadata *metadata, void *client_data);
static void error_callback(const void *decoder, FLAC__StreamDecoderErrorStatus status, void *client_data);
static void print_error_with_state(const DecoderSession *d, const char *message);
static void print_stats(const DecoderSession *decoder_session);


/*
 * public routines
 */
int flac__decode_wav(const char *infilename, const char *outfilename, FLAC__bool analysis_mode, analysis_options aopts, wav_decode_options_t options)
{
	DecoderSession decoder_session;

	if(!
		DecoderSession_construct(
			&decoder_session,
#ifdef FLAC__HAS_OGG
			options.common.is_ogg,
#else
			/*is_ogg=*/false,
#endif
			options.common.verbose,
			/*is_wave_out=*/true,
			options.common.continue_through_decode_errors,
			analysis_mode,
			aopts,
			&options.common.skip_specification,
			infilename,
			outfilename
		)
	)
		return 1;

	if(!DecoderSession_init_decoder(&decoder_session, options.common, infilename))
		return DecoderSession_finish_error(&decoder_session);

	if(!DecoderSession_process(&decoder_session))
		return DecoderSession_finish_error(&decoder_session);

	return DecoderSession_finish_ok(&decoder_session);
}

int flac__decode_raw(const char *infilename, const char *outfilename, FLAC__bool analysis_mode, analysis_options aopts, raw_decode_options_t options)
{
	DecoderSession decoder_session;

	decoder_session.is_big_endian = options.is_big_endian;
	decoder_session.is_unsigned_samples = options.is_unsigned_samples;

	if(!
		DecoderSession_construct(
			&decoder_session,
#ifdef FLAC__HAS_OGG
			options.common.is_ogg,
#else
			/*is_ogg=*/false,
#endif
			options.common.verbose,
			/*is_wave_out=*/false,
			options.common.continue_through_decode_errors,
			analysis_mode,
			aopts,
			&options.common.skip_specification,
			infilename,
			outfilename
		)
	)
		return 1;

	if(!DecoderSession_init_decoder(&decoder_session, options.common, infilename))
		return DecoderSession_finish_error(&decoder_session);

	if(!DecoderSession_process(&decoder_session))
		return DecoderSession_finish_error(&decoder_session);

	return DecoderSession_finish_ok(&decoder_session);
}

FLAC__bool DecoderSession_construct(DecoderSession *d, FLAC__bool is_ogg, FLAC__bool verbose, FLAC__bool is_wave_out, FLAC__bool continue_through_decode_errors, FLAC__bool analysis_mode, analysis_options aopts, utils__SkipUntilSpecification *skip_specification, const char *infilename, const char *outfilename)
{
#ifdef FLAC__HAS_OGG
	d->is_ogg = is_ogg;
#else
	(void)is_ogg;
#endif

	d->verbose = verbose;
	d->is_wave_out = is_wave_out;
	d->continue_through_decode_errors = continue_through_decode_errors;
	d->test_only = (0 == outfilename);
	d->analysis_mode = analysis_mode;
	d->aopts = aopts;
	d->skip_specification = skip_specification;

	d->inbasefilename = grabbag__file_get_basename(infilename);
	d->outfilename = outfilename;

	d->samples_processed = 0;
	d->frame_counter = 0;
	d->abort_flag = false;

	d->wave_chunk_size_fixup.needs_fixup = false;

	d->decoder.flac.file = 0;
#ifdef FLAC__HAS_OGG
	d->decoder.ogg.stream = 0;
#endif

#ifdef FLAC__HAS_OGG
	d->fin = 0;
#endif
	d->fout = 0; /* initialized with an open file later if necessary */

	FLAC__ASSERT(!(d->test_only && d->analysis_mode));

	if(!d->test_only) {
		if(0 == strcmp(outfilename, "-")) {
			d->fout = grabbag__file_get_binary_stdout();
		}
		else {
			if(0 == (d->fout = fopen(outfilename, "wb"))) {
				fprintf(stderr, "%s: ERROR: can't open output file %s\n", d->inbasefilename, outfilename);
				DecoderSession_destroy(d, /*error_occurred=*/true);
				return false;
			}
		}
	}

#ifdef FLAC__HAS_OGG
	if(d->is_ogg) {
		if (0 == strcmp(infilename, "-")) {
			d->fin = grabbag__file_get_binary_stdin();
		} else {
			if (0 == (d->fin = fopen(infilename, "rb"))) {
				fprintf(stderr, "%s: ERROR: can't open input file %s\n", d->inbasefilename, infilename);
				DecoderSession_destroy(d, /*error_occurred=*/true);
				return false;
			}
		}
	}
#endif

	if(analysis_mode)
		flac__analyze_init(aopts);

	return true;
}

void DecoderSession_destroy(DecoderSession *d, FLAC__bool error_occurred)
{
	if(0 != d->fout && d->fout != stdout) {
		fclose(d->fout);
		if(error_occurred)
			unlink(d->outfilename);
	}
#ifdef FLAC__HAS_OGG
	if(0 != d->fin && d->fin != stdin)
		fclose(d->fin);
#endif
}

FLAC__bool DecoderSession_init_decoder(DecoderSession *decoder_session, decode_options_t decode_options, const char *infilename)
{
	FLAC__uint32 test = 1;

	is_big_endian_host_ = (*((FLAC__byte*)(&test)))? false : true;

#ifdef FLAC__HAS_OGG
	if(decoder_session->is_ogg) {
		decoder_session->decoder.ogg.stream = OggFLAC__stream_decoder_new();

		if(0 == decoder_session->decoder.ogg.stream) {
			fprintf(stderr, "%s: ERROR creating the decoder instance\n", decoder_session->inbasefilename);
			return false;
		}

		if(!decode_options.use_first_serial_number)
			OggFLAC__stream_decoder_set_serial_number(decoder_session->decoder.ogg.stream, decode_options.serial_number);

		OggFLAC__stream_decoder_set_read_callback(decoder_session->decoder.ogg.stream, read_callback);
		/*
		 * The three ugly casts here are to 'downcast' the 'void *' argument of
		 * the callback down to 'FLAC__StreamDecoder *'.  In C++ this would be
		 * unnecessary but here the cast makes the C compiler happy.
		 */
		OggFLAC__stream_decoder_set_write_callback(decoder_session->decoder.ogg.stream, (FLAC__StreamDecoderWriteStatus (*)(const OggFLAC__StreamDecoder *, const FLAC__Frame *, const FLAC__int32 * const [], void *))write_callback);
		OggFLAC__stream_decoder_set_metadata_callback(decoder_session->decoder.ogg.stream, (void (*)(const OggFLAC__StreamDecoder *, const FLAC__StreamMetadata *, void *))metadata_callback);
		OggFLAC__stream_decoder_set_error_callback(decoder_session->decoder.ogg.stream, (void (*)(const OggFLAC__StreamDecoder *, FLAC__StreamDecoderErrorStatus, void *))error_callback);
		OggFLAC__stream_decoder_set_client_data(decoder_session->decoder.ogg.stream, decoder_session);

		if(OggFLAC__stream_decoder_init(decoder_session->decoder.ogg.stream) != OggFLAC__STREAM_DECODER_OK) {
			print_error_with_state(decoder_session, "ERROR initializing decoder");
			return false;
		}
	}
	else
#else
	(void)decode_options;
#endif
	{
		decoder_session->decoder.flac.file = FLAC__file_decoder_new();

		if(0 == decoder_session->decoder.flac.file) {
			fprintf(stderr, "%s: ERROR creating the decoder instance\n", decoder_session->inbasefilename);
			return false;
		}

		FLAC__file_decoder_set_md5_checking(decoder_session->decoder.flac.file, true);
		FLAC__file_decoder_set_filename(decoder_session->decoder.flac.file, infilename);
		/*
		 * The three ugly casts here are to 'downcast' the 'void *' argument of
		 * the callback down to 'FLAC__FileDecoder *'.
		 */
		FLAC__file_decoder_set_write_callback(decoder_session->decoder.flac.file, (FLAC__StreamDecoderWriteStatus (*)(const FLAC__FileDecoder *, const FLAC__Frame *, const FLAC__int32 * const [], void *))write_callback);
		FLAC__file_decoder_set_metadata_callback(decoder_session->decoder.flac.file, (void (*)(const FLAC__FileDecoder *, const FLAC__StreamMetadata *, void *))metadata_callback);
		FLAC__file_decoder_set_error_callback(decoder_session->decoder.flac.file, (void (*)(const FLAC__FileDecoder *, FLAC__StreamDecoderErrorStatus, void *))error_callback);
		FLAC__file_decoder_set_client_data(decoder_session->decoder.flac.file, decoder_session);

		if(FLAC__file_decoder_init(decoder_session->decoder.flac.file) != FLAC__FILE_DECODER_OK) {
			print_error_with_state(decoder_session, "ERROR initializing decoder");
			return false;
		}
	}

	return true;
}

FLAC__bool DecoderSession_process(DecoderSession *d)
{
#ifdef FLAC__HAS_OGG
	if(d->is_ogg) {
		if(!OggFLAC__stream_decoder_process_until_end_of_metadata(d->decoder.ogg.stream)) {
			if(d->verbose) fprintf(stderr, "\n");
			print_error_with_state(d, "ERROR while decoding metadata");
			return false;
		}
		if(OggFLAC__stream_decoder_get_FLAC_stream_decoder_state(d->decoder.ogg.stream) != FLAC__STREAM_DECODER_SEARCH_FOR_FRAME_SYNC && OggFLAC__stream_decoder_get_FLAC_stream_decoder_state(d->decoder.ogg.stream) != FLAC__STREAM_DECODER_END_OF_STREAM) {
			if(d->verbose) fprintf(stderr, "\n");
			print_error_with_state(d, "ERROR during metadata decoding");
			return false;
		}
	}
	else
#endif
	{
		if(!FLAC__file_decoder_process_until_end_of_metadata(d->decoder.flac.file)) {
			if(d->verbose) fprintf(stderr, "\n");
			print_error_with_state(d, "ERROR while decoding metadata");
			return false;
		}
		if(FLAC__file_decoder_get_state(d->decoder.flac.file) != FLAC__FILE_DECODER_OK && FLAC__file_decoder_get_state(d->decoder.flac.file) != FLAC__FILE_DECODER_END_OF_FILE) {
			if(d->verbose) fprintf(stderr, "\n");
			print_error_with_state(d, "ERROR during metadata decoding");
			return false;
		}
	}
	if(d->abort_flag)
		return false;

	if(d->skip_specification->value.samples > 0) {
		const FLAC__uint64 skip = (FLAC__uint64)d->skip_specification->value.samples;

#ifdef FLAC__HAS_OGG
		if(d->is_ogg) { /*@@@ (move this check into main.c) */
			fprintf(stderr, "%s: ERROR, can't skip when decoding Ogg-FLAC yet; convert to native-FLAC first\n", d->inbasefilename);
			return false;
		}
#endif
		if(!FLAC__file_decoder_seek_absolute(d->decoder.flac.file, skip)) {
			print_error_with_state(d, "ERROR seeking while skipping bytes");
			return false;
		}
		if(!FLAC__file_decoder_process_until_end_of_file(d->decoder.flac.file)) {
			if(d->verbose) fprintf(stderr, "\n");
			print_error_with_state(d, "ERROR while decoding frames");
			return false;
		}
		if(FLAC__file_decoder_get_state(d->decoder.flac.file) != FLAC__FILE_DECODER_OK && FLAC__file_decoder_get_state(d->decoder.flac.file) != FLAC__FILE_DECODER_END_OF_FILE) {
			if(d->verbose) fprintf(stderr, "\n");
			print_error_with_state(d, "ERROR during decoding");
			return false;
		}
	}
	else {
#ifdef FLAC__HAS_OGG
		if(d->is_ogg) {
			if(!OggFLAC__stream_decoder_process_until_end_of_stream(d->decoder.ogg.stream)) {
				if(d->verbose) fprintf(stderr, "\n");
				print_error_with_state(d, "ERROR while decoding data");
				return false;
			}
			if(OggFLAC__stream_decoder_get_FLAC_stream_decoder_state(d->decoder.ogg.stream) != FLAC__STREAM_DECODER_SEARCH_FOR_METADATA && OggFLAC__stream_decoder_get_FLAC_stream_decoder_state(d->decoder.ogg.stream) != FLAC__STREAM_DECODER_END_OF_STREAM) {
				if(d->verbose) fprintf(stderr, "\n");
				print_error_with_state(d, "ERROR during decoding");
				return false;
			}
		}
		else
#endif
		{
			if(!FLAC__file_decoder_process_until_end_of_file(d->decoder.flac.file)) {
				if(d->verbose) fprintf(stderr, "\n");
				print_error_with_state(d, "ERROR while decoding data");
				return false;
			}
			if(FLAC__file_decoder_get_state(d->decoder.flac.file) != FLAC__FILE_DECODER_OK && FLAC__file_decoder_get_state(d->decoder.flac.file) != FLAC__FILE_DECODER_END_OF_FILE) {
				if(d->verbose) fprintf(stderr, "\n");
				print_error_with_state(d, "ERROR during decoding");
				return false;
			}
		}
	}

	return true;
}

int DecoderSession_finish_ok(DecoderSession *d)
{
	FLAC__bool md5_failure = false;

#ifdef FLAC__HAS_OGG
	if(d->is_ogg) {
		if(d->decoder.ogg.stream) {
			OggFLAC__stream_decoder_finish(d->decoder.ogg.stream);
			md5_failure = false;
			print_stats(d);
			OggFLAC__stream_decoder_delete(d->decoder.ogg.stream);
		}
	}
	else
#endif
	{
		if(d->decoder.flac.file) {
			md5_failure = !FLAC__file_decoder_finish(d->decoder.flac.file);
			print_stats(d);
			FLAC__file_decoder_delete(d->decoder.flac.file);
		}
	}
	if(d->analysis_mode)
		flac__analyze_finish(d->aopts);
	if(md5_failure) {
		fprintf(stderr, "\r%s: WARNING, MD5 signature mismatch\n", d->inbasefilename);
	}
	else {
		if(d->verbose)
			fprintf(stderr, "\r%s: %s         \n", d->inbasefilename, d->test_only? "ok           ":d->analysis_mode?"done           ":"done");
	}
	DecoderSession_destroy(d, /*error_occurred=*/false);
	if(d->is_wave_out && d->wave_chunk_size_fixup.needs_fixup)
		if(!fixup_wave_chunk_size(d->outfilename, d->wave_chunk_size_fixup.riff_offset, d->wave_chunk_size_fixup.data_offset, (FLAC__uint32)d->samples_processed))
			return 1;
	return 0;
}

int DecoderSession_finish_error(DecoderSession *d)
{
#ifdef FLAC__HAS_OGG
	if(d->is_ogg) {
		if(d->decoder.ogg.stream) {
			OggFLAC__stream_decoder_finish(d->decoder.ogg.stream);
			OggFLAC__stream_decoder_delete(d->decoder.ogg.stream);
		}
	}
	else
#endif
	{
		if(d->decoder.flac.file) {
			FLAC__file_decoder_finish(d->decoder.flac.file);
			FLAC__file_decoder_delete(d->decoder.flac.file);
		}
	}
	if(d->analysis_mode)
		flac__analyze_finish(d->aopts);
	DecoderSession_destroy(d, /*error_occurred=*/true);
	return 1;
}

FLAC__bool write_little_endian_uint16(FILE *f, FLAC__uint16 val)
{
	FLAC__byte *b = (FLAC__byte*)(&val);
	if(is_big_endian_host_) {
		FLAC__byte tmp;
		tmp = b[1]; b[1] = b[0]; b[0] = tmp;
	}
	return fwrite(b, 1, 2, f) == 2;
}

FLAC__bool write_little_endian_uint32(FILE *f, FLAC__uint32 val)
{
	FLAC__byte *b = (FLAC__byte*)(&val);
	if(is_big_endian_host_) {
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
FLAC__StreamDecoderReadStatus read_callback(const OggFLAC__StreamDecoder *decoder, FLAC__byte buffer[], unsigned *bytes, void *client_data)
{
	DecoderSession *decoder_session = (DecoderSession*)client_data;
	FILE *fin = decoder_session->fin;
	size_t bytes_read;

	(void)decoder; /* avoid compiler warning */

	if (decoder_session->abort_flag)
		return FLAC__STREAM_DECODER_READ_STATUS_ABORT;

	if(feof(fin))
		return FLAC__STREAM_DECODER_READ_STATUS_END_OF_STREAM;

	bytes_read = fread(buffer, 1, *bytes, fin);

	if(ferror(fin))
		return FLAC__STREAM_DECODER_READ_STATUS_ABORT;

	*bytes = (unsigned)bytes_read;

	return FLAC__STREAM_DECODER_READ_STATUS_CONTINUE;
}
#endif

FLAC__StreamDecoderWriteStatus write_callback(const void *decoder, const FLAC__Frame *frame, const FLAC__int32 * const buffer[], void *client_data)
{
	DecoderSession *decoder_session = (DecoderSession*)client_data;
	FILE *fout = decoder_session->fout;
	const unsigned bps = frame->header.bits_per_sample, channels = frame->header.channels;
	FLAC__bool is_big_endian = (decoder_session->is_wave_out? false : decoder_session->is_big_endian);
	FLAC__bool is_unsigned_samples = (decoder_session->is_wave_out? bps<=8 : decoder_session->is_unsigned_samples);
	unsigned wide_samples = frame->header.blocksize, wide_sample, sample, channel, byte;
	static FLAC__int8 s8buffer[FLAC__MAX_BLOCK_SIZE * FLAC__MAX_CHANNELS * sizeof(FLAC__int32)]; /* WATCHOUT: can be up to 2 megs */
	FLAC__uint8  *u8buffer  = (FLAC__uint8  *)s8buffer;
	FLAC__int16  *s16buffer = (FLAC__int16  *)s8buffer;
	FLAC__uint16 *u16buffer = (FLAC__uint16 *)s8buffer;
	FLAC__int32  *s32buffer = (FLAC__int32  *)s8buffer;
	FLAC__uint32 *u32buffer = (FLAC__uint32 *)s8buffer;

	(void)decoder;

	if(decoder_session->abort_flag)
		return FLAC__STREAM_DECODER_WRITE_STATUS_ABORT;

	if(bps != decoder_session->bps) {
		fprintf(stderr, "%s: ERROR, bits-per-sample is %u in frame but %u in STREAMINFO\n", decoder_session->inbasefilename, bps, decoder_session->bps);
		return FLAC__STREAM_DECODER_WRITE_STATUS_ABORT;
	}
	if(channels != decoder_session->channels) {
		fprintf(stderr, "%s: ERROR, channels is %u in frame but %u in STREAMINFO\n", decoder_session->inbasefilename, channels, decoder_session->channels);
		return FLAC__STREAM_DECODER_WRITE_STATUS_ABORT;
	}
	if(frame->header.sample_rate != decoder_session->sample_rate) {
		fprintf(stderr, "%s: ERROR, sample rate is %u in frame but %u in STREAMINFO\n", decoder_session->inbasefilename, frame->header.sample_rate, decoder_session->sample_rate);
		return FLAC__STREAM_DECODER_WRITE_STATUS_ABORT;
	}

	decoder_session->samples_processed += wide_samples;
	decoder_session->frame_counter++;

	if(decoder_session->verbose && !(decoder_session->frame_counter & 0x3f))
		print_stats(decoder_session);

	if(decoder_session->analysis_mode) {
		flac__analyze_frame(frame, decoder_session->frame_counter-1, decoder_session->aopts, fout);
	}
	else if(!decoder_session->test_only) {
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
			if(is_big_endian != is_big_endian_host_) {
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
			if(is_big_endian != is_big_endian_host_) {
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

void metadata_callback(const void *decoder, const FLAC__StreamMetadata *metadata, void *client_data)
{
	DecoderSession *decoder_session = (DecoderSession*)client_data;
	(void)decoder;
	if(metadata->type == FLAC__METADATA_TYPE_STREAMINFO) {
		FLAC__uint64 skip;
		decoder_session->bps = metadata->data.stream_info.bits_per_sample;
		decoder_session->channels = metadata->data.stream_info.channels;
		decoder_session->sample_rate = metadata->data.stream_info.sample_rate;

		flac__utils_canonicalize_skip_until_specification(decoder_session->skip_specification, decoder_session->sample_rate);
		FLAC__ASSERT(decoder_session->skip_specification->value.samples >= 0);
		skip = (FLAC__uint64)decoder_session->skip_specification->value.samples;

		/* remember, metadata->data.stream_info.total_samples can be 0, meaning 'unknown' */
		if(metadata->data.stream_info.total_samples > 0 && skip >= metadata->data.stream_info.total_samples) {
			fprintf(stderr, "%s: ERROR trying to skip more samples than in stream\n", decoder_session->inbasefilename);
			decoder_session->abort_flag = true;
			return;
		}
		else if(metadata->data.stream_info.total_samples == 0 && skip > 0) {
			fprintf(stderr, "%s: ERROR, can't skip when FLAC metadata has total sample count of 0\n", decoder_session->inbasefilename);
			decoder_session->abort_flag = true;
			return;
		}
		else
			decoder_session->total_samples = metadata->data.stream_info.total_samples - skip;

		if(decoder_session->bps != 8 && decoder_session->bps != 16 && decoder_session->bps != 24) {
			fprintf(stderr, "%s: ERROR: bits per sample is not 8/16/24\n", decoder_session->inbasefilename);
			decoder_session->abort_flag = true;
			return;
		}

		/* write the WAVE headers if necessary */
		if(!decoder_session->analysis_mode && !decoder_session->test_only && decoder_session->is_wave_out) {
			FLAC__uint64 data_size = decoder_session->total_samples * decoder_session->channels * ((decoder_session->bps+7)/8);
			if(decoder_session->total_samples == 0) {
				if(decoder_session->fout == stdout) {
					fprintf(stderr, "%s: WARNING, don't have accurate sample count available for WAVE header.\n", decoder_session->inbasefilename);
					fprintf(stderr, "             Generated WAVE file will have a data chunk size of 0.  Try\n");
					fprintf(stderr, "             decoding directly to a file instead.\n");
				}
				else {
					decoder_session->wave_chunk_size_fixup.needs_fixup = true;
				}
			}
			if(data_size >= 0xFFFFFFDC) {
				fprintf(stderr, "%s: ERROR: stream is too big to fit in a single WAVE file chunk\n", decoder_session->inbasefilename);
				decoder_session->abort_flag = true;
				return;
			}
			if(fwrite("RIFF", 1, 4, decoder_session->fout) != 4) decoder_session->abort_flag = true;
			if(decoder_session->wave_chunk_size_fixup.needs_fixup)
				decoder_session->wave_chunk_size_fixup.riff_offset = ftell(decoder_session->fout);
			if(!write_little_endian_uint32(decoder_session->fout, (FLAC__uint32)(data_size+36))) decoder_session->abort_flag = true; /* filesize-8 */
			if(fwrite("WAVEfmt ", 1, 8, decoder_session->fout) != 8) decoder_session->abort_flag = true;
			if(fwrite("\020\000\000\000", 1, 4, decoder_session->fout) != 4) decoder_session->abort_flag = true; /* chunk size = 16 */
			if(fwrite("\001\000", 1, 2, decoder_session->fout) != 2) decoder_session->abort_flag = true; /* compression code == 1 */
			if(!write_little_endian_uint16(decoder_session->fout, (FLAC__uint16)(decoder_session->channels))) decoder_session->abort_flag = true;
			if(!write_little_endian_uint32(decoder_session->fout, decoder_session->sample_rate)) decoder_session->abort_flag = true;
			if(!write_little_endian_uint32(decoder_session->fout, decoder_session->sample_rate * decoder_session->channels * ((decoder_session->bps+7) / 8))) decoder_session->abort_flag = true; /* @@@ or is it (sample_rate*channels*bps) / 8 ??? */
			if(!write_little_endian_uint16(decoder_session->fout, (FLAC__uint16)(decoder_session->channels * ((decoder_session->bps+7) / 8)))) decoder_session->abort_flag = true; /* block align */
			if(!write_little_endian_uint16(decoder_session->fout, (FLAC__uint16)(decoder_session->bps))) decoder_session->abort_flag = true; /* bits per sample */
			if(fwrite("data", 1, 4, decoder_session->fout) != 4) decoder_session->abort_flag = true;
			if(decoder_session->wave_chunk_size_fixup.needs_fixup)
				decoder_session->wave_chunk_size_fixup.data_offset = ftell(decoder_session->fout);
			if(!write_little_endian_uint32(decoder_session->fout, (FLAC__uint32)data_size)) decoder_session->abort_flag = true; /* data size */
		}
	}
}

void error_callback(const void *decoder, FLAC__StreamDecoderErrorStatus status, void *client_data)
{
	DecoderSession *decoder_session = (DecoderSession*)client_data;
	(void)decoder;
	fprintf(stderr, "%s: *** Got error code %d:%s\n", decoder_session->inbasefilename, status, FLAC__StreamDecoderErrorStatusString[status]);
	if(!decoder_session->continue_through_decode_errors)
		decoder_session->abort_flag = true;
}

void print_error_with_state(const DecoderSession *d, const char *message)
{
	const int ilen = strlen(d->inbasefilename) + 1;

	fprintf(stderr, "\n%s: %s\n", d->inbasefilename, message);

#ifdef FLAC__HAS_OGG
	if(d->is_ogg) {
		const OggFLAC__StreamDecoderState osd_state = OggFLAC__stream_decoder_get_state(d->decoder.ogg.stream);
		if(osd_state != OggFLAC__STREAM_DECODER_FLAC_STREAM_DECODER_ERROR) {
			fprintf(stderr, "%*s state = %d:%s\n", ilen, "", (int)osd_state, OggFLAC__StreamDecoderStateString[osd_state]);
		}
		else {
			const FLAC__StreamDecoderState fsd_state = OggFLAC__stream_decoder_get_FLAC_stream_decoder_state(d->decoder.ogg.stream);
			fprintf(stderr, "%*s state = %d:%s\n", ilen, "", (int)fsd_state, FLAC__StreamDecoderStateString[fsd_state]);
		}
	}
	else
#endif
	{
		const FLAC__FileDecoderState ffd_state = FLAC__file_decoder_get_state(d->decoder.flac.file);
		if(ffd_state != FLAC__FILE_DECODER_SEEKABLE_STREAM_DECODER_ERROR) {
			fprintf(stderr, "%*s state = %d:%s\n", ilen, "", (int)ffd_state, FLAC__FileDecoderStateString[ffd_state]);
		}
		else {
			const FLAC__SeekableStreamDecoderState fssd_state = FLAC__file_decoder_get_seekable_stream_decoder_state(d->decoder.flac.file);
			if(fssd_state != FLAC__SEEKABLE_STREAM_DECODER_STREAM_DECODER_ERROR) {
				fprintf(stderr, "%*s state = %d:%s\n", ilen, "", (int)fssd_state, FLAC__SeekableStreamDecoderStateString[fssd_state]);
			}
			else {
				const FLAC__StreamDecoderState fsd_state = FLAC__file_decoder_get_stream_decoder_state(d->decoder.flac.file);
				fprintf(stderr, "%*s state = %d:%s\n", ilen, "", (int)fsd_state, FLAC__StreamDecoderStateString[fsd_state]);
			}
		}
	}
}

void print_stats(const DecoderSession *decoder_session)
{
	if(decoder_session->verbose) {
#if defined _MSC_VER || defined __MINGW32__
		/* with VC++ you have to spoon feed it the casting */
		const double progress = (double)(FLAC__int64)decoder_session->samples_processed / (double)(FLAC__int64)decoder_session->total_samples * 100.0;
#else
		const double progress = (double)decoder_session->samples_processed / (double)decoder_session->total_samples * 100.0;
#endif
		if(decoder_session->total_samples > 0) {
			fprintf(stderr, "\r%s: %s%u%% complete",
				decoder_session->inbasefilename,
				decoder_session->test_only? "testing, " : decoder_session->analysis_mode? "analyzing, " : "",
				(unsigned)floor(progress + 0.5)
			);
		}
		else {
			fprintf(stderr, "\r%s: %s %u samples",
				decoder_session->inbasefilename,
				decoder_session->test_only? "tested" : decoder_session->analysis_mode? "analyzed" : "wrote",
				(unsigned)decoder_session->samples_processed
			);
		}
	}
}
