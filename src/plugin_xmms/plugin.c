/* libxmms-flac - XMMS FLAC input plugin
 * Copyright (C) 2000,2001  Josh Coalson
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
#include <assert.h>
#include <glib.h>

#include "xmms/plugin.h"
#include "xmms/util.h"
#include "FLAC/all.h"

typedef struct {
	byte raw[128];
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
	bool abort_flag;
	bool is_playing;
	bool eof;
	unsigned total_samples;
	unsigned bits_per_sample;
	unsigned channels;
	unsigned sample_rate;
	unsigned length_in_msec;
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

static void *play_loop_(void *arg);
static bool decoder_init_(const char *filename);
static bool get_id3v1_tag_(const char *filename, id3v1_struct *tag);
static FLAC__StreamDecoderWriteStatus write_callback_(const FLAC__FileDecoder *decoder, const FLAC__Frame *frame, const int32 *buffer[], void *client_data);
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

static int16 reservoir_[FLAC__MAX_BLOCK_SIZE * 2 * 2]; /* *2 for max channels, another *2 for overflow */
#ifdef RESERVOIR_TEST
static int16 output_[FLAC__MAX_BLOCK_SIZE * 2]; /* *2 for max channels */
#endif
static unsigned reservoir_samples_;
static FLAC__FileDecoder *decoder_;
static file_info_struct file_info_;
static pthread_t decode_thread_;
static bool audio_error_ = false;

InputPlugin *get_iplugin_info()
{
	flac_ip.description = g_strdup_printf("FLAC Player v%s", FLAC__VERSION_STRING);
	return &flac_ip;
}

void FLAC_XMMS__init()
{
	decoder_ = FLAC__file_decoder_get_new_instance();
}

int FLAC_XMMS__is_our_file(char *filename)
{
	char *ext;

	ext = strrchr(filename, '.');
	if (ext)
		if (!strcasecmp(ext, ".flac") || !strcasecmp(ext, ".fla"))
			return 1;
	return 0;
}

void FLAC_XMMS__play_file(char *filename)
{
	FILE *f;
	id3v1_struct tag;

	if(0 == (f = fopen(filename, "r")))
		return;
	fclose(f);

	if(!decoder_init_(filename))
		return;

	reservoir_samples_ = 0;
	audio_error_ = false;
	file_info_.is_playing = true;
	file_info_.eof = false;

	if (flac_ip.output->open_audio(FMT_S16_NE, file_info_.sample_rate, file_info_.channels) == 0) {
		audio_error_ = true;
		if(decoder_ && decoder_->state != FLAC__FILE_DECODER_UNINITIALIZED)
			FLAC__file_decoder_finish(decoder_);
		return;
	}

	(void)get_id3v1_tag_(filename, &tag);
	flac_ip.set_info(tag.description, file_info_.length_in_msec, file_info_.sample_rate * file_info_.channels * file_info_.bits_per_sample, file_info_.sample_rate, file_info_.channels);

	file_info_.seek_to_in_sec = -1;
	pthread_create(&decode_thread_, NULL, play_loop_, NULL);
}

void FLAC_XMMS__stop()
{
	if(file_info_.is_playing) {
		file_info_.is_playing = false;
		pthread_join(decode_thread_, NULL);
		flac_ip.output->close_audio();
		if(decoder_ && decoder_->state != FLAC__FILE_DECODER_UNINITIALIZED)
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
	if(decoder_)
		FLAC__file_decoder_free_instance(decoder_);
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
		FLAC__FileDecoder *tmp_decoder = FLAC__file_decoder_get_new_instance();
		file_info_struct tmp_file_info;
		if(0 == tmp_decoder) {
			*length_in_msec = -1;
			return;
		}
		tmp_file_info.abort_flag = false;
		tmp_decoder->check_md5 = false; /* turn off MD5 checking in the decoder */
		if(FLAC__file_decoder_init(tmp_decoder, filename, write_callback_, metadata_callback_, error_callback_, &tmp_file_info) != FLAC__FILE_DECODER_OK) {
			*length_in_msec = -1;
			return;
		}
		if(!FLAC__file_decoder_process_metadata(tmp_decoder)) {
			*length_in_msec = -1;
			return;
		}

		*length_in_msec = (int)tmp_file_info.length_in_msec;

		if(tmp_decoder->state != FLAC__FILE_DECODER_UNINITIALIZED)
			FLAC__file_decoder_finish(tmp_decoder);
		FLAC__file_decoder_free_instance(tmp_decoder);
	}
}

