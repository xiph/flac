/* flac - Command-line FLAC encoder/decoder
 * Copyright (C) 2000,2001,2002,2003,2004,2005,2006  Josh Coalson
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
#include <errno.h>
#include <math.h> /* for floor() */
#include <stdio.h> /* for FILE et al. */
#include <string.h> /* for strcmp() */
#include "FLAC/all.h"
#include "share/grabbag.h"
#include "share/replaygain_synthesis.h"
#include "decode.h"

#ifdef FLAC__HAS_OGG
#include "OggFLAC/file_decoder.h"
#endif

typedef struct {
#ifdef FLAC__HAS_OGG
	FLAC__bool is_ogg;
#endif

	FLAC__bool is_aiff_out;
	FLAC__bool is_wave_out;
	FLAC__bool continue_through_decode_errors;

	struct {
		replaygain_synthesis_spec_t spec;
		FLAC__bool apply; /* 'spec.apply' is just a request; this 'apply' means we actually parsed the RG tags and are ready to go */
		double scale;
		DitherContext dither_context;
	} replaygain;

	FLAC__bool test_only;
	FLAC__bool analysis_mode;
	analysis_options aopts;
	utils__SkipUntilSpecification *skip_specification;
	utils__SkipUntilSpecification *until_specification; /* a canonicalized value of 0 mean end-of-stream (i.e. --until=-0) */
	utils__CueSpecification *cue_specification;

	const char *inbasefilename;
	const char *outfilename;

	FLAC__uint64 samples_processed;
	unsigned frame_counter;
	FLAC__bool abort_flag;
	FLAC__bool aborting_due_to_until; /* true if we intentionally abort decoding prematurely because we hit the --until point */

	struct {
		FLAC__bool needs_fixup;
		unsigned riff_offset; /* or FORM offset for AIFF */
		unsigned data_offset; /* or SSND offset for AIFF */
		unsigned frames_offset; /* AIFF only */
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
			OggFLAC__FileDecoder *file;
		} ogg;
#endif
	} decoder;

	FILE *fout;
} DecoderSession;


static FLAC__bool is_big_endian_host_;


/*
 * local routines
 */
static FLAC__bool DecoderSession_construct(DecoderSession *d, FLAC__bool is_ogg, FLAC__bool is_aiff_out, FLAC__bool is_wave_out, FLAC__bool continue_through_decode_errors, replaygain_synthesis_spec_t replaygain_synthesis_spec, FLAC__bool analysis_mode, analysis_options aopts, utils__SkipUntilSpecification *skip_specification, utils__SkipUntilSpecification *until_specification, utils__CueSpecification *cue_specification, const char *infilename, const char *outfilename);
static void DecoderSession_destroy(DecoderSession *d, FLAC__bool error_occurred);
static FLAC__bool DecoderSession_init_decoder(DecoderSession *d, decode_options_t decode_options, const char *infilename);
static FLAC__bool DecoderSession_process(DecoderSession *d);
static int DecoderSession_finish_ok(DecoderSession *d);
static int DecoderSession_finish_error(DecoderSession *d);
static FLAC__bool canonicalize_until_specification(utils__SkipUntilSpecification *spec, const char *inbasefilename, unsigned sample_rate, FLAC__uint64 skip, FLAC__uint64 total_samples_in_input);
static FLAC__bool write_necessary_headers(DecoderSession *decoder_session);
static FLAC__bool write_little_endian_uint16(FILE *f, FLAC__uint16 val);
static FLAC__bool write_little_endian_uint32(FILE *f, FLAC__uint32 val);
static FLAC__bool write_big_endian_uint16(FILE *f, FLAC__uint16 val);
static FLAC__bool write_big_endian_uint32(FILE *f, FLAC__uint32 val);
static FLAC__bool write_sane_extended(FILE *f, unsigned val);
static FLAC__bool fixup_wave_chunk_size(const char *outfilename, FLAC__bool is_wave_out, unsigned riff_offset, unsigned data_offset, unsigned frames_offset, FLAC__uint32 total_samples, unsigned channels, unsigned bps);
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
int flac__decode_aiff(const char *infilename, const char *outfilename, FLAC__bool analysis_mode, analysis_options aopts, wav_decode_options_t options)
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
			/*is_aiff_out=*/true,
			/*is_wave_out=*/false,
			options.common.continue_through_decode_errors,
			options.common.replaygain_synthesis_spec,
			analysis_mode,
			aopts,
			&options.common.skip_specification,
			&options.common.until_specification,
			options.common.has_cue_specification? &options.common.cue_specification : 0,
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
			/*is_aiff_out=*/false,
			/*is_wave_out=*/true,
			options.common.continue_through_decode_errors,
			options.common.replaygain_synthesis_spec,
			analysis_mode,
			aopts,
			&options.common.skip_specification,
			&options.common.until_specification,
			options.common.has_cue_specification? &options.common.cue_specification : 0,
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
			/*is_aiff_out=*/false,
			/*is_wave_out=*/false,
			options.common.continue_through_decode_errors,
			options.common.replaygain_synthesis_spec,
			analysis_mode,
			aopts,
			&options.common.skip_specification,
			&options.common.until_specification,
			options.common.has_cue_specification? &options.common.cue_specification : 0,
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

