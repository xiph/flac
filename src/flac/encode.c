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
#include <stdlib.h> /* for malloc */
#include <string.h> /* for strcmp() */
#include "FLAC/all.h"
#include "encode.h"
#include "file.h"
#ifdef FLAC__HAS_OGG
#include "ogg/ogg.h"
#endif

#ifdef min
#undef min
#endif
#define min(x,y) ((x)<(y)?(x):(y))

/* this MUST be >= 588 so that sector aligning can take place with one read */
#define CHUNK_OF_SAMPLES 2048

typedef enum {
	FLAC__VERIFY_OK,
	FLAC__VERIFY_FAILED_IN_FRAME,
	FLAC__VERIFY_FAILED_IN_METADATA
} verify_code;

static const char *verify_code_string[] = {
	"FLAC__VERIFY_OK",
	"FLAC__VERIFY_FAILED_IN_FRAME",
	"FLAC__VERIFY_FAILED_IN_METADATA"
};

typedef struct {
	FLAC__int32 *original[FLAC__MAX_CHANNELS];
	unsigned size; /* of each original[] in samples */
	unsigned tail; /* in wide samples */
	const FLAC__byte *encoded_signal;
	unsigned encoded_signal_capacity;
	unsigned encoded_bytes;
	FLAC__bool into_frames;
	verify_code result;
	FLAC__StreamDecoder *decoder;
} verify_fifo_struct;

#ifdef FLAC__HAS_OGG
typedef struct {
	ogg_stream_state os;
	ogg_page og;
} ogg_info_struct;
#endif

typedef struct {
	const char *inbasefilename;
	FILE *fout;
	const char *outfilename;
	FLAC__StreamEncoder *encoder;
	FLAC__bool verify;
	FLAC__bool verbose;
	FLAC__uint64 unencoded_size;
	FLAC__uint64 total_samples_to_encode;
	FLAC__uint64 bytes_written;
	FLAC__uint64 samples_written;
	FLAC__uint64 stream_offset; /* i.e. number of bytes before the first byte of the the first frame's header */
	unsigned current_frame;
	verify_fifo_struct verify_fifo;
	FLAC__StreamMetaData_SeekTable seek_table;
	unsigned first_seek_point_to_check;
#ifdef FLAC__HAS_OGG
	FLAC__bool use_ogg;
	ogg_info_struct ogg;
#endif
} encoder_wrapper_struct;

static FLAC__bool is_big_endian_host;

static unsigned char ucbuffer[CHUNK_OF_SAMPLES*FLAC__MAX_CHANNELS*((FLAC__REFERENCE_CODEC_MAX_BITS_PER_SAMPLE+7)/8)];
static signed char *scbuffer = (signed char *)ucbuffer;
static FLAC__uint16 *usbuffer = (FLAC__uint16 *)ucbuffer;
static FLAC__int16 *ssbuffer = (FLAC__int16 *)ucbuffer;

static FLAC__int32 in[FLAC__MAX_CHANNELS][CHUNK_OF_SAMPLES];
static FLAC__int32 *input[FLAC__MAX_CHANNELS];

/* local routines */
static FLAC__bool init(encoder_wrapper_struct *encoder_wrapper);
static FLAC__bool init_encoder(encode_options_t options, unsigned channels, unsigned bps, unsigned sample_rate, encoder_wrapper_struct *encoder_wrapper);
static FLAC__bool convert_to_seek_table(char *requested_seek_points, int num_requested_seek_points, FLAC__uint64 stream_samples, unsigned blocksize, FLAC__StreamMetaData_SeekTable *seek_table);
static void append_point_to_seek_table(FLAC__StreamMetaData_SeekTable *seek_table, FLAC__uint64 sample, FLAC__uint64 stream_samples, FLAC__uint64 blocksize);
static int seekpoint_compare(const FLAC__StreamMetaData_SeekPoint *l, const FLAC__StreamMetaData_SeekPoint *r);
static void format_input(FLAC__int32 *dest[], unsigned wide_samples, FLAC__bool is_big_endian, FLAC__bool is_unsigned_samples, unsigned channels, unsigned bps, encoder_wrapper_struct *encoder_wrapper);
static void append_to_verify_fifo(encoder_wrapper_struct *encoder_wrapper, const FLAC__int32 *input[], unsigned channels, unsigned wide_samples);
static FLAC__StreamEncoderWriteStatus write_callback(const FLAC__StreamEncoder *encoder, const FLAC__byte buffer[], unsigned bytes, unsigned samples, unsigned current_frame, void *client_data);
static void metadata_callback(const FLAC__StreamEncoder *encoder, const FLAC__StreamMetaData *metadata, void *client_data);
static FLAC__StreamDecoderReadStatus verify_read_callback(const FLAC__StreamDecoder *decoder, FLAC__byte buffer[], unsigned *bytes, void *client_data);
static FLAC__StreamDecoderWriteStatus verify_write_callback(const FLAC__StreamDecoder *decoder, const FLAC__Frame *frame, const FLAC__int32 *buffer[], void *client_data);
static void verify_metadata_callback(const FLAC__StreamDecoder *decoder, const FLAC__StreamMetaData *metadata, void *client_data);
static void verify_error_callback(const FLAC__StreamDecoder *decoder, FLAC__StreamDecoderErrorStatus status, void *client_data);
static void print_stats(const encoder_wrapper_struct *encoder_wrapper);
static FLAC__bool read_little_endian_uint16(FILE *f, FLAC__uint16 *val, FLAC__bool eof_ok, const char *fn);
static FLAC__bool read_little_endian_uint32(FILE *f, FLAC__uint32 *val, FLAC__bool eof_ok, const char *fn);
static FLAC__bool write_big_endian_uint16(FILE *f, FLAC__uint16 val);
static FLAC__bool write_big_endian_uint64(FILE *f, FLAC__uint64 val);

