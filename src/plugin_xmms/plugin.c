/* libxmms-flac - XMMS FLAC input plugin
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

#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <glib.h>

#include <xmms/plugin.h>
#include <xmms/util.h>
#include <xmms/configfile.h>
#include <xmms/titlestring.h>

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#ifdef HAVE_LANGINFO_CODESET
#include <langinfo.h>
#endif

#include "FLAC/all.h"
#include "plugin_common/all.h"
#include "share/grabbag.h"
#include "configure.h"
#include "wrap_id3.h"
#include "charset.h"

#ifdef min
#undef min
#endif
#define min(x,y) ((x)<(y)?(x):(y))

extern void FLAC_XMMS__file_info_box(char *filename);

typedef struct {
	FLAC__bool abort_flag;
	FLAC__bool is_playing;
	FLAC__bool eof;
	FLAC__bool play_thread_open; /* if true, is_playing must also be true */
	unsigned total_samples;
	unsigned bits_per_sample;
	unsigned channels;
	unsigned sample_rate;
	unsigned length_in_msec;
	gchar *title;
	AFormat sample_format;
	unsigned sample_format_bytes_per_sample;
	int seek_to_in_sec;
	FLAC__bool has_replaygain;
	double replay_scale;
	DitherContext dither_context;
} file_info_struct;

static void FLAC_XMMS__init();
static int  FLAC_XMMS__is_our_file(char *filename);
static void FLAC_XMMS__play_file(char *filename);
static void FLAC_XMMS__stop();
static void FLAC_XMMS__pause(short p);
static void FLAC_XMMS__seek(int time);
static int  FLAC_XMMS__get_time();
static void FLAC_XMMS__cleanup();
static void FLAC_XMMS__get_song_info(char *filename, char **title, int *length);

static void *play_loop_(void *arg);
static FLAC__bool safe_decoder_init_(const char *filename, FLAC__FileDecoder *decoder);
static void safe_decoder_finish_(FLAC__FileDecoder *decoder);
static void safe_decoder_delete_(FLAC__FileDecoder *decoder);
static FLAC__StreamDecoderWriteStatus write_callback_(const FLAC__FileDecoder *decoder, const FLAC__Frame *frame, const FLAC__int32 * const buffer[], void *client_data);
static void metadata_callback_(const FLAC__FileDecoder *decoder, const FLAC__StreamMetadata *metadata, void *client_data);
static void error_callback_(const FLAC__FileDecoder *decoder, FLAC__StreamDecoderErrorStatus status, void *client_data);

InputPlugin flac_ip =
{
	NULL,
	NULL,
	"Reference FLAC Player v" VERSION,
	FLAC_XMMS__init,
	FLAC_XMMS__aboutbox,
	FLAC_XMMS__configure,
	FLAC_XMMS__is_our_file,
	NULL,
	FLAC_XMMS__play_file,
	FLAC_XMMS__stop,
	FLAC_XMMS__pause,
	FLAC_XMMS__seek,
	NULL,
	FLAC_XMMS__get_time,
	NULL,
	NULL,
	FLAC_XMMS__cleanup,
	NULL,
	NULL,
	NULL,
	NULL,
	FLAC_XMMS__get_song_info,
	FLAC_XMMS__file_info_box,
	NULL
};

#define SAMPLES_PER_WRITE 512
#define SAMPLE_BUFFER_SIZE ((FLAC__MAX_BLOCK_SIZE + SAMPLES_PER_WRITE) * FLAC_PLUGIN__MAX_SUPPORTED_CHANNELS * (24/8))
static FLAC__byte sample_buffer_[SAMPLE_BUFFER_SIZE];
static unsigned sample_buffer_first_, sample_buffer_last_;

static FLAC__FileDecoder *decoder_ = 0;
static file_info_struct file_info_;
static pthread_t decode_thread_;
static FLAC__bool audio_error_ = false;
static FLAC__bool is_big_endian_host_;

