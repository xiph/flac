/* test_seeking - Seeking tester for libFLAC and libOggFLAC
 * Copyright (C) 2004  Josh Coalson
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

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#if defined _MSC_VER || defined __MINGW32__
#include <time.h>
#else
#include <sys/time.h>
#endif
#include "FLAC/assert.h"
#include "FLAC/file_decoder.h"
#include "OggFLAC/file_decoder.h"

typedef struct {
	FLAC__uint64 total_samples;
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

	if (metadata->type == FLAC__METADATA_TYPE_STREAMINFO)
		dcd->total_samples = metadata->data.stream_info.total_samples;
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

static FLAC__bool seek_barrage_native_flac(const char *filename, unsigned count)
{
	FLAC__FileDecoder *decoder;
	decoder_client_data_struct decoder_client_data;
	unsigned i;
	long int n;

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
	if (decoder_client_data.total_samples == 0 || decoder_client_data.total_samples > (FLAC__uint64)RAND_MAX) {
		printf("ERROR: must be 0 < total_samples < %u\n", (unsigned)RAND_MAX);
		return false;
	}
	n = (long int)decoder_client_data.total_samples;

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
		else {
#if !defined _MSC_VER && !defined __MINGW32__
			pos = (FLAC__uint64)(random() % n);
#else
			pos = (FLAC__uint64)(rand() % n);
#endif
		}

		printf("seek(%llu)... ", pos);
		fflush(stdout);
		if(!FLAC__file_decoder_seek_absolute(decoder, pos))
			return die_f_("FLAC__file_decoder_seek_absolute() FAILED", decoder);

		printf("decode_frame... ");
		fflush(stdout);
		if(!FLAC__file_decoder_process_single(decoder))
			return die_f_("FLAC__file_decoder_process_single() FAILED", decoder);

		printf("decode_frame... ");
		fflush(stdout);
		if(!FLAC__file_decoder_process_single(decoder))
			return die_f_("FLAC__file_decoder_process_single() FAILED", decoder);

		printf("OK\n");
		fflush(stdout);
	}

	printf("\nPASSED!\n");

	return true;
}

static FLAC__bool seek_barrage_ogg_flac(const char *filename, unsigned count)
{
	OggFLAC__FileDecoder *decoder;
	decoder_client_data_struct decoder_client_data;
	unsigned i;
	long int n;

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
	if (decoder_client_data.total_samples == 0 || decoder_client_data.total_samples > (FLAC__uint64)RAND_MAX) {
		printf("ERROR: must be 0 < total_samples < %u\n", (unsigned)RAND_MAX);
		return false;
	}
	n = (long int)decoder_client_data.total_samples;

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
		else {
#if !defined _MSC_VER && !defined __MINGW32__
			pos = (FLAC__uint64)(random() % n);
#else
			pos = (FLAC__uint64)(rand() % n);
#endif
		}

		printf("seek(%llu)... ", pos);
		fflush(stdout);
		if(!OggFLAC__file_decoder_seek_absolute(decoder, pos))
			return die_of_("OggFLAC__file_decoder_seek_absolute() FAILED", decoder);

		printf("decode_frame... ");
		fflush(stdout);
		if(!OggFLAC__file_decoder_process_single(decoder))
			return die_of_("OggFLAC__file_decoder_process_single() FAILED", decoder);

		printf("decode_frame... ");
		fflush(stdout);
		if(!OggFLAC__file_decoder_process_single(decoder))
			return die_of_("OggFLAC__file_decoder_process_single() FAILED", decoder);

		printf("OK\n");
		fflush(stdout);
	}

	printf("\nPASSED!\n");

	return true;
}

int main(int argc, char *argv[])
{
	const char *filename;
	unsigned count = 0;

	static const char * const usage = "usage: test_seeking file.flac [#seeks]\n";

	if (argc < 1 || argc > 3) {
		fprintf(stderr, usage);
		return 1;
	}

	filename = argv[1];

	if (argc > 2)
		count = strtoul(argv[2], 0, 10);

	if (count < 20)
		fprintf(stderr, "WARNING: random seeks don't kick in until after 20 preprogrammed ones\n");

#if !defined _MSC_VER && !defined __MINGW32__
	{
		struct timeval tv;

		if(gettimeofday(&tv, 0) < 0) {
			fprintf(stderr, "WARNING: couldn't seed RNG with time\n");
			tv.tv_usec = 4321;
		}
		srandom(tv.tv_usec);
	}
#else
	srand(time(0));
#endif

	(void) signal(SIGINT, our_sigint_handler_);

	{
		FLAC__bool ok;
		if (strlen(filename) > 4 && 0 == strcmp(filename+strlen(filename)-4, ".ogg"))
			ok = seek_barrage_ogg_flac(filename, count);
		else
			ok = seek_barrage_native_flac(filename, count);
		return ok? 0 : 2;
	}
}