FLAC__bool DecoderSession_construct(DecoderSession *d, FLAC__bool is_ogg, FLAC__bool is_aiff_out, FLAC__bool is_wave_out, FLAC__bool continue_through_decode_errors, replaygain_synthesis_spec_t replaygain_synthesis_spec, FLAC__bool analysis_mode, analysis_options aopts, utils__SkipUntilSpecification *skip_specification, utils__SkipUntilSpecification *until_specification, utils__CueSpecification *cue_specification, const char *infilename, const char *outfilename)
{
#ifdef FLAC__HAS_OGG
	d->is_ogg = is_ogg;
#else
	(void)is_ogg;
#endif

	d->is_aiff_out = is_aiff_out;
	d->is_wave_out = is_wave_out;
	d->continue_through_decode_errors = continue_through_decode_errors;
	d->replaygain.spec = replaygain_synthesis_spec;
	d->replaygain.apply = false;
	d->replaygain.scale = 0.0;
	/* d->replaygain.dither_context gets initialized later once we know the sample resolution */
	d->test_only = (0 == outfilename);
	d->analysis_mode = analysis_mode;
	d->aopts = aopts;
	d->skip_specification = skip_specification;
	d->until_specification = until_specification;
	d->cue_specification = cue_specification;

	d->inbasefilename = grabbag__file_get_basename(infilename);
	d->outfilename = outfilename;

	d->samples_processed = 0;
	d->frame_counter = 0;
	d->abort_flag = false;
	d->aborting_due_to_until = false;

	d->wave_chunk_size_fixup.needs_fixup = false;

	d->decoder.flac.file = 0;
#ifdef FLAC__HAS_OGG
	d->decoder.ogg.file = 0;
#endif

	d->fout = 0; /* initialized with an open file later if necessary */

	FLAC__ASSERT(!(d->test_only && d->analysis_mode));

	if(!d->test_only) {
		if(0 == strcmp(outfilename, "-")) {
			d->fout = grabbag__file_get_binary_stdout();
		}
		else {
			if(0 == (d->fout = fopen(outfilename, "wb"))) {
				flac__utils_printf(stderr, 1, "%s: ERROR: can't open output file %s\n", d->inbasefilename, outfilename);
				DecoderSession_destroy(d, /*error_occurred=*/true);
				return false;
			}
		}
	}

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
}

