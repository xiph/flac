/* in_flac - Winamp2 FLAC input plugin
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

#include <windows.h>
#include <mmreg.h>
#include <msacm.h>
#include <math.h>
#include <stdio.h>

#include "in2.h"
#include "FLAC/all.h"
#include "plugin_common/all.h"

#ifdef max
#undef max
#endif
#define max(a,b) ((a)>(b)?(a):(b))


#define FLAC__DO_DITHER

BOOL WINAPI _DllMainCRTStartup(HANDLE hInst, ULONG ul_reason_for_call, LPVOID lpReserved)
{
	return TRUE;
}

/* post this to the main window at end of file (after playback as stopped) */
#define WM_WA_MPEG_EOF WM_USER+2

typedef struct {
	FLAC__bool abort_flag;
	unsigned total_samples;
	unsigned bits_per_sample;
	unsigned channels;
	unsigned sample_rate;
	unsigned length_in_msec;
} file_info_struct;

static FLAC__bool safe_decoder_init_(const char *infilename, FLAC__FileDecoder *decoder);
static void safe_decoder_finish_(FLAC__FileDecoder *decoder);
static void safe_decoder_delete_(FLAC__FileDecoder *decoder);
static FLAC__StreamDecoderWriteStatus write_callback_(const FLAC__FileDecoder *decoder, const FLAC__Frame *frame, const FLAC__int32 * const buffer[], void *client_data);
static void metadata_callback_(const FLAC__FileDecoder *decoder, const FLAC__StreamMetadata *metadata, void *client_data);
static void error_callback_(const FLAC__FileDecoder *decoder, FLAC__StreamDecoderErrorStatus status, void *client_data);
static void get_description_(const char *filename, char *description, unsigned max_size);

In_Module mod_; /* the output module (declared near the bottom of this file) */
char lastfn_[MAX_PATH]; /* currently playing file (used for getting info on the current file) */
int decode_pos_ms_; /* current decoding position, in milliseconds */
int paused_; /* are we paused? */
int seek_needed_; /* if != -1, it is the point that the decode thread should seek to, in ms. */

#define SAMPLES_PER_WRITE 576
FLAC__int32 reservoir_[FLAC__MAX_BLOCK_SIZE * 2/*for overflow*/ * FLAC_PLUGIN__MAX_SUPPORTED_CHANNELS];
char sample_buffer_[SAMPLES_PER_WRITE * FLAC_PLUGIN__MAX_SUPPORTED_CHANNELS * (24/8) * 2]; /* (24/8) for max bytes per sample, and 2 for who knows what */
unsigned wide_samples_in_reservoir_;
static file_info_struct file_info_;
static FLAC__FileDecoder *decoder_;

int killDecodeThread = 0;					/* the kill switch for the decode thread */
HANDLE thread_handle = INVALID_HANDLE_VALUE;	/* the handle to the decode thread */

DWORD WINAPI __stdcall DecodeThread(void *b); /* the decode thread procedure */


static void do_vis(char *data, int nch, int resolution, int position, unsigned samples)
{
	static char vis_buffer[SAMPLES_PER_WRITE * FLAC_PLUGIN__MAX_SUPPORTED_CHANNELS];
	char *ptr;
	int size, count;

	/*
	 * Winamp visuals may have problems accepting sample sizes larger than
	 * 16 bits, so we reduce the sample size here if necessary.
	 */

	switch(resolution) {
		case 32:
		case 24:
			size  = resolution / 8;
			count = samples * nch;

			ptr = vis_buffer;
			while(count--) {
				data += size;
				*ptr++ = data[-1] ^ 0x80;
			}

			data = vis_buffer;
			resolution = 8;

			/* fall through */
		case 16:
		case 8:
		default:
			mod_.SAAddPCMData(data, nch, resolution, position);
			mod_.VSAAddPCMData(data, nch, resolution, position);
	}
}

void config(HWND hwndParent)
{
	MessageBox(hwndParent, "No configuration.", "Configuration", MB_OK);
	/* if we had a configuration we'd want to write it here :) */
}
void about(HWND hwndParent)
{
	MessageBox(hwndParent, "Winamp FLAC Plugin v" FLAC__VERSION_STRING ", by Josh Coalson\nSee http://flac.sourceforge.net/", "About FLAC Plugin", MB_OK);
}

void init()
{
	decoder_ = FLAC__file_decoder_new();
	strcpy(lastfn_, "");
}