#define BITRATE_HIST_SEGMENT_MSEC 500
/* 500ms * 50 = 25s should be enough */
#define BITRATE_HIST_SIZE 50
static unsigned bitrate_history_[BITRATE_HIST_SIZE];


InputPlugin *get_iplugin_info()
{
	flac_ip.description = g_strdup_printf("Reference FLAC Player v%s", FLAC__VERSION_STRING);
	return &flac_ip;
}

void FLAC_XMMS__init()
{
	ConfigFile *cfg;
	FLAC__uint32 test = 1;
 
	is_big_endian_host_ = (*((FLAC__byte*)(&test)))? false : true;

	flac_cfg.title.tag_override = FALSE;
	g_free(flac_cfg.title.tag_format);
	flac_cfg.title.convert_char_set = FALSE;

	cfg = xmms_cfg_open_default_file();

	/* title */

	xmms_cfg_read_boolean(cfg, "flac", "title.tag_override", &flac_cfg.title.tag_override);

	if(!xmms_cfg_read_string(cfg, "flac", "title.tag_format", &flac_cfg.title.tag_format))
		flac_cfg.title.tag_format = g_strdup("%p - %t");

	xmms_cfg_read_boolean(cfg, "flac", "title.convert_char_set", &flac_cfg.title.convert_char_set);

	if(!xmms_cfg_read_string(cfg, "flac", "title.file_char_set", &flac_cfg.title.file_char_set))
		flac_cfg.title.file_char_set = FLAC_plugin__charset_get_current();

	if(!xmms_cfg_read_string(cfg, "flac", "title.user_char_set", &flac_cfg.title.user_char_set))
		flac_cfg.title.user_char_set = FLAC_plugin__charset_get_current();

	/* replaygain */

	xmms_cfg_read_boolean(cfg, "flac", "output.replaygain.enable", &flac_cfg.output.replaygain.enable);

	xmms_cfg_read_boolean(cfg, "flac", "output.replaygain.album_mode", &flac_cfg.output.replaygain.album_mode);

	if(!xmms_cfg_read_int(cfg, "flac", "output.replaygain.preamp", &flac_cfg.output.replaygain.preamp))
		flac_cfg.output.replaygain.preamp = 0;

	xmms_cfg_read_boolean(cfg, "flac", "output.replaygain.hard_limit", &flac_cfg.output.replaygain.hard_limit);

	xmms_cfg_read_boolean(cfg, "flac", "output.resolution.normal.dither_24_to_16", &flac_cfg.output.resolution.normal.dither_24_to_16);
	xmms_cfg_read_boolean(cfg, "flac", "output.resolution.replaygain.dither", &flac_cfg.output.resolution.replaygain.dither);

	if(!xmms_cfg_read_int(cfg, "flac", "output.resolution.replaygain.noise_shaping", &flac_cfg.output.resolution.replaygain.noise_shaping))
		flac_cfg.output.resolution.replaygain.noise_shaping = 1;

	if(!xmms_cfg_read_int(cfg, "flac", "output.resolution.replaygain.bps_out", &flac_cfg.output.resolution.replaygain.bps_out))
		flac_cfg.output.resolution.replaygain.bps_out = 16;

	decoder_ = FLAC__file_decoder_new();

	xmms_cfg_free(cfg);
}

int FLAC_XMMS__is_our_file(char *filename)
{
	char *ext;

	ext = strrchr(filename, '.');
	if(ext)
		if(!strcasecmp(ext, ".flac") || !strcasecmp(ext, ".fla"))
			return 1;
	return 0;
}

