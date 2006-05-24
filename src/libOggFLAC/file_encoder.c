/* libOggFLAC - Free Lossless Audio Codec + Ogg library
 * Copyright (C) 2002,2003,2004,2005,2006  Josh Coalson
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * - Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *
 * - Redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in the
 * documentation and/or other materials provided with the distribution.
 *
 * - Neither the name of the Xiph.org Foundation nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#if HAVE_CONFIG_H
#  include <config.h>
#endif

#include <stdio.h>
#include <stdlib.h> /* for malloc() */
#include <string.h> /* for strlen(), strcpy() */
#if defined _MSC_VER || defined __MINGW32__
#include <sys/types.h> /* for off_t */
//@@@ [2G limit] hacks for MSVC6
#define fseeko fseek
#define ftello ftell
#endif
#include "FLAC/assert.h"
#include "OggFLAC/seekable_stream_encoder.h"
#include "protected/file_encoder.h"

#ifdef max
#undef max
#endif
#define max(x,y) ((x)>(y)?(x):(y))

/***********************************************************************
 *
 * Private class method prototypes
 *
 ***********************************************************************/

/* unpublished debug routines */
extern FLAC__bool OggFLAC__seekable_stream_encoder_disable_constant_subframes(OggFLAC__SeekableStreamEncoder *encoder, FLAC__bool value);
extern FLAC__bool OggFLAC__seekable_stream_encoder_disable_fixed_subframes(OggFLAC__SeekableStreamEncoder *encoder, FLAC__bool value);
extern FLAC__bool OggFLAC__seekable_stream_encoder_disable_verbatim_subframes(OggFLAC__SeekableStreamEncoder *encoder, FLAC__bool value);

static void set_defaults_(OggFLAC__FileEncoder *encoder);
static OggFLAC__SeekableStreamEncoderReadStatus read_callback_(const OggFLAC__SeekableStreamEncoder *encoder, FLAC__byte buffer[], unsigned *bytes, void *client_data);
static FLAC__SeekableStreamEncoderSeekStatus seek_callback_(const OggFLAC__SeekableStreamEncoder *encoder, FLAC__uint64 absolute_byte_offset, void *client_data);
static FLAC__SeekableStreamEncoderTellStatus tell_callback_(const OggFLAC__SeekableStreamEncoder *encoder, FLAC__uint64 *absolute_byte_offset, void *client_data);
static FLAC__StreamEncoderWriteStatus write_callback_(const OggFLAC__SeekableStreamEncoder *encoder, const FLAC__byte buffer[], unsigned bytes, unsigned samples, unsigned current_frame, void *client_data);

/***********************************************************************
 *
 * Private class data
 *
 ***********************************************************************/

typedef struct OggFLAC__FileEncoderPrivate {
	OggFLAC__FileEncoderProgressCallback progress_callback;
	void *client_data;
	char *filename;
	FLAC__uint64 bytes_written;
	FLAC__uint64 samples_written;
	unsigned frames_written;
	unsigned total_frames_estimate;
	OggFLAC__SeekableStreamEncoder *seekable_stream_encoder;
	FILE *file;
} OggFLAC__FileEncoderPrivate;

/***********************************************************************
 *
 * Public static class data
 *
 ***********************************************************************/

OggFLAC_API const char * const OggFLAC__FileEncoderStateString[] = {
	"OggFLAC__FILE_ENCODER_OK",
	"OggFLAC__FILE_ENCODER_NO_FILENAME",
	"OggFLAC__FILE_ENCODER_SEEKABLE_STREAM_ENCODER_ERROR",
	"OggFLAC__FILE_ENCODER_FATAL_ERROR_WHILE_WRITING",
	"OggFLAC__FILE_ENCODER_ERROR_OPENING_FILE",
	"OggFLAC__FILE_ENCODER_MEMORY_ALLOCATION_ERROR",
	"OggFLAC__FILE_ENCODER_ALREADY_INITIALIZED",
	"OggFLAC__FILE_ENCODER_UNINITIALIZED"
};


/***********************************************************************
 *
 * Class constructor/destructor
 *
 ***********************************************************************/

OggFLAC_API OggFLAC__FileEncoder *OggFLAC__file_encoder_new()
{
	OggFLAC__FileEncoder *encoder;

	FLAC__ASSERT(sizeof(int) >= 4); /* we want to die right away if this is not true */

	encoder = (OggFLAC__FileEncoder*)calloc(1, sizeof(OggFLAC__FileEncoder));
	if(encoder == 0) {
		return 0;
	}

	encoder->protected_ = (OggFLAC__FileEncoderProtected*)calloc(1, sizeof(OggFLAC__FileEncoderProtected));
	if(encoder->protected_ == 0) {
		free(encoder);
		return 0;
	}

	encoder->private_ = (OggFLAC__FileEncoderPrivate*)calloc(1, sizeof(OggFLAC__FileEncoderPrivate));
	if(encoder->private_ == 0) {
		free(encoder->protected_);
		free(encoder);
		return 0;
	}

	encoder->private_->seekable_stream_encoder = OggFLAC__seekable_stream_encoder_new();
	if(0 == encoder->private_->seekable_stream_encoder) {
		free(encoder->private_);
		free(encoder->protected_);
		free(encoder);
		return 0;
	}

	encoder->private_->file = 0;

	set_defaults_(encoder);

	encoder->protected_->state = OggFLAC__FILE_ENCODER_UNINITIALIZED;

	return encoder;
}

