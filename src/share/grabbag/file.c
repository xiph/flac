/* grabbag - Convenience lib for various routines common to several tools
 * Copyright (C) 2002-2009  Josh Coalson
 * Copyright (C) 2011-2025  Xiph.Org Foundation
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#if defined _MSC_VER || defined __MINGW32__
#include <sys/utime.h> /* for utime() */
#include <io.h> /* for chmod(), _setmode(), unlink() */
#include <fcntl.h> /* for _O_BINARY */
#else
#include <sys/types.h> /* some flavors of BSD (like OS X) require this to get time_t */
#endif
#if defined __EMX__
#include <io.h> /* for setmode(), O_BINARY */
#include <fcntl.h> /* for _O_BINARY */
#endif
#include <sys/stat.h> /* for stat(), maybe chmod() */
#if defined _WIN32 && !defined __CYGWIN__
#else
#include <unistd.h> /* for unlink() */
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h> /* for strrchr() */
#if defined _WIN32 && !defined __CYGWIN__
// for GetFileInformationByHandle() etc
#include <windows.h>
#include <winbase.h>
#endif
#include "share/grabbag.h"
#include "share/compat.h"


void grabbag__file_copy_metadata(const char *srcpath, const char *destpath)
{
	struct flac_stat_s srcstat;

	if(0 == flac_stat(srcpath, &srcstat)) {
#if defined(_POSIX_C_SOURCE) && (_POSIX_C_SOURCE >= 200809L) && !defined(_WIN32)
		struct timespec srctime[2] = {};
		srctime[0].tv_sec = srcstat.st_atime;
		srctime[1].tv_sec = srcstat.st_mtime;
#else
		struct utimbuf srctime;
		srctime.actime = srcstat.st_atime;
		srctime.modtime = srcstat.st_mtime;
#endif
		(void)flac_chmod(destpath, srcstat.st_mode);
		(void)flac_utime(destpath, &srctime);
	}
}

FLAC__off_t grabbag__file_get_filesize(const char *srcpath)
{
	struct flac_stat_s srcstat;

	if(0 == flac_stat(srcpath, &srcstat))
		return srcstat.st_size;
	else
		return -1;
}

const char *grabbag__file_get_basename(const char *srcpath)
{
	const char *p;

	p = strrchr(srcpath, '/');
	if(0 == p) {
#if defined _WIN32 && !defined __CYGWIN__
		p = strrchr(srcpath, '\\');
		if(0 == p)
#endif
			return srcpath;
	}
	return ++p;
}

FLAC__bool grabbag__file_change_stats(const char *filename, FLAC__bool read_only)
{
	struct flac_stat_s stats;

	if(0 == flac_stat(filename, &stats)) {
#if !defined _MSC_VER && !defined __MINGW32__
		if(read_only) {
			stats.st_mode &= ~S_IWUSR;
			stats.st_mode &= ~S_IWGRP;
			stats.st_mode &= ~S_IWOTH;
		}
		else {
			stats.st_mode |= S_IWUSR;
		}
#else
		if(read_only)
			stats.st_mode &= ~S_IWRITE;
		else
			stats.st_mode |= S_IWRITE;
#endif
		if(0 != flac_chmod(filename, stats.st_mode))
			return false;
	}
	else
		return false;

	return true;
}

