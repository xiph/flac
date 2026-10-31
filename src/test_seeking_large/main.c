/* test_seeking_large - Seeking tester for libFLAC using large generated streams
 * Copyright (C) 2026  Xiph.Org Foundation
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
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

/*
 * Encodes large streams from deterministic signals, seeks around in them and
 * checks every decoded sample against the signal.
 *
 * The streams cover what seeking has to cope with: streams with and without
 * seek table, of unknown length, with a short last frame, with very small
 * frames, multichannel high resolution audio, quiet passages (whose Rice
 * coded residuals are full of 0xff bytes, which start a frame sync code) and
 * audio data containing fake frame headers with a valid CRC-8, some of them
 * in pairs that agree with each other.  Copies of some streams get damaged
 * frames, seeking in those must never deliver wrong samples.
 *
 * The time spent in FLAC__stream_decoder_seek_absolute() is reported as
 * well, so this doubles as a seeking benchmark.
 *
 * usage: test_seeking_large [--minutes N] [--seeks N] [--stream NAME] [--keep]
 *   --minutes N    length of the long streams, default 10
 *   --seeks N      number of random seeks per test, default 1000
 *   --stream NAME  only test the named stream, can be given more than once
 *   --keep         keep the generated files
 */

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef _WIN32
#include <windows.h>
#else
#include <sys/time.h>
#endif
#include "FLAC/metadata.h"
#include "FLAC/stream_decoder.h"
#include "FLAC/stream_encoder.h"
#include "share/compat.h"

typedef enum {
	SIGNAL_MUSIC, /* tones and noise of varying levels, including silence and very quiet passages */
	SIGNAL_DECOY /* full scale noise containing fake frame headers; gets encoded as verbatim subframes */
} SignalType;

typedef struct {
	const char *name;
	SignalType signal;
	uint32_t channels;
	uint32_t bits_per_sample;
	uint32_t sample_rate;
	uint32_t blocksize;
	uint32_t compression_level;
	uint32_t length_percent; /* stream length in percent of --minutes */
	FLAC__bool seek_table;
	FLAC__bool unknown_length; /* encode to an unseekable output, so STREAMINFO lacks the total samples */
	FLAC__bool damage; /* also test a copy with damaged frames */
} StreamConfig;

static const StreamConfig stream_configs_[] = {
	/* name                       signal        ch bps  rate   block level len% seektable unknown damage */
	{ "stereo16",                 SIGNAL_MUSIC,  2, 16, 44100, 4096, 5, 100, true,  false, true  },
	{ "stereo16-no-seektable",    SIGNAL_MUSIC,  2, 16, 44100, 4096, 5, 100, false, false, true  },
	{ "stereo16-unknown-length",  SIGNAL_MUSIC,  2, 16, 48000, 4608, 8,  50, false, true,  false },
	{ "surround24-96k",           SIGNAL_MUSIC,  6, 24, 96000, 4096, 5,  20, true,  false, false },
	{ "mono8-small-blocks",       SIGNAL_MUSIC,  1,  8, 22050,  576, 3, 100, false, false, false },
	{ "decoy",                    SIGNAL_DECOY,  1, 16, 44100, 4096, 0,  50, false, false, false },
	{ "decoy-seektable",          SIGNAL_DECOY,  1, 16, 44100, 4096, 0,  50, true,  false, false }
};

/* fake frame headers in decoy streams claim one of these sample rates and bits per sample codes */
#define DECOY_SAMPLE_RATE_CODE 9 /* 44100 Hz */
#define DECOY_BPS_CODE 4 /* 16 bits */

typedef struct {
	const StreamConfig *config;
	FLAC__uint64 total_samples;
	FLAC__uint64 frame; /* index of the frame in samples, or -1 */
	FLAC__int32 *samples; /* interleaved */
	FLAC__uint64 decoy_count; /* fake frame headers in the frames generated so far */
} Signal;

typedef struct {
	FILE *file;
	FLAC__uint64 file_size;
	Signal *signal;
	/* the last block passed to the write callback */
	FLAC__int32 *block; /* interleaved */
	FLAC__uint64 block_sample;
	uint32_t block_size;
	FLAC__bool got_block;
	/* frame end positions, recorded while decoding sequentially */
	FLAC__uint64 *frame_ends;
	FLAC__uint64 frame_count;
	FLAC__bool record_frame_ends;
	/* I/O statistics */
	FLAC__uint64 reads, bytes_read, seeks;
	FLAC__uint64 max_seeks, max_seeks_target; /* the most seek callbacks a single seek took */
} DecoderClient;

static uint32_t minutes_ = 10;
static uint32_t seek_count_ = 1000;
static FLAC__bool keep_files_ = false;
static const char *selected_streams_[16];
static uint32_t selected_stream_count_ = 0;

static double now_(void)
{
#ifdef _WIN32
	LARGE_INTEGER count, frequency;
	QueryPerformanceCounter(&count);
	QueryPerformanceFrequency(&frequency);
	return (double)count.QuadPart / (double)frequency.QuadPart;
#else
	struct timeval tv;
	gettimeofday(&tv, NULL);
	return (double)tv.tv_sec + (double)tv.tv_usec * 1e-6;
#endif
}

/*
 * signal generation
 */

static FLAC__uint64 hash_(FLAC__uint64 x)
{
	/* splitmix64 */
	x += FLAC__U64L(0x9e3779b97f4a7c15);
	x = (x ^ (x >> 30)) * FLAC__U64L(0xbf58476d1ce4e5b9);
	x = (x ^ (x >> 27)) * FLAC__U64L(0x94d049bb133111eb);
	return x ^ (x >> 31);
}