void FLAC_XMMS__play_file(char *filename)
{
	FILE *f;

	sample_buffer_first_ = sample_buffer_last_ = 0;
	audio_error_ = false;
	file_info_.abort_flag = false;
	file_info_.is_playing = false;
	file_info_.eof = false;
	file_info_.play_thread_open = false;
	file_info_.has_replaygain = false;

	if(0 == (f = fopen(filename, "r")))
		return;
	fclose(f);

	if(decoder_ == 0)
		return;

	if(!safe_decoder_init_(filename, decoder_))
		return;

	if(file_info_.has_replaygain && flac_cfg.output.replaygain.enable) {
		if(flac_cfg.output.resolution.replaygain.bps_out == 8) {
			file_info_.sample_format = FMT_U8;
			file_info_.sample_format_bytes_per_sample = 1;
		}
		else if(flac_cfg.output.resolution.replaygain.bps_out == 16) {
			file_info_.sample_format = (is_big_endian_host_) ? FMT_S16_BE : FMT_S16_LE;
			file_info_.sample_format_bytes_per_sample = 2;
		}
		else {
			/*@@@ need some error here like wa2: MessageBox(mod_.hMainWindow, "ERROR: plugin can only handle 8/16-bit samples\n", "ERROR: plugin can only handle 8/16-bit samples", 0); */
			fprintf(stderr, "libxmms-flac: can't handle %d bit output\n", flac_cfg.output.resolution.replaygain.bps_out);
			safe_decoder_finish_(decoder_);
			return;
		}
	}
	else {
		if(file_info_.bits_per_sample == 8) {
			file_info_.sample_format = FMT_U8;
			file_info_.sample_format_bytes_per_sample = 1;
		}
		else if(file_info_.bits_per_sample == 16 || (file_info_.bits_per_sample == 24 && flac_cfg.output.resolution.normal.dither_24_to_16)) {
			file_info_.sample_format = (is_big_endian_host_) ? FMT_S16_BE : FMT_S16_LE;
			file_info_.sample_format_bytes_per_sample = 2;
		}
		else {
			/*@@@ need some error here like wa2: MessageBox(mod_.hMainWindow, "ERROR: plugin can only handle 8/16-bit samples\n", "ERROR: plugin can only handle 8/16-bit samples", 0); */
			fprintf(stderr, "libxmms-flac: can't handle %d bit output\n", file_info_.bits_per_sample);
			safe_decoder_finish_(decoder_);
			return;
		}
	}
	FLAC__plugin_common__init_dither_context(&file_info_.dither_context, file_info_.sample_format_bytes_per_sample * 8, flac_cfg.output.resolution.replaygain.noise_shaping);
	file_info_.is_playing = true;

	if(flac_ip.output->open_audio(file_info_.sample_format, file_info_.sample_rate, file_info_.channels) == 0) {
		audio_error_ = true;
		safe_decoder_finish_(decoder_);
		return;
	}

	file_info_.title = flac_format_song_title(filename);
	flac_ip.set_info(file_info_.title, file_info_.length_in_msec, file_info_.sample_rate * file_info_.channels * file_info_.bits_per_sample, file_info_.sample_rate, file_info_.channels);

	file_info_.seek_to_in_sec = -1;
	file_info_.play_thread_open = true;
	pthread_create(&decode_thread_, NULL, play_loop_, NULL);
}

void FLAC_XMMS__stop()
{
	if(file_info_.is_playing) {
		file_info_.is_playing = false;
		if(file_info_.play_thread_open) {
			file_info_.play_thread_open = false;
			pthread_join(decode_thread_, NULL);
		}
		flac_ip.output->close_audio();
		safe_decoder_finish_(decoder_);
	}
}

void FLAC_XMMS__pause(short p)
{
	flac_ip.output->pause(p);
}

void FLAC_XMMS__seek(int time)
{
	file_info_.seek_to_in_sec = time;
	file_info_.eof = false;

	while(file_info_.seek_to_in_sec != -1)
		xmms_usleep(10000);
}

int FLAC_XMMS__get_time()
{
	if(audio_error_)
		return -2;
	if(!file_info_.is_playing || (file_info_.eof && !flac_ip.output->buffer_playing()))
		return -1;
	else
		return flac_ip.output->output_time();
}

void FLAC_XMMS__cleanup()
{
	safe_decoder_delete_(decoder_);
	decoder_ = 0;
}