int flac__encode_wav(FILE *infile, long infilesize, const char *infilename, const char *outfilename, const FLAC__byte *lookahead, unsigned lookahead_length, wav_encode_options_t options)
{
	encoder_wrapper_struct encoder_wrapper;
	FLAC__bool is_unsigned_samples = false;
	unsigned channels = 0, bps = 0, sample_rate = 0, data_bytes;
	size_t bytes_per_wide_sample, bytes_read;
	FLAC__uint16 x;
	FLAC__uint32 xx;
	FLAC__bool got_fmt_chunk = false, got_data_chunk = false;
	unsigned align_remainder = 0;
	int info_align_carry = -1, info_align_zero = -1;

	FLAC__ASSERT(!options.sector_align || options.common.skip == 0);

	encoder_wrapper.encoder = 0;
	encoder_wrapper.verify = options.common.verify;
	encoder_wrapper.verbose = options.common.verbose;
	encoder_wrapper.bytes_written = 0;
	encoder_wrapper.samples_written = 0;
	encoder_wrapper.stream_offset = 0;
	encoder_wrapper.inbasefilename = flac__file_get_basename(infilename);
	encoder_wrapper.outfilename = outfilename;
	encoder_wrapper.seek_table.points = 0;
	encoder_wrapper.first_seek_point_to_check = 0;
#ifdef FLAC__HAS_OGG
	encoder_wrapper.use_ogg = options.common.use_ogg;
#endif
	(void)infilesize;
	(void)lookahead;
	(void)lookahead_length;

	if(0 == strcmp(outfilename, "-")) {
		encoder_wrapper.fout = file__get_binary_stdout();
	}
	else {
		if(0 == (encoder_wrapper.fout = fopen(outfilename, "wb"))) {
			fprintf(stderr, "%s: ERROR: can't open output file %s\n", encoder_wrapper.inbasefilename, outfilename);
			if(infile != stdin)
				fclose(infile);
			return 1;
		}
	}

	if(!init(&encoder_wrapper))
		goto wav_abort_;

	/*
	 * lookahead[] already has "RIFFxxxxWAVE", do sub-chunks
	 */
	while(!feof(infile)) {
		if(!read_little_endian_uint32(infile, &xx, true, encoder_wrapper.inbasefilename))
			goto wav_abort_;
		if(feof(infile))
			break;
		if(xx == 0x20746d66) { /* "fmt " */
			if(got_fmt_chunk) {
				fprintf(stderr, "%s: WARNING: skipping extra 'fmt ' sub-chunk\n", encoder_wrapper.inbasefilename);
			}
			else {
				/* fmt sub-chunk size */
				if(!read_little_endian_uint32(infile, &xx, false, encoder_wrapper.inbasefilename))
					goto wav_abort_;
				if(xx < 16) {
					fprintf(stderr, "%s: ERROR: found non-standard 'fmt ' sub-chunk which has length = %u\n", encoder_wrapper.inbasefilename, (unsigned)xx);
					goto wav_abort_;
				}
				else if(xx != 16 && xx != 18) {
					fprintf(stderr, "%s: WARNING: found non-standard 'fmt ' sub-chunk which has length = %u\n", encoder_wrapper.inbasefilename, (unsigned)xx);
				}
				data_bytes = xx;
				/* compression code */
				if(!read_little_endian_uint16(infile, &x, false, encoder_wrapper.inbasefilename))
					goto wav_abort_;
				if(x != 1) {
					fprintf(stderr, "%s: ERROR: unsupported compression type %u\n", encoder_wrapper.inbasefilename, (unsigned)x);
					goto wav_abort_;
				}
				/* number of channels */
				if(!read_little_endian_uint16(infile, &x, false, encoder_wrapper.inbasefilename))
					goto wav_abort_;
				if(x == 0 || x > FLAC__MAX_CHANNELS) {
					fprintf(stderr, "%s: ERROR: unsupported number channels %u\n", encoder_wrapper.inbasefilename, (unsigned)x);
					goto wav_abort_;
				}
				else if(options.sector_align && x != 2) {
					fprintf(stderr, "%s: ERROR: file has %u channels, must be 2 for --sector-align\n", encoder_wrapper.inbasefilename, (unsigned)x);
					goto wav_abort_;
				}
				channels = x;
				/* sample rate */
				if(!read_little_endian_uint32(infile, &xx, false, encoder_wrapper.inbasefilename))
					goto wav_abort_;
				if(!FLAC__format_is_valid_sample_rate(xx)) {
					fprintf(stderr, "%s: ERROR: unsupported sample rate %u\n", encoder_wrapper.inbasefilename, (unsigned)xx);
					goto wav_abort_;
				}
				else if(options.sector_align && xx != 44100) {
					fprintf(stderr, "%s: ERROR: file's sample rate is %u, must be 44100 for --sector-align\n", encoder_wrapper.inbasefilename, (unsigned)xx);
					goto wav_abort_;
				}
				sample_rate = xx;
				/* avg bytes per second (ignored) */
				if(!read_little_endian_uint32(infile, &xx, false, encoder_wrapper.inbasefilename))
					goto wav_abort_;
				/* block align (ignored) */
				if(!read_little_endian_uint16(infile, &x, false, encoder_wrapper.inbasefilename))
					goto wav_abort_;
				/* bits per sample */
				if(!read_little_endian_uint16(infile, &x, false, encoder_wrapper.inbasefilename))
					goto wav_abort_;
				if(x != 8 && x != 16 && x != 24) {
					fprintf(stderr, "%s: ERROR: unsupported bits per sample %u\n", encoder_wrapper.inbasefilename, (unsigned)x);
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
						if(fread(ucbuffer, 1, need, infile) < need) {
							fprintf(stderr, "%s: ERROR during read while skipping samples\n", encoder_wrapper.inbasefilename);
							goto wav_abort_;
						}
						left -= need;
					}
				}

				got_fmt_chunk = true;
			}
		}
		else if(xx == 0x61746164) { /* "data" */
			if(got_data_chunk) {
				fprintf(stderr, "%s: WARNING: skipping extra 'data' sub-chunk\n", encoder_wrapper.inbasefilename);
			}
			else if(!got_fmt_chunk) {
				fprintf(stderr, "%s: ERROR: got 'data' sub-chunk before 'fmt' sub-chunk\n", encoder_wrapper.inbasefilename);
				goto wav_abort_;
			}
			else {
				/* data size */
				if(!read_little_endian_uint32(infile, &xx, false, encoder_wrapper.inbasefilename))
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
								fprintf(stderr, "%s: ERROR during read while skipping samples\n", encoder_wrapper.inbasefilename);
								goto wav_abort_;
							}
							left -= need;
						}
					}
				}

				data_bytes -= (unsigned)options.common.skip * bytes_per_wide_sample; /*@@@ WATCHOUT: 4GB limit */
				encoder_wrapper.total_samples_to_encode = data_bytes / bytes_per_wide_sample + *options.align_reservoir_samples;
				if(options.sector_align) {
					align_remainder = (unsigned)(encoder_wrapper.total_samples_to_encode % 588);
					if(options.is_last_file)
						encoder_wrapper.total_samples_to_encode += (588-align_remainder); /* will pad with zeroes */
					else
						encoder_wrapper.total_samples_to_encode -= align_remainder; /* will stop short and carry over to next file */
				}

				/* +44 for the size of the WAV headers; this is just an estimate for the progress indicator and doesn't need to be exact */
				encoder_wrapper.unencoded_size = encoder_wrapper.total_samples_to_encode * bytes_per_wide_sample + 44;

				if(!init_encoder(options.common, channels, bps, sample_rate, &encoder_wrapper))
					goto wav_abort_;

				encoder_wrapper.verify_fifo.into_frames = true;

				/*
				 * first do any samples in the reservoir
				 */
				if(options.sector_align && *options.align_reservoir_samples > 0) {
					/* NOTE: some versions of GCC can't figure out const-ness right and will give you an 'incompatible pointer type' warning on arg 2 here: */
					append_to_verify_fifo(&encoder_wrapper, options.align_reservoir, channels, *options.align_reservoir_samples);

					/* NOTE: some versions of GCC can't figure out const-ness right and will give you an 'incompatible pointer type' warning on arg 2 here: */
					if(!FLAC__stream_encoder_process(encoder_wrapper.encoder, options.align_reservoir, *options.align_reservoir_samples)) {
						fprintf(stderr, "%s: ERROR during encoding, state = %d:%s\n", encoder_wrapper.inbasefilename, FLAC__stream_encoder_get_state(encoder_wrapper.encoder), FLAC__StreamEncoderStateString[FLAC__stream_encoder_get_state(encoder_wrapper.encoder)]);
						goto wav_abort_;
					}
				}

				/*
				 * decrement the data_bytes counter if we need to align the file
				 */
				if(options.sector_align) {
					if(options.is_last_file) {
						*options.align_reservoir_samples = 0;
					}
					else {
						*options.align_reservoir_samples = align_remainder;
						data_bytes -= (*options.align_reservoir_samples) * bytes_per_wide_sample;
					}
				}

				/*
				 * now do from the file
				 */
				while(data_bytes > 0) {
					bytes_read = fread(ucbuffer, sizeof(unsigned char), min(data_bytes, CHUNK_OF_SAMPLES * bytes_per_wide_sample), infile);
					if(bytes_read == 0) {
						if(ferror(infile)) {
							fprintf(stderr, "%s: ERROR during read\n", encoder_wrapper.inbasefilename);
							goto wav_abort_;
						}
						else if(feof(infile)) {
							fprintf(stderr, "%s: WARNING: unexpected EOF; expected %u samples, got %u samples\n", encoder_wrapper.inbasefilename, (unsigned)encoder_wrapper.total_samples_to_encode, (unsigned)encoder_wrapper.samples_written);
							data_bytes = 0;
						}
					}
					else {
						if(bytes_read % bytes_per_wide_sample != 0) {
							fprintf(stderr, "%s: ERROR: got partial sample\n", encoder_wrapper.inbasefilename);
							goto wav_abort_;
						}
						else {
							unsigned wide_samples = bytes_read / bytes_per_wide_sample;
							format_input(input, wide_samples, false, is_unsigned_samples, channels, bps, &encoder_wrapper);

							/* NOTE: some versions of GCC can't figure out const-ness right and will give you an 'incompatible pointer type' warning on arg 2 here: */
							if(!FLAC__stream_encoder_process(encoder_wrapper.encoder, input, wide_samples)) {
								fprintf(stderr, "%s: ERROR during encoding, state = %d:%s\n", encoder_wrapper.inbasefilename, FLAC__stream_encoder_get_state(encoder_wrapper.encoder), FLAC__StreamEncoderStateString[FLAC__stream_encoder_get_state(encoder_wrapper.encoder)]);
								goto wav_abort_;
							}
							data_bytes -= bytes_read;
						}
					}
				}

				/*
				 * now read unaligned samples into reservoir or pad with zeroes if necessary
				 */
				if(options.sector_align) {
					if(options.is_last_file) {
						unsigned wide_samples = 588 - align_remainder;
						if(wide_samples < 588) {
							unsigned channel;

							info_align_zero = wide_samples;
							data_bytes = wide_samples * bytes_per_wide_sample;
							for(channel = 0; channel < channels; channel++)
								memset(input[channel], 0, data_bytes);
							/* NOTE: some versions of GCC can't figure out const-ness right and will give you an 'incompatible pointer type' warning on arg 2 here: */
							append_to_verify_fifo(&encoder_wrapper, input, channels, wide_samples);

							/* NOTE: some versions of GCC can't figure out const-ness right and will give you an 'incompatible pointer type' warning on arg 2 here: */
							if(!FLAC__stream_encoder_process(encoder_wrapper.encoder, input, wide_samples)) {
								fprintf(stderr, "%s: ERROR during encoding, state = %d:%s\n", encoder_wrapper.inbasefilename, FLAC__stream_encoder_get_state(encoder_wrapper.encoder), FLAC__StreamEncoderStateString[FLAC__stream_encoder_get_state(encoder_wrapper.encoder)]);
								goto wav_abort_;
							}
						}
					}
					else {
						if(*options.align_reservoir_samples > 0) {
							FLAC__ASSERT(CHUNK_OF_SAMPLES >= 588);
							bytes_read = fread(ucbuffer, sizeof(unsigned char), (*options.align_reservoir_samples) * bytes_per_wide_sample, infile);
							if(bytes_read == 0 && ferror(infile)) {
								fprintf(stderr, "%s: ERROR during read\n", encoder_wrapper.inbasefilename);
								goto wav_abort_;
							}
							else if(bytes_read != (*options.align_reservoir_samples) * bytes_per_wide_sample) {
								fprintf(stderr, "%s: WARNING: unexpected EOF; read %u bytes; expected %u samples, got %u samples\n", encoder_wrapper.inbasefilename, (unsigned)bytes_read, (unsigned)encoder_wrapper.total_samples_to_encode, (unsigned)encoder_wrapper.samples_written);
								data_bytes = 0;
							}
							else {
								info_align_carry = *options.align_reservoir_samples;
								format_input(options.align_reservoir, *options.align_reservoir_samples, false, is_unsigned_samples, channels, bps, &encoder_wrapper);
							}
						}
					}
				}

				got_data_chunk = true;
			}
		}
		else {
			fprintf(stderr, "%s: WARNING: skipping unknown sub-chunk '%c%c%c%c'\n", encoder_wrapper.inbasefilename, (char)(xx&255), (char)((xx>>8)&255), (char)((xx>>16)&255), (char)(xx>>24));
			/* sub-chunk size */
			if(!read_little_endian_uint32(infile, &xx, false, encoder_wrapper.inbasefilename))
				goto wav_abort_;
			if(fseek(infile, xx, SEEK_CUR) < 0) {
				/* can't seek input, read ahead manually... */
				unsigned left, need;
				const unsigned chunk = sizeof(ucbuffer);
				for(left = xx; left > 0; ) {
					need = min(left, chunk);
					if(fread(ucbuffer, 1, need, infile) < need) {
						fprintf(stderr, "%s: ERROR during read while skipping unsupported sub-chunk\n", encoder_wrapper.inbasefilename);
						goto wav_abort_;
					}
					left -= need;
				}
			}
		}
	}

	if(encoder_wrapper.encoder) {
		FLAC__stream_encoder_finish(encoder_wrapper.encoder);
		FLAC__stream_encoder_delete(encoder_wrapper.encoder);
#ifdef FLAC__HAS_OGG
		if(encoder_wrapper.use_ogg)
			ogg_stream_clear(&encoder_wrapper.ogg.os);
#endif
	}
	if(encoder_wrapper.verbose && encoder_wrapper.total_samples_to_encode > 0) {
		print_stats(&encoder_wrapper);
		fprintf(stderr, "\n");
	}
	if(0 != encoder_wrapper.seek_table.points)
		free(encoder_wrapper.seek_table.points);
	if(options.common.verify) {
		FLAC__stream_decoder_finish(encoder_wrapper.verify_fifo.decoder);
		FLAC__stream_decoder_delete(encoder_wrapper.verify_fifo.decoder);
		if(encoder_wrapper.verify_fifo.result != FLAC__VERIFY_OK) {
			fprintf(stderr, "Verify FAILED! (%s)  Do not trust %s\n", verify_code_string[encoder_wrapper.verify_fifo.result], outfilename);
			return 1;
		}
	}
	if(info_align_carry >= 0)
		fprintf(stderr, "%s: INFO: sector alignment causing %d samples to be carried over\n", encoder_wrapper.inbasefilename, info_align_carry);
	if(info_align_zero >= 0)
		fprintf(stderr, "%s: INFO: sector alignment causing %d zero samples to be appended\n", encoder_wrapper.inbasefilename, info_align_zero);
	if(infile != stdin)
		fclose(infile);
	return 0;
