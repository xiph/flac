/* flac - Command-line FLAC encoder/decoder
 * Copyright (C) 2000,2001,2002,2003  Josh Coalson
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
#include <limits.h> /* for LONG_MAX */
#include <math.h> /* for floor() */
#include <stdarg.h>
#include <stdio.h> /* for FILE etc. */
#include <stdlib.h> /* for malloc */
#include <string.h> /* for strcmp() */
#include "FLAC/all.h"
#include "share/grabbag.h"
#include "encode.h"

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#ifdef FLAC__HAS_OGG
#include "OggFLAC/stream_encoder.h"
#endif

#ifdef min
#undef min
#endif
#define min(x,y) ((x)<(y)?(x):(y))

/* this MUST be >= 588 so that sector aligning can take place with one read */
#define CHUNK_OF_SAMPLES 2048

typedef struct {
#ifdef FLAC__HAS_OGG
	FLAC__bool use_ogg;
#endif
	FLAC__bool verify;
	FLAC__bool verbose;
	FLAC__bool is_stdout;
	const char *inbasefilename;
	const char *outfilename;

	FLAC__uint64 skip;
	FLAC__uint64 until; /* a value of 0 mean end-of-stream (i.e. --until=-0) */
	FLAC__bool replay_gain;
	unsigned channels;
	unsigned bits_per_sample;
	unsigned sample_rate;
	FLAC__uint64 unencoded_size;
	FLAC__uint64 total_samples_to_encode;
	FLAC__uint64 bytes_written;
	FLAC__uint64 samples_written;
	unsigned blocksize;
	unsigned stats_mask;

	/*
	 * We use flac.stream for encoding native FLAC to stdout
	 * We use flac.file for encoding native FLAC to a regular file
	 * We use ogg.stream for encoding Ogg FLAC to either stdout or a regular file
	 */
	union {
		union {
			FLAC__StreamEncoder *stream;
			FLAC__FileEncoder *file;
		} flac;
#ifdef FLAC__HAS_OGG
		union {
			OggFLAC__StreamEncoder *stream;
		} ogg;
#endif
	} encoder;

	FILE *fin;
	FILE *fout;
	FLAC__StreamMetadata *seek_table_template;
} EncoderSession;


static FLAC__bool is_big_endian_host_;

static unsigned char ucbuffer_[CHUNK_OF_SAMPLES*FLAC__MAX_CHANNELS*((FLAC__REFERENCE_CODEC_MAX_BITS_PER_SAMPLE+7)/8)];
static signed char *scbuffer_ = (signed char *)ucbuffer_;
static FLAC__uint16 *usbuffer_ = (FLAC__uint16 *)ucbuffer_;
static FLAC__int16 *ssbuffer_ = (FLAC__int16 *)ucbuffer_;

static FLAC__int32 in_[FLAC__MAX_CHANNELS][CHUNK_OF_SAMPLES];
static FLAC__int32 *input_[FLAC__MAX_CHANNELS];


/*
 * unpublished debug routines from the FLAC libs
 */
extern FLAC__bool FLAC__stream_encoder_disable_constant_subframes(FLAC__StreamEncoder *encoder, FLAC__bool value);
extern FLAC__bool FLAC__stream_encoder_disable_fixed_subframes(FLAC__StreamEncoder *encoder, FLAC__bool value);
extern FLAC__bool FLAC__stream_encoder_disable_verbatim_subframes(FLAC__StreamEncoder *encoder, FLAC__bool value);
extern FLAC__bool FLAC__file_encoder_disable_constant_subframes(FLAC__FileEncoder *encoder, FLAC__bool value);
extern FLAC__bool FLAC__file_encoder_disable_fixed_subframes(FLAC__FileEncoder *encoder, FLAC__bool value);
extern FLAC__bool FLAC__file_encoder_disable_verbatim_subframes(FLAC__FileEncoder *encoder, FLAC__bool value);
extern FLAC__bool OggFLAC__stream_encoder_disable_constant_subframes(OggFLAC__StreamEncoder *encoder, FLAC__bool value);
extern FLAC__bool OggFLAC__stream_encoder_disable_fixed_subframes(OggFLAC__StreamEncoder *encoder, FLAC__bool value);
extern FLAC__bool OggFLAC__stream_encoder_disable_verbatim_subframes(OggFLAC__StreamEncoder *encoder, FLAC__bool value);

/*
 * local routines
 */
static FLAC__bool EncoderSession_construct(EncoderSession *e, FLAC__bool use_ogg, FLAC__bool verify, FLAC__bool verbose, FILE *infile, const char *infilename, const char *outfilename);
static void EncoderSession_destroy(EncoderSession *e);
static int EncoderSession_finish_ok(EncoderSession *e, int info_align_carry, int info_align_zero);
static int EncoderSession_finish_error(EncoderSession *e);
static FLAC__bool EncoderSession_init_encoder(EncoderSession *e, encode_options_t options, unsigned channels, unsigned bps, unsigned sample_rate);
static FLAC__bool EncoderSession_process(EncoderSession *e, const FLAC__int32 * const buffer[], unsigned samples);
static FLAC__bool convert_to_seek_table_template(const char *requested_seek_points, int num_requested_seek_points, FLAC__StreamMetadata *cuesheet, EncoderSession *e);
static FLAC__bool canonicalize_until_specification(utils__SkipUntilSpecification *spec, const char *inbasefilename, unsigned sample_rate, FLAC__uint64 skip, FLAC__uint64 total_samples_in_input);
static void format_input(FLAC__int32 *dest[], unsigned wide_samples, FLAC__bool is_big_endian, FLAC__bool is_unsigned_samples, unsigned channels, unsigned bps);
#ifdef FLAC__HAS_OGG
static FLAC__StreamEncoderWriteStatus ogg_stream_encoder_write_callback(const OggFLAC__StreamEncoder *encoder, const FLAC__byte buffer[], unsigned bytes, unsigned samples, unsigned current_frame, void *client_data);
#endif
static FLAC__StreamEncoderWriteStatus flac_stream_encoder_write_callback(const FLAC__StreamEncoder *encoder, const FLAC__byte buffer[], unsigned bytes, unsigned samples, unsigned current_frame, void *client_data);
static void flac_stream_encoder_metadata_callback(const FLAC__StreamEncoder *encoder, const FLAC__StreamMetadata *metadata, void *client_data);
static void flac_file_encoder_progress_callback(const FLAC__FileEncoder *encoder, FLAC__uint64 bytes_written, FLAC__uint64 samples_written, unsigned frames_written, unsigned total_frames_estimate, void *client_data);
static FLAC__bool parse_cuesheet_(FLAC__StreamMetadata **cuesheet, const char *cuesheet_filename, const char *inbasefilename, FLAC__uint64 lead_out_offset);
static void print_stats(const EncoderSession *encoder_session);
static void print_error_with_state(const EncoderSession *e, const char *message);
static void print_verify_error(EncoderSession *e);
static FLAC__bool read_little_endian_uint16(FILE *f, FLAC__uint16 *val, FLAC__bool eof_ok, const char *fn);
static FLAC__bool read_little_endian_uint32(FILE *f, FLAC__uint32 *val, FLAC__bool eof_ok, const char *fn);
static FLAC__bool read_big_endian_uint16(FILE *f, FLAC__uint16 *val, FLAC__bool eof_ok, const char *fn);
static FLAC__bool read_big_endian_uint32(FILE *f, FLAC__uint32 *val, FLAC__bool eof_ok, const char *fn);
static FLAC__bool read_sane_extended(FILE *f, FLAC__uint32 *val, FLAC__bool eof_ok, const char *fn);

/*
 * public routines
 */
