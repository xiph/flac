/* in_flac - Winamp FLAC input plugin
 * Copyright (C) 2000  Josh Coalson
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
#include <assert.h>

#include "in2.h"
#include "FLAC/all.h"

BOOL WINAPI _DllMainCRTStartup(HANDLE hInst, ULONG ul_reason_for_call, LPVOID lpReserved)
{
	return TRUE;
}

/* post this to the main window at end of file (after playback as stopped) */
#define WM_WA_MPEG_EOF WM_USER+2

typedef struct {
	bool abort_flag;
	unsigned total_samples;
	unsigned bits_per_sample;
	unsigned channels;
	unsigned sample_rate;
	unsigned length_in_ms;
} stream_info_struct;

static bool stream_init(const char *infile);
static FLAC__StreamDecoderWriteStatus write_callback(const FLAC__FileDecoder *decoder, const FLAC__FrameHeader *header, const int32 *buffer[], void *client_data);
static void metadata_callback(const FLAC__FileDecoder *decoder, const FLAC__StreamMetaData *metadata, void *client_data);
static void error_callback(const FLAC__FileDecoder *decoder, FLAC__StreamDecoderErrorStatus status, void *client_data);

In_Module mod; /* the output module (declared near the bottom of this file) */
char lastfn[MAX_PATH]; /* currently playing file (used for getting info on the current file) */
int decode_pos_ms; /* current decoding position, in milliseconds */
int paused; /* are we paused? */
int seek_needed; /* if != -1, it is the point that the decode thread should seek to, in ms. */
int16 reservoir[FLAC__MAX_BLOCK_SIZE * 2]; /* 2 for max channels */
char sample_buffer[576 * 2 * (16/8) * 2]; /* 2 for max channels, (16/8) for max bytes per sample, and 2 for who knows what */
unsigned samples_in_reservoir;
static stream_info_struct stream_info;
static FLAC__FileDecoder *decoder;

int killDecodeThread=0;					/* the kill switch for the decode thread */
HANDLE thread_handle=INVALID_HANDLE_VALUE;	/* the handle to the decode thread */

DWORD WINAPI __stdcall DecodeThread(void *b); /* the decode thread procedure */

void config(HWND hwndParent)
{
	MessageBox(hwndParent, "No configuration.", "Configuration", MB_OK);
	/* if we had a configuration we'd want to write it here :) */
}
void about(HWND hwndParent)
{
	MessageBox(hwndParent,"Winamp FLAC Plugin v0.2, by Josh Coalson\nSee http://flac.sourceforge.net/","About FLAC Plugin",MB_OK);
}

void init()
{
	decoder = FLAC__file_decoder_get_new_instance();
}

void quit()
{
	if(decoder)
		FLAC__file_decoder_free_instance(decoder);
}

int isourfile(char *fn) { return 0; }
/* used for detecting URL streams.. unused here. strncmp(fn,"http://",7) to detect HTTP streams, etc */

int play(char *fn)
{
	int maxlatency;
	int thread_id;
	HANDLE input_file=INVALID_HANDLE_VALUE;

	if(0 == decoder) {
		return 1;
	}

	input_file = CreateFile(fn,GENERIC_READ,FILE_SHARE_READ|FILE_SHARE_WRITE,NULL,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,NULL);
	if (input_file == INVALID_HANDLE_VALUE) {
		return 1;
	}
	CloseHandle(input_file);

	if(!stream_init(fn)) {
		return 1;
	}

	strcpy(lastfn,fn);
	paused=0;
	decode_pos_ms=0;
	seek_needed=-1;
	samples_in_reservoir = 0;

	maxlatency = mod.outMod->Open(stream_info.sample_rate, stream_info.channels, stream_info.bits_per_sample, -1,-1);
	if (maxlatency < 0) { /* error opening device */
		return 1;
	}

	/* dividing by 1000 for the first parameter of setinfo makes it */
	/* display 'H'... for hundred.. i.e. 14H Kbps. */
	mod.SetInfo((stream_info.sample_rate*stream_info.bits_per_sample*stream_info.channels)/1000,stream_info.sample_rate/1000,stream_info.channels,1);

	/* initialize vis stuff */
	mod.SAVSAInit(maxlatency,stream_info.sample_rate);
	mod.VSASetInfo(stream_info.sample_rate,stream_info.channels);

	mod.outMod->SetVolume(-666); /* set the output plug-ins default volume */

	killDecodeThread=0;
	thread_handle = (HANDLE) CreateThread(NULL,0,(LPTHREAD_START_ROUTINE) DecodeThread,(void *) &killDecodeThread,0,&thread_id);

	return 0;
}