wav_abort_:
	if(encoder_wrapper.verbose && encoder_wrapper.total_samples_to_encode > 0)
		fprintf(stderr, "\n");
	if(encoder_wrapper.encoder) {
		FLAC__stream_encoder_finish(encoder_wrapper.encoder);
		FLAC__stream_encoder_delete(encoder_wrapper.encoder);
#ifdef FLAC__HAS_OGG
		if(encoder_wrapper.use_ogg)
			ogg_stream_clear(&encoder_wrapper.ogg.os);
#endif
	}
	if(0 != encoder_wrapper.seek_table.points)
		free(encoder_wrapper.seek_table.points);
	if(options.common.verify) {
		FLAC__stream_decoder_finish(encoder_wrapper.verify_fifo.decoder);
		FLAC__stream_decoder_delete(encoder_wrapper.verify_fifo.decoder);
		if(encoder_wrapper.verify_fifo.result != FLAC__VERIFY_OK) {
			fprintf(stderr, "Verify FAILED! (%s)  Do not trust %s\n", verify_code_string[encoder_wrapper.verify_fifo.result], outfilename);
			return 1;
		}
	}
	if(infile != stdin)
		fclose(infile);
	unlink(outfilename);
	return 1;
}

int flac__encode_raw(FILE *infile, long infilesize, const char *infilename, const char *outfilename, const FLAC__byte *lookahead, unsigned lookahead_length, raw_encode_options_t options)
{
	encoder_wrapper_struct encoder_wrapper;
	size_t bytes_read;
	const size_t bytes_per_wide_sample = options.channels * (options.bps >> 3);

	encoder_wrapper.encoder = 0;
	encoder_wrapper.verify = options.common.verify;
	encoder_wrapper.verbose = options.common.verbose;
	encoder_wrapper.bytes_written = 0;
	encoder_wrapper.samples_written = 0;
	encoder_wrapper.stream_offset = 0;
	encoder_wrapper.inbasefilename = flac__file_get_basename(infilename);
	encoder_wrapper.outfilename = outfilename;
	encoder_wrapper.seek_table.points = 0;
	encoder_wrapper.first_seek_point_to_check = 0;
#ifdef FLAC__HAS_OGG
	encoder_wrapper.use_ogg = options.common.use_ogg;
#endif

	if(0 == strcmp(outfilename, "-")) {
		encoder_wrapper.fout = file__get_binary_stdout();
	}
	else {
		if(0 == (encoder_wrapper.fout = fopen(outfilename, "wb"))) {
			fprintf(stderr, "ERROR: can't open output file %s\n", outfilename);
			if(infile != stdin)
				fclose(infile);
			return 1;
		}
	}

	if(!init(&encoder_wrapper))
		goto raw_abort_;

	/* get the file length */
	if(infilesize < 0) {
		encoder_wrapper.total_samples_to_encode = encoder_wrapper.unencoded_size = 0;
	}
	else {
		encoder_wrapper.total_samples_to_encode = (unsigned)infilesize / bytes_per_wide_sample - options.common.skip;
		encoder_wrapper.unencoded_size = encoder_wrapper.total_samples_to_encode * bytes_per_wide_sample;
	}

	if(encoder_wrapper.verbose && encoder_wrapper.total_samples_to_encode <= 0)
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
						fprintf(stderr, "%s: ERROR during read while skipping samples\n", encoder_wrapper.inbasefilename);
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

	if(!init_encoder(options.common, options.channels, options.bps, options.sample_rate, &encoder_wrapper))
		goto raw_abort_;

	encoder_wrapper.verify_fifo.into_frames = true;

	while(!feof(infile)) {
		if(lookahead_length > 0) {
			FLAC__ASSERT(lookahead_length < CHUNK_OF_SAMPLES * bytes_per_wide_sample);
			memcpy(ucbuffer, lookahead, lookahead_length);
			bytes_read = fread(ucbuffer+lookahead_length, sizeof(unsigned char), CHUNK_OF_SAMPLES * bytes_per_wide_sample - lookahead_length, infile) + lookahead_length;
			if(ferror(infile)) {
				fprintf(stderr, "%s: ERROR during read\n", encoder_wrapper.inbasefilename);
				goto raw_abort_;
			}
			lookahead_length = 0;
		}
		else
			bytes_read = fread(ucbuffer, sizeof(unsigned char), CHUNK_OF_SAMPLES * bytes_per_wide_sample, infile);

		if(bytes_read == 0) {
			if(ferror(infile)) {
				fprintf(stderr, "%s: ERROR during read\n", encoder_wrapper.inbasefilename);
				goto raw_abort_;
			}
		}
		else if(bytes_read % bytes_per_wide_sample != 0) {
			fprintf(stderr, "%s: ERROR: got partial sample\n", encoder_wrapper.inbasefilename);
			goto raw_abort_;
		}
		else {
			unsigned wide_samples = bytes_read / bytes_per_wide_sample;
			format_input(input, wide_samples, options.is_big_endian, options.is_unsigned_samples, options.channels, options.bps, &encoder_wrapper);

			/* NOTE: some versions of GCC can't figure out const-ness right and will give you an 'incompatible pointer type' warning on arg 2 here: */
			if(!FLAC__stream_encoder_process(encoder_wrapper.encoder, input, wide_samples)) {
				fprintf(stderr, "%s: ERROR during encoding, state = %d:%s\n", encoder_wrapper.inbasefilename, FLAC__stream_encoder_get_state(encoder_wrapper.encoder), FLAC__StreamEncoderStateString[FLAC__stream_encoder_get_state(encoder_wrapper.encoder)]);
				goto raw_abort_;
			}
		}
	}

	if(encoder_wrapper.encoder) {
		FLAC__stream_encoder_finish(encoder_wrapper.encoder);
		FLAC__stream_encoder_delete(encoder_wrapper.encoder);
#ifdef FLAC__HAS_OGG
		if(encoder_wrapper.use_ogg)
			ogg_stream_clear(&encoder_wrapper.ogg.os);
#endif
	}
	if(encoder_wrapper.verbose && encoder_wrapper.total_samples_to_encode > 0) {
		print_stats(&encoder_wrapper);
		fprintf(stderr, "\n");
	}
	if(0 != encoder_wrapper.seek_table.points)
		free(encoder_wrapper.seek_table.points);
	if(options.common.verify) {
		FLAC__stream_decoder_finish(encoder_wrapper.verify_fifo.decoder);
		FLAC__stream_decoder_delete(encoder_wrapper.verify_fifo.decoder);
		if(encoder_wrapper.verify_fifo.result != FLAC__VERIFY_OK) {
			fprintf(stderr, "Verify FAILED! (%s)  Do not trust %s\n", verify_code_string[encoder_wrapper.verify_fifo.result], outfilename);
			return 1;
		}
	}
	if(infile != stdin)
		fclose(infile);
	return 0;
raw_abort_:
	if(encoder_wrapper.verbose && encoder_wrapper.total_samples_to_encode > 0)
		fprintf(stderr, "\n");
	if(encoder_wrapper.encoder) {
		FLAC__stream_encoder_finish(encoder_wrapper.encoder);
		FLAC__stream_encoder_delete(encoder_wrapper.encoder);
#ifdef FLAC__HAS_OGG
		if(encoder_wrapper.use_ogg)
			ogg_stream_clear(&encoder_wrapper.ogg.os);
#endif
	}
	if(0 != encoder_wrapper.seek_table.points)
		free(encoder_wrapper.seek_table.points);
	if(options.common.verify) {
		FLAC__stream_decoder_finish(encoder_wrapper.verify_fifo.decoder);
		FLAC__stream_decoder_delete(encoder_wrapper.verify_fifo.decoder);
		if(encoder_wrapper.verify_fifo.result != FLAC__VERIFY_OK) {
			fprintf(stderr, "Verify FAILED! (%s)  Do not trust %s\n", verify_code_string[encoder_wrapper.verify_fifo.result], outfilename);
			return 1;
		}
	}
	if(infile != stdin)
		fclose(infile);
	unlink(outfilename);
	return 1;
}

