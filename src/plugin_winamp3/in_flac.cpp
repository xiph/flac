/* in_flac - Winamp3 FLAC input plugin
 * Copyright (C) 2001  Josh Coalson
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

#include <windows.h>
#include <stdio.h>
#include "core_api.h"
#include "FLAC/all.h"

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
	unsigned total_samples;
	unsigned bits_per_sample;
	unsigned channels;
	unsigned sample_rate;
	unsigned length_in_msec;
} file_info_struct;

static FLAC__bool get_id3v1_tag_(const char *filename, id3v1_struct *tag);
static FLAC__bool decoder_init_(const char *filename);
static FLAC__StreamDecoderWriteStatus write_callback_(const FLAC__FileDecoder *decoder, const FLAC__Frame *frame, const FLAC__int32 *buffer[], void *client_data);
static void metadata_callback_(const FLAC__FileDecoder *decoder, const FLAC__StreamMetaData *metadata, void *client_data);
static void error_callback_(const FLAC__FileDecoder *decoder, FLAC__StreamDecoderErrorStatus status, void *client_data);

static char plugin_description_[] = "Reference FLAC Player v" FLAC__VERSION_STRING;
static HINSTANCE g_hDllInstance_;

class FLAC_Info : public WInputInfo
{
	private:
		id3v1_struct tag_;
		int length_in_msec_;
	public:
		FLAC_Info();
		~FLAC_Info();
		int Open(char *url);
		void GetTitle(char *buf, int maxlen);
		void GetInfoString(char *buf, int maxlen);
		inline int GetLength(void) { return length_in_msec_; }
};

FLAC_Info::FLAC_Info() : WInputInfo()
{
	length_in_msec_ = -1;
}

FLAC_Info::~FLAC_Info()
{
}

int FLAC_Info::Open(char *url)
{
	const char *filename = url; /* @@@ right now we only handle files */

	(void)get_id3v1_tag_(filename, &tag_);

	file_info_struct tmp_file_info;
	FLAC__FileDecoder *tmp_decoder = FLAC__file_decoder_new();
	if(0 == tmp_decoder) {
		length_in_msec_ = -1;
		return 1;
	}
	tmp_file_info.abort_flag = false;
	FLAC__file_decoder_set_md5_checking(tmp_decoder, false);
	FLAC__file_decoder_set_filename(tmp_decoder, filename);
	FLAC__file_decoder_set_write_callback(tmp_decoder, write_callback_);
	FLAC__file_decoder_set_metadata_callback(tmp_decoder, metadata_callback_);
	FLAC__file_decoder_set_error_callback(tmp_decoder, error_callback_);
	FLAC__file_decoder_set_client_data(tmp_decoder, &tmp_file_info);
	if(FLAC__file_decoder_init(tmp_decoder) != FLAC__FILE_DECODER_OK) {
		length_in_msec_ = -1;
		return 1;
	}
	if(!FLAC__file_decoder_process_metadata(tmp_decoder)) {
		length_in_msec_ = -1;
		return 1;
	}

	length_in_msec_ = (int)tmp_file_info.length_in_msec;

	if(FLAC__file_decoder_get_state(tmp_decoder) != FLAC__FILE_DECODER_UNINITIALIZED)
		FLAC__file_decoder_finish(tmp_decoder);
	FLAC__file_decoder_delete(tmp_decoder);

	return 0;
}

void FLAC_Info::GetTitle(char *buf, int maxlen)
{
	strncpy(buf, title, maxlen-1);
	buf[maxlen-1] = 0;
}

void FLAC_Info::GetInfoString(char *buf, int maxlen)
{
	strncpy(buf, description, maxlen-1);
	buf[maxlen-1] = 0;
}


class FLAC_Source : public WInputSource
{
	private:
		id3v1_struct tag_;
		FLAC__FileDecoder *decoder_;
		FLAC__int16 reservoir_[FLAC__MAX_BLOCK_SIZE * 2 * 2]; /* *2 for max channels, another *2 for overflow */
		unsigned reservoir_samples_;
		file_info_struct file_info_;
		FLAC__bool audio_error_;
	public:
		FLAC_Source();
		~FLAC_Source();
		inline char *GetDescription(void) { return plugin_description_; }
		inline int UsesOutputFilters(void) { return 1; }
		int Open(char *url, FLAC__bool *killswitch);
		int GetSamples(char *sample_buffer, int bytes, int *bps, int *nch, int *srate, FLAC__bool *killswitch);
		int SetPosition(int); // sets position in ms
		void GetTitle(char *buf, int maxlen);
		void GetInfoString(char *buf, int maxlen);
		inline int GetLength(void) { return (int)file_info_.length_in_msec; }
		void cleanup();
};

FLAC_Source::FLAC_Source() : WInputSource()
{
	decoder_ = FLAC__file_decoder_new();
	file_info_.length_in_msec = -1;
	audio_error_ = false;
}

FLAC_Source::~FLAC_Source()
{
	if(decoder_)
		FLAC__file_decoder_delete(decoder_);
}

void FLAC_Source::GetTitle(char *buf, int maxlen)
{
	strncpy(buf, title, maxlen-1);
	buf[maxlen-1] = 0;
}

void FLAC_Source::GetInfoString(char *buf, int maxlen)
{
	strncpy(buf, description, maxlen-1);
	buf[maxlen-1] = 0;
}