/***********************************************************************
 * local routines
 **********************************************************************/

#ifndef RESERVOIR_TEST
void *play_loop_(void *arg)
{

	(void)arg;

	while(file_info_.is_playing) {
		if(!file_info_.eof) {
			(void)FLAC__file_decoder_process_one_frame(decoder_);
			if(reservoir_samples_ > 0) {
				unsigned bytes = reservoir_samples_ * ((file_info_.bits_per_sample+7)/8) * file_info_.channels;
				flac_ip.add_vis_pcm(flac_ip.output->written_time(), FMT_S16_NE, file_info_.channels, bytes, (char*)reservoir_);
				while(flac_ip.output->buffer_free() < (int)bytes && file_info_.is_playing && file_info_.seek_to_in_sec == -1)
					xmms_usleep(10000);
				if(file_info_.is_playing && file_info_.seek_to_in_sec == -1)
					flac_ip.output->write_audio((char*)reservoir_, bytes);
				reservoir_samples_ = 0;
			}
			else {
				file_info_.eof = true;
				xmms_usleep(10000);
			}
		}
		else
			xmms_usleep(10000);
		if (file_info_.seek_to_in_sec != -1) {
			const double distance = (double)file_info_.seek_to_in_sec * 1000.0 / (double)file_info_.length_in_msec;
			unsigned target_sample = (unsigned)(distance * (double)file_info_.total_samples);
			if(FLAC__file_decoder_seek_absolute(decoder_, (uint64)target_sample)) {
				flac_ip.output->flush(file_info_.seek_to_in_sec * 1000);
				file_info_.seek_to_in_sec = -1;
				file_info_.eof = false;
			}
		}

	}
	if(decoder_ && decoder_->state != FLAC__FILE_DECODER_UNINITIALIZED)
		FLAC__file_decoder_finish(decoder_);

	/* are these two calls necessary? */
	flac_ip.output->buffer_free();
	flac_ip.output->buffer_free();

	pthread_exit(NULL);
	return 0; /* to silence the compiler warning about not returning a value */
}
#else
void *play_loop_(void *arg)
{

	(void)arg;

	while(file_info_.is_playing) {
		if(!file_info_.eof) {
			while(samples_in_reservoir < 576) {
				if(decoder->state == FLAC__FILE_DECODER_END_OF_FILE) {
					file_info_.eof = true;
					break;
				}
				else if(!FLAC__file_decoder_process_one_frame(decoder_))
					break;
			}
			if(reservoir_samples_ > 0) {
				unsigned i, n = min(samples_in_reservoir, 576), delta;
				unsigned bytes = reservoir_samples_ * ((file_info_.bits_per_sample+7)/8) * file_info_.channels;
				const unsigned channels = file_info_.channels;

				for(i = 0; i < n*channels; i++)
					output_[i] = reservoir_[i];
				delta = i;
				for( ; i < reservoir_samples_*channels; i++)
					reservoir[i-delta] = reservoir[i];
				reservoir_samples_ -= n;

				flac_ip.add_vis_pcm(flac_ip.output->written_time(), FMT_S16_NE, channels, bytes, (char*)output_);
				while(flac_ip.output->buffer_free() < (int)bytes && file_info_.is_playing && file_info_.seek_to_in_sec == -1)
					xmms_usleep(10000);
				if(file_info_.is_playing && file_info_.seek_to_in_sec == -1)
					flac_ip.output->write_audio((char*)output_, bytes);
			}
			else {
				file_info_.eof = true;
				xmms_usleep(10000);
			}
		}
		else
			xmms_usleep(10000);
		if (file_info_.seek_to_in_sec != -1) {
			const double distance = (double)file_info_.seek_to_in_sec * 1000.0 / (double)file_info_.length_in_msec;
			unsigned target_sample = (unsigned)(distance * (double)file_info_.total_samples);
			if(FLAC__file_decoder_seek_absolute(decoder_, (uint64)target_sample)) {
				flac_ip.output->flush(file_info_.seek_to_in_sec * 1000);
				file_info_.seek_to_in_sec = -1;
				file_info_.eof = false;
				reservoir_samples_ = 0;
			}
		}

	}
	if(decoder_ && decoder_->state != FLAC__FILE_DECODER_UNINITIALIZED)
		FLAC__file_decoder_finish(decoder_);

	/* are these two calls necessary? */
	flac_ip.output->buffer_free();
	flac_ip.output->buffer_free();

	pthread_exit(NULL);
	return 0; /* to silence the compiler warning about not returning a value */
}
#endif

