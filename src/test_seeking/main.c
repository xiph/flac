/* test_seeking - Seeking tester for libFLAC
 * Copyright (C) 2004,2005,2006  Josh Coalson
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

#if HAVE_CONFIG_H
#  include <config.h>
#endif

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#if defined _MSC_VER || defined __MINGW32__
#include <time.h>
#else
#include <sys/time.h>
#endif
#include <sys/stat.h> /* for stat() */
#include "FLAC/assert.h"
#include "FLAC/stream_decoder.h"

typedef struct {
	FLAC__bool got_data;
	FLAC__uint64 total_samples;
	unsigned channels;
	unsigned bits_per_sample;
	FLAC__bool quiet;
	FLAC__bool ignore_errors;
	FLAC__bool error_occurred;
} DecoderClientData;

static FLAC__bool stop_signal_ = false;

static void our_sigint_handler_(int signal)
{
	(void)signal;
	printf("(caught SIGINT) ");
	fflush(stdout);
	stop_signal_ = true;
}

static FLAC__bool die_(const char *msg)
{
	printf("ERROR: %s\n", msg);
	return false;
}

static FLAC__bool die_s_(const char *msg, const FLAC__StreamDecoder *decoder)
{
	FLAC__StreamDecoderState state = FLAC__stream_decoder_get_state(decoder);

	if(msg)
		printf("FAILED, %s", msg);
	else
		printf("FAILED");

	printf(", state = %u (%s)\n", (unsigned)state, FLAC__StreamDecoderStateString[state]);

	return false;
}

static off_t get_filesize_(const char *srcpath)
{
	struct stat srcstat;

	if(0 == stat(srcpath, &srcstat))
		return srcstat.st_size;
	else
		return -1;
}

static FLAC__StreamDecoderWriteStatus write_callback_(const FLAC__StreamDecoder *decoder, const FLAC__Frame *frame, const FLAC__int32 * const buffer[], void *client_data)
{
	DecoderClientData *dcd = (DecoderClientData*)client_data;

	(void)decoder, (void)buffer;

	if(0 == dcd) {
		printf("ERROR: client_data in write callback is NULL\n");
		return FLAC__STREAM_DECODER_WRITE_STATUS_ABORT;
	}

	if(dcd->error_occurred)
		return FLAC__STREAM_DECODER_WRITE_STATUS_ABORT;

	if (frame->header.number_type == FLAC__FRAME_NUMBER_TYPE_FRAME_NUMBER) {
		if (!dcd->quiet)
			printf("frame@%uf(%u)... ", frame->header.number.frame_number, frame->header.blocksize);
	}
	else if (frame->header.number_type == FLAC__FRAME_NUMBER_TYPE_SAMPLE_NUMBER) {
		if (!dcd->quiet)
#ifdef _MSC_VER
			printf("frame@%I64u(%u)... ", frame->header.number.sample_number, frame->header.blocksize);
#else
			printf("frame@%llu(%u)... ", (unsigned long long)frame->header.number.sample_number, frame->header.blocksize);
#endif
	}
	else {
		FLAC__ASSERT(0);
		dcd->error_occurred = true;
		return FLAC__STREAM_DECODER_WRITE_STATUS_ABORT;
	}
	fflush(stdout);

	return FLAC__STREAM_DECODER_WRITE_STATUS_CONTINUE;
}

static void metadata_callback_(const FLAC__StreamDecoder *decoder, const FLAC__StreamMetadata *metadata, void *client_data)
{
	DecoderClientData *dcd = (DecoderClientData*)client_data;

	(void)decoder;

	if(0 == dcd) {
		printf("ERROR: client_data in metadata callback is NULL\n");
		return;
	}

	if(dcd->error_occurred)
		return;

	if (!dcd->got_data && metadata->type == FLAC__METADATA_TYPE_STREAMINFO) {
		dcd->got_data = true;
		dcd->total_samples = metadata->data.stream_info.total_samples;
		dcd->channels = metadata->data.stream_info.channels;
		dcd->bits_per_sample = metadata->data.stream_info.bits_per_sample;
	}
}

static void error_callback_(const FLAC__StreamDecoder *decoder, FLAC__StreamDecoderErrorStatus status, void *client_data)
{
	DecoderClientData *dcd = (DecoderClientData*)client_data;

	(void)decoder;

	if(0 == dcd) {
		printf("ERROR: client_data in error callback is NULL\n");
		return;
	}

	if(!dcd->ignore_errors) {
		printf("ERROR: got error callback: err = %u (%s)\n", (unsigned)status, FLAC__StreamDecoderErrorStatusString[status]);
		dcd->error_occurred = true;
	}
}