static FLAC__uint64 random_(FLAC__uint64 *state)
{
	*state += 1;
	return hash_(*state);
}

static FLAC__int32 music_sample_(const StreamConfig *config, uint32_t channel, FLAC__uint64 n)
{
	static const uint32_t noise_bits_table[8] = { 0, 0, 1, 2, 4, 8, 12, 32 };
	/* the character of the signal changes every two seconds */
	const FLAC__uint64 section = hash_(n / (2 * config->sample_rate));
	const FLAC__int64 max = ((FLAC__int64)1 << (config->bits_per_sample - 1)) - 1;
	const FLAC__int64 period = 40 + (FLAC__int64)((section >> 8) % 400) + 7 * channel;
	const FLAC__int64 amplitude = (section >> 20) % 4 == 0? 0 : max / (FLAC__int64)(2 + (section >> 24) % 6);
	uint32_t noise_bits = noise_bits_table[(section >> 4) % 8];
	FLAC__int64 phase = (FLAC__int64)(n % (FLAC__uint64)period), value;

	/* triangle wave */
	value = (phase < period / 2? phase : period - phase) * 4 - period;
	value = value * amplitude / period;

	if(noise_bits > config->bits_per_sample)
		noise_bits = config->bits_per_sample;
	if(noise_bits > 0) {
		const FLAC__uint64 noise = hash_(n * 8 + channel) & ((FLAC__U64L(1) << noise_bits) - 1);
		value += (FLAC__int64)noise - ((FLAC__int64)1 << (noise_bits - 1));
	}

	if(value > max)
		value = max;
	if(value < -max - 1)
		value = -max - 1;
	return (FLAC__int32)value;
}

static FLAC__byte crc8_(const FLAC__byte *data, uint32_t len)
{
	FLAC__byte crc = 0;
	uint32_t i, j;

	for(i = 0; i < len; i++) {
		crc ^= data[i];
		for(j = 0; j < 8; j++)
			crc = (FLAC__byte)(crc & 0x80? (crc << 1) ^ 0x07 : crc << 1);
	}
	return crc;
}

/* writes a frame header for a frame of a fixed-blocksize stream; returns its length */
static uint32_t make_frame_header_(FLAC__byte *header, uint32_t blocksize_code, uint32_t sample_rate_code, uint32_t channels_code, uint32_t bps_code, uint32_t frame_number)
{
	uint32_t len = 0;

	header[len++] = 0xff;
	header[len++] = 0xf8;
	header[len++] = (FLAC__byte)(blocksize_code << 4 | sample_rate_code);
	header[len++] = (FLAC__byte)(channels_code << 4 | bps_code << 1);
	/* frame number, UTF-8 coded */
	if(frame_number < 0x80)
		header[len++] = (FLAC__byte)frame_number;
	else if(frame_number < 0x800) {
		header[len++] = (FLAC__byte)(0xc0 | frame_number >> 6);
		header[len++] = (FLAC__byte)(0x80 | (frame_number & 0x3f));
	}
	else if(frame_number < 0x10000) {
		header[len++] = (FLAC__byte)(0xe0 | frame_number >> 12);
		header[len++] = (FLAC__byte)(0x80 | (frame_number >> 6 & 0x3f));
		header[len++] = (FLAC__byte)(0x80 | (frame_number & 0x3f));
	}
	else {
		header[len++] = (FLAC__byte)(0xf0 | frame_number >> 18);
		header[len++] = (FLAC__byte)(0x80 | (frame_number >> 12 & 0x3f));
		header[len++] = (FLAC__byte)(0x80 | (frame_number >> 6 & 0x3f));
		header[len++] = (FLAC__byte)(0x80 | (frame_number & 0x3f));
	}
	header[len] = crc8_(header, len);
	return len + 1;
}

/* puts bytes into a mono 16 bit verbatim subframe, two bytes per sample */
static void embed_bytes_(FLAC__int32 *samples, const FLAC__byte *bytes, uint32_t len)
{
	uint32_t i;

	for(i = 0; i < len; i += 2) {
		const uint32_t low = i + 1 < len? bytes[i + 1] : (uint32_t)(samples[i / 2] & 0xff);
		samples[i / 2] = (FLAC__int32)(FLAC__int16)(bytes[i] << 8 | low);
	}
}