OggFLAC_API void OggFLAC__file_encoder_delete(OggFLAC__FileEncoder *encoder)
{
	FLAC__ASSERT(0 != encoder);
	FLAC__ASSERT(0 != encoder->protected_);
	FLAC__ASSERT(0 != encoder->private_);
	FLAC__ASSERT(0 != encoder->private_->seekable_stream_encoder);

	(void)OggFLAC__file_encoder_finish(encoder);

	OggFLAC__seekable_stream_encoder_delete(encoder->private_->seekable_stream_encoder);

	free(encoder->private_);
	free(encoder->protected_);
	free(encoder);
}

/***********************************************************************
 *
 * Public class methods
 *
 ***********************************************************************/

OggFLAC_API OggFLAC__FileEncoderState OggFLAC__file_encoder_init(OggFLAC__FileEncoder *encoder)
{
	FLAC__ASSERT(0 != encoder);

	if(encoder->protected_->state != OggFLAC__FILE_ENCODER_UNINITIALIZED)
		return encoder->protected_->state = OggFLAC__FILE_ENCODER_ALREADY_INITIALIZED;

	if(0 == encoder->private_->filename)
		return encoder->protected_->state = OggFLAC__FILE_ENCODER_NO_FILENAME;

	encoder->private_->file = fopen(encoder->private_->filename, "w+b");

	if(encoder->private_->file == 0)
		return encoder->protected_->state = OggFLAC__FILE_ENCODER_ERROR_OPENING_FILE;

	encoder->private_->bytes_written = 0;
	encoder->private_->samples_written = 0;
	encoder->private_->frames_written = 0;

	OggFLAC__seekable_stream_encoder_set_read_callback(encoder->private_->seekable_stream_encoder, read_callback_);
	OggFLAC__seekable_stream_encoder_set_seek_callback(encoder->private_->seekable_stream_encoder, seek_callback_);
	OggFLAC__seekable_stream_encoder_set_tell_callback(encoder->private_->seekable_stream_encoder, tell_callback_);
	OggFLAC__seekable_stream_encoder_set_write_callback(encoder->private_->seekable_stream_encoder, write_callback_);
	OggFLAC__seekable_stream_encoder_set_client_data(encoder->private_->seekable_stream_encoder, encoder);

	if(OggFLAC__seekable_stream_encoder_init(encoder->private_->seekable_stream_encoder) != OggFLAC__SEEKABLE_STREAM_ENCODER_OK)
		return encoder->protected_->state = OggFLAC__FILE_ENCODER_SEEKABLE_STREAM_ENCODER_ERROR;

	{
		unsigned blocksize = OggFLAC__file_encoder_get_blocksize(encoder);

		FLAC__ASSERT(blocksize != 0);
		encoder->private_->total_frames_estimate = (unsigned)((OggFLAC__file_encoder_get_total_samples_estimate(encoder) + blocksize - 1) / blocksize);
	}

	return encoder->protected_->state = OggFLAC__FILE_ENCODER_OK;
}

OggFLAC_API void OggFLAC__file_encoder_finish(OggFLAC__FileEncoder *encoder)
{
	FLAC__ASSERT(0 != encoder);

	if(encoder->protected_->state == OggFLAC__FILE_ENCODER_UNINITIALIZED)
		return;

	FLAC__ASSERT(0 != encoder->private_->seekable_stream_encoder);

	/* OggFLAC__seekable_stream_encoder_finish() might write data so we must close the file after it. */

	OggFLAC__seekable_stream_encoder_finish(encoder->private_->seekable_stream_encoder);

	if(0 != encoder->private_->file) {
		fclose(encoder->private_->file);
		encoder->private_->file = 0;
	}

	if(0 != encoder->private_->filename) {
		free(encoder->private_->filename);
		encoder->private_->filename = 0;
	}

	set_defaults_(encoder);

	encoder->protected_->state = OggFLAC__FILE_ENCODER_UNINITIALIZED;
}

OggFLAC_API FLAC__bool OggFLAC__file_encoder_set_serial_number(OggFLAC__FileEncoder *encoder, long serial_number)
{
	FLAC__ASSERT(0 != encoder);
	FLAC__ASSERT(0 != encoder->private_);
	FLAC__ASSERT(0 != encoder->protected_);
	FLAC__ASSERT(0 != encoder->private_->seekable_stream_encoder);
	if(encoder->protected_->state != OggFLAC__FILE_ENCODER_UNINITIALIZED)
		return false;
	return OggFLAC__seekable_stream_encoder_set_serial_number(encoder->private_->seekable_stream_encoder, serial_number);
}

OggFLAC_API FLAC__bool OggFLAC__file_encoder_set_verify(OggFLAC__FileEncoder *encoder, FLAC__bool value)
{
	FLAC__ASSERT(0 != encoder);
	FLAC__ASSERT(0 != encoder->private_);
	FLAC__ASSERT(0 != encoder->protected_);
	FLAC__ASSERT(0 != encoder->private_->seekable_stream_encoder);
	if(encoder->protected_->state != OggFLAC__FILE_ENCODER_UNINITIALIZED)
		return false;
	return OggFLAC__seekable_stream_encoder_set_verify(encoder->private_->seekable_stream_encoder, value);
}