FLAC__bool init(encoder_wrapper_struct *encoder_wrapper)
{
	unsigned i;
	FLAC__uint32 test = 1;

	is_big_endian_host = (*((FLAC__byte*)(&test)))? false : true;

	for(i = 0; i < FLAC__MAX_CHANNELS; i++)
		input[i] = &(in[i][0]);

	encoder_wrapper->encoder = FLAC__stream_encoder_new();
	if(0 == encoder_wrapper->encoder) {
		fprintf(stderr, "%s: ERROR creating the encoder instance\n", encoder_wrapper->inbasefilename);
		return false;
	}

#ifdef FLAC__HAS_OGG
	if(encoder_wrapper->use_ogg) {
		if(ogg_stream_init(&encoder_wrapper->ogg.os, 0) != 0) {
			fprintf(stderr, "%s: ERROR initializing the Ogg stream\n", encoder_wrapper->inbasefilename);
			FLAC__stream_encoder_delete(encoder_wrapper->encoder);
			return false;
		}
	}
#endif

	return true;
}

FLAC__bool init_encoder(encode_options_t options, unsigned channels, unsigned bps, unsigned sample_rate, encoder_wrapper_struct *encoder_wrapper)
{
	unsigned i;

	if(channels != 2)
		options.do_mid_side = options.loose_mid_side = false;

	if(encoder_wrapper->verify) {
		/* set up the fifo which will hold the original signal to compare against */
		encoder_wrapper->verify_fifo.size = options.blocksize + CHUNK_OF_SAMPLES;
		for(i = 0; i < channels; i++) {
			if(0 == (encoder_wrapper->verify_fifo.original[i] = (FLAC__int32*)malloc(sizeof(FLAC__int32) * encoder_wrapper->verify_fifo.size))) {
				fprintf(stderr, "%s: ERROR allocating verify buffers\n", encoder_wrapper->inbasefilename);
				return false;
			}
		}
		encoder_wrapper->verify_fifo.tail = 0;
		encoder_wrapper->verify_fifo.into_frames = false;
		encoder_wrapper->verify_fifo.result = FLAC__VERIFY_OK;

		/* set up a stream decoder for verification */
		encoder_wrapper->verify_fifo.decoder = FLAC__stream_decoder_new();
		if(0 == encoder_wrapper->verify_fifo.decoder) {
			fprintf(stderr, "%s: ERROR creating the verify decoder instance\n", encoder_wrapper->inbasefilename);
			return false;
		}
		FLAC__stream_decoder_set_read_callback(encoder_wrapper->verify_fifo.decoder, verify_read_callback);
		FLAC__stream_decoder_set_write_callback(encoder_wrapper->verify_fifo.decoder, verify_write_callback);
		FLAC__stream_decoder_set_metadata_callback(encoder_wrapper->verify_fifo.decoder, verify_metadata_callback);
		FLAC__stream_decoder_set_error_callback(encoder_wrapper->verify_fifo.decoder, verify_error_callback);
		FLAC__stream_decoder_set_client_data(encoder_wrapper->verify_fifo.decoder, encoder_wrapper);
		if(FLAC__stream_decoder_init(encoder_wrapper->verify_fifo.decoder) != FLAC__STREAM_DECODER_SEARCH_FOR_METADATA) {
			fprintf(stderr, "%s: ERROR initializing decoder, state = %d:%s\n", encoder_wrapper->inbasefilename, FLAC__stream_decoder_get_state(encoder_wrapper->verify_fifo.decoder), FLAC__StreamDecoderStateString[FLAC__stream_decoder_get_state(encoder_wrapper->verify_fifo.decoder)]);
			return false;
		}
	}

	if(!convert_to_seek_table(options.requested_seek_points, options.num_requested_seek_points, encoder_wrapper->total_samples_to_encode, options.blocksize, &encoder_wrapper->seek_table)) {
		fprintf(stderr, "%s: ERROR allocating seek table\n", encoder_wrapper->inbasefilename);
		return false;
	}

	FLAC__stream_encoder_set_streamable_subset(encoder_wrapper->encoder, !options.lax);
	FLAC__stream_encoder_set_do_mid_side_stereo(encoder_wrapper->encoder, options.do_mid_side);
	FLAC__stream_encoder_set_loose_mid_side_stereo(encoder_wrapper->encoder, options.loose_mid_side);
	FLAC__stream_encoder_set_channels(encoder_wrapper->encoder, channels);
	FLAC__stream_encoder_set_bits_per_sample(encoder_wrapper->encoder, bps);
	FLAC__stream_encoder_set_sample_rate(encoder_wrapper->encoder, sample_rate);
	FLAC__stream_encoder_set_blocksize(encoder_wrapper->encoder, options.blocksize);
	FLAC__stream_encoder_set_max_lpc_order(encoder_wrapper->encoder, options.max_lpc_order);
	FLAC__stream_encoder_set_qlp_coeff_precision(encoder_wrapper->encoder, options.qlp_coeff_precision);
	FLAC__stream_encoder_set_do_qlp_coeff_prec_search(encoder_wrapper->encoder, options.do_qlp_coeff_prec_search);
	FLAC__stream_encoder_set_do_escape_coding(encoder_wrapper->encoder, options.do_escape_coding);
	FLAC__stream_encoder_set_do_exhaustive_model_search(encoder_wrapper->encoder, options.do_exhaustive_model_search);
	FLAC__stream_encoder_set_min_residual_partition_order(encoder_wrapper->encoder, options.min_residual_partition_order);
	FLAC__stream_encoder_set_max_residual_partition_order(encoder_wrapper->encoder, options.max_residual_partition_order);
	FLAC__stream_encoder_set_rice_parameter_search_dist(encoder_wrapper->encoder, options.rice_parameter_search_dist);
	FLAC__stream_encoder_set_total_samples_estimate(encoder_wrapper->encoder, encoder_wrapper->total_samples_to_encode);
	FLAC__stream_encoder_set_seek_table(encoder_wrapper->encoder, (encoder_wrapper->seek_table.num_points > 0)? &encoder_wrapper->seek_table : 0);
	FLAC__stream_encoder_set_padding(encoder_wrapper->encoder, options.padding);
	FLAC__stream_encoder_set_last_metadata_is_last(encoder_wrapper->encoder, true);
	FLAC__stream_encoder_set_write_callback(encoder_wrapper->encoder, write_callback);
	FLAC__stream_encoder_set_metadata_callback(encoder_wrapper->encoder, metadata_callback);
	FLAC__stream_encoder_set_client_data(encoder_wrapper->encoder, encoder_wrapper);

	if(FLAC__stream_encoder_init(encoder_wrapper->encoder) != FLAC__STREAM_ENCODER_OK) {
		fprintf(stderr, "%s: ERROR initializing encoder, state = %d:%s\n", encoder_wrapper->inbasefilename, FLAC__stream_encoder_get_state(encoder_wrapper->encoder), FLAC__StreamEncoderStateString[FLAC__stream_encoder_get_state(encoder_wrapper->encoder)]);
		return false;
	}

	/* the above call writes all the metadata, so we save the stream offset now */
	encoder_wrapper->stream_offset = encoder_wrapper->bytes_written;

	return true;
}

