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

#ifndef _FLACPCM_H
#define _FLACPCM_H

#include "studio/services/svc_mediaconverter.h"
#include "studio/services/servicei.h"
#include "studio/corecb.h"
#include "studio/wac.h"
#include "attribs/cfgitemi.h"
#include "attribs/attrint.h"
extern "C" {
#include "FLAC/seekable_stream_decoder.h"
};

class FlacPcm : public svc_mediaConverterI {
public:
	FlacPcm();
	virtual ~FlacPcm();

	// service
	static const char *getServiceName() { return "FLAC to PCM converter service"; }

	virtual int canConvertFrom(svc_fileReader *reader, const char *name, const char *chunktype)
	{ return name && !STRICMP(Std::extension(name), "flac"); } // only accepts *.flac files

	virtual const char *getConverterTo() { return "PCM"; }

	virtual int getInfos(MediaInfo *infos);

	virtual int processData(MediaInfo *infos, ChunkList *chunk_list, bool *killswitch);

	virtual int getLatency(void) { return 0; }

	// callbacks
	virtual int corecb_onSeeked(int newpos);

protected:
	bool needs_seek;
	FLAC__uint64 seek_sample;
	unsigned samples_in_reservoir;
	bool abort_flag;
	svc_fileReader *reader;
	FLAC__SeekableStreamDecoder *decoder;
	FLAC__StreamMetadata streaminfo;
	FLAC__int16 reservoir[FLAC__MAX_BLOCK_SIZE * 2 * 2]; // *2 for max channels, another *2 for overflow
	unsigned char output[576 * 2 * (16/8)]; // *2 for max channels, (16/8) for max bytes per sample

	unsigned lengthInMsec() { return (unsigned)((FLAC__uint64)1000 * streaminfo.data.stream_info.total_samples / streaminfo.data.stream_info.sample_rate); }
private:
	void cleanup();
	static FLAC__SeekableStreamDecoderReadStatus readCallback_(const FLAC__SeekableStreamDecoder *decoder, FLAC__byte buffer[], unsigned *bytes, void *client_data);
	static FLAC__SeekableStreamDecoderSeekStatus seekCallback_(const FLAC__SeekableStreamDecoder *decoder, FLAC__uint64 absolute_byte_offset, void *client_data);
	static FLAC__SeekableStreamDecoderTellStatus tellCallback_(const FLAC__SeekableStreamDecoder *decoder, FLAC__uint64 *absolute_byte_offset, void *client_data);
	static FLAC__SeekableStreamDecoderLengthStatus lengthCallback_(const FLAC__SeekableStreamDecoder *decoder, FLAC__uint64 *stream_length, void *client_data);
	static FLAC__bool eofCallback_(const FLAC__SeekableStreamDecoder *decoder, void *client_data);
	static FLAC__StreamDecoderWriteStatus writeCallback_(const FLAC__SeekableStreamDecoder *decoder, const FLAC__Frame *frame, const FLAC__int32 *buffer[], void *client_data);
	static void metadataCallback_(const FLAC__SeekableStreamDecoder *decoder, const FLAC__StreamMetadata *metadata, void *client_data);
	static void errorCallback_(const FLAC__SeekableStreamDecoder *decoder, FLAC__StreamDecoderErrorStatus status, void *client_data);
};
#endif