void quit()
{
	safe_decoder_delete_(decoder_);
	decoder_ = 0;
}

int isourfile(char *fn) { return 0; }
/* used for detecting URL streams.. unused here. strncmp(fn, "http://", 7) to detect HTTP streams, etc */

int play(char *fn)
{
	int maxlatency;
	int thread_id;
	HANDLE input_file = INVALID_HANDLE_VALUE;
#ifdef FLAC__DO_DITHER
	const unsigned output_bits_per_sample = min(file_info_.bits_per_sample, 16);
#else
	const unsigned output_bits_per_sample = file_info_.bits_per_sample;
#endif

	if(0 == decoder_) {
		return 1;
	}

	input_file = CreateFile(fn, GENERIC_READ, FILE_SHARE_READ|FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if(input_file == INVALID_HANDLE_VALUE) {
		return -1;
	}
	CloseHandle(input_file);

	if(!safe_decoder_init_(fn, decoder_)) {
		return 1;
	}

	strcpy(lastfn_, fn);
	paused_ = 0;
	decode_pos_ms_ = 0;
	seek_needed_ = -1;
	wide_samples_in_reservoir_ = 0;

	maxlatency = mod_.outMod->Open(file_info_.sample_rate, file_info_.channels, output_bits_per_sample, -1, -1);
	if(maxlatency < 0) { /* error opening device */
		return 1;
	}

	/* dividing by 1000 for the first parameter of setinfo makes it */
	/* display 'H'... for hundred.. i.e. 14H Kbps. */
	mod_.SetInfo((file_info_.sample_rate*file_info_.bits_per_sample*file_info_.channels)/1000, file_info_.sample_rate/1000, file_info_.channels, 1);

	/* initialize vis stuff */
	mod_.SAVSAInit(maxlatency, file_info_.sample_rate);
	mod_.VSASetInfo(file_info_.sample_rate, file_info_.channels);

	mod_.outMod->SetVolume(-666); /* set the output plug-ins default volume */

	killDecodeThread = 0;
	thread_handle = (HANDLE) CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE) DecodeThread, (void *) &killDecodeThread, 0, &thread_id);

	return 0;
}

void pause()
{
	paused_ = 1;
	mod_.outMod->Pause(1);
}

void unpause()
{
	paused_ = 0;
	mod_.outMod->Pause(0);
}
int ispaused()
{
	return paused_;
}

void stop()
{
	if(thread_handle != INVALID_HANDLE_VALUE) {
		killDecodeThread = 1;
		if(WaitForSingleObject(thread_handle, INFINITE) == WAIT_TIMEOUT) {
			MessageBox(mod_.hMainWindow, "error asking thread to die!\n", "error killing decode thread", 0);
			TerminateThread(thread_handle, 0);
		}
		CloseHandle(thread_handle);
		thread_handle = INVALID_HANDLE_VALUE;
	}
	safe_decoder_finish_(decoder_);

	mod_.outMod->Close();

	mod_.SAVSADeInit();
}

int getlength()
{
	return (int)file_info_.length_in_msec;
}

int getoutputtime()
{
	return decode_pos_ms_ + (mod_.outMod->GetOutputTime() - mod_.outMod->GetWrittenTime());
}

void setoutputtime(int time_in_ms)
{
	seek_needed_ = time_in_ms;
}

void setvolume(int volume) { mod_.outMod->SetVolume(volume); }
void setpan(int pan) { mod_.outMod->SetPan(pan); }

int infoDlg(char *fn, HWND hwnd)
{
	/* @@@TODO: implement info dialog. */
	return 0;
}

void getfileinfo(char *filename, char *title, int *length_in_msec)
{
	FLAC__StreamMetadata streaminfo;

	if(0 == filename || filename[0] == '\0') {
		filename = lastfn_;
		if(length_in_msec) {
			*length_in_msec = getlength();
			length_in_msec = 0; /* force skip in following code */
		}
	}

	if(!FLAC__metadata_get_streaminfo(filename, &streaminfo)) {
		MessageBox(mod_.hMainWindow, filename, "ERROR: invalid/missing FLAC metadata", 0);
		if(title) {
			static const char *errtitle = "Invalid FLAC File: ";
			sprintf(title, "%s\"%s\"", errtitle, filename);
		}
		if(length_in_msec)
			*length_in_msec = -1;
		return;
	}

	if(title) {
		get_description_(filename, title, MAX_PATH);
	}
	if(length_in_msec)
		*length_in_msec = (int)(streaminfo.data.stream_info.total_samples * 10 / (streaminfo.data.stream_info.sample_rate / 100));
}