/* returns the number of fake frame headers in the frame */
static uint32_t make_decoy_frame_(const Signal *signal, FLAC__uint64 frame, uint32_t frame_samples, FLAC__int32 *samples)
{
	const uint32_t blocksize = signal->config->blocksize;
	/* fake frames must lie within the stream to look plausible */
	const FLAC__uint64 full_frames = signal->total_samples / blocksize;
	const FLAC__uint64 h = hash_(frame ^ FLAC__U64L(0x5eed));
	const uint32_t pos = 16 + (uint32_t)((h >> 8) % (blocksize / 2));
	const uint32_t sample_rate_code = (h >> 32) & 1? DECOY_SAMPLE_RATE_CODE : 0;
	const uint32_t bps_code = (h >> 33) & 1? DECOY_BPS_CODE : 0;
	static const FLAC__byte sync_run[] = { 0xff, 0xff, 0xff, 0xf8, 0xff, 0xf9, 0xff };
	FLAC__byte header[16];
	FLAC__int64 fake_frame[2] = { -1, -1 };
	uint32_t i, count = 0;

	for(i = 0; i < frame_samples; i++)
		samples[i] = (FLAC__int32)(FLAC__int16)(hash_((frame * blocksize + i) ^ FLAC__U64L(0xdec0)) & 0xffff);

	if(frame_samples < blocksize)
		return 0;

	switch(h % 6) {
		case 0:
			break;
		case 1: /* the next frame */
			fake_frame[0] = (FLAC__int64)frame + 1;
			break;
		case 2: /* this frame */
			fake_frame[0] = (FLAC__int64)frame;
			break;
		case 3: /* the previous frame */
			fake_frame[0] = (FLAC__int64)frame - 1;
			break;
		case 4: /* two frames ahead, followed by a fake header for the frame after that */
			fake_frame[0] = (FLAC__int64)frame + 2;
			fake_frame[1] = (FLAC__int64)frame + 3;
			break;
		case 5: /* anywhere */
			if(full_frames > 0)
				fake_frame[0] = (FLAC__int64)((h >> 40) % full_frames);
			break;
	}
	for(i = 0; i < 2; i++) {
		if(fake_frame[i] >= 0 && (FLAC__uint64)fake_frame[i] < full_frames) {
			const uint32_t len = make_frame_header_(header, 12 /* 4096 samples */, sample_rate_code, 0 /* mono */, bps_code, (uint32_t)fake_frame[i]);
			embed_bytes_(samples + pos + 50 * i, header, len);
			count++;
		}
	}

	/* bytes that look like the start of a sync code */
	embed_bytes_(samples + pos + 200, sync_run, sizeof(sync_run));

	return count;
}

static FLAC__bool signal_init_(Signal *signal, const StreamConfig *config, FLAC__uint64 total_samples)
{
	signal->config = config;
	signal->total_samples = total_samples;
	signal->frame = (FLAC__uint64)(-1);
	signal->decoy_count = 0;
	signal->samples = malloc(sizeof(FLAC__int32) * config->blocksize * config->channels);
	return signal->samples != NULL;
}

static void signal_free_(Signal *signal)
{
	free(signal->samples);
	signal->samples = NULL;
}

/* makes signal->samples hold the given frame; returns the number of samples in the frame */
static uint32_t signal_get_frame_(Signal *signal, FLAC__uint64 frame)
{
	const StreamConfig *config = signal->config;
	const FLAC__uint64 first_sample = frame * config->blocksize;
	const uint32_t frame_samples = (uint32_t)(signal->total_samples - first_sample < config->blocksize? signal->total_samples - first_sample : config->blocksize);
	uint32_t i, channel;

	if(signal->frame != frame) {
		signal->frame = frame;
		if(config->signal == SIGNAL_DECOY)
			signal->decoy_count += make_decoy_frame_(signal, frame, frame_samples, signal->samples);
		else
			for(i = 0; i < frame_samples; i++)
				for(channel = 0; channel < config->channels; channel++)
					signal->samples[i * config->channels + channel] = music_sample_(config, channel, first_sample + i);
	}
	return frame_samples;
}

static FLAC__int32 signal_get_sample_(Signal *signal, uint32_t channel, FLAC__uint64 n)
{
	const FLAC__uint64 frame = n / signal->config->blocksize;

	signal_get_frame_(signal, frame);
	return signal->samples[(n - frame * signal->config->blocksize) * signal->config->channels + channel];
}

/*
 * encoding
 */

static FLAC__StreamEncoderWriteStatus encoder_write_callback_(const FLAC__StreamEncoder *encoder, const FLAC__byte buffer[], size_t bytes, uint32_t samples, uint32_t current_frame, void *client_data)
{
	(void)encoder, (void)samples, (void)current_frame;
	return fwrite(buffer, 1, bytes, (FILE*)client_data) == bytes? FLAC__STREAM_ENCODER_WRITE_STATUS_OK : FLAC__STREAM_ENCODER_WRITE_STATUS_FATAL_ERROR;
}