int
flac__encode_aif(FILE *infile, long infilesize, const char *infilename, const char *outfilename,
	const FLAC__byte *lookahead, unsigned lookahead_length, wav_encode_options_t options)
{
	EncoderSession encoder_session;
	FLAC__uint16 x;
	FLAC__uint32 xx;
	unsigned int channels= 0U, bps= 0U, sample_rate= 0U, sample_frames= 0U;
	FLAC__bool got_comm_chunk= false, got_ssnd_chunk= false;
	int info_align_carry= -1, info_align_zero= -1;

	(void)infilesize; /* silence compiler warning about unused parameter */
	(void)lookahead; /* silence compiler warning about unused parameter */
	(void)lookahead_length; /* silence compiler warning about unused parameter */

	if(!
		EncoderSession_construct(
			&encoder_session,
#ifdef FLAC__HAS_OGG
			options.common.use_ogg,
#else
			/*use_ogg=*/false,
#endif
			options.common.verify,
			options.common.verbose,
			infile,
			infilename,
			outfilename
		)
	)
		return 1;

	/* lookahead[] already has "FORMxxxxAIFF", do sub-chunks */

	while(1) {
		size_t c= 0U;
		char chunk_id[4];

		/* chunk identifier; really conservative about behavior of fread() and feof() */
		if(feof(infile) || ((c= fread(chunk_id, 1U, 4U, infile)), c==0U && feof(infile)))
			break;
		else if(c<4U || feof(infile)) {
			fprintf(stderr, "%s: ERROR: incomplete chunk identifier\n", encoder_session.inbasefilename);
			return EncoderSession_finish_error(&encoder_session);
		}

		if(got_comm_chunk==false && !strncmp(chunk_id, "COMM", 4)) { /* common chunk */
			unsigned long skip;

			/* COMM chunk size */
			if(!read_big_endian_uint32(infile, &xx, false, encoder_session.inbasefilename))
				return EncoderSession_finish_error(&encoder_session);
			else if(xx<18U) {
				fprintf(stderr, "%s: ERROR: non-standard 'COMM' chunk has length = %u\n", encoder_session.inbasefilename, (unsigned int)xx);
				return EncoderSession_finish_error(&encoder_session);
			}
			else if(xx!=18U)
				fprintf(stderr, "%s: WARNING: non-standard 'COMM' chunk has length = %u\n", encoder_session.inbasefilename, (unsigned int)xx);
			skip= (xx-18U)+(xx & 1U);

			/* number of channels */
			if(!read_big_endian_uint16(infile, &x, false, encoder_session.inbasefilename))
				return EncoderSession_finish_error(&encoder_session);
			else if(x==0U || x>FLAC__MAX_CHANNELS) {
				fprintf(stderr, "%s: ERROR: unsupported number channels %u\n", encoder_session.inbasefilename, (unsigned int)x);
				return EncoderSession_finish_error(&encoder_session);
			}
			else if(options.common.sector_align && x!=2U) {
				fprintf(stderr, "%s: ERROR: file has %u channels, must be 2 for --sector-align\n", encoder_session.inbasefilename, (unsigned int)x);
				return EncoderSession_finish_error(&encoder_session);
			}
			channels= x;

			/* number of sample frames */
			if(!read_big_endian_uint32(infile, &xx, false, encoder_session.inbasefilename))
				return EncoderSession_finish_error(&encoder_session);
			sample_frames= xx;

			/* bits per sample */
			if(!read_big_endian_uint16(infile, &x, false, encoder_session.inbasefilename))
				return EncoderSession_finish_error(&encoder_session);
			else if(x!=8U && x!=16U && x!=24U) {
				fprintf(stderr, "%s: ERROR: unsupported bits per sample %u\n", encoder_session.inbasefilename, (unsigned int)x);
				return EncoderSession_finish_error(&encoder_session);
			}
			else if(options.common.sector_align && x!=16U) {
				fprintf(stderr, "%s: ERROR: file has %u bits per sample, must be 16 for --sector-align\n", encoder_session.inbasefilename, (unsigned int)x);
				return EncoderSession_finish_error(&encoder_session);
			}
			bps= x;

			/* sample rate */
			if(!read_sane_extended(infile, &xx, false, encoder_session.inbasefilename))
				return EncoderSession_finish_error(&encoder_session);
			else if(!FLAC__format_sample_rate_is_valid(xx)) {
				fprintf(stderr, "%s: ERROR: unsupported sample rate %u\n", encoder_session.inbasefilename, (unsigned int)xx);
				return EncoderSession_finish_error(&encoder_session);
			}
			else if(options.common.sector_align && xx!=44100U) {
				fprintf(stderr, "%s: ERROR: file's sample rate is %u, must be 44100 for --sector-align\n", encoder_session.inbasefilename, (unsigned int)xx);
				return EncoderSession_finish_error(&encoder_session);
			}
			sample_rate= xx;

			/* skip any extra data in the COMM chunk */
			FLAC__ASSERT(skip<=LONG_MAX);
			while(skip>0U && fseek(infile, skip, SEEK_CUR)<0) {
				unsigned int need= min(skip, sizeof ucbuffer_);
				if(fread(ucbuffer_, 1U, need, infile)<need) {
					fprintf(stderr, "%s: ERROR during read while skipping extra COMM data\n", encoder_session.inbasefilename);
					return EncoderSession_finish_error(&encoder_session);
				}
				skip-= need;
			}

			/*
			 * now that we know the sample rate, canonicalize the
			 * --skip string to a number of samples:
			 */
			flac__utils_canonicalize_skip_until_specification(&options.common.skip_specification, sample_rate);
			FLAC__ASSERT(options.common.skip_specification.value.samples >= 0);
			encoder_session.skip = (FLAC__uint64)options.common.skip_specification.value.samples;
			FLAC__ASSERT(!options.common.sector_align || encoder_session.skip == 0);

			got_comm_chunk= true;
		}
		else if(got_ssnd_chunk==false && !strncmp(chunk_id, "SSND", 4)) { /* sound data chunk */
			unsigned int offset= 0U, block_size= 0U, align_remainder= 0U, data_bytes;
			size_t bytes_per_frame= channels*(bps>>3);
			FLAC__uint64 total_samples_in_input, trim = 0;
			FLAC__bool pad= false;

			if(got_comm_chunk==false) {
				fprintf(stderr, "%s: ERROR: got 'SSND' chunk before 'COMM' chunk\n", encoder_session.inbasefilename);
				return EncoderSession_finish_error(&encoder_session);
			}

			/* SSND chunk size */
			if(!read_big_endian_uint32(infile, &xx, false, encoder_session.inbasefilename))
				return EncoderSession_finish_error(&encoder_session);
			else if(xx!=(sample_frames*bytes_per_frame + 8U)) {
				fprintf(stderr, "%s: ERROR: SSND chunk size inconsistent with sample frame count\n", encoder_session.inbasefilename);
				return EncoderSession_finish_error(&encoder_session);
			}
			data_bytes= xx;
			pad= (data_bytes & 1U) ? true : false;
			data_bytes-= 8U; /* discount the offset and block size fields */

			/* offset */
			if(!read_big_endian_uint32(infile, &xx, false, encoder_session.inbasefilename))
				return EncoderSession_finish_error(&encoder_session);
			else if(xx!=0U) {
				fprintf(stderr, "%s: ERROR: offset is %u; must be 0\n", encoder_session.inbasefilename, (unsigned int)xx);
				return EncoderSession_finish_error(&encoder_session);
			}
			offset= xx;

			/* block size */
			if(!read_big_endian_uint32(infile, &xx, false, encoder_session.inbasefilename))
				return EncoderSession_finish_error(&encoder_session);
			else if(xx!=0U) {
				fprintf(stderr, "%s: ERROR: block size is %u; must be 0\n", encoder_session.inbasefilename, (unsigned int)xx);
				return EncoderSession_finish_error(&encoder_session);
			}
			block_size= xx;

			/* *options.common.align_reservoir_samples will be 0 unless --sector-align is used */
			FLAC__ASSERT(options.common.sector_align || *options.common.align_reservoir_samples == 0);
			total_samples_in_input = data_bytes / bytes_per_frame + *options.common.align_reservoir_samples;

			/*
			 * now that we know the input size, canonicalize the
			 * --until string to an absolute sample number:
			 */
			if(!canonicalize_until_specification(&options.common.until_specification, encoder_session.inbasefilename, sample_rate, encoder_session.skip, total_samples_in_input))
				return EncoderSession_finish_error(&encoder_session);
			encoder_session.until = (FLAC__uint64)options.common.until_specification.value.samples;
			FLAC__ASSERT(!options.common.sector_align || encoder_session.until == 0);

			if(encoder_session.skip>0U) {
				FLAC__uint64 remaining= encoder_session.skip*bytes_per_frame;

				/* do 1<<30 bytes at a time, since 1<<30 is a nice round number, and */
				/* is guaranteed to be less than LONG_MAX */
				while(remaining>0U)
				{
					unsigned long skip= (unsigned long)(remaining % (1U<<30));

					FLAC__ASSERT(skip<=LONG_MAX);
					while(skip>0 && fseek(infile, skip, SEEK_CUR)<0) {
						unsigned int need= min(skip, sizeof ucbuffer_);
						if(fread(ucbuffer_, 1U, need, infile)<need) {
							fprintf(stderr, "%s: ERROR during read while skipping samples\n", encoder_session.inbasefilename);
							return EncoderSession_finish_error(&encoder_session);
						}
						skip-= need;
					}

					remaining-= skip;
				}
			}

			data_bytes-= (unsigned int)encoder_session.skip*bytes_per_frame; /*@@@ WATCHOUT: 4GB limit */
			encoder_session.total_samples_to_encode= total_samples_in_input - encoder_session.skip;
			if(encoder_session.until > 0) {
				trim = total_samples_in_input - encoder_session.until;
				FLAC__ASSERT(total_samples_in_input > 0);
				FLAC__ASSERT(!options.common.sector_align);
				data_bytes-= (unsigned int)trim*bytes_per_frame;
				encoder_session.total_samples_to_encode-= trim;
			}
			if(options.common.sector_align) {
				align_remainder= (unsigned int)(encoder_session.total_samples_to_encode % 588U);
				if(options.common.is_last_file)
					encoder_session.total_samples_to_encode+= (588U-align_remainder); /* will pad with zeroes */
				else
					encoder_session.total_samples_to_encode-= align_remainder; /* will stop short and carry over to next file */
			}

			/* +54 for the size of the AIFF headers; this is just an estimate for the progress indicator and doesn't need to be exact */
			encoder_session.unencoded_size= encoder_session.total_samples_to_encode*bytes_per_frame+54;

			if(!EncoderSession_init_encoder(&encoder_session, options.common, channels, bps, sample_rate))
				return EncoderSession_finish_error(&encoder_session);

			/* first do any samples in the reservoir */
			if(options.common.sector_align && *options.common.align_reservoir_samples>0U) {

				if(!EncoderSession_process(&encoder_session, (const FLAC__int32 *const *)options.common.align_reservoir, *options.common.align_reservoir_samples)) {
					print_error_with_state(&encoder_session, "ERROR during encoding");
					return EncoderSession_finish_error(&encoder_session);
				}
			}

			/* decrement the data_bytes counter if we need to align the file */
			if(options.common.sector_align) {
				if(options.common.is_last_file)
					*options.common.align_reservoir_samples= 0U;
				else {
					*options.common.align_reservoir_samples= align_remainder;
					data_bytes-= (*options.common.align_reservoir_samples)*bytes_per_frame;
				}
			}

			/* now do from the file */
			while(data_bytes>0) {
				size_t bytes_read= fread(ucbuffer_, 1U, min(data_bytes, CHUNK_OF_SAMPLES*bytes_per_frame), infile);

				if(bytes_read==0U) {
					if(ferror(infile)) {
						fprintf(stderr, "%s: ERROR during read\n", encoder_session.inbasefilename);
						return EncoderSession_finish_error(&encoder_session);
					}
					else if(feof(infile)) {
						fprintf(stderr, "%s: WARNING: unexpected EOF; expected %u samples, got %u samples\n", encoder_session.inbasefilename, (unsigned int)encoder_session.total_samples_to_encode, (unsigned int)encoder_session.samples_written);
						data_bytes= 0;
					}
				}
				else {
					if(bytes_read % bytes_per_frame != 0U) {
						fprintf(stderr, "%s: ERROR: got partial sample\n", encoder_session.inbasefilename);
						return EncoderSession_finish_error(&encoder_session);
					}
					else {
						unsigned int frames= bytes_read/bytes_per_frame;
						format_input(input_, frames, true, false, channels, bps);

						if(!EncoderSession_process(&encoder_session, (const FLAC__int32 *const *)input_, frames)) {
							print_error_with_state(&encoder_session, "ERROR during encoding");
							return EncoderSession_finish_error(&encoder_session);
						}
						else
							data_bytes-= bytes_read;
					}
				}
			}

			if(trim>0) {
				FLAC__uint64 remaining= (unsigned int)trim*bytes_per_frame;

				FLAC__ASSERT(!options.common.sector_align);

				/* do 1<<30 bytes at a time, since 1<<30 is a nice round number, and */
				/* is guaranteed to be less than LONG_MAX */
				while(remaining>0U)
				{
					unsigned long skip= (unsigned long)(remaining % (1U<<30));

					FLAC__ASSERT(skip<=LONG_MAX);
					while(skip>0 && fseek(infile, skip, SEEK_CUR)<0) {
						unsigned int need= min(skip, sizeof ucbuffer_);
						if(fread(ucbuffer_, 1U, need, infile)<need) {
							fprintf(stderr, "%s: ERROR during read while skipping samples\n", encoder_session.inbasefilename);
							return EncoderSession_finish_error(&encoder_session);
						}
						skip-= need;
					}

					remaining-= skip;
				}
			}

			/* now read unaligned samples into reservoir or pad with zeroes if necessary */
			if(options.common.sector_align) {
				if(options.common.is_last_file) {
					unsigned int pad_frames= 588U-align_remainder;

					if(pad_frames<588U) {
						unsigned int i;

						info_align_zero= pad_frames;
						for(i= 0U; i<channels; ++i)
							memset(input_[i], 0, pad_frames*(bps>>3));

						if(!EncoderSession_process(&encoder_session, (const FLAC__int32 *const *)input_, pad_frames)) {
							print_error_with_state(&encoder_session, "ERROR during encoding");
							return EncoderSession_finish_error(&encoder_session);
						}
					}
				}
				else {
					if(*options.common.align_reservoir_samples > 0) {
						size_t bytes_read= fread(ucbuffer_, 1U, (*options.common.align_reservoir_samples)*bytes_per_frame, infile);

						FLAC__ASSERT(CHUNK_OF_SAMPLES>=588U);
						if(bytes_read==0U && ferror(infile)) {
							fprintf(stderr, "%s: ERROR during read\n", encoder_session.inbasefilename);
							return EncoderSession_finish_error(&encoder_session);
						}
						else if(bytes_read != (*options.common.align_reservoir_samples) * bytes_per_frame)
							fprintf(stderr, "%s: WARNING: unexpected EOF; read %u bytes; expected %u samples, got %u samples\n", encoder_session.inbasefilename, (unsigned int)bytes_read, (unsigned int)encoder_session.total_samples_to_encode, (unsigned int)encoder_session.samples_written);
						else {
							info_align_carry= *options.common.align_reservoir_samples;
							format_input(options.common.align_reservoir, *options.common.align_reservoir_samples, true, false, channels, bps);
						}
					}
				}
			}

			if(pad==true) {
				unsigned char tmp;

				if(fread(&tmp, 1U, 1U, infile)<1U) {
					fprintf(stderr, "%s: ERROR during read of SSND pad byte\n", encoder_session.inbasefilename);
					return EncoderSession_finish_error(&encoder_session);
				}
			}

			got_ssnd_chunk= true;
		}
		else { /* other chunk */
			if(!strncmp(chunk_id, "COMM", 4))
				fprintf(stderr, "%s: WARNING: skipping extra 'COMM' chunk\n", encoder_session.inbasefilename);
			else if(!strncmp(chunk_id, "SSND", 4))
				fprintf(stderr, "%s: WARNING: skipping extra 'SSND' chunk\n", encoder_session.inbasefilename);
			else
				fprintf(stderr, "%s: WARNING: skipping unknown chunk '%s'\n", encoder_session.inbasefilename, chunk_id);

			/* chunk size */
			if(!read_big_endian_uint32(infile, &xx, false, encoder_session.inbasefilename))
				return EncoderSession_finish_error(&encoder_session);
			else {
				unsigned long skip= xx+(xx & 1U);

				FLAC__ASSERT(skip<=LONG_MAX);
				while(skip>0U && fseek(infile, skip, SEEK_CUR)<0) {
					unsigned int need= min(skip, sizeof ucbuffer_);
					if(fread(ucbuffer_, 1U, need, infile)<need) {
						fprintf(stderr, "%s: ERROR during read while skipping unknown chunk\n", encoder_session.inbasefilename);
						return EncoderSession_finish_error(&encoder_session);
					}
					skip-= need;
				}
			}
		}
	}

	if(got_ssnd_chunk==false && sample_frames!=0U) {
		fprintf(stderr, "%s: ERROR: missing SSND chunk\n", encoder_session.inbasefilename);
		return EncoderSession_finish_error(&encoder_session);
	}

	return EncoderSession_finish_ok(&encoder_session, info_align_carry, info_align_zero);
}