void FLAC_XMMS__get_song_info(char *filename, char **title, int *length_in_msec)
{
	FLAC__StreamMetadata streaminfo;

	if(0 == filename)
		filename = "";

	if(!FLAC__metadata_get_streaminfo(filename, &streaminfo)) {
		/* @@@ how to report the error? */
		if(title) {
			static const char *errtitle = "Invalid FLAC File: ";
			*title = g_malloc(strlen(errtitle) + 1 + strlen(filename) + 1 + 1);
			sprintf(*title, "%s\"%s\"", errtitle, filename);
		}
		if(length_in_msec)
			*length_in_msec = -1;
		return;
	}

	if(title) {
		*title = flac_format_song_title(filename);
	}
	if(length_in_msec)
		*length_in_msec = streaminfo.data.stream_info.total_samples * 10 / (streaminfo.data.stream_info.sample_rate / 100);
}

/***********************************************************************
 * local routines
 **********************************************************************/

void *play_loop_(void *arg)
{
	unsigned written_time_last = 0, bh_index_last_w = 0, bh_index_last_o = BITRATE_HIST_SIZE, blocksize = 1;
	FLAC__uint64 decode_position_last = 0, decode_position_frame_last = 0, decode_position_frame = 0;

	(void)arg;

	while(file_info_.is_playing) {
		if(!file_info_.eof) {
			while(sample_buffer_last_ - sample_buffer_first_ < SAMPLES_PER_WRITE) {
				unsigned s;

				s = sample_buffer_last_ - sample_buffer_first_;
				if(FLAC__file_decoder_get_state(decoder_) == FLAC__FILE_DECODER_END_OF_FILE) {
					file_info_.eof = true;
					break;
				}
				else if(!FLAC__file_decoder_process_single(decoder_)) {
					/*@@@ this should probably be a dialog */
					fprintf(stderr, "libxmms-flac: READ ERROR processing frame\n");
					file_info_.eof = true;
					break;
				}
				blocksize = sample_buffer_last_ - sample_buffer_first_ - s;
				decode_position_frame_last = decode_position_frame;
				if(!FLAC__file_decoder_get_decode_position(decoder_, &decode_position_frame))
					decode_position_frame = 0;
			}
			if(sample_buffer_last_ - sample_buffer_first_ > 0) {
				const unsigned n = min(sample_buffer_last_ - sample_buffer_first_, SAMPLES_PER_WRITE);
				int bytes = n * file_info_.channels * file_info_.sample_format_bytes_per_sample;
				FLAC__byte *sample_buffer_start = sample_buffer_ + sample_buffer_first_ * file_info_.channels * file_info_.sample_format_bytes_per_sample;
				unsigned written_time, bh_index_w;
				FLAC__uint64 decode_position;

				sample_buffer_first_ += n;
				flac_ip.add_vis_pcm(flac_ip.output->written_time(), file_info_.sample_format, file_info_.channels, bytes, sample_buffer_start);
				while(flac_ip.output->buffer_free() < (int)bytes && file_info_.is_playing && file_info_.seek_to_in_sec == -1)
					xmms_usleep(10000);
				if(file_info_.is_playing && file_info_.seek_to_in_sec == -1)
					flac_ip.output->write_audio(sample_buffer_start, bytes);

				/* compute current bitrate */

				written_time = flac_ip.output->written_time();
				bh_index_w = written_time / BITRATE_HIST_SEGMENT_MSEC % BITRATE_HIST_SIZE;
				if(bh_index_w != bh_index_last_w) {
					bh_index_last_w = bh_index_w;
					decode_position = decode_position_frame - (double)(sample_buffer_last_ - sample_buffer_first_) * (double)(decode_position_frame - decode_position_frame_last) / (double)blocksize;
					bitrate_history_[(bh_index_w + BITRATE_HIST_SIZE - 1) % BITRATE_HIST_SIZE] =
						decode_position > decode_position_last && written_time > written_time_last ?
							8000 * (decode_position - decode_position_last) / (written_time - written_time_last) :
							file_info_.sample_rate * file_info_.channels * file_info_.bits_per_sample;
					decode_position_last = decode_position;
					written_time_last = written_time;
				}
			}
			else {
				file_info_.eof = true;
				xmms_usleep(10000);
			}
		}
		else
			xmms_usleep(10000);
		if(file_info_.seek_to_in_sec != -1) {
			const double distance = (double)file_info_.seek_to_in_sec * 1000.0 / (double)file_info_.length_in_msec;
			unsigned target_sample = (unsigned)(distance * (double)file_info_.total_samples);
			if(FLAC__file_decoder_seek_absolute(decoder_, (FLAC__uint64)target_sample)) {
				flac_ip.output->flush(file_info_.seek_to_in_sec * 1000);
				bh_index_last_w = bh_index_last_o = flac_ip.output->output_time() / BITRATE_HIST_SEGMENT_MSEC % BITRATE_HIST_SIZE;
				if(!FLAC__file_decoder_get_decode_position(decoder_, &decode_position_frame))
					decode_position_frame = 0;
				file_info_.seek_to_in_sec = -1;
				file_info_.eof = false;
				sample_buffer_first_ = sample_buffer_last_ = 0;
			}
		}
		else {
			/* display the right bitrate from history */
			unsigned bh_index_o = flac_ip.output->output_time() / BITRATE_HIST_SEGMENT_MSEC % BITRATE_HIST_SIZE;
			if(bh_index_o != bh_index_last_o && bh_index_o != bh_index_last_w && bh_index_o != (bh_index_last_w + 1) % BITRATE_HIST_SIZE) {
				bh_index_last_o = bh_index_o;
				flac_ip.set_info(file_info_.title, file_info_.length_in_msec, bitrate_history_[bh_index_o], file_info_.sample_rate, file_info_.channels);
			}
		}
	}

	safe_decoder_finish_(decoder_);

	/* are these two calls necessary? */
	flac_ip.output->buffer_free();
	flac_ip.output->buffer_free();

	g_free(file_info_.title);

	pthread_exit(NULL);
	return 0; /* to silence the compiler warning about not returning a value */
}