void pause()
{
	paused=1;
	mod.outMod->Pause(1);
}

void unpause()
{
	paused=0;
	mod.outMod->Pause(0);
}
int ispaused()
{
	return paused;
}

void stop()
{
	if (thread_handle != INVALID_HANDLE_VALUE) {
		killDecodeThread=1;
		if (WaitForSingleObject(thread_handle,INFINITE) == WAIT_TIMEOUT) {
			MessageBox(mod.hMainWindow,"error asking thread to die!\n","error killing decode thread",0);
			TerminateThread(thread_handle,0);
		}
		CloseHandle(thread_handle);
		thread_handle = INVALID_HANDLE_VALUE;
	}
	if(decoder) {
		if(decoder->state != FLAC__FILE_DECODER_UNINITIALIZED)
			FLAC__file_decoder_finish(decoder);
	}

	mod.outMod->Close();

	mod.SAVSADeInit();
}

int getlength()
{
	return (int)stream_info.length_in_ms;
}

int getoutputtime()
{
	return decode_pos_ms+(mod.outMod->GetOutputTime()-mod.outMod->GetWrittenTime());
}

void setoutputtime(int time_in_ms)
{
	seek_needed=time_in_ms;
}

void setvolume(int volume) { mod.outMod->SetVolume(volume); }
void setpan(int pan) { mod.outMod->SetPan(pan); }

int infoDlg(char *fn, HWND hwnd)
{
	/* TODO: implement info dialog. */
	return 0;
}

void getfileinfo(char *filename, char *title, int *length_in_ms)
{
	if (!filename || !*filename) { /* currently playing file */
		if (length_in_ms)
			*length_in_ms=getlength();
		if (title) {
			char *p=lastfn+strlen(lastfn);
			while (*p != '\\' && p >= lastfn) p--;
			strcpy(title,++p);
		}
	}
	else { /* some other file */
		if (length_in_ms) {
			FLAC__FileDecoder *tmp_decoder = FLAC__file_decoder_get_new_instance();
			stream_info_struct tmp_stream_info;
			tmp_stream_info.abort_flag = false;
			if(FLAC__file_decoder_init(tmp_decoder, filename, write_callback, metadata_callback, error_callback, &tmp_stream_info) != FLAC__FILE_DECODER_OK)
				return;
			if(!FLAC__file_decoder_process_metadata(tmp_decoder))
				return;

			*length_in_ms = (int)tmp_stream_info.length_in_ms;

			if(tmp_decoder->state != FLAC__FILE_DECODER_UNINITIALIZED)
				FLAC__file_decoder_finish(tmp_decoder);
			FLAC__file_decoder_free_instance(tmp_decoder);
		}
		if (title) {
			char *p=filename+strlen(filename);
			while (*p != '\\' && p >= filename) p--;
			strcpy(title,++p);
		}
	}
}

void eq_set(int on, char data[10], int preamp)
{
}

DWORD WINAPI __stdcall DecodeThread(void *b)
{
	int done=0;

	while (! *((int *)b) ) {
		unsigned channels = stream_info.channels;
		unsigned bits_per_sample = stream_info.bits_per_sample;
		unsigned bytes_per_sample = (bits_per_sample+7)/8;
		unsigned sample_rate = stream_info.sample_rate;
		if (seek_needed != -1) {
			const double distance = (double)seek_needed / (double)getlength();
			unsigned target_sample = (unsigned)(distance * (double)stream_info.total_samples);
			if(FLAC__file_decoder_seek_absolute(decoder, (uint64)target_sample)) {
				decode_pos_ms = (int)(distance * (double)getlength());
				seek_needed=-1;
				done=0;
				mod.outMod->Flush(decode_pos_ms);
			}
		}
		if (done) {
			if (!mod.outMod->IsPlaying()) {
				PostMessage(mod.hMainWindow,WM_WA_MPEG_EOF,0,0);
				return 0;
			}
			Sleep(10);
		}
		else if (mod.outMod->CanWrite() >= ((int)(576*channels*bytes_per_sample) << (mod.dsp_isactive()?1:0))) {
			while(samples_in_reservoir < 576) {
				if(decoder->state == FLAC__FILE_DECODER_END_OF_FILE) {
					done = 1;
					break;
				}
				else if(!FLAC__file_decoder_process_one_frame(decoder))
					break;
			}

			if (samples_in_reservoir == 0) {
				done=1;
			}
			else {
				unsigned i, n = min(samples_in_reservoir, 576), delta;
				int l;
				signed short *ssbuffer = (signed short *)sample_buffer;

				for(i = 0; i < n*channels; i++)
					ssbuffer[i] = reservoir[i];
				delta = i;
				for( ; i < samples_in_reservoir*channels; i++)
					reservoir[i-delta] = reservoir[i];
				samples_in_reservoir -= n;
				l = n * channels * bytes_per_sample;

				mod.SAAddPCMData((char *)sample_buffer,channels,bits_per_sample,decode_pos_ms);
				mod.VSAAddPCMData((char *)sample_buffer,channels,bits_per_sample,decode_pos_ms);
				decode_pos_ms+=(576*1000)/sample_rate;
				if (mod.dsp_isactive())
					l=mod.dsp_dosamples((short *)sample_buffer,n/channels/bytes_per_sample,bits_per_sample,channels,sample_rate) * (channels*bytes_per_sample);
				mod.outMod->Write(sample_buffer,l);
			}
		}
		else Sleep(20);
	}
	return 0;
}