int flac__encode_wav(FILE *infile, long infilesize, const char *infilename, const char *outfilename, const FLAC__byte *lookahead, unsigned lookahead_length, wav_encode_options_t options)
{
	EncoderSession encoder_session;
	FLAC__bool is_unsigned_samples = false;
	unsigned channels = 0, bps = 0, sample_rate = 0, data_bytes;
	size_t bytes_per_wide_sample, bytes_read;
	FLAC__uint16 x;
	FLAC__uint32 xx;
	FLAC__bool got_fmt_chunk = false, got_data_chunk = false;
	unsigned align_remainder = 0;
	int info_align_carry = -1, info_align_zero = -1;

	(void)infilesize;
	(void)lookahead;
	(void)lookahead_length;

	if(!
		EncoderSession_construct(
			&encoder_session,
#ifdef FLAC__HAS_OGG
			options.common.use_ogg,
#else
			/*use_ogg=*/false,
#endif
			options.common.verify,
			options.common.verbose,
			infile,
			infilename,
			outfilename
		)
	)
		return 1;

	/*
	 * lookahead[] already has "RIFFxxxxWAVE", do sub-chunks
	 */
	while(!feof(infile)) {
		if(!read_little_endian_uint32(infile, &xx, true, encoder_session.inbasefilename))
			return EncoderSession_finish_error(&encoder_session);
		if(feof(infile))
			break;
		if(xx == 0x20746d66 && !got_fmt_chunk) { /* "fmt " */
			/* fmt sub-chunk size */
			if(!read_little_endian_uint32(infile, &xx, false, encoder_session.inbasefilename))
				return EncoderSession_finish_error(&encoder_session);
			if(xx < 16) {
				fprintf(stderr, "%s: ERROR: found non-standard 'fmt ' sub-chunk which has length = %u\n", encoder_session.inbasefilename, (unsigned)xx);
				return EncoderSession_finish_error(&encoder_session);
			}
			else if(xx != 16 && xx != 18) {
				fprintf(stderr, "%s: WARNING: found non-standard 'fmt ' sub-chunk which has length = %u\n", encoder_session.inbasefilename, (unsigned)xx);
			}
			data_bytes = xx;
			/* compression code */
			if(!read_little_endian_uint16(infile, &x, false, encoder_session.inbasefilename))
				return EncoderSession_finish_error(&encoder_session);
			if(x != 1) {
				fprintf(stderr, "%s: ERROR: unsupported compression type %u\n", encoder_session.inbasefilename, (unsigned)x);
				return EncoderSession_finish_error(&encoder_session);
			}
			/* number of channels */
			if(!read_little_endian_uint16(infile, &x, false, encoder_session.inbasefilename))
				return EncoderSession_finish_error(&encoder_session);
			if(x == 0 || x > FLAC__MAX_CHANNELS) {
				fprintf(stderr, "%s: ERROR: unsupported number channels %u\n", encoder_session.inbasefilename, (unsigned)x);
				return EncoderSession_finish_error(&encoder_session);
			}
			else if(options.common.sector_align && x != 2) {
				fprintf(stderr, "%s: ERROR: file has %u channels, must be 2 for --sector-align\n", encoder_session.inbasefilename, (unsigned)x);
				return EncoderSession_finish_error(&encoder_session);
			}
			channels = x;
			/* sample rate */
			if(!read_little_endian_uint32(infile, &xx, false, encoder_session.inbasefilename))
				return EncoderSession_finish_error(&encoder_session);
			if(!FLAC__format_sample_rate_is_valid(xx)) {
				fprintf(stderr, "%s: ERROR: unsupported sample rate %u\n", encoder_session.inbasefilename, (unsigned)xx);
				return EncoderSession_finish_error(&encoder_session);
			}
			else if(options.common.sector_align && xx != 44100) {
				fprintf(stderr, "%s: ERROR: file's sample rate is %u, must be 44100 for --sector-align\n", encoder_session.inbasefilename, (unsigned)xx);
				return EncoderSession_finish_error(&encoder_session);
			}
			sample_rate = xx;
			/* avg bytes per second (ignored) */
			if(!read_little_endian_uint32(infile, &xx, false, encoder_session.inbasefilename))
				return EncoderSession_finish_error(&encoder_session);
			/* block align (ignored) */
			if(!read_little_endian_uint16(infile, &x, false, encoder_session.inbasefilename))
				return EncoderSession_finish_error(&encoder_session);
			/* bits per sample */
			if(!read_little_endian_uint16(infile, &x, false, encoder_session.inbasefilename))
				return EncoderSession_finish_error(&encoder_session);
			if(x != 8 && x != 16 && x != 24) {
				fprintf(stderr, "%s: ERROR: unsupported bits per sample %u\n", encoder_session.inbasefilename, (unsigned)x);
				return EncoderSession_finish_error(&encoder_session);
			}
			else if(options.common.sector_align && x != 16) {
				fprintf(stderr, "%s: ERROR: file has %u bits per sample, must be 16 for --sector-align\n", encoder_session.inbasefilename, (unsigned)x);
				return EncoderSession_finish_error(&encoder_session);
			}
			bps = x;
			is_unsigned_samples = (x == 8);

			/* skip any extra data in the fmt sub-chunk */
			data_bytes -= 16;
			if(data_bytes > 0) {
				unsigned left, need;
				for(left = data_bytes; left > 0; ) {
					need = min(left, CHUNK_OF_SAMPLES);
					if(fread(ucbuffer_, 1U, need, infile) < need) {
						fprintf(stderr, "%s: ERROR during read while skipping samples\n", encoder_session.inbasefilename);
						return EncoderSession_finish_error(&encoder_session);
					}
					left -= need;
				}
			}

			/*
			 * now that we know the sample rate, canonicalize the
			 * --skip string to a number of samples:
			 */
			flac__utils_canonicalize_skip_until_specification(&options.common.skip_specification, sample_rate);
			FLAC__ASSERT(options.common.skip_specification.value.samples >= 0);
			encoder_session.skip = (FLAC__uint64)options.common.skip_specification.value.samples;
			FLAC__ASSERT(!options.common.sector_align || encoder_session.skip == 0);

			got_fmt_chunk = true;
		}
		else if(xx == 0x61746164 && !got_data_chunk && got_fmt_chunk) { /* "data" */
			FLAC__uint64 total_samples_in_input, trim = 0;

			/* data size */
			if(!read_little_endian_uint32(infile, &xx, false, encoder_session.inbasefilename))
				return EncoderSession_finish_error(&encoder_session);
			data_bytes = xx;

			bytes_per_wide_sample = channels * (bps >> 3);

			/* *options.common.align_reservoir_samples will be 0 unless --sector-align is used */
			FLAC__ASSERT(options.common.sector_align || *options.common.align_reservoir_samples == 0);
			total_samples_in_input = data_bytes / bytes_per_wide_sample + *options.common.align_reservoir_samples;

			/*
			 * now that we know the input size, canonicalize the
			 * --until string to an absolute sample number:
			 */
			if(!canonicalize_until_specification(&options.common.until_specification, encoder_session.inbasefilename, sample_rate, encoder_session.skip, total_samples_in_input))
				return EncoderSession_finish_error(&encoder_session);
			encoder_session.until = (FLAC__uint64)options.common.until_specification.value.samples;
			FLAC__ASSERT(!options.common.sector_align || encoder_session.until == 0);

			if(encoder_session.skip > 0) {
				if(fseek(infile, bytes_per_wide_sample * (unsigned)encoder_session.skip, SEEK_CUR) < 0) {
					/* can't seek input, read ahead manually... */
					unsigned left, need;
					for(left = (unsigned)encoder_session.skip; left > 0; ) { /*@@@ WATCHOUT: 4GB limit */
						need = min(left, CHUNK_OF_SAMPLES);
						if(fread(ucbuffer_, bytes_per_wide_sample, need, infile) < need) {
							fprintf(stderr, "%s: ERROR during read while skipping samples\n", encoder_session.inbasefilename);
							return EncoderSession_finish_error(&encoder_session);
						}
						left -= need;
					}
				}
			}

			data_bytes -= (unsigned)encoder_session.skip * bytes_per_wide_sample; /*@@@ WATCHOUT: 4GB limit */
			encoder_session.total_samples_to_encode = total_samples_in_input - encoder_session.skip;
			if(encoder_session.until > 0) {
				trim = total_samples_in_input - encoder_session.until;
				FLAC__ASSERT(total_samples_in_input > 0);
				FLAC__ASSERT(!options.common.sector_align);
				data_bytes -= (unsigned int)trim * bytes_per_wide_sample;
				encoder_session.total_samples_to_encode -= trim;
			}
			if(options.common.sector_align) {
				align_remainder = (unsigned)(encoder_session.total_samples_to_encode % 588);
				if(options.common.is_last_file)
					encoder_session.total_samples_to_encode += (588-align_remainder); /* will pad with zeroes */
				else
					encoder_session.total_samples_to_encode -= align_remainder; /* will stop short and carry over to next file */
			}

			/* +44 for the size of the WAV headers; this is just an estimate for the progress indicator and doesn't need to be exact */
			encoder_session.unencoded_size = encoder_session.total_samples_to_encode * bytes_per_wide_sample + 44;

			if(!EncoderSession_init_encoder(&encoder_session, options.common, channels, bps, sample_rate))
				return EncoderSession_finish_error(&encoder_session);

			/*
			 * first do any samples in the reservoir
			 */
			if(options.common.sector_align && *options.common.align_reservoir_samples > 0) {
				if(!EncoderSession_process(&encoder_session, (const FLAC__int32 * const *)options.common.align_reservoir, *options.common.align_reservoir_samples)) {
					print_error_with_state(&encoder_session, "ERROR during encoding");
					return EncoderSession_finish_error(&encoder_session);
				}
			}

			/*
			 * decrement the data_bytes counter if we need to align the file
			 */
			if(options.common.sector_align) {
				if(options.common.is_last_file) {
					*options.common.align_reservoir_samples = 0;
				}
				else {
					*options.common.align_reservoir_samples = align_remainder;
					data_bytes -= (*options.common.align_reservoir_samples) * bytes_per_wide_sample;
				}
			}

			/*
			 * now do from the file
			 */
			while(data_bytes > 0) {
				bytes_read = fread(ucbuffer_, sizeof(unsigned char), min(data_bytes, CHUNK_OF_SAMPLES * bytes_per_wide_sample), infile);
				if(bytes_read == 0) {
					if(ferror(infile)) {
						fprintf(stderr, "%s: ERROR during read\n", encoder_session.inbasefilename);
						return EncoderSession_finish_error(&encoder_session);
					}
					else if(feof(infile)) {
						fprintf(stderr, "%s: WARNING: unexpected EOF; expected %u samples, got %u samples\n", encoder_session.inbasefilename, (unsigned)encoder_session.total_samples_to_encode, (unsigned)encoder_session.samples_written);
						data_bytes = 0;
					}
				}
				else {
					if(bytes_read % bytes_per_wide_sample != 0) {
						fprintf(stderr, "%s: ERROR: got partial sample\n", encoder_session.inbasefilename);
						return EncoderSession_finish_error(&encoder_session);
					}
					else {
						unsigned wide_samples = bytes_read / bytes_per_wide_sample;
						format_input(input_, wide_samples, false, is_unsigned_samples, channels, bps);

						if(!EncoderSession_process(&encoder_session, (const FLAC__int32 * const *)input_, wide_samples)) {
							print_error_with_state(&encoder_session, "ERROR during encoding");
							return EncoderSession_finish_error(&encoder_session);
						}
						data_bytes -= bytes_read;
					}
				}
			}

			if(trim > 0) {
				if(fseek(infile, bytes_per_wide_sample * (unsigned)trim, SEEK_CUR) < 0) {
					/* can't seek input, read ahead manually... */
					unsigned left, need;
					for(left = (unsigned)trim; left > 0; ) { /*@@@ WATCHOUT: 4GB limit */
						need = min(left, CHUNK_OF_SAMPLES);
						if(fread(ucbuffer_, bytes_per_wide_sample, need, infile) < need) {
							fprintf(stderr, "%s: ERROR during read while skipping samples\n", encoder_session.inbasefilename);
							return EncoderSession_finish_error(&encoder_session);
						}
						left -= need;
					}
				}
			}

			/*
			 * now read unaligned samples into reservoir or pad with zeroes if necessary
			 */
			if(options.common.sector_align) {
				if(options.common.is_last_file) {
					unsigned wide_samples = 588 - align_remainder;
					if(wide_samples < 588) {
						unsigned channel;

						info_align_zero = wide_samples;
						data_bytes = wide_samples * (bps >> 3);
						for(channel = 0; channel < channels; channel++)
							memset(input_[channel], 0, data_bytes);

						if(!EncoderSession_process(&encoder_session, (const FLAC__int32 * const *)input_, wide_samples)) {
							print_error_with_state(&encoder_session, "ERROR during encoding");
							return EncoderSession_finish_error(&encoder_session);
						}
					}
				}
				else {
					if(*options.common.align_reservoir_samples > 0) {
						FLAC__ASSERT(CHUNK_OF_SAMPLES >= 588);
						bytes_read = fread(ucbuffer_, sizeof(unsigned char), (*options.common.align_reservoir_samples) * bytes_per_wide_sample, infile);
						if(bytes_read == 0 && ferror(infile)) {
							fprintf(stderr, "%s: ERROR during read\n", encoder_session.inbasefilename);
							return EncoderSession_finish_error(&encoder_session);
						}
						else if(bytes_read != (*options.common.align_reservoir_samples) * bytes_per_wide_sample) {
							fprintf(stderr, "%s: WARNING: unexpected EOF; read %u bytes; expected %u samples, got %u samples\n", encoder_session.inbasefilename, (unsigned)bytes_read, (unsigned)encoder_session.total_samples_to_encode, (unsigned)encoder_session.samples_written);
							data_bytes = 0;
						}
						else {
							info_align_carry = *options.common.align_reservoir_samples;
							format_input(options.common.align_reservoir, *options.common.align_reservoir_samples, false, is_unsigned_samples, channels, bps);
						}
					}
				}
			}

			got_data_chunk = true;
		}
		else {
			if(xx == 0x20746d66 && got_fmt_chunk) { /* "fmt " */
				fprintf(stderr, "%s: WARNING: skipping extra 'fmt ' sub-chunk\n", encoder_session.inbasefilename);
			}
			else if(xx == 0x61746164) { /* "data" */
				if(got_data_chunk) {
					fprintf(stderr, "%s: WARNING: skipping extra 'data' sub-chunk\n", encoder_session.inbasefilename);
				}
				else if(!got_fmt_chunk) {
					fprintf(stderr, "%s: ERROR: got 'data' sub-chunk before 'fmt' sub-chunk\n", encoder_session.inbasefilename);
					return EncoderSession_finish_error(&encoder_session);
				}
				else {
					FLAC__ASSERT(0);
				}
			}
			else {
				fprintf(stderr, "%s: WARNING: skipping unknown sub-chunk '%c%c%c%c'\n", encoder_session.inbasefilename, (char)(xx&255), (char)((xx>>8)&255), (char)((xx>>16)&255), (char)(xx>>24));
			}
			/* sub-chunk size */
			if(!read_little_endian_uint32(infile, &xx, false, encoder_session.inbasefilename))
				return EncoderSession_finish_error(&encoder_session);
			if(fseek(infile, xx, SEEK_CUR) < 0) {
				/* can't seek input, read ahead manually... */
				unsigned left, need;
				const unsigned chunk = sizeof(ucbuffer_);
				for(left = xx; left > 0; ) {
					need = min(left, chunk);
					if(fread(ucbuffer_, 1, need, infile) < need) {
						fprintf(stderr, "%s: ERROR during read while skipping unsupported sub-chunk\n", encoder_session.inbasefilename);
						return EncoderSession_finish_error(&encoder_session);
					}
					left -= need;
				}
			}
		}
	}

	return EncoderSession_finish_ok(&encoder_session, info_align_carry, info_align_zero);
}

