/* libFLAC - Free Lossless Audio Codec library
 * Copyright (C) 2000,2001,2002,2003  Josh Coalson
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA  02111-1307, USA.
 */

#ifndef FLAC__EXPORT_H
#define FLAC__EXPORT_H

#if defined(FLAC__NO_DLL) || !defined(_MSC_VER)
#define FLAC_API

#else

#ifdef FLAC_API_EXPORTS
#define	FLAC_API	_declspec(dllexport)
#else
#define FLAC_API	_declspec(dllimport)
#define __LIBNAME__ "libFLAC.lib"
#pragma comment(lib, __LIBNAME__)
#undef __LIBNAME__

#endif
#endif
#endif
