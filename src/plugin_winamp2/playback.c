/* in_flac - Winamp2 FLAC input plugin
 * Copyright (C) 2000,2001,2002,2003,2004,2005  Josh Coalson
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

#include <stdlib.h>
#include <string.h> /* for memmove() */
#include "playback.h"
#include "share/grabbag.h"


static FLAC__int32 reservoir_[FLAC_PLUGIN__MAX_SUPPORTED_CHANNELS][FLAC__MAX_BLOCK_SIZE * 2/*for overflow*/];
static FLAC__int32 *reservoir__[FLAC_PLUGIN__MAX_SUPPORTED_CHANNELS] = { reservoir_[0], reservoir_[1] }; /*@@@ kind of a hard-coded hack */
static unsigned wide_samples_in_reservoir_;
static output_config_t cfg;     /* local copy */

static unsigned bh_index_last_w, bh_index_last_o, written_time_last;
static FLAC__int64 decode_position, decode_position_last;

/*
 *  callbacks
 */

static FLAC__StreamDecoderWriteStatus write_callback(const FLAC__FileDecoder *decoder, const FLAC__Frame *frame, const FLAC__int32 * const buffer[], void *client_data)
{
	file_info_struct *file_info = (file_info_struct*)client_data;
	const unsigned channels = file_info->channels, wide_samples = frame->header.blocksize;
	unsigned channel;

	(void)decoder;

	if (file_info->abort_flag)
		return FLAC__STREAM_DECODER_WRITE_STATUS_ABORT;

	for (channel = 0; channel < channels; channel++)
		memcpy(&reservoir_[channel][wide_samples_in_reservoir_], buffer[channel], sizeof(buffer[0][0]) * wide_samples);

	wide_samples_in_reservoir_ += wide_samples;

	return FLAC__STREAM_DECODER_WRITE_STATUS_CONTINUE;
}

static void metadata_callback(const FLAC__FileDecoder *decoder, const FLAC__StreamMetadata *metadata, void *client_data)
{
	file_info_struct *file_info = (file_info_struct*)client_data;
	(void)decoder;

	if (metadata->type == FLAC__METADATA_TYPE_STREAMINFO)
	{
		FLAC__ASSERT(metadata->data.stream_info.total_samples < 0x100000000); /* this plugin can only handle < 4 gigasamples */
		file_info->total_samples = (unsigned)(metadata->data.stream_info.total_samples&0xfffffffful);
		file_info->bits_per_sample = metadata->data.stream_info.bits_per_sample;
		file_info->channels = metadata->data.stream_info.channels;
		file_info->sample_rate = metadata->data.stream_info.sample_rate;

		if (file_info->bits_per_sample!=8 && file_info->bits_per_sample!=16 && file_info->bits_per_sample!=24)
		{
			FLAC_plugin__show_error("This plugin can only handle 8/16/24-bit samples.");
			file_info->abort_flag = true;
			return;
		}
		file_info->length_in_msec = (unsigned)((double)file_info->total_samples / (double)file_info->sample_rate * 1000.0 + 0.5);
	}
	else if (metadata->type == FLAC__METADATA_TYPE_VORBIS_COMMENT)
	{
		double gain, peak;
		if (grabbag__replaygain_load_from_vorbiscomment(metadata, cfg.replaygain.album_mode, &gain, &peak))
		{
			file_info->has_replaygain = true;
			file_info->replay_scale = grabbag__replaygain_compute_scale_factor(peak, gain, (double)cfg.replaygain.preamp, !cfg.replaygain.hard_limit);
		}
	}
}

static void error_callback(const FLAC__FileDecoder *decoder, FLAC__StreamDecoderErrorStatus status, void *client_data)
{
	file_info_struct *file_info = (file_info_struct*)client_data;
	(void)decoder;

	if (cfg.misc.stop_err || status!=FLAC__STREAM_DECODER_ERROR_STATUS_LOST_SYNC)
		file_info->abort_flag = true;
}