static FLAC__bool encode_(Signal *signal, const char *filename)
{
	const StreamConfig *config = signal->config;
	FLAC__StreamEncoder *encoder = FLAC__stream_encoder_new();
	FLAC__StreamMetadata *seek_table = NULL;
	FLAC__StreamEncoderInitStatus init_status;
	FILE *file = NULL;
	FLAC__uint64 frame;
	FLAC__bool ok = encoder != NULL;

	ok = ok && FLAC__stream_encoder_set_verify(encoder, false);
	ok = ok && FLAC__stream_encoder_set_compression_level(encoder, config->compression_level);
	ok = ok && FLAC__stream_encoder_set_channels(encoder, config->channels);
	ok = ok && FLAC__stream_encoder_set_bits_per_sample(encoder, config->bits_per_sample);
	ok = ok && FLAC__stream_encoder_set_sample_rate(encoder, config->sample_rate);
	ok = ok && FLAC__stream_encoder_set_blocksize(encoder, config->blocksize);
	if(!config->unknown_length)
		ok = ok && FLAC__stream_encoder_set_total_samples_estimate(encoder, signal->total_samples);
	if(config->seek_table) {
		ok = ok && (seek_table = FLAC__metadata_object_new(FLAC__METADATA_TYPE_SEEKTABLE)) != NULL;
		ok = ok && FLAC__metadata_object_seektable_template_append_spaced_points_by_samples(seek_table, config->sample_rate * 10, signal->total_samples);
		ok = ok && FLAC__metadata_object_seektable_template_sort(seek_table, /*compact=*/true);
		ok = ok && FLAC__stream_encoder_set_metadata(encoder, &seek_table, 1);
	}
	if(!ok) {
		printf("FAILED setting up the encoder\n");
		goto done;
	}

	if(config->unknown_length) {
		/* without seek callback, the encoder can't update STREAMINFO when done */
		if((file = flac_fopen(filename, "wb")) == NULL) {
			printf("FAILED opening %s\n", filename);
			ok = false;
			goto done;
		}
		init_status = FLAC__stream_encoder_init_stream(encoder, encoder_write_callback_, NULL, NULL, NULL, file);
	}
	else
		init_status = FLAC__stream_encoder_init_file(encoder, filename, NULL, NULL);
	if(init_status != FLAC__STREAM_ENCODER_INIT_STATUS_OK) {
		printf("FAILED initializing the encoder: %s\n", FLAC__StreamEncoderInitStatusString[init_status]);
		ok = false;
		goto done;
	}

	for(frame = 0; ok && frame * config->blocksize < signal->total_samples; frame++) {
		const uint32_t frame_samples = signal_get_frame_(signal, frame);
		ok = FLAC__stream_encoder_process_interleaved(encoder, signal->samples, frame_samples);
	}
	ok = FLAC__stream_encoder_finish(encoder) && ok;
	if(!ok)
		printf("FAILED encoding, state = %s\n", FLAC__stream_encoder_get_resolved_state_string(encoder));

done:
	if(encoder)
		FLAC__stream_encoder_delete(encoder);
	if(seek_table)
		FLAC__metadata_object_delete(seek_table);
	if(file)
		fclose(file);
	return ok;
}

/*
 * decoding
 */

static FLAC__StreamDecoderReadStatus decoder_read_callback_(const FLAC__StreamDecoder *decoder, FLAC__byte buffer[], size_t *bytes, void *client_data)
{
	DecoderClient *client = (DecoderClient*)client_data;
	(void)decoder;

	*bytes = fread(buffer, 1, *bytes, client->file);
	client->reads++;
	client->bytes_read += *bytes;
	if(ferror(client->file))
		return FLAC__STREAM_DECODER_READ_STATUS_ABORT;
	return *bytes == 0? FLAC__STREAM_DECODER_READ_STATUS_END_OF_STREAM : FLAC__STREAM_DECODER_READ_STATUS_CONTINUE;
}

static FLAC__StreamDecoderSeekStatus decoder_seek_callback_(const FLAC__StreamDecoder *decoder, FLAC__uint64 absolute_byte_offset, void *client_data)
{
	DecoderClient *client = (DecoderClient*)client_data;
	(void)decoder;

	client->seeks++;
	return fseeko(client->file, (FLAC__off_t)absolute_byte_offset, SEEK_SET) == 0? FLAC__STREAM_DECODER_SEEK_STATUS_OK : FLAC__STREAM_DECODER_SEEK_STATUS_ERROR;
}

static FLAC__StreamDecoderTellStatus decoder_tell_callback_(const FLAC__StreamDecoder *decoder, FLAC__uint64 *absolute_byte_offset, void *client_data)
{
	DecoderClient *client = (DecoderClient*)client_data;
	const FLAC__off_t pos = ftello(client->file);
	(void)decoder;

	if(pos < 0)
		return FLAC__STREAM_DECODER_TELL_STATUS_ERROR;
	*absolute_byte_offset = (FLAC__uint64)pos;
	return FLAC__STREAM_DECODER_TELL_STATUS_OK;
}

static FLAC__StreamDecoderLengthStatus decoder_length_callback_(const FLAC__StreamDecoder *decoder, FLAC__uint64 *stream_length, void *client_data)
{
	(void)decoder;
	*stream_length = ((DecoderClient*)client_data)->file_size;
	return FLAC__STREAM_DECODER_LENGTH_STATUS_OK;
}

static FLAC__bool decoder_eof_callback_(const FLAC__StreamDecoder *decoder, void *client_data)
{
	(void)decoder;
	return feof(((DecoderClient*)client_data)->file)? true : false;
}

static FLAC__StreamDecoderWriteStatus decoder_write_callback_(const FLAC__StreamDecoder *decoder, const FLAC__Frame *frame, const FLAC__int32 * const buffer[], void *client_data)
{
	DecoderClient *client = (DecoderClient*)client_data;
	const uint32_t channels = client->signal->config->channels;
	uint32_t i, channel;

	/* only copy the samples here, checking them would distort the time measurement */
	if(frame->header.channels != channels || frame->header.blocksize > client->signal->config->blocksize)
		return FLAC__STREAM_DECODER_WRITE_STATUS_ABORT;
	for(i = 0; i < frame->header.blocksize; i++)
		for(channel = 0; channel < channels; channel++)
			client->block[i * channels + channel] = buffer[channel][i];
	client->block_sample = frame->header.number.sample_number;
	client->block_size = frame->header.blocksize;
	client->got_block = true;

	if(client->record_frame_ends) {
		FLAC__uint64 position;
		if(!FLAC__stream_decoder_get_decode_position(decoder, &position))
			return FLAC__STREAM_DECODER_WRITE_STATUS_ABORT;
		client->frame_ends[client->frame_count++] = position;
	}
	return FLAC__STREAM_DECODER_WRITE_STATUS_CONTINUE;
}

static void decoder_error_callback_(const FLAC__StreamDecoder *decoder, FLAC__StreamDecoderErrorStatus status, void *client_data)
{
	(void)decoder, (void)status, (void)client_data;
}