OggFLAC_API FLAC__bool OggFLAC__file_encoder_set_streamable_subset(OggFLAC__FileEncoder *encoder, FLAC__bool value)
{
	FLAC__ASSERT(0 != encoder);
	FLAC__ASSERT(0 != encoder->private_);
	FLAC__ASSERT(0 != encoder->protected_);
	FLAC__ASSERT(0 != encoder->private_->seekable_stream_encoder);
	if(encoder->protected_->state != OggFLAC__FILE_ENCODER_UNINITIALIZED)
		return false;
	return OggFLAC__seekable_stream_encoder_set_streamable_subset(encoder->private_->seekable_stream_encoder, value);
}

OggFLAC_API FLAC__bool OggFLAC__file_encoder_set_do_mid_side_stereo(OggFLAC__FileEncoder *encoder, FLAC__bool value)
{
	FLAC__ASSERT(0 != encoder);
	FLAC__ASSERT(0 != encoder->private_);
	FLAC__ASSERT(0 != encoder->protected_);
	FLAC__ASSERT(0 != encoder->private_->seekable_stream_encoder);
	if(encoder->protected_->state != OggFLAC__FILE_ENCODER_UNINITIALIZED)
		return false;
	return OggFLAC__seekable_stream_encoder_set_do_mid_side_stereo(encoder->private_->seekable_stream_encoder, value);
}

OggFLAC_API FLAC__bool OggFLAC__file_encoder_set_loose_mid_side_stereo(OggFLAC__FileEncoder *encoder, FLAC__bool value)
{
	FLAC__ASSERT(0 != encoder);
	FLAC__ASSERT(0 != encoder->private_);
	FLAC__ASSERT(0 != encoder->protected_);
	FLAC__ASSERT(0 != encoder->private_->seekable_stream_encoder);
	if(encoder->protected_->state != OggFLAC__FILE_ENCODER_UNINITIALIZED)
		return false;
	return OggFLAC__seekable_stream_encoder_set_loose_mid_side_stereo(encoder->private_->seekable_stream_encoder, value);
}

OggFLAC_API FLAC__bool OggFLAC__file_encoder_set_channels(OggFLAC__FileEncoder *encoder, unsigned value)
{
	FLAC__ASSERT(0 != encoder);
	FLAC__ASSERT(0 != encoder->private_);
	FLAC__ASSERT(0 != encoder->protected_);
	FLAC__ASSERT(0 != encoder->private_->seekable_stream_encoder);
	if(encoder->protected_->state != OggFLAC__FILE_ENCODER_UNINITIALIZED)
		return false;
	return OggFLAC__seekable_stream_encoder_set_channels(encoder->private_->seekable_stream_encoder, value);
}

OggFLAC_API FLAC__bool OggFLAC__file_encoder_set_bits_per_sample(OggFLAC__FileEncoder *encoder, unsigned value)
{
	FLAC__ASSERT(0 != encoder);
	FLAC__ASSERT(0 != encoder->private_);
	FLAC__ASSERT(0 != encoder->protected_);
	FLAC__ASSERT(0 != encoder->private_->seekable_stream_encoder);
	if(encoder->protected_->state != OggFLAC__FILE_ENCODER_UNINITIALIZED)
		return false;
	return OggFLAC__seekable_stream_encoder_set_bits_per_sample(encoder->private_->seekable_stream_encoder, value);
}

OggFLAC_API FLAC__bool OggFLAC__file_encoder_set_sample_rate(OggFLAC__FileEncoder *encoder, unsigned value)
{
	FLAC__ASSERT(0 != encoder);
	FLAC__ASSERT(0 != encoder->private_);
	FLAC__ASSERT(0 != encoder->protected_);
	FLAC__ASSERT(0 != encoder->private_->seekable_stream_encoder);
	if(encoder->protected_->state != OggFLAC__FILE_ENCODER_UNINITIALIZED)
		return false;
	return OggFLAC__seekable_stream_encoder_set_sample_rate(encoder->private_->seekable_stream_encoder, value);
}

OggFLAC_API FLAC__bool OggFLAC__file_encoder_set_blocksize(OggFLAC__FileEncoder *encoder, unsigned value)
{
	FLAC__ASSERT(0 != encoder);
	FLAC__ASSERT(0 != encoder->private_);
	FLAC__ASSERT(0 != encoder->protected_);
	FLAC__ASSERT(0 != encoder->private_->seekable_stream_encoder);
	if(encoder->protected_->state != OggFLAC__FILE_ENCODER_UNINITIALIZED)
		return false;
	return OggFLAC__seekable_stream_encoder_set_blocksize(encoder->private_->seekable_stream_encoder, value);
}

OggFLAC_API FLAC__bool OggFLAC__file_encoder_set_apodization(OggFLAC__FileEncoder *encoder, const char *specification)
{
	FLAC__ASSERT(0 != encoder);
	FLAC__ASSERT(0 != encoder->private_);
	FLAC__ASSERT(0 != encoder->protected_);
	FLAC__ASSERT(0 != encoder->private_->seekable_stream_encoder);
	if(encoder->protected_->state != OggFLAC__FILE_ENCODER_UNINITIALIZED)
		return false;
	return OggFLAC__seekable_stream_encoder_set_apodization(encoder->private_->seekable_stream_encoder, specification);
}