FLAC__bool safe_decoder_init_(const char *filename, FLAC__FileDecoder *decoder)
{
	if(decoder == 0)
		return false;

	safe_decoder_finish_(decoder);

	FLAC__file_decoder_set_md5_checking(decoder, false);
	FLAC__file_decoder_set_filename(decoder, filename);
	FLAC__file_decoder_set_metadata_ignore_all(decoder);
	FLAC__file_decoder_set_metadata_respond(decoder, FLAC__METADATA_TYPE_STREAMINFO);
	FLAC__file_decoder_set_metadata_respond(decoder, FLAC__METADATA_TYPE_VORBIS_COMMENT);
	FLAC__file_decoder_set_write_callback(decoder, write_callback_);
	FLAC__file_decoder_set_metadata_callback(decoder, metadata_callback_);
	FLAC__file_decoder_set_error_callback(decoder, error_callback_);
	FLAC__file_decoder_set_client_data(decoder, &file_info_);
	if(FLAC__file_decoder_init(decoder) != FLAC__FILE_DECODER_OK)
		return false;

	if(!FLAC__file_decoder_process_until_end_of_metadata(decoder))
		return false;

	return true;
}

void safe_decoder_finish_(FLAC__FileDecoder *decoder)
{
	if(decoder && FLAC__file_decoder_get_state(decoder) != FLAC__FILE_DECODER_UNINITIALIZED)
		FLAC__file_decoder_finish(decoder);
}

void safe_decoder_delete_(FLAC__FileDecoder *decoder)
{
	if(decoder) {
		safe_decoder_finish_(decoder);
		FLAC__file_decoder_delete(decoder);
	}
}

