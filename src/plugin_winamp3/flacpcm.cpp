/* FLAC input plugin for Winamp3
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
 *
 * NOTE: this code is derived from the 'rawpcm' example by
 * Nullsoft; the original license for the 'rawpcm' example follows.
 */
/*

  Nullsoft WASABI Source File License

  Copyright 1999-2001 Nullsoft, Inc.

    This software is provided 'as-is', without any express or implied
    warranty.  In no event will the authors be held liable for any damages
    arising from the use of this software.

    Permission is granted to anyone to use this software for any purpose,
    including commercial applications, and to alter it and redistribute it
    freely, subject to the following restrictions:

    1. The origin of this software must not be misrepresented; you must not
       claim that you wrote the original software. If you use this software
       in a product, an acknowledgment in the product documentation would be
       appreciated but is not required.
    2. Altered source versions must be plainly marked as such, and must not be
       misrepresented as being the original software.
    3. This notice may not be removed or altered from any source distribution.


  Brennan Underwood
  brennan@nullsoft.com

*/

#include "flacpcm.h"
extern "C" {
#include "FLAC/metadata.h"
};


struct id3v1_struct {
	FLAC__byte raw[128];
	char title[31];
	char artist[31];
	char album[31];
	char comment[31];
	unsigned year;
	unsigned track; /* may be 0 if v1 (not v1.1) tag */
	unsigned genre;
	char description[1024]; /* the formatted description passed to the gui */
};


static bool get_id3v1_tag_(const char *filename, id3v1_struct *tag);


FlacPcm::FlacPcm():
needs_seek(false),
seek_sample(0),
samples_in_reservoir(0),
abort_flag(false),
reader(0),
decoder(0)
{
}

FlacPcm::~FlacPcm()
{
	cleanup();
}

int FlacPcm::getInfos(MediaInfo *infos)
{
	reader = infos->getReader();
	if(!reader) return 0;

	//@@@ to be really "clean" we should go through the reader instead of directly to the file...
	if(!FLAC__metadata_get_streaminfo(infos->getFilename(), &streaminfo))
		return 1;

	id3v1_struct tag;

	bool has_tag = get_id3v1_tag_(infos->getFilename(), &tag);

	infos->setLength(lengthInMsec());
	//@@@ infos->setTitle(Std::filename(infos->getFilename()));
	infos->setTitle(tag.description);
	infos->setInfo(StringPrintf("FLAC:<%ihz:%ibps:%dch>", streaminfo.data.stream_info.sample_rate, streaminfo.data.stream_info.bits_per_sample, streaminfo.data.stream_info.channels)); //@@@ fix later
	if(has_tag) {
		infos->setData("Title", tag.title);
		infos->setData("Artist", tag.artist);
		infos->setData("Album", tag.album);
	}

	return 0;
}