FLAC__bool convert_to_seek_table(char *requested_seek_points, int num_requested_seek_points, FLAC__uint64 stream_samples, unsigned blocksize, FLAC__StreamMetaData_SeekTable *seek_table)
{
	unsigned i, j, real_points, placeholders;
	char *pt = requested_seek_points, *q;
	FLAC__bool first;

	seek_table->num_points = 0;

	if(num_requested_seek_points == 0)
		return true;

	if(num_requested_seek_points < 0) {
		strcpy(requested_seek_points, "100x<");
		num_requested_seek_points = 1;
	}

	/* first count how many individual seek point we may need */
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

	/* make some space */
	if(0 == (seek_table->points = (FLAC__StreamMetaData_SeekPoint*)malloc(sizeof(FLAC__StreamMetaData_SeekPoint) * (real_points+placeholders))))
		return false;

	/* initialize the seek_table.  we set frame_samples to zero to signify the points have not yet been hit by a frame write yet. */
	for(i = 0; i < real_points+placeholders; i++) {
		seek_table->points[i].sample_number = FLAC__STREAM_METADATA_SEEKPOINT_PLACEHOLDER;
		seek_table->points[i].stream_offset = 0;
		seek_table->points[i].frame_samples = 0;
	}

	for(i = 0; i < (unsigned)num_requested_seek_points; i++) {
		q = strchr(pt, '<');
		FLAC__ASSERT(0 != q);
		*q++ = '\0';

		if(0 == strcmp(pt, "X")) { /* -S X */
			; /* we append placeholders later */
		}
		else if(pt[strlen(pt)-1] == 'x') { /* -S #x */
			if(stream_samples > 0) { /* we can only do these if we know the number of samples to encode up front */
				unsigned j, n;
				n = (unsigned)atoi(pt);
				for(j = 0; j < n; j++)
					append_point_to_seek_table(seek_table, stream_samples * (FLAC__uint64)j / (FLAC__uint64)n, stream_samples, blocksize);
			}
		}
		else { /* -S # */
			append_point_to_seek_table(seek_table, (FLAC__uint64)atoi(pt), stream_samples, blocksize);
		}

		pt = q;
	}

	/* sort the seekpoints */
	qsort(seek_table->points, seek_table->num_points, sizeof(FLAC__StreamMetaData_SeekPoint), (int (*)(const void *, const void *))seekpoint_compare);

	/* uniqify the seekpoints */
	first = true;
	for(i = j = 0; i < seek_table->num_points; i++) {
		if(!first) {
			if(seek_table->points[i].sample_number == seek_table->points[j-1].sample_number)
				continue;
		}
		first = false;
		seek_table->points[j++] = seek_table->points[i];
	}
	seek_table->num_points = j;

	/* append placeholders */
	for(i = 0, j = seek_table->num_points; i < placeholders; i++, j++)
		seek_table->points[j].sample_number = FLAC__STREAM_METADATA_SEEKPOINT_PLACEHOLDER;
	seek_table->num_points += placeholders;

	return true;
}