/* read mode:
 * 0 - no read after seek
 * 1 - read 2 frames
 * 2 - read until end
 */
static FLAC__bool seek_barrage(FLAC__bool is_ogg, const char *filename, off_t filesize, unsigned count, FLAC__int64 total_samples, unsigned read_mode)
{
	FLAC__StreamDecoder *decoder;
	DecoderClientData decoder_client_data;
	unsigned i;
	long int n;

	decoder_client_data.got_data = false;
	decoder_client_data.total_samples = 0;
	decoder_client_data.quiet = false;
	decoder_client_data.ignore_errors = false;
	decoder_client_data.error_occurred = false;

	printf("\n+++ seek test: FLAC__StreamDecoder (%s FLAC, read_mode=%u)\n\n", is_ogg? "Ogg":"native", read_mode);

	decoder = FLAC__stream_decoder_new();
	if(0 == decoder)
		return die_("FLAC__stream_decoder_new() FAILED, returned NULL\n");

	if(is_ogg) {
		if(FLAC__stream_decoder_init_ogg_file(decoder, filename, write_callback_, metadata_callback_, error_callback_, &decoder_client_data) != FLAC__STREAM_DECODER_INIT_STATUS_OK)
			return die_s_("FLAC__stream_decoder_init_file() FAILED", decoder);
	}
	else {
		if(FLAC__stream_decoder_init_file(decoder, filename, write_callback_, metadata_callback_, error_callback_, &decoder_client_data) != FLAC__STREAM_DECODER_INIT_STATUS_OK)
			return die_s_("FLAC__stream_decoder_init_file() FAILED", decoder);
	}

	if(!FLAC__stream_decoder_process_until_end_of_metadata(decoder))
		return die_s_("FLAC__stream_decoder_process_until_end_of_metadata() FAILED", decoder);

	if(!is_ogg) { /* not necessary to do this for Ogg because of its seeking method */
	/* process until end of stream to make sure we can still seek in that state */
		decoder_client_data.quiet = true;
		if(!FLAC__stream_decoder_process_until_end_of_stream(decoder))
			return die_s_("FLAC__stream_decoder_process_until_end_of_stream() FAILED", decoder);
		decoder_client_data.quiet = false;

		printf("stream decoder state is %s\n", FLAC__stream_decoder_get_resolved_state_string(decoder));
		if(FLAC__stream_decoder_get_state(decoder) != FLAC__STREAM_DECODER_END_OF_STREAM)
			return die_s_("expected FLAC__STREAM_DECODER_END_OF_STREAM", decoder);
	}

#ifdef _MSC_VER
	printf("file's total_samples is %I64u\n", decoder_client_data.total_samples);
#else
	printf("file's total_samples is %llu\n", (unsigned long long)decoder_client_data.total_samples);
#endif
#if !defined _MSC_VER && !defined __MINGW32__ && !defined __EMX__
	if (decoder_client_data.total_samples > (FLAC__uint64)RAND_MAX) {
		printf("ERROR: must be total_samples < %u\n", (unsigned)RAND_MAX);
		return false;
	}
#endif
	n = (long int)decoder_client_data.total_samples;

	if(n == 0 && total_samples >= 0)
		n = (long int)total_samples;

	/* if we don't have a total samples count, just guess based on the file size */
	/* @@@ for is_ogg we should get it from last page's granulepos */
	if(n == 0) {
		/* 8 would imply no compression, 9 guarantees that we will get some samples off the end of the stream to test that case */
		n = 9 * filesize / (decoder_client_data.channels * decoder_client_data.bits_per_sample);
#if !defined _MSC_VER && !defined __MINGW32__
		if(n > RAND_MAX)
			n = RAND_MAX;
#endif
	}

	printf("Begin seek barrage, count=%u\n", count);

	for (i = 0; !stop_signal_ && (count == 0 || i < count); i++) {
		FLAC__uint64 pos;

		/* for the first 10, seek to the first 10 samples */
		if (n >= 10 && i < 10) {
			pos = i;
		}
		/* for the second 10, seek to the last 10 samples */
		else if (n >= 10 && i < 20) {
			pos = n - 1 - (i-10);
		}
		/* for the third 10, seek past the end and make sure we fail properly as expected */
		else if (i < 30) {
			pos = n + (i-20);
		}
		else {
#if !defined _MSC_VER && !defined __MINGW32__
			pos = (FLAC__uint64)(random() % n);
#else
			/* RAND_MAX is only 32767 in my MSVC */
			pos = (FLAC__uint64)((rand()<<15|rand()) % n);
#endif
		}

#ifdef _MSC_VER
		printf("seek(%I64u)... ", pos);
#else
		printf("seek(%llu)... ", (unsigned long long)pos);
#endif
		fflush(stdout);
		if(!FLAC__stream_decoder_seek_absolute(decoder, pos)) {
			if(pos >= (FLAC__uint64)n)
				printf("seek past end failed as expected... ");
			else if(decoder_client_data.total_samples == 0 && total_samples <= 0)
				printf("seek failed, assuming it was past EOF... ");
			else
				return die_s_("FLAC__stream_decoder_seek_absolute() FAILED", decoder);
			if(!FLAC__stream_decoder_flush(decoder))
				return die_s_("FLAC__stream_decoder_flush() FAILED", decoder);
		}
		else if(read_mode == 1) {
			printf("decode_frame... ");
			fflush(stdout);
			if(!FLAC__stream_decoder_process_single(decoder))
				return die_s_("FLAC__stream_decoder_process_single() FAILED", decoder);

			printf("decode_frame... ");
			fflush(stdout);
			if(!FLAC__stream_decoder_process_single(decoder))
				return die_s_("FLAC__stream_decoder_process_single() FAILED", decoder);
		}
		else if(read_mode == 2) {
			printf("decode_all... ");
			fflush(stdout);
			decoder_client_data.quiet = true;
			if(!FLAC__stream_decoder_process_until_end_of_stream(decoder))
				return die_s_("FLAC__stream_decoder_process_until_end_of_stream() FAILED", decoder);
			decoder_client_data.quiet = false;
		}

		printf("OK\n");
		fflush(stdout);
	}

	if(FLAC__stream_decoder_get_state(decoder) != FLAC__STREAM_DECODER_UNINITIALIZED) {
		if(!FLAC__stream_decoder_finish(decoder))
			return die_s_("FLAC__stream_decoder_finish() FAILED", decoder);
	}

	printf("\nPASSED!\n");

	return true;
}

