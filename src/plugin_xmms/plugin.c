/* libxmms-flac - XMMS FLAC input plugin
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

#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <glib.h>

#include "xmms/plugin.h"
#include "xmms/util.h"
#include "FLAC/all.h"

#ifdef min
#undef min
#endif
#define min(x,y) ((x)<(y)?(x):(y))

typedef struct {
	FLAC__byte raw[128];
	char title[31];
	char artist[31];
	char album[31];
	char comment[31];
	unsigned year;
	unsigned track; /* may be 0 if v1 (not v1.1) tag */
	unsigned genre;
	char description[1024]; /* the formatted description passed to xmms */
} id3v1_struct;

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
	AFormat sample_format;
	int seek_to_in_sec;
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

static FLAC__bool get_id3v1_tag_(const char *filename, id3v1_struct *tag);
static void *play_loop_(void *arg);
static FLAC__bool decoder_init_(const char *filename);
static FLAC__StreamDecoderWriteStatus write_callback_(const FLAC__FileDecoder *decoder, const FLAC__Frame *frame, const FLAC__int32 *buffer[], void *client_data);
static void metadata_callback_(const FLAC__FileDecoder *decoder, const FLAC__StreamMetaData *metadata, void *client_data);
static void error_callback_(const FLAC__FileDecoder *decoder, FLAC__StreamDecoderErrorStatus status, void *client_data);


InputPlugin flac_ip =
{
	NULL,
	NULL,
	"FLAC Player v" FLAC__VERSION_STRING,
	FLAC_XMMS__init,
	NULL,
	NULL,
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
	NULL,			/* file_info_box */
	NULL
};

#define SAMPLES_PER_WRITE 512
static FLAC__byte reservoir_[FLAC__MAX_BLOCK_SIZE * 2 * 2 * 2]; /* *2 for max bytes-per-sample, *2 for max channels, another *2 for overflow */
static FLAC__byte output_[FLAC__MAX_BLOCK_SIZE * 2 * 2]; /* *2 for max bytes-per-sample, *2 for max channels */
static unsigned reservoir_samples_ = 0;
static FLAC__FileDecoder *decoder_ = 0;
static file_info_struct file_info_;
static pthread_t decode_thread_;
static FLAC__bool audio_error_ = false;

InputPlugin *get_iplugin_info()
{
	flac_ip.description = g_strdup_printf("FLAC Player v%s", FLAC__VERSION_STRING);
	return &flac_ip;
}

void FLAC_XMMS__init()
{
	decoder_ = FLAC__file_decoder_new();
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
	id3v1_struct tag;

	reservoir_samples_ = 0;
	audio_error_ = false;
	file_info_.abort_flag = false;
	file_info_.is_playing = false;
	file_info_.eof = false;
	file_info_.play_thread_open = false;

	if(0 == (f = fopen(filename, "r")))
		return;
	fclose(f);

	if(!decoder_init_(filename))
		return;

	file_info_.is_playing = true;

	if(flac_ip.output->open_audio(file_info_.sample_format, file_info_.sample_rate, file_info_.channels) == 0) {
		audio_error_ = true;
		if(decoder_)
			FLAC__file_decoder_finish(decoder_);
		return;
	}

	(void)get_id3v1_tag_(filename, &tag);
	flac_ip.set_info(tag.description, file_info_.length_in_msec, file_info_.sample_rate * file_info_.channels * file_info_.bits_per_sample, file_info_.sample_rate, file_info_.channels);

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
		if(decoder_)
			FLAC__file_decoder_finish(decoder_);
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
	if(decoder_) {
		FLAC__file_decoder_delete(decoder_);
		decoder_ = 0;
	}
}

