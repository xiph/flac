/* flac - Command-line FLAC encoder/decoder
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

#ifdef _MSC_VER
#include <sys/utime.h> /* for utime() */
#include <io.h> /* for chmod() */
#else
#include <utime.h> /* for utime() */
#include <unistd.h> /* for chown() */
#endif
#include <sys/stat.h> /* for stat() */
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
#ifndef _MSC_VER
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