OggFLAC_API FLAC__bool OggFLAC__file_encoder_set_max_lpc_order(OggFLAC__FileEncoder *encoder, unsigned value)
{
	FLAC__ASSERT(0 != encoder);
	FLAC__ASSERT(0 != encoder->private_);
	FLAC__ASSERT(0 != encoder->protected_);
	FLAC__ASSERT(0 != encoder->private_->seekable_stream_encoder);
	if(encoder->protected_->state != OggFLAC__FILE_ENCODER_UNINITIALIZED)
		return false;
	return OggFLAC__seekable_stream_encoder_set_max_lpc_order(encoder->private_->seekable_stream_encoder, value);
}

OggFLAC_API FLAC__bool OggFLAC__file_encoder_set_qlp_coeff_precision(OggFLAC__FileEncoder *encoder, unsigned value)
{
	FLAC__ASSERT(0 != encoder);
	FLAC__ASSERT(0 != encoder->private_);
	FLAC__ASSERT(0 != encoder->protected_);
	FLAC__ASSERT(0 != encoder->private_->seekable_stream_encoder);
	if(encoder->protected_->state != OggFLAC__FILE_ENCODER_UNINITIALIZED)
		return false;
	return OggFLAC__seekable_stream_encoder_set_qlp_coeff_precision(encoder->private_->seekable_stream_encoder, value);
}

OggFLAC_API FLAC__bool OggFLAC__file_encoder_set_do_qlp_coeff_prec_search(OggFLAC__FileEncoder *encoder, FLAC__bool value)
{
	FLAC__ASSERT(0 != encoder);
	FLAC__ASSERT(0 != encoder->private_);
	FLAC__ASSERT(0 != encoder->protected_);
	FLAC__ASSERT(0 != encoder->private_->seekable_stream_encoder);
	if(encoder->protected_->state != OggFLAC__FILE_ENCODER_UNINITIALIZED)
		return false;
	return OggFLAC__seekable_stream_encoder_set_do_qlp_coeff_prec_search(encoder->private_->seekable_stream_encoder, value);
}

OggFLAC_API FLAC__bool OggFLAC__file_encoder_set_do_escape_coding(OggFLAC__FileEncoder *encoder, FLAC__bool value)
{
	FLAC__ASSERT(0 != encoder);
	FLAC__ASSERT(0 != encoder->private_);
	FLAC__ASSERT(0 != encoder->protected_);
	FLAC__ASSERT(0 != encoder->private_->seekable_stream_encoder);
	if(encoder->protected_->state != OggFLAC__FILE_ENCODER_UNINITIALIZED)
		return false;
	return OggFLAC__seekable_stream_encoder_set_do_escape_coding(encoder->private_->seekable_stream_encoder, value);
}

OggFLAC_API FLAC__bool OggFLAC__file_encoder_set_do_exhaustive_model_search(OggFLAC__FileEncoder *encoder, FLAC__bool value)
{
	FLAC__ASSERT(0 != encoder);
	FLAC__ASSERT(0 != encoder->private_);
	FLAC__ASSERT(0 != encoder->protected_);
	FLAC__ASSERT(0 != encoder->private_->seekable_stream_encoder);
	if(encoder->protected_->state != OggFLAC__FILE_ENCODER_UNINITIALIZED)
		return false;
	return OggFLAC__seekable_stream_encoder_set_do_exhaustive_model_search(encoder->private_->seekable_stream_encoder, value);
}

OggFLAC_API FLAC__bool OggFLAC__file_encoder_set_min_residual_partition_order(OggFLAC__FileEncoder *encoder, unsigned value)
{
	FLAC__ASSERT(0 != encoder);
	FLAC__ASSERT(0 != encoder->private_);
	FLAC__ASSERT(0 != encoder->protected_);
	FLAC__ASSERT(0 != encoder->private_->seekable_stream_encoder);
	if(encoder->protected_->state != OggFLAC__FILE_ENCODER_UNINITIALIZED)
		return false;
	return OggFLAC__seekable_stream_encoder_set_min_residual_partition_order(encoder->private_->seekable_stream_encoder, value);
}

OggFLAC_API FLAC__bool OggFLAC__file_encoder_set_max_residual_partition_order(OggFLAC__FileEncoder *encoder, unsigned value)
{
	FLAC__ASSERT(0 != encoder);
	FLAC__ASSERT(0 != encoder->private_);
	FLAC__ASSERT(0 != encoder->protected_);
	FLAC__ASSERT(0 != encoder->private_->seekable_stream_encoder);
	if(encoder->protected_->state != OggFLAC__FILE_ENCODER_UNINITIALIZED)
		return false;
	return OggFLAC__seekable_stream_encoder_set_max_residual_partition_order(encoder->private_->seekable_stream_encoder, value);
}