FLAC__StreamDecoderWriteStatus write_callback_(const FLAC__FileDecoder *decoder, const FLAC__Frame *frame, const FLAC__int32 * const buffer[], void *client_data)
{
	file_info_struct *file_info = (file_info_struct *)client_data;
	const unsigned channels = file_info->channels, wide_samples = frame->header.blocksize;
	const unsigned bits_per_sample = file_info->bits_per_sample;
	FLAC__byte *sample_buffer_start;

	(void)decoder;

	if(file_info->abort_flag)
		return FLAC__STREAM_DECODER_WRITE_STATUS_ABORT;

	if((sample_buffer_last_ + wide_samples) > (SAMPLE_BUFFER_SIZE / (channels * file_info->sample_format_bytes_per_sample))) {
		memmove(sample_buffer_, sample_buffer_ + sample_buffer_first_ * channels * file_info->sample_format_bytes_per_sample, (sample_buffer_last_ - sample_buffer_first_) * channels * file_info->sample_format_bytes_per_sample);
		sample_buffer_last_ -= sample_buffer_first_;
		sample_buffer_first_ = 0;
	}
	sample_buffer_start = sample_buffer_ + sample_buffer_last_ * channels * file_info->sample_format_bytes_per_sample;
	if(file_info_.has_replaygain && flac_cfg.output.replaygain.enable) {
		FLAC__plugin_common__apply_gain(
				sample_buffer_start,
				!is_big_endian_host_,
				buffer,
				wide_samples,
				channels,
				bits_per_sample,
				file_info->sample_format_bytes_per_sample * 8,
				file_info_.replay_scale,
				flac_cfg.output.replaygain.hard_limit,
				flac_cfg.output.resolution.replaygain.dither,
				&file_info_.dither_context
		);
	}
	else if(is_big_endian_host_) {
		FLAC__plugin_common__pack_pcm_signed_big_endian(
			sample_buffer_start,
			buffer,
			wide_samples,
			channels,
			bits_per_sample,
			file_info->sample_format_bytes_per_sample * 8
		);
	}
	else {
		FLAC__plugin_common__pack_pcm_signed_little_endian(
			sample_buffer_start,
			buffer,
			wide_samples,
			channels,
			bits_per_sample,
			file_info->sample_format_bytes_per_sample * 8
		);
	}

	sample_buffer_last_ += wide_samples;

	return FLAC__STREAM_DECODER_WRITE_STATUS_CONTINUE;
}

void metadata_callback_(const FLAC__FileDecoder *decoder, const FLAC__StreamMetadata *metadata, void *client_data)
{
	file_info_struct *file_info = (file_info_struct *)client_data;
	(void)decoder;
	if(metadata->type == FLAC__METADATA_TYPE_STREAMINFO) {
		FLAC__ASSERT(metadata->data.stream_info.total_samples < 0x100000000); /* this plugin can only handle < 4 gigasamples */
		file_info->total_samples = (unsigned)(metadata->data.stream_info.total_samples&0xffffffff);
		file_info->bits_per_sample = metadata->data.stream_info.bits_per_sample;
		file_info->channels = metadata->data.stream_info.channels;
		file_info->sample_rate = metadata->data.stream_info.sample_rate;
		file_info->length_in_msec = (FLAC__uint64)file_info->total_samples * 10 / (file_info->sample_rate / 100);
	}
	else if(metadata->type == FLAC__METADATA_TYPE_VORBIS_COMMENT) {
		double gain, peak;
		if(grabbag__replaygain_load_from_vorbiscomment(metadata, flac_cfg.output.replaygain.album_mode, &gain, &peak)) {
			file_info_.has_replaygain = true;
			file_info_.replay_scale = grabbag__replaygain_compute_scale_factor(peak, gain, (double)flac_cfg.output.replaygain.preamp, /*prevent_clipping=*/!flac_cfg.output.replaygain.hard_limit);
		}
	}
}

void error_callback_(const FLAC__FileDecoder *decoder, FLAC__StreamDecoderErrorStatus status, void *client_data)
{
	file_info_struct *file_info = (file_info_struct *)client_data;
	(void)decoder;
	if(status != FLAC__STREAM_DECODER_ERROR_STATUS_LOST_SYNC)
		file_info->abort_flag = true;
}