void append_point_to_seek_table(FLAC__StreamMetaData_SeekTable *seek_table, FLAC__uint64 sample, FLAC__uint64 stream_samples, FLAC__uint64 blocksize)
{
	const FLAC__uint64 target_sample = (sample / blocksize) * blocksize;

	if(stream_samples == 0 || target_sample < stream_samples)
		seek_table->points[seek_table->num_points++].sample_number = target_sample;
}

int seekpoint_compare(const FLAC__StreamMetaData_SeekPoint *l, const FLAC__StreamMetaData_SeekPoint *r)
{
	/* we don't just 'return l->sample_number - r->sample_number' since the result (FLAC__int64) might overflow an 'int' */
	if(l->sample_number == r->sample_number)
		return 0;
	else if(l->sample_number < r->sample_number)
		return -1;
	else
		return 1;
}

void format_input(FLAC__int32 *dest[], unsigned wide_samples, FLAC__bool is_big_endian, FLAC__bool is_unsigned_samples, unsigned channels, unsigned bps, encoder_wrapper_struct *encoder_wrapper)
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

	/* NOTE: some versions of GCC can't figure out const-ness right and will give you an 'incompatible pointer type' warning on arg 2 here: */
	append_to_verify_fifo(encoder_wrapper, dest, channels, wide_samples);
}