OggFLAC_API FLAC__bool OggFLAC__file_encoder_set_rice_parameter_search_dist(OggFLAC__FileEncoder *encoder, unsigned value)
{
	FLAC__ASSERT(0 != encoder);
	FLAC__ASSERT(0 != encoder->private_);
	FLAC__ASSERT(0 != encoder->protected_);
	FLAC__ASSERT(0 != encoder->private_->seekable_stream_encoder);
	if(encoder->protected_->state != OggFLAC__FILE_ENCODER_UNINITIALIZED)
		return false;
	return OggFLAC__seekable_stream_encoder_set_rice_parameter_search_dist(encoder->private_->seekable_stream_encoder, value);
}

OggFLAC_API FLAC__bool OggFLAC__file_encoder_set_total_samples_estimate(OggFLAC__FileEncoder *encoder, FLAC__uint64 value)
{
	FLAC__ASSERT(0 != encoder);
	FLAC__ASSERT(0 != encoder->private_);
	FLAC__ASSERT(0 != encoder->protected_);
	FLAC__ASSERT(0 != encoder->private_->seekable_stream_encoder);
	if(encoder->protected_->state != OggFLAC__FILE_ENCODER_UNINITIALIZED)
		return false;
	return OggFLAC__seekable_stream_encoder_set_total_samples_estimate(encoder->private_->seekable_stream_encoder, value);
}

OggFLAC_API FLAC__bool OggFLAC__file_encoder_set_metadata(OggFLAC__FileEncoder *encoder, FLAC__StreamMetadata **metadata, unsigned num_blocks)
{
	FLAC__ASSERT(0 != encoder);
	FLAC__ASSERT(0 != encoder->private_);
	FLAC__ASSERT(0 != encoder->protected_);
	FLAC__ASSERT(0 != encoder->private_->seekable_stream_encoder);
	if(encoder->protected_->state != OggFLAC__FILE_ENCODER_UNINITIALIZED)
		return false;
	return OggFLAC__seekable_stream_encoder_set_metadata(encoder->private_->seekable_stream_encoder, metadata, num_blocks);
}

OggFLAC_API FLAC__bool OggFLAC__file_encoder_set_filename(OggFLAC__FileEncoder *encoder, const char *value)
{
	FLAC__ASSERT(0 != encoder);
	FLAC__ASSERT(0 != encoder->private_);
	FLAC__ASSERT(0 != encoder->protected_);
	FLAC__ASSERT(0 != value);
	if(encoder->protected_->state != OggFLAC__FILE_ENCODER_UNINITIALIZED)
		return false;
	if(0 != encoder->private_->filename) {
		free(encoder->private_->filename);
		encoder->private_->filename = 0;
	}
	if(0 == (encoder->private_->filename = (char*)malloc(strlen(value)+1))) {
		encoder->protected_->state = OggFLAC__FILE_ENCODER_MEMORY_ALLOCATION_ERROR;
		return false;
	}
	strcpy(encoder->private_->filename, value);
	return true;
}

OggFLAC_API FLAC__bool OggFLAC__file_encoder_set_progress_callback(OggFLAC__FileEncoder *encoder, OggFLAC__FileEncoderProgressCallback value)
{
	FLAC__ASSERT(0 != encoder);
	FLAC__ASSERT(0 != encoder->private_);
	FLAC__ASSERT(0 != encoder->protected_);
	if(encoder->protected_->state != OggFLAC__FILE_ENCODER_UNINITIALIZED)
		return false;
	encoder->private_->progress_callback = value;
	return true;
}

OggFLAC_API FLAC__bool OggFLAC__file_encoder_set_client_data(OggFLAC__FileEncoder *encoder, void *value)
{
	FLAC__ASSERT(0 != encoder);
	FLAC__ASSERT(0 != encoder->private_);
	FLAC__ASSERT(0 != encoder->protected_);
	if(encoder->protected_->state != OggFLAC__FILE_ENCODER_UNINITIALIZED)
		return false;
	encoder->private_->client_data = value;
	return true;
}

/*
 * These three functions are not static, but not publically exposed in
 * include/OggFLAC/ either.  They are used by the test suite.
 */
OggFLAC_API FLAC__bool OggFLAC__file_encoder_disable_constant_subframes(OggFLAC__FileEncoder *encoder, FLAC__bool value)
{
	FLAC__ASSERT(0 != encoder);
	FLAC__ASSERT(0 != encoder->private_);
	FLAC__ASSERT(0 != encoder->protected_);
	if(encoder->protected_->state != OggFLAC__FILE_ENCODER_UNINITIALIZED)
		return false;
	return OggFLAC__seekable_stream_encoder_disable_constant_subframes(encoder->private_->seekable_stream_encoder, value);
}

OggFLAC_API FLAC__bool OggFLAC__file_encoder_disable_fixed_subframes(OggFLAC__FileEncoder *encoder, FLAC__bool value)
{
	FLAC__ASSERT(0 != encoder);
	FLAC__ASSERT(0 != encoder->private_);
	FLAC__ASSERT(0 != encoder->protected_);
	if(encoder->protected_->state != OggFLAC__FILE_ENCODER_UNINITIALIZED)
		return false;
	return OggFLAC__seekable_stream_encoder_disable_fixed_subframes(encoder->private_->seekable_stream_encoder, value);
}

