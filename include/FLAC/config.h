/* libFLAC - Free Lossless Audio Codec library
 * Copyright (C) 2001  Josh Coalson
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

#ifndef FLAC__CONFIG_H
#define FLAC__CONFIG_H

/* define the correctly-capitalized symbols from the autoconf-crippled ones */

#ifdef FLaC__ALIGN_MALLOC_DATA
#define FLAC__ALIGN_MALLOC_DATA
#endif

#ifdef FLaC__NO_ASM
#define FLAC__NO_ASM
#endif

#ifdef FLaC__HAS_NASM
#define FLAC__HAS_NASM
#endif

#ifdef FLaC__HAS_XMMS
#define FLAC__HAS_XMMS
#endif

#ifdef FLaC__CPU_IA32
#define FLAC__CPU_IA32
#endif

#ifdef FLaC__CPU_PPC
#define FLAC__CPU_PPC
#endif

#ifdef FLaC__CPU_SPARC
#define FLAC__CPU_SPARC
#endif

#endif
