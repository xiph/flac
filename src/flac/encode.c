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
#include <limits.h> /* for LONG_MAX */
#include <math.h> /* for floor() */
#include <stdio.h> /* for FILE etc. */
#include <stdlib.h> /* for malloc */
#include <string.h> /* for strcmp() */
#include "FLAC/all.h"
#include "encode.h"
#include "file.h"
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
	const char *inbasefilename;
	const char *outfilename;

	FLAC__bool verify_failed;
	FLAC__uint64 unencoded_size;
	FLAC__uint64 total_samples_to_encode;
	FLAC__uint64 bytes_written;
	FLAC__uint64 samples_written;
	unsigned current_frame;

	union {
		FLAC__FileEncoder *flac;
#ifdef FLAC__HAS_OGG
		OggFLAC__StreamEncoder *ogg;
#endif
	} encoder;

	FILE *fin;
	FILE *fout;
	FLAC__StreamMetadata *seek_table_template;
} EncoderSession;

static FLAC__bool is_big_endian_host;

static unsigned char ucbuffer[CHUNK_OF_SAMPLES*FLAC__MAX_CHANNELS*((FLAC__REFERENCE_CODEC_MAX_BITS_PER_SAMPLE+7)/8)];
static signed char *scbuffer = (signed char *)ucbuffer;
static FLAC__uint16 *usbuffer = (FLAC__uint16 *)ucbuffer;
static FLAC__int16 *ssbuffer = (FLAC__int16 *)ucbuffer;

static FLAC__int32 in[FLAC__MAX_CHANNELS][CHUNK_OF_SAMPLES];
static FLAC__int32 *input[FLAC__MAX_CHANNELS];