OggFLAC_API FLAC__bool OggFLAC__file_encoder_disable_verbatim_subframes(OggFLAC__FileEncoder *encoder, FLAC__bool value)
{
	FLAC__ASSERT(0 != encoder);
	FLAC__ASSERT(0 != encoder->private_);
	FLAC__ASSERT(0 != encoder->protected_);
	if(encoder->protected_->state != OggFLAC__FILE_ENCODER_UNINITIALIZED)
		return false;
	return OggFLAC__seekable_stream_encoder_disable_verbatim_subframes(encoder->private_->seekable_stream_encoder, value);
}

OggFLAC_API OggFLAC__FileEncoderState OggFLAC__file_encoder_get_state(const OggFLAC__FileEncoder *encoder)
{
	FLAC__ASSERT(0 != encoder);
	FLAC__ASSERT(0 != encoder->protected_);
	return encoder->protected_->state;
}

OggFLAC_API OggFLAC__SeekableStreamEncoderState OggFLAC__file_encoder_get_seekable_stream_encoder_state(const OggFLAC__FileEncoder *encoder)
{
	FLAC__ASSERT(0 != encoder);
	FLAC__ASSERT(0 != encoder->private_);
	return OggFLAC__seekable_stream_encoder_get_state(encoder->private_->seekable_stream_encoder);
}

OggFLAC_API FLAC__StreamEncoderState OggFLAC__file_encoder_get_FLAC_stream_encoder_state(const OggFLAC__FileEncoder *encoder)
{
	FLAC__ASSERT(0 != encoder);
	FLAC__ASSERT(0 != encoder->private_);
	return OggFLAC__seekable_stream_encoder_get_FLAC_stream_encoder_state(encoder->private_->seekable_stream_encoder);
}

OggFLAC_API FLAC__StreamDecoderState OggFLAC__file_encoder_get_verify_decoder_state(const OggFLAC__FileEncoder *encoder)
{
	FLAC__ASSERT(0 != encoder);
	FLAC__ASSERT(0 != encoder->private_);
	return OggFLAC__seekable_stream_encoder_get_verify_decoder_state(encoder->private_->seekable_stream_encoder);
}

OggFLAC_API const char *OggFLAC__file_encoder_get_resolved_state_string(const OggFLAC__FileEncoder *encoder)
{
	if(encoder->protected_->state != OggFLAC__FILE_ENCODER_SEEKABLE_STREAM_ENCODER_ERROR)
		return OggFLAC__FileEncoderStateString[encoder->protected_->state];
	else
		return OggFLAC__seekable_stream_encoder_get_resolved_state_string(encoder->private_->seekable_stream_encoder);
}

OggFLAC_API void OggFLAC__file_encoder_get_verify_decoder_error_stats(const OggFLAC__FileEncoder *encoder, FLAC__uint64 *absolute_sample, unsigned *frame_number, unsigned *channel, unsigned *sample, FLAC__int32 *expected, FLAC__int32 *got)
{
	FLAC__ASSERT(0 != encoder);
	FLAC__ASSERT(0 != encoder->private_);
	OggFLAC__seekable_stream_encoder_get_verify_decoder_error_stats(encoder->private_->seekable_stream_encoder, absolute_sample, frame_number, channel, sample, expected, got);
}

OggFLAC_API FLAC__bool OggFLAC__file_encoder_get_verify(const OggFLAC__FileEncoder *encoder)
{
	FLAC__ASSERT(0 != encoder);
	FLAC__ASSERT(0 != encoder->private_);
	return OggFLAC__seekable_stream_encoder_get_verify(encoder->private_->seekable_stream_encoder);
}

OggFLAC_API FLAC__bool OggFLAC__file_encoder_get_streamable_subset(const OggFLAC__FileEncoder *encoder)
{
	FLAC__ASSERT(0 != encoder);
	FLAC__ASSERT(0 != encoder->private_);
	return OggFLAC__seekable_stream_encoder_get_streamable_subset(encoder->private_->seekable_stream_encoder);
}

OggFLAC_API FLAC__bool OggFLAC__file_encoder_get_do_mid_side_stereo(const OggFLAC__FileEncoder *encoder)
{
	FLAC__ASSERT(0 != encoder);
	FLAC__ASSERT(0 != encoder->private_);
	return OggFLAC__seekable_stream_encoder_get_do_mid_side_stereo(encoder->private_->seekable_stream_encoder);
}

OggFLAC_API FLAC__bool OggFLAC__file_encoder_get_loose_mid_side_stereo(const OggFLAC__FileEncoder *encoder)
{
	FLAC__ASSERT(0 != encoder);
	FLAC__ASSERT(0 != encoder->private_);
	return OggFLAC__seekable_stream_encoder_get_loose_mid_side_stereo(encoder->private_->seekable_stream_encoder);
}

OggFLAC_API unsigned OggFLAC__file_encoder_get_channels(const OggFLAC__FileEncoder *encoder)
{
	FLAC__ASSERT(0 != encoder);
	FLAC__ASSERT(0 != encoder->private_);
	return OggFLAC__seekable_stream_encoder_get_channels(encoder->private_->seekable_stream_encoder);
}

OggFLAC_API unsigned OggFLAC__file_encoder_get_bits_per_sample(const OggFLAC__FileEncoder *encoder)
{
	FLAC__ASSERT(0 != encoder);
	FLAC__ASSERT(0 != encoder->private_);
	return OggFLAC__seekable_stream_encoder_get_bits_per_sample(encoder->private_->seekable_stream_encoder);
}