FLAC__bool DecoderSession_init_decoder(DecoderSession *decoder_session, decode_options_t decode_options, const char *infilename)
{
	FLAC__uint32 test = 1;

	is_big_endian_host_ = (*((FLAC__byte*)(&test)))? false : true;

#ifdef FLAC__HAS_OGG
	if(decoder_session->is_ogg) {
		decoder_session->decoder.ogg.file = OggFLAC__file_decoder_new();

		if(0 == decoder_session->decoder.ogg.file) {
			flac__utils_printf(stderr, 1, "%s: ERROR creating the decoder instance\n", decoder_session->inbasefilename);
			return false;
		}

		OggFLAC__file_decoder_set_md5_checking(decoder_session->decoder.ogg.file, true);
		OggFLAC__file_decoder_set_filename(decoder_session->decoder.ogg.file, infilename);
		if(!decode_options.use_first_serial_number)
			OggFLAC__file_decoder_set_serial_number(decoder_session->decoder.ogg.file, decode_options.serial_number);
		if (0 != decoder_session->cue_specification)
			OggFLAC__file_decoder_set_metadata_respond(decoder_session->decoder.ogg.file, FLAC__METADATA_TYPE_CUESHEET);
		if (decoder_session->replaygain.spec.apply)
			OggFLAC__file_decoder_set_metadata_respond(decoder_session->decoder.ogg.file, FLAC__METADATA_TYPE_VORBIS_COMMENT);

		/*
		 * The three ugly casts here are to 'downcast' the 'void *' argument of
		 * the callback down to 'OggFLAC__FileDecoder *'.  In C++ this would be
		 * unnecessary but here the cast makes the C compiler happy.
		 */
		OggFLAC__file_decoder_set_write_callback(decoder_session->decoder.ogg.file, (FLAC__StreamDecoderWriteStatus (*)(const OggFLAC__FileDecoder *, const FLAC__Frame *, const FLAC__int32 * const [], void *))write_callback);
		OggFLAC__file_decoder_set_metadata_callback(decoder_session->decoder.ogg.file, (void (*)(const OggFLAC__FileDecoder *, const FLAC__StreamMetadata *, void *))metadata_callback);
		OggFLAC__file_decoder_set_error_callback(decoder_session->decoder.ogg.file, (void (*)(const OggFLAC__FileDecoder *, FLAC__StreamDecoderErrorStatus, void *))error_callback);
		OggFLAC__file_decoder_set_client_data(decoder_session->decoder.ogg.file, decoder_session);

		if(OggFLAC__file_decoder_init(decoder_session->decoder.ogg.file) != OggFLAC__FILE_DECODER_OK) {
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
			flac__utils_printf(stderr, 1, "%s: ERROR creating the decoder instance\n", decoder_session->inbasefilename);
			return false;
		}

		FLAC__file_decoder_set_md5_checking(decoder_session->decoder.flac.file, true);
		FLAC__file_decoder_set_filename(decoder_session->decoder.flac.file, infilename);
		if (0 != decoder_session->cue_specification)
			FLAC__file_decoder_set_metadata_respond(decoder_session->decoder.flac.file, FLAC__METADATA_TYPE_CUESHEET);
		if (decoder_session->replaygain.spec.apply)
			FLAC__file_decoder_set_metadata_respond(decoder_session->decoder.flac.file, FLAC__METADATA_TYPE_VORBIS_COMMENT);
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
		if(!OggFLAC__file_decoder_process_until_end_of_metadata(d->decoder.ogg.file)) {
			flac__utils_printf(stderr, 2, "\n");
			print_error_with_state(d, "ERROR while decoding metadata");
			return false;
		}
		if(OggFLAC__file_decoder_get_state(d->decoder.ogg.file) != OggFLAC__FILE_DECODER_OK && OggFLAC__file_decoder_get_state(d->decoder.ogg.file) != OggFLAC__FILE_DECODER_END_OF_FILE) {
			flac__utils_printf(stderr, 2, "\n");
			print_error_with_state(d, "ERROR during metadata decoding");
			return false;
		}
	}
	else
#endif
	{
		if(!FLAC__file_decoder_process_until_end_of_metadata(d->decoder.flac.file)) {
			flac__utils_printf(stderr, 2, "\n");
			print_error_with_state(d, "ERROR while decoding metadata");
			return false;
		}
		if(FLAC__file_decoder_get_state(d->decoder.flac.file) != FLAC__FILE_DECODER_OK && FLAC__file_decoder_get_state(d->decoder.flac.file) != FLAC__FILE_DECODER_END_OF_FILE) {
			flac__utils_printf(stderr, 2, "\n");
			print_error_with_state(d, "ERROR during metadata decoding");
			return false;
		}
	}
	if(d->abort_flag)
		return false;

	/* write the WAVE/AIFF headers if necessary */
	if(!write_necessary_headers(d)) {
		d->abort_flag = true;
		return false;
	}

	if(d->skip_specification->value.samples > 0) {
		const FLAC__uint64 skip = (FLAC__uint64)d->skip_specification->value.samples;

#ifdef FLAC__HAS_OGG
		if(d->is_ogg) {
			if(!OggFLAC__file_decoder_seek_absolute(d->decoder.ogg.file, skip)) {
				print_error_with_state(d, "ERROR seeking while skipping bytes");
				return false;
			}
			if(!OggFLAC__file_decoder_process_until_end_of_file(d->decoder.ogg.file) && !d->aborting_due_to_until) {
				flac__utils_printf(stderr, 2, "\n");
				print_error_with_state(d, "ERROR while decoding frames");
				return false;
			}
			if(OggFLAC__file_decoder_get_state(d->decoder.ogg.file) != OggFLAC__FILE_DECODER_OK && OggFLAC__file_decoder_get_state(d->decoder.ogg.file) != OggFLAC__FILE_DECODER_END_OF_FILE && !d->aborting_due_to_until) {
				flac__utils_printf(stderr, 2, "\n");
				print_error_with_state(d, "ERROR during decoding");
				return false;
			}
		}
		else
#endif
		{
			if(!FLAC__file_decoder_seek_absolute(d->decoder.flac.file, skip)) {
				print_error_with_state(d, "ERROR seeking while skipping bytes");
				return false;
			}
			if(!FLAC__file_decoder_process_until_end_of_file(d->decoder.flac.file) && !d->aborting_due_to_until) {
				flac__utils_printf(stderr, 2, "\n");
				print_error_with_state(d, "ERROR while decoding frames");
				return false;
			}
			if(FLAC__file_decoder_get_state(d->decoder.flac.file) != FLAC__FILE_DECODER_OK && FLAC__file_decoder_get_state(d->decoder.flac.file) != FLAC__FILE_DECODER_END_OF_FILE && !d->aborting_due_to_until) {
				flac__utils_printf(stderr, 2, "\n");
				print_error_with_state(d, "ERROR during decoding");
				return false;
			}
		}
	}
	else {
#ifdef FLAC__HAS_OGG
		if(d->is_ogg) {
			if(!OggFLAC__file_decoder_process_until_end_of_file(d->decoder.ogg.file) && !d->aborting_due_to_until) {
				flac__utils_printf(stderr, 2, "\n");
				print_error_with_state(d, "ERROR while decoding data");
				return false;
			}
			if(OggFLAC__file_decoder_get_state(d->decoder.ogg.file) != OggFLAC__FILE_DECODER_OK && OggFLAC__file_decoder_get_state(d->decoder.ogg.file) != OggFLAC__FILE_DECODER_END_OF_FILE && !d->aborting_due_to_until) {
				flac__utils_printf(stderr, 2, "\n");
				print_error_with_state(d, "ERROR during decoding");
				return false;
			}
		}
		else
#endif
		{
			if(!FLAC__file_decoder_process_until_end_of_file(d->decoder.flac.file) && !d->aborting_due_to_until) {
				flac__utils_printf(stderr, 2, "\n");
				print_error_with_state(d, "ERROR while decoding data");
				return false;
			}
			if(FLAC__file_decoder_get_state(d->decoder.flac.file) != FLAC__FILE_DECODER_OK && FLAC__file_decoder_get_state(d->decoder.flac.file) != FLAC__FILE_DECODER_END_OF_FILE && !d->aborting_due_to_until) {
				flac__utils_printf(stderr, 2, "\n");
				print_error_with_state(d, "ERROR during decoding");
				return false;
			}
		}
	}

	if((d->is_wave_out || d->is_aiff_out) && ((d->total_samples * d->channels * ((d->bps+7)/8)) & 1)) {
		if(flac__utils_fwrite("\000", 1, 1, d->fout) != 1) {
			print_error_with_state(d, d->is_wave_out?
				"ERROR writing pad byte to WAVE data chunk" :
				"ERROR writing pad byte to AIFF SSND chunk"
			);
			return false;
		}
	}

	return true;
}