FLAC__bool grabbag__file_are_same(const char *f1, const char *f2)
{
#if defined _WIN32 && !defined __CYGWIN__
#if !defined(WINAPI_FAMILY_PARTITION)
#define WINAPI_FAMILY_PARTITION(x) x
#define WINAPI_PARTITION_DESKTOP 1
#endif
	/* see
	 *  http://www.hydrogenaudio.org/forums/index.php?showtopic=49439&pid=444300&st=0
	 *  http://msdn.microsoft.com/library/default.asp?url=/library/en-us/fileio/fs/getfileinformationbyhandle.asp
	 *  http://msdn.microsoft.com/library/default.asp?url=/library/en-us/fileio/fs/by_handle_file_information_str.asp
	 *  http://msdn.microsoft.com/library/default.asp?url=/library/en-us/fileio/fs/createfile.asp
	 * apparently both the files have to be open at the same time for the comparison to work
	 */
	FLAC__bool same = false;
	BY_HANDLE_FILE_INFORMATION info1, info2;
	HANDLE h1, h2;
	BOOL ok = 1;
	h1 = CreateFile_utf8(f1, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	h2 = CreateFile_utf8(f2, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if(h1 == INVALID_HANDLE_VALUE || h2 == INVALID_HANDLE_VALUE)
		ok = 0;
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
	ok &= GetFileInformationByHandle(h1, &info1);
	ok &= GetFileInformationByHandle(h2, &info2);
	if(ok)
		same =
			info1.dwVolumeSerialNumber == info2.dwVolumeSerialNumber &&
			info1.nFileIndexHigh == info2.nFileIndexHigh &&
			info1.nFileIndexLow == info2.nFileIndexLow
		;
#else // !WINAPI_PARTITION_DESKTOP
	FILE_ID_INFO id_info1, id_info2;
	same = GetFileInformationByHandleEx(h1, FileIdInfo, &id_info1, sizeof (id_info1)) &&
	       GetFileInformationByHandleEx(h2, FileIdInfo, &id_info2, sizeof (id_info2)) &&
	       id_info1.VolumeSerialNumber == id_info2.VolumeSerialNumber &&
	       memcmp(&id_info1.FileId, &id_info2.FileId, sizeof(id_info1.FileId)) == 0;
#endif // !WINAPI_PARTITION_DESKTOP
	if(h1 != INVALID_HANDLE_VALUE)
		CloseHandle(h1);
	if(h2 != INVALID_HANDLE_VALUE)
		CloseHandle(h2);
	return same;
#else
	struct flac_stat_s s1, s2;
	return f1 && f2 && flac_stat(f1, &s1) == 0 && flac_stat(f2, &s2) == 0 && s1.st_ino == s2.st_ino && s1.st_dev == s2.st_dev;
#endif
}

FLAC__bool grabbag__file_remove_file(const char *filename)
{
	return grabbag__file_change_stats(filename, /*read_only=*/false) && 0 == flac_unlink(filename);
}

FILE *grabbag__file_get_binary_stdin(void)
{
	/* if something breaks here it is probably due to the presence or
	 * absence of an underscore before the identifiers 'setmode',
	 * 'fileno', and/or 'O_BINARY'; check your system header files.
	 */
#if defined _MSC_VER || defined __MINGW32__
	_setmode(_fileno(stdin), _O_BINARY);
#elif defined __EMX__
	setmode(fileno(stdin), O_BINARY);
#endif

	return stdin;
}

FILE *grabbag__file_get_binary_stdout(void)
{
	/* if something breaks here it is probably due to the presence or
	 * absence of an underscore before the identifiers 'setmode',
	 * 'fileno', and/or 'O_BINARY'; check your system header files.
	 */
#if defined _MSC_VER || defined __MINGW32__
	_setmode(_fileno(stdout), _O_BINARY);
#elif defined __EMX__
	setmode(fileno(stdout), O_BINARY);
#endif

	return stdout;
}

void grabbag__file_input_reader_open(grabbag__file_input_reader *input_reader, FILE *stream)
{
	memset(input_reader, 0, sizeof(*input_reader));
	input_reader->stream = stream;
	input_reader->buf = NULL;
	input_reader->pos = 0;
	input_reader->size = 0;
	input_reader->maxsize = 0;
	input_reader->surrogate_buffer = 0;
	input_reader->eof = 0;
	input_reader->error = 0;
}

void grabbag__file_input_reader_close(grabbag__file_input_reader *input_reader)
{
	input_reader->stream = NULL;
	free(input_reader->buf);
	input_reader->buf = NULL;
	input_reader->pos = 0;
	input_reader->size = 0;
	input_reader->maxsize = 0;
	input_reader->surrogate_buffer = 0;
	input_reader->eof = 0;
	input_reader->error = 0;
}

#if defined _WIN32 && !defined __CYGWIN__
static inline FLAC__bool is_high_surrogate(const wchar_t wc)
{
	return ((unsigned)wc >= (unsigned)HIGH_SURROGATE_START) &&
		   ((unsigned)wc <= (unsigned)HIGH_SURROGATE_END);
}
#endif

#if defined _WIN32 && !defined __CYGWIN__
static FLAC__bool console_cp_stdin_to_utf8(const char *from, char **to)
{
	wchar_t *unicode = NULL;
	char *utf8 = NULL;
	FLAC__bool ret = 0;
	/* Don't use CP_ACP or CP_OEMCP here since the actual console codepage in use can be different */
	const UINT console_cp_stdin = GetConsoleCP();

	do {
		int len;

		len = MultiByteToWideChar(console_cp_stdin, 0, from, -1, NULL, 0);
		if(len == 0)
			break;
		unicode = (wchar_t *)malloc((size_t)len * sizeof(wchar_t));
		if(unicode == NULL)
			break;
		len = MultiByteToWideChar(console_cp_stdin, 0, from, -1, unicode, len);
		if(len == 0)
			break;

		len = WideCharToMultiByte(CP_UTF8, 0, unicode, -1, NULL, 0, NULL, NULL);
		if(len == 0)
			break;
		utf8 = (char *)malloc((size_t)len * sizeof(char));
		if(utf8 == NULL)
			break;
		len = WideCharToMultiByte(CP_UTF8, 0, unicode, -1, utf8, len, NULL, NULL);
		if(len == 0)
			break;

		ret = 1;

	} while(0);

	free(unicode);

	if(ret == 1) {
		*to = utf8;
	}
	else {
		free(utf8);
		*to = NULL;
	}
	return ret;
}
#endif

static size_t grabbag__file_input_reader_refill(grabbag__file_input_reader *input_reader)
{
#if defined _WIN32 && !defined __CYGWIN__
	HANDLE handle;

	/* Convert from standard stream into handle */
	if(input_reader->stream == stdin) {
		handle = GetStdHandle(STD_INPUT_HANDLE);
	}
	else {
		handle = INVALID_HANDLE_VALUE;
	}

	if(input_reader->buf == NULL) {
		input_reader->maxsize = 65536U;
		input_reader->buf = malloc(input_reader->maxsize);
		if(input_reader->buf == NULL) {
			return 0;
		}
	}

	/* Move remaining part of buffer to the beginning of the buffer */
	{
		size_t bytes_left = input_reader->size - input_reader->pos;
		if(bytes_left > 0) {
			memmove(&input_reader->buf[0], &input_reader->buf[input_reader->pos], bytes_left);
		}
		input_reader->pos = 0;
		input_reader->size = bytes_left;
	}

	if((handle != INVALID_HANDLE_VALUE) && (handle != NULL)) {
		const DWORD fileType = GetFileType(handle);
		if(fileType == FILE_TYPE_CHAR) {
			/* Reading from the console, must use UTF-16 */
			wchar_t wstr[32768];
			size_t bytes_left = input_reader->maxsize - input_reader->size;
			DWORD wchars_to_read = (bytes_left / 2);
			DWORD wchars_read = 0;
			size_t wpos = 0;

			/* Put back buffered surrogate */
			if(input_reader->surrogate_buffer != 0) {
				wstr[wpos++] = input_reader->surrogate_buffer;
				input_reader->surrogate_buffer = 0;
				if(wchars_to_read > 0) {
					--wchars_to_read;
				}
			}

			if(ReadConsoleW(handle, &wstr[wpos], wchars_to_read, &wchars_read, NULL)) {
				int utf8_size = 0;
				wchars_read += wpos;

				/* Check if start of surrogate pair, then back up one character to avoid splitting it. */
				if((wchars_read > 0) && is_high_surrogate(wstr[wchars_read - 1])) {
					input_reader->surrogate_buffer = wstr[wchars_read - 1];
					--wchars_read;
				}

				utf8_size = WideCharToMultiByte(CP_UTF8, 0, wstr, wchars_read, &input_reader->buf[input_reader->size], bytes_left, NULL, NULL);
				if(utf8_size == 0) {
					/* failed */
					input_reader->eof = true;
				}
				input_reader->size += utf8_size;
				return utf8_size;
			}
			else
			{
				return 0;
			}
		}
		else if((fileType == FILE_TYPE_PIPE) && (!input_reader->force_utf8)) {
			/* Redirect from pipe, convert from current console input codepage. */
			char buffer[65537];
			DWORD chars_to_read = (DWORD)(sizeof(buffer) - 1);
			DWORD chars_read = 0;

			if(ReadFile(handle, &buffer[0], chars_to_read, &chars_read, NULL) != 0) {
				size_t i;
				char *utf8_str = NULL;
				buffer[chars_read] = '\0';
				if (console_cp_stdin_to_utf8(&buffer[0], &utf8_str) == 1) {
					size_t utf8_size = strlen(utf8_str);
					memcpy(&input_reader->buf[input_reader->size], utf8_str, utf8_size);
					input_reader->size += utf8_size;
					free(utf8_str);
					return utf8_size;
				}
				else
				{
					/* fail */
				}

				free(utf8_str);
			}
		}
		else {
			/* Redirect from file, assume it is already UTF-8. */
			size_t bytes_left = input_reader->maxsize - input_reader->size;
			DWORD chars_read = 0;

			if(ReadFile(handle, &input_reader->buf[input_reader->size], (DWORD)bytes_left, &chars_read, NULL) != 0) {
				input_reader->size += chars_read;
				return chars_read;
			}
		}
	} 
	else {
		/* regular file on disk */
		size_t bytes_left = input_reader->maxsize - input_reader->size;
		size_t chars_read = fread(&input_reader->buf[input_reader->size], 1, bytes_left, input_reader->stream);
		input_reader->size += chars_read;
		return chars_read;	
	}
#endif
	return 0;
}

FLAC__bool grabbag__file_input_reader_next_line(grabbag__file_input_reader *input_reader, char **line)
{
	if(input_reader->eof) {
		return false;
	}

	do {
		const size_t start_pos = input_reader->pos;
		size_t pos = input_reader->pos;
		size_t end = 0;

		while(pos < input_reader->size) {
			/* accept any of \r \n \r\n \n\r as the line ending */
			const char c = input_reader->buf[pos];
			if((c == '\r') || (c == '\n')) {
				input_reader->buf[pos] = '\0';
				++pos;
				if((pos < input_reader->size) && (c != input_reader->buf[pos]) && ((input_reader->buf[pos] == '\r') || (input_reader->buf[pos] == '\n'))) {
					++pos;
				}
				*line = &input_reader->buf[input_reader->pos];
				input_reader->pos = pos;
				return true;
			}

			++pos;
		}

		/* No newline found, try to read more data into buffer */
		if(grabbag__file_input_reader_refill(input_reader) == 0) {
			/* No more data, end of stream */
			input_reader->eof = true;

			if((input_reader->pos < input_reader->size) && (input_reader->size < input_reader->maxsize)) {
				/* Last line didn't have ending newline, accept this anyway */
				input_reader->buf[input_reader->size] = '\0';
				*line = &input_reader->buf[input_reader->pos];
				input_reader->pos = pos;
				return true;
			}
			else {
				/* Line too long */
				return false;
			}
		}

	} while(true);

	return false;
}