OggFLAC_API unsigned OggFLAC__file_encoder_get_sample_rate(const OggFLAC__FileEncoder *encoder)
{
	FLAC__ASSERT(0 != encoder);
	FLAC__ASSERT(0 != encoder->private_);
	return OggFLAC__seekable_stream_encoder_get_sample_rate(encoder->private_->seekable_stream_encoder);
}

OggFLAC_API unsigned OggFLAC__file_encoder_get_blocksize(const OggFLAC__FileEncoder *encoder)
{
	FLAC__ASSERT(0 != encoder);
	FLAC__ASSERT(0 != encoder->private_);
	return OggFLAC__seekable_stream_encoder_get_blocksize(encoder->private_->seekable_stream_encoder);
}

OggFLAC_API unsigned OggFLAC__file_encoder_get_max_lpc_order(const OggFLAC__FileEncoder *encoder)
{
	FLAC__ASSERT(0 != encoder);
	FLAC__ASSERT(0 != encoder->private_);
	return OggFLAC__seekable_stream_encoder_get_max_lpc_order(encoder->private_->seekable_stream_encoder);
}

OggFLAC_API unsigned OggFLAC__file_encoder_get_qlp_coeff_precision(const OggFLAC__FileEncoder *encoder)
{
	FLAC__ASSERT(0 != encoder);
	FLAC__ASSERT(0 != encoder->private_);
	return OggFLAC__seekable_stream_encoder_get_qlp_coeff_precision(encoder->private_->seekable_stream_encoder);
}

OggFLAC_API FLAC__bool OggFLAC__file_encoder_get_do_qlp_coeff_prec_search(const OggFLAC__FileEncoder *encoder)
{
	FLAC__ASSERT(0 != encoder);
	FLAC__ASSERT(0 != encoder->private_);
	return OggFLAC__seekable_stream_encoder_get_do_qlp_coeff_prec_search(encoder->private_->seekable_stream_encoder);
}

OggFLAC_API FLAC__bool OggFLAC__file_encoder_get_do_escape_coding(const OggFLAC__FileEncoder *encoder)
{
	FLAC__ASSERT(0 != encoder);
	FLAC__ASSERT(0 != encoder->private_);
	return OggFLAC__seekable_stream_encoder_get_do_escape_coding(encoder->private_->seekable_stream_encoder);
}

OggFLAC_API FLAC__bool OggFLAC__file_encoder_get_do_exhaustive_model_search(const OggFLAC__FileEncoder *encoder)
{
	FLAC__ASSERT(0 != encoder);
	FLAC__ASSERT(0 != encoder->private_);
	return OggFLAC__seekable_stream_encoder_get_do_exhaustive_model_search(encoder->private_->seekable_stream_encoder);
}

OggFLAC_API unsigned OggFLAC__file_encoder_get_min_residual_partition_order(const OggFLAC__FileEncoder *encoder)
{
	FLAC__ASSERT(0 != encoder);
	FLAC__ASSERT(0 != encoder->private_);
	return OggFLAC__seekable_stream_encoder_get_min_residual_partition_order(encoder->private_->seekable_stream_encoder);
}

OggFLAC_API unsigned OggFLAC__file_encoder_get_max_residual_partition_order(const OggFLAC__FileEncoder *encoder)
{
	FLAC__ASSERT(0 != encoder);
	FLAC__ASSERT(0 != encoder->private_);
	return OggFLAC__seekable_stream_encoder_get_max_residual_partition_order(encoder->private_->seekable_stream_encoder);
}

OggFLAC_API unsigned OggFLAC__file_encoder_get_rice_parameter_search_dist(const OggFLAC__FileEncoder *encoder)
{
	FLAC__ASSERT(0 != encoder);
	FLAC__ASSERT(0 != encoder->private_);
	return OggFLAC__seekable_stream_encoder_get_rice_parameter_search_dist(encoder->private_->seekable_stream_encoder);
}

OggFLAC_API FLAC__uint64 OggFLAC__file_encoder_get_total_samples_estimate(const OggFLAC__FileEncoder *encoder)
{
	FLAC__ASSERT(0 != encoder);
	FLAC__ASSERT(0 != encoder->private_);
	return OggFLAC__seekable_stream_encoder_get_total_samples_estimate(encoder->private_->seekable_stream_encoder);
}

OggFLAC_API FLAC__bool OggFLAC__file_encoder_process(OggFLAC__FileEncoder *encoder, const FLAC__int32 * const buffer[], unsigned samples)
{
	FLAC__ASSERT(0 != encoder);
	FLAC__ASSERT(0 != encoder->private_);
	if(!OggFLAC__seekable_stream_encoder_process(encoder->private_->seekable_stream_encoder, buffer, samples)) {
		encoder->protected_->state = OggFLAC__FILE_ENCODER_SEEKABLE_STREAM_ENCODER_ERROR;
		return false;
	}
	else
		return true;
}

/* 'samples' is channel-wide samples, e.g. for 1 second at 44100Hz, 'samples' = 44100 regardless of the number of channels */
OggFLAC_API FLAC__bool OggFLAC__file_encoder_process_interleaved(OggFLAC__FileEncoder *encoder, const FLAC__int32 buffer[], unsigned samples)
{
	FLAC__ASSERT(0 != encoder);
	FLAC__ASSERT(0 != encoder->private_);
	if(!OggFLAC__seekable_stream_encoder_process_interleaved(encoder->private_->seekable_stream_encoder, buffer, samples)) {
		encoder->protected_->state = OggFLAC__FILE_ENCODER_SEEKABLE_STREAM_ENCODER_ERROR;
		return false;
	}
	else
		return true;
}


