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

#if defined _MSC_VER || defined __MINGW32__
#include <io.h> /* for _setmode() */
#include <fcntl.h> /* for _O_BINARY */
#endif
#if defined __CYGWIN__ || defined __EMX__
#include <io.h> /* for setmode(), O_BINARY */
#include <fcntl.h> /* for _O_BINARY */
#endif
#include <stdio.h>
#include <stdlib.h> /* for calloc() */
#include <string.h> /* for memcpy() */
#include <sys/types.h> /* for off_t */
#if defined _MSC_VER || defined __MINGW32__
/*@@@ [2G limit] hacks for MSVC6 */
#define fseeko fseek
#define ftello ftell
#endif
#include "FLAC/assert.h"
#include "OggFLAC/stream_encoder.h"
#include "protected/stream_encoder.h"
#include "private/ogg_helper.h"

#ifdef max
#undef max
#endif
#define max(a,b) ((a)>(b)?(a):(b))

/***********************************************************************
 *
 * Private class method prototypes
 *
 ***********************************************************************/

/* unpublished debug routines */
extern FLAC__bool FLAC__stream_encoder_disable_constant_subframes(FLAC__StreamEncoder *encoder, FLAC__bool value);
extern FLAC__bool FLAC__stream_encoder_disable_fixed_subframes(FLAC__StreamEncoder *encoder, FLAC__bool value);
extern FLAC__bool FLAC__stream_encoder_disable_verbatim_subframes(FLAC__StreamEncoder *encoder, FLAC__bool value);

static void set_defaults_(OggFLAC__StreamEncoder *encoder);
static FLAC__StreamEncoderWriteStatus write_callback_(const FLAC__StreamEncoder *encoder, const FLAC__byte buffer[], unsigned bytes, unsigned samples, unsigned current_frame, void *client_data);
static void metadata_callback_(const FLAC__StreamEncoder *encoder, const FLAC__StreamMetadata *metadata, void *client_data);
static OggFLAC__StreamEncoderReadStatus file_read_callback_(const OggFLAC__StreamEncoder *encoder, FLAC__byte buffer[], unsigned *bytes, void *client_data);
static FLAC__StreamEncoderSeekStatus file_seek_callback_(const FLAC__StreamEncoder *encoder, FLAC__uint64 absolute_byte_offset, void *client_data);
static FLAC__StreamEncoderTellStatus file_tell_callback_(const FLAC__StreamEncoder *encoder, FLAC__uint64 *absolute_byte_offset, void *client_data);
static FLAC__StreamEncoderWriteStatus file_write_callback_(const FLAC__StreamEncoder *encoder, const FLAC__byte buffer[], unsigned bytes, unsigned samples, unsigned current_frame, void *client_data);
static FILE *get_binary_stdout_();


/***********************************************************************
 *
 * Private class data
 *
 ***********************************************************************/

typedef struct OggFLAC__StreamEncoderPrivate {
	OggFLAC__StreamEncoderReadCallback read_callback;
	FLAC__StreamEncoderWriteCallback write_callback;
	FLAC__StreamEncoderSeekCallback seek_callback;
	FLAC__StreamEncoderTellCallback tell_callback;
	FLAC__StreamEncoderMetadataCallback metadata_callback;
	FLAC__StreamEncoderProgressCallback progress_callback;
	void *client_data;
#if 0 //@@@@@@
	FLAC__StreamEncoder *FLAC_stream_encoder;
#endif
	FLAC__StreamMetadata_SeekTable *seek_table;
	/* internal vars (all the above are class settings) */
	unsigned first_seekpoint_to_check;
	FILE *file;                            /* only used when encoding to a file */
	FLAC__uint64 bytes_written;
	FLAC__uint64 samples_written;
	unsigned frames_written;
	unsigned total_frames_estimate;
} OggFLAC__StreamEncoderPrivate;


/***********************************************************************
 *
 * Public static class data
 *
 ***********************************************************************/

OggFLAC_API const char * const OggFLAC__StreamEncoderStateString[] = {
	"OggFLAC__STREAM_ENCODER_OK",
	"OggFLAC__STREAM_ENCODER_UNINITIALIZED",
	"OggFLAC__STREAM_ENCODER_OGG_ERROR",
	"OggFLAC__STREAM_ENCODER_FLAC_STREAM_ENCODER_ERROR",
	"OggFLAC__STREAM_ENCODER_CLIENT_ERROR",
	"OggFLAC__STREAM_ENCODER_IO_ERROR",
	"OggFLAC__STREAM_ENCODER_MEMORY_ALLOCATION_ERROR"
};

OggFLAC_API const char * const OggFLAC__treamEncoderReadStatusString[] = {
	"OggFLAC__STREAM_ENCODER_READ_STATUS_CONTINUE",
	"OggFLAC__STREAM_ENCODER_READ_STATUS_END_OF_STREAM",
	"OggFLAC__STREAM_ENCODER_READ_STATUS_ABORT",
	"OggFLAC__STREAM_ENCODER_READ_STATUS_UNSUPPORTED"
};


/***********************************************************************
 *
 * Class constructor/destructor
 *
 */
OggFLAC_API OggFLAC__StreamEncoder *OggFLAC__stream_encoder_new()
{
	OggFLAC__StreamEncoder *encoder;
	FLAC__StreamEncoder *parent;

	FLAC__ASSERT(sizeof(int) >= 4); /* we want to die right away if this is not true */

	encoder = (OggFLAC__StreamEncoder*)calloc(1, sizeof(OggFLAC__StreamEncoder));
	if(encoder == 0) {
		return 0;
	}

	encoder->protected_ = (OggFLAC__StreamEncoderProtected*)calloc(1, sizeof(OggFLAC__StreamEncoderProtected));
	if(encoder->protected_ == 0) {
		free(encoder);
		return 0;
	}

	encoder->private_ = (OggFLAC__StreamEncoderPrivate*)calloc(1, sizeof(OggFLAC__StreamEncoderPrivate));
	if(encoder->private_ == 0) {
		free(encoder->protected_);
		free(encoder);
		return 0;
	}

	parent = FLAC__stream_encoder_new();
	if(0 == parent) {
		free(encoder->private_);
		free(encoder->protected_);
		free(encoder);
		return 0;
	}
	encoder->super_ = *parent;

	encoder->private_->file = 0;

	set_defaults_(encoder);

	encoder->protected_->state = OggFLAC__STREAM_ENCODER_UNINITIALIZED;

	return encoder;
}

OggFLAC_API void OggFLAC__stream_encoder_delete(OggFLAC__StreamEncoder *encoder)
{
	FLAC__ASSERT(0 != encoder);
	FLAC__ASSERT(0 != encoder->protected_);
	FLAC__ASSERT(0 != encoder->private_);

	(void)OggFLAC__stream_encoder_finish(encoder);

	free(encoder->private_);
	free(encoder->protected_);
	/* don't free(encoder) because FLAC__stream_encoder_delete() will do it */

	/* call superclass destructor last */
	FLAC__stream_encoder_delete((FLAC__StreamEncoder*)encoder);
}


/***********************************************************************
 *
 * Public class methods
 *
 ***********************************************************************/