int main(int argc, char *argv[])
{
	const char *filename;
	unsigned count = 0, read_mode;
	FLAC__int64 samples = -1;
	off_t filesize;

	static const char * const usage = "usage: test_seeking file.flac [#seeks] [#samples-in-file.flac]\n";

	if (argc < 2 || argc > 4) {
		fprintf(stderr, usage);
		return 1;
	}

	filename = argv[1];

	if (argc > 2)
		count = strtoul(argv[2], 0, 10);
	if (argc > 3)
		samples = strtoull(argv[3], 0, 10);

	if (count < 30)
		fprintf(stderr, "WARNING: random seeks don't kick in until after 30 preprogrammed ones\n");

#if !defined _MSC_VER && !defined __MINGW32__
	{
		struct timeval tv;

		if (gettimeofday(&tv, 0) < 0) {
			fprintf(stderr, "WARNING: couldn't seed RNG with time\n");
			tv.tv_usec = 4321;
		}
		srandom(tv.tv_usec);
	}
#else
	srand(time(0));
#endif

	filesize = get_filesize_(filename);
	if (filesize < 0) {
		fprintf(stderr, "ERROR: can't determine filesize for %s\n", filename);
		return 1;
	}

	(void) signal(SIGINT, our_sigint_handler_);

	for (read_mode = 0; read_mode <= 2; read_mode++) {
		FLAC__bool ok;
		if (strlen(filename) > 4 && 0 == strcmp(filename+strlen(filename)-4, ".ogg")) {
#if FLAC__HAS_OGG
			ok = seek_barrage(/*is_ogg=*/true, filename, filesize, count, samples, read_mode);
#else
			fprintf(stderr, "ERROR: Ogg FLAC not supported\n");
			ok = false;
#endif
		}
		else {
			ok = seek_barrage(/*is_ogg=*/false, filename, filesize, count, samples, read_mode);
		}
		if (!ok)
			return 2;
	}

	return 0;
}