static FLAC__bool decoder_open_(FLAC__StreamDecoder **decoder, DecoderClient *client, Signal *signal, const char *filename)
{
	FLAC__StreamDecoderInitStatus init_status;
	const uint32_t frames = (uint32_t)((signal->total_samples + signal->config->blocksize - 1) / signal->config->blocksize);

	memset(client, 0, sizeof(*client));
	client->signal = signal;
	if((client->file = flac_fopen(filename, "rb")) == NULL) {
		printf("FAILED opening %s\n", filename);
		return false;
	}
	if(fseeko(client->file, 0, SEEK_END) != 0 || (client->file_size = (FLAC__uint64)ftello(client->file)) == 0 || fseeko(client->file, 0, SEEK_SET) != 0) {
		printf("FAILED getting the size of %s\n", filename);
		return false;
	}
	client->block = malloc(sizeof(FLAC__int32) * signal->config->blocksize * signal->config->channels);
	client->frame_ends = malloc(sizeof(FLAC__uint64) * frames);
	if((*decoder = FLAC__stream_decoder_new()) == NULL || client->block == NULL || client->frame_ends == NULL) {
		printf("FAILED allocating memory\n");
		return false;
	}
	init_status = FLAC__stream_decoder_init_stream(*decoder, decoder_read_callback_, decoder_seek_callback_, decoder_tell_callback_, decoder_length_callback_, decoder_eof_callback_, decoder_write_callback_, NULL, decoder_error_callback_, client);
	if(init_status != FLAC__STREAM_DECODER_INIT_STATUS_OK) {
		printf("FAILED initializing the decoder: %s\n", FLAC__StreamDecoderInitStatusString[init_status]);
		return false;
	}
	if(!FLAC__stream_decoder_process_until_end_of_metadata(*decoder)) {
		printf("FAILED reading metadata, state = %s\n", FLAC__stream_decoder_get_resolved_state_string(*decoder));
		return false;
	}
	return true;
}

static void decoder_close_(FLAC__StreamDecoder *decoder, DecoderClient *client)
{
	if(decoder) {
		FLAC__stream_decoder_finish(decoder);
		FLAC__stream_decoder_delete(decoder);
	}
	if(client->file)
		fclose(client->file);
	free(client->block);
	free(client->frame_ends);
}

/* checks the last block written, which must start at the given sample */
static FLAC__bool check_block_(DecoderClient *client, FLAC__uint64 sample)
{
	const uint32_t channels = client->signal->config->channels;
	uint32_t i, channel;

	if(!client->got_block) {
		printf("FAILED, no samples written for sample %" PRIu64 "\n", sample);
		return false;
	}
	if(client->block_sample != sample) {
		printf("FAILED, expected samples from %" PRIu64 " on, got %" PRIu64 "\n", sample, client->block_sample);
		return false;
	}
	if(client->block_sample + client->block_size > client->signal->total_samples) {
		printf("FAILED, got samples past the end of the stream at %" PRIu64 "\n", client->block_sample);
		return false;
	}
	for(i = 0; i < client->block_size; i++) {
		for(channel = 0; channel < channels; channel++) {
			const FLAC__int32 expected = signal_get_sample_(client->signal, channel, sample + i);
			if(client->block[i * channels + channel] != expected) {
				printf("FAILED, sample %" PRIu64 " channel %u is %d, expected %d\n", sample + i, channel, client->block[i * channels + channel], expected);
				return false;
			}
		}
	}
	return true;
}

/* seeks to target and checks the samples written, then decodes and checks some more frames */
static FLAC__bool check_seek_(FLAC__StreamDecoder *decoder, DecoderClient *client, FLAC__uint64 target, uint32_t frames_after, double *seek_time)
{
	const double start = now_();
	const FLAC__uint64 seeks = client->seeks;
	FLAC__uint64 next_sample;
	uint32_t i;

	client->got_block = false;
	if(!FLAC__stream_decoder_seek_absolute(decoder, target)) {
		printf("FAILED seeking to sample %" PRIu64 ", state = %s\n", target, FLAC__stream_decoder_get_resolved_state_string(decoder));
		return false;
	}
	if(seek_time)
		*seek_time += now_() - start;
	if(client->seeks - seeks > client->max_seeks) {
		client->max_seeks = client->seeks - seeks;
		client->max_seeks_target = target;
	}
	if(!check_block_(client, target))
		return false;

	next_sample = client->block_sample + client->block_size;
	for(i = 0; i < frames_after && next_sample < client->signal->total_samples; i++) {
		client->got_block = false;
		if(!FLAC__stream_decoder_process_single(decoder)) {
			printf("FAILED decoding after seeking to sample %" PRIu64 ", state = %s\n", target, FLAC__stream_decoder_get_resolved_state_string(decoder));
			return false;
		}
		if(!check_block_(client, next_sample))
			return false;
		next_sample = client->block_sample + client->block_size;
	}
	return true;
}

static void print_stats_(const DecoderClient *client, uint32_t seeks, double seek_time)
{
	printf("%u seeks, %.1f us/seek, %.2f seek calls/seek (at most %" PRIu64 ", seeking to %" PRIu64 "), %.1f kB read/seek\n",
		seeks,
		seek_time * 1e6 / seeks,
		(double)client->seeks / seeks,
		client->max_seeks,
		client->max_seeks_target,
		(double)client->bytes_read / 1024.0 / seeks
	);
}