OggFLAC_API FLAC__StreamEncoderInitStatus OggFLAC__stream_encoder_init_stream(OggFLAC__StreamEncoder *encoder, OggFLAC__StreamEncoderReadCallback read_callback, FLAC__StreamEncoderWriteCallback write_callback, FLAC__StreamEncoderSeekCallback seek_callback, FLAC__StreamEncoderTellCallback tell_callback, FLAC__StreamEncoderMetadataCallback metadata_callback, void *client_data)
{
	FLAC__ASSERT(0 != encoder);

	if(encoder->protected_->state != OggFLAC__STREAM_ENCODER_UNINITIALIZED)
		return FLAC__STREAM_ENCODER_INIT_STATUS_ALREADY_INITIALIZED;

	if(0 == write_callback || (seek_callback && (0 == read_callback || 0 == tell_callback)))
		return FLAC__STREAM_ENCODER_INIT_STATUS_INVALID_CALLBACKS;

	/* check seek table before FLAC__stream_encoder_init_stream() does, just to avoid messing up the encoder state for a trivial error */
	if(0 != encoder->private_->seek_table && !FLAC__format_seektable_is_legal(encoder->private_->seek_table))
		return FLAC__STREAM_ENCODER_INIT_STATUS_INVALID_METADATA;

	/* set state to OK; from here on, errors are fatal and we'll override the state then */
	encoder->protected_->state = OggFLAC__STREAM_ENCODER_OK;

	if(!OggFLAC__ogg_encoder_aspect_init(&encoder->protected_->ogg_encoder_aspect)) {
		encoder->protected_->state = OggFLAC__STREAM_ENCODER_OGG_ERROR;
		return FLAC__STREAM_ENCODER_INIT_STATUS_ENCODER_ERROR;
	}

	encoder->private_->read_callback = read_callback;
	encoder->private_->write_callback = write_callback;
	encoder->private_->seek_callback = seek_callback;
	encoder->private_->tell_callback = tell_callback;
	encoder->private_->metadata_callback = metadata_callback;
	encoder->private_->client_data = client_data;

	/*
	 * These must be done before we init the stream encoder because that
	 * calls the write_callback, which uses these values.
	 */
	encoder->private_->first_seekpoint_to_check = 0;
	encoder->private_->samples_written = 0;
	encoder->protected_->streaminfo_offset = 0;
	encoder->protected_->seektable_offset = 0;
	encoder->protected_->audio_offset = 0;

	/* we do our own special metadata updating inside Ogg here, so we don't pass in our seek/tell callbacks */
	if(FLAC__stream_encoder_init_stream((FLAC__StreamEncoder*)encoder, write_callback_, /*seek_callback=*/0, /*tell_callback=*/0, metadata_callback_, /*client_data=*/encoder) != FLAC__STREAM_ENCODER_INIT_STATUS_OK) {
		encoder->protected_->state = OggFLAC__STREAM_ENCODER_FLAC_STREAM_ENCODER_ERROR;
		return FLAC__STREAM_ENCODER_INIT_STATUS_ENCODER_ERROR;
	}

	/*
	 * Initializing the stream encoder writes all the metadata, so we
	 * save the stream offset now.
	 */
	if(encoder->private_->tell_callback && encoder->private_->tell_callback((FLAC__StreamEncoder*)encoder, &encoder->protected_->audio_offset, encoder->private_->client_data) == FLAC__STREAM_ENCODER_TELL_STATUS_ERROR) { /* FLAC__STREAM_ENCODER_TELL_STATUS_UNSUPPORTED just means we didn't get the offset; no error */
		encoder->protected_->state = OggFLAC__STREAM_ENCODER_CLIENT_ERROR;
		return FLAC__STREAM_ENCODER_INIT_STATUS_ENCODER_ERROR;
	}

	return FLAC__STREAM_ENCODER_INIT_STATUS_OK;
}

OggFLAC_API FLAC__StreamEncoderInitStatus OggFLAC__stream_encoder_init_FILE(OggFLAC__StreamEncoder *encoder, FILE *file, FLAC__StreamEncoderProgressCallback progress_callback, void *client_data)
{
	FLAC__StreamEncoderInitStatus init_status;

	FLAC__ASSERT(0 != encoder);
	FLAC__ASSERT(0 != file);

	if(encoder->protected_->state != OggFLAC__STREAM_ENCODER_UNINITIALIZED)
		return FLAC__STREAM_ENCODER_INIT_STATUS_ALREADY_INITIALIZED;

	/*
	 * To make sure that our file does not go unclosed after an error, we
	 * must assign the FILE pointer before any further error can occur in
	 * this routine.
	 */
	if(file == stdout)
		file = get_binary_stdout_(); /* just to be safe */

	encoder->private_->file = file;

	encoder->private_->progress_callback = progress_callback;
	encoder->private_->bytes_written = 0;
	encoder->private_->samples_written = 0;
	encoder->private_->frames_written = 0;

	init_status = OggFLAC__stream_encoder_init_stream(encoder, file_read_callback_, file_write_callback_, file_seek_callback_, file_tell_callback_, /*metadata_callback=*/0, client_data);
	if(init_status != FLAC__STREAM_ENCODER_INIT_STATUS_OK) {
		/* the above function sets the state for us in case of an error */
		return init_status;
	}

	{
		unsigned blocksize = OggFLAC__stream_encoder_get_blocksize(encoder);

		FLAC__ASSERT(blocksize != 0);
		encoder->private_->total_frames_estimate = (unsigned)((OggFLAC__stream_encoder_get_total_samples_estimate(encoder) + blocksize - 1) / blocksize);
	}

	return init_status;
}

OggFLAC_API FLAC__StreamEncoderInitStatus OggFLAC__stream_encoder_init_file(OggFLAC__StreamEncoder *encoder, const char *filename, FLAC__StreamEncoderProgressCallback progress_callback, void *client_data)
{
	FILE *file;

	FLAC__ASSERT(0 != encoder);

	/*
	 * To make sure that our file does not go unclosed after an error, we
	 * have to do the same entrance checks here that are later performed
	 * in FLAC__stream_encoder_init_FILE() before the FILE* is assigned.
	 */
	if(encoder->protected_->state != OggFLAC__STREAM_ENCODER_UNINITIALIZED)
		return FLAC__STREAM_ENCODER_INIT_STATUS_ALREADY_INITIALIZED;

	file = filename? fopen(filename, "w+b") : stdout;

	if(file == 0) {
		encoder->protected_->state = OggFLAC__STREAM_ENCODER_IO_ERROR;
		return FLAC__STREAM_ENCODER_INIT_STATUS_ENCODER_ERROR;
	}

	return OggFLAC__stream_encoder_init_FILE(encoder, file, progress_callback, client_data);
}

OggFLAC_API void OggFLAC__stream_encoder_finish(OggFLAC__StreamEncoder *encoder)
{
	FLAC__ASSERT(0 != encoder);
	FLAC__ASSERT(0 != encoder->private_);
	FLAC__ASSERT(0 != encoder->protected_);

	if(encoder->protected_->state == OggFLAC__STREAM_ENCODER_UNINITIALIZED)
		return;

	//@@@@@@superclass;FLAC__ASSERT(0 != encoder->private_->FLAC_stream_encoder);

	FLAC__stream_encoder_finish((FLAC__StreamEncoder*)encoder);

	OggFLAC__ogg_encoder_aspect_finish(&encoder->protected_->ogg_encoder_aspect);

	set_defaults_(encoder);

	encoder->protected_->state = OggFLAC__STREAM_ENCODER_UNINITIALIZED;
}

OggFLAC_API FLAC__bool OggFLAC__stream_encoder_set_serial_number(OggFLAC__StreamEncoder *encoder, long value)
{
	FLAC__ASSERT(0 != encoder);
	FLAC__ASSERT(0 != encoder->private_);
	FLAC__ASSERT(0 != encoder->protected_);
	//@@@@@@superclass;FLAC__ASSERT(0 != encoder->private_->FLAC_stream_encoder);
	if(encoder->protected_->state != OggFLAC__STREAM_ENCODER_UNINITIALIZED)
		return false;
	OggFLAC__ogg_encoder_aspect_set_serial_number(&encoder->protected_->ogg_encoder_aspect, value);
	return true;
}