/* local routines */
static FLAC__bool init_encoder(encode_options_t options, unsigned channels, unsigned bps, unsigned sample_rate, EncoderSession *encoder_session);
static FLAC__bool convert_to_seek_table_template(char *requested_seek_points, int num_requested_seek_points, FLAC__uint64 stream_samples, FLAC__StreamMetadata *seek_table_template);
static void format_input(FLAC__int32 *dest[], unsigned wide_samples, FLAC__bool is_big_endian, FLAC__bool is_unsigned_samples, unsigned channels, unsigned bps);
static FLAC__StreamEncoderWriteStatus write_callback(const FLAC__StreamEncoder *encoder, const FLAC__byte buffer[], unsigned bytes, unsigned samples, unsigned current_frame, void *client_data);
static void metadata_callback(const FLAC__StreamEncoder *encoder, const FLAC__StreamMetadata *metadata, void *client_data);
static void print_stats(const EncoderSession *encoder_session);
static FLAC__bool read_little_endian_uint16(FILE *f, FLAC__uint16 *val, FLAC__bool eof_ok, const char *fn);
static FLAC__bool read_little_endian_uint32(FILE *f, FLAC__uint32 *val, FLAC__bool eof_ok, const char *fn);
static FLAC__bool read_big_endian_uint16(FILE *f, FLAC__uint16 *val, FLAC__bool eof_ok, const char *fn);
static FLAC__bool read_big_endian_uint32(FILE *f, FLAC__uint32 *val, FLAC__bool eof_ok, const char *fn);
static FLAC__bool read_sane_extended(FILE *f, FLAC__uint32 *val, FLAC__bool eof_ok, const char *fn);
static FLAC__bool write_big_endian_uint16(FILE *f, FLAC__uint16 val);
static FLAC__bool write_big_endian_uint64(FILE *f, FLAC__uint64 val);

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
	enum { NORMAL, DONE, ERROR, MISMATCH } status= NORMAL;

	FLAC__ASSERT(!options.common.sector_align || options.common.skip == 0);

	(void)infilesize; /* silence compiler warning about unused parameter */
	(void)lookahead; /* silence compiler warning about unused parameter */
	(void)lookahead_length; /* silence compiler warning about unused parameter */

	if(!EncoderSession_construct(&encoder_session, options.common.use_ogg, options.common.verify, options.common.verbose, infile, infilename, outfilename))
		return 1;

	/* lookahead[] already has "FORMxxxxAIFF", do sub-chunks */

	while(status==NORMAL) {
		size_t c= 0U;
		char chunk_id[4];

		/* chunk identifier; really conservative about behavior of fread() and feof() */
		if(feof(infile) || ((c= fread(chunk_id, 1U, 4U, infile)), c==0U && feof(infile)))
			status= DONE;
		else if(c<4U || feof(infile)) {
			fprintf(stderr, "%s: ERROR: incomplete chunk identifier\n", encoder_session.inbasefilename);
			status= ERROR;
		}

		if(status==NORMAL && got_comm_chunk==false && !strncmp(chunk_id, "COMM", 4)) { /* common chunk */
			unsigned long skip;

			if(status==NORMAL) {
				/* COMM chunk size */
				if(!read_big_endian_uint32(infile, &xx, false, encoder_session.inbasefilename))
					status= ERROR;
				else if(xx<18U) {
					fprintf(stderr, "%s: ERROR: non-standard 'COMM' chunk has length = %u\n", encoder_session.inbasefilename, (unsigned int)xx);
					status= ERROR;
				}
				else if(xx!=18U)
					fprintf(stderr, "%s: WARNING: non-standard 'COMM' chunk has length = %u\n", encoder_session.inbasefilename, (unsigned int)xx);
				skip= (xx-18U)+(xx & 1U);
			}

			if(status==NORMAL) {
				/* number of channels */
				if(!read_big_endian_uint16(infile, &x, false, encoder_session.inbasefilename))
					status= ERROR;
				else if(x==0U || x>FLAC__MAX_CHANNELS) {
					fprintf(stderr, "%s: ERROR: unsupported number channels %u\n", encoder_session.inbasefilename, (unsigned int)x);
					status= ERROR;
				}
				else if(options.common.sector_align && x!=2U) {
					fprintf(stderr, "%s: ERROR: file has %u channels, must be 2 for --sector-align\n", encoder_session.inbasefilename, (unsigned int)x);
					status= ERROR;
				}
				channels= x;
			}

			if(status==NORMAL) {
				/* number of sample frames */
				if(!read_big_endian_uint32(infile, &xx, false, encoder_session.inbasefilename))
					status= ERROR;
				sample_frames= xx;
			}

			if(status==NORMAL) {
				/* bits per sample */
				if(!read_big_endian_uint16(infile, &x, false, encoder_session.inbasefilename))
					status= ERROR;
				else if(x!=8U && x!=16U && x!=24U) {
					fprintf(stderr, "%s: ERROR: unsupported bits per sample %u\n", encoder_session.inbasefilename, (unsigned int)x);
					status= ERROR;
				}
				else if(options.common.sector_align && x!=16U) {
					fprintf(stderr, "%s: ERROR: file has %u bits per sample, must be 16 for --sector-align\n", encoder_session.inbasefilename, (unsigned int)x);
					status= ERROR;
				}
				bps= x;
			}

			if(status==NORMAL) {
				/* sample rate */
				if(!read_sane_extended(infile, &xx, false, encoder_session.inbasefilename))
					status= ERROR;
				else if(!FLAC__format_sample_rate_is_valid(xx)) {
					fprintf(stderr, "%s: ERROR: unsupported sample rate %u\n", encoder_session.inbasefilename, (unsigned int)xx);
					status= ERROR;
				}
				else if(options.common.sector_align && xx!=44100U) {
					fprintf(stderr, "%s: ERROR: file's sample rate is %u, must be 44100 for --sector-align\n", encoder_session.inbasefilename, (unsigned int)xx);
					status= ERROR;
				}
				sample_rate= xx;
			}

			/* skip any extra data in the COMM chunk */
			FLAC__ASSERT(skip<=LONG_MAX);
			while(status==NORMAL && skip>0U && fseek(infile, skip, SEEK_CUR)<0) {
				unsigned int need= min(skip, sizeof ucbuffer);
				if(fread(ucbuffer, 1U, need, infile)<need) {
					fprintf(stderr, "%s: ERROR during read while skipping extra COMM data\n", encoder_session.inbasefilename);
					status= ERROR;
				}
				skip-= need;
			}

			got_comm_chunk= true;
		}
		else if(status==NORMAL && got_ssnd_chunk==false && !strncmp(chunk_id, "SSND", 4)) { /* sound data chunk */
			unsigned int offset= 0U, block_size= 0U, align_remainder= 0U, data_bytes;
			size_t bytes_per_frame= channels*(bps>>3);
			FLAC__bool pad= false;

			if(status==NORMAL && got_comm_chunk==false) {
				fprintf(stderr, "%s: ERROR: got 'SSND' chunk before 'COMM' chunk\n", encoder_session.inbasefilename);
				status= ERROR;
			}

			if(status==NORMAL) {
				/* SSND chunk size */
				if(!read_big_endian_uint32(infile, &xx, false, encoder_session.inbasefilename))
					status= ERROR;
				else if(xx!=(sample_frames*bytes_per_frame + 8U)) {
					fprintf(stderr, "%s: ERROR: SSND chunk size inconsistent with sample frame count\n", encoder_session.inbasefilename);
					status= ERROR;
				}
				data_bytes= xx;
				pad= (data_bytes & 1U) ? true : false;
			}

			if(status==NORMAL) {
				/* offset */
				if(!read_big_endian_uint32(infile, &xx, false, encoder_session.inbasefilename))
					status= ERROR;
				else if(xx!=0U) {
					fprintf(stderr, "%s: ERROR: offset is %u; must be 0\n", encoder_session.inbasefilename, (unsigned int)xx);
					status= ERROR;
				}
				offset= xx;
			}

			if(status==NORMAL) {
				/* block size */
				if(!read_big_endian_uint32(infile, &xx, false, encoder_session.inbasefilename))
					status= ERROR;
				else if(xx!=0U) {
					fprintf(stderr, "%s: ERROR: block size is %u; must be 0\n", encoder_session.inbasefilename, (unsigned int)xx);
					status= ERROR;
				}
				block_size= xx;
			}

			if(status==NORMAL && options.common.skip>0U) {
				FLAC__uint64 remaining= options.common.skip*bytes_per_frame;

				/* do 1<<30 bytes at a time, since 1<<30 is a nice round number, and */
				/* is guaranteed to be less than LONG_MAX */
				for(; remaining>0U; remaining-= remaining>(1U<<30) ? remaining : (1U<<30))
				{
					unsigned long skip= remaining % (1U<<30);

					FLAC__ASSERT(skip<=LONG_MAX);
					while(status==NORMAL && skip>0 && fseek(infile, skip, SEEK_CUR)<0) {
						unsigned int need= min(skip, sizeof ucbuffer);
						if(fread(ucbuffer, 1U, need, infile)<need) {
							fprintf(stderr, "%s: ERROR during read while skipping samples\n", encoder_session.inbasefilename);
							status= ERROR;
						}
						skip-= need;
					}
				}
			}

			if(status==NORMAL) {
				data_bytes-= (8U + (unsigned int)options.common.skip*bytes_per_frame); /*@@@ WATCHOUT: 4GB limit */
				encoder_session.total_samples_to_encode= data_bytes/bytes_per_frame + *options.common.align_reservoir_samples;
				if(options.common.sector_align) {
					align_remainder= (unsigned int)(encoder_session.total_samples_to_encode % 588U);
					if(options.common.is_last_file)
						encoder_session.total_samples_to_encode+= (588U-align_remainder); /* will pad with zeroes */
					else
						encoder_session.total_samples_to_encode-= align_remainder; /* will stop short and carry over to next file */
				}

				/* +54 for the size of the AIFF headers; this is just an estimate for the progress indicator and doesn't need to be exact */
				encoder_session.unencoded_size= encoder_session.total_samples_to_encode*bytes_per_frame+54;

				if(!init_encoder(options.common, channels, bps, sample_rate, &encoder_session))
					status= ERROR;
			}

			/* first do any samples in the reservoir */
			if(status==NORMAL && options.common.sector_align && *options.common.align_reservoir_samples>0U) {

				if(!FLAC__stream_encoder_process(encoder_session.encoder, (const FLAC__int32 *const *)options.common.align_reservoir, *options.common.align_reservoir_samples)) {
					fprintf(stderr, "%s: ERROR during encoding, state = %d:%s\n", encoder_session.inbasefilename, FLAC__stream_encoder_get_state(encoder_session.encoder), FLAC__StreamEncoderStateString[FLAC__stream_encoder_get_state(encoder_session.encoder)]);
					encoder_session.verify_failed = (FLAC__stream_encoder_get_state(encoder_session.encoder) == FLAC__STREAM_ENCODER_VERIFY_MISMATCH_IN_AUDIO_DATA);
					status= ERROR;
				}
			}

			/* decrement the data_bytes counter if we need to align the file */
			if(status==NORMAL && options.common.sector_align) {
				if(options.common.is_last_file)
					*options.common.align_reservoir_samples= 0U;
				else {
					*options.common.align_reservoir_samples= align_remainder;
					data_bytes-= (*options.common.align_reservoir_samples)*bytes_per_frame;
				}
			}

			/* now do from the file */
			while(status==NORMAL && data_bytes>0) {
				size_t bytes_read= fread(ucbuffer, 1U, min(data_bytes, CHUNK_OF_SAMPLES*bytes_per_frame), infile);

				if(bytes_read==0U) {
					if(ferror(infile)) {
						fprintf(stderr, "%s: ERROR during read\n", encoder_session.inbasefilename);
						status= ERROR;
					}
					else if(feof(infile)) {
						fprintf(stderr, "%s: WARNING: unexpected EOF; expected %u samples, got %u samples\n", encoder_session.inbasefilename, (unsigned int)encoder_session.total_samples_to_encode, (unsigned int)encoder_session.samples_written);
						data_bytes= 0;
					}
				}
				else {
					if(bytes_read % bytes_per_frame != 0U) {
						fprintf(stderr, "%s: ERROR: got partial sample\n", encoder_session.inbasefilename);
						status= ERROR;
					}
					else {
						unsigned int frames= bytes_read/bytes_per_frame;
						format_input(input, frames, true, false, channels, bps);

						if(!FLAC__stream_encoder_process(encoder_session.encoder, (const FLAC__int32 *const *)input, frames)) {
							fprintf(stderr, "%s: ERROR during encoding, state = %d:%s\n", encoder_session.inbasefilename, FLAC__stream_encoder_get_state(encoder_session.encoder), FLAC__StreamEncoderStateString[FLAC__stream_encoder_get_state(encoder_session.encoder)]);
							encoder_session.verify_failed = (FLAC__stream_encoder_get_state(encoder_session.encoder) == FLAC__STREAM_ENCODER_VERIFY_MISMATCH_IN_AUDIO_DATA);
							status= ERROR;
						}
						else
							data_bytes-= bytes_read;
					}
				}
			}

			/* now read unaligned samples into reservoir or pad with zeroes if necessary */
			if(status==NORMAL && options.common.sector_align) {
				if(options.common.is_last_file) {
					unsigned int pad_frames= 588U-align_remainder;

					if(pad_frames<588U) {
						unsigned int i;

						info_align_zero= pad_frames;
						for(i= 0U; i<channels; ++i)
							memset(input[i], 0, pad_frames*(bps>>3));

						if(!FLAC__stream_encoder_process(encoder_session.encoder, (const FLAC__int32 *const *)input, pad_frames)) {
							fprintf(stderr, "%s: ERROR during encoding, state = %d:%s\n", encoder_session.inbasefilename, FLAC__stream_encoder_get_state(encoder_session.encoder), FLAC__StreamEncoderStateString[FLAC__stream_encoder_get_state(encoder_session.encoder)]);
							encoder_session.verify_failed = (FLAC__stream_encoder_get_state(encoder_session.encoder) == FLAC__STREAM_ENCODER_VERIFY_MISMATCH_IN_AUDIO_DATA);
							status= ERROR;
						}
					}
				}
				else {
					if(*options.common.align_reservoir_samples > 0) {
						size_t bytes_read= fread(ucbuffer, 1U, (*options.common.align_reservoir_samples)*bytes_per_frame, infile);

						FLAC__ASSERT(CHUNK_OF_SAMPLES>=588U);
						if(bytes_read==0U && ferror(infile)) {
							fprintf(stderr, "%s: ERROR during read\n", encoder_session.inbasefilename);
							status= ERROR;
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

			if(status==NORMAL && pad==true) {
				unsigned char tmp;

				if(fread(&tmp, 1U, 1U, infile)<1U) {
					fprintf(stderr, "%s: ERROR during read of SSND pad byte\n", encoder_session.inbasefilename);
					status= ERROR;
				}
			}

			got_ssnd_chunk= true;
		}
		else if(status==NORMAL) { /* other chunk */
			if(!strncmp(chunk_id, "COMM", 4))
				fprintf(stderr, "%s: WARNING: skipping extra 'COMM' chunk\n", encoder_session.inbasefilename);
			else if(!strncmp(chunk_id, "SSND", 4))
				fprintf(stderr, "%s: WARNING: skipping extra 'SSND' chunk\n", encoder_session.inbasefilename);
			else
				fprintf(stderr, "%s: WARNING: skipping unknown chunk '%s'\n", encoder_session.inbasefilename, chunk_id);

			/* chunk size */
			if(!read_big_endian_uint32(infile, &xx, false, encoder_session.inbasefilename))
				status= ERROR;
			else {
				unsigned long skip= xx+(xx & 1U);

				FLAC__ASSERT(skip<=LONG_MAX);
				while(status==NORMAL && skip>0U && fseek(infile, skip, SEEK_CUR)<0) {
					unsigned int need= min(skip, sizeof ucbuffer);
					if(fread(ucbuffer, 1U, need, infile)<need) {
						fprintf(stderr, "%s: ERROR during read while skipping unknown chunk\n", encoder_session.inbasefilename);
						status= ERROR;
					}
					skip-= need;
				}
			}
		}
	}

	if(got_ssnd_chunk==false && sample_frames!=0U) {
		fprintf(stderr, "%s: ERROR: missing SSND chunk\n", encoder_session.inbasefilename);
		status= ERROR;
	}

	if(encoder_session.encoder)
		FLAC__stream_encoder_finish(encoder_session.encoder);
	if(encoder_session.verbose && encoder_session.total_samples_to_encode > 0) {
		if(status==DONE)
			print_stats(&encoder_session);
		fprintf(stderr, "\n");
	}

	if(encoder_session.verify_failed) {
		fprintf(stderr, "Verify FAILED!  Do not trust %s\n", outfilename);
		status= MISMATCH;
	}

	if(status==DONE) {
		if(info_align_carry >= 0)
			fprintf(stderr, "%s: INFO: sector alignment causing %d samples to be carried over\n", encoder_session.inbasefilename, info_align_carry);
		if(info_align_zero >= 0)
			fprintf(stderr, "%s: INFO: sector alignment causing %d zero samples to be appended\n", encoder_session.inbasefilename, info_align_zero);
	}
	else if(status==ERROR)
		unlink(outfilename);

	EncoderSession_destroy(&encoder_session);

	return status==ERROR || status==MISMATCH;
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

	FLAC__ASSERT(!options.common.sector_align || options.common.skip == 0);

	(void)infilesize;
	(void)lookahead;
	(void)lookahead_length;

	if(!EncoderSession_construct(&encoder_session, options.common.use_ogg, options.common.verify, options.common.verbose, infile, infilename, outfilename))
		return 1;

	/*
	 * lookahead[] already has "RIFFxxxxWAVE", do sub-chunks
	 */
	while(!feof(infile)) {
		if(!read_little_endian_uint32(infile, &xx, true, encoder_session.inbasefilename))
			goto wav_abort_;
		if(feof(infile))
			break;
		if(xx == 0x20746d66 && !got_fmt_chunk) { /* "fmt " */
			/* fmt sub-chunk size */
			if(!read_little_endian_uint32(infile, &xx, false, encoder_session.inbasefilename))
				goto wav_abort_;
			if(xx < 16) {
				fprintf(stderr, "%s: ERROR: found non-standard 'fmt ' sub-chunk which has length = %u\n", encoder_session.inbasefilename, (unsigned)xx);
				goto wav_abort_;
			}
			else if(xx != 16 && xx != 18) {
				fprintf(stderr, "%s: WARNING: found non-standard 'fmt ' sub-chunk which has length = %u\n", encoder_session.inbasefilename, (unsigned)xx);
			}
			data_bytes = xx;
			/* compression code */
			if(!read_little_endian_uint16(infile, &x, false, encoder_session.inbasefilename))
				goto wav_abort_;
			if(x != 1) {
				fprintf(stderr, "%s: ERROR: unsupported compression type %u\n", encoder_session.inbasefilename, (unsigned)x);
				goto wav_abort_;
			}
			/* number of channels */
			if(!read_little_endian_uint16(infile, &x, false, encoder_session.inbasefilename))
				goto wav_abort_;
			if(x == 0 || x > FLAC__MAX_CHANNELS) {
				fprintf(stderr, "%s: ERROR: unsupported number channels %u\n", encoder_session.inbasefilename, (unsigned)x);
				goto wav_abort_;
			}
			else if(options.common.sector_align && x != 2) {
				fprintf(stderr, "%s: ERROR: file has %u channels, must be 2 for --sector-align\n", encoder_session.inbasefilename, (unsigned)x);
				goto wav_abort_;
			}
			channels = x;
			/* sample rate */
			if(!read_little_endian_uint32(infile, &xx, false, encoder_session.inbasefilename))
				goto wav_abort_;
			if(!FLAC__format_sample_rate_is_valid(xx)) {
				fprintf(stderr, "%s: ERROR: unsupported sample rate %u\n", encoder_session.inbasefilename, (unsigned)xx);
				goto wav_abort_;
			}
			else if(options.common.sector_align && xx != 44100) {
				fprintf(stderr, "%s: ERROR: file's sample rate is %u, must be 44100 for --sector-align\n", encoder_session.inbasefilename, (unsigned)xx);
				goto wav_abort_;
			}
			sample_rate = xx;
			/* avg bytes per second (ignored) */
			if(!read_little_endian_uint32(infile, &xx, false, encoder_session.inbasefilename))
				goto wav_abort_;
			/* block align (ignored) */
			if(!read_little_endian_uint16(infile, &x, false, encoder_session.inbasefilename))
				goto wav_abort_;
			/* bits per sample */
			if(!read_little_endian_uint16(infile, &x, false, encoder_session.inbasefilename))
				goto wav_abort_;
			if(x != 8 && x != 16 && x != 24) {
				fprintf(stderr, "%s: ERROR: unsupported bits per sample %u\n", encoder_session.inbasefilename, (unsigned)x);
				goto wav_abort_;
			}
			else if(options.common.sector_align && x != 16) {
				fprintf(stderr, "%s: ERROR: file has %u bits per sample, must be 16 for --sector-align\n", encoder_session.inbasefilename, (unsigned)x);
				goto wav_abort_;
			}
			bps = x;
			is_unsigned_samples = (x == 8);

			/* skip any extra data in the fmt sub-chunk */
			data_bytes -= 16;
			if(data_bytes > 0) {
				unsigned left, need;
				for(left = data_bytes; left > 0; ) {
					need = min(left, CHUNK_OF_SAMPLES);
					if(fread(ucbuffer, 1U, need, infile) < need) {
						fprintf(stderr, "%s: ERROR during read while skipping samples\n", encoder_session.inbasefilename);
						goto wav_abort_;
					}
					left -= need;
				}
			}

			got_fmt_chunk = true;
		}
		else if(xx == 0x61746164 && !got_data_chunk && got_fmt_chunk) { /* "data" */
			/* data size */
			if(!read_little_endian_uint32(infile, &xx, false, encoder_session.inbasefilename))
				goto wav_abort_;
			data_bytes = xx;

			bytes_per_wide_sample = channels * (bps >> 3);

			if(options.common.skip > 0) {
				if(fseek(infile, bytes_per_wide_sample * (unsigned)options.common.skip, SEEK_CUR) < 0) {
					/* can't seek input, read ahead manually... */
					unsigned left, need;
					for(left = (unsigned)options.common.skip; left > 0; ) { /*@@@ WATCHOUT: 4GB limit */
						need = min(left, CHUNK_OF_SAMPLES);
						if(fread(ucbuffer, bytes_per_wide_sample, need, infile) < need) {
							fprintf(stderr, "%s: ERROR during read while skipping samples\n", encoder_session.inbasefilename);
							goto wav_abort_;
						}
						left -= need;
					}
				}
			}

			data_bytes -= (unsigned)options.common.skip * bytes_per_wide_sample; /*@@@ WATCHOUT: 4GB limit */
			encoder_session.total_samples_to_encode = data_bytes / bytes_per_wide_sample + *options.common.align_reservoir_samples;
			if(options.common.sector_align) {
				align_remainder = (unsigned)(encoder_session.total_samples_to_encode % 588);
				if(options.common.is_last_file)
					encoder_session.total_samples_to_encode += (588-align_remainder); /* will pad with zeroes */
				else
					encoder_session.total_samples_to_encode -= align_remainder; /* will stop short and carry over to next file */
			}

			/* +44 for the size of the WAV headers; this is just an estimate for the progress indicator and doesn't need to be exact */
			encoder_session.unencoded_size = encoder_session.total_samples_to_encode * bytes_per_wide_sample + 44;

			if(!init_encoder(options.common, channels, bps, sample_rate, &encoder_session))
				goto wav_abort_;

			/*
			 * first do any samples in the reservoir
			 */
			if(options.common.sector_align && *options.common.align_reservoir_samples > 0) {
				if(!FLAC__stream_encoder_process(encoder_session.encoder, (const FLAC__int32 * const *)options.common.align_reservoir, *options.common.align_reservoir_samples)) {
					fprintf(stderr, "%s: ERROR during encoding, state = %d:%s\n", encoder_session.inbasefilename, FLAC__stream_encoder_get_state(encoder_session.encoder), FLAC__StreamEncoderStateString[FLAC__stream_encoder_get_state(encoder_session.encoder)]);
					encoder_session.verify_failed = (FLAC__stream_encoder_get_state(encoder_session.encoder) == FLAC__STREAM_ENCODER_VERIFY_MISMATCH_IN_AUDIO_DATA);
					goto wav_abort_;
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
				bytes_read = fread(ucbuffer, sizeof(unsigned char), min(data_bytes, CHUNK_OF_SAMPLES * bytes_per_wide_sample), infile);
				if(bytes_read == 0) {
					if(ferror(infile)) {
						fprintf(stderr, "%s: ERROR during read\n", encoder_session.inbasefilename);
						goto wav_abort_;
					}
					else if(feof(infile)) {
						fprintf(stderr, "%s: WARNING: unexpected EOF; expected %u samples, got %u samples\n", encoder_session.inbasefilename, (unsigned)encoder_session.total_samples_to_encode, (unsigned)encoder_session.samples_written);
						data_bytes = 0;
					}
				}
				else {
					if(bytes_read % bytes_per_wide_sample != 0) {
						fprintf(stderr, "%s: ERROR: got partial sample\n", encoder_session.inbasefilename);
						goto wav_abort_;
					}
					else {
						unsigned wide_samples = bytes_read / bytes_per_wide_sample;
						format_input(input, wide_samples, false, is_unsigned_samples, channels, bps);

						if(!FLAC__stream_encoder_process(encoder_session.encoder, (const FLAC__int32 * const *)input, wide_samples)) {
							fprintf(stderr, "%s: ERROR during encoding, state = %d:%s\n", encoder_session.inbasefilename, FLAC__stream_encoder_get_state(encoder_session.encoder), FLAC__StreamEncoderStateString[FLAC__stream_encoder_get_state(encoder_session.encoder)]);
							encoder_session.verify_failed = (FLAC__stream_encoder_get_state(encoder_session.encoder) == FLAC__STREAM_ENCODER_VERIFY_MISMATCH_IN_AUDIO_DATA);
							goto wav_abort_;
						}
						data_bytes -= bytes_read;
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
							memset(input[channel], 0, data_bytes);

						if(!FLAC__stream_encoder_process(encoder_session.encoder, (const FLAC__int32 * const *)input, wide_samples)) {
							fprintf(stderr, "%s: ERROR during encoding, state = %d:%s\n", encoder_session.inbasefilename, FLAC__stream_encoder_get_state(encoder_session.encoder), FLAC__StreamEncoderStateString[FLAC__stream_encoder_get_state(encoder_session.encoder)]);
							encoder_session.verify_failed = (FLAC__stream_encoder_get_state(encoder_session.encoder) == FLAC__STREAM_ENCODER_VERIFY_MISMATCH_IN_AUDIO_DATA);
							goto wav_abort_;
						}
					}
				}
				else {
					if(*options.common.align_reservoir_samples > 0) {
						FLAC__ASSERT(CHUNK_OF_SAMPLES >= 588);
						bytes_read = fread(ucbuffer, sizeof(unsigned char), (*options.common.align_reservoir_samples) * bytes_per_wide_sample, infile);
						if(bytes_read == 0 && ferror(infile)) {
							fprintf(stderr, "%s: ERROR during read\n", encoder_session.inbasefilename);
							goto wav_abort_;
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
					goto wav_abort_;
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
				goto wav_abort_;
			if(fseek(infile, xx, SEEK_CUR) < 0) {
				/* can't seek input, read ahead manually... */
				unsigned left, need;
				const unsigned chunk = sizeof(ucbuffer);
				for(left = xx; left > 0; ) {
					need = min(left, chunk);
					if(fread(ucbuffer, 1, need, infile) < need) {
						fprintf(stderr, "%s: ERROR during read while skipping unsupported sub-chunk\n", encoder_session.inbasefilename);
						goto wav_abort_;
					}
					left -= need;
				}
			}
		}
	}

	if(encoder_session.encoder)
		FLAC__stream_encoder_finish(encoder_session.encoder);
	if(encoder_session.verbose && encoder_session.total_samples_to_encode > 0) {
		print_stats(&encoder_session);
		fprintf(stderr, "\n");
	}
	if(encoder_session.verify_failed) {
		fprintf(stderr, "Verify FAILED!  Do not trust %s\n", outfilename);
		return 1;
	}
	if(info_align_carry >= 0)
		fprintf(stderr, "%s: INFO: sector alignment causing %d samples to be carried over\n", encoder_session.inbasefilename, info_align_carry);
	if(info_align_zero >= 0)
		fprintf(stderr, "%s: INFO: sector alignment causing %d zero samples to be appended\n", encoder_session.inbasefilename, info_align_zero);
	EncoderSession_destroy(&encoder_session);
	return 0;
wav_abort_:
	if(encoder_session.verbose && encoder_session.total_samples_to_encode > 0)
		fprintf(stderr, "\n");
	EncoderSession_destroy(&encoder_session);
	if(encoder_session.verify_failed)
		fprintf(stderr, "Verify FAILED!  Do not trust %s\n", outfilename);
	else
		unlink(outfilename);
	return 1;
}

int flac__encode_raw(FILE *infile, long infilesize, const char *infilename, const char *outfilename, const FLAC__byte *lookahead, unsigned lookahead_length, raw_encode_options_t options)
{
	EncoderSession encoder_session;
	size_t bytes_read;
	const size_t bytes_per_wide_sample = options.channels * (options.bps >> 3);
	unsigned align_remainder = 0;
	int info_align_carry = -1, info_align_zero = -1;

	FLAC__ASSERT(!options.common.sector_align || options.common.skip == 0);
	FLAC__ASSERT(!options.common.sector_align || options.channels == 2);
	FLAC__ASSERT(!options.common.sector_align || options.bps == 16);
	FLAC__ASSERT(!options.common.sector_align || options.sample_rate == 44100);
	FLAC__ASSERT(!options.common.sector_align || infilesize >= 0);

	if(!EncoderSession_construct(&encoder_session, options.common.use_ogg, options.common.verify, options.common.verbose, infile, infilename, outfilename))
		return 1;

	/* get the file length */
	if(infilesize < 0) {
		encoder_session.total_samples_to_encode = encoder_session.unencoded_size = 0;
	}
	else {
		if(options.common.sector_align) {
			FLAC__ASSERT(options.common.skip == 0);
			encoder_session.total_samples_to_encode = (unsigned)infilesize / bytes_per_wide_sample + *options.common.align_reservoir_samples;
			align_remainder = (unsigned)(encoder_session.total_samples_to_encode % 588);
			if(options.common.is_last_file)
				encoder_session.total_samples_to_encode += (588-align_remainder); /* will pad with zeroes */
			else
				encoder_session.total_samples_to_encode -= align_remainder; /* will stop short and carry over to next file */
		}
		else {
			encoder_session.total_samples_to_encode = (unsigned)infilesize / bytes_per_wide_sample - options.common.skip;
		}

		encoder_session.unencoded_size = encoder_session.total_samples_to_encode * bytes_per_wide_sample;
	}

	if(encoder_session.verbose && encoder_session.total_samples_to_encode <= 0)
		fprintf(stderr, "(No runtime statistics possible; please wait for encoding to finish...)\n");

	if(options.common.skip > 0) {
		unsigned skip_bytes = bytes_per_wide_sample * (unsigned)options.common.skip;
		if(skip_bytes > lookahead_length) {
			skip_bytes -= lookahead_length;
			lookahead_length = 0;
			if(fseek(infile, (long)skip_bytes, SEEK_CUR) < 0) {
				/* can't seek input, read ahead manually... */
				unsigned left, need;
				const unsigned chunk = sizeof(ucbuffer);
				for(left = skip_bytes; left > 0; ) {
					need = min(left, chunk);
					if(fread(ucbuffer, 1, need, infile) < need) {
						fprintf(stderr, "%s: ERROR during read while skipping samples\n", encoder_session.inbasefilename);
						goto raw_abort_;
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

	if(!init_encoder(options.common, options.channels, options.bps, options.sample_rate, &encoder_session))
		goto raw_abort_;

	/*
	 * first do any samples in the reservoir
	 */
	if(options.common.sector_align && *options.common.align_reservoir_samples > 0) {
		if(!FLAC__stream_encoder_process(encoder_session.encoder, (const FLAC__int32 * const *)options.common.align_reservoir, *options.common.align_reservoir_samples)) {
			fprintf(stderr, "%s: ERROR during encoding, state = %d:%s\n", encoder_session.inbasefilename, FLAC__stream_encoder_get_state(encoder_session.encoder), FLAC__StreamEncoderStateString[FLAC__stream_encoder_get_state(encoder_session.encoder)]);
			encoder_session.verify_failed = (FLAC__stream_encoder_get_state(encoder_session.encoder) == FLAC__STREAM_ENCODER_VERIFY_MISMATCH_IN_AUDIO_DATA);
			goto raw_abort_;
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
		}
	}

	/*
	 * now do from the file
	 */
	while(!feof(infile)) {
		if(lookahead_length > 0) {
			FLAC__ASSERT(lookahead_length < CHUNK_OF_SAMPLES * bytes_per_wide_sample);
			memcpy(ucbuffer, lookahead, lookahead_length);
			bytes_read = fread(ucbuffer+lookahead_length, sizeof(unsigned char), CHUNK_OF_SAMPLES * bytes_per_wide_sample - lookahead_length, infile) + lookahead_length;
			if(ferror(infile)) {
				fprintf(stderr, "%s: ERROR during read\n", encoder_session.inbasefilename);
				goto raw_abort_;
			}
			lookahead_length = 0;
		}
		else
			bytes_read = fread(ucbuffer, sizeof(unsigned char), CHUNK_OF_SAMPLES * bytes_per_wide_sample, infile);

		if(bytes_read == 0) {
			if(ferror(infile)) {
				fprintf(stderr, "%s: ERROR during read\n", encoder_session.inbasefilename);
				goto raw_abort_;
			}
		}
		else if(bytes_read % bytes_per_wide_sample != 0) {
			fprintf(stderr, "%s: ERROR: got partial sample\n", encoder_session.inbasefilename);
			goto raw_abort_;
		}
		else {
			unsigned wide_samples = bytes_read / bytes_per_wide_sample;
			format_input(input, wide_samples, options.is_big_endian, options.is_unsigned_samples, options.channels, options.bps);

			if(!FLAC__stream_encoder_process(encoder_session.encoder, (const FLAC__int32 * const *)input, wide_samples)) {
				fprintf(stderr, "%s: ERROR during encoding, state = %d:%s\n", encoder_session.inbasefilename, FLAC__stream_encoder_get_state(encoder_session.encoder), FLAC__StreamEncoderStateString[FLAC__stream_encoder_get_state(encoder_session.encoder)]);
				encoder_session.verify_failed = (FLAC__stream_encoder_get_state(encoder_session.encoder) == FLAC__STREAM_ENCODER_VERIFY_MISMATCH_IN_AUDIO_DATA);
				goto raw_abort_;
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
					memset(input[channel], 0, data_bytes);

				if(!FLAC__stream_encoder_process(encoder_session.encoder, (const FLAC__int32 * const *)input, wide_samples)) {
					fprintf(stderr, "%s: ERROR during encoding, state = %d:%s\n", encoder_session.inbasefilename, FLAC__stream_encoder_get_state(encoder_session.encoder), FLAC__StreamEncoderStateString[FLAC__stream_encoder_get_state(encoder_session.encoder)]);
					encoder_session.verify_failed = (FLAC__stream_encoder_get_state(encoder_session.encoder) == FLAC__STREAM_ENCODER_VERIFY_MISMATCH_IN_AUDIO_DATA);
					goto raw_abort_;
				}
			}
		}
		else {
			if(*options.common.align_reservoir_samples > 0) {
				FLAC__ASSERT(CHUNK_OF_SAMPLES >= 588);
				bytes_read = fread(ucbuffer, sizeof(unsigned char), (*options.common.align_reservoir_samples) * bytes_per_wide_sample, infile);
				if(bytes_read == 0 && ferror(infile)) {
					fprintf(stderr, "%s: ERROR during read\n", encoder_session.inbasefilename);
					goto raw_abort_;
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

	if(encoder_session.encoder)
		FLAC__stream_encoder_finish(encoder_session.encoder);
	if(encoder_session.verbose && encoder_session.total_samples_to_encode > 0) {
		print_stats(&encoder_session);
		fprintf(stderr, "\n");
	}
	if(encoder_session.verify_failed) {
		fprintf(stderr, "Verify FAILED!  Do not trust %s\n", outfilename);
		return 1;
	}
	if(info_align_carry >= 0)
		fprintf(stderr, "%s: INFO: sector alignment causing %d samples to be carried over\n", encoder_session.inbasefilename, info_align_carry);
	if(info_align_zero >= 0)
		fprintf(stderr, "%s: INFO: sector alignment causing %d zero samples to be appended\n", encoder_session.inbasefilename, info_align_zero);
	EncoderSession_destroy(&encoder_session);
	return 0;
raw_abort_:
	if(encoder_session.verbose && encoder_session.total_samples_to_encode > 0)
		fprintf(stderr, "\n");
	EncoderSession_destroy(&encoder_session);
	if(encoder_session.verify_failed)
		fprintf(stderr, "Verify FAILED!  Do not trust %s\n", outfilename);
	else
		unlink(outfilename);
	return 1;
}

FLAC__bool init_encoder(encode_options_t options, unsigned channels, unsigned bps, unsigned sample_rate, EncoderSession *encoder_session)
{
	unsigned num_metadata;
	FLAC__StreamMetadata padding;
	FLAC__StreamMetadata *metadata[2];

	if(channels != 2)
		options.do_mid_side = options.loose_mid_side = false;

	if(!convert_to_seek_table_template(options.requested_seek_points, options.num_requested_seek_points, encoder_session->total_samples_to_encode, encoder_session->seek_table_template)) {
		fprintf(stderr, "%s: ERROR allocating memory for seek table\n", encoder_session->inbasefilename);
		return false;
	}

	num_metadata = 0;
	if(encoder_session->seek_table_template->data.seek_table.num_points > 0) {
		encoder_session->seek_table_template->is_last = false; /* the encoder will set this for us */
		metadata[num_metadata++] = encoder_session->seek_table_template;
	}
	if(options.padding > 0) {
		padding.is_last = false; /* the encoder will set this for us */
		padding.type = FLAC__METADATA_TYPE_PADDING;
		padding.length = (unsigned)options.padding;
		metadata[num_metadata++] = &padding;
	}

	FLAC__stream_encoder_set_verify(encoder_session->encoder, options.verify);
	FLAC__stream_encoder_set_streamable_subset(encoder_session->encoder, !options.lax);
	FLAC__stream_encoder_set_do_mid_side_stereo(encoder_session->encoder, options.do_mid_side);
	FLAC__stream_encoder_set_loose_mid_side_stereo(encoder_session->encoder, options.loose_mid_side);
	FLAC__stream_encoder_set_channels(encoder_session->encoder, channels);
	FLAC__stream_encoder_set_bits_per_sample(encoder_session->encoder, bps);
	FLAC__stream_encoder_set_sample_rate(encoder_session->encoder, sample_rate);
	FLAC__stream_encoder_set_blocksize(encoder_session->encoder, options.blocksize);
	FLAC__stream_encoder_set_max_lpc_order(encoder_session->encoder, options.max_lpc_order);
	FLAC__stream_encoder_set_qlp_coeff_precision(encoder_session->encoder, options.qlp_coeff_precision);
	FLAC__stream_encoder_set_do_qlp_coeff_prec_search(encoder_session->encoder, options.do_qlp_coeff_prec_search);
	FLAC__stream_encoder_set_do_escape_coding(encoder_session->encoder, options.do_escape_coding);
	FLAC__stream_encoder_set_do_exhaustive_model_search(encoder_session->encoder, options.do_exhaustive_model_search);
	FLAC__stream_encoder_set_min_residual_partition_order(encoder_session->encoder, options.min_residual_partition_order);
	FLAC__stream_encoder_set_max_residual_partition_order(encoder_session->encoder, options.max_residual_partition_order);
	FLAC__stream_encoder_set_rice_parameter_search_dist(encoder_session->encoder, options.rice_parameter_search_dist);
	FLAC__stream_encoder_set_total_samples_estimate(encoder_session->encoder, encoder_session->total_samples_to_encode);
	FLAC__stream_encoder_set_metadata(encoder_session->encoder, (num_metadata > 0)? metadata : 0, num_metadata);
	FLAC__stream_encoder_set_write_callback(encoder_session->encoder, write_callback);
	FLAC__stream_encoder_set_metadata_callback(encoder_session->encoder, metadata_callback);
	FLAC__stream_encoder_set_client_data(encoder_session->encoder, encoder_session);

	if(FLAC__stream_encoder_init(encoder_session->encoder) != FLAC__STREAM_ENCODER_OK) {
		fprintf(stderr, "%s: ERROR initializing encoder, state = %d:%s\n", encoder_session->inbasefilename, FLAC__stream_encoder_get_state(encoder_session->encoder), FLAC__StreamEncoderStateString[FLAC__stream_encoder_get_state(encoder_session->encoder)]);
		encoder_session->verify_failed = (FLAC__stream_encoder_get_state(encoder_session->encoder) == FLAC__STREAM_ENCODER_VERIFY_MISMATCH_IN_AUDIO_DATA);
		return false;
	}

	return true;
}

FLAC__bool convert_to_seek_table_template(char *requested_seek_points, int num_requested_seek_points, FLAC__uint64 stream_samples, FLAC__StreamMetadata *seek_table_template)
{
	unsigned i, real_points, placeholders;
	char *pt = requested_seek_points, *q;

	if(num_requested_seek_points == 0)
		return true;

	if(num_requested_seek_points < 0) {
		strcpy(requested_seek_points, "100x<");
		num_requested_seek_points = 1;
	}

	/* first count how many individual seek points we may need */
	real_points = placeholders = 0;
	for(i = 0; i < (unsigned)num_requested_seek_points; i++) {
		q = strchr(pt, '<');
		FLAC__ASSERT(0 != q);
		*q = '\0';

		if(0 == strcmp(pt, "X")) { /* -S X */
			placeholders++;
		}
		else if(pt[strlen(pt)-1] == 'x') { /* -S #x */
			if(stream_samples > 0) /* we can only do these if we know the number of samples to encode up front */
				real_points += (unsigned)atoi(pt);
		}
		else { /* -S # */
			real_points++;
		}
		*q++ = '<';

		pt = q;
	}
	pt = requested_seek_points;

	for(i = 0; i < (unsigned)num_requested_seek_points; i++) {
		q = strchr(pt, '<');
		FLAC__ASSERT(0 != q);
		*q++ = '\0';

		if(0 == strcmp(pt, "X")) { /* -S X */
			if(!FLAC__metadata_object_seektable_template_append_placeholders(seek_table_template, 1))
				return false;
		}
		else if(pt[strlen(pt)-1] == 'x') { /* -S #x */
			if(stream_samples > 0) { /* we can only do these if we know the number of samples to encode up front */
				if(!FLAC__metadata_object_seektable_template_append_spaced_points(seek_table_template, atoi(pt), stream_samples))
					return false;
			}
		}
		else { /* -S # */
			FLAC__uint64 n = (unsigned)atoi(pt);
			if(!FLAC__metadata_object_seektable_template_append_point(seek_table_template, n))
				return false;
		}

		pt = q;
	}

	if(!FLAC__metadata_object_seektable_template_sort(seek_table_template, /*compact=*/true))
		return false;

	return true;
}

void format_input(FLAC__int32 *dest[], unsigned wide_samples, FLAC__bool is_big_endian, FLAC__bool is_unsigned_samples, unsigned channels, unsigned bps)
{
	unsigned wide_sample, sample, channel, byte;

	if(bps == 8) {
		if(is_unsigned_samples) {
			for(sample = wide_sample = 0; wide_sample < wide_samples; wide_sample++)
				for(channel = 0; channel < channels; channel++, sample++)
					dest[channel][wide_sample] = (FLAC__int32)ucbuffer[sample] - 0x80;
		}
		else {
			for(sample = wide_sample = 0; wide_sample < wide_samples; wide_sample++)
				for(channel = 0; channel < channels; channel++, sample++)
					dest[channel][wide_sample] = (FLAC__int32)scbuffer[sample];
		}
	}
	else if(bps == 16) {
		if(is_big_endian != is_big_endian_host) {
			unsigned char tmp;
			const unsigned bytes = wide_samples * channels * (bps >> 3);
			for(byte = 0; byte < bytes; byte += 2) {
				tmp = ucbuffer[byte];
				ucbuffer[byte] = ucbuffer[byte+1];
				ucbuffer[byte+1] = tmp;
			}
		}
		if(is_unsigned_samples) {
			for(sample = wide_sample = 0; wide_sample < wide_samples; wide_sample++)
				for(channel = 0; channel < channels; channel++, sample++)
					dest[channel][wide_sample] = (FLAC__int32)usbuffer[sample] - 0x8000;
		}
		else {
			for(sample = wide_sample = 0; wide_sample < wide_samples; wide_sample++)
				for(channel = 0; channel < channels; channel++, sample++)
					dest[channel][wide_sample] = (FLAC__int32)ssbuffer[sample];
		}
	}
	else if(bps == 24) {
		if(!is_big_endian) {
			unsigned char tmp;
			const unsigned bytes = wide_samples * channels * (bps >> 3);
			for(byte = 0; byte < bytes; byte += 3) {
				tmp = ucbuffer[byte];
				ucbuffer[byte] = ucbuffer[byte+2];
				ucbuffer[byte+2] = tmp;
			}
		}
		if(is_unsigned_samples) {
			for(byte = sample = wide_sample = 0; wide_sample < wide_samples; wide_sample++)
				for(channel = 0; channel < channels; channel++, sample++) {
					dest[channel][wide_sample]  = ucbuffer[byte++]; dest[channel][wide_sample] <<= 8;
					dest[channel][wide_sample] |= ucbuffer[byte++]; dest[channel][wide_sample] <<= 8;
					dest[channel][wide_sample] |= ucbuffer[byte++];
					dest[channel][wide_sample] -= 0x800000;
				}
		}
		else {
			for(byte = sample = wide_sample = 0; wide_sample < wide_samples; wide_sample++)
				for(channel = 0; channel < channels; channel++, sample++) {
					dest[channel][wide_sample]  = scbuffer[byte++]; dest[channel][wide_sample] <<= 8;
					dest[channel][wide_sample] |= ucbuffer[byte++]; dest[channel][wide_sample] <<= 8;
					dest[channel][wide_sample] |= ucbuffer[byte++];
				}
		}
	}
	else {
		FLAC__ASSERT(0);
	}
}

FLAC__StreamEncoderWriteStatus write_callback(const FLAC__StreamEncoder *encoder, const FLAC__byte buffer[], unsigned bytes, unsigned samples, unsigned current_frame, void *client_data)
{
	EncoderSession *encoder_session = (EncoderSession*)client_data;
	const unsigned mask = (FLAC__stream_encoder_get_do_exhaustive_model_search(encoder) || FLAC__stream_encoder_get_do_qlp_coeff_prec_search(encoder))? 0x1f : 0x7f;

	encoder_session->bytes_written += bytes;
	encoder_session->samples_written += samples;
	encoder_session->current_frame = current_frame;

	if(samples && encoder_session->verbose && encoder_session->total_samples_to_encode > 0 && !(current_frame & mask))
		print_stats(encoder_session);

	if(fwrite(buffer, sizeof(FLAC__byte), bytes, encoder_session->fout) == bytes)
		return FLAC__STREAM_ENCODER_WRITE_STATUS_OK;
	else
		return FLAC__STREAM_ENCODER_WRITE_STATUS_FATAL_ERROR;
}

#if 0
FLAC__StreamDecoderWriteStatus verify_write_callback(const FLAC__StreamDecoder *decoder, const FLAC__Frame *frame, const FLAC__int32 * const buffer[], void *client_data)
{
			fprintf(stderr, "\n%s: ERROR: mismatch in decoded data, verify FAILED!\n", encoder_session->inbasefilename);
			fprintf(stderr, "       Please submit a bug report to\n");
			fprintf(stderr, "           http://sourceforge.net/bugs/?func=addbug&group_id=13478\n");
			fprintf(stderr, "       Make sure to include an email contact in the comment and/or use the\n");
			fprintf(stderr, "       \"Monitor\" feature to monitor the bug status.\n");

			fprintf(stderr, "       Absolute sample=%u, frame=%u, channel=%u, sample=%u, expected %d, got %d\n", (unsigned)frame->header.number.sample_number + sample, (unsigned)frame->header.number.sample_number / FLAC__stream_decoder_get_blocksize(decoder), channel, sample, expect, got);
}
#endif

void print_stats(const EncoderSession *encoder_session)
{
#if defined _MSC_VER || defined __MINGW32__
	/* with VC++ you have to spoon feed it the casting */
	const double progress = (double)(FLAC__int64)encoder_session->samples_written / (double)(FLAC__int64)encoder_session->total_samples_to_encode;
	const double ratio = (double)(FLAC__int64)encoder_session->bytes_written / ((double)(FLAC__int64)encoder_session->unencoded_size * progress);
#else
	const double progress = (double)encoder_session->samples_written / (double)encoder_session->total_samples_to_encode;
	const double ratio = (double)encoder_session->bytes_written / ((double)encoder_session->unencoded_size * progress);
#endif

	if(encoder_session->samples_written == encoder_session->total_samples_to_encode) {
		fprintf(stderr, "\r%s:%s wrote %u bytes, ratio=%0.3f",
			encoder_session->inbasefilename,
			encoder_session->verify? (encoder_session->verify_failed? " Verify FAILED!" : " Verify OK,") : "",
			(unsigned)encoder_session->bytes_written,
			ratio
		);
	}
	else {
		fprintf(stderr, "\r%s: %u%% complete, ratio=%0.3f", encoder_session->inbasefilename, (unsigned)floor(progress * 100.0 + 0.5), ratio);
	}
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
		if(is_big_endian_host) {
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
		if(is_big_endian_host) {
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
	*val= (FLAC__uint32)(p>>shift)+(p>>(shift-1) & 0x1);

	return true;
}

FLAC__bool write_big_endian_uint16(FILE *f, FLAC__uint16 val)
{
	if(!is_big_endian_host) {
		FLAC__byte *b = (FLAC__byte *)&val, tmp;
		tmp = b[0]; b[0] = b[1]; b[1] = tmp;
	}
	return fwrite(&val, 1, 2, f) == 2;
}

FLAC__bool write_big_endian_uint64(FILE *f, FLAC__uint64 val)
{
	if(!is_big_endian_host) {
		FLAC__byte *b = (FLAC__byte *)&val, tmp;
		tmp = b[0]; b[0] = b[7]; b[7] = tmp;
		tmp = b[1]; b[1] = b[6]; b[6] = tmp;
		tmp = b[2]; b[2] = b[5]; b[5] = tmp;
		tmp = b[3]; b[3] = b[4]; b[4] = tmp;
	}
	return fwrite(&val, 1, 8, f) == 8;
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
#endif
	e->verify = verify;
	e->verbose = verbose;
	e->inbasefilename = flac__file_get_basename(infilename);
	e->outfilename = outfilename;

	e->verify_failed = false;
	e->unencoded_size = 0;
	e->total_samples_to_encode = 0;
	e->bytes_written = 0;
	e->samples_written = 0;
	e->current_frame = 0;

	e->encoder.flac = 0;
#ifdef FLAC__HAS_OGG
	e->encoder.ogg = 0;
#endif

	e->fin = infile;
	e->fout = 0;
	e->seek_table_template = 0;

	if(0 == strcmp(outfilename, "-")) {
		e->fout = file__get_binary_stdout();
	}
	else {
		if(0 == (e->fout = fopen(outfilename, "wb"))) {
			fprintf(stderr, "%s: ERROR: can't open output file %s\n", e->inbasefilename, outfilename);
			EncoderSession_destroy(e);
			return false;
		}
	}

	if(0 == (e->seek_table_template = FLAC__metadata_object_new(FLAC__METADATA_TYPE_SEEKTABLE)) {
		fprintf(stderr, "%s: ERROR allocating memory for seek table\n", e->inbasefilename);
		return false;
	}

#ifdef FLAC__HAS_OGG
	if(e->use_ogg) {
		e->encoder.ogg = OggFLAC__stream_encoder_new();
		if(0 == e->encoder.flac) {
			fprintf(stderr, "%s: ERROR creating the encoder instance\n", e->inbasefilename);
			EncoderSession_destroy(e);
			return false;
		}
	}
	else
#endif
	{
		e->encoder.flac = FLAC__file_encoder_new();
		if(0 == e->encoder.flac) {
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

#ifdef FLAC__HAS_OGG
	if(e->use_ogg) {
		if(0 != e->encoder.ogg) {
			OggFLAC__stream_encoder_delete(e->encoder.ogg);
			e->encoder.ogg = 0;
		}
	}
	else
#endif
	{
		if(0 != e->encoder.flac) {
			FLAC__file_encoder_delete(e->encoder.flac);
			e->encoder.flac = 0;
		}
	}

	if(0 != e->seek_table_template) {
		FLAC__metadata_object_delete(e->seek_table_template);
		e->seek_table_template = 0;
	}
}