int flac__encode_raw(FILE *infile, long infilesize, const char *infilename, const char *outfilename, const FLAC__byte *lookahead, unsigned lookahead_length, raw_encode_options_t options)
{
	EncoderSession encoder_session;
	size_t bytes_read;
	const size_t bytes_per_wide_sample = options.channels * (options.bps >> 3);
	unsigned align_remainder = 0;
	int info_align_carry = -1, info_align_zero = -1;
	FLAC__uint64 total_samples_in_input = 0;;

	FLAC__ASSERT(!options.common.sector_align || options.channels == 2);
	FLAC__ASSERT(!options.common.sector_align || options.bps == 16);
	FLAC__ASSERT(!options.common.sector_align || options.sample_rate == 44100);
	FLAC__ASSERT(!options.common.sector_align || infilesize >= 0);
	FLAC__ASSERT(!options.common.replay_gain || options.channels <= 2);
	FLAC__ASSERT(!options.common.replay_gain || grabbag__replaygain_is_valid_sample_frequency(options.sample_rate));

	if(!
		EncoderSession_construct(
			&encoder_session,
#ifdef FLAC__HAS_OGG
			options.common.use_ogg,
#else
			/*use_ogg=*/false,
#endif
			options.common.verify,
			options.common.verbose,
			infile,
			infilename,
			outfilename
		)
	)
		return 1;

	/*
	 * now that we know the sample rate, canonicalize the
	 * --skip string to a number of samples:
	 */
	flac__utils_canonicalize_skip_until_specification(&options.common.skip_specification, options.sample_rate);
	FLAC__ASSERT(options.common.skip_specification.value.samples >= 0);
	encoder_session.skip = (FLAC__uint64)options.common.skip_specification.value.samples;
	FLAC__ASSERT(!options.common.sector_align || encoder_session.skip == 0);

	if(infilesize < 0)
		total_samples_in_input = 0;
	else {
		/* *options.common.align_reservoir_samples will be 0 unless --sector-align is used */
		FLAC__ASSERT(options.common.sector_align || *options.common.align_reservoir_samples == 0);
		total_samples_in_input = (unsigned)infilesize / bytes_per_wide_sample + *options.common.align_reservoir_samples;
	}

	/*
	 * now that we know the input size, canonicalize the
	 * --until strings to a number of samples:
	 */
	if(!canonicalize_until_specification(&options.common.until_specification, encoder_session.inbasefilename, options.sample_rate, encoder_session.skip, total_samples_in_input))
		return EncoderSession_finish_error(&encoder_session);
	encoder_session.until = (FLAC__uint64)options.common.until_specification.value.samples;
	FLAC__ASSERT(!options.common.sector_align || encoder_session.until == 0);

	encoder_session.total_samples_to_encode = total_samples_in_input - encoder_session.skip;
	if(encoder_session.until > 0) {
		const FLAC__uint64 trim = total_samples_in_input - encoder_session.until;
		FLAC__ASSERT(total_samples_in_input > 0);
		FLAC__ASSERT(!options.common.sector_align);
		encoder_session.total_samples_to_encode -= trim;
	}
	if(infilesize >= 0 && options.common.sector_align) {
		FLAC__ASSERT(encoder_session.skip == 0);
		align_remainder = (unsigned)(encoder_session.total_samples_to_encode % 588);
		if(options.common.is_last_file)
			encoder_session.total_samples_to_encode += (588-align_remainder); /* will pad with zeroes */
		else
			encoder_session.total_samples_to_encode -= align_remainder; /* will stop short and carry over to next file */
	}
	encoder_session.unencoded_size = encoder_session.total_samples_to_encode * bytes_per_wide_sample;

	if(encoder_session.verbose && encoder_session.total_samples_to_encode <= 0)
		fprintf(stderr, "(No runtime statistics possible; please wait for encoding to finish...)\n");

	if(encoder_session.skip > 0) {
		unsigned skip_bytes = bytes_per_wide_sample * (unsigned)encoder_session.skip;
		if(skip_bytes > lookahead_length) {
			skip_bytes -= lookahead_length;
			lookahead_length = 0;
			if(fseek(infile, (long)skip_bytes, SEEK_CUR) < 0) {
				/* can't seek input, read ahead manually... */
				unsigned left, need;
				const unsigned chunk = sizeof(ucbuffer_);
				for(left = skip_bytes; left > 0; ) {
					need = min(left, chunk);
					if(fread(ucbuffer_, 1, need, infile) < need) {
						fprintf(stderr, "%s: ERROR during read while skipping samples\n", encoder_session.inbasefilename);
						return EncoderSession_finish_error(&encoder_session);
					}
					left -= need;
				}
			}
		}
		else {
			lookahead += skip_bytes;
			lookahead_length -= skip_bytes;
		}
	}

	if(!EncoderSession_init_encoder(&encoder_session, options.common, options.channels, options.bps, options.sample_rate))
		return EncoderSession_finish_error(&encoder_session);

	/*
	 * first do any samples in the reservoir
	 */
	if(options.common.sector_align && *options.common.align_reservoir_samples > 0) {
		if(!EncoderSession_process(&encoder_session, (const FLAC__int32 * const *)options.common.align_reservoir, *options.common.align_reservoir_samples)) {
			print_error_with_state(&encoder_session, "ERROR during encoding");
			return EncoderSession_finish_error(&encoder_session);
		}
	}

	/*
	 * decrement infilesize if we need to align the file
	 */
	if(options.common.sector_align) {
		FLAC__ASSERT(infilesize >= 0);
		if(options.common.is_last_file) {
			*options.common.align_reservoir_samples = 0;
		}
		else {
			*options.common.align_reservoir_samples = align_remainder;
			infilesize -= (long)((*options.common.align_reservoir_samples) * bytes_per_wide_sample);
			FLAC__ASSERT(infilesize >= 0);
		}
	}

	/*
	 * now do from the file
	 */
	if(infilesize < 0) {
		while(!feof(infile)) {
			if(lookahead_length > 0) {
				FLAC__ASSERT(lookahead_length < CHUNK_OF_SAMPLES * bytes_per_wide_sample);
				memcpy(ucbuffer_, lookahead, lookahead_length);
				bytes_read = fread(ucbuffer_+lookahead_length, sizeof(unsigned char), CHUNK_OF_SAMPLES * bytes_per_wide_sample - lookahead_length, infile) + lookahead_length;
				if(ferror(infile)) {
					fprintf(stderr, "%s: ERROR during read\n", encoder_session.inbasefilename);
					return EncoderSession_finish_error(&encoder_session);
				}
				lookahead_length = 0;
			}
			else
				bytes_read = fread(ucbuffer_, sizeof(unsigned char), CHUNK_OF_SAMPLES * bytes_per_wide_sample, infile);

			if(bytes_read == 0) {
				if(ferror(infile)) {
					fprintf(stderr, "%s: ERROR during read\n", encoder_session.inbasefilename);
					return EncoderSession_finish_error(&encoder_session);
				}
			}
			else if(bytes_read % bytes_per_wide_sample != 0) {
				fprintf(stderr, "%s: ERROR: got partial sample\n", encoder_session.inbasefilename);
				return EncoderSession_finish_error(&encoder_session);
			}
			else {
				unsigned wide_samples = bytes_read / bytes_per_wide_sample;
				format_input(input_, wide_samples, options.is_big_endian, options.is_unsigned_samples, options.channels, options.bps);

				if(!EncoderSession_process(&encoder_session, (const FLAC__int32 * const *)input_, wide_samples)) {
					print_error_with_state(&encoder_session, "ERROR during encoding");
					return EncoderSession_finish_error(&encoder_session);
				}
			}
		}
	}
	else {
		const FLAC__uint64 max_input_bytes = encoder_session.total_samples_to_encode * bytes_per_wide_sample;
		FLAC__uint64 total_input_bytes_read = 0;
		while(total_input_bytes_read < max_input_bytes) {
			size_t wanted = (CHUNK_OF_SAMPLES * bytes_per_wide_sample);
			wanted = min(wanted, (size_t)(max_input_bytes - total_input_bytes_read));

			if(lookahead_length > 0) {
				FLAC__ASSERT(lookahead_length < wanted);
				memcpy(ucbuffer_, lookahead, lookahead_length);
				bytes_read = fread(ucbuffer_+lookahead_length, sizeof(unsigned char), wanted - lookahead_length, infile) + lookahead_length;
				if(ferror(infile)) {
					fprintf(stderr, "%s: ERROR during read\n", encoder_session.inbasefilename);
					return EncoderSession_finish_error(&encoder_session);
				}
				lookahead_length = 0;
			}
			else
				bytes_read = fread(ucbuffer_, sizeof(unsigned char), wanted, infile);

			if(bytes_read == 0) {
				if(ferror(infile)) {
					fprintf(stderr, "%s: ERROR during read\n", encoder_session.inbasefilename);
					return EncoderSession_finish_error(&encoder_session);
				}
				else if(feof(infile)) {
					fprintf(stderr, "%s: WARNING: unexpected EOF; expected %u samples, got %u samples\n", encoder_session.inbasefilename, (unsigned)encoder_session.total_samples_to_encode, (unsigned)encoder_session.samples_written);
					total_input_bytes_read = max_input_bytes;
				}
			}
			else {
				if(bytes_read % bytes_per_wide_sample != 0) {
					fprintf(stderr, "%s: ERROR: got partial sample\n", encoder_session.inbasefilename);
					return EncoderSession_finish_error(&encoder_session);
				}
				else {
					unsigned wide_samples = bytes_read / bytes_per_wide_sample;
					format_input(input_, wide_samples, options.is_big_endian, options.is_unsigned_samples, options.channels, options.bps);

					if(!EncoderSession_process(&encoder_session, (const FLAC__int32 * const *)input_, wide_samples)) {
						print_error_with_state(&encoder_session, "ERROR during encoding");
						return EncoderSession_finish_error(&encoder_session);
					}
					total_input_bytes_read += bytes_read;
				}
			}
		}
	}

	/*
	 * now read unaligned samples into reservoir or pad with zeroes if necessary
	 */
	if(options.common.sector_align) {
		if(options.common.is_last_file) {
			unsigned wide_samples = 588 - align_remainder;
			if(wide_samples < 588) {
				unsigned channel, data_bytes;

				info_align_zero = wide_samples;
				data_bytes = wide_samples * (options.bps >> 3);
				for(channel = 0; channel < options.channels; channel++)
					memset(input_[channel], 0, data_bytes);

				if(!EncoderSession_process(&encoder_session, (const FLAC__int32 * const *)input_, wide_samples)) {
					print_error_with_state(&encoder_session, "ERROR during encoding");
					return EncoderSession_finish_error(&encoder_session);
				}
			}
		}
		else {
			if(*options.common.align_reservoir_samples > 0) {
				FLAC__ASSERT(CHUNK_OF_SAMPLES >= 588);
				bytes_read = fread(ucbuffer_, sizeof(unsigned char), (*options.common.align_reservoir_samples) * bytes_per_wide_sample, infile);
				if(bytes_read == 0 && ferror(infile)) {
					fprintf(stderr, "%s: ERROR during read\n", encoder_session.inbasefilename);
					return EncoderSession_finish_error(&encoder_session);
				}
				else if(bytes_read != (*options.common.align_reservoir_samples) * bytes_per_wide_sample) {
					fprintf(stderr, "%s: WARNING: unexpected EOF; read %u bytes; expected %u samples, got %u samples\n", encoder_session.inbasefilename, (unsigned)bytes_read, (unsigned)encoder_session.total_samples_to_encode, (unsigned)encoder_session.samples_written);
				}
				else {
					info_align_carry = *options.common.align_reservoir_samples;
					format_input(options.common.align_reservoir, *options.common.align_reservoir_samples, false, options.is_unsigned_samples, options.channels, options.bps);
				}
			}
		}
	}

	return EncoderSession_finish_ok(&encoder_session, info_align_carry, info_align_zero);
}