OggFLAC_API FLAC__bool OggFLAC__stream_encoder_set_verify(OggFLAC__StreamEncoder *encoder, FLAC__bool value)
{
	FLAC__ASSERT(0 != encoder);
	FLAC__ASSERT(0 != encoder->private_);
	FLAC__ASSERT(0 != encoder->protected_);
	//@@@@@@superclass;FLAC__ASSERT(0 != encoder->private_->FLAC_stream_encoder);
	if(encoder->protected_->state != OggFLAC__STREAM_ENCODER_UNINITIALIZED)
		return false;
	return FLAC__stream_encoder_set_verify((FLAC__StreamEncoder*)encoder, value);
}

OggFLAC_API FLAC__bool OggFLAC__stream_encoder_set_streamable_subset(OggFLAC__StreamEncoder *encoder, FLAC__bool value)
{
	FLAC__ASSERT(0 != encoder);
	FLAC__ASSERT(0 != encoder->private_);
	FLAC__ASSERT(0 != encoder->protected_);
	//@@@@@@superclass;FLAC__ASSERT(0 != encoder->private_->FLAC_stream_encoder);
	if(encoder->protected_->state != OggFLAC__STREAM_ENCODER_UNINITIALIZED)
		return false;
	return FLAC__stream_encoder_set_streamable_subset((FLAC__StreamEncoder*)encoder, value);
}

OggFLAC_API FLAC__bool OggFLAC__stream_encoder_set_do_mid_side_stereo(OggFLAC__StreamEncoder *encoder, FLAC__bool value)
{
	FLAC__ASSERT(0 != encoder);
	FLAC__ASSERT(0 != encoder->private_);
	FLAC__ASSERT(0 != encoder->protected_);
	//@@@@@@superclass;FLAC__ASSERT(0 != encoder->private_->FLAC_stream_encoder);
	if(encoder->protected_->state != OggFLAC__STREAM_ENCODER_UNINITIALIZED)
		return false;
	return FLAC__stream_encoder_set_do_mid_side_stereo((FLAC__StreamEncoder*)encoder, value);
}

OggFLAC_API FLAC__bool OggFLAC__stream_encoder_set_loose_mid_side_stereo(OggFLAC__StreamEncoder *encoder, FLAC__bool value)
{
	FLAC__ASSERT(0 != encoder);
	FLAC__ASSERT(0 != encoder->private_);
	FLAC__ASSERT(0 != encoder->protected_);
	//@@@@@@superclass;FLAC__ASSERT(0 != encoder->private_->FLAC_stream_encoder);
	if(encoder->protected_->state != OggFLAC__STREAM_ENCODER_UNINITIALIZED)
		return false;
	return FLAC__stream_encoder_set_loose_mid_side_stereo((FLAC__StreamEncoder*)encoder, value);
}

OggFLAC_API FLAC__bool OggFLAC__stream_encoder_set_channels(OggFLAC__StreamEncoder *encoder, unsigned value)
{
	FLAC__ASSERT(0 != encoder);
	FLAC__ASSERT(0 != encoder->private_);
	FLAC__ASSERT(0 != encoder->protected_);
	//@@@@@@superclass;FLAC__ASSERT(0 != encoder->private_->FLAC_stream_encoder);
	if(encoder->protected_->state != OggFLAC__STREAM_ENCODER_UNINITIALIZED)
		return false;
	return FLAC__stream_encoder_set_channels((FLAC__StreamEncoder*)encoder, value);
}

OggFLAC_API FLAC__bool OggFLAC__stream_encoder_set_bits_per_sample(OggFLAC__StreamEncoder *encoder, unsigned value)
{
	FLAC__ASSERT(0 != encoder);
	FLAC__ASSERT(0 != encoder->private_);
	FLAC__ASSERT(0 != encoder->protected_);
	//@@@@@@superclass;FLAC__ASSERT(0 != encoder->private_->FLAC_stream_encoder);
	if(encoder->protected_->state != OggFLAC__STREAM_ENCODER_UNINITIALIZED)
		return false;
	return FLAC__stream_encoder_set_bits_per_sample((FLAC__StreamEncoder*)encoder, value);
}

OggFLAC_API FLAC__bool OggFLAC__stream_encoder_set_sample_rate(OggFLAC__StreamEncoder *encoder, unsigned value)
{
	FLAC__ASSERT(0 != encoder);
	FLAC__ASSERT(0 != encoder->private_);
	FLAC__ASSERT(0 != encoder->protected_);
	//@@@@@@superclass;FLAC__ASSERT(0 != encoder->private_->FLAC_stream_encoder);
	if(encoder->protected_->state != OggFLAC__STREAM_ENCODER_UNINITIALIZED)
		return false;
	return FLAC__stream_encoder_set_sample_rate((FLAC__StreamEncoder*)encoder, value);
}

OggFLAC_API FLAC__bool OggFLAC__stream_encoder_set_blocksize(OggFLAC__StreamEncoder *encoder, unsigned value)
{
	FLAC__ASSERT(0 != encoder);
	FLAC__ASSERT(0 != encoder->private_);
	FLAC__ASSERT(0 != encoder->protected_);
	//@@@@@@superclass;FLAC__ASSERT(0 != encoder->private_->FLAC_stream_encoder);
	if(encoder->protected_->state != OggFLAC__STREAM_ENCODER_UNINITIALIZED)
		return false;
	return FLAC__stream_encoder_set_blocksize((FLAC__StreamEncoder*)encoder, value);
}

OggFLAC_API FLAC__bool OggFLAC__stream_encoder_set_apodization(OggFLAC__StreamEncoder *encoder, const char *specification)
{
	FLAC__ASSERT(0 != encoder);
	FLAC__ASSERT(0 != encoder->private_);
	FLAC__ASSERT(0 != encoder->protected_);
	//@@@@@@superclass;FLAC__ASSERT(0 != encoder->private_->FLAC_stream_encoder);
	if(encoder->protected_->state != OggFLAC__STREAM_ENCODER_UNINITIALIZED)
		return false;
	return FLAC__stream_encoder_set_apodization((FLAC__StreamEncoder*)encoder, specification);
}

OggFLAC_API FLAC__bool OggFLAC__stream_encoder_set_max_lpc_order(OggFLAC__StreamEncoder *encoder, unsigned value)
{
	FLAC__ASSERT(0 != encoder);
	FLAC__ASSERT(0 != encoder->private_);
	FLAC__ASSERT(0 != encoder->protected_);
	//@@@@@@superclass;FLAC__ASSERT(0 != encoder->private_->FLAC_stream_encoder);
	if(encoder->protected_->state != OggFLAC__STREAM_ENCODER_UNINITIALIZED)
		return false;
	return FLAC__stream_encoder_set_max_lpc_order((FLAC__StreamEncoder*)encoder, value);
}

OggFLAC_API FLAC__bool OggFLAC__stream_encoder_set_qlp_coeff_precision(OggFLAC__StreamEncoder *encoder, unsigned value)
{
	FLAC__ASSERT(0 != encoder);
	FLAC__ASSERT(0 != encoder->private_);
	FLAC__ASSERT(0 != encoder->protected_);
	//@@@@@@superclass;FLAC__ASSERT(0 != encoder->private_->FLAC_stream_encoder);
	if(encoder->protected_->state != OggFLAC__STREAM_ENCODER_UNINITIALIZED)
		return false;
	return FLAC__stream_encoder_set_qlp_coeff_precision((FLAC__StreamEncoder*)encoder, value);
}