int FlacPcm::processData(MediaInfo *infos, ChunkList *chunk_list, bool *killswitch)
{
	reader = infos->getReader();
	if(reader == 0)
		return 0;

	if(decoder == 0) {
		decoder = FLAC__seekable_stream_decoder_new();
		if(decoder == 0)
			return 0;
		FLAC__seekable_stream_decoder_set_md5_checking(decoder, false);
		FLAC__seekable_stream_decoder_set_read_callback(decoder, readCallback_);
		FLAC__seekable_stream_decoder_set_seek_callback(decoder, seekCallback_);
		FLAC__seekable_stream_decoder_set_tell_callback(decoder, tellCallback_);
		FLAC__seekable_stream_decoder_set_length_callback(decoder, lengthCallback_);
		FLAC__seekable_stream_decoder_set_eof_callback(decoder, eofCallback_);
		FLAC__seekable_stream_decoder_set_write_callback(decoder, writeCallback_);
		FLAC__seekable_stream_decoder_set_metadata_callback(decoder, metadataCallback_);
		FLAC__seekable_stream_decoder_set_error_callback(decoder, errorCallback_);
		FLAC__seekable_stream_decoder_set_client_data(decoder, this);

		if(FLAC__seekable_stream_decoder_init(decoder) != FLAC__SEEKABLE_STREAM_DECODER_OK) {
			cleanup();
			return 0;
		}
		if(!FLAC__seekable_stream_decoder_process_until_end_of_metadata(decoder)) {
			cleanup();
			return 0;
		}
	}

	if(needs_seek) {
		FLAC__seekable_stream_decoder_seek_absolute(decoder, seek_sample);
		needs_seek = false;
	}

	bool eof = false;

	while(samples_in_reservoir < 576) {
		if(FLAC__seekable_stream_decoder_get_state(decoder) == FLAC__SEEKABLE_STREAM_DECODER_END_OF_STREAM) {
			eof = true;
			break;
		}
		else if(!FLAC__seekable_stream_decoder_process_single(decoder)) {
			//@@@ how to do this?  MessageBox(mod_.hMainWindow, FLAC__FileDecoderStateString[FLAC__file_decoder_get_state(decoder_)], "READ ERROR processing frame", 0);
			eof = true;
			break;
		}
	}

	if(samples_in_reservoir == 0) {
		eof = true;
	}
	else {
		const unsigned channels = streaminfo.data.stream_info.channels;
		const unsigned bits_per_sample = streaminfo.data.stream_info.bits_per_sample;
		const unsigned bytes_per_sample = (bits_per_sample+7)/8;
		const unsigned sample_rate = streaminfo.data.stream_info.sample_rate;
		unsigned i, n = min(samples_in_reservoir, 576), delta;
		signed short *ssbuffer = (signed short*)output;

		for(i = 0; i < n*channels; i++)
			ssbuffer[i] = reservoir[i];
		delta = i;
		for( ; i < samples_in_reservoir*channels; i++)
			reservoir[i-delta] = reservoir[i];
		samples_in_reservoir -= n;

		const int bytes = n * channels * bytes_per_sample;

		ChunkInfosI *ci=new ChunkInfosI();
		ci->addInfo("srate", sample_rate);
		ci->addInfo("bps", bits_per_sample);
		ci->addInfo("nch", channels);

		chunk_list->setChunk("PCM", output, bytes, ci);
	}

	if(eof)
		return 0;

	return 1;
}

int FlacPcm::corecb_onSeeked(int newpos)
{
	if(streaminfo.data.stream_info.total_samples == 0 || newpos < 0)
		return 1;

	needs_seek = true;
	seek_sample = (FLAC__uint64)((double)newpos / (double)lengthInMsec() * (double)(FLAC__int64)streaminfo.data.stream_info.total_samples);
	return 0;
}

void FlacPcm::cleanup()
{
	if(decoder) {
		FLAC__seekable_stream_decoder_finish(decoder);
		FLAC__seekable_stream_decoder_delete(decoder);
		decoder = 0;
	}
}
FLAC__SeekableStreamDecoderReadStatus FlacPcm::readCallback_(const FLAC__SeekableStreamDecoder *decoder, FLAC__byte buffer[], unsigned *bytes, void *client_data)
{
	FlacPcm *instance = (FlacPcm*)client_data;
	*bytes = instance->reader->read((char*)buffer, *bytes);
	return FLAC__SEEKABLE_STREAM_DECODER_READ_STATUS_OK;
}

FLAC__SeekableStreamDecoderSeekStatus FlacPcm::seekCallback_(const FLAC__SeekableStreamDecoder *decoder, FLAC__uint64 absolute_byte_offset, void *client_data)
{
	FlacPcm *instance = (FlacPcm*)client_data;
	if(instance->reader->seek((int)absolute_byte_offset) < 0)
		return FLAC__SEEKABLE_STREAM_DECODER_SEEK_STATUS_ERROR;
	else
		return FLAC__SEEKABLE_STREAM_DECODER_SEEK_STATUS_OK;
}