FLAC__bool EncoderSession_construct(EncoderSession *e, FLAC__bool use_ogg, FLAC__bool verify, FLAC__bool verbose, FILE *infile, const char *infilename, const char *outfilename)
{
	unsigned i;
	FLAC__uint32 test = 1;

	/*
	 * initialize globals
	 */

	is_big_endian_host_ = (*((FLAC__byte*)(&test)))? false : true;

	for(i = 0; i < FLAC__MAX_CHANNELS; i++)
		input_[i] = &(in_[i][0]);


	/*
	 * initialize instance
	 */

#ifdef FLAC__HAS_OGG
	e->use_ogg = use_ogg;
#else
	(void)use_ogg;
#endif
	e->verify = verify;
	e->verbose = verbose;

	e->is_stdout = (0 == strcmp(outfilename, "-"));

	e->inbasefilename = grabbag__file_get_basename(infilename);
	e->outfilename = outfilename;

	e->skip = 0; /* filled in later after the sample_rate is known */
	e->unencoded_size = 0;
	e->total_samples_to_encode = 0;
	e->bytes_written = 0;
	e->samples_written = 0;
	e->blocksize = 0;
	e->stats_mask = 0;

	e->encoder.flac.stream = 0;
	e->encoder.flac.file = 0;
#ifdef FLAC__HAS_OGG
	e->encoder.ogg.stream = 0;
#endif

	e->fin = infile;
	e->fout = 0;
	e->seek_table_template = 0;

	if(e->is_stdout) {
		e->fout = grabbag__file_get_binary_stdout();
	}
#ifdef FLAC__HAS_OGG
	else {
		if(e->use_ogg) {
			if(0 == (e->fout = fopen(outfilename, "wb"))) {
				fprintf(stderr, "%s: ERROR: can't open output file %s\n", e->inbasefilename, outfilename);
				EncoderSession_destroy(e);
				return false;
			}
		}
	}
#endif

	if(0 == (e->seek_table_template = FLAC__metadata_object_new(FLAC__METADATA_TYPE_SEEKTABLE))) {
		fprintf(stderr, "%s: ERROR allocating memory for seek table\n", e->inbasefilename);
		return false;
	}

#ifdef FLAC__HAS_OGG
	if(e->use_ogg) {
		e->encoder.ogg.stream = OggFLAC__stream_encoder_new();
		if(0 == e->encoder.ogg.stream) {
			fprintf(stderr, "%s: ERROR creating the encoder instance\n", e->inbasefilename);
			EncoderSession_destroy(e);
			return false;
		}
	}
	else
#endif
	if(e->is_stdout) {
		e->encoder.flac.stream = FLAC__stream_encoder_new();
		if(0 == e->encoder.flac.stream) {
			fprintf(stderr, "%s: ERROR creating the encoder instance\n", e->inbasefilename);
			EncoderSession_destroy(e);
			return false;
		}
	}
	else {
		e->encoder.flac.file = FLAC__file_encoder_new();
		if(0 == e->encoder.flac.file) {
			fprintf(stderr, "%s: ERROR creating the encoder instance\n", e->inbasefilename);
			EncoderSession_destroy(e);
			return false;
		}
	}

	return true;
}

void EncoderSession_destroy(EncoderSession *e)
{
	if(e->fin != stdin)
		fclose(e->fin);
	if(0 != e->fout && e->fout != stdout)
		fclose(e->fout);

#ifdef FLAC__HAS_OGG
	if(e->use_ogg) {
		if(0 != e->encoder.ogg.stream) {
			OggFLAC__stream_encoder_delete(e->encoder.ogg.stream);
			e->encoder.ogg.stream = 0;
		}
	}
	else
#endif
	if(e->is_stdout) {
		if(0 != e->encoder.flac.stream) {
			FLAC__stream_encoder_delete(e->encoder.flac.stream);
			e->encoder.flac.stream = 0;
		}
	}
	else {
		if(0 != e->encoder.flac.file) {
			FLAC__file_encoder_delete(e->encoder.flac.file);
			e->encoder.flac.file = 0;
		}
	}

	if(0 != e->seek_table_template) {
		FLAC__metadata_object_delete(e->seek_table_template);
		e->seek_table_template = 0;
	}
}

int EncoderSession_finish_ok(EncoderSession *e, int info_align_carry, int info_align_zero)
{
	FLAC__StreamEncoderState fse_state = FLAC__STREAM_ENCODER_OK;
	int ret = 0;

#ifdef FLAC__HAS_OGG
	if(e->use_ogg) {
		if(e->encoder.ogg.stream) {
			fse_state = OggFLAC__stream_encoder_get_FLAC_stream_encoder_state(e->encoder.ogg.stream);
			OggFLAC__stream_encoder_finish(e->encoder.ogg.stream);
		}
	}
	else
#endif
	if(e->is_stdout) {
		if(e->encoder.flac.stream) {
			fse_state = FLAC__stream_encoder_get_state(e->encoder.flac.stream);
			FLAC__stream_encoder_finish(e->encoder.flac.stream);
		}
	}
	else {
		if(e->encoder.flac.file) {
			fse_state = FLAC__file_encoder_get_stream_encoder_state(e->encoder.flac.file);
			FLAC__file_encoder_finish(e->encoder.flac.file);
		}
	}

	if(e->verbose && e->total_samples_to_encode > 0) {
		print_stats(e);
		fprintf(stderr, "\n");
	}

	if(fse_state == FLAC__STREAM_ENCODER_VERIFY_MISMATCH_IN_AUDIO_DATA) {
		print_verify_error(e);
		ret = 1;
	}
	else {
		if(info_align_carry >= 0)
			fprintf(stderr, "%s: INFO: sector alignment causing %d samples to be carried over\n", e->inbasefilename, info_align_carry);
		if(info_align_zero >= 0)
			fprintf(stderr, "%s: INFO: sector alignment causing %d zero samples to be appended\n", e->inbasefilename, info_align_zero);
	}

	EncoderSession_destroy(e);

	return ret;
}