int DecoderSession_finish_ok(DecoderSession *d)
{
	FLAC__bool md5_failure = false;

#ifdef FLAC__HAS_OGG
	if(d->is_ogg) {
		if(d->decoder.ogg.file) {
			md5_failure = !OggFLAC__file_decoder_finish(d->decoder.ogg.file) && !d->aborting_due_to_until;
			print_stats(d);
			OggFLAC__file_decoder_delete(d->decoder.ogg.file);
		}
	}
	else
#endif
	{
		if(d->decoder.flac.file) {
			md5_failure = !FLAC__file_decoder_finish(d->decoder.flac.file) && !d->aborting_due_to_until;
			print_stats(d);
			FLAC__file_decoder_delete(d->decoder.flac.file);
		}
	}
	if(d->analysis_mode)
		flac__analyze_finish(d->aopts);
	if(md5_failure) {
		flac__utils_printf(stderr, 1, "\r%s: WARNING, MD5 signature mismatch\n", d->inbasefilename);
	}
	else {
		flac__utils_printf(stderr, 2, "\r%s: %s         \n", d->inbasefilename, d->test_only? "ok           ":d->analysis_mode?"done           ":"done");
	}
	DecoderSession_destroy(d, /*error_occurred=*/false);
	if((d->is_wave_out || d->is_aiff_out) && d->wave_chunk_size_fixup.needs_fixup)
		if(!fixup_wave_chunk_size(d->outfilename, d->is_wave_out, d->wave_chunk_size_fixup.riff_offset, d->wave_chunk_size_fixup.data_offset, d->wave_chunk_size_fixup.frames_offset, (FLAC__uint32)d->samples_processed, d->channels, d->bps))
			return 1;
	return 0;
}

