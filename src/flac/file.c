/* flac - Command-line FLAC encoder/decoder
 * Copyright (C) 2001,2002  Josh Coalson
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

#if defined _MSC_VER || defined __MINGW32__
#include <sys/utime.h> /* for utime() */
#include <io.h> /* for chmod(), _setmode() */
#include <fcntl.h> /* for _O_BINARY */
#else
#include <sys/types.h> /* some flavors of BSD (like OS X) require this to get time_t */
#include <utime.h> /* for utime() */
#include <unistd.h> /* for chown() */
#endif
#ifdef __CYGWIN__
#include <io.h> /* for _setmode(), O_BINARY */
#endif
#include <sys/stat.h> /* for stat(), maybe chmod() */
#include <string.h> /* for strrchr() */
#include "file.h"

void flac__file_copy_metadata(const char *srcpath, const char *destpath)
{
	struct stat srcstat;
	struct utimbuf srctime;

	if(0 == stat(srcpath, &srcstat)) {
		srctime.actime = srcstat.st_atime;
		srctime.modtime = srcstat.st_mtime;
		(void)chmod(destpath, srcstat.st_mode);
		(void)utime(destpath, &srctime);
#if !defined _MSC_VER && !defined __MINGW32__
		(void)chown(destpath, srcstat.st_uid, -1);
		(void)chown(destpath, -1, srcstat.st_gid);
#endif
	}
}

off_t flac__file_get_filesize(const char *srcpath)
{
	struct stat srcstat;

	if(0 == stat(srcpath, &srcstat))
		return srcstat.st_size;
	else
		return -1;
}

const char *flac__file_get_basename(const char *srcpath)
{
	const char *p;

	p = strrchr(srcpath, '\\');
	if(0 == p) {
		p = strrchr(srcpath, '/');
		if(0 == p)
			return srcpath;
	}
	return ++p;
}

FILE *file__get_binary_stdin()
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
#endif

	return stdin;
}

FILE *file__get_binary_stdout()
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
#endif

	return stdout;
}