int EncoderSession_finish_error(EncoderSession *e)
{
	FLAC__StreamEncoderState fse_state;

	if(e->verbose && e->total_samples_to_encode > 0)
		fprintf(stderr, "\n");

#ifdef FLAC__HAS_OGG
	if(e->use_ogg) {
		fse_state = OggFLAC__stream_encoder_get_FLAC_stream_encoder_state(e->encoder.ogg.stream);
	}
	else
#endif
	if(e->is_stdout) {
		fse_state = FLAC__stream_encoder_get_state(e->encoder.flac.stream);
	}
	else {
		fse_state = FLAC__file_encoder_get_stream_encoder_state(e->encoder.flac.file);
	}

	if(fse_state == FLAC__STREAM_ENCODER_VERIFY_MISMATCH_IN_AUDIO_DATA)
		print_verify_error(e);
	else
		unlink(e->outfilename);

	EncoderSession_destroy(e);

	return 1;
}

FLAC__bool EncoderSession_init_encoder(EncoderSession *e, encode_options_t options, unsigned channels, unsigned bps, unsigned sample_rate)
{
	unsigned num_metadata;
	FLAC__StreamMetadata padding, *cuesheet = 0;
	FLAC__StreamMetadata *metadata[4];

	e->replay_gain = options.replay_gain;
	e->channels = channels;
	e->bits_per_sample = bps;
	e->sample_rate = sample_rate;

	if(e->replay_gain) {
		if(channels != 1 && channels != 2) {
			fprintf(stderr, "%s: ERROR, number of channels (%u) must be 1 or 2 for --replay-gain\n", e->inbasefilename, channels);
			return false;
		}
		if(!grabbag__replaygain_is_valid_sample_frequency(sample_rate)) {
			fprintf(stderr, "%s: ERROR, invalid sample rate (%u) for --replay-gain\n", e->inbasefilename, sample_rate);
			return false;
		}
		if(options.is_first_file) {
			if(!grabbag__replaygain_init(sample_rate)) {
				fprintf(stderr, "%s: ERROR initializing ReplayGain stage\n", e->inbasefilename);
				return false;
			}
		}
	}

	if(channels != 2)
		options.do_mid_side = options.loose_mid_side = false;

	if(!parse_cuesheet_(&cuesheet, options.cuesheet_filename, e->inbasefilename, e->total_samples_to_encode))
		return false;

	if(!convert_to_seek_table_template(options.requested_seek_points, options.num_requested_seek_points, options.cued_seekpoints? cuesheet : 0, e)) {
		fprintf(stderr, "%s: ERROR allocating memory for seek table\n", e->inbasefilename);
		if(0 != cuesheet)
			free(cuesheet);
		return false;
	}

	num_metadata = 0;
	if(e->seek_table_template->data.seek_table.num_points > 0) {
		e->seek_table_template->is_last = false; /* the encoder will set this for us */
		metadata[num_metadata++] = e->seek_table_template;
	}
	if(0 != cuesheet)
		metadata[num_metadata++] = cuesheet;
	metadata[num_metadata++] = options.vorbis_comment;
	if(options.padding > 0) {
		padding.is_last = false; /* the encoder will set this for us */
		padding.type = FLAC__METADATA_TYPE_PADDING;
		padding.length = (unsigned)options.padding;
		metadata[num_metadata++] = &padding;
	}

	e->blocksize = options.blocksize;
	e->stats_mask = (options.do_exhaustive_model_search || options.do_qlp_coeff_prec_search)? 0x0f : 0x3f;

#ifdef FLAC__HAS_OGG
	if(e->use_ogg) {
		if(options.has_serial_number)
			OggFLAC__stream_encoder_set_serial_number(e->encoder.ogg.stream, options.serial_number);
		OggFLAC__stream_encoder_set_verify(e->encoder.ogg.stream, options.verify);
		OggFLAC__stream_encoder_set_streamable_subset(e->encoder.ogg.stream, !options.lax);
		OggFLAC__stream_encoder_set_do_mid_side_stereo(e->encoder.ogg.stream, options.do_mid_side);
		OggFLAC__stream_encoder_set_loose_mid_side_stereo(e->encoder.ogg.stream, options.loose_mid_side);
		OggFLAC__stream_encoder_set_channels(e->encoder.ogg.stream, channels);
		OggFLAC__stream_encoder_set_bits_per_sample(e->encoder.ogg.stream, bps);
		OggFLAC__stream_encoder_set_sample_rate(e->encoder.ogg.stream, sample_rate);
		OggFLAC__stream_encoder_set_blocksize(e->encoder.ogg.stream, options.blocksize);
		OggFLAC__stream_encoder_set_max_lpc_order(e->encoder.ogg.stream, options.max_lpc_order);
		OggFLAC__stream_encoder_set_qlp_coeff_precision(e->encoder.ogg.stream, options.qlp_coeff_precision);
		OggFLAC__stream_encoder_set_do_qlp_coeff_prec_search(e->encoder.ogg.stream, options.do_qlp_coeff_prec_search);
		OggFLAC__stream_encoder_set_do_escape_coding(e->encoder.ogg.stream, options.do_escape_coding);
		OggFLAC__stream_encoder_set_do_exhaustive_model_search(e->encoder.ogg.stream, options.do_exhaustive_model_search);
		OggFLAC__stream_encoder_set_min_residual_partition_order(e->encoder.ogg.stream, options.min_residual_partition_order);
		OggFLAC__stream_encoder_set_max_residual_partition_order(e->encoder.ogg.stream, options.max_residual_partition_order);
		OggFLAC__stream_encoder_set_rice_parameter_search_dist(e->encoder.ogg.stream, options.rice_parameter_search_dist);
		OggFLAC__stream_encoder_set_total_samples_estimate(e->encoder.ogg.stream, e->total_samples_to_encode);
		OggFLAC__stream_encoder_set_metadata(e->encoder.ogg.stream, (num_metadata > 0)? metadata : 0, num_metadata);
		OggFLAC__stream_encoder_set_write_callback(e->encoder.ogg.stream, ogg_stream_encoder_write_callback);
		OggFLAC__stream_encoder_set_client_data(e->encoder.ogg.stream, e);

		OggFLAC__stream_encoder_disable_constant_subframes(e->encoder.ogg.stream, options.debug.disable_constant_subframes);
		OggFLAC__stream_encoder_disable_fixed_subframes(e->encoder.ogg.stream, options.debug.disable_fixed_subframes);
		OggFLAC__stream_encoder_disable_verbatim_subframes(e->encoder.ogg.stream, options.debug.disable_verbatim_subframes);

		if(OggFLAC__stream_encoder_init(e->encoder.ogg.stream) != FLAC__STREAM_ENCODER_OK) {
			print_error_with_state(e, "ERROR initializing encoder");
			if(0 != cuesheet)
				free(cuesheet);
			return false;
		}
	}
	else
#endif
	if(e->is_stdout) {
		FLAC__stream_encoder_set_verify(e->encoder.flac.stream, options.verify);
		FLAC__stream_encoder_set_streamable_subset(e->encoder.flac.stream, !options.lax);
		FLAC__stream_encoder_set_do_mid_side_stereo(e->encoder.flac.stream, options.do_mid_side);
		FLAC__stream_encoder_set_loose_mid_side_stereo(e->encoder.flac.stream, options.loose_mid_side);
		FLAC__stream_encoder_set_channels(e->encoder.flac.stream, channels);
		FLAC__stream_encoder_set_bits_per_sample(e->encoder.flac.stream, bps);
		FLAC__stream_encoder_set_sample_rate(e->encoder.flac.stream, sample_rate);
		FLAC__stream_encoder_set_blocksize(e->encoder.flac.stream, options.blocksize);
		FLAC__stream_encoder_set_max_lpc_order(e->encoder.flac.stream, options.max_lpc_order);
		FLAC__stream_encoder_set_qlp_coeff_precision(e->encoder.flac.stream, options.qlp_coeff_precision);
		FLAC__stream_encoder_set_do_qlp_coeff_prec_search(e->encoder.flac.stream, options.do_qlp_coeff_prec_search);
		FLAC__stream_encoder_set_do_escape_coding(e->encoder.flac.stream, options.do_escape_coding);
		FLAC__stream_encoder_set_do_exhaustive_model_search(e->encoder.flac.stream, options.do_exhaustive_model_search);
		FLAC__stream_encoder_set_min_residual_partition_order(e->encoder.flac.stream, options.min_residual_partition_order);
		FLAC__stream_encoder_set_max_residual_partition_order(e->encoder.flac.stream, options.max_residual_partition_order);
		FLAC__stream_encoder_set_rice_parameter_search_dist(e->encoder.flac.stream, options.rice_parameter_search_dist);
		FLAC__stream_encoder_set_total_samples_estimate(e->encoder.flac.stream, e->total_samples_to_encode);
		FLAC__stream_encoder_set_metadata(e->encoder.flac.stream, (num_metadata > 0)? metadata : 0, num_metadata);
		FLAC__stream_encoder_set_write_callback(e->encoder.flac.stream, flac_stream_encoder_write_callback);
		FLAC__stream_encoder_set_metadata_callback(e->encoder.flac.stream, flac_stream_encoder_metadata_callback);
		FLAC__stream_encoder_set_client_data(e->encoder.flac.stream, e);

		FLAC__stream_encoder_disable_constant_subframes(e->encoder.flac.stream, options.debug.disable_constant_subframes);
		FLAC__stream_encoder_disable_fixed_subframes(e->encoder.flac.stream, options.debug.disable_fixed_subframes);
		FLAC__stream_encoder_disable_verbatim_subframes(e->encoder.flac.stream, options.debug.disable_verbatim_subframes);

		if(FLAC__stream_encoder_init(e->encoder.flac.stream) != FLAC__STREAM_ENCODER_OK) {
			print_error_with_state(e, "ERROR initializing encoder");
			if(0 != cuesheet)
				free(cuesheet);
			return false;
		}
	}
	else {
		FLAC__file_encoder_set_filename(e->encoder.flac.file, e->outfilename);
		FLAC__file_encoder_set_verify(e->encoder.flac.file, options.verify);
		FLAC__file_encoder_set_streamable_subset(e->encoder.flac.file, !options.lax);
		FLAC__file_encoder_set_do_mid_side_stereo(e->encoder.flac.file, options.do_mid_side);
		FLAC__file_encoder_set_loose_mid_side_stereo(e->encoder.flac.file, options.loose_mid_side);
		FLAC__file_encoder_set_channels(e->encoder.flac.file, channels);
		FLAC__file_encoder_set_bits_per_sample(e->encoder.flac.file, bps);
		FLAC__file_encoder_set_sample_rate(e->encoder.flac.file, sample_rate);
		FLAC__file_encoder_set_blocksize(e->encoder.flac.file, options.blocksize);
		FLAC__file_encoder_set_max_lpc_order(e->encoder.flac.file, options.max_lpc_order);
		FLAC__file_encoder_set_qlp_coeff_precision(e->encoder.flac.file, options.qlp_coeff_precision);
		FLAC__file_encoder_set_do_qlp_coeff_prec_search(e->encoder.flac.file, options.do_qlp_coeff_prec_search);
		FLAC__file_encoder_set_do_escape_coding(e->encoder.flac.file, options.do_escape_coding);
		FLAC__file_encoder_set_do_exhaustive_model_search(e->encoder.flac.file, options.do_exhaustive_model_search);
		FLAC__file_encoder_set_min_residual_partition_order(e->encoder.flac.file, options.min_residual_partition_order);
		FLAC__file_encoder_set_max_residual_partition_order(e->encoder.flac.file, options.max_residual_partition_order);
		FLAC__file_encoder_set_rice_parameter_search_dist(e->encoder.flac.file, options.rice_parameter_search_dist);
		FLAC__file_encoder_set_total_samples_estimate(e->encoder.flac.file, e->total_samples_to_encode);
		FLAC__file_encoder_set_metadata(e->encoder.flac.file, (num_metadata > 0)? metadata : 0, num_metadata);
		FLAC__file_encoder_set_progress_callback(e->encoder.flac.file, flac_file_encoder_progress_callback);
		FLAC__file_encoder_set_client_data(e->encoder.flac.file, e);

		FLAC__file_encoder_disable_constant_subframes(e->encoder.flac.file, options.debug.disable_constant_subframes);
		FLAC__file_encoder_disable_fixed_subframes(e->encoder.flac.file, options.debug.disable_fixed_subframes);
		FLAC__file_encoder_disable_verbatim_subframes(e->encoder.flac.file, options.debug.disable_verbatim_subframes);

		if(FLAC__file_encoder_init(e->encoder.flac.file) != FLAC__FILE_ENCODER_OK) {
			print_error_with_state(e, "ERROR initializing encoder");
			if(0 != cuesheet)
				free(cuesheet);
			return false;
		}
	}

	if(0 != cuesheet)
		free(cuesheet);

	return true;
}