int DecoderSession_finish_error(DecoderSession *d)
{
#ifdef FLAC__HAS_OGG
	if(d->is_ogg) {
		if(d->decoder.ogg.file) {
			OggFLAC__file_decoder_finish(d->decoder.ogg.file);
			OggFLAC__file_decoder_delete(d->decoder.ogg.file);
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

FLAC__bool canonicalize_until_specification(utils__SkipUntilSpecification *spec, const char *inbasefilename, unsigned sample_rate, FLAC__uint64 skip, FLAC__uint64 total_samples_in_input)
{
	/* convert from mm:ss.sss to sample number if necessary */
	flac__utils_canonicalize_skip_until_specification(spec, sample_rate);

	/* special case: if "--until=-0", use the special value '0' to mean "end-of-stream" */
	if(spec->is_relative && spec->value.samples == 0) {
		spec->is_relative = false;
		return true;
	}

	/* in any other case the total samples in the input must be known */
	if(total_samples_in_input == 0) {
		flac__utils_printf(stderr, 1, "%s: ERROR, cannot use --until when FLAC metadata has total sample count of 0\n", inbasefilename);
		return false;
	}

	FLAC__ASSERT(spec->value_is_samples);

	/* convert relative specifications to absolute */
	if(spec->is_relative) {
		if(spec->value.samples <= 0)
			spec->value.samples += (FLAC__int64)total_samples_in_input;
		else
			spec->value.samples += skip;
		spec->is_relative = false;
	}

	/* error check */
	if(spec->value.samples < 0) {
		flac__utils_printf(stderr, 1, "%s: ERROR, --until value is before beginning of input\n", inbasefilename);
		return false;
	}
	if((FLAC__uint64)spec->value.samples <= skip) {
		flac__utils_printf(stderr, 1, "%s: ERROR, --until value is before --skip point\n", inbasefilename);
		return false;
	}
	if((FLAC__uint64)spec->value.samples > total_samples_in_input) {
		flac__utils_printf(stderr, 1, "%s: ERROR, --until value is after end of input\n", inbasefilename);
		return false;
	}

	return true;
}

FLAC__bool write_necessary_headers(DecoderSession *decoder_session)
{
	/* write the WAVE/AIFF headers if necessary */
	if(!decoder_session->analysis_mode && !decoder_session->test_only && (decoder_session->is_wave_out || decoder_session->is_aiff_out)) {
		const char *fmt_desc = decoder_session->is_wave_out? "WAVE" : "AIFF";
		FLAC__uint64 data_size = decoder_session->total_samples * decoder_session->channels * ((decoder_session->bps+7)/8);
		const FLAC__uint32 aligned_data_size = (FLAC__uint32)((data_size+1) & (~1U)); /* we'll check for overflow later */
		if(decoder_session->total_samples == 0) {
			if(decoder_session->fout == stdout) {
				flac__utils_printf(stderr, 1, "%s: WARNING, don't have accurate sample count available for %s header.\n", decoder_session->inbasefilename, fmt_desc);
				flac__utils_printf(stderr, 1, "             Generated %s file will have a data chunk size of 0.  Try\n", fmt_desc);
				flac__utils_printf(stderr, 1, "             decoding directly to a file instead.\n");
			}
			else {
				decoder_session->wave_chunk_size_fixup.needs_fixup = true;
			}
		}
		if(data_size >= 0xFFFFFFDC) {
			flac__utils_printf(stderr, 1, "%s: ERROR: stream is too big to fit in a single %s file chunk\n", decoder_session->inbasefilename, fmt_desc);
			return false;
		}
		if(decoder_session->is_wave_out) {
			if(flac__utils_fwrite("RIFF", 1, 4, decoder_session->fout) != 4)
				return false;

			if(decoder_session->wave_chunk_size_fixup.needs_fixup)
				decoder_session->wave_chunk_size_fixup.riff_offset = ftell(decoder_session->fout);

			if(!write_little_endian_uint32(decoder_session->fout, aligned_data_size+36)) /* filesize-8 */
				return false;

			if(flac__utils_fwrite("WAVEfmt ", 1, 8, decoder_session->fout) != 8)
				return false;

			if(flac__utils_fwrite("\020\000\000\000", 1, 4, decoder_session->fout) != 4) /* chunk size = 16 */
				return false;

			if(flac__utils_fwrite("\001\000", 1, 2, decoder_session->fout) != 2) /* compression code == 1 */
				return false;

			if(!write_little_endian_uint16(decoder_session->fout, (FLAC__uint16)(decoder_session->channels)))
				return false;

			if(!write_little_endian_uint32(decoder_session->fout, decoder_session->sample_rate))
				return false;

			if(!write_little_endian_uint32(decoder_session->fout, decoder_session->sample_rate * decoder_session->channels * ((decoder_session->bps+7) / 8))) /* @@@ or is it (sample_rate*channels*bps) / 8 ??? */
				return false;

			if(!write_little_endian_uint16(decoder_session->fout, (FLAC__uint16)(decoder_session->channels * ((decoder_session->bps+7) / 8)))) /* block align */
				return false;

			if(!write_little_endian_uint16(decoder_session->fout, (FLAC__uint16)(decoder_session->bps))) /* bits per sample */
				return false;

			if(flac__utils_fwrite("data", 1, 4, decoder_session->fout) != 4)
				return false;

			if(decoder_session->wave_chunk_size_fixup.needs_fixup)
				decoder_session->wave_chunk_size_fixup.data_offset = ftell(decoder_session->fout);

			if(!write_little_endian_uint32(decoder_session->fout, (FLAC__uint32)data_size)) /* data size */
				return false;
		}
		else {
			if(flac__utils_fwrite("FORM", 1, 4, decoder_session->fout) != 4)
				return false;

			if(decoder_session->wave_chunk_size_fixup.needs_fixup)
				decoder_session->wave_chunk_size_fixup.riff_offset = ftell(decoder_session->fout);

			if(!write_big_endian_uint32(decoder_session->fout, aligned_data_size+46)) /* filesize-8 */
				return false;

			if(flac__utils_fwrite("AIFFCOMM", 1, 8, decoder_session->fout) != 8)
				return false;

			if(flac__utils_fwrite("\000\000\000\022", 1, 4, decoder_session->fout) != 4) /* chunk size = 18 */
				return false;

			if(!write_big_endian_uint16(decoder_session->fout, (FLAC__uint16)(decoder_session->channels)))
				return false;

			if(decoder_session->wave_chunk_size_fixup.needs_fixup)
				decoder_session->wave_chunk_size_fixup.frames_offset = ftell(decoder_session->fout);

			if(!write_big_endian_uint32(decoder_session->fout, (FLAC__uint32)decoder_session->total_samples))
				return false;

			if(!write_big_endian_uint16(decoder_session->fout, (FLAC__uint16)(decoder_session->bps)))
				return false;

			if(!write_sane_extended(decoder_session->fout, decoder_session->sample_rate))
				return false;

			if(flac__utils_fwrite("SSND", 1, 4, decoder_session->fout) != 4)
				return false;

			if(decoder_session->wave_chunk_size_fixup.needs_fixup)
				decoder_session->wave_chunk_size_fixup.data_offset = ftell(decoder_session->fout);

			if(!write_big_endian_uint32(decoder_session->fout, (FLAC__uint32)data_size+8)) /* data size */
				return false;

			if(!write_big_endian_uint32(decoder_session->fout, 0/*offset*/))
				return false;

			if(!write_big_endian_uint32(decoder_session->fout, 0/*block_size*/))
				return false;
		}
	}

	return true;
}

FLAC__bool write_little_endian_uint16(FILE *f, FLAC__uint16 val)
{
	FLAC__byte *b = (FLAC__byte*)(&val);
	if(is_big_endian_host_) {
		FLAC__byte tmp;
		tmp = b[1]; b[1] = b[0]; b[0] = tmp;
	}
	return flac__utils_fwrite(b, 1, 2, f) == 2;
}

FLAC__bool write_little_endian_uint32(FILE *f, FLAC__uint32 val)
{
	FLAC__byte *b = (FLAC__byte*)(&val);
	if(is_big_endian_host_) {
		FLAC__byte tmp;
		tmp = b[3]; b[3] = b[0]; b[0] = tmp;
		tmp = b[2]; b[2] = b[1]; b[1] = tmp;
	}
	return flac__utils_fwrite(b, 1, 4, f) == 4;
}

FLAC__bool write_big_endian_uint16(FILE *f, FLAC__uint16 val)
{
	FLAC__byte *b = (FLAC__byte*)(&val);
	if(!is_big_endian_host_) {
		FLAC__byte tmp;
		tmp = b[1]; b[1] = b[0]; b[0] = tmp;
	}
	return flac__utils_fwrite(b, 1, 2, f) == 2;
}

FLAC__bool write_big_endian_uint32(FILE *f, FLAC__uint32 val)
{
	FLAC__byte *b = (FLAC__byte*)(&val);
	if(!is_big_endian_host_) {
		FLAC__byte tmp;
		tmp = b[3]; b[3] = b[0]; b[0] = tmp;
		tmp = b[2]; b[2] = b[1]; b[1] = tmp;
	}
	return flac__utils_fwrite(b, 1, 4, f) == 4;
}

FLAC__bool write_sane_extended(FILE *f, unsigned val)
	/* Write to 'f' a SANE extended representation of 'val'.  Return false if
	* the write succeeds; return true otherwise.
	*
	* SANE extended is an 80-bit IEEE-754 representation with sign bit, 15 bits
	* of exponent, and 64 bits of significand (mantissa).  Unlike most IEEE-754
	* representations, it does not imply a 1 above the MSB of the significand.
	*
	* Preconditions:
	*  val!=0U
	*/
{
	unsigned int shift, exponent;

	FLAC__ASSERT(val!=0U); /* handling 0 would require a special case */

	for(shift= 0U; (val>>(31-shift))==0U; ++shift)
		;
	val<<= shift;
	exponent= 63U-(shift+32U); /* add 32 for unused second word */

	if(!write_big_endian_uint16(f, (FLAC__uint16)(exponent+0x3FFF)))
		return false;
	if(!write_big_endian_uint32(f, val))
		return false;
	if(!write_big_endian_uint32(f, 0)) /* unused second word */
		return false;

	return true;
}

FLAC__bool fixup_wave_chunk_size(const char *outfilename, FLAC__bool is_wave_out, unsigned riff_offset, unsigned data_offset, unsigned frames_offset, FLAC__uint32 total_samples, unsigned channels, unsigned bps)
{
	const char *fmt_desc = (is_wave_out? "WAVE" : "AIFF");
	FLAC__bool (*write_it)(FILE *, FLAC__uint32) = (is_wave_out? write_little_endian_uint32 : write_big_endian_uint32);
	FILE *f = fopen(outfilename, "r+b");
	FLAC__uint32 data_size, aligned_data_size;

	if(0 == f) {
		flac__utils_printf(stderr, 1, "ERROR, couldn't open file %s while fixing up %s chunk size\n", outfilename, fmt_desc);
		return false;
	}

	data_size = aligned_data_size = total_samples * channels * ((bps+7)/8);
	if(aligned_data_size & 1)
		aligned_data_size++;

	if(fseek(f, riff_offset, SEEK_SET) < 0) {
		flac__utils_printf(stderr, 1, "ERROR, couldn't seek in file %s while fixing up %s chunk size\n", outfilename, fmt_desc);
		fclose(f);
		return false;
	}
	if(!write_it(f, aligned_data_size + (is_wave_out? 36 : 46))) {
		flac__utils_printf(stderr, 1, "ERROR, couldn't write size in file %s while fixing up %s chunk size\n", outfilename, fmt_desc);
		fclose(f);
		return false;
	}
	if(!is_wave_out) {
		if(fseek(f, frames_offset, SEEK_SET) < 0) {
			flac__utils_printf(stderr, 1, "ERROR, couldn't seek in file %s while fixing up %s chunk size\n", outfilename, fmt_desc);
			fclose(f);
			return false;
		}
		if(!write_it(f, total_samples)) {
			flac__utils_printf(stderr, 1, "ERROR, couldn't write size in file %s while fixing up %s chunk size\n", outfilename, fmt_desc);
			fclose(f);
			return false;
		}
	}
	if(fseek(f, data_offset, SEEK_SET) < 0) {
		flac__utils_printf(stderr, 1, "ERROR, couldn't seek in file %s while fixing up %s chunk size\n", outfilename, fmt_desc);
		fclose(f);
		return false;
	}
	if(!write_it(f, data_size + (is_wave_out? 0 : 8))) {
		flac__utils_printf(stderr, 1, "ERROR, couldn't write size in file %s while fixing up %s chunk size\n", outfilename, fmt_desc);
		fclose(f);
		return false;
	}
	fclose(f);
	return true;
}

FLAC__StreamDecoderWriteStatus write_callback(const void *decoder, const FLAC__Frame *frame, const FLAC__int32 * const buffer[], void *client_data)
{
	DecoderSession *decoder_session = (DecoderSession*)client_data;
	FILE *fout = decoder_session->fout;
	const unsigned bps = frame->header.bits_per_sample, channels = frame->header.channels;
	FLAC__bool is_big_endian = (decoder_session->is_aiff_out? true : (decoder_session->is_wave_out? false : decoder_session->is_big_endian));
	FLAC__bool is_unsigned_samples = (decoder_session->is_aiff_out? false : (decoder_session->is_wave_out? bps<=8 : decoder_session->is_unsigned_samples));
	unsigned wide_samples = frame->header.blocksize, wide_sample, sample, channel, byte;
	static FLAC__int8 s8buffer[FLAC__MAX_BLOCK_SIZE * FLAC__MAX_CHANNELS * sizeof(FLAC__int32)]; /* WATCHOUT: can be up to 2 megs */
	FLAC__uint8  *u8buffer  = (FLAC__uint8  *)s8buffer;
	FLAC__int16  *s16buffer = (FLAC__int16  *)s8buffer;
	FLAC__uint16 *u16buffer = (FLAC__uint16 *)s8buffer;
	FLAC__int32  *s32buffer = (FLAC__int32  *)s8buffer;
	FLAC__uint32 *u32buffer = (FLAC__uint32 *)s8buffer;
	size_t bytes_to_write = 0;

	(void)decoder;

	if(decoder_session->abort_flag)
		return FLAC__STREAM_DECODER_WRITE_STATUS_ABORT;

	if(bps != decoder_session->bps) {
		flac__utils_printf(stderr, 1, "%s: ERROR, bits-per-sample is %u in frame but %u in STREAMINFO\n", decoder_session->inbasefilename, bps, decoder_session->bps);
		return FLAC__STREAM_DECODER_WRITE_STATUS_ABORT;
	}
	if(channels != decoder_session->channels) {
		flac__utils_printf(stderr, 1, "%s: ERROR, channels is %u in frame but %u in STREAMINFO\n", decoder_session->inbasefilename, channels, decoder_session->channels);
		return FLAC__STREAM_DECODER_WRITE_STATUS_ABORT;
	}
	if(frame->header.sample_rate != decoder_session->sample_rate) {
		flac__utils_printf(stderr, 1, "%s: ERROR, sample rate is %u in frame but %u in STREAMINFO\n", decoder_session->inbasefilename, frame->header.sample_rate, decoder_session->sample_rate);
		return FLAC__STREAM_DECODER_WRITE_STATUS_ABORT;
	}

	/*
	 * limit the number of samples to accept based on --until
	 */
	FLAC__ASSERT(!decoder_session->skip_specification->is_relative);
	FLAC__ASSERT(decoder_session->skip_specification->value.samples >= 0);
	FLAC__ASSERT(!decoder_session->until_specification->is_relative);
	FLAC__ASSERT(decoder_session->until_specification->value.samples >= 0);
	if(decoder_session->until_specification->value.samples > 0) {
		const FLAC__uint64 skip = (FLAC__uint64)decoder_session->skip_specification->value.samples;
		const FLAC__uint64 until = (FLAC__uint64)decoder_session->until_specification->value.samples;
		const FLAC__uint64 input_samples_passed = skip + decoder_session->samples_processed;
		FLAC__ASSERT(until >= input_samples_passed);
		if(input_samples_passed + wide_samples > until)
			wide_samples = (unsigned)(until - input_samples_passed);
		if (wide_samples == 0) {
			decoder_session->abort_flag = true;
			decoder_session->aborting_due_to_until = true;
			return FLAC__STREAM_DECODER_WRITE_STATUS_ABORT;
		}
	}

	if(wide_samples > 0) {
		decoder_session->samples_processed += wide_samples;
		decoder_session->frame_counter++;

		if(!(decoder_session->frame_counter & 0x3f))
			print_stats(decoder_session);

		if(decoder_session->analysis_mode) {
			flac__analyze_frame(frame, decoder_session->frame_counter-1, decoder_session->aopts, fout);
		}
		else if(!decoder_session->test_only) {
			if (decoder_session->replaygain.apply) {
				bytes_to_write = FLAC__replaygain_synthesis__apply_gain(
					u8buffer,
					!is_big_endian,
					is_unsigned_samples,
					buffer,
					wide_samples,
					channels,
					bps, /* source_bps */
					bps, /* target_bps */
					decoder_session->replaygain.scale,
					decoder_session->replaygain.spec.limiter == RGSS_LIMIT__HARD, /* hard_limit */
					decoder_session->replaygain.spec.noise_shaping != NOISE_SHAPING_NONE, /* do_dithering */
					&decoder_session->replaygain.dither_context
				);
			}
			else if(bps == 8) {
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
				bytes_to_write = sample;
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
				bytes_to_write = 2 * sample;
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
				bytes_to_write = 3 * sample;
			}
			else {
				FLAC__ASSERT(0);
			}
		}
	}
	if(bytes_to_write > 0) {
		if(flac__utils_fwrite(u8buffer, 1, bytes_to_write, fout) != bytes_to_write) {
			/* if a pipe closed when writing to stdout, we let it go without an error message */
			if(errno == EPIPE && decoder_session->fout == stdout)
				decoder_session->aborting_due_to_until = true;
			decoder_session->abort_flag = true;
			return FLAC__STREAM_DECODER_WRITE_STATUS_ABORT;
		}
	}
	return FLAC__STREAM_DECODER_WRITE_STATUS_CONTINUE;
}

void metadata_callback(const void *decoder, const FLAC__StreamMetadata *metadata, void *client_data)
{
	DecoderSession *decoder_session = (DecoderSession*)client_data;
	(void)decoder;
	if(metadata->type == FLAC__METADATA_TYPE_STREAMINFO) {
		FLAC__uint64 skip, until;
		decoder_session->bps = metadata->data.stream_info.bits_per_sample;
		decoder_session->channels = metadata->data.stream_info.channels;
		decoder_session->sample_rate = metadata->data.stream_info.sample_rate;

		flac__utils_canonicalize_skip_until_specification(decoder_session->skip_specification, decoder_session->sample_rate);
		FLAC__ASSERT(decoder_session->skip_specification->value.samples >= 0);
		skip = (FLAC__uint64)decoder_session->skip_specification->value.samples;

		/* remember, metadata->data.stream_info.total_samples can be 0, meaning 'unknown' */
		if(metadata->data.stream_info.total_samples > 0 && skip >= metadata->data.stream_info.total_samples) {
			flac__utils_printf(stderr, 1, "%s: ERROR trying to --skip more samples than in stream\n", decoder_session->inbasefilename);
			decoder_session->abort_flag = true;
			return;
		}
		else if(metadata->data.stream_info.total_samples == 0 && skip > 0) {
			flac__utils_printf(stderr, 1, "%s: ERROR, can't --skip when FLAC metadata has total sample count of 0\n", decoder_session->inbasefilename);
			decoder_session->abort_flag = true;
			return;
		}
		FLAC__ASSERT(skip == 0 || 0 == decoder_session->cue_specification);
		decoder_session->total_samples = metadata->data.stream_info.total_samples - skip;

		/* note that we use metadata->data.stream_info.total_samples instead of decoder_session->total_samples */
		if(!canonicalize_until_specification(decoder_session->until_specification, decoder_session->inbasefilename, decoder_session->sample_rate, skip, metadata->data.stream_info.total_samples)) {
			decoder_session->abort_flag = true;
			return;
		}
		FLAC__ASSERT(decoder_session->until_specification->value.samples >= 0);
		until = (FLAC__uint64)decoder_session->until_specification->value.samples;

		if(until > 0) {
			FLAC__ASSERT(decoder_session->total_samples != 0);
			FLAC__ASSERT(0 == decoder_session->cue_specification);
			decoder_session->total_samples -= (metadata->data.stream_info.total_samples - until);
		}

		if(decoder_session->bps != 8 && decoder_session->bps != 16 && decoder_session->bps != 24) {
			flac__utils_printf(stderr, 1, "%s: ERROR: bits per sample is not 8/16/24\n", decoder_session->inbasefilename);
			decoder_session->abort_flag = true;
			return;
		}
	}
	else if(metadata->type == FLAC__METADATA_TYPE_CUESHEET) {
		/* remember, at this point, decoder_session->total_samples can be 0, meaning 'unknown' */
		if(decoder_session->total_samples == 0) {
			flac__utils_printf(stderr, 1, "%s: ERROR can't use --cue when FLAC metadata has total sample count of 0\n", decoder_session->inbasefilename);
			decoder_session->abort_flag = true;
			return;
		}

		flac__utils_canonicalize_cue_specification(decoder_session->cue_specification, &metadata->data.cue_sheet, decoder_session->total_samples, decoder_session->skip_specification, decoder_session->until_specification);

		FLAC__ASSERT(!decoder_session->skip_specification->is_relative);
		FLAC__ASSERT(decoder_session->skip_specification->value_is_samples);

		FLAC__ASSERT(!decoder_session->until_specification->is_relative);
		FLAC__ASSERT(decoder_session->until_specification->value_is_samples);

		FLAC__ASSERT(decoder_session->skip_specification->value.samples >= 0);
		FLAC__ASSERT(decoder_session->until_specification->value.samples >= 0);
		FLAC__ASSERT((FLAC__uint64)decoder_session->until_specification->value.samples <= decoder_session->total_samples);
		FLAC__ASSERT(decoder_session->skip_specification->value.samples <= decoder_session->until_specification->value.samples);

		decoder_session->total_samples = decoder_session->until_specification->value.samples - decoder_session->skip_specification->value.samples;
	}
	else if(metadata->type == FLAC__METADATA_TYPE_VORBIS_COMMENT) {
		if (decoder_session->replaygain.spec.apply) {
			double gain, peak;
			if (!(decoder_session->replaygain.apply = grabbag__replaygain_load_from_vorbiscomment(metadata, decoder_session->replaygain.spec.use_album_gain, /*strict=*/false, &gain, &peak))) {
				flac__utils_printf(stderr, 1, "%s: WARNING: can't get %s (or even %s) ReplayGain tags\n", decoder_session->inbasefilename, decoder_session->replaygain.spec.use_album_gain? "album":"track", decoder_session->replaygain.spec.use_album_gain? "track":"album");
			}
			else {
				const char *ls[] = { "no", "peak", "hard" };
				const char *ns[] = { "no", "low", "medium", "high" };
				decoder_session->replaygain.scale = grabbag__replaygain_compute_scale_factor(peak, gain, decoder_session->replaygain.spec.preamp, decoder_session->replaygain.spec.limiter == RGSS_LIMIT__PEAK);
				FLAC__ASSERT(decoder_session->bps > 0 && decoder_session->bps <= 32);
				FLAC__replaygain_synthesis__init_dither_context(&decoder_session->replaygain.dither_context, decoder_session->bps, decoder_session->replaygain.spec.noise_shaping);
				flac__utils_printf(stderr, 1, "%s: INFO: applying %s ReplayGain (gain=%0.2fdB+preamp=%0.1fdB, %s noise shaping, %s limiting) to output\n", decoder_session->inbasefilename, decoder_session->replaygain.spec.use_album_gain? "album":"track", gain, decoder_session->replaygain.spec.preamp, ns[decoder_session->replaygain.spec.noise_shaping], ls[decoder_session->replaygain.spec.limiter]);
				flac__utils_printf(stderr, 1, "%s: WARNING: applying ReplayGain is not lossless\n", decoder_session->inbasefilename);
			}
		}
	}
}

void error_callback(const void *decoder, FLAC__StreamDecoderErrorStatus status, void *client_data)
{
	DecoderSession *decoder_session = (DecoderSession*)client_data;
	(void)decoder;
	flac__utils_printf(stderr, 1, "%s: *** Got error code %d:%s\n", decoder_session->inbasefilename, status, FLAC__StreamDecoderErrorStatusString[status]);
	if(!decoder_session->continue_through_decode_errors)
		decoder_session->abort_flag = true;
}

void print_error_with_state(const DecoderSession *d, const char *message)
{
	const int ilen = strlen(d->inbasefilename) + 1;
	const char *state_string;

	flac__utils_printf(stderr, 1, "\n%s: %s\n", d->inbasefilename, message);

#ifdef FLAC__HAS_OGG
	if(d->is_ogg) {
		state_string = OggFLAC__file_decoder_get_resolved_state_string(d->decoder.ogg.file);
	}
	else
#endif
	{
		state_string = FLAC__file_decoder_get_resolved_state_string(d->decoder.flac.file);
	}

	flac__utils_printf(stderr, 1, "%*s state = %s\n", ilen, "", state_string);

	/* print out some more info for some errors: */
	if (0 == strcmp(state_string, FLAC__StreamDecoderStateString[FLAC__STREAM_DECODER_UNPARSEABLE_STREAM])) {
		flac__utils_printf(stderr, 1,
			"\n"
			"The FLAC stream may have been created by a more advanced encoder.  Try\n"
			"  metaflac --show-vendor-tag %s\n"
			"If the version number is greater than %s, this decoder is probably\n"
			"not able to decode the file.  If the version number is not, the file\n"
			"may be corrupted, or you may have found a bug.  In this case please\n"
			"submit a bug report to\n"
			"    http://sourceforge.net/bugs/?func=addbug&group_id=13478\n"
			"Make sure to use the \"Monitor\" feature to monitor the bug status.\n",
			d->inbasefilename, FLAC__VERSION_STRING
		);
	}
	else if (
		0 == strcmp(state_string, FLAC__FileDecoderStateString[FLAC__FILE_DECODER_ERROR_OPENING_FILE])
#ifdef FLAC__HAS_OGG
		|| 0 == strcmp(state_string, OggFLAC__FileDecoderStateString[OggFLAC__FILE_DECODER_ERROR_OPENING_FILE])
#endif
	) {
		flac__utils_printf(stderr, 1,
			"\n"
			"An error occurred opening the input file; it is likely that it does not exist\n"
			"or is not readable.\n"
		);
	}
}

void print_stats(const DecoderSession *decoder_session)
{
	if(flac__utils_verbosity_ >= 2) {
#if defined _MSC_VER || defined __MINGW32__
		/* with MSVC you have to spoon feed it the casting */
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