OggFLAC_API FLAC__bool OggFLAC__stream_encoder_set_do_qlp_coeff_prec_search(OggFLAC__StreamEncoder *encoder, FLAC__bool value)
{
	FLAC__ASSERT(0 != encoder);
	FLAC__ASSERT(0 != encoder->private_);
	FLAC__ASSERT(0 != encoder->protected_);
	//@@@@@@superclass;FLAC__ASSERT(0 != encoder->private_->FLAC_stream_encoder);
	if(encoder->protected_->state != OggFLAC__STREAM_ENCODER_UNINITIALIZED)
		return false;
	return FLAC__stream_encoder_set_do_qlp_coeff_prec_search((FLAC__StreamEncoder*)encoder, value);
}

OggFLAC_API FLAC__bool OggFLAC__stream_encoder_set_do_escape_coding(OggFLAC__StreamEncoder *encoder, FLAC__bool value)
{
	FLAC__ASSERT(0 != encoder);
	FLAC__ASSERT(0 != encoder->private_);
	FLAC__ASSERT(0 != encoder->protected_);
	//@@@@@@superclass;FLAC__ASSERT(0 != encoder->private_->FLAC_stream_encoder);
	if(encoder->protected_->state != OggFLAC__STREAM_ENCODER_UNINITIALIZED)
		return false;
	return FLAC__stream_encoder_set_do_escape_coding((FLAC__StreamEncoder*)encoder, value);
}

OggFLAC_API FLAC__bool OggFLAC__stream_encoder_set_do_exhaustive_model_search(OggFLAC__StreamEncoder *encoder, FLAC__bool value)
{
	FLAC__ASSERT(0 != encoder);
	FLAC__ASSERT(0 != encoder->private_);
	FLAC__ASSERT(0 != encoder->protected_);
	//@@@@@@superclass;FLAC__ASSERT(0 != encoder->private_->FLAC_stream_encoder);
	if(encoder->protected_->state != OggFLAC__STREAM_ENCODER_UNINITIALIZED)
		return false;
	return FLAC__stream_encoder_set_do_exhaustive_model_search((FLAC__StreamEncoder*)encoder, value);
}

OggFLAC_API FLAC__bool OggFLAC__stream_encoder_set_min_residual_partition_order(OggFLAC__StreamEncoder *encoder, unsigned value)
{
	FLAC__ASSERT(0 != encoder);
	FLAC__ASSERT(0 != encoder->private_);
	FLAC__ASSERT(0 != encoder->protected_);
	//@@@@@@superclass;FLAC__ASSERT(0 != encoder->private_->FLAC_stream_encoder);
	if(encoder->protected_->state != OggFLAC__STREAM_ENCODER_UNINITIALIZED)
		return false;
	return FLAC__stream_encoder_set_min_residual_partition_order((FLAC__StreamEncoder*)encoder, value);
}

OggFLAC_API FLAC__bool OggFLAC__stream_encoder_set_max_residual_partition_order(OggFLAC__StreamEncoder *encoder, unsigned value)
{
	FLAC__ASSERT(0 != encoder);
	FLAC__ASSERT(0 != encoder->private_);
	FLAC__ASSERT(0 != encoder->protected_);
	//@@@@@@superclass;FLAC__ASSERT(0 != encoder->private_->FLAC_stream_encoder);
	if(encoder->protected_->state != OggFLAC__STREAM_ENCODER_UNINITIALIZED)
		return false;
	return FLAC__stream_encoder_set_max_residual_partition_order((FLAC__StreamEncoder*)encoder, value);
}

OggFLAC_API FLAC__bool OggFLAC__stream_encoder_set_rice_parameter_search_dist(OggFLAC__StreamEncoder *encoder, unsigned value)
{
	FLAC__ASSERT(0 != encoder);
	FLAC__ASSERT(0 != encoder->private_);
	FLAC__ASSERT(0 != encoder->protected_);
	//@@@@@@superclass;FLAC__ASSERT(0 != encoder->private_->FLAC_stream_encoder);
	if(encoder->protected_->state != OggFLAC__STREAM_ENCODER_UNINITIALIZED)
		return false;
	return FLAC__stream_encoder_set_rice_parameter_search_dist((FLAC__StreamEncoder*)encoder, value);
}

OggFLAC_API FLAC__bool OggFLAC__stream_encoder_set_total_samples_estimate(OggFLAC__StreamEncoder *encoder, FLAC__uint64 value)
{
	FLAC__ASSERT(0 != encoder);
	FLAC__ASSERT(0 != encoder->private_);
	FLAC__ASSERT(0 != encoder->protected_);
	//@@@@@@superclass;FLAC__ASSERT(0 != encoder->private_->FLAC_stream_encoder);
	if(encoder->protected_->state != OggFLAC__STREAM_ENCODER_UNINITIALIZED)
		return false;
	return FLAC__stream_encoder_set_total_samples_estimate((FLAC__StreamEncoder*)encoder, value);
}

OggFLAC_API FLAC__bool OggFLAC__stream_encoder_set_metadata(OggFLAC__StreamEncoder *encoder, FLAC__StreamMetadata **metadata, unsigned num_blocks)
{
	FLAC__ASSERT(0 != encoder);
	FLAC__ASSERT(0 != encoder->private_);
	FLAC__ASSERT(0 != encoder->protected_);
	//@@@@@@superclass;FLAC__ASSERT(0 != encoder->private_->FLAC_stream_encoder);
	if(encoder->protected_->state != OggFLAC__STREAM_ENCODER_UNINITIALIZED)
		return false;
	/* reorder metadata if necessary to ensure that any VORBIS_COMMENT is the first, according to the mapping spec */
	if(0 != metadata && num_blocks > 1) {
		unsigned i;
		for(i = 1; i < num_blocks; i++) {
			if(0 != metadata[i] && metadata[i]->type == FLAC__METADATA_TYPE_VORBIS_COMMENT) {
				FLAC__StreamMetadata *vc = metadata[i];
				for( ; i > 0; i--)
					metadata[i] = metadata[i-1];
				metadata[0] = vc;
				break;
			}
		}
	}
	if(0 != metadata && num_blocks > 0) {
		unsigned i;
		for(i = 0; i < num_blocks; i++) {
			/* keep track of any SEEKTABLE block */
			if(0 != metadata[i] && metadata[i]->type == FLAC__METADATA_TYPE_SEEKTABLE) {
				encoder->private_->seek_table = &metadata[i]->data.seek_table;
				break; /* take only the first one */
			}
		}
	}
	if(!OggFLAC__ogg_encoder_aspect_set_num_metadata(&encoder->protected_->ogg_encoder_aspect, num_blocks))
		return false;
	return FLAC__stream_encoder_set_metadata((FLAC__StreamEncoder*)encoder, metadata, num_blocks);
}

/*
 * These three functions are not static, but not publically exposed in
 * include/OggFLAC/ either.  They are used by the test suite.
 */
OggFLAC_API FLAC__bool OggFLAC__stream_encoder_disable_constant_subframes(OggFLAC__StreamEncoder *encoder, FLAC__bool value)
{
	FLAC__ASSERT(0 != encoder);
	FLAC__ASSERT(0 != encoder->private_);
	FLAC__ASSERT(0 != encoder->protected_);
	if(encoder->protected_->state != OggFLAC__STREAM_ENCODER_UNINITIALIZED)
		return false;
	return FLAC__stream_encoder_disable_constant_subframes((FLAC__StreamEncoder*)encoder, value);
}