FLAC__bool EncoderSession_process(EncoderSession *e, const FLAC__int32 * const buffer[], unsigned samples)
{
	if(e->replay_gain) {
		if(!grabbag__replaygain_analyze(buffer, e->channels==2, e->bits_per_sample, samples))
			fprintf(stderr, "%s: WARNING, error while calculating ReplayGain\n", e->inbasefilename);
	}

#ifdef FLAC__HAS_OGG
	if(e->use_ogg) {
		return OggFLAC__stream_encoder_process(e->encoder.ogg.stream, buffer, samples);
	}
	else
#endif
	if(e->is_stdout) {
		return FLAC__stream_encoder_process(e->encoder.flac.stream, buffer, samples);
	}
	else {
		return FLAC__file_encoder_process(e->encoder.flac.file, buffer, samples);
	}
}

FLAC__bool convert_to_seek_table_template(const char *requested_seek_points, int num_requested_seek_points, FLAC__StreamMetadata *cuesheet, EncoderSession *e)
{
	FLAC__bool only_placeholders;
	FLAC__bool has_real_points;

	if(num_requested_seek_points == 0 && 0 == cuesheet)
		return true;

	if(num_requested_seek_points < 0) {
		requested_seek_points = "10s;";
		num_requested_seek_points = 1;
	}

	if(e->is_stdout)
		only_placeholders = true;
#ifdef FLAC__HAS_OGG
	else if(e->use_ogg)
		only_placeholders = true;
#endif
	else
		only_placeholders = false;

	if(num_requested_seek_points > 0) {
		if(!grabbag__seektable_convert_specification_to_template(requested_seek_points, only_placeholders, e->total_samples_to_encode, e->sample_rate, e->seek_table_template, &has_real_points))
			return false;
	}

	if(0 != cuesheet) {
		unsigned i, j;
		const FLAC__StreamMetadata_CueSheet *cs = &cuesheet->data.cue_sheet;
		for(i = 0; i < cs->num_tracks; i++) {
			const FLAC__StreamMetadata_CueSheet_Track *tr = cs->tracks+i;
			for(j = 0; j < tr->num_indices; j++) {
				if(!FLAC__metadata_object_seektable_template_append_point(e->seek_table_template, tr->offset + tr->indices[j].offset))
					return false;
				has_real_points = true;
			}
		}
		if(has_real_points)
			if(!FLAC__metadata_object_seektable_template_sort(e->seek_table_template, /*compact=*/true))
				return false;
	}

	if(has_real_points) {
		if(e->is_stdout)
			fprintf(stderr, "%s: WARNING, cannot write back seekpoints when encoding to stdout\n", e->inbasefilename);
#ifdef FLAC__HAS_OGG
		else if(e->use_ogg)
			fprintf(stderr, "%s: WARNING, cannot write back seekpoints when encoding to Ogg yet\n", e->inbasefilename);
#endif
	}

	return true;
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
		fprintf(stderr, "%s: ERROR, cannot use --until when input length is unknown\n", inbasefilename);
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
		fprintf(stderr, "%s: ERROR, --until value is before beginning of input\n", inbasefilename);
		return false;
	}
	if((FLAC__uint64)spec->value.samples <= skip) {
		fprintf(stderr, "%s: ERROR, --until value is before --skip point\n", inbasefilename);
		return false;
	}
	if((FLAC__uint64)spec->value.samples > total_samples_in_input) {
		fprintf(stderr, "%s: ERROR, --until value is after end of input\n", inbasefilename);
		return false;
	}

	return true;
}

void format_input(FLAC__int32 *dest[], unsigned wide_samples, FLAC__bool is_big_endian, FLAC__bool is_unsigned_samples, unsigned channels, unsigned bps)
{
	unsigned wide_sample, sample, channel, byte;

	if(bps == 8) {
		if(is_unsigned_samples) {
			for(sample = wide_sample = 0; wide_sample < wide_samples; wide_sample++)
				for(channel = 0; channel < channels; channel++, sample++)
					dest[channel][wide_sample] = (FLAC__int32)ucbuffer_[sample] - 0x80;
		}
		else {
			for(sample = wide_sample = 0; wide_sample < wide_samples; wide_sample++)
				for(channel = 0; channel < channels; channel++, sample++)
					dest[channel][wide_sample] = (FLAC__int32)scbuffer_[sample];
		}
	}
	else if(bps == 16) {
		if(is_big_endian != is_big_endian_host_) {
			unsigned char tmp;
			const unsigned bytes = wide_samples * channels * (bps >> 3);
			for(byte = 0; byte < bytes; byte += 2) {
				tmp = ucbuffer_[byte];
				ucbuffer_[byte] = ucbuffer_[byte+1];
				ucbuffer_[byte+1] = tmp;
			}
		}
		if(is_unsigned_samples) {
			for(sample = wide_sample = 0; wide_sample < wide_samples; wide_sample++)
				for(channel = 0; channel < channels; channel++, sample++)
					dest[channel][wide_sample] = (FLAC__int32)usbuffer_[sample] - 0x8000;
		}
		else {
			for(sample = wide_sample = 0; wide_sample < wide_samples; wide_sample++)
				for(channel = 0; channel < channels; channel++, sample++)
					dest[channel][wide_sample] = (FLAC__int32)ssbuffer_[sample];
		}
	}
	else if(bps == 24) {
		if(!is_big_endian) {
			unsigned char tmp;
			const unsigned bytes = wide_samples * channels * (bps >> 3);
			for(byte = 0; byte < bytes; byte += 3) {
				tmp = ucbuffer_[byte];
				ucbuffer_[byte] = ucbuffer_[byte+2];
				ucbuffer_[byte+2] = tmp;
			}
		}
		if(is_unsigned_samples) {
			for(byte = sample = wide_sample = 0; wide_sample < wide_samples; wide_sample++)
				for(channel = 0; channel < channels; channel++, sample++) {
					dest[channel][wide_sample]  = ucbuffer_[byte++]; dest[channel][wide_sample] <<= 8;
					dest[channel][wide_sample] |= ucbuffer_[byte++]; dest[channel][wide_sample] <<= 8;
					dest[channel][wide_sample] |= ucbuffer_[byte++];
					dest[channel][wide_sample] -= 0x800000;
				}
		}
		else {
			for(byte = sample = wide_sample = 0; wide_sample < wide_samples; wide_sample++)
				for(channel = 0; channel < channels; channel++, sample++) {
					dest[channel][wide_sample]  = scbuffer_[byte++]; dest[channel][wide_sample] <<= 8;
					dest[channel][wide_sample] |= ucbuffer_[byte++]; dest[channel][wide_sample] <<= 8;
					dest[channel][wide_sample] |= ucbuffer_[byte++];
				}
		}
	}
	else {
		FLAC__ASSERT(0);
	}
}

#ifdef FLAC__HAS_OGG
FLAC__StreamEncoderWriteStatus ogg_stream_encoder_write_callback(const OggFLAC__StreamEncoder *encoder, const FLAC__byte buffer[], unsigned bytes, unsigned samples, unsigned current_frame, void *client_data)
{
	EncoderSession *encoder_session = (EncoderSession*)client_data;

	(void)encoder;

	encoder_session->bytes_written += bytes;
	/*
	 * With Ogg FLAC we don't get one write callback per frame and
	 * we don't have good number for 'samples', so we estimate based
	 * on the frame number and the knowledge that all blocks (except
	 * the last) are the same size.
	 */
	(void)samples;
	encoder_session->samples_written = (current_frame+1) * encoder_session->blocksize;

	if(encoder_session->verbose && encoder_session->total_samples_to_encode > 0 && !(current_frame & encoder_session->stats_mask))
		print_stats(encoder_session);

	if(flac__utils_fwrite(buffer, sizeof(FLAC__byte), bytes, encoder_session->fout) == bytes)
		return FLAC__STREAM_ENCODER_WRITE_STATUS_OK;
	else
		return FLAC__STREAM_ENCODER_WRITE_STATUS_FATAL_ERROR;
}
#endif

FLAC__StreamEncoderWriteStatus flac_stream_encoder_write_callback(const FLAC__StreamEncoder *encoder, const FLAC__byte buffer[], unsigned bytes, unsigned samples, unsigned current_frame, void *client_data)
{
	EncoderSession *encoder_session = (EncoderSession*)client_data;

	(void)encoder;

	encoder_session->bytes_written += bytes;
	encoder_session->samples_written += samples;

	if(samples && encoder_session->verbose && encoder_session->total_samples_to_encode > 0 && !(current_frame & encoder_session->stats_mask))
		print_stats(encoder_session);

	if(flac__utils_fwrite(buffer, sizeof(FLAC__byte), bytes, encoder_session->fout) == bytes)
		return FLAC__STREAM_ENCODER_WRITE_STATUS_OK;
	else
		return FLAC__STREAM_ENCODER_WRITE_STATUS_FATAL_ERROR;
}

void flac_stream_encoder_metadata_callback(const FLAC__StreamEncoder *encoder, const FLAC__StreamMetadata *metadata, void *client_data)
{
	/*
	 * Nothing to do; if we get here, we're decoding to stdout, in
	 * which case we can't seek backwards to write new metadata.
	 */
	(void)encoder, (void)metadata, (void)client_data;
}

void flac_file_encoder_progress_callback(const FLAC__FileEncoder *encoder, FLAC__uint64 bytes_written, FLAC__uint64 samples_written, unsigned frames_written, unsigned total_frames_estimate, void *client_data)
{
	EncoderSession *encoder_session = (EncoderSession*)client_data;

	(void)encoder, (void)total_frames_estimate;

	encoder_session->bytes_written = bytes_written;
	encoder_session->samples_written = samples_written;

	if(encoder_session->verbose && encoder_session->total_samples_to_encode > 0 && !((frames_written-1) & encoder_session->stats_mask))
		print_stats(encoder_session);
}