int FLAC_Source::Open(char *url, FLAC__bool *killswitch)
{
	const char *filename = url; /* @@@ right now we only handle files */

	{
		FILE *f = fopen(filename, "r");
		if(0 == f)
			return 1;
		fclose(f);
	}

	(void)get_id3v1_tag_(filename, &tag);

	file_info_.length_in_msec = -1;

	if(!decoder_init_(filename))
		return 1;

	if(file_info_.abort_flag) {
		cleanup();
		return 1;
	}

	reservoir_samples_ = 0;
	audio_error_ = false;
	file_info_.is_playing = true;
	file_info_.eof = false;

	return 0;
}

int FLAC_Source::GetSamples(char *sample_buffer, int bytes, int *bps, int *nch, int *srate, FLAC__bool *killswitch)
{
	int return_bytes = 0;

	*bps = (int)file_info_.bits_per_sample;
	*nch = (int)file_info_.channels;
	*srate = (int)file_info_.sample_rate;

	if(!file_info_.eof) {
		const unsigned channels = file_info_.channels;
		const unsigned bytes_per_sample = (file_info_.bits_per_sample+7) / 8;
		const unsigned wide_samples = bytes / channels / bytes_per_sample;
if(bytes&0x3)fprintf(stderr,"@@@ Got odd buffer size request\n");
		while(reservoir_samples_ < wide_samples) {
			if(FLAC__file_decoder_get_state(decoder) == FLAC__FILE_DECODER_END_OF_FILE) {
				file_info_.eof = true;
				break;
			}
			else if(!FLAC__file_decoder_process_one_frame(decoder_))
				break;
		}
		if(reservoir_samples_ > 0) {
			unsigned i, n = min(reservoir_samples_, wide_samples), delta;
			FLAC__int16 output = (FLAC__int16*)sample_buffer;

			for(i = 0; i < n*channels; i++)
				output[i] = reservoir_[i];
			delta = i;
			for( ; i < reservoir_samples_*channels; i++)
				reservoir[i-delta] = reservoir[i];
			reservoir_samples_ -= n;

			return_bytes = (int)(n * bytes_per_sample * channels);
		}
		else {
			file_info_.eof = true;
		}
	}
	return return_bytes;
}

int FLAC_Source::SetPosition(int position)
{
	const double distance = (double)position * 1000.0 / (double)file_info_.length_in_msec;
	unsigned target_sample = (unsigned)(distance * (double)file_info_.total_samples);
	if(FLAC__file_decoder_seek_absolute(decoder_, (FLAC__uint64)target_sample)) {
		file_info_.eof = false;
		reservoir_samples_ = 0;
		return 0;
	}
	else {
		return 1;
	}
}

void FLAC_Source::cleanup()
{
	if(decoder_ && FLAC__file_decoder_get_state(decoder_) != FLAC__FILE_DECODER_UNINITIALIZED)
		FLAC__file_decoder_finish(decoder_);

	reservoir_samples_ = 0;
	audio_error_ = false;
	file_info_.is_playing = false;
	file_info_.eof = false;
	file_info_.length_in_msec = -1;
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

FLAC__bool decoder_init_(const char *filename)
{
	if(decoder_ == 0)
		return false;

	FLAC__file_decoder_set_md5_checking(decoder, false);
	FLAC__file_decoder_set_filename(decoder, filename);
	FLAC__file_decoder_set_write_callback(decoder, write_callback_);
	FLAC__file_decoder_set_metadata_callback(decoder, metadata_callback_);
	FLAC__file_decoder_set_error_callback(decoder, error_callback_);
	FLAC__file_decoder_set_client_data(decoder, &file_info_);
	if(FLAC__file_decoder_init(decoder_) != FLAC__FILE_DECODER_OK)
		return false;

	file_info_.abort_flag = false;

	if(!FLAC__file_decoder_process_metadata(decoder_))
		return false;

	return true;
}

FLAC__StreamDecoderWriteStatus write_callback_(const FLAC__FileDecoder *decoder, const FLAC__Frame *frame, const FLAC__int32 *buffer[], void *client_data)
{
	file_info_struct *file_info = (file_info_struct *)client_data;
	const unsigned bps = file_info->bits_per_sample, channels = file_info->channels, wide_samples = frame->header.blocksize;
	unsigned wide_sample, sample, channel;

	(void)decoder;

	if(file_info->abort_flag)
		return FLAC__STREAM_DECODER_WRITE_ABORT;

	FLAC__ASSERT(bps == 16);

	for(sample = reservoir_samples_*channels, wide_sample = 0; wide_sample < wide_samples; wide_sample++)
		for(channel = 0; channel < channels; channel++, sample++)
			reservoir_[sample] = (FLAC__int16)buffer[channel][wide_sample];

	reservoir_samples_ += wide_samples;

	return FLAC__STREAM_DECODER_WRITE_CONTINUE;
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


/***********************************************************************
 * C-level interface
 **********************************************************************/

static int C_level__FLAC_is_mine_(char *filename)
{
	return 0;
}

static WInputSource *C_level__FLAC_create_()
{
	return new FLAC_Source();
}

static WInputInfo *C_level__FLAC_create_info_()
{
	return new FLAC_Info();
}

input_source C_level__FLAC_source_ = {
	IN_VER,
	plugin_description_,
	"FLAC;FLAC Audio File (*.FLAC)",
	C_level__FLAC_is_mine_,
	C_level__FLAC_create_,
	C_level__FLAC_create_info_
};

extern "C"
{
	__declspec (dllexport) int getInputSource(HINSTANCE hDllInstance, input_source **inStruct)
	{
		g_hDllInstance_ = hDllInstance;
		*inStruct = &C_level__FLAC_source_;
		return 1;
	}
}