OggFLAC_API FLAC__bool OggFLAC__stream_encoder_disable_fixed_subframes(OggFLAC__StreamEncoder *encoder, FLAC__bool value)
{
	FLAC__ASSERT(0 != encoder);
	FLAC__ASSERT(0 != encoder->private_);
	FLAC__ASSERT(0 != encoder->protected_);
	if(encoder->protected_->state != OggFLAC__STREAM_ENCODER_UNINITIALIZED)
		return false;
	return FLAC__stream_encoder_disable_fixed_subframes((FLAC__StreamEncoder*)encoder, value);
}

OggFLAC_API FLAC__bool OggFLAC__stream_encoder_disable_verbatim_subframes(OggFLAC__StreamEncoder *encoder, FLAC__bool value)
{
	FLAC__ASSERT(0 != encoder);
	FLAC__ASSERT(0 != encoder->private_);
	FLAC__ASSERT(0 != encoder->protected_);
	if(encoder->protected_->state != OggFLAC__STREAM_ENCODER_UNINITIALIZED)
		return false;
	return FLAC__stream_encoder_disable_verbatim_subframes((FLAC__StreamEncoder*)encoder, value);
}

OggFLAC_API OggFLAC__StreamEncoderState OggFLAC__stream_encoder_get_state(const OggFLAC__StreamEncoder *encoder)
{
	FLAC__ASSERT(0 != encoder);
	FLAC__ASSERT(0 != encoder->private_);
	FLAC__ASSERT(0 != encoder->protected_);
	return encoder->protected_->state;
}

OggFLAC_API FLAC__StreamEncoderState OggFLAC__stream_encoder_get_FLAC_stream_encoder_state(const OggFLAC__StreamEncoder *encoder)
{
	FLAC__ASSERT(0 != encoder);
	FLAC__ASSERT(0 != encoder->private_);
	FLAC__ASSERT(0 != encoder->protected_);
	return FLAC__stream_encoder_get_state((FLAC__StreamEncoder*)encoder);
}

OggFLAC_API FLAC__StreamDecoderState OggFLAC__stream_encoder_get_verify_decoder_state(const OggFLAC__StreamEncoder *encoder)
{
	FLAC__ASSERT(0 != encoder);
	FLAC__ASSERT(0 != encoder->private_);
	FLAC__ASSERT(0 != encoder->protected_);
	return FLAC__stream_encoder_get_verify_decoder_state((FLAC__StreamEncoder*)encoder);
}

OggFLAC_API const char *OggFLAC__stream_encoder_get_resolved_state_string(const OggFLAC__StreamEncoder *encoder)
{
	if(encoder->protected_->state != OggFLAC__STREAM_ENCODER_FLAC_STREAM_ENCODER_ERROR)
		return OggFLAC__StreamEncoderStateString[encoder->protected_->state];
	else
		return FLAC__stream_encoder_get_resolved_state_string((FLAC__StreamEncoder*)encoder);
}

OggFLAC_API void OggFLAC__stream_encoder_get_verify_decoder_error_stats(const OggFLAC__StreamEncoder *encoder, FLAC__uint64 *absolute_sample, unsigned *frame_number, unsigned *channel, unsigned *sample, FLAC__int32 *expected, FLAC__int32 *got)
{
	FLAC__ASSERT(0 != encoder);
	FLAC__ASSERT(0 != encoder->private_);
	FLAC__stream_encoder_get_verify_decoder_error_stats((FLAC__StreamEncoder*)encoder, absolute_sample, frame_number, channel, sample, expected, got);
}

OggFLAC_API FLAC__bool OggFLAC__stream_encoder_get_verify(const OggFLAC__StreamEncoder *encoder)
{
	FLAC__ASSERT(0 != encoder);
	FLAC__ASSERT(0 != encoder->private_);
	FLAC__ASSERT(0 != encoder->protected_);
	return FLAC__stream_encoder_get_verify((FLAC__StreamEncoder*)encoder);
}

OggFLAC_API FLAC__bool OggFLAC__stream_encoder_get_streamable_subset(const OggFLAC__StreamEncoder *encoder)
{
	FLAC__ASSERT(0 != encoder);
	FLAC__ASSERT(0 != encoder->private_);
	FLAC__ASSERT(0 != encoder->protected_);
	return FLAC__stream_encoder_get_streamable_subset((FLAC__StreamEncoder*)encoder);
}

OggFLAC_API FLAC__bool OggFLAC__stream_encoder_get_do_mid_side_stereo(const OggFLAC__StreamEncoder *encoder)
{
	FLAC__ASSERT(0 != encoder);
	FLAC__ASSERT(0 != encoder->private_);
	FLAC__ASSERT(0 != encoder->protected_);
	return FLAC__stream_encoder_get_do_mid_side_stereo((FLAC__StreamEncoder*)encoder);
}

OggFLAC_API FLAC__bool OggFLAC__stream_encoder_get_loose_mid_side_stereo(const OggFLAC__StreamEncoder *encoder)
{
	FLAC__ASSERT(0 != encoder);
	FLAC__ASSERT(0 != encoder->private_);
	FLAC__ASSERT(0 != encoder->protected_);
	return FLAC__stream_encoder_get_loose_mid_side_stereo((FLAC__StreamEncoder*)encoder);
}

OggFLAC_API unsigned OggFLAC__stream_encoder_get_channels(const OggFLAC__StreamEncoder *encoder)
{
	FLAC__ASSERT(0 != encoder);
	FLAC__ASSERT(0 != encoder->private_);
	FLAC__ASSERT(0 != encoder->protected_);
	return FLAC__stream_encoder_get_channels((FLAC__StreamEncoder*)encoder);
}

OggFLAC_API unsigned OggFLAC__stream_encoder_get_bits_per_sample(const OggFLAC__StreamEncoder *encoder)
{
	FLAC__ASSERT(0 != encoder);
	FLAC__ASSERT(0 != encoder->private_);
	FLAC__ASSERT(0 != encoder->protected_);
	return FLAC__stream_encoder_get_bits_per_sample((FLAC__StreamEncoder*)encoder);
}

OggFLAC_API unsigned OggFLAC__stream_encoder_get_sample_rate(const OggFLAC__StreamEncoder *encoder)
{
	FLAC__ASSERT(0 != encoder);
	FLAC__ASSERT(0 != encoder->private_);
	FLAC__ASSERT(0 != encoder->protected_);
	return FLAC__stream_encoder_get_sample_rate((FLAC__StreamEncoder*)encoder);
}

OggFLAC_API unsigned OggFLAC__stream_encoder_get_blocksize(const OggFLAC__StreamEncoder *encoder)
{
	FLAC__ASSERT(0 != encoder);
	FLAC__ASSERT(0 != encoder->private_);
	FLAC__ASSERT(0 != encoder->protected_);
	return FLAC__stream_encoder_get_blocksize((FLAC__StreamEncoder*)encoder);
}

OggFLAC_API unsigned OggFLAC__stream_encoder_get_max_lpc_order(const OggFLAC__StreamEncoder *encoder)
{
	FLAC__ASSERT(0 != encoder);
	FLAC__ASSERT(0 != encoder->private_);
	FLAC__ASSERT(0 != encoder->protected_);
	return FLAC__stream_encoder_get_max_lpc_order((FLAC__StreamEncoder*)encoder);
}

OggFLAC_API unsigned OggFLAC__stream_encoder_get_qlp_coeff_precision(const OggFLAC__StreamEncoder *encoder)
{
	FLAC__ASSERT(0 != encoder);
	FLAC__ASSERT(0 != encoder->private_);
	FLAC__ASSERT(0 != encoder->protected_);
	return FLAC__stream_encoder_get_qlp_coeff_precision((FLAC__StreamEncoder*)encoder);
}