FLAC__SeekableStreamDecoderTellStatus FlacPcm::tellCallback_(const FLAC__SeekableStreamDecoder *decoder, FLAC__uint64 *absolute_byte_offset, void *client_data)
{
	FlacPcm *instance = (FlacPcm*)client_data;
	int pos = instance->reader->getPos();
	if(pos < 0)
		return FLAC__SEEKABLE_STREAM_DECODER_TELL_STATUS_ERROR;
	else {
		*absolute_byte_offset = pos;
		return FLAC__SEEKABLE_STREAM_DECODER_TELL_STATUS_OK;
	}
}

FLAC__SeekableStreamDecoderLengthStatus FlacPcm::lengthCallback_(const FLAC__SeekableStreamDecoder *decoder, FLAC__uint64 *stream_length, void *client_data)
{
	FlacPcm *instance = (FlacPcm*)client_data;
	int length = instance->reader->getPos();
	if(length < 0)
		return FLAC__SEEKABLE_STREAM_DECODER_LENGTH_STATUS_ERROR;
	else {
		*stream_length = length;
		return FLAC__SEEKABLE_STREAM_DECODER_LENGTH_STATUS_OK;
	}
}

FLAC__bool FlacPcm::eofCallback_(const FLAC__SeekableStreamDecoder *decoder, void *client_data)
{
	FlacPcm *instance = (FlacPcm*)client_data;
	return instance->reader->getPos() == instance->reader->getLength();
}

FLAC__StreamDecoderWriteStatus FlacPcm::writeCallback_(const FLAC__SeekableStreamDecoder *decoder, const FLAC__Frame *frame, const FLAC__int32 * const buffer[], void *client_data)
{
	FlacPcm *instance = (FlacPcm*)client_data;
	const unsigned bps = instance->streaminfo.data.stream_info.bits_per_sample, channels = instance->streaminfo.data.stream_info.channels, wide_samples = frame->header.blocksize;
	unsigned wide_sample, sample, channel;

	(void)decoder;

	if(instance->abort_flag)
		return FLAC__STREAM_DECODER_WRITE_STATUS_ABORT;

	for(sample = instance->samples_in_reservoir*channels, wide_sample = 0; wide_sample < wide_samples; wide_sample++)
		for(channel = 0; channel < channels; channel++, sample++)
			instance->reservoir[sample] = (FLAC__int16)buffer[channel][wide_sample];

	instance->samples_in_reservoir += wide_samples;

	return FLAC__STREAM_DECODER_WRITE_STATUS_CONTINUE;
}

void FlacPcm::metadataCallback_(const FLAC__SeekableStreamDecoder *decoder, const FLAC__StreamMetadata *metadata, void *client_data)
{
	FlacPcm *instance = (FlacPcm*)client_data;
	(void)decoder;
	if(metadata->type == FLAC__METADATA_TYPE_STREAMINFO) {
		instance->streaminfo = *metadata;

		if(instance->streaminfo.data.stream_info.bits_per_sample != 16) {
			//@@@ how to do this?  MessageBox(mod.hMainWindow, "ERROR: plugin can only handle 16-bit samples\n", "ERROR: plugin can only handle 16-bit samples", 0);
			instance->abort_flag = true;
			return;
		}
	}
}

void FlacPcm::errorCallback_(const FLAC__SeekableStreamDecoder *decoder, FLAC__StreamDecoderErrorStatus status, void *client_data)
{
	FlacPcm *instance = (FlacPcm*)client_data;
	(void)decoder;
	if(status != FLAC__STREAM_DECODER_ERROR_STATUS_LOST_SYNC)
		instance->abort_flag = true;
}

/***********************************************************************
 * local routines
 **********************************************************************/

bool get_id3v1_tag_(const char *filename, id3v1_struct *tag)
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
	if(strncmp((const char*)tag->raw, "TAG", 3))
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