/*
 *  init/delete
 */

FLAC__bool FLAC_plugin__decoder_init(FLAC__FileDecoder *decoder, const char *filename, FLAC__int64 filesize, file_info_struct *file_info, output_config_t *config)
{
	FLAC__ASSERT(decoder);
	FLAC_plugin__decoder_finish(decoder);
	/* init decoder */
	FLAC__file_decoder_set_md5_checking(decoder, false);
	FLAC__file_decoder_set_filename(decoder, filename);
	FLAC__file_decoder_set_metadata_ignore_all(decoder);
	FLAC__file_decoder_set_metadata_respond(decoder, FLAC__METADATA_TYPE_STREAMINFO);
	FLAC__file_decoder_set_metadata_respond(decoder, FLAC__METADATA_TYPE_VORBIS_COMMENT);
	FLAC__file_decoder_set_metadata_callback(decoder, metadata_callback);
	FLAC__file_decoder_set_write_callback(decoder, write_callback);
	FLAC__file_decoder_set_error_callback(decoder, error_callback);
	FLAC__file_decoder_set_client_data(decoder, file_info);

	if (FLAC__file_decoder_init(decoder) != FLAC__FILE_DECODER_OK)
	{
		FLAC_plugin__show_error("Error while initializing decoder (%s).", FLAC__FileDecoderStateString[FLAC__file_decoder_get_state(decoder)]);
		return false;
	}
	/* process */
	cfg = *config;
	wide_samples_in_reservoir_ = 0;
	file_info->is_playing = false;
	file_info->abort_flag = false;
	file_info->has_replaygain = false;

	if (!FLAC__file_decoder_process_until_end_of_metadata(decoder))
	{
		FLAC_plugin__show_error("Error while processing metadata (%s).", FLAC__FileDecoderStateString[FLAC__file_decoder_get_state(decoder)]);
		return false;
	}
	/* check results */
	if (file_info->abort_flag) return false;                /* metadata callback already popped up the error dialog */
	/* init replaygain */
	file_info->output_bits_per_sample = file_info->has_replaygain && cfg.replaygain.enable ?
		cfg.resolution.replaygain.bps_out :
		cfg.resolution.normal.dither_24_to_16 ? min(file_info->bits_per_sample, 16) : file_info->bits_per_sample;

	if (file_info->has_replaygain && cfg.replaygain.enable && cfg.resolution.replaygain.dither)
		FLAC__replaygain_synthesis__init_dither_context(&file_info->dither_context, file_info->bits_per_sample, cfg.resolution.replaygain.noise_shaping);
	/* more inits */
	file_info->eof = false;
	file_info->seek_to = -1;
	file_info->is_playing = true;
	file_info->average_bps = (unsigned)(filesize / (125.*file_info->total_samples/file_info->sample_rate));
	
	bh_index_last_w = 0;
	bh_index_last_o = BITRATE_HIST_SIZE;
	decode_position = 0;
	decode_position_last = 0;
	written_time_last = 0;

	return true;
}

void FLAC_plugin__decoder_finish(FLAC__FileDecoder *decoder)
{
	if (decoder && FLAC__file_decoder_get_state(decoder)!=FLAC__FILE_DECODER_UNINITIALIZED)
		FLAC__file_decoder_finish(decoder);
}

void FLAC_plugin__decoder_delete(FLAC__FileDecoder *decoder)
{
	if (decoder)
	{
		FLAC_plugin__decoder_finish(decoder);
		FLAC__file_decoder_delete(decoder);
	}
}

/*
 *  decode
 */