OggFLAC_API FLAC__bool OggFLAC__stream_encoder_get_do_qlp_coeff_prec_search(const OggFLAC__StreamEncoder *encoder)
{
	FLAC__ASSERT(0 != encoder);
	FLAC__ASSERT(0 != encoder->private_);
	FLAC__ASSERT(0 != encoder->protected_);
	return FLAC__stream_encoder_get_do_qlp_coeff_prec_search((FLAC__StreamEncoder*)encoder);
}

OggFLAC_API FLAC__bool OggFLAC__stream_encoder_get_do_escape_coding(const OggFLAC__StreamEncoder *encoder)
{
	FLAC__ASSERT(0 != encoder);
	FLAC__ASSERT(0 != encoder->private_);
	FLAC__ASSERT(0 != encoder->protected_);
	return FLAC__stream_encoder_get_do_escape_coding((FLAC__StreamEncoder*)encoder);
}

OggFLAC_API FLAC__bool OggFLAC__stream_encoder_get_do_exhaustive_model_search(const OggFLAC__StreamEncoder *encoder)
{
	FLAC__ASSERT(0 != encoder);
	FLAC__ASSERT(0 != encoder->private_);
	FLAC__ASSERT(0 != encoder->protected_);
	return FLAC__stream_encoder_get_do_exhaustive_model_search((FLAC__StreamEncoder*)encoder);
}

OggFLAC_API unsigned OggFLAC__stream_encoder_get_min_residual_partition_order(const OggFLAC__StreamEncoder *encoder)
{
	FLAC__ASSERT(0 != encoder);
	FLAC__ASSERT(0 != encoder->private_);
	FLAC__ASSERT(0 != encoder->protected_);
	return FLAC__stream_encoder_get_min_residual_partition_order((FLAC__StreamEncoder*)encoder);
}

OggFLAC_API unsigned OggFLAC__stream_encoder_get_max_residual_partition_order(const OggFLAC__StreamEncoder *encoder)
{
	FLAC__ASSERT(0 != encoder);
	FLAC__ASSERT(0 != encoder->private_);
	FLAC__ASSERT(0 != encoder->protected_);
	return FLAC__stream_encoder_get_max_residual_partition_order((FLAC__StreamEncoder*)encoder);
}

OggFLAC_API unsigned OggFLAC__stream_encoder_get_rice_parameter_search_dist(const OggFLAC__StreamEncoder *encoder)
{
	FLAC__ASSERT(0 != encoder);
	FLAC__ASSERT(0 != encoder->private_);
	FLAC__ASSERT(0 != encoder->protected_);
	return FLAC__stream_encoder_get_rice_parameter_search_dist((FLAC__StreamEncoder*)encoder);
}

OggFLAC_API FLAC__uint64 OggFLAC__stream_encoder_get_total_samples_estimate(const OggFLAC__StreamEncoder *encoder)
{
	FLAC__ASSERT(0 != encoder);
	FLAC__ASSERT(0 != encoder->private_);
	FLAC__ASSERT(0 != encoder->protected_);
	return FLAC__stream_encoder_get_total_samples_estimate((FLAC__StreamEncoder*)encoder);
}

OggFLAC_API FLAC__bool OggFLAC__stream_encoder_process(OggFLAC__StreamEncoder *encoder, const FLAC__int32 * const buffer[], unsigned samples)
{
	FLAC__ASSERT(0 != encoder);
	FLAC__ASSERT(0 != encoder->private_);
	FLAC__ASSERT(0 != encoder->protected_);
	if(!FLAC__stream_encoder_process((FLAC__StreamEncoder*)encoder, buffer, samples)) {
		encoder->protected_->state = OggFLAC__STREAM_ENCODER_FLAC_STREAM_ENCODER_ERROR;
		return false;
	}
	else
		return true;
}

