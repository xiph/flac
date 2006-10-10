/* grabbag - Convenience lib for various routines common to several tools
 * Copyright (C) 2002,2003,2004,2005,2006  Josh Coalson
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

#if HAVE_CONFIG_H
#  include <config.h>
#endif

#if defined _MSC_VER || defined __MINGW32__
#include <sys/utime.h> /* for utime() */
#include <io.h> /* for chmod(), _setmode(), unlink() */
#include <fcntl.h> /* for _O_BINARY */
#else
#include <sys/types.h> /* some flavors of BSD (like OS X) require this to get time_t */
#include <utime.h> /* for utime() */
#endif
#if defined __CYGWIN__ || defined __EMX__
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
#include "share/grabbag.h"


void grabbag__file_copy_metadata(const char *srcpath, const char *destpath)
{
	struct stat srcstat;
	struct utimbuf srctime;

	if(0 == stat(srcpath, &srcstat)) {
		srctime.actime = srcstat.st_atime;
		srctime.modtime = srcstat.st_mtime;
		(void)chmod(destpath, srcstat.st_mode);
		(void)utime(destpath, &srctime);
	}
}

off_t grabbag__file_get_filesize(const char *srcpath)
{
	struct stat srcstat;

	if(0 == stat(srcpath, &srcstat))
		return srcstat.st_size;
	else
		return -1;
}

const char *grabbag__file_get_basename(const char *srcpath)
{
	const char *p;

	p = strrchr(srcpath, '/');
	if(0 == p) {
		p = strrchr(srcpath, '\\');
		if(0 == p)
			return srcpath;
	}
	return ++p;
}

FLAC__bool grabbag__file_change_stats(const char *filename, FLAC__bool read_only)
{
	struct stat stats;

	if(0 == stat(filename, &stats)) {
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
		if(0 != chmod(filename, stats.st_mode))
			return false;
	}
	else
		return false;

	return true;
}

FLAC__bool grabbag__file_are_same(const char *f1, const char *f2)
{
	struct stat s1, s2;
#if defined _MSC_VER || defined __MINGW32__
	return f1 && f2 && 0 == strcmp(s1, s2); /*@@@@@@ need better method than strcmp */
#else
	return f1 && f2 && stat(f1, &s1) == 0 && stat(f2, &s2) == 0 && s1.st_ino == s2.st_ino;
#endif
}

FLAC__bool grabbag__file_remove_file(const char *filename)
{
	return grabbag__file_change_stats(filename, /*read_only=*/false) && 0 == unlink(filename);
}

FILE *grabbag__file_get_binary_stdin()
{
	/* if something breaks here it is probably due to the presence or
	 * absence of an underscore before the identifiers 'setmode',
	 * 'fileno', and/or 'O_BINARY'; check your system header files.
	 */
#if defined _MSC_VER || defined __MINGW32__
	_setmode(_fileno(stdin), _O_BINARY);
#elif defined __CYGWIN__
	/* almost certainly not needed for any modern Cygwin, but let's be safe... */
	setmode(_fileno(stdin), _O_BINARY);
#elif defined __EMX__
	setmode(fileno(stdin), O_BINARY);
#endif

	return stdin;
}

FILE *grabbag__file_get_binary_stdout()
{
	/* if something breaks here it is probably due to the presence or
	 * absence of an underscore before the identifiers 'setmode',
	 * 'fileno', and/or 'O_BINARY'; check your system header files.
	 */
#if defined _MSC_VER || defined __MINGW32__
	_setmode(_fileno(stdout), _O_BINARY);
#elif defined __CYGWIN__
	/* almost certainly not needed for any modern Cygwin, but let's be safe... */
	setmode(_fileno(stdout), _O_BINARY);
#elif defined __EMX__
	setmode(fileno(stdout), O_BINARY);
#endif

	return stdout;
}
