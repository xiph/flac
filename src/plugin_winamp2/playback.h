/* in_flac - Winamp2 FLAC input plugin
 * Copyright (C) 2000,2001,2002,2003,2004,2005,2006  Josh Coalson
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

#include "FLAC/all.h"
#include "share/replaygain_synthesis.h"
#include "plugin_common/all.h"

/*
 *  constants
 */

#define SAMPLES_PER_WRITE           576

#define BITRATE_HIST_SEGMENT_MSEC   500
#define BITRATE_HIST_SIZE           64

/*
 *  common structures
 */

typedef struct {
	volatile FLAC__bool is_playing;
	volatile FLAC__bool abort_flag;
	volatile FLAC__bool eof;
	volatile int seek_to;
	unsigned total_samples;
	unsigned bits_per_sample;
	unsigned output_bits_per_sample;
	unsigned channels;
	unsigned sample_rate;
	unsigned length_in_msec;
	unsigned average_bps;
	FLAC__bool has_replaygain;
	double replay_scale;
	DitherContext dither_context;
} file_info_struct;


typedef struct {
	struct {
		FLAC__bool enable;
		FLAC__bool album_mode;
		int  preamp;
		FLAC__bool hard_limit;
	} replaygain;
	struct {
		struct {
			FLAC__bool dither_24_to_16;
		} normal;
		struct {
			FLAC__bool dither;
			int  noise_shaping; /* value must be one of NoiseShaping enum, see plugin_common/replaygain_synthesis.h */
			int  bps_out;
		} replaygain;
	} resolution;
	struct {
		FLAC__bool stop_err;
	} misc;
} output_config_t;

/*
 *  protopytes
 */

FLAC__bool FLAC_plugin__decoder_init(FLAC__FileDecoder *decoder, const char *filename, FLAC__int64 filesize, file_info_struct *file_info, output_config_t *config);
void FLAC_plugin__decoder_finish(FLAC__FileDecoder *decoder);
void FLAC_plugin__decoder_delete(FLAC__FileDecoder *decoder);

int FLAC_plugin__seek(FLAC__FileDecoder *decoder, file_info_struct *file_info);
unsigned FLAC_plugin__decode(FLAC__FileDecoder *decoder, file_info_struct *file_info, char *sample_buffer);
int FLAC_plugin__get_rate(unsigned written_time, unsigned output_time, file_info_struct *file_info);

/*
 *  these should be defined in plug-in
 */

extern void FLAC_plugin__show_error(const char *message,...);