void append_to_verify_fifo(encoder_wrapper_struct *encoder_wrapper, const FLAC__int32 *src[], unsigned channels, unsigned wide_samples)
{
	if(encoder_wrapper->verify) {
		unsigned channel;
		for(channel = 0; channel < channels; channel++)
			memcpy(&encoder_wrapper->verify_fifo.original[channel][encoder_wrapper->verify_fifo.tail], src[channel], sizeof(FLAC__int32) * wide_samples);
		encoder_wrapper->verify_fifo.tail += wide_samples;
		FLAC__ASSERT(encoder_wrapper->verify_fifo.tail <= encoder_wrapper->verify_fifo.size);
	}
}

FLAC__StreamEncoderWriteStatus write_callback(const FLAC__StreamEncoder *encoder, const FLAC__byte buffer[], unsigned bytes, unsigned samples, unsigned current_frame, void *client_data)
{
	encoder_wrapper_struct *encoder_wrapper = (encoder_wrapper_struct *)client_data;
	const unsigned mask = (FLAC__stream_encoder_get_do_exhaustive_model_search(encoder) || FLAC__stream_encoder_get_do_qlp_coeff_prec_search(encoder))? 0x1f : 0x7f;

	/* mark the current seek point if hit (if stream_offset == 0 that means we're still writing metadata and haven't hit the first frame yet) */
	if(encoder_wrapper->stream_offset > 0 && encoder_wrapper->seek_table.num_points > 0) {
		FLAC__uint64 current_sample = (FLAC__uint64)current_frame * (FLAC__uint64)FLAC__stream_encoder_get_blocksize(encoder), test_sample;
		unsigned i;
		for(i = encoder_wrapper->first_seek_point_to_check; i < encoder_wrapper->seek_table.num_points; i++) {
			test_sample = encoder_wrapper->seek_table.points[i].sample_number;
			if(test_sample > current_sample) {
				break;
			}
			else if(test_sample == current_sample) {
				encoder_wrapper->seek_table.points[i].stream_offset = encoder_wrapper->bytes_written - encoder_wrapper->stream_offset;
				encoder_wrapper->seek_table.points[i].frame_samples = FLAC__stream_encoder_get_blocksize(encoder);
				encoder_wrapper->first_seek_point_to_check++;
				break;
			}
			else {
				encoder_wrapper->first_seek_point_to_check++;
			}
		}
	}

	encoder_wrapper->bytes_written += bytes;
	encoder_wrapper->samples_written += samples;
	encoder_wrapper->current_frame = current_frame;

	if(samples && encoder_wrapper->verbose && encoder_wrapper->total_samples_to_encode > 0 && !(current_frame & mask))
		print_stats(encoder_wrapper);

	if(encoder_wrapper->verify) {
		encoder_wrapper->verify_fifo.encoded_signal = buffer;
		encoder_wrapper->verify_fifo.encoded_bytes = bytes;
		if(encoder_wrapper->verify_fifo.into_frames) {
			if(!FLAC__stream_decoder_process_one_frame(encoder_wrapper->verify_fifo.decoder)) {
				encoder_wrapper->verify_fifo.result = FLAC__VERIFY_FAILED_IN_FRAME;
				return FLAC__STREAM_ENCODER_WRITE_FATAL_ERROR;
			}
		}
		else {
			if(!FLAC__stream_decoder_process_metadata(encoder_wrapper->verify_fifo.decoder)) {
				encoder_wrapper->verify_fifo.result = FLAC__VERIFY_FAILED_IN_METADATA;
				return FLAC__STREAM_ENCODER_WRITE_FATAL_ERROR;
			}
		}
	}

#ifdef FLAC__HAS_OGG
	if(encoder_wrapper->use_ogg) {
		ogg_packet op;

		memset(&op, 0, sizeof(op));
		op.packet = (unsigned char *)buffer;
		op.granulepos = encoder_wrapper->samples_written - 1;
		/*@@@ WATCHOUT:
		 * this depends on the behavior of libFLAC that we will get one
		 * write_callback first with all the metadata (and 'samples'
		 * will be 0), then one write_callback for each frame.
		 */
		op.packetno = (samples == 0? -1 : (int)encoder_wrapper->current_frame);
		op.bytes = bytes;

		if (encoder_wrapper->bytes_written == bytes)
			op.b_o_s = 1;

		if (encoder_wrapper->total_samples_to_encode == encoder_wrapper->samples_written)
			op.e_o_s = 1;

		ogg_stream_packetin(&encoder_wrapper->ogg.os, &op);

		while(ogg_stream_pageout(&encoder_wrapper->ogg.os, &encoder_wrapper->ogg.og) != 0) {
			int written;
			written = fwrite(encoder_wrapper->ogg.og.header, 1, encoder_wrapper->ogg.og.header_len, encoder_wrapper->fout);
			if (written != encoder_wrapper->ogg.og.header_len)
				return FLAC__STREAM_ENCODER_WRITE_FATAL_ERROR;

			written = fwrite(encoder_wrapper->ogg.og.body, 1, encoder_wrapper->ogg.og.body_len, encoder_wrapper->fout);
			if (written != encoder_wrapper->ogg.og.body_len)
				return FLAC__STREAM_ENCODER_WRITE_FATAL_ERROR;
		}

		return FLAC__STREAM_ENCODER_WRITE_OK;
	}
	else
#endif
	{
		if(fwrite(buffer, sizeof(FLAC__byte), bytes, encoder_wrapper->fout) == bytes)
			return FLAC__STREAM_ENCODER_WRITE_OK;
		else
			return FLAC__STREAM_ENCODER_WRITE_FATAL_ERROR;
	}
}

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