void FLAC_XMMS__get_song_info(char *filename, char **title, int *length_in_msec)
{
	id3v1_struct tag;

	if(title) {
		(void)get_id3v1_tag_(filename, &tag);
		*title = g_malloc(strlen(tag.description)+1);
		strcpy(*title, tag.description);
	}
	if(length_in_msec) {
		FLAC__FileDecoder *tmp_decoder = FLAC__file_decoder_new();
		file_info_struct tmp_file_info;
		if(0 == tmp_decoder) {
			*length_in_msec = -1;
			return;
		}
		tmp_file_info.abort_flag = false;
		FLAC__file_decoder_set_md5_checking(tmp_decoder, false);
		FLAC__file_decoder_set_filename(tmp_decoder, filename);
		FLAC__file_decoder_set_write_callback(tmp_decoder, write_callback_);
		FLAC__file_decoder_set_metadata_callback(tmp_decoder, metadata_callback_);
		FLAC__file_decoder_set_error_callback(tmp_decoder, error_callback_);
		FLAC__file_decoder_set_client_data(tmp_decoder, &tmp_file_info);
		if(FLAC__file_decoder_init(tmp_decoder) != FLAC__FILE_DECODER_OK) {
			*length_in_msec = -1;
			return;
		}
		if(!FLAC__file_decoder_process_metadata(tmp_decoder)) {
			*length_in_msec = -1;
			return;
		}

		*length_in_msec = (int)tmp_file_info.length_in_msec;

		FLAC__file_decoder_finish(tmp_decoder);
		FLAC__file_decoder_delete(tmp_decoder);
	}
}

/***********************************************************************
 * local routines
 **********************************************************************/

FLAC__bool get_id3v1_tag_(const char *filename, id3v1_struct *tag)
{
	const char *temp;
	FILE *f = fopen(filename, "rb");
	memset(tag, 0, sizeof(id3v1_struct));

	/* set the title and description to the filename by default */
	temp = strrchr(filename, '/');
	if(!temp)
		temp = filename;
	else
		temp++;
	strcpy(tag->description, temp);
	*strrchr(tag->description, '.') = '\0';
	strncpy(tag->title, tag->description, 30); tag->title[30] = '\0';

	if(0 == f)
		return false;
	if(-1 == fseek(f, -128, SEEK_END)) {
		fclose(f);
		return false;
	}
	if(fread(tag->raw, 1, 128, f) < 128) {
		fclose(f);
		return false;
	}
	fclose(f);
	if(strncmp(tag->raw, "TAG", 3))
		return false;
	else {
		char year_str[5];

		memcpy(tag->title, tag->raw+3, 30);
		memcpy(tag->artist, tag->raw+33, 30);
		memcpy(tag->album, tag->raw+63, 30);
		memcpy(year_str, tag->raw+93, 4); year_str[4] = '\0'; tag->year = atoi(year_str);
		memcpy(tag->comment, tag->raw+97, 30);
		tag->genre = (unsigned)((FLAC__byte)tag->raw[127]);
		tag->track = (unsigned)((FLAC__byte)tag->raw[126]);

		sprintf(tag->description, "%s - %s", tag->artist, tag->title);

		return true;
	}
}

void *play_loop_(void *arg)
{
	(void)arg;

	while(file_info_.is_playing) {
		if(!file_info_.eof) {
			while(reservoir_samples_ < SAMPLES_PER_WRITE) {
				if(FLAC__file_decoder_get_state(decoder_) == FLAC__FILE_DECODER_END_OF_FILE) {
					file_info_.eof = true;
					break;
				}
				else if(!FLAC__file_decoder_process_one_frame(decoder_))
					break;
			}
			if(reservoir_samples_ > 0) {
				const unsigned channels = file_info_.channels;
				const unsigned bytes_per_sample = (file_info_.bits_per_sample+7)/8;
				unsigned i, n = min(reservoir_samples_, SAMPLES_PER_WRITE), delta;
				unsigned bytes = n * bytes_per_sample * channels;

				for(i = 0; i < bytes; i++)
					output_[i] = reservoir_[i];
				delta = i;
				for( ; i < reservoir_samples_*bytes_per_sample*channels; i++)
					reservoir_[i-delta] = reservoir_[i];
				reservoir_samples_ -= n;

				flac_ip.add_vis_pcm(flac_ip.output->written_time(), file_info_.sample_format, channels, bytes, output_);
				while(flac_ip.output->buffer_free() < (int)bytes && file_info_.is_playing && file_info_.seek_to_in_sec == -1)
					xmms_usleep(10000);
				if(file_info_.is_playing && file_info_.seek_to_in_sec == -1)
					flac_ip.output->write_audio(output_, bytes);
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
				file_info_.seek_to_in_sec = -1;
				file_info_.eof = false;
				reservoir_samples_ = 0;
			}
		}
	}

	if(decoder_)
		FLAC__file_decoder_finish(decoder_);

	/* are these two calls necessary? */
	flac_ip.output->buffer_free();
	flac_ip.output->buffer_free();

	pthread_exit(NULL);
	return 0; /* to silence the compiler warning about not returning a value */
}