/***********************************************************************
 *
 * Private class methods
 *
 ***********************************************************************/

void set_defaults_(OggFLAC__FileEncoder *encoder)
{
	FLAC__ASSERT(0 != encoder);
	FLAC__ASSERT(0 != encoder->private_);

	encoder->private_->progress_callback = 0;
	encoder->private_->client_data = 0;
	encoder->private_->total_frames_estimate = 0;
	encoder->private_->filename = 0;
}

OggFLAC__SeekableStreamEncoderReadStatus read_callback_(const OggFLAC__SeekableStreamEncoder *encoder, FLAC__byte buffer[], unsigned *bytes, void *client_data)
{
	OggFLAC__FileEncoder *file_encoder = (OggFLAC__FileEncoder*)client_data;

	(void)encoder;

	FLAC__ASSERT(0 != file_encoder);

	*bytes = (unsigned)fread(buffer, 1, *bytes, file_encoder->private_->file);
	if (*bytes == 0) {
		if (feof(file_encoder->private_->file))
			return OggFLAC__SEEKABLE_STREAM_ENCODER_READ_STATUS_END_OF_STREAM;
		else if (ferror(file_encoder->private_->file))
			return OggFLAC__SEEKABLE_STREAM_ENCODER_READ_STATUS_ABORT;
	}
	return OggFLAC__SEEKABLE_STREAM_ENCODER_READ_STATUS_CONTINUE;
}

FLAC__SeekableStreamEncoderSeekStatus seek_callback_(const OggFLAC__SeekableStreamEncoder *encoder, FLAC__uint64 absolute_byte_offset, void *client_data)
{
	OggFLAC__FileEncoder *file_encoder = (OggFLAC__FileEncoder*)client_data;

	(void)encoder;

	FLAC__ASSERT(0 != file_encoder);

	if(fseeko(file_encoder->private_->file, (off_t)absolute_byte_offset, SEEK_SET) < 0)
		return FLAC__SEEKABLE_STREAM_ENCODER_SEEK_STATUS_ERROR;
	else
		return FLAC__SEEKABLE_STREAM_ENCODER_SEEK_STATUS_OK;
}

FLAC__SeekableStreamEncoderTellStatus tell_callback_(const OggFLAC__SeekableStreamEncoder *encoder, FLAC__uint64 *absolute_byte_offset, void *client_data)
{
	OggFLAC__FileEncoder *file_encoder = (OggFLAC__FileEncoder*)client_data;
	off_t offset;

	(void)encoder;

	FLAC__ASSERT(0 != file_encoder);

	offset = ftello(file_encoder->private_->file);

	if(offset < 0) {
		return FLAC__SEEKABLE_STREAM_ENCODER_TELL_STATUS_ERROR;
	}
	else {
		*absolute_byte_offset = (FLAC__uint64)offset;
		return FLAC__SEEKABLE_STREAM_ENCODER_TELL_STATUS_OK;
	}
}

#ifdef FLAC__VALGRIND_TESTING
static size_t local__fwrite(const void *ptr, size_t size, size_t nmemb, FILE *stream)
{
	size_t ret = fwrite(ptr, size, nmemb, stream);
	if(!ferror(stream))
		fflush(stream);
	return ret;
}
#else
#define local__fwrite fwrite
#endif

FLAC__StreamEncoderWriteStatus write_callback_(const OggFLAC__SeekableStreamEncoder *encoder, const FLAC__byte buffer[], unsigned bytes, unsigned samples, unsigned current_frame, void *client_data)
{
	OggFLAC__FileEncoder *file_encoder = (OggFLAC__FileEncoder*)client_data;

	(void)encoder;

	FLAC__ASSERT(0 != file_encoder);

	if(local__fwrite(buffer, sizeof(FLAC__byte), bytes, file_encoder->private_->file) == bytes) {
		file_encoder->private_->bytes_written += bytes;
		file_encoder->private_->samples_written += samples;
		/* we keep a high watermark on the number of frames written
		 * because when the encoder goes back to write metadata,
		 * 'current_frame' will drop back to 0.
		 */
		file_encoder->private_->frames_written = max(file_encoder->private_->frames_written, current_frame+1);
		/*@@@ We would like to add an '&& samples > 0' to the if
		 * clause here but currently because of the nature of our Ogg
		 * writing implementation, 'samples' is always 0 (see
		 * ogg_encoder_aspect.c).  The downside is extra progress
		 * callbacks.
		 */
		if(0 != file_encoder->private_->progress_callback)
			file_encoder->private_->progress_callback(file_encoder, file_encoder->private_->bytes_written, file_encoder->private_->samples_written, file_encoder->private_->frames_written, file_encoder->private_->total_frames_estimate, file_encoder->private_->client_data);
		return FLAC__STREAM_ENCODER_WRITE_STATUS_OK;
	}
	else
		return FLAC__STREAM_ENCODER_WRITE_STATUS_FATAL_ERROR;
}