void eq_set(int on, char data[10], int preamp)
{
}

DWORD WINAPI __stdcall DecodeThread(void *b)
{
	int done = 0;

	while(! *((int *)b) ) {
		const unsigned channels = file_info_.channels;
		const unsigned bits_per_sample = file_info_.bits_per_sample;
#ifdef FLAC__DO_DITHER
		const unsigned target_bps = min(bits_per_sample, 16);
#else
		const unsigned target_bps = bits_per_sample;
#endif
		const unsigned sample_rate = file_info_.sample_rate;
		if(seek_needed_ != -1) {
			const double distance = (double)seek_needed_ / (double)getlength();
			const unsigned target_sample = (unsigned)(distance * (double)file_info_.total_samples);
			if(FLAC__file_decoder_seek_absolute(decoder_, (FLAC__uint64)target_sample)) {
				decode_pos_ms_ = (int)(distance * (double)getlength());
				seek_needed_ = -1;
				done = 0;
				mod_.outMod->Flush(decode_pos_ms_);
			}
		}
		if(done) {
			if(!mod_.outMod->IsPlaying()) {
				PostMessage(mod_.hMainWindow, WM_WA_MPEG_EOF, 0, 0);
				return 0;
			}
			Sleep(10);
		}
		else if(mod_.outMod->CanWrite() >= ((int)(SAMPLES_PER_WRITE*channels*((target_bps+7)/8)) << (mod_.dsp_isactive()?1:0))) {
			while(wide_samples_in_reservoir_ < SAMPLES_PER_WRITE) {
				if(FLAC__file_decoder_get_state(decoder_) == FLAC__FILE_DECODER_END_OF_FILE) {
					done = 1;
					break;
				}
				else if(!FLAC__file_decoder_process_single(decoder_)) {
					MessageBox(mod_.hMainWindow, FLAC__FileDecoderStateString[FLAC__file_decoder_get_state(decoder_)], "READ ERROR processing frame", 0);
					done = 1;
					break;
				}
			}

			if(wide_samples_in_reservoir_ == 0) {
				done = 1;
			}
			else {
				const unsigned n = min(wide_samples_in_reservoir_, SAMPLES_PER_WRITE);
				const unsigned delta = n * channels;
				int bytes = (int)FLAC__plugin_common__pack_pcm_signed_little_endian(sample_buffer_, reservoir_, n, channels, bits_per_sample, target_bps);
				unsigned i;

				for(i = delta; i < wide_samples_in_reservoir_ * channels; i++)
					reservoir_[i-delta] = reservoir_[i];
				wide_samples_in_reservoir_ -= n;

				do_vis((char *)sample_buffer_, channels, target_bps, decode_pos_ms_, n);
				decode_pos_ms_ += (n*1000 + sample_rate/2)/sample_rate;
				if(mod_.dsp_isactive())
					bytes = mod_.dsp_dosamples((short *)sample_buffer_, n, target_bps, channels, sample_rate) * (channels*target_bps/8);
				mod_.outMod->Write(sample_buffer_, bytes);
			}
		}
		else Sleep(20);
	}
	return 0;
}



In_Module mod_ =
{
	IN_VER,
	"Reference FLAC Player v" FLAC__VERSION_STRING,
	0,	/* hMainWindow */
	0,  /* hDllInstance */
	"FLAC\0FLAC Audio File (*.FLAC)\0"
	,
	1,	/* is_seekable */
	1, /* uses output */
	config,
	about,
	init,
	quit,
	getfileinfo,
	infoDlg,
	isourfile,
	play,
	pause,
	unpause,
	ispaused,
	stop,

	getlength,
	getoutputtime,
	setoutputtime,

	setvolume,
	setpan,

	0,0,0,0,0,0,0,0,0, /* vis stuff */


	0,0, /* dsp */

	eq_set,

	NULL,		/* setinfo */

	0 /* out_mod */

};

__declspec( dllexport ) In_Module * winampGetInModule2()
{
	return &mod_;
}


/***********************************************************************
 * local routines
 **********************************************************************/
