/* test_seeking - Seeking tester for libFLAC and libOggFLAC
 * Copyright (C) 2004,2005  Josh Coalson
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
#include "FLAC/file_decoder.h"
#ifdef FLAC__HAS_OGG
#include "OggFLAC/file_decoder.h"
#endif

typedef struct {
	FLAC__bool got_data;
	FLAC__uint64 total_samples;
	unsigned channels;
	unsigned bits_per_sample;
	FLAC__bool ignore_errors;
	FLAC__bool error_occurred;
} decoder_client_data_struct;

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

static FLAC__bool die_f_(const char *msg, const FLAC__FileDecoder *decoder)
{
	FLAC__FileDecoderState state = FLAC__file_decoder_get_state(decoder);

	if(msg)
		printf("FAILED, %s", msg);
	else
		printf("FAILED");

	printf(", state = %u (%s)\n", (unsigned)state, FLAC__FileDecoderStateString[state]);
	if(state == FLAC__FILE_DECODER_SEEKABLE_STREAM_DECODER_ERROR) {
		FLAC__SeekableStreamDecoderState state_ = FLAC__file_decoder_get_seekable_stream_decoder_state(decoder);
		printf("      seekable stream decoder state = %u (%s)\n", (unsigned)state, FLAC__SeekableStreamDecoderStateString[state_]);
		if(state_ == FLAC__SEEKABLE_STREAM_DECODER_STREAM_DECODER_ERROR) {
			FLAC__StreamDecoderState state__ = FLAC__file_decoder_get_stream_decoder_state(decoder);
			printf("      stream decoder state = %u (%s)\n", (unsigned)state__, FLAC__StreamDecoderStateString[state__]);
		}
	}

	return false;
}

#ifdef FLAC__HAS_OGG
static FLAC__bool die_of_(const char *msg, const OggFLAC__FileDecoder *decoder)
{
	OggFLAC__FileDecoderState state = OggFLAC__file_decoder_get_state(decoder);

	if(msg)
		printf("FAILED, %s", msg);
	else
		printf("FAILED");

	printf(", state = %u (%s)\n", (unsigned)state, OggFLAC__SeekableStreamDecoderStateString[state]);
	if(state == OggFLAC__FILE_DECODER_SEEKABLE_STREAM_DECODER_ERROR) {
		OggFLAC__SeekableStreamDecoderState state_ = OggFLAC__file_decoder_get_seekable_stream_decoder_state(decoder);
		printf("      seekable stream decoder state = %u (%s)\n", (unsigned)state_, OggFLAC__SeekableStreamDecoderStateString[state_]);
		if(state_ == OggFLAC__SEEKABLE_STREAM_DECODER_STREAM_DECODER_ERROR) {
			OggFLAC__StreamDecoderState state__ = OggFLAC__file_decoder_get_stream_decoder_state(decoder);
			printf("      stream decoder state = %u (%s)\n", (unsigned)state__, OggFLAC__StreamDecoderStateString[state__]);
			if(state__ == OggFLAC__STREAM_DECODER_FLAC_STREAM_DECODER_ERROR) {
				FLAC__StreamDecoderState state___ = OggFLAC__file_decoder_get_FLAC_stream_decoder_state(decoder);
				printf("      FLAC stream decoder state = %u (%s)\n", (unsigned)state___, FLAC__StreamDecoderStateString[state___]);
			}
		}
	}

	return false;
}
#endif

static off_t get_filesize_(const char *srcpath)
{
	struct stat srcstat;

	if(0 == stat(srcpath, &srcstat))
		return srcstat.st_size;
	else
		return -1;
}

static FLAC__StreamDecoderWriteStatus file_decoder_write_callback_(const FLAC__FileDecoder *decoder, const FLAC__Frame *frame, const FLAC__int32 * const buffer[], void *client_data)
{
	decoder_client_data_struct *dcd = (decoder_client_data_struct*)client_data;

	(void)decoder, (void)buffer;

	if(0 == dcd) {
		printf("ERROR: client_data in write callback is NULL\n");
		return FLAC__STREAM_DECODER_WRITE_STATUS_ABORT;
	}

	if(dcd->error_occurred)
		return FLAC__STREAM_DECODER_WRITE_STATUS_ABORT;

	if (frame->header.number_type == FLAC__FRAME_NUMBER_TYPE_FRAME_NUMBER)
		printf("frame@%uf(%u)... ", frame->header.number.frame_number, frame->header.blocksize);
	else if (frame->header.number_type == FLAC__FRAME_NUMBER_TYPE_SAMPLE_NUMBER)
		printf("frame@%llu(%u)... ", frame->header.number.sample_number, frame->header.blocksize);
	else {
		FLAC__ASSERT(0);
		dcd->error_occurred = true;
		return FLAC__STREAM_DECODER_WRITE_STATUS_ABORT;
	}
	fflush(stdout);

	return FLAC__STREAM_DECODER_WRITE_STATUS_CONTINUE;
}

static void file_decoder_metadata_callback_(const FLAC__FileDecoder *decoder, const FLAC__StreamMetadata *metadata, void *client_data)
{
	decoder_client_data_struct *dcd = (decoder_client_data_struct*)client_data;

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

static void file_decoder_error_callback_(const FLAC__FileDecoder *decoder, FLAC__StreamDecoderErrorStatus status, void *client_data)
{
	decoder_client_data_struct *dcd = (decoder_client_data_struct*)client_data;

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

static FLAC__bool seek_barrage_native_flac(const char *filename, off_t filesize, unsigned count)
{
	FLAC__FileDecoder *decoder;
	decoder_client_data_struct decoder_client_data;
	unsigned i;
	long int n;

	decoder_client_data.got_data = false;
	decoder_client_data.total_samples = 0;
	decoder_client_data.ignore_errors = false;
	decoder_client_data.error_occurred = false;

	printf("\n+++ seek test: FLAC__FileDecoder\n\n");

	decoder = FLAC__file_decoder_new();
	if(0 == decoder)
		return die_("FLAC__file_decoder_new() FAILED, returned NULL\n");

	if(!FLAC__file_decoder_set_write_callback(decoder, file_decoder_write_callback_))
		return die_f_("FLAC__file_decoder_set_write_callback() FAILED", decoder);

	if(!FLAC__file_decoder_set_metadata_callback(decoder, file_decoder_metadata_callback_))
		return die_f_("FLAC__file_decoder_set_metadata_callback() FAILED", decoder);

	if(!FLAC__file_decoder_set_error_callback(decoder, file_decoder_error_callback_))
		return die_f_("FLAC__file_decoder_set_error_callback() FAILED", decoder);

	if(!FLAC__file_decoder_set_client_data(decoder, &decoder_client_data))
		return die_f_("FLAC__file_decoder_set_client_data() FAILED", decoder);

	if(!FLAC__file_decoder_set_filename(decoder, filename))
		return die_f_("FLAC__file_decoder_set_filename() FAILED", decoder);

	if(FLAC__file_decoder_init(decoder) != FLAC__FILE_DECODER_OK)
		return die_f_("FLAC__file_decoder_init() FAILED", decoder);

	if(!FLAC__file_decoder_process_until_end_of_metadata(decoder))
		return die_f_("FLAC__file_decoder_process_until_end_of_metadata() FAILED", decoder);

	printf("file's total_samples is %llu\n", decoder_client_data.total_samples);
#if !defined _MSC_VER && !defined __MINGW32__ && !defined __EMX__
	if (decoder_client_data.total_samples > (FLAC__uint64)RAND_MAX) {
		printf("ERROR: must be total_samples < %u\n", (unsigned)RAND_MAX);
		return false;
	}
#endif
	n = (long int)decoder_client_data.total_samples;

	/* if we don't have a total samples count, just guess based on the file size */
	/* @@@ should get it from last page's granulepos */
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

		printf("seek(%llu)... ", pos);
		fflush(stdout);
		if(!FLAC__file_decoder_seek_absolute(decoder, pos)) {
			if(pos < (FLAC__uint64)n && decoder_client_data.total_samples != 0)
				return die_f_("FLAC__file_decoder_seek_absolute() FAILED", decoder);
			else if(decoder_client_data.total_samples == 0)
				printf("seek failed, assuming it was past EOF... ");
			else
				printf("seek past end failed as expected... ");

			/* hack to work around a deficiency in the seek API's behavior */
			/* seeking past EOF sets the file decoder state to non-OK and there's no ..._flush() or ..._reset() call to reset it */
			if(!FLAC__file_decoder_finish(decoder))
				return die_f_("FLAC__file_decoder_finish() FAILED", decoder);

			if(!FLAC__file_decoder_set_write_callback(decoder, file_decoder_write_callback_))
				return die_f_("FLAC__file_decoder_set_write_callback() FAILED", decoder);

			if(!FLAC__file_decoder_set_metadata_callback(decoder, file_decoder_metadata_callback_))
				return die_f_("FLAC__file_decoder_set_metadata_callback() FAILED", decoder);

			if(!FLAC__file_decoder_set_error_callback(decoder, file_decoder_error_callback_))
				return die_f_("FLAC__file_decoder_set_error_callback() FAILED", decoder);

			if(!FLAC__file_decoder_set_client_data(decoder, &decoder_client_data))
				return die_f_("FLAC__file_decoder_set_client_data() FAILED", decoder);

			if(!FLAC__file_decoder_set_filename(decoder, filename))
				return die_f_("FLAC__file_decoder_set_filename() FAILED", decoder);

			if(FLAC__file_decoder_init(decoder) != FLAC__FILE_DECODER_OK)
				return die_f_("FLAC__file_decoder_init() FAILED", decoder);

			if(!FLAC__file_decoder_process_until_end_of_metadata(decoder))
				return die_f_("FLAC__file_decoder_process_until_end_of_metadata() FAILED", decoder);
		}
		else {
			printf("decode_frame... ");
			fflush(stdout);
			if(!FLAC__file_decoder_process_single(decoder))
				return die_f_("FLAC__file_decoder_process_single() FAILED", decoder);

			printf("decode_frame... ");
			fflush(stdout);
			if(!FLAC__file_decoder_process_single(decoder))
				return die_f_("FLAC__file_decoder_process_single() FAILED", decoder);
		}

		printf("OK\n");
		fflush(stdout);
	}

	if(FLAC__file_decoder_get_state(decoder) != FLAC__FILE_DECODER_UNINITIALIZED) {
		if(!FLAC__file_decoder_finish(decoder))
			return die_f_("FLAC__file_decoder_finish() FAILED", decoder);
	}

	printf("\nPASSED!\n");

	return true;
}