/* decodes the whole stream and checks all samples; records where frames end for the damage test */
static FLAC__bool test_sequential_(Signal *signal, const char *filename, FLAC__uint64 **frame_ends, FLAC__uint64 *first_frame_offset)
{
	FLAC__StreamDecoder *decoder = NULL;
	DecoderClient client;
	FLAC__uint64 next_sample = 0;
	FLAC__bool ok;

	printf("  decoding sequentially ... ");
	fflush(stdout);

	ok = decoder_open_(&decoder, &client, signal, filename);
	if(ok && !FLAC__stream_decoder_get_decode_position(decoder, first_frame_offset)) {
		printf("FAILED getting the decode position\n");
		ok = false;
	}
	client.record_frame_ends = true;
	while(ok && next_sample < signal->total_samples) {
		client.got_block = false;
		if(!FLAC__stream_decoder_process_single(decoder)) {
			printf("FAILED decoding, state = %s\n", FLAC__stream_decoder_get_resolved_state_string(decoder));
			ok = false;
		}
		else if((ok = check_block_(&client, next_sample)))
			next_sample = client.block_sample + client.block_size;
	}
	if(ok && (!FLAC__stream_decoder_process_single(decoder) || FLAC__stream_decoder_get_state(decoder) != FLAC__STREAM_DECODER_END_OF_STREAM)) {
		printf("FAILED, expected end of stream after %" PRIu64 " samples, state = %s\n", next_sample, FLAC__stream_decoder_get_resolved_state_string(decoder));
		ok = false;
	}
	if(ok) {
		printf("OK (%" PRIu64 " frames)\n", client.frame_count);
		*frame_ends = client.frame_ends;
		client.frame_ends = NULL;
	}

	decoder_close_(decoder, &client);
	return ok;
}

/* seeks to the start, end and frame boundaries */
static FLAC__bool test_edge_cases_(Signal *signal, const char *filename)
{
	const FLAC__uint64 total_samples = signal->total_samples;
	const FLAC__uint64 blocksize = signal->config->blocksize;
	const FLAC__uint64 last_frame = (total_samples - 1) / blocksize * blocksize;
	const FLAC__uint64 targets[] = {
		0, 1, blocksize - 1, blocksize, blocksize + 1, 2 * blocksize - 1, 2 * blocksize,
		total_samples / 2, total_samples / 2 / blocksize * blocksize, total_samples / 2 / blocksize * blocksize - 1,
		total_samples / 3, total_samples * 2 / 3,
		last_frame - blocksize - 1, last_frame - blocksize, last_frame - 1, last_frame, last_frame + 1,
		total_samples - 2, total_samples - 1,
		/* again, from somewhere else than the start */
		total_samples / 2, 0, total_samples - 1, last_frame - 1, 0
	};
	const FLAC__uint64 past_end[] = { total_samples, total_samples + 1, total_samples + 10 * blocksize, total_samples * 2 };
	FLAC__StreamDecoder *decoder = NULL;
	DecoderClient client;
	uint32_t i;
	FLAC__bool ok;

	printf("  seeking to edge cases ... ");
	fflush(stdout);

	ok = decoder_open_(&decoder, &client, signal, filename);
	for(i = 0; ok && i < sizeof(targets) / sizeof(targets[0]); i++)
		ok = check_seek_(decoder, &client, targets[i], 2, NULL);

	/* seek after having decoded to the end of the stream */
	if(ok) {
		while(ok && FLAC__stream_decoder_get_state(decoder) != FLAC__STREAM_DECODER_END_OF_STREAM)
			ok = FLAC__stream_decoder_process_single(decoder);
		ok = ok && check_seek_(decoder, &client, total_samples / 4, 1, NULL);
		if(!ok)
			printf("FAILED seeking after the end of the stream\n");
	}

	/* seeking past the end must fail; with unknown stream length, only the search can tell */
	for(i = 0; ok && i < sizeof(past_end) / sizeof(past_end[0]); i++) {
		if(FLAC__stream_decoder_seek_absolute(decoder, past_end[i])) {
			printf("FAILED, seeking past the end to sample %" PRIu64 " succeeded\n", past_end[i]);
			ok = false;
		}
		else if(!FLAC__stream_decoder_flush(decoder)) {
			printf("FAILED flushing the decoder\n");
			ok = false;
		}
	}
	ok = ok && check_seek_(decoder, &client, total_samples / 3, 1, NULL);

	if(ok)
		printf("OK (%u seeks)\n", (uint32_t)(sizeof(targets) / sizeof(targets[0]) + 1 + sizeof(past_end) / sizeof(past_end[0]) + 1));

	decoder_close_(decoder, &client);
	return ok;
}

/* seeks to random positions */
static FLAC__bool test_random_(Signal *signal, const char *filename, FLAC__uint64 seed)
{
	FLAC__StreamDecoder *decoder = NULL;
	DecoderClient client;
	FLAC__uint64 random_state = seed;
	double seek_time = 0.0;
	uint32_t i;
	FLAC__bool ok;

	printf("  seeking randomly ... ");
	fflush(stdout);

	ok = decoder_open_(&decoder, &client, signal, filename);
	client.reads = client.bytes_read = client.seeks = 0;
	for(i = 0; ok && i < seek_count_; i++)
		ok = check_seek_(decoder, &client, random_(&random_state) % signal->total_samples, 0, &seek_time);
	if(ok) {
		printf("OK, ");
		print_stats_(&client, seek_count_, seek_time);
	}

	decoder_close_(decoder, &client);
	return ok;
}

