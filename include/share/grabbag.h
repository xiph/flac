/* grabbag - Convenience lib for various routines common to several tools
 * Copyright (C) 2002  Josh Coalson
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

#ifndef SHARE__GRABBAG_H
#define SHARE__GRABBAG_H

#if defined(FLAC__NO_DLL) || defined(unix) || defined(__CYGWIN__) || defined(__CYGWIN32__)
#define GRABBAG_API

#else

#ifdef GRABBAG_API_EXPORTS
#define	GRABBAG_API	_declspec(dllexport)
#else
#define GRABBAG_API	_declspec(dllimport)
#define __LIBNAME__ "grabbag.lib"
#pragma comment(lib, __LIBNAME__)
#undef __LIBNAME__

#endif
#endif

/* These can't be included by themselves, only from within grabbag.h */
#include "grabbag/cuesheet.h"
#include "grabbag/file.h"
#include "grabbag/replaygain.h"
#include "grabbag/seektable.h"

#endif