#ifdef FLAC__HAS_OGG
static FLAC__bool seek_barrage_ogg_flac(const char *filename, off_t filesize, unsigned count)
{
	OggFLAC__FileDecoder *decoder;
	decoder_client_data_struct decoder_client_data;
	unsigned i;
	long int n;

	decoder_client_data.got_data = false;
	decoder_client_data.total_samples = 0;
	decoder_client_data.ignore_errors = false;
	decoder_client_data.error_occurred = false;

	printf("\n+++ seek test: OggFLAC__FileDecoder\n\n");

	decoder = OggFLAC__file_decoder_new();
	if(0 == decoder)
		return die_("OggFLAC__file_decoder_new() FAILED, returned NULL\n");

	if(!OggFLAC__file_decoder_set_write_callback(decoder, (OggFLAC__FileDecoderWriteCallback)file_decoder_write_callback_))
		return die_of_("OggFLAC__file_decoder_set_write_callback() FAILED", decoder);

	if(!OggFLAC__file_decoder_set_metadata_callback(decoder, (OggFLAC__FileDecoderMetadataCallback)file_decoder_metadata_callback_))
		return die_of_("OggFLAC__file_decoder_set_metadata_callback() FAILED", decoder);

	if(!OggFLAC__file_decoder_set_error_callback(decoder, (OggFLAC__FileDecoderErrorCallback)file_decoder_error_callback_))
		return die_of_("OggFLAC__file_decoder_set_error_callback() FAILED", decoder);

	if(!OggFLAC__file_decoder_set_client_data(decoder, &decoder_client_data))
		return die_of_("OggFLAC__file_decoder_set_client_data() FAILED", decoder);

	if(!OggFLAC__file_decoder_set_filename(decoder, filename))
		return die_of_("OggFLAC__file_decoder_set_filename() FAILED", decoder);

	if(OggFLAC__file_decoder_init(decoder) != OggFLAC__FILE_DECODER_OK)
		return die_of_("OggFLAC__file_decoder_init() FAILED", decoder);

	if(!OggFLAC__file_decoder_process_until_end_of_metadata(decoder))
		return die_of_("OggFLAC__file_decoder_process_until_end_of_metadata() FAILED", decoder);

	printf("file's total_samples is %llu\n", decoder_client_data.total_samples);
#if !defined _MSC_VER && !defined __MINGW32__
	if (decoder_client_data.total_samples > (FLAC__uint64)RAND_MAX) {
		printf("ERROR: must be total_samples < %u\n", (unsigned)RAND_MAX);
		return false;
	}
#endif
	n = (long int)decoder_client_data.total_samples;

	/* if we don't have a total samples count, just guess based on the file size */
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

		printf("seek(%llu)... ", pos);
		fflush(stdout);
		if(!OggFLAC__file_decoder_seek_absolute(decoder, pos)) {
			if(pos < (FLAC__uint64)n && decoder_client_data.total_samples != 0)
				return die_of_("OggFLAC__file_decoder_seek_absolute() FAILED", decoder);
			else if(decoder_client_data.total_samples == 0)
				printf("seek failed, assuming it was past EOF... ");
			else
				printf("seek past end failed as expected... ");

			/* hack to work around a deficiency in the seek API's behavior */
			/* seeking past EOF sets the file decoder state to non-OK and there's no ..._flush() or ..._reset() call to reset it */
			if(!OggFLAC__file_decoder_finish(decoder))
				return die_of_("OggFLAC__file_decoder_finish() FAILED", decoder);

			if(!OggFLAC__file_decoder_set_write_callback(decoder, (OggFLAC__FileDecoderWriteCallback)file_decoder_write_callback_))
				return die_of_("OggFLAC__file_decoder_set_write_callback() FAILED", decoder);

			if(!OggFLAC__file_decoder_set_metadata_callback(decoder, (OggFLAC__FileDecoderMetadataCallback)file_decoder_metadata_callback_))
				return die_of_("OggFLAC__file_decoder_set_metadata_callback() FAILED", decoder);

			if(!OggFLAC__file_decoder_set_error_callback(decoder, (OggFLAC__FileDecoderErrorCallback)file_decoder_error_callback_))
				return die_of_("OggFLAC__file_decoder_set_error_callback() FAILED", decoder);

			if(!OggFLAC__file_decoder_set_client_data(decoder, &decoder_client_data))
				return die_of_("OggFLAC__file_decoder_set_client_data() FAILED", decoder);

			if(!OggFLAC__file_decoder_set_filename(decoder, filename))
				return die_of_("OggFLAC__file_decoder_set_filename() FAILED", decoder);

			if(OggFLAC__file_decoder_init(decoder) != OggFLAC__FILE_DECODER_OK)
				return die_of_("OggFLAC__file_decoder_init() FAILED", decoder);

			if(!OggFLAC__file_decoder_process_until_end_of_metadata(decoder))
				return die_of_("OggFLAC__file_decoder_process_until_end_of_metadata() FAILED", decoder);
		}
		else {
			printf("decode_frame... ");
			fflush(stdout);
			if(!OggFLAC__file_decoder_process_single(decoder))
				return die_of_("OggFLAC__file_decoder_process_single() FAILED", decoder);

			printf("decode_frame... ");
			fflush(stdout);
			if(!OggFLAC__file_decoder_process_single(decoder))
				return die_of_("OggFLAC__file_decoder_process_single() FAILED", decoder);
		}

		printf("OK\n");
		fflush(stdout);
	}

	if(OggFLAC__file_decoder_get_state(decoder) != OggFLAC__FILE_DECODER_UNINITIALIZED) {
		if(!OggFLAC__file_decoder_finish(decoder))
			return die_of_("OggFLAC__file_decoder_finish() FAILED", decoder);
	}

	printf("\nPASSED!\n");

	return true;
}
#endif

int main(int argc, char *argv[])
{
	const char *filename;
	unsigned count = 0;
	off_t filesize;

	static const char * const usage = "usage: test_seeking file.flac [#seeks]\n";

	if (argc < 1 || argc > 3) {
		fprintf(stderr, usage);
		return 1;
	}

	filename = argv[1];

	if (argc > 2)
		count = strtoul(argv[2], 0, 10);

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

	{
		FLAC__bool ok;
		if (strlen(filename) > 4 && 0 == strcmp(filename+strlen(filename)-4, ".ogg")) {
#ifdef FLAC__HAS_OGG
			ok = seek_barrage_ogg_flac(filename, filesize, count);
#else
			fprintf(stderr, "ERROR: Ogg FLAC not supported\n");
			ok = false;
#endif
		}
		else {
			ok = seek_barrage_native_flac(filename, filesize, count);
		}
		return ok? 0 : 2;
	}
}
