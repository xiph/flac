/* libFLAC - Free Lossless Audio Codec library
 * Copyright (C) 2000,2001  Josh Coalson
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

#ifndef FLAC__ORDINALS_H
#define FLAC__ORDINALS_H

#ifdef bool
#undef bool
#endif
#ifdef true
#undef true
#endif
#ifdef false
#undef false
#endif
#ifdef int8
#undef int8
#endif
#ifdef uint8
#undef uint8
#endif
#ifdef int16
#undef int16
#endif
#ifdef uint16
#undef uint16
#endif
#ifdef int32
#undef int32
#endif
#ifdef uint32
#undef uint32
#endif
#ifdef int64
#undef int64
#endif
#ifdef uint64
#undef uint64
#endif
#ifdef byte
#undef byte
#endif
#ifdef real
#undef real
#endif

#define true 1
#define false 0

typedef int bool;
typedef signed char int8;
typedef unsigned char uint8;
typedef signed short int16;
typedef unsigned short uint16;
typedef signed int int32;
typedef unsigned int uint32;
#if defined _WIN32 && !defined __CYGWIN__
typedef __int64 int64;
typedef unsigned __int64 uint64;
#else
typedef long long int int64;
typedef unsigned long long uint64;
#endif
typedef uint8 byte;
typedef float real;

#endif
