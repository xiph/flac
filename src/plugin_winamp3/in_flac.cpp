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

static char plugin_description_[] = "Reference FLAC Player v" FLAC__VERSION_STRING;
static HINSTANCE g_hDllInstance_;

class FLAC_Info : public WInputInfo
{
	private:
		char title[MAX_PATH], infostr[MAX_PATH];
		int length_in_ms;
	public:
		FLAC_Info();
		~FLAC_Info();
		int Open(char *url);
		void GetTitle(char *buf, int maxlen);
		void GetInfoString(char *buf, int maxlen);
		inline int GetLength(void) { return length_in_ms; }
};

FLAC_Info::FLAC_Info() : WInputInfo()
{
	infostr[0] = title[0] = 0;
	length_in_ms = -1;
}

FLAC_Info::~FLAC_Info()
{
}

int FLAC_Info::Open(char *url)
{
	char *p=url+strlen(url);
	while (*p != '\\' && p >= url) p--;
	strcpy(title, ++p);

	HANDLE hFile;
	length_in_ms = -1;
	hFile = CreateFile(url, GENERIC_READ, FILE_SHARE_READ|FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hFile != INVALID_HANDLE_VALUE) {
		length_in_ms = (GetFileSize(hFile, NULL)*10)/(SAMPLERATE/100*NCH*(BPS/8));
	}
	CloseHandle(hFile);
	return 0;
}

void FLAC_Info::GetTitle(char *buf, int maxlen)
{
	strncpy(buf, title, maxlen-1);
	buf[maxlen-1]=0;
}

void FLAC_Info::GetInfoString(char *buf, int maxlen)
{
	strncpy(buf, infostr, maxlen-1);
	buf[maxlen-1]=0;
}


class FLAC_Source : public WInputSource
{
	private:
		char title[MAX_PATH], infostr[MAX_PATH];
		int length_in_ms;
		HANDLE input_file;
	public:
		FLAC_Source();
		~FLAC_Source();
		inline char *GetDescription(void) { return plugin_description_; }
		inline int UsesOutputFilters(void) { return 1; }
		int Open(char *url, bool *killswitch);
		int GetSamples(char *sample_buffer, int bytes, int *bps, int *nch, int *srate, bool *killswitch);
		int SetPosition(int); // sets position in ms
		void GetTitle(char *buf, int maxlen);
		void GetInfoString(char *buf, int maxlen);
		inline int GetLength(void) { return -1; }
};

void FLAC_Source::GetTitle(char *buf, int maxlen)
{
	strncpy(buf, title, maxlen-1);
	buf[maxlen-1] = 0;
}

void FLAC_Source::GetInfoString(char *buf, int maxlen)
{
	strncpy(buf, infostr, maxlen-1);
	buf[maxlen-1] = 0;
}

FLAC_Source::FLAC_Source() : WInputSource()
{
	infostr[0] = title[0] = 0;
	length_in_ms = -1;
	input_file = NULL;
}

FLAC_Source::~FLAC_Source()
{
	if(input_file)
		CloseHandle(input_file);
}

int FLAC_Source::Open(char *url, bool *killswitch)
{
	char *p = url+strlen(url);
	while (*p != '\\' && p >= url) p--;
	strcpy(title, ++p);

	length_in_ms = -1;
	input_file = CreateFile(url, GENERIC_READ, FILE_SHARE_READ|FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (input_file == INVALID_HANDLE_VALUE) return 1;

	length_in_ms = (GetFileSize(input_file, NULL)*10)/(SAMPLERATE/100*NCH*(BPS/8));
	return 0;
}

int FLAC_Source::GetSamples(char *sample_buffer, int bytes, int *bps, int *nch, int *srate, bool *killswitch)
{
	*srate = SAMPLERATE;
	*bps = BPS;
	*nch = NCH;

	unsigned long l;
	ReadFile(input_file, sample_buffer, bytes, &l, NULL);

	return l;
}

int FLAC_Source::SetPosition(int position)
{
	return 1; // seeking not possible
}

 
/***********************************************************************
 * local routines
 **********************************************************************/


 
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