int FLAC_plugin__seek(FLAC__FileDecoder *decoder, file_info_struct *file_info)
{
	int pos;
	const FLAC__uint64 target_sample =
		(FLAC__uint64)file_info->total_samples*file_info->seek_to / file_info->length_in_msec;

	if (!FLAC__file_decoder_seek_absolute(decoder, target_sample))
		return -1;

	file_info->seek_to = -1;
	file_info->eof = false;
	wide_samples_in_reservoir_ = 0;
	pos = (int)(target_sample*1000 / file_info->sample_rate);

	bh_index_last_o = bh_index_last_w = (pos/BITRATE_HIST_SEGMENT_MSEC) % BITRATE_HIST_SIZE;
	if (!FLAC__file_decoder_get_decode_position(decoder, &decode_position))
		decode_position = 0;

	return pos;
}

unsigned FLAC_plugin__decode(FLAC__FileDecoder *decoder, file_info_struct *file_info, char *sample_buffer)
{
	/* fill reservoir */
	while (wide_samples_in_reservoir_ < SAMPLES_PER_WRITE)
	{
		if (FLAC__file_decoder_get_state(decoder) == FLAC__FILE_DECODER_END_OF_FILE)
		{
			file_info->eof = true;
			break;
		}
		else if (!FLAC__file_decoder_process_single(decoder))
		{
			FLAC_plugin__show_error("Error while processing frame (%s).", FLAC__FileDecoderStateString[FLAC__file_decoder_get_state(decoder)]);
			file_info->eof = true;
			break;
		}
		if (!FLAC__file_decoder_get_decode_position(decoder, &decode_position))
			decode_position = 0;
	}
	/* output samples */
	if (wide_samples_in_reservoir_ > 0)
	{
		const unsigned n = min(wide_samples_in_reservoir_, SAMPLES_PER_WRITE);
		const unsigned channels = file_info->channels;
		unsigned i;
		int bytes;

		if (cfg.replaygain.enable && file_info->has_replaygain)
		{
			bytes = FLAC__replaygain_synthesis__apply_gain(
				sample_buffer,
				true, /* little_endian_data_out */
				file_info->output_bits_per_sample == 8, /* unsigned_data_out */
				reservoir__,
				n,
				channels,
				file_info->bits_per_sample,
				file_info->output_bits_per_sample,
				file_info->replay_scale,
				cfg.replaygain.hard_limit,
				cfg.resolution.replaygain.dither,
				&file_info->dither_context
			);
		}
		else
		{
			bytes = FLAC__plugin_common__pack_pcm_signed_little_endian(
				sample_buffer,
				reservoir__,
				n,
				channels,
				file_info->bits_per_sample,
				file_info->output_bits_per_sample
			);
		}

		wide_samples_in_reservoir_ -= n;
		for (i = 0; i < channels; i++)
			memmove(&reservoir_[i][0], &reservoir_[i][n], sizeof(reservoir_[0][0]) * wide_samples_in_reservoir_);

		return bytes;
	}
	else
	{
		file_info->eof = true;
		return 0;
	}
}

int FLAC_plugin__get_rate(unsigned written_time, unsigned output_time, file_info_struct *file_info)
{
	static int bitrate_history_[BITRATE_HIST_SIZE];
	unsigned bh_index_w = (written_time/BITRATE_HIST_SEGMENT_MSEC) % BITRATE_HIST_SIZE;
	unsigned bh_index_o = (output_time/BITRATE_HIST_SEGMENT_MSEC) % BITRATE_HIST_SIZE;

	/* written bitrate */
	if (bh_index_w != bh_index_last_w)
	{
		bitrate_history_[(bh_index_w + BITRATE_HIST_SIZE-1)%BITRATE_HIST_SIZE] =
			decode_position>decode_position_last && written_time > written_time_last ?
			(unsigned)(8000*(decode_position - decode_position_last)/(written_time - written_time_last)) :
			file_info->average_bps;

		bh_index_last_w = bh_index_w;
		written_time_last = written_time;
		decode_position_last = decode_position;
	}

	/* output bitrate */
	if (bh_index_o!=bh_index_last_o && bh_index_o!=bh_index_last_w)
	{
		bh_index_last_o = bh_index_o;
		return bitrate_history_[bh_index_o];
	}

	return 0;
}