OggFLAC_API FLAC__bool OggFLAC__stream_encoder_process_interleaved(OggFLAC__StreamEncoder *encoder, const FLAC__int32 buffer[], unsigned samples)
{
	FLAC__ASSERT(0 != encoder);
	FLAC__ASSERT(0 != encoder->private_);
	FLAC__ASSERT(0 != encoder->protected_);
	if(!FLAC__stream_encoder_process_interleaved((FLAC__StreamEncoder*)encoder, buffer, samples)) {
		encoder->protected_->state = OggFLAC__STREAM_ENCODER_FLAC_STREAM_ENCODER_ERROR;
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

void set_defaults_(OggFLAC__StreamEncoder *encoder)
{
	FLAC__ASSERT(0 != encoder);
	FLAC__ASSERT(0 != encoder->private_);
	FLAC__ASSERT(0 != encoder->protected_);

	encoder->private_->read_callback = 0;
	encoder->private_->write_callback = 0;
	encoder->private_->seek_callback = 0;
	encoder->private_->tell_callback = 0;
	encoder->private_->metadata_callback = 0;
	encoder->private_->progress_callback = 0;
	encoder->private_->client_data = 0;

	encoder->private_->seek_table = 0;

	OggFLAC__ogg_encoder_aspect_set_defaults(&encoder->protected_->ogg_encoder_aspect);
}

FLAC__StreamEncoderWriteStatus write_callback_(const FLAC__StreamEncoder *super, const FLAC__byte buffer[], unsigned bytes, unsigned samples, unsigned current_frame, void *client_data)
{
	OggFLAC__StreamEncoder *encoder = (OggFLAC__StreamEncoder*)super;
	FLAC__StreamEncoderWriteStatus status;
	FLAC__uint64 output_position;

	(void)client_data; /* silence compiler warning about unused parameter */

	/* FLAC__STREAM_ENCODER_TELL_STATUS_UNSUPPORTED just means we didn't get the offset; no error */
	if(encoder->private_->tell_callback && encoder->private_->tell_callback((FLAC__StreamEncoder*)encoder, &output_position, encoder->private_->client_data) == FLAC__STREAM_ENCODER_TELL_STATUS_ERROR) {
		encoder->protected_->state = OggFLAC__STREAM_ENCODER_CLIENT_ERROR;
		return FLAC__STREAM_ENCODER_WRITE_STATUS_FATAL_ERROR;
	}

	/*
	 * Watch for the STREAMINFO block and first SEEKTABLE block to go by and store their offsets.
	 */
	if(samples == 0) {
		FLAC__MetadataType type = (buffer[0] & 0x7f);
		if(type == FLAC__METADATA_TYPE_STREAMINFO)
			encoder->protected_->streaminfo_offset = output_position;
		else if(type == FLAC__METADATA_TYPE_SEEKTABLE && encoder->protected_->seektable_offset == 0)
			encoder->protected_->seektable_offset = output_position;
	}

	/*
	 * Mark the current seek point if hit (if audio_offset == 0 that
	 * means we're still writing metadata and haven't hit the first
	 * frame yet)
	 */
	if(0 != encoder->private_->seek_table && encoder->protected_->audio_offset > 0 && encoder->private_->seek_table->num_points > 0) {
		const unsigned blocksize = FLAC__stream_encoder_get_blocksize((FLAC__StreamEncoder*)encoder);
		const FLAC__uint64 frame_first_sample = encoder->private_->samples_written;
		const FLAC__uint64 frame_last_sample = frame_first_sample + (FLAC__uint64)blocksize - 1;
		FLAC__uint64 test_sample;
		unsigned i;
		for(i = encoder->private_->first_seekpoint_to_check; i < encoder->private_->seek_table->num_points; i++) {
			test_sample = encoder->private_->seek_table->points[i].sample_number;
			if(test_sample > frame_last_sample) {
				break;
			}
			else if(test_sample >= frame_first_sample) {
				encoder->private_->seek_table->points[i].sample_number = frame_first_sample;
				encoder->private_->seek_table->points[i].stream_offset = output_position - encoder->protected_->audio_offset;
				encoder->private_->seek_table->points[i].frame_samples = blocksize;
				encoder->private_->first_seekpoint_to_check++;
				/* DO NOT: "break;" and here's why:
				 * The seektable template may contain more than one target
				 * sample for any given frame; we will keep looping, generating
				 * duplicate seekpoints for them, and we'll clean it up later,
				 * just before writing the seektable back to the metadata.
				 */
			}
			else {
				encoder->private_->first_seekpoint_to_check++;
			}
		}
	}

	status = OggFLAC__ogg_encoder_aspect_write_callback_wrapper(&encoder->protected_->ogg_encoder_aspect, FLAC__stream_encoder_get_total_samples_estimate((FLAC__StreamEncoder*)encoder), buffer, bytes, samples, current_frame, (OggFLAC__OggEncoderAspectWriteCallbackProxy)encoder->private_->write_callback, encoder, encoder->private_->client_data);

	if(status == FLAC__STREAM_ENCODER_WRITE_STATUS_OK)
		encoder->private_->samples_written += samples;
	else
		encoder->protected_->state = OggFLAC__STREAM_ENCODER_CLIENT_ERROR;

	return status;
}

void metadata_callback_(const FLAC__StreamEncoder *super, const FLAC__StreamMetadata *metadata, void *client_data)
{
	OggFLAC__StreamEncoder *encoder = (OggFLAC__StreamEncoder*)super;
	FLAC__byte b[max(6, FLAC__STREAM_METADATA_SEEKPOINT_LENGTH)];
	const FLAC__uint64 samples = metadata->data.stream_info.total_samples;
	const unsigned min_framesize = metadata->data.stream_info.min_framesize;
	const unsigned max_framesize = metadata->data.stream_info.max_framesize;
	ogg_page page;

	FLAC__ASSERT(metadata->type == FLAC__METADATA_TYPE_STREAMINFO);

	/* We get called by the stream encoder when the encoding process
	 * has finished so that we can update the STREAMINFO and SEEKTABLE
	 * blocks.
	 */

	(void)client_data; /* silence compiler warning about unused parameter */

	/* All this is based on intimate knowledge of the stream header
	 * layout, but a change to the header format that would break this
	 * would also break all streams encoded in the previous format.
	 */

	/**
	 ** Write STREAMINFO stats
	 **/
	simple_ogg_page__init(&page);
	if(!simple_ogg_page__get_at(encoder, encoder->protected_->streaminfo_offset, &page, encoder->private_->seek_callback, encoder->private_->read_callback, encoder->private_->client_data)) {
		simple_ogg_page__clear(&page);
		return; /* state already set */
	}

	/*
	 * Write MD5 signature
	 */
	{
		const unsigned md5_offset =
			FLAC__STREAM_METADATA_HEADER_LENGTH +
			(
				FLAC__STREAM_METADATA_STREAMINFO_MIN_BLOCK_SIZE_LEN +
				FLAC__STREAM_METADATA_STREAMINFO_MAX_BLOCK_SIZE_LEN +
				FLAC__STREAM_METADATA_STREAMINFO_MIN_FRAME_SIZE_LEN +
				FLAC__STREAM_METADATA_STREAMINFO_MAX_FRAME_SIZE_LEN +
				FLAC__STREAM_METADATA_STREAMINFO_SAMPLE_RATE_LEN +
				FLAC__STREAM_METADATA_STREAMINFO_CHANNELS_LEN +
				FLAC__STREAM_METADATA_STREAMINFO_BITS_PER_SAMPLE_LEN +
				FLAC__STREAM_METADATA_STREAMINFO_TOTAL_SAMPLES_LEN
			) / 8;

		if(md5_offset + 16 > (unsigned)page.body_len) {
			encoder->protected_->state = OggFLAC__STREAM_ENCODER_OGG_ERROR;
			simple_ogg_page__clear(&page);
			return;
		}
		memcpy(page.body + md5_offset, metadata->data.stream_info.md5sum, 16);
	}

	/*
	 * Write total samples
	 */
	{
		const unsigned total_samples_byte_offset =
			FLAC__STREAM_METADATA_HEADER_LENGTH +
			(
				FLAC__STREAM_METADATA_STREAMINFO_MIN_BLOCK_SIZE_LEN +
				FLAC__STREAM_METADATA_STREAMINFO_MAX_BLOCK_SIZE_LEN +
				FLAC__STREAM_METADATA_STREAMINFO_MIN_FRAME_SIZE_LEN +
				FLAC__STREAM_METADATA_STREAMINFO_MAX_FRAME_SIZE_LEN +
				FLAC__STREAM_METADATA_STREAMINFO_SAMPLE_RATE_LEN +
				FLAC__STREAM_METADATA_STREAMINFO_CHANNELS_LEN +
				FLAC__STREAM_METADATA_STREAMINFO_BITS_PER_SAMPLE_LEN
				- 4
			) / 8;

		if(total_samples_byte_offset + 5 > (unsigned)page.body_len) {
			encoder->protected_->state = OggFLAC__STREAM_ENCODER_OGG_ERROR;
			simple_ogg_page__clear(&page);
			return;
		}
		b[0] = (FLAC__byte)page.body[total_samples_byte_offset] & 0xF0;
		b[0] |= (FLAC__byte)((samples >> 32) & 0x0F);
		b[1] = (FLAC__byte)((samples >> 24) & 0xFF);
		b[2] = (FLAC__byte)((samples >> 16) & 0xFF);
		b[3] = (FLAC__byte)((samples >> 8) & 0xFF);
		b[4] = (FLAC__byte)(samples & 0xFF);
		memcpy(page.body + total_samples_byte_offset, b, 5);
	}

	/*
	 * Write min/max framesize
	 */
	{
		const unsigned min_framesize_offset =
			FLAC__STREAM_METADATA_HEADER_LENGTH +
			(
				FLAC__STREAM_METADATA_STREAMINFO_MIN_BLOCK_SIZE_LEN +
				FLAC__STREAM_METADATA_STREAMINFO_MAX_BLOCK_SIZE_LEN
			) / 8;

		if(min_framesize_offset + 6 > (unsigned)page.body_len) {
			encoder->protected_->state = OggFLAC__STREAM_ENCODER_OGG_ERROR;
			simple_ogg_page__clear(&page);
			return;
		}
		b[0] = (FLAC__byte)((min_framesize >> 16) & 0xFF);
		b[1] = (FLAC__byte)((min_framesize >> 8) & 0xFF);
		b[2] = (FLAC__byte)(min_framesize & 0xFF);
		b[3] = (FLAC__byte)((max_framesize >> 16) & 0xFF);
		b[4] = (FLAC__byte)((max_framesize >> 8) & 0xFF);
		b[5] = (FLAC__byte)(max_framesize & 0xFF);
		memcpy(page.body + min_framesize_offset, b, 6);
	}
	if(!simple_ogg_page__set_at(encoder, encoder->protected_->streaminfo_offset, &page, encoder->private_->seek_callback, encoder->private_->write_callback, encoder->private_->client_data)) {
		simple_ogg_page__clear(&page);
		return; /* state already set */
	}
	simple_ogg_page__clear(&page);

	/*
	 * Write seektable
	 */
	if(0 != encoder->private_->seek_table && encoder->private_->seek_table->num_points > 0 && encoder->protected_->seektable_offset > 0) {
		unsigned i;
		FLAC__byte *p;

		FLAC__format_seektable_sort(encoder->private_->seek_table);

		FLAC__ASSERT(FLAC__format_seektable_is_legal(encoder->private_->seek_table));

		simple_ogg_page__init(&page);
		if(!simple_ogg_page__get_at(encoder, encoder->protected_->seektable_offset, &page, encoder->private_->seek_callback, encoder->private_->read_callback, encoder->private_->client_data)) {
			simple_ogg_page__clear(&page);
			return; /* state already set */
		}

		if(FLAC__STREAM_METADATA_HEADER_LENGTH + (18*encoder->private_->seek_table->num_points) > (unsigned)page.body_len) {
			encoder->protected_->state = OggFLAC__STREAM_ENCODER_OGG_ERROR;
			simple_ogg_page__clear(&page);
			return;
		}

		for(i = 0, p = page.body + FLAC__STREAM_METADATA_HEADER_LENGTH; i < encoder->private_->seek_table->num_points; i++, p += 18) {
			FLAC__uint64 xx;
			unsigned x;
			xx = encoder->private_->seek_table->points[i].sample_number;
			b[7] = (FLAC__byte)xx; xx >>= 8;
			b[6] = (FLAC__byte)xx; xx >>= 8;
			b[5] = (FLAC__byte)xx; xx >>= 8;
			b[4] = (FLAC__byte)xx; xx >>= 8;
			b[3] = (FLAC__byte)xx; xx >>= 8;
			b[2] = (FLAC__byte)xx; xx >>= 8;
			b[1] = (FLAC__byte)xx; xx >>= 8;
			b[0] = (FLAC__byte)xx; xx >>= 8;
			xx = encoder->private_->seek_table->points[i].stream_offset;
			b[15] = (FLAC__byte)xx; xx >>= 8;
			b[14] = (FLAC__byte)xx; xx >>= 8;
			b[13] = (FLAC__byte)xx; xx >>= 8;
			b[12] = (FLAC__byte)xx; xx >>= 8;
			b[11] = (FLAC__byte)xx; xx >>= 8;
			b[10] = (FLAC__byte)xx; xx >>= 8;
			b[9] = (FLAC__byte)xx; xx >>= 8;
			b[8] = (FLAC__byte)xx; xx >>= 8;
			x = encoder->private_->seek_table->points[i].frame_samples;
			b[17] = (FLAC__byte)x; x >>= 8;
			b[16] = (FLAC__byte)x; x >>= 8;
			if(encoder->private_->write_callback((FLAC__StreamEncoder*)encoder, b, 18, 0, 0, encoder->private_->client_data) != FLAC__STREAM_ENCODER_WRITE_STATUS_OK) {
				encoder->protected_->state = OggFLAC__STREAM_ENCODER_CLIENT_ERROR;
				simple_ogg_page__clear(&page);
				return;
			}
			memcpy(p, b, 18);
		}

		if(!simple_ogg_page__set_at(encoder, encoder->protected_->seektable_offset, &page, encoder->private_->seek_callback, encoder->private_->write_callback, encoder->private_->client_data)) {
			simple_ogg_page__clear(&page);
			return; /* state already set */
		}
		simple_ogg_page__clear(&page);
	}

	if(encoder->private_->metadata_callback)
		encoder->private_->metadata_callback((FLAC__StreamEncoder*)encoder, metadata, client_data);
}

OggFLAC__StreamEncoderReadStatus file_read_callback_(const OggFLAC__StreamEncoder *encoder, FLAC__byte buffer[], unsigned *bytes, void *client_data)
{
	(void)client_data;

	*bytes = (unsigned)fread(buffer, 1, *bytes, encoder->private_->file);
	if (*bytes == 0) {
		if (feof(encoder->private_->file))
			return OggFLAC__STREAM_ENCODER_READ_STATUS_END_OF_STREAM;
		else if (ferror(encoder->private_->file))
			return OggFLAC__STREAM_ENCODER_READ_STATUS_ABORT;
	}
	return OggFLAC__STREAM_ENCODER_READ_STATUS_CONTINUE;
}

FLAC__StreamEncoderSeekStatus file_seek_callback_(const FLAC__StreamEncoder *super, FLAC__uint64 absolute_byte_offset, void *client_data)
{
	OggFLAC__StreamEncoder *encoder = (OggFLAC__StreamEncoder*)super;

	(void)client_data;

	if(fseeko(encoder->private_->file, (off_t)absolute_byte_offset, SEEK_SET) < 0)
		return FLAC__STREAM_ENCODER_SEEK_STATUS_ERROR;
	else
		return FLAC__STREAM_ENCODER_SEEK_STATUS_OK;
}

FLAC__StreamEncoderTellStatus file_tell_callback_(const FLAC__StreamEncoder *super, FLAC__uint64 *absolute_byte_offset, void *client_data)
{
	OggFLAC__StreamEncoder *encoder = (OggFLAC__StreamEncoder*)super;
	off_t offset;

	(void)client_data;

	offset = ftello(encoder->private_->file);

	if(offset < 0) {
		return FLAC__STREAM_ENCODER_TELL_STATUS_ERROR;
	}
	else {
		*absolute_byte_offset = (FLAC__uint64)offset;
		return FLAC__STREAM_ENCODER_TELL_STATUS_OK;
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

FLAC__StreamEncoderWriteStatus file_write_callback_(const FLAC__StreamEncoder *super, const FLAC__byte buffer[], unsigned bytes, unsigned samples, unsigned current_frame, void *client_data)
{
	OggFLAC__StreamEncoder *encoder = (OggFLAC__StreamEncoder*)super;

	(void)client_data;

	if(local__fwrite(buffer, sizeof(FLAC__byte), bytes, encoder->private_->file) == bytes) {
		encoder->private_->bytes_written += bytes;
		encoder->private_->samples_written += samples;
		/* we keep a high watermark on the number of frames written because
		 * when the encoder goes back to write metadata, 'current_frame'
		 * will drop back to 0.
		 */
		encoder->private_->frames_written = max(encoder->private_->frames_written, current_frame+1);
		/*@@@ We would like to add an '&& samples > 0' to the if
		 * clause here but currently because of the nature of our Ogg
		 * writing implementation, 'samples' is always 0 (see
		 * ogg_encoder_aspect.c).  The downside is extra progress
		 * callbacks.
		 */
		if(0 != encoder->private_->progress_callback /*@@@ && samples > 0 */)
			encoder->private_->progress_callback((FLAC__StreamEncoder*)encoder, encoder->private_->bytes_written, encoder->private_->samples_written, encoder->private_->frames_written, encoder->private_->total_frames_estimate, encoder->private_->client_data);
		return FLAC__STREAM_ENCODER_WRITE_STATUS_OK;
	}
	else
		return FLAC__STREAM_ENCODER_WRITE_STATUS_FATAL_ERROR;
}

/*
 * This will forcibly set stdout to binary mode (for OSes that require it)
 */
FILE *get_binary_stdout_()
{
	/* if something breaks here it is probably due to the presence or
	 * absence of an underscore before the identifiers 'setmode',
	 * 'fileno', and/or 'O_BINARY'; check your system header files.
	 */
#if defined _MSC_VER || defined __MINGW32__
	_setmode(_fileno(stdout), _O_BINARY);
#elif defined __CYGWIN__
	/* almost certainly not needed for any modern Cygwin, but let's be safe... */
	setmode(_fileno(stdout), _O_BINARY);
#elif defined __EMX__
	setmode(fileno(stdout), O_BINARY);
#endif

	return stdout;
}