/* seeks back and forth by small distances, decoding a little after each seek, like an audio editor does */
static FLAC__bool test_scrubbing_(Signal *signal, const char *filename, FLAC__uint64 seed)
{
	const FLAC__int64 blocksize = signal->config->blocksize;
	FLAC__StreamDecoder *decoder = NULL;
	DecoderClient client;
	FLAC__uint64 random_state = seed;
	FLAC__int64 position = (FLAC__int64)(random_(&random_state) % signal->total_samples);
	double seek_time = 0.0;
	uint32_t i;
	FLAC__bool ok;

	printf("  scrubbing ... ");
	fflush(stdout);

	ok = decoder_open_(&decoder, &client, signal, filename);
	client.reads = client.bytes_read = client.seeks = 0;
	for(i = 0; ok && i < seek_count_; i++) {
		const FLAC__uint64 r = random_(&random_state);
		FLAC__int64 step;
		switch(r % 4) {
			case 0: /* within a frame or so */
				step = (FLAC__int64)((r >> 8) % (2 * (FLAC__uint64)blocksize)) - blocksize;
				break;
			case 1: /* a few frames */
				step = (FLAC__int64)((r >> 8) % (16 * (FLAC__uint64)blocksize)) - 8 * blocksize;
				break;
			case 2: /* up to a few seconds */
				step = (FLAC__int64)((r >> 8) % (8 * (FLAC__uint64)signal->config->sample_rate)) - 4 * (FLAC__int64)signal->config->sample_rate;
				break;
			default: /* play on */
				step = (FLAC__int64)((r >> 8) % (FLAC__uint64)blocksize);
				break;
		}
		position += step;
		if(position < 0)
			position = 0;
		if(position >= (FLAC__int64)signal->total_samples)
			position = (FLAC__int64)signal->total_samples - 1;
		ok = check_seek_(decoder, &client, (FLAC__uint64)position, 1 + (uint32_t)(r >> 60) % 3, &seek_time);
	}
	if(ok) {
		printf("OK, ");
		print_stats_(&client, seek_count_, seek_time);
	}

	decoder_close_(decoder, &client);
	return ok;
}

/* damages frames in a copy of the stream; seeking must still work outside of these, and must never deliver wrong samples */
static FLAC__bool test_damaged_(Signal *signal, const char *filename, const char *damaged_filename, const FLAC__uint64 *frame_ends, FLAC__uint64 first_frame_offset, FLAC__uint64 seed)
{
	const uint32_t damage_count = 50, damage_length = 16;
	const FLAC__uint64 blocksize = signal->config->blocksize;
	const FLAC__uint64 frames = (signal->total_samples + blocksize - 1) / blocksize;
	FLAC__StreamDecoder *decoder = NULL;
	DecoderClient client;
	FLAC__uint64 random_state = seed, file_size, frame;
	FLAC__byte *data = NULL, *damaged = NULL;
	FILE *file;
	uint32_t i, good_seeks = 0, damaged_seeks = 0, damaged_hits = 0;
	FLAC__bool ok = true;

	printf("  seeking in a damaged copy ... ");
	fflush(stdout);

	/* make the copy */
	if((file = flac_fopen(filename, "rb")) == NULL || fseeko(file, 0, SEEK_END) != 0 || (file_size = (FLAC__uint64)ftello(file)) == 0 || fseeko(file, 0, SEEK_SET) != 0 ||
	   (data = malloc((size_t)file_size)) == NULL || fread(data, 1, (size_t)file_size, file) != file_size) {
		printf("FAILED reading %s\n", filename);
		ok = false;
	}
	if(file)
		fclose(file);
	if(ok && (damaged = calloc((size_t)frames, 1)) == NULL) {
		printf("FAILED allocating memory\n");
		ok = false;
	}
	for(i = 0; ok && i < damage_count; i++) {
		const FLAC__uint64 offset = first_frame_offset + random_(&random_state) % (file_size - first_frame_offset - damage_length);
		uint32_t j;
		for(j = 0; j < damage_length; j++)
			data[offset + j] ^= (FLAC__byte)(0x5a + j);
		/* mark the frames touched */
		for(frame = 0; frame < frames; frame++) {
			const FLAC__uint64 frame_start = frame == 0? first_frame_offset : frame_ends[frame - 1];
			if(offset < frame_ends[frame] && offset + damage_length > frame_start)
				damaged[frame] = 1;
		}
	}
	if(ok && ((file = flac_fopen(damaged_filename, "wb")) == NULL || fwrite(data, 1, (size_t)file_size, file) != file_size || fclose(file) != 0)) {
		printf("FAILED writing %s\n", damaged_filename);
		ok = false;
	}
	free(data);

	ok = ok && decoder_open_(&decoder, &client, signal, damaged_filename);
	for(i = 0; ok && i < seek_count_; i++) {
		const FLAC__uint64 target = random_(&random_state) % signal->total_samples;
		frame = target / blocksize;
		if(!damaged[frame]) {
			ok = check_seek_(decoder, &client, target, 0, NULL);
			good_seeks++;
		}
		else {
			/* the target frame is damaged, so the seek must fail; if it doesn't, the samples must be right anyway */
			client.got_block = false;
			if(FLAC__stream_decoder_seek_absolute(decoder, target)) {
				ok = check_block_(&client, target);
				damaged_hits++;
			}
			else if(!FLAC__stream_decoder_flush(decoder)) {
				printf("FAILED flushing the decoder\n");
				ok = false;
			}
			damaged_seeks++;
		}
	}
	if(ok)
		printf("OK (%u seeks to intact frames, %u seeks to damaged frames of which %u succeeded)\n", good_seeks, damaged_seeks, damaged_hits);

	decoder_close_(decoder, &client);
	free(damaged);
	if(!keep_files_)
		flac_unlink(damaged_filename);
	return ok;
}