FLAC__bool parse_cuesheet_(FLAC__StreamMetadata **cuesheet, const char *cuesheet_filename, const char *inbasefilename, FLAC__uint64 lead_out_offset)
{
	FILE *f;
	unsigned last_line_read;
	const char *error_message;

	if(0 == cuesheet_filename)
		return true;

	if(lead_out_offset == 0) {
		fprintf(stderr, "%s: ERROR cannot import cuesheet when the number of input samples to encode is unknown\n", inbasefilename);
		return false;
	}

	if(0 == (f = fopen(cuesheet_filename, "r"))) {
		fprintf(stderr, "%s: ERROR opening cuesheet \"%s\" for reading\n", inbasefilename, cuesheet_filename);
		return false;
	}

	*cuesheet = grabbag__cuesheet_parse(f, &error_message, &last_line_read, /*@@@@is_cdda=*/true, lead_out_offset);

	fclose(f);

	if(0 == *cuesheet) {
		fprintf(stderr, "%s: ERROR parsing cuesheet \"%s\" on line %u: %s\n", inbasefilename, cuesheet_filename, last_line_read, error_message);
		return false;
	}

	return true;
}

void print_stats(const EncoderSession *encoder_session)
{
	const FLAC__uint64 samples_written = min(encoder_session->total_samples_to_encode, encoder_session->samples_written);
#if defined _MSC_VER || defined __MINGW32__
	/* with VC++ you have to spoon feed it the casting */
	const double progress = (double)(FLAC__int64)samples_written / (double)(FLAC__int64)encoder_session->total_samples_to_encode;
	const double ratio = (double)(FLAC__int64)encoder_session->bytes_written / ((double)(FLAC__int64)encoder_session->unencoded_size * progress);
#else
	const double progress = (double)samples_written / (double)encoder_session->total_samples_to_encode;
	const double ratio = (double)encoder_session->bytes_written / ((double)encoder_session->unencoded_size * progress);
#endif

	if(samples_written == encoder_session->total_samples_to_encode) {
		fprintf(stderr, "\r%s:%s wrote %u bytes, ratio=%0.3f",
			encoder_session->inbasefilename,
			encoder_session->verify? " Verify OK," : "",
			(unsigned)encoder_session->bytes_written,
			ratio
		);
	}
	else {
		fprintf(stderr, "\r%s: %u%% complete, ratio=%0.3f", encoder_session->inbasefilename, (unsigned)floor(progress * 100.0 + 0.5), ratio);
	}
}

void print_error_with_state(const EncoderSession *e, const char *message)
{
	const int ilen = strlen(e->inbasefilename) + 1;

	fprintf(stderr, "\n%s: %s\n", e->inbasefilename, message);

#ifdef FLAC__HAS_OGG
	if(e->use_ogg) {
		const OggFLAC__StreamEncoderState ose_state = OggFLAC__stream_encoder_get_state(e->encoder.ogg.stream);
		if(ose_state != OggFLAC__STREAM_ENCODER_FLAC_STREAM_ENCODER_ERROR) {
			fprintf(stderr, "%*s state = %d:%s\n", ilen, "", (int)ose_state, OggFLAC__StreamEncoderStateString[ose_state]);
		}
		else {
			const FLAC__StreamEncoderState fse_state = OggFLAC__stream_encoder_get_FLAC_stream_encoder_state(e->encoder.ogg.stream);
			if(fse_state != FLAC__STREAM_ENCODER_VERIFY_DECODER_ERROR) {
				fprintf(stderr, "%*s state = %d:%s\n", ilen, "", (int)fse_state, FLAC__StreamEncoderStateString[fse_state]);
			}
			else {
				const FLAC__StreamDecoderState fsd_state = OggFLAC__stream_encoder_get_verify_decoder_state(e->encoder.ogg.stream);
				fprintf(stderr, "%*s state = %d:%s\n", ilen, "", (int)fsd_state, FLAC__StreamDecoderStateString[fsd_state]);
			}
		}
	}
	else
#endif
	if(e->is_stdout) {
		const FLAC__StreamEncoderState fse_state = FLAC__stream_encoder_get_state(e->encoder.flac.stream);
		if(fse_state != FLAC__STREAM_ENCODER_VERIFY_DECODER_ERROR) {
			fprintf(stderr, "%*s state = %d:%s\n", ilen, "", (int)fse_state, FLAC__StreamEncoderStateString[fse_state]);
		}
		else {
			const FLAC__StreamDecoderState fsd_state = FLAC__stream_encoder_get_verify_decoder_state(e->encoder.flac.stream);
			fprintf(stderr, "%*s state = %d:%s\n", ilen, "", (int)fsd_state, FLAC__StreamDecoderStateString[fsd_state]);
		}
	}
	else {
		const FLAC__FileEncoderState ffe_state = FLAC__file_encoder_get_state(e->encoder.flac.file);
		if(ffe_state != FLAC__FILE_ENCODER_SEEKABLE_STREAM_ENCODER_ERROR) {
			fprintf(stderr, "%*s state = %d:%s\n", ilen, "", (int)ffe_state, FLAC__FileEncoderStateString[ffe_state]);
		}
		else {
			const FLAC__SeekableStreamEncoderState fsse_state = FLAC__file_encoder_get_seekable_stream_encoder_state(e->encoder.flac.file);
			if(fsse_state != FLAC__SEEKABLE_STREAM_ENCODER_STREAM_ENCODER_ERROR) {
				fprintf(stderr, "%*s state = %d:%s\n", ilen, "", (int)fsse_state, FLAC__SeekableStreamEncoderStateString[fsse_state]);
			}
			else {
				const FLAC__StreamEncoderState fse_state = FLAC__file_encoder_get_stream_encoder_state(e->encoder.flac.file);
				if(fse_state != FLAC__STREAM_ENCODER_VERIFY_DECODER_ERROR) {
					fprintf(stderr, "%*s state = %d:%s\n", ilen, "", (int)fse_state, FLAC__StreamEncoderStateString[fse_state]);
				}
				else {
					const FLAC__StreamDecoderState fsd_state = FLAC__file_encoder_get_verify_decoder_state(e->encoder.flac.file);
					fprintf(stderr, "%*s state = %d:%s\n", ilen, "", (int)fsd_state, FLAC__StreamDecoderStateString[fsd_state]);
				}
			}
		}
	}
}

void print_verify_error(EncoderSession *e)
{
	FLAC__uint64 absolute_sample;
	unsigned frame_number;
	unsigned channel;
	unsigned sample;
	FLAC__int32 expected;
	FLAC__int32 got;

#ifdef FLAC__HAS_OGG
	if(e->use_ogg) {
		OggFLAC__stream_encoder_get_verify_decoder_error_stats(e->encoder.ogg.stream, &absolute_sample, &frame_number, &channel, &sample, &expected, &got);
	}
	else
#endif
	if(e->is_stdout) {
		FLAC__stream_encoder_get_verify_decoder_error_stats(e->encoder.flac.stream, &absolute_sample, &frame_number, &channel, &sample, &expected, &got);
	}
	else {
		FLAC__file_encoder_get_verify_decoder_error_stats(e->encoder.flac.file, &absolute_sample, &frame_number, &channel, &sample, &expected, &got);
	}

	fprintf(stderr, "%s: ERROR: mismatch in decoded data, verify FAILED!\n", e->inbasefilename);
	fprintf(stderr, "       Absolute sample=%u, frame=%u, channel=%u, sample=%u, expected %d, got %d\n", (unsigned)absolute_sample, frame_number, channel, sample, expected, got);
	fprintf(stderr, "       Please submit a bug report to\n");
	fprintf(stderr, "           http://sourceforge.net/bugs/?func=addbug&group_id=13478\n");
	fprintf(stderr, "       Make sure to include an email contact in the comment and/or use the\n");
	fprintf(stderr, "       \"Monitor\" feature to monitor the bug status.\n");
	fprintf(stderr, "Verify FAILED!  Do not trust %s\n", e->outfilename);
}

FLAC__bool read_little_endian_uint16(FILE *f, FLAC__uint16 *val, FLAC__bool eof_ok, const char *fn)
{
	size_t bytes_read = fread(val, 1, 2, f);

	if(bytes_read == 0) {
		if(!eof_ok) {
			fprintf(stderr, "%s: ERROR: unexpected EOF\n", fn);
			return false;
		}
		else
			return true;
	}
	else if(bytes_read < 2) {
		fprintf(stderr, "%s: ERROR: unexpected EOF\n", fn);
		return false;
	}
	else {
		if(is_big_endian_host_) {
			FLAC__byte tmp, *b = (FLAC__byte*)val;
			tmp = b[1]; b[1] = b[0]; b[0] = tmp;
		}
		return true;
	}
}

FLAC__bool read_little_endian_uint32(FILE *f, FLAC__uint32 *val, FLAC__bool eof_ok, const char *fn)
{
	size_t bytes_read = fread(val, 1, 4, f);

	if(bytes_read == 0) {
		if(!eof_ok) {
			fprintf(stderr, "%s: ERROR: unexpected EOF\n", fn);
			return false;
		}
		else
			return true;
	}
	else if(bytes_read < 4) {
		fprintf(stderr, "%s: ERROR: unexpected EOF\n", fn);
		return false;
	}
	else {
		if(is_big_endian_host_) {
			FLAC__byte tmp, *b = (FLAC__byte*)val;
			tmp = b[3]; b[3] = b[0]; b[0] = tmp;
			tmp = b[2]; b[2] = b[1]; b[1] = tmp;
		}
		return true;
	}
}

FLAC__bool
read_big_endian_uint16(FILE *f, FLAC__uint16 *val, FLAC__bool eof_ok, const char *fn)
{
	unsigned char buf[4];
	size_t bytes_read= fread(buf, 1, 2, f);

	if(bytes_read==0U && eof_ok)
		return true;
	else if(bytes_read<2U) {
		fprintf(stderr, "%s: ERROR: unexpected EOF\n", fn);
		return false;
	}

	/* this is independent of host endianness */
	*val= (FLAC__uint16)(buf[0])<<8 | buf[1];

	return true;
}

FLAC__bool
read_big_endian_uint32(FILE *f, FLAC__uint32 *val, FLAC__bool eof_ok, const char *fn)
{
	unsigned char buf[4];
	size_t bytes_read= fread(buf, 1, 4, f);

	if(bytes_read==0U && eof_ok)
		return true;
	else if(bytes_read<4U) {
		fprintf(stderr, "%s: ERROR: unexpected EOF\n", fn);
		return false;
	}

	/* this is independent of host endianness */
	*val= (FLAC__uint32)(buf[0])<<24 | (FLAC__uint32)(buf[1])<<16 |
		(FLAC__uint32)(buf[2])<<8 | buf[3];

	return true;
}

FLAC__bool
read_sane_extended(FILE *f, FLAC__uint32 *val, FLAC__bool eof_ok, const char *fn)
	/* Read an IEEE 754 80-bit (aka SANE) extended floating point value from 'f',
	 * convert it into an integral value and store in 'val'.  Return false if only
	 * between 1 and 9 bytes remain in 'f', if 0 bytes remain in 'f' and 'eof_ok' is
	 * false, or if the value is negative, between zero and one, or too large to be
	 * represented by 'val'; return true otherwise.
	 */
{
	unsigned int i;
	unsigned char buf[10];
	size_t bytes_read= fread(buf, 1U, 10U, f);
	FLAC__int16 e= ((FLAC__uint16)(buf[0])<<8 | (FLAC__uint16)(buf[1]))-0x3FFF;
	FLAC__int16 shift= 63-e;
	FLAC__uint64 p= 0U;

	if(bytes_read==0U && eof_ok)
		return true;
	else if(bytes_read<10U) {
		fprintf(stderr, "%s: ERROR: unexpected EOF\n", fn);
		return false;
	}
	else if((buf[0]>>7)==1U || e<0 || e>63) {
		fprintf(stderr, "%s: ERROR: invalid floating-point value\n", fn);
		return false;
	}

	for(i= 0U; i<8U; ++i)
		p|= (FLAC__uint64)(buf[i+2])<<(56U-i*8);
	*val= (FLAC__uint32)((p>>shift)+(p>>(shift-1) & 0x1));

	return true;
}