FLAC__StreamDecoderReadStatus verify_read_callback(const FLAC__StreamDecoder *decoder, FLAC__byte buffer[], unsigned *bytes, void *client_data)
{
	encoder_wrapper_struct *encoder_wrapper = (encoder_wrapper_struct *)client_data;
	const unsigned encoded_bytes = encoder_wrapper->verify_fifo.encoded_bytes;
	(void)decoder;

	if(encoded_bytes <= *bytes) {
		*bytes = encoded_bytes;
		memcpy(buffer, encoder_wrapper->verify_fifo.encoded_signal, *bytes);
	}
	else {
		memcpy(buffer, encoder_wrapper->verify_fifo.encoded_signal, *bytes);
		encoder_wrapper->verify_fifo.encoded_signal += *bytes;
		encoder_wrapper->verify_fifo.encoded_bytes -= *bytes;
	}

	return FLAC__STREAM_DECODER_READ_CONTINUE;
}

FLAC__StreamDecoderWriteStatus verify_write_callback(const FLAC__StreamDecoder *decoder, const FLAC__Frame *frame, const FLAC__int32 *buffer[], void *client_data)
{
	encoder_wrapper_struct *encoder_wrapper = (encoder_wrapper_struct *)client_data;
	unsigned channel, l, r;
	const unsigned channels = FLAC__stream_decoder_get_channels(decoder);
	const unsigned bytes_per_block = sizeof(FLAC__int32) * FLAC__stream_decoder_get_blocksize(decoder);

	for(channel = 0; channel < channels; channel++) {
		if(0 != memcmp(buffer[channel], encoder_wrapper->verify_fifo.original[channel], bytes_per_block)) {
			unsigned sample = 0;
			int expect = 0, got = 0;
			fprintf(stderr, "\n%s: ERROR: mismatch in decoded data, verify FAILED!\n", encoder_wrapper->inbasefilename);
			fprintf(stderr, "       Please submit a bug report to\n");
			fprintf(stderr, "           http://sourceforge.net/bugs/?func=addbug&group_id=13478\n");
			fprintf(stderr, "       Make sure to include an email contact in the comment and/or use the\n");
			fprintf(stderr, "       \"Monitor\" feature to monitor the bug status.\n");
			for(l = 0, r = FLAC__stream_decoder_get_blocksize(decoder); l < r; l++) {
				if(buffer[channel][l] != encoder_wrapper->verify_fifo.original[channel][l]) {
					sample = l;
					expect = (int)encoder_wrapper->verify_fifo.original[channel][l];
					got = (int)buffer[channel][l];
					break;
				}
			}
			FLAC__ASSERT(l < r);
			FLAC__ASSERT(frame->header.number_type == FLAC__FRAME_NUMBER_TYPE_SAMPLE_NUMBER);
			fprintf(stderr, "       Absolute sample=%u, frame=%u, channel=%u, sample=%u, expected %d, got %d\n", (unsigned)frame->header.number.sample_number + sample, (unsigned)frame->header.number.sample_number / FLAC__stream_decoder_get_blocksize(decoder), channel, sample, expect, got); /*@@@ WATCHOUT: 4GB limit */
			return FLAC__STREAM_DECODER_WRITE_ABORT;
		}
	}
	/* dequeue the frame from the fifo */
	for(channel = 0; channel < channels; channel++) {
		for(l = 0, r = frame->header.blocksize; r < encoder_wrapper->verify_fifo.tail; l++, r++) {
			encoder_wrapper->verify_fifo.original[channel][l] = encoder_wrapper->verify_fifo.original[channel][r];
		}
	}
	encoder_wrapper->verify_fifo.tail -= frame->header.blocksize;
	return FLAC__STREAM_DECODER_WRITE_CONTINUE;
}

void verify_metadata_callback(const FLAC__StreamDecoder *decoder, const FLAC__StreamMetaData *metadata, void *client_data)
{
	(void)decoder;
	(void)metadata;
	(void)client_data;
}

void verify_error_callback(const FLAC__StreamDecoder *decoder, FLAC__StreamDecoderErrorStatus status, void *client_data)
{
	encoder_wrapper_struct *encoder_wrapper = (encoder_wrapper_struct *)client_data;
	(void)decoder;
	fprintf(stderr, "\n%s: ERROR: verification decoder returned error %d:%s\n", encoder_wrapper->inbasefilename, status, FLAC__StreamDecoderErrorStatusString[status]);
}

void print_stats(const encoder_wrapper_struct *encoder_wrapper)
{
#if defined _MSC_VER || defined __MINGW32__
	/* with VC++ you have to spoon feed it the casting */
	const double progress = (double)(FLAC__int64)encoder_wrapper->samples_written / (double)(FLAC__int64)encoder_wrapper->total_samples_to_encode;
	const double ratio = (double)(FLAC__int64)encoder_wrapper->bytes_written / ((double)(FLAC__int64)encoder_wrapper->unencoded_size * progress);
#else
	const double progress = (double)encoder_wrapper->samples_written / (double)encoder_wrapper->total_samples_to_encode;
	const double ratio = (double)encoder_wrapper->bytes_written / ((double)encoder_wrapper->unencoded_size * progress);
#endif

	if(encoder_wrapper->samples_written == encoder_wrapper->total_samples_to_encode) {
		fprintf(stderr, "\r%s:%s wrote %u bytes, ratio=%0.3f",
			encoder_wrapper->inbasefilename,
			encoder_wrapper->verify? (encoder_wrapper->verify_fifo.result == FLAC__VERIFY_OK? " Verify OK," : " Verify FAILED!") : "",
			(unsigned)encoder_wrapper->bytes_written,
			ratio
		);
	}
	else {
		fprintf(stderr, "\r%s: %u%% complete, ratio=%0.3f", encoder_wrapper->inbasefilename, (unsigned)floor(progress * 100.0 + 0.5), ratio);
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