/* makes sure the decoy stream really contains the fake frame headers, which requires verbatim subframes */
static FLAC__bool check_decoys_(const Signal *signal, const char *filename)
{
	const FLAC__uint64 frames = (signal->total_samples + signal->config->blocksize - 1) / signal->config->blocksize;
	/* real frame headers, fake ones, and the sync code in the sync_run of every full frame */
	const FLAC__uint64 expected = frames + signal->decoy_count + signal->total_samples / signal->config->blocksize;
	FLAC__uint64 headers = 0;
	uint32_t state = 0;
	int c;
	FILE *file;

	if((file = flac_fopen(filename, "rb")) == NULL) {
		printf("FAILED opening %s\n", filename);
		return false;
	}
	/* count the first two bytes of all frame headers, real and fake */
	while((c = getc(file)) != EOF) {
		if(state == 1 && c == 0xf8)
			headers++;
		state = c == 0xff;
	}
	fclose(file);

	printf("  stream contains %" PRIu64 " fake frame headers ... ", signal->decoy_count);
	if(headers < expected) {
		printf("FAILED, found only %" PRIu64 " of at least %" PRIu64 " sync codes; the stream must use verbatim subframes\n", headers, expected);
		return false;
	}
	printf("OK\n");
	return true;
}

static FLAC__bool test_stream_(const StreamConfig *config, uint32_t index)
{
	char filename[64], damaged_filename[64];
	Signal signal;
	FLAC__uint64 *frame_ends = NULL, first_frame_offset = 0;
	const FLAC__uint64 seed = hash_(index);
	/* make the stream end with a short frame */
	const FLAC__uint64 total_samples = (FLAC__uint64)config->sample_rate * 60 * minutes_ * config->length_percent / 100 + config->blocksize / 3;
	FLAC__bool ok;

	sprintf(filename, "test_seeking_large.%s.flac", config->name);
	sprintf(damaged_filename, "test_seeking_large.%s.damaged.flac", config->name);

	printf("%s: %" PRIu64 " samples, %u channels, %u bits, %u Hz, blocksize %u%s%s\n",
		config->name, total_samples, config->channels, config->bits_per_sample, config->sample_rate, config->blocksize,
		config->seek_table? ", seek table" : "", config->unknown_length? ", unknown length" : "");

	if(!signal_init_(&signal, config, total_samples)) {
		printf("FAILED allocating memory\n");
		return false;
	}

	printf("  encoding ... ");
	fflush(stdout);
	ok = encode_(&signal, filename);
	if(ok)
		printf("OK\n");

	if(ok && config->signal == SIGNAL_DECOY)
		ok = check_decoys_(&signal, filename);
	ok = ok && test_sequential_(&signal, filename, &frame_ends, &first_frame_offset);
	ok = ok && test_edge_cases_(&signal, filename);
	ok = ok && test_random_(&signal, filename, seed);
	ok = ok && test_scrubbing_(&signal, filename, seed + 1);
	if(config->damage)
		ok = ok && test_damaged_(&signal, filename, damaged_filename, frame_ends, first_frame_offset, seed + 2);

	free(frame_ends);
	signal_free_(&signal);
	if(!keep_files_)
		flac_unlink(filename);
	return ok;
}

int main(int argc, char *argv[])
{
	uint32_t i, failures = 0;

	for(i = 1; i < (uint32_t)argc; i++) {
		if(0 == strcmp(argv[i], "--minutes") && i + 1 < (uint32_t)argc)
			minutes_ = (uint32_t)atoi(argv[++i]);
		else if(0 == strcmp(argv[i], "--seeks") && i + 1 < (uint32_t)argc)
			seek_count_ = (uint32_t)atoi(argv[++i]);
		else if(0 == strcmp(argv[i], "--stream") && i + 1 < (uint32_t)argc && selected_stream_count_ < sizeof(selected_streams_) / sizeof(selected_streams_[0]))
			selected_streams_[selected_stream_count_++] = argv[++i];
		else if(0 == strcmp(argv[i], "--keep"))
			keep_files_ = true;
		else {
			printf("usage: %s [--minutes N] [--seeks N] [--stream NAME] [--keep]\n", argv[0]);
			return 1;
		}
	}
	if(minutes_ == 0 || seek_count_ == 0) {
		printf("ERROR: --minutes and --seeks must be positive\n");
		return 1;
	}

	for(i = 0; i < sizeof(stream_configs_) / sizeof(stream_configs_[0]); i++) {
		FLAC__bool selected = selected_stream_count_ == 0;
		uint32_t j;
		for(j = 0; j < selected_stream_count_; j++)
			selected = selected || 0 == strcmp(selected_streams_[j], stream_configs_[i].name);
		if(selected && !test_stream_(&stream_configs_[i], i))
			failures++;
	}

	if(failures) {
		printf("\n%u of %u streams FAILED\n", failures, (uint32_t)(sizeof(stream_configs_) / sizeof(stream_configs_[0])));
		return 1;
	}
	printf("\nPASSED\n");
	return 0;
}