FLAC__bool decoder_init_(const char *filename)
{
	if(decoder_ == 0)
		return false;

	FLAC__file_decoder_set_md5_checking(decoder_, false);
	FLAC__file_decoder_set_filename(decoder_, filename);
	FLAC__file_decoder_set_write_callback(decoder_, write_callback_);
	FLAC__file_decoder_set_metadata_callback(decoder_, metadata_callback_);
	FLAC__file_decoder_set_error_callback(decoder_, error_callback_);
	FLAC__file_decoder_set_client_data(decoder_, &file_info_);
	if(FLAC__file_decoder_init(decoder_) != FLAC__FILE_DECODER_OK)
		return false;

	if(!FLAC__file_decoder_process_metadata(decoder_))
		return false;

	return true;
}

FLAC__StreamDecoderWriteStatus write_callback_(const FLAC__FileDecoder *decoder, const FLAC__Frame *frame, const FLAC__int32 *buffer[], void *client_data)
{
	file_info_struct *file_info = (file_info_struct *)client_data;
	const unsigned bps = file_info->bits_per_sample, channels = file_info->channels, wide_samples = frame->header.blocksize;
	unsigned wide_sample, sample, channel;
	FLAC__int8 *scbuffer = (FLAC__int8*)reservoir_;
	FLAC__int16 *ssbuffer = (FLAC__int16*)reservoir_;

	(void)decoder;

	if(file_info->abort_flag)
		return FLAC__STREAM_DECODER_WRITE_STATUS_ABORT;

	if(bps == 8) {
		for(sample = reservoir_samples_*channels, wide_sample = 0; wide_sample < wide_samples; wide_sample++)
			for(channel = 0; channel < channels; channel++, sample++)
				scbuffer[sample] = (FLAC__int8)buffer[channel][wide_sample];
	}
	else if(bps == 16) {
		for(sample = reservoir_samples_*channels, wide_sample = 0; wide_sample < wide_samples; wide_sample++)
			for(channel = 0; channel < channels; channel++, sample++)
				ssbuffer[sample] = (FLAC__int16)buffer[channel][wide_sample];
	}
	else {
		file_info->abort_flag = true;
		return FLAC__STREAM_DECODER_WRITE_STATUS_ABORT;
	}

	reservoir_samples_ += wide_samples;

	return FLAC__STREAM_DECODER_WRITE_STATUS_CONTINUE;
}

void metadata_callback_(const FLAC__FileDecoder *decoder, const FLAC__StreamMetaData *metadata, void *client_data)
{
	file_info_struct *file_info = (file_info_struct *)client_data;
	(void)decoder;
	if(metadata->type == FLAC__METADATA_TYPE_STREAMINFO) {
		FLAC__ASSERT(metadata->data.stream_info.total_samples < 0x100000000); /* this plugin can only handle < 4 gigasamples */
		file_info->total_samples = (unsigned)(metadata->data.stream_info.total_samples&0xffffffff);
		file_info->bits_per_sample = metadata->data.stream_info.bits_per_sample;
		file_info->channels = metadata->data.stream_info.channels;
		file_info->sample_rate = metadata->data.stream_info.sample_rate;

		if(file_info->bits_per_sample == 8) {
			file_info->sample_format = FMT_S8;
		}
		else if(file_info->bits_per_sample == 16) {
			file_info->sample_format = FMT_S16_NE;
		}
		else {
			file_info->abort_flag = true;
			return;
		}
		file_info->length_in_msec = file_info->total_samples * 10 / (file_info->sample_rate / 100);
	}
}

void error_callback_(const FLAC__FileDecoder *decoder, FLAC__StreamDecoderErrorStatus status, void *client_data)
{
	file_info_struct *file_info = (file_info_struct *)client_data;
	(void)decoder;
	if(status != FLAC__STREAM_DECODER_ERROR_STATUS_LOST_SYNC)
		file_info->abort_flag = true;
}