bool decoder_init_(const char *filename)
{
	if(decoder_ == 0)
		return false;

	decoder_->check_md5 = false; /* turn off MD5 checking in the decoder */

	if(FLAC__file_decoder_init(decoder_, filename, write_callback_, metadata_callback_, error_callback_, &file_info_) != FLAC__FILE_DECODER_OK)
		return false;

	file_info_.abort_flag = false;

	if(!FLAC__file_decoder_process_metadata(decoder_))
		return false;

	return true;
}

bool get_id3v1_tag_(const char *filename, id3v1_struct *tag)
{
	const char *temp;
	FILE *f = fopen(filename, "rb");
	memset(tag, 0, sizeof(id3v1_struct));

	/* set the description to the filename by default */
	temp = strrchr(filename, '/');
	if(!temp)
		temp = filename;
	else
		temp++;
	strcpy(tag->description, temp);
	*strrchr(tag->description, '.') = '\0';

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
		tag->genre = (unsigned)((byte)tag->raw[127]);
		tag->track = (unsigned)((byte)tag->raw[126]);

		sprintf(tag->description, "%s - %s", tag->artist, tag->title);

		return true;
	}
}

#ifndef RESERVOIR_TEST
FLAC__StreamDecoderWriteStatus write_callback_(const FLAC__FileDecoder *decoder, const FLAC__Frame *frame, const int32 *buffer[], void *client_data)
{
	file_info_struct *file_info = (file_info_struct *)client_data;
	const unsigned bps = file_info->bits_per_sample, channels = file_info->channels, wide_samples = frame->header.blocksize;
	unsigned wide_sample, sample, channel;

	(void)decoder;

	if(file_info->abort_flag)
		return FLAC__STREAM_DECODER_WRITE_ABORT;

	assert(bps == 16);

	for(sample = wide_sample = 0; wide_sample < wide_samples; wide_sample++)
		for(channel = 0; channel < channels; channel++, sample++)
			reservoir_[sample] = (int16)buffer[channel][wide_sample];

	reservoir_samples_ = wide_samples;

	return FLAC__STREAM_DECODER_WRITE_CONTINUE;
}
#else
FLAC__StreamDecoderWriteStatus write_callback_(const FLAC__FileDecoder *decoder, const FLAC__Frame *frame, const int32 *buffer[], void *client_data)
{
	file_info_struct *file_info = (file_info_struct *)client_data;
	const unsigned bps = file_info->bits_per_sample, channels = file_info->channels, wide_samples = frame->header.blocksize;
	unsigned wide_sample, sample, channel;

	(void)decoder;

	if(file_info->abort_flag)
		return FLAC__STREAM_DECODER_WRITE_ABORT;

	assert(bps == 16);

	for(sample = reservoir_samples_*channels, wide_sample = 0; wide_sample < wide_samples; wide_sample++)
		for(channel = 0; channel < channels; channel++, sample++)
			reservoir_[sample] = (int16)buffer[channel][wide_sample];

	reservoir_samples_ += wide_samples;

	return FLAC__STREAM_DECODER_WRITE_CONTINUE;
}
#endif

void metadata_callback_(const FLAC__FileDecoder *decoder, const FLAC__StreamMetaData *metadata, void *client_data)
{
	file_info_struct *file_info = (file_info_struct *)client_data;
	(void)decoder;
	if(metadata->type == FLAC__METADATA_TYPE_ENCODING) {
		assert(metadata->data.encoding.total_samples < 0x100000000); /* this plugin can only handle < 4 gigasamples */
		file_info->total_samples = (unsigned)(metadata->data.encoding.total_samples&0xffffffff);
		file_info->bits_per_sample = metadata->data.encoding.bits_per_sample;
		file_info->channels = metadata->data.encoding.channels;
		file_info->sample_rate = metadata->data.encoding.sample_rate;

		if(file_info->bits_per_sample != 16) {
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
	if(status != FLAC__STREAM_DECODER_ERROR_LOST_SYNC)
		file_info->abort_flag = true;
}
