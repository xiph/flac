/* libOggFLAC++ - Free Lossless Audio Codec + Ogg library
 * Copyright (C) 2002  Josh Coalson
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

#ifndef OggFLACPP__EXPORT_H
#define OggFLACPP__EXPORT_H

#if defined(unix) || defined(__CYGWIN__) || defined(__CYGWIN32__)
#define OggFLACPP_API

#else

#ifdef OggFLACPP_API_EXPORTS
#define	OggFLACPP_API	_declspec(dllexport)
#else
#define OggFLACPP_API	_declspec(dllimport)
#define __LIBNAME__ "libOggFLAC++.lib"
#pragma comment(lib, __LIBNAME__)
#undef __LIBNAME__

#endif
#endif
#endif