FLAC__bool safe_decoder_init_(const char *filename, FLAC__FileDecoder *decoder)
{
	if(decoder == 0) {
		MessageBox(mod_.hMainWindow, "Decoder instance is NULL", "ERROR initializing decoder", 0);
		return false;
	}

	safe_decoder_finish_(decoder);

	FLAC__file_decoder_set_md5_checking(decoder, false);
	FLAC__file_decoder_set_filename(decoder, filename);
	FLAC__file_decoder_set_write_callback(decoder, write_callback_);
	FLAC__file_decoder_set_metadata_callback(decoder, metadata_callback_);
	FLAC__file_decoder_set_error_callback(decoder, error_callback_);
	FLAC__file_decoder_set_client_data(decoder, &file_info_);
	if(FLAC__file_decoder_init(decoder) != FLAC__FILE_DECODER_OK) {
		MessageBox(mod_.hMainWindow, FLAC__FileDecoderStateString[FLAC__file_decoder_get_state(decoder)], "ERROR initializing decoder", 0);
		return false;
	}

	file_info_.abort_flag = false;
	if(!FLAC__file_decoder_process_until_end_of_metadata(decoder)) {
		MessageBox(mod_.hMainWindow, FLAC__FileDecoderStateString[FLAC__file_decoder_get_state(decoder)], "ERROR processing metadata", 0);
		return false;
	}

	if(file_info_.abort_flag) {
		/* metadata callback already popped up the error dialog */
		return false;
	}

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
	unsigned wide_sample, offset_sample, channel;

	(void)decoder;

	if(file_info->abort_flag)
		return FLAC__STREAM_DECODER_WRITE_STATUS_ABORT;

	for(offset_sample = wide_samples_in_reservoir_ * channels, wide_sample = 0; wide_sample < wide_samples; wide_sample++)
		for(channel = 0; channel < channels; channel++, offset_sample++)
			reservoir_[offset_sample] = buffer[channel][wide_sample];

	wide_samples_in_reservoir_ += wide_samples;

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

#ifdef FLAC__DO_DITHER
		if(file_info->bits_per_sample != 8 && file_info->bits_per_sample != 16 && file_info->bits_per_sample != 24) {
			MessageBox(mod_.hMainWindow, "ERROR: plugin can only handle 8/16/24-bit samples\n", "ERROR: plugin can only handle 8/16/24-bit samples", 0);
			file_info->abort_flag = true;
			return;
		}
#else
		if(file_info->bits_per_sample != 8 && file_info->bits_per_sample != 16 && file_info->bits_per_sample != 24) {
			MessageBox(mod_.hMainWindow, "ERROR: plugin can only handle 8/16/24-bit samples\n", "ERROR: plugin can only handle 8/16/24-bit samples", 0);
			file_info->abort_flag = true;
			return;
		}
#endif
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

static FLAC__bool local__is_blank(const char *s)
{
	return 0 == s || *s == '\0';
}

void get_description_(const char *filename, char *description, unsigned max_size)
{
	FLAC_Plugin__CanonicalTag tag;

	FLAC_plugin__canonical_tag_init(&tag);
	FLAC_plugin__canonical_tag_get_combined(filename, &tag);

	/* @@@ when config window is done, add code here for converting to user charset ala plugin_xmms */

	if(local__is_blank(tag.performer) && local__is_blank(tag.composer) && local__is_blank(tag.title)) {
		/* set the description to the filename */
		char *p;
		const char *temp = strrchr(filename, '\\');
		if(0 == temp)
			temp = strrchr(filename, '/');
		if(0 == temp)
			temp = filename;
		else
			temp++;
		strncpy(description, temp, max_size);
		description[max_size-1] = '\0';
		if(0 != (p = strrchr(description, '.')))
			*p = '\0';
	}
	else {
		char *artist = !local__is_blank(tag.performer)? tag.performer : !local__is_blank(tag.composer)? tag.composer : "Unknown Artist";
		char *title = !local__is_blank(tag.title)? tag.title : "Untitled";

		/* there's no snprintf in VC6 so we get sloppy */
#if 0
		snprintf(description, max_size, "%s - %s", artist, title);
#else
		const unsigned needed = strlen(artist) + strlen(title) + 3 + 1;
		if(needed <= max_size)
			sprintf(description, "%s - %s", artist, title);
		else {
			char *p = malloc(needed);
			if(0 != p) {
				sprintf(p, "%s - %s", artist, title);
				p[max_size-1] = '\0';
				strcpy(description, p);
			}
		}
#endif
	}
		
	FLAC_plugin__canonical_tag_clear(&tag);
}