In_Module mod =
{
	IN_VER,
	"Reference FLAC Player v0.0"
#ifdef __alpha
	"(AXP)"
#else
	"(x86)"
#endif
	,
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
	return &mod;
}


/***********************************************************************
 * local routines
 **********************************************************************/
bool stream_init(const char *infile)
{
	if(FLAC__file_decoder_init(decoder, infile, write_callback, metadata_callback, error_callback, &stream_info) != FLAC__FILE_DECODER_OK) {
		MessageBox(mod.hMainWindow,"ERROR initializing decoder, state = %d\n","ERROR initializing decoder",0);
		return false;
	}

	stream_info.abort_flag = false;
	if(!FLAC__file_decoder_process_metadata(decoder)) {
		return false;
	}

	return true;
}

FLAC__StreamDecoderWriteStatus write_callback(const FLAC__FileDecoder *decoder, const FLAC__FrameHeader *header, const int32 *buffer[], void *client_data)
{
	stream_info_struct *stream_info = (stream_info_struct *)client_data;
	unsigned bps = stream_info->bits_per_sample, channels = stream_info->channels;
	unsigned wide_samples = header->blocksize, wide_sample, sample, channel, offset;

	(void)decoder;

	if(stream_info->abort_flag)
		return FLAC__STREAM_DECODER_WRITE_ABORT;

	offset = samples_in_reservoir * channels;

	for(sample = wide_sample = 0; wide_sample < wide_samples; wide_sample++)
		for(channel = 0; channel < channels; channel++, sample++)
			reservoir[offset+sample] = (int16)buffer[channel][wide_sample];

	samples_in_reservoir += wide_samples;

	return FLAC__STREAM_DECODER_WRITE_CONTINUE;
}

void metadata_callback(const FLAC__FileDecoder *decoder, const FLAC__StreamMetaData *metadata, void *client_data)
{
	stream_info_struct *stream_info = (stream_info_struct *)client_data;
	(void)decoder;
	if(metadata->type == FLAC__METADATA_TYPE_ENCODING) {
		assert(metadata->data.encoding.total_samples < 0x100000000); /* this plugin can only handle < 4 gigasamples */
		stream_info->total_samples = (unsigned)(metadata->data.encoding.total_samples&0xffffffff);
		stream_info->bits_per_sample = metadata->data.encoding.bits_per_sample;
		stream_info->channels = metadata->data.encoding.channels;
		stream_info->sample_rate = metadata->data.encoding.sample_rate;

		if(stream_info->bits_per_sample != 16) {
			MessageBox(mod.hMainWindow,"ERROR: plugin can only handle 16-bit samples\n","ERROR: plugin can only handle 16-bit samples",0);
			stream_info->abort_flag = true;
			return;
		}
		stream_info->length_in_ms = stream_info->total_samples * 10 / (stream_info->sample_rate / 100);
	}
}

void error_callback(const FLAC__FileDecoder *decoder, FLAC__StreamDecoderErrorStatus status, void *client_data)
{
	stream_info_struct *stream_info = (stream_info_struct *)client_data;
	(void)decoder;
	if(status != FLAC__STREAM_DECODER_ERROR_LOST_SYNC)
		stream_info->abort_flag = true;
}
